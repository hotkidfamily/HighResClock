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

bool d2dtext::Init(HWND hwnd)
{
    _hwnd = hwnd;
    GetClientRect(_hwnd, &_initRc);

    return true;
}

bool d2dtext::Config(bool invalue)
{
    _show_render_fps = invalue;

    return true;
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
    Microsoft::WRL::ComPtr<ID2D1Factory> _factory;
    Microsoft::WRL::ComPtr<IDWriteFactory> _writerFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> _clockFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> _dbgFormat;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> _target;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _clockBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _dbgBrush;
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
        
        CRect rc;
        GetClientRect(_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.Width(), rc.Height());

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

        float scalex = rc.Width() * 1.0f / _initRc.Width();
        float scaley = rc.Height() * 1.0f / _initRc.Height();
        float scale = (std::min)(scalex, scaley);

        hr = _writerFactory->CreateTextFormat(L"Monaco", // Font family name
                                              NULL,      // Font collection(NULL sets it to the system font collection)
                                              DWRITE_FONT_WEIGHT_REGULAR, // Weight
                                              DWRITE_FONT_STYLE_NORMAL,   // Style
                                              DWRITE_FONT_STRETCH_NORMAL, // Stretch
                                              50.0f * scale,              // Size
                                              L"en-us",                   // Local
                                              &_clockFormat               // Pointer to recieve the created object
        );
        if (FAILED(hr))
        {
            break;
        }

        hr = _writerFactory->CreateTextFormat(L"Monaco", // Font family name
                                              NULL,      // Font collection(NULL sets it to the system font collection)
                                              DWRITE_FONT_WEIGHT_REGULAR, // Weight
                                              DWRITE_FONT_STYLE_ITALIC,   // Style
                                              DWRITE_FONT_STRETCH_NORMAL, // Stretch
                                              25.0f,                      // Size
                                              L"en-us",                   // Local
                                              &_dbgFormat                 // Pointer to recieve the created object
        );
        if (FAILED(hr))
        {
            break;
        }

        {
            Microsoft::WRL::ComPtr<IDWriteTextLayout> pTextLayout = nullptr;
            std::wstring msg = L"1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            hr = _writerFactory->CreateTextLayout(msg.c_str(), msg.size(), _dbgFormat.Get(), 1000.0f, 1000.0f,
                                                  &pTextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS textMetrics;
                hr = pTextLayout->GetMetrics(&textMetrics);
                if (SUCCEEDED(hr))
                {
                    dbgFontHeight = textMetrics.height;
                }
            }
        }
        {
            Microsoft::WRL::ComPtr<IDWriteTextLayout> pTextLayout = nullptr;
            std::wstring msg = L"1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            hr = _writerFactory->CreateTextLayout(msg.c_str(), msg.size(), _clockFormat.Get(), 1000.0f, 1000.0f,
                                                  &pTextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS textMetrics;
                hr = pTextLayout->GetMetrics(&textMetrics);
                if (SUCCEEDED(hr))
                {
                    clockFontHeight = textMetrics.height;
                }
            }
        }

    } while (0);

    if (!FAILED(hr))
    {
        D2D1_SIZE_F rtSize = _target->GetSize();
        auto clockRect = D2D1::RectF(0, (rtSize.height - clockFontHeight)/2, rtSize.width, rtSize.height);
        auto dbgRect = D2D1::RectF(0, rtSize.height - dbgFontHeight, rtSize.width, rtSize.height);

        std::wstringstream wss;
        std::wstringstream dbgWss;

        int count = 0;
        auto start = std::chrono::high_resolution_clock::now();
        while (!_stop)
        {
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

                    dbgWss << std::fixed << std::setprecision(2) << "f/l/h:" << _statics->fps() << "/" << min << "/"
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