#include "pch.h"
#include "d2dtext.h"

#include <functional>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <iomanip>

#include "SlidingWindow.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")

using namespace Microsoft::WRL;

bool d2dtext::Init(HWND hwnd)
{
    _hwnd = hwnd;
    CRect r;
    GetClientRect(_hwnd, &r);
    _initSz = { r.Width(), r.Height() };
    _UpdateSz = _initSz;

    return true;
}

bool d2dtext::Config(bool invalue)
{
    _show_render_fps = invalue;

    return true;
}

bool d2dtext::Resize(SIZE v)
{
    _UpdateSz = v;
    return false;
}

bool d2dtext::Start()
{
    End();

    _stop = false;
    _thread = std::move(std::thread(std::bind(&d2dtext::_run, this)));

    return true;
}

bool d2dtext::End()
{
    if (_thread.joinable())
    {
        _stop = true;
        _thread.join();
    }
    return true;
}

bool d2dtext::_run()
{
    HRESULT hr = E_FAIL;
    ComPtr<ID2D1Factory> _factory;
    ComPtr<IDWriteFactory> _writerFactory;
    ComPtr<IDWriteTextFormat> _clockFormat;
    ComPtr<IDWriteTextFormat> _dbgFormat;
    ComPtr<ID2D1HwndRenderTarget> _target;
    ComPtr<ID2D1SolidColorBrush> _clockBrush;
    ComPtr<ID2D1SolidColorBrush> _dbgBrush;
    float dbgFontHeight = NAN;
    float clockFontHeight = NAN;
    std::unique_ptr<utils::DurationSlidingWindow> _statics = std::make_unique<utils::DurationSlidingWindow>(2000);

    do
    {
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &_factory);
        if (FAILED(hr))
        {
            break;
        }

        // Create DirectWrite Factory
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &_writerFactory);
        if (FAILED(hr))
        {
            break;
        }

        D2D1_SIZE_U size = D2D1::SizeU(_initSz.cx, _initSz.cy);
        hr = _factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                              D2D1::HwndRenderTargetProperties(_hwnd, size), &_target);
        if (FAILED(hr))
        {
            break;
        }

        hr = _target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightCyan), &_clockBrush);
        if (FAILED(hr))
        {
            break;
        }

        hr = _target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightYellow), &_dbgBrush);
        if (FAILED(hr))
        {
            break;
        }

        hr = _createFont(_writerFactory.Get(), _clockFormat, clockFontHeight, 72);
        hr = _createFont(_writerFactory.Get(), _dbgFormat, dbgFontHeight, 28);

    } while (0);

    if (!FAILED(hr))
    {
        std::wstringstream wss;
        std::wstringstream dbgWss;

        int count = 0;
        auto start = std::chrono::high_resolution_clock::now();
        while (!_stop)
        {
            if (_UpdateSz.cx != 0 && _UpdateSz.cy != 0)
            {
                D2D1_SIZE_U rsz = { _UpdateSz.cx, _UpdateSz.cy };
                _target->Resize(rsz);

                float scalex = _UpdateSz.cx * 1.0f / _initSz.cx;
                float scaley = _UpdateSz.cy * 1.0f / _initSz.cy;
                float scale = (std::min)(scalex, scaley);

                if (scale > abs(scale - 1.0f) > 0.2f)
                {
                    int fs = 72 * scale;
                    hr = _createFont(_writerFactory.Get(), _clockFormat, clockFontHeight, fs);
                }

                _UpdateSz = { 0, 0 };
            }
            D2D1_SIZE_F rtSize = _target->GetSize();
            auto clockRect = D2D1::RectF(0, (rtSize.height - clockFontHeight) / 2, rtSize.width, rtSize.height);
            auto dbgRect = D2D1::RectF(0, rtSize.height - dbgFontHeight, rtSize.width, rtSize.height);

            wss.str(L"");

            _target->BeginDraw();
            _target->SetTransform(D2D1::Matrix3x2F::Identity());
            _target->Clear(D2D1::ColorF(D2D1::ColorF(0.4f, 0.4f, 0.4f)));

            count++;

            {
                auto now = std::chrono::system_clock::now();
                time_t tt = std::chrono::system_clock::to_time_t(now);
                uint64_t millseconds
                    = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
                    - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
                tm lt;
                localtime_s(&lt, &tt);
                wchar_t strTime[25] = { 0 };
                swprintf_s(strTime, 25, L"%02d:%02d:%02d.%03d", lt.tm_hour, lt.tm_min, lt.tm_sec, (int)millseconds);
                wss << strTime;
            }

            _statics->append();

            auto clockMsg = wss.str();

            _target->DrawTextW(clockMsg.c_str(),   // Text to render
                               clockMsg.size(),    // Text length
                               _clockFormat.Get(), // Text format
                               clockRect,          // The region of the window where the text will be rendered
                               _clockBrush.Get(),  // The brush used to draw the text
                               D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);

            if (_show_render_fps)
            {
                auto end = std::chrono::high_resolution_clock::now();

                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                if (duration > 1000)
                {
                    start = end;
                    uint64_t min, max;
                    dbgWss.str(L"");

                    _statics->minMax(min, max);

                    dbgWss << std::fixed << std::setprecision(2) << "f/l/h:" << _statics->fps() << "/" << min << "~"
                           << max;
                }
                auto dbgMsg = dbgWss.str();
                _target->DrawTextW(dbgMsg.c_str(),   // Text to render
                                   dbgMsg.size(),    // Text length
                                   _dbgFormat.Get(), // Text format
                                   dbgRect,          // The region of the window where the text will be rendered
                                   _dbgBrush.Get(),  // The brush used to draw the text
                                   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
            }

            hr = _target->EndDraw();

            if (hr == D2DERR_RECREATE_TARGET)
            {
                hr = S_OK;
            }
        }
    }

    return true;
}

HRESULT d2dtext::_createFont(IDWriteFactory *factory,
                             ComPtr<IDWriteTextFormat> &format,
                             float &fontHeight,
                             int fontSize)
{
    HRESULT hr;
    ComPtr<IDWriteTextFormat> reqFormat;
    float height = NAN;
    do
    {
        hr = factory->CreateTextFormat(L"Monaco", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
                                       DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &reqFormat);
        if (FAILED(hr))
        {
            break;
        }

        ComPtr<IDWriteTextLayout> pTextLayout = nullptr;
        std::wstring msg = L"1234567890:";
        hr = factory->CreateTextLayout(msg.c_str(), msg.size(), reqFormat.Get(), 1000.0f, 1000.0f, &pTextLayout);

        if (SUCCEEDED(hr))
        {
            DWRITE_TEXT_METRICS textMetrics;
            hr = pTextLayout->GetMetrics(&textMetrics);
            if (SUCCEEDED(hr))
            {
                height = textMetrics.height;
            }
        }
    } while (0);

    if (SUCCEEDED(hr))
    {
        format = reqFormat;
        fontHeight = height;
    }

    return hr;
}
