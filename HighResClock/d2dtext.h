#pragma once

#define NOMINMAX

#include <wrl/client.h>
#include <d2d1.h>
#include <dwrite.h>

#include <thread>

class d2dtext {
  public:
    d2dtext()
    {
    }

    ~d2dtext()
    {
        End();
    }

    bool Init(HWND v);
    bool Config(bool);

    bool Resize(SIZE);

    bool Start();
    bool End();

  protected:
    bool _run();
    HRESULT _createFont(IDWriteFactory *factory,
                        Microsoft::WRL::ComPtr<IDWriteTextFormat> &format,
                        float &fontHeight,
                        int fontSize);

  private:
    std::thread _thread;
    bool _stop = true;
    bool _show_render_fps = false;

    HWND _hwnd;
    SIZE _initSz{};
    SIZE _UpdateSz{};
};
