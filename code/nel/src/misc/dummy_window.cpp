// NeL - MMORPG Framework <http://dev.ryzom.com/projects/nel/>
// Copyright (C) 2010  Winch Gate Property Limited
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "stdmisc.h"
#include "nel/misc/dummy_window.h"


#ifdef NL_OS_WINDOWS

namespace NLMISC
{


static LRESULT CALLBACK nlDefaultWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


//***************************************************************
CDummyWindow::CDummyWindow() : _HWnd(NULL)
{
}

//***************************************************************
bool CDummyWindow::init(HINSTANCE hInstance, WNDPROC winProc)
{
	release();
	static const char *INVISIBLE_WINDOW_CLASS = "nl_invisible_wnd_class";
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	if (!GetClassInfoEx(hInstance, INVISIBLE_WINDOW_CLASS, &wc))
	{
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc		= nlDefaultWinProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= hInstance;
		wc.hIcon			= 0;
		wc.hCursor			= 0;
		wc.hbrBackground	= 0;
		wc.lpszMenuName		= 0;
		wc.lpszClassName	= INVISIBLE_WINDOW_CLASS;
		wc.hIconSm			= 0;
		RegisterClassEx(&wc);
	}
    _HWnd = CreateWindow(INVISIBLE_WINDOW_CLASS, "", WS_POPUP,
                         CW_USEDEFAULT,CW_USEDEFAULT,
                         CW_USEDEFAULT,CW_USEDEFAULT,
                         NULL, 0,
                         hInstance, 0);
	if (_HWnd)
	{
		if (winProc) SetWindowLongPtr(_HWnd, GWLP_WNDPROC, (LONG_PTR) winProc);
		return true;
	}
	return false;
}

//***************************************************************
void CDummyWindow::release()
{
	if (_HWnd)
	{
		DestroyWindow(_HWnd);
		_HWnd = NULL;
	}
}

//***************************************************************
CDummyWindow::~CDummyWindow()
{
	release();
}

} // NLMISC

#endif // NL_OS_WINDOWS
