
// IntroMovie.cpp
// The intro movie that plays at startup.
//
// Copyright (c) 2010 Michael Imamura.
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.

#include "StdAfx.h"

#ifdef WITH_DIRECTSHOW
#	include <comdef.h>
#	include <dshow.h>
#	pragma comment(lib, "Strmiids.lib")
#else
#	include <Vfw.h>
#	pragma comment(lib, "vfw32.lib")
#endif

#include "../../engine/Util/Config.h"
#include "../../engine/Util/Str.h"

#include "IntroMovie.h"

using namespace HoverRace;
using namespace HoverRace::Client;
using namespace HoverRace::Util;

IntroMovie::IntroMovie(HWND hwnd, HINSTANCE hinst) :
	hwnd(hwnd),
#ifdef WITH_DIRECTSHOW
	graph(NULL), winCtl(NULL), mediaCtl(NULL)
#else
	movieWnd(NULL)
#endif
{
	Config *cfg = Config::GetInstance();
	OS::path_t movieFilename = cfg->GetMediaPath("Intro.avi");

	// ensure that movie path exists
	if(!exists(movieFilename))
		return; // poor user gets no movie
		// maybe we should throw an exception instead of failing silently?

#ifdef WITH_DIRECTSHOW
	HRESULT hr;
	if (FAILED(hr = InitDirectShow(movieFilename))) {
		Clean();
	}

#else

	movieWnd = MCIWndCreateW(
		hwnd, hinst, 
		WS_CHILD | MCIWNDF_NOMENU | MCIWNDF_NOPLAYBAR, 
		movieFilename.string().c_str());

	// Fill the client area.
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	MoveWindow(movieWnd, 0, 0,
		clientRect.right - clientRect.left,
		clientRect.bottom - clientRect.top,
		TRUE);
#endif
}

IntroMovie::~IntroMovie()
{
	Clean();
}

void IntroMovie::Clean()
{
#ifdef WITH_DIRECTSHOW
	if (mediaCtl != NULL)
		mediaCtl->Release();
	if (winCtl != NULL)
		winCtl->Release();
	if (graph != NULL)
		graph->Release();
#else
	if (movieWnd != NULL) {
		MCIWndClose(movieWnd);
		Sleep(1000);  // Is this necessary?
		MCIWndDestroy(movieWnd);
	}
#endif
}

#ifdef WITH_DIRECTSHOW
HRESULT IntroMovie::InitDirectShow(const Util::OS::path_t &movieFilename)
{
	HRESULT hr;

	if (FAILED(hr = CoCreateInstance(CLSID_FilterGraph, NULL,
		CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
		(void**)&graph))) return hr;

	_COM_SMARTPTR_TYPEDEF(IBaseFilter, __uuidof(IBaseFilter));
	IBaseFilterPtr vmr;
	if (FAILED(hr = CoCreateInstance(CLSID_VideoMixingRenderer, NULL, 
		CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&vmr))) return hr;
	if (FAILED(hr = graph->AddFilter(vmr, L"Video Mixing Renderer"))) return hr;

	_COM_SMARTPTR_TYPEDEF(IVMRFilterConfig, __uuidof(IVMRFilterConfig));
	IVMRFilterConfigPtr filterCfg;
	if (FAILED(hr = vmr->QueryInterface(IID_IVMRFilterConfig, (void**)&filterCfg))) return hr;
	if (FAILED(hr = filterCfg->SetRenderingMode(VMRMode_Windowless))) return hr;

	if (FAILED(hr = vmr->QueryInterface(IID_IVMRWindowlessControl, (void**)&winCtl))) return hr;

	if (FAILED(hr = winCtl->SetVideoClippingWindow(hwnd))) return hr;

	if (FAILED(hr = graph->RenderFile(movieFilename.string().c_str(), NULL))) return hr;

	if (FAILED(hr = graph->QueryInterface(IID_IMediaControl, (void**)&mediaCtl))) return hr;

	return ERROR_SUCCESS;
}
#endif

void IntroMovie::Play()
{
#ifdef WITH_DIRECTSHOW
	if (mediaCtl != NULL)
		mediaCtl->Run();
#else
	if (movieWnd != NULL)
		MCIWndPlay(movieWnd);
#endif
}

void IntroMovie::ResetSize()
{
	RECT clientRect, movieRect;

	if (GetClientRect(hwnd, &clientRect)) {
#		ifdef WITH_DIRECTSHOW
			HRESULT hr;
			long w, h;

			if (mediaCtl != NULL) {
				if (FAILED(hr = winCtl->GetNativeVideoSize(&w, &h, NULL, NULL))) {
					return;
				}
				SetRect(&movieRect, 0, 0, w, h);
				winCtl->SetVideoPosition(&movieRect, &clientRect);
			}
#		else
			if (GetWindowRect(movieWnd, &movieRect)) {
				SetWindowPos(movieWnd, HWND_TOP,
					0, 0,
					clientRect.right - clientRect.left,
					clientRect.bottom - clientRect.top,
					SWP_SHOWWINDOW);
			}
#		endif
	}
}

void IntroMovie::Repaint(HDC hdc)
{
#ifdef WITH_DIRECTSHOW
	if (winCtl != NULL)
		winCtl->RepaintVideo(hwnd, hdc);
#endif
}

void IntroMovie::ResetPalette(bool background)
{
#ifdef WITH_DIRECTSHOW
	if (winCtl != NULL)
		winCtl->DisplayModeChanged();
#else
	MCIWndRealize(movieWnd, background);
#endif
}
