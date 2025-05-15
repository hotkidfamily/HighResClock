#pragma once

#define NOMINMAX

#include <wrl/client.h>
#include <d2d1.h>
#include <dwrite.h>

#include <thread>

class d2dtext
{
public:
    d2dtext() {}

	~d2dtext()
    {
        End();
    }

	bool Init(HWND hwnd);
    bool Config(bool);

	bool Start();
	bool End();

protected:
	bool _run();
	
private:

	std::thread _thread;
	bool _stop = true;
    bool _show_render_fps = false;

	HWND _hwnd = nullptr;
    CRect _initRc{0};
};

