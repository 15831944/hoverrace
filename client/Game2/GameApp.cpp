// MR_GameApp.cpp
//
// Copyright (c) 1995-1998 - Richard Langlois and Grokksoft Inc.
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
//

#include "StdAfx.h"

#include <sstream>

#include "GameApp.h"
#include "AboutDialog.h"
#include "FirstChoiceDialog.h"
#include "FullscreenTest.h"
#include "Controller.h"
#include "resource.h"
#include "NetworkSession.h"
#include "PrefsDialog.h"
#include "HighConsole.h"
#include "HighObserver.h"
#include "TrackSelect.h"
#include "TrackDownloadDialog.h"
#include "../../engine/Util/DllObjectFactory.h"
#include "../../engine/VideoServices/ColorPalette.h"
#include "../../engine/VideoServices/VideoBuffer.h"
#include "../../engine/MainCharacter/MainCharacter.h"
#include "../../engine/Util/FuzzyLogic.h"
#include "../../engine/Util/Profiler.h"
#include "../../engine/Util/StrRes.h"
#include "../../engine/Util/Config.h"
#include "../../engine/Util/OS.h"
#include "../../engine/Util/Str.h"
#include "InternetRoom.h"
#include "CheckUpdateServerDialog.h"

#include <vfw.h>

using namespace HoverRace::Client;

// If MR_AVI_CAPTURE is defined
// #define MR_AVI_CAPTUREh

/*
#ifdef MR_AVI_CAPTURE
#include <Vfw.h>

const gCaptureFrameRate = 20; // nb frames sec (between 8-24 is good)

PAVIFILE         gCaptureFile           = NULL;
PAVISTREAM       gCaptureStream         = NULL;
int              gCaptureFrameNo        = 0;

void InitCapture( const char* pFileName, MR_VideoBuffer* pVideoBuffer );
void CloseCapture();
void CaptureScreen( MR_VideoBuffer* pVideoBuffer );

#endif
*/

// Constants for menu.
// Some of these are shared by accelerator keys in the resources.
#define ID_GAME_SPLITSCREEN             40003
#define ID_GAME_SPLITSCREEN3            40192
#define ID_GAME_SPLITSCREEN4            40193
#define ID_GAME_NETWORK_CONNECT         40005
#define ID_GAME_NETWORK_SERVER          40006
#define ID_GAME_NETWORK_INTERNET        40008
#define ID_GAME_NEW                     0xE100
#define ID_VIEW_3DACTION                32775
#define ID_VIEW_COCKPIT                 40002
#define ID_VIEW_PLAYERSLIST             40009
#define ID_VIEW_MOREMESSAGES            40010
#define ID_SETTING_REFRESHCOLORS        40013
#define ID_SETTING_FULLSCREEN           40046
#define ID_SETTING_WINDOW               32784
#define ID_SETTING_PROPERTIES           40048
#define ID_HELP_CONTENTS                32772
#define ID_HELP_SITE                    40007
#define ID_HELP_UPDATES					32773

#define MR_APP_CLASS_NAME L"HoverRaceClass"

#define MRM_RETURN2WINDOWMODE  1
#define MRM_EXIT_MENU_LOOP     2

#ifdef WITH_OPENAL
#	define SOUNDSERVER_INIT(s) MR_SoundServer::Init()
#else
#	define SOUNDSERVER_INIT(s) MR_SoundServer::Init(s)
#endif

using boost::format;
using boost::str;

using namespace HoverRace;
using namespace HoverRace::Util;

enum MR_InControler { MR_KDB, MR_JOY1, MR_JOY2, MR_JOY3, MR_JOY4 };

enum
{
	AxeTop,
	AxeBottom,
	AxeLeft,
	AxeRight,
	Btn1, Btn2, Btn3, Btn4, Btn5, Btn6, Btn7, Btn8
};

struct ControlKey
{
	const char *mOldKeyName;
	int mStringId;
	MR_InControler mControler;
	int mKeyValue;								  // Key or Joy Control
};

static const ControlKey KeyChoice[] =
{
	{	"Disable",			IDS_DISABLE,	MR_KDB,		0			},	// 0
	{	"Shift",			IDS_SHIFT,		MR_KDB,		VK_SHIFT	},	// 1
	{	"Ctrl",				IDS_CTRL,		MR_KDB,		VK_CONTROL	},	// 2
	{	"Up",				IDS_UP,			MR_KDB,		VK_UP		},	// 3
	{	"Down",				IDS_DOWN,		MR_KDB,		VK_DOWN		},	// 4
	{	"Right",			IDS_RIGHT,		MR_KDB,		VK_RIGHT	},	// 5
	{	"Left",				IDS_LEFT,		MR_KDB,		VK_LEFT		},	// 6
	{	"Insert",			IDS_INS,		MR_KDB,		VK_INSERT	},	// 7
	{	"Delete",			IDS_DEL,		MR_KDB,		VK_DELETE	},	// 8
	{	"Enter",			IDS_ENTER,		MR_KDB,		VK_RETURN	},	// 9
	{	"End",				IDS_END,		MR_KDB,		VK_END		},	// 10
	{	"Tab",				IDS_TAB,		MR_KDB,		VK_TAB		},	// 11
	{	"Joystick Btn1",	IDS_JOY1BTN1,	MR_JOY1,	Btn1		},	// 12
	{	"Joystick Btn2",	IDS_JOY1BTN2,	MR_JOY1,	Btn2		},	// 13
	{	"Joystick Btn3",	IDS_JOY1BTN3,	MR_JOY1,	Btn3		},	// 14
	{	"Joystick Btn4",	IDS_JOY1BTN4,	MR_JOY1,	Btn4		},	// 15
	{	"Joystick Btn5",	IDS_JOY1BTN5,	MR_JOY1,	Btn5		},	// 16
	{	"Joystick Btn6",	IDS_JOY1BTN6,	MR_JOY1,	Btn6		},	// 17
	{	"Joystick Btn7",	IDS_JOY1BTN7,	MR_JOY1,	Btn7		},	// 18
	{	"Joystick Btn8",	IDS_JOY1BTN8,	MR_JOY1,	Btn8		},	// 19
	{	"Joystick Top",		IDS_JOY1TOP,	MR_JOY1,	AxeTop		},	// 20
	{	"Joystick Bottom",	IDS_JOY1DOWN,	MR_JOY1,	AxeBottom	},	// 21
	{	"Joystick Right",	IDS_JOY1RIGHT,	MR_JOY1,	AxeRight	},	// 22
	{	"Joystick Left",	IDS_JOY1LEFT,	MR_JOY1,	AxeLeft		},	// 23
	{	"Joystick2 Btn1",	IDS_JOY2BTN1,	MR_JOY2,	Btn1		},	// 24
	{	"Joystick2 Btn2",	IDS_JOY2BTN2,	MR_JOY2,	Btn2		},	// 25
	{	"Joystick2 Btn3",	IDS_JOY2BTN3,	MR_JOY2,	Btn3		},	// 26
	{	"Joystick2 Btn4",	IDS_JOY2BTN4,	MR_JOY2,	Btn4		},	// 27
	{	"Joystick2 Btn5",	IDS_JOY2BTN5,	MR_JOY2,	Btn5		},	// 28
	{	"Joystick2 Btn6",	IDS_JOY2BTN6,	MR_JOY2,	Btn6		},	// 29
	{	"Joystick2 Btn7",	IDS_JOY2BTN7,	MR_JOY2,	Btn7		},	// 30
	{	"Joystick2 Btn8",	IDS_JOY2BTN8,	MR_JOY2,	Btn8		},	// 31
	{	"Joystick2 Top",	IDS_JOY2TOP,	MR_JOY2,	AxeTop		},	// 32
	{	"Joystick2 Bottom",	IDS_JOY2DOWN,	MR_JOY2,	AxeBottom	},	// 33
	{	"Joystick2 Right",	IDS_JOY2RIGHT,	MR_JOY2,	AxeRight	},	// 34
	{	"Joystick2 Left",	IDS_JOY2LEFT,	MR_JOY2,	AxeLeft		},	// 35
	{	"Joystick3 Btn1",	IDS_JOY3BTN1,	MR_JOY3,	Btn1		},	// 36
	{	"Joystick3 Btn2",	IDS_JOY3BTN2,	MR_JOY3,	Btn2		},	// 37
	{	"Joystick3 Btn3",	IDS_JOY3BTN3,	MR_JOY3,	Btn3		},	// 38
	{	"Joystick3 Btn4",	IDS_JOY3BTN4,	MR_JOY3,	Btn4		},	// 39
	{	"Joystick3 Btn5",	IDS_JOY3BTN5,	MR_JOY3,	Btn5		},	// 40
	{	"Joystick3 Btn6",	IDS_JOY3BTN6,	MR_JOY3,	Btn6		},	// 41
	{	"Joystick3 Btn7",	IDS_JOY3BTN7,	MR_JOY3,	Btn7		},	// 42
	{	"Joystick3 Btn8",	IDS_JOY3BTN8,	MR_JOY3,	Btn8		},	// 43
	{	"Joystick3 Top",	IDS_JOY3TOP,	MR_JOY3,	AxeTop		},	// 44
	{	"Joystick3 Bottom",	IDS_JOY3DOWN,	MR_JOY3,	AxeBottom	},	// 45
	{	"Joystick3 Right",	IDS_JOY3RIGHT,	MR_JOY3,	AxeRight	},	// 46
	{	"Joystick3 Left",	IDS_JOY3LEFT,	MR_JOY3,	AxeLeft		},	// 47
	{	"Joystick4 Btn1",	IDS_JOY4BTN1,	MR_JOY4,	Btn1		},	// 48
	{	"Joystick4 Btn2",	IDS_JOY4BTN2,	MR_JOY4,	Btn2		},	// 49
	{	"Joystick4 Btn3",	IDS_JOY4BTN3,	MR_JOY4,	Btn3		},	// 50
	{	"Joystick4 Btn4",	IDS_JOY4BTN4,	MR_JOY4,	Btn4		},	// 51
	{	"Joystick4 Btn5",	IDS_JOY4BTN5,	MR_JOY4,	Btn5		},	// 52
	{	"Joystick4 Btn6",	IDS_JOY4BTN6,	MR_JOY4,	Btn6		},	// 53
	{	"Joystick4 Btn7",	IDS_JOY4BTN7,	MR_JOY4,	Btn7		},	// 54
	{	"Joystick4 Btn8",	IDS_JOY4BTN8,	MR_JOY4,	Btn8		},	// 55
	{	"Joystick4 Top",	IDS_JOY4TOP,	MR_JOY4,	AxeTop		},	// 56
	{	"Joystick4 Bottom",	IDS_JOY4DOWN,	MR_JOY4,	AxeBottom	},	// 57
	{	"Joystick4 Right",	IDS_JOY4RIGHT,	MR_JOY4,	AxeRight	},	// 58
	{	"Joystick4 Left",	IDS_JOY4LEFT,	MR_JOY4,	AxeLeft		},	// 59
	{	"Space",			IDS_SPACE,		MR_KDB,		VK_SPACE	},  // 60, player 2 only from here
	{	"A Key",			IDS_KEY_A,		MR_KDB,		'A'			},	// 61
	{	"B Key",			IDS_KEY_B,		MR_KDB,		'B'			},	// 62
	{	"C Key",			IDS_KEY_C,		MR_KDB,		'C'			},	// 63
	{	"D Key",			IDS_KEY_D,		MR_KDB,		'D'			},	// 64
	{	"E Key",			IDS_KEY_E,		MR_KDB,		'E'			},	// 65
	{	"F Key",			IDS_KEY_F,		MR_KDB,		'F'			},	// 66
	{	"G Key",			IDS_KEY_G,		MR_KDB,		'G'			},	// 67
	{	"H Key",			IDS_KEY_H,		MR_KDB,		'H'			},	// 68
	{	"I Key",			IDS_KEY_I,		MR_KDB,		'I'			},	// 69
	{	"J Key",			IDS_KEY_J,		MR_KDB,		'J'			},	// 70
	{	"K Key",			IDS_KEY_K,		MR_KDB,		'K'			},	// 71
	{	"L Key",			IDS_KEY_L,		MR_KDB,		'L'			},	// 72
	{	"M Key",			IDS_KEY_M,		MR_KDB,		'M'			},	// 73
	{	"N Key",			IDS_KEY_N,		MR_KDB,		'N'			},	// 74
	{	"O Key",			IDS_KEY_O,		MR_KDB,		'O'			},	// 75
	{	"P Key",			IDS_KEY_P,		MR_KDB,		'P'			},	// 76
	{	"Q Key",			IDS_KEY_Q,		MR_KDB,		'Q'			},	// 77
	{	"R Key",			IDS_KEY_R,		MR_KDB,		'R'			},	// 78
	{	"S Key",			IDS_KEY_S,		MR_KDB,		'S'			},	// 79
	{	"T Key",			IDS_KEY_T,		MR_KDB,		'T'			},	// 80
	{	"U Key",			IDS_KEY_U,		MR_KDB,		'U'			},	// 81
	{	"V Key",			IDS_KEY_V,		MR_KDB,		'V'			},	// 82
	{	"W Key",			IDS_KEY_W,		MR_KDB,		'W'			},	// 83
	{	"X Key",			IDS_KEY_X,		MR_KDB,		'X'			},	// 84
	{	"Y Key",			IDS_KEY_Y,		MR_KDB,		'Y'			},	// 85
	{	"Z Key",			IDS_KEY_Z,		MR_KDB,		'Z'			},	// 86
	{	"Not a Key",		IDS_DISABLE,	MR_KDB,		0			},
	{	"Not a Key",		IDS_DISABLE,	MR_KDB,		0			},
	{	"Not a Key",		IDS_DISABLE,	MR_KDB,		0			},
	{	"Not a Key",		IDS_DISABLE,	MR_KDB,		0			},
};

#define NB_KEY_PLAYER_1   60
#define NB_KEY_PLAYER_2   87
#define NB_KEY_PLAYER_3   87
#define NB_KEY_PLAYER_4   87

BOOL gFirstKDBCall = TRUE;						  // Set to TRUE on each new game
BOOL gFirstKDBResetJoy1 = TRUE;					  // Set to TRUE on each new scan
BOOL gFirstKDBResetJoy2 = TRUE;					  // Set to TRUE on each new scan
BOOL gFirstKDBResetJoy3 = TRUE;					  // Set to TRUE on each new scan
BOOL gFirstKDBResetJoy4 = TRUE;					  // Set to TRUE on each new scan

static BOOL CheckKeyState(int pKeyIndex)
{
	BOOL lReturnValue = FALSE;

	static JOYINFOEX lJoystick1;
	static JOYINFOEX lJoystick2;
	static JOYINFOEX lJoystick3;
	static JOYINFOEX lJoystick4;

	switch (KeyChoice[pKeyIndex].mControler) {
		case MR_KDB:
			lReturnValue = GetAsyncKeyState(KeyChoice[pKeyIndex].mKeyValue);
			break;
		case MR_JOY1:
			if(gFirstKDBResetJoy1) {
				gFirstKDBResetJoy1 = FALSE;

				lJoystick1.dwSize = sizeof(lJoystick1);
				lJoystick1.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				joyGetPosEx(0, &lJoystick1);

				/*
				   TRACE( "Joy %d %d %d %d %d %d \n",
				   lJoystick1.dwXpos, lJoystick1.dwYpos,
				   lJoystick1.dwButtons&1,
				   lJoystick1.dwButtons&2,
				   lJoystick1.dwButtons&4,
				   lJoystick1.dwButtons&8              );
				 */
			}

			switch (KeyChoice[pKeyIndex].mKeyValue) {
				case AxeTop:
					lReturnValue = (lJoystick1.dwYpos < 16000);
					break;
				case AxeBottom:
					lReturnValue = (lJoystick1.dwYpos > 48000);
					break;
				case AxeLeft:
					lReturnValue = (lJoystick1.dwXpos < 16000);
					break;
				case AxeRight:
					lReturnValue = (lJoystick1.dwXpos > 48000);
					break;
				case Btn1:
					lReturnValue = (lJoystick1.dwButtons & 1);
					break;
				case Btn2:
					lReturnValue = (lJoystick1.dwButtons & 2);
					break;
				case Btn3:
					lReturnValue = (lJoystick1.dwButtons & 4);
					break;
				case Btn4:
					lReturnValue = (lJoystick1.dwButtons & 8);
					break;
				case Btn5:
					lReturnValue = (lJoystick1.dwButtons & 16);
					break;
				case Btn6:
					lReturnValue = (lJoystick1.dwButtons & 32);
					break;
				case Btn7:
					lReturnValue = (lJoystick1.dwButtons & 64);
					break;
				case Btn8:
					lReturnValue = (lJoystick1.dwButtons & 128);
					break;
			}
			break;
		case MR_JOY2:
			if(gFirstKDBResetJoy2) {
				gFirstKDBResetJoy2 = FALSE;

				lJoystick2.dwSize = sizeof(lJoystick2);
				lJoystick2.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				joyGetPosEx(0, &lJoystick2);
			}

			switch (KeyChoice[pKeyIndex].mKeyValue) {
				case AxeTop:
					lReturnValue = (lJoystick2.dwYpos < 16000);
					break;
				case AxeBottom:
					lReturnValue = (lJoystick2.dwYpos > 48000);
					break;
				case AxeLeft:
					lReturnValue = (lJoystick2.dwXpos < 16000);
					break;
				case AxeRight:
					lReturnValue = (lJoystick2.dwXpos > 48000);
					break;
				case Btn1:
					lReturnValue = (lJoystick2.dwButtons & 1);
					break;
				case Btn2:
					lReturnValue = (lJoystick2.dwButtons & 2);
					break;
				case Btn3:
					lReturnValue = (lJoystick2.dwButtons & 4);
					break;
				case Btn4:
					lReturnValue = (lJoystick2.dwButtons & 8);
					break;
				case Btn5:
					lReturnValue = (lJoystick2.dwButtons & 16);
					break;
				case Btn6:
					lReturnValue = (lJoystick2.dwButtons & 32);
					break;
				case Btn7:
					lReturnValue = (lJoystick2.dwButtons & 64);
					break;
				case Btn8:
					lReturnValue = (lJoystick2.dwButtons & 128);
					break;
			}
			break;
		case MR_JOY3:
			if(gFirstKDBResetJoy3) {
				gFirstKDBResetJoy3 = FALSE;

				lJoystick3.dwSize = sizeof(lJoystick3);
				lJoystick3.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				joyGetPosEx(0, &lJoystick3);
			}

			switch (KeyChoice[pKeyIndex].mKeyValue) {
				case AxeTop:
					lReturnValue = (lJoystick3.dwYpos < 16000);
					break;
				case AxeBottom:
					lReturnValue = (lJoystick3.dwYpos > 48000);
					break;
				case AxeLeft:
					lReturnValue = (lJoystick3.dwXpos < 16000);
					break;
				case AxeRight:
					lReturnValue = (lJoystick3.dwXpos > 48000);
					break;
				case Btn1:
					lReturnValue = (lJoystick3.dwButtons & 1);
					break;
				case Btn2:
					lReturnValue = (lJoystick3.dwButtons & 2);
					break;
				case Btn3:
					lReturnValue = (lJoystick3.dwButtons & 4);
					break;
				case Btn4:
					lReturnValue = (lJoystick3.dwButtons & 8);
					break;
				case Btn5:
					lReturnValue = (lJoystick3.dwButtons & 16);
					break;
				case Btn6:
					lReturnValue = (lJoystick3.dwButtons & 32);
					break;
				case Btn7:
					lReturnValue = (lJoystick3.dwButtons & 64);
					break;
				case Btn8:
					lReturnValue = (lJoystick3.dwButtons & 128);
					break;
			}
			break;
		case MR_JOY4:
			if(gFirstKDBResetJoy4) {
				gFirstKDBResetJoy4 = FALSE;

				lJoystick4.dwSize = sizeof(lJoystick4);
				lJoystick4.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				joyGetPosEx(0, &lJoystick4);
			}

			switch (KeyChoice[pKeyIndex].mKeyValue) {
				case AxeTop:
					lReturnValue = (lJoystick4.dwYpos < 16000);
					break;
				case AxeBottom:
					lReturnValue = (lJoystick4.dwYpos > 48000);
					break;
				case AxeLeft:
					lReturnValue = (lJoystick4.dwXpos < 16000);
					break;
				case AxeRight:
					lReturnValue = (lJoystick4.dwXpos > 48000);
					break;
				case Btn1:
					lReturnValue = (lJoystick4.dwButtons & 1);
					break;
				case Btn2:
					lReturnValue = (lJoystick4.dwButtons & 2);
					break;
				case Btn3:
					lReturnValue = (lJoystick4.dwButtons & 4);
					break;
				case Btn4:
					lReturnValue = (lJoystick4.dwButtons & 8);
					break;
				case Btn5:
					lReturnValue = (lJoystick4.dwButtons & 16);
					break;
				case Btn6:
					lReturnValue = (lJoystick4.dwButtons & 32);
					break;
				case Btn7:
					lReturnValue = (lJoystick4.dwButtons & 64);
					break;
				case Btn8:
					lReturnValue = (lJoystick4.dwButtons & 128);
					break;
			}
			break;
	}
	return lReturnValue;
}

MR_GameApp *MR_GameApp::This;

// Local prototypes
static HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPSTR lpString, HPALETTE FAR * lphPalette);
static HPALETTE CreateDIBPalette(LPBITMAPINFO lpbmi, LPINT lpiNumColors);

// class MR_GameThread

unsigned long MR_GameThread::Loop(LPVOID pThread)
{
	MR_GameThread *lThis = (MR_GameThread *) pThread;

	while(true) {
		EnterCriticalSection(&lThis->mMutex);
		MR_GameApp *gameApp = lThis->mGameApp;

		if(lThis->mTerminate)
			break;

		ASSERT(lThis->mGameApp->mCurrentSession != NULL);

		OS::timestamp_t tick = OS::Time();

		// Game processing
		HighConsole *highConsole = gameApp->highConsole;
		if (highConsole != NULL && highConsole->isVisible()) {
			highConsole->Advance(tick);
		}
		else {
			gameApp->ReadAsyncInputController();
		}

		MR_SAMPLE_START(Process, "Process");

#ifdef MR_AVI_CAPTURE
		gameApp->mCurrentSession->Process(1000 / gCaptureFrameRate);
#else
		gameApp->mCurrentSession->Process();
#endif

		MR_SAMPLE_END(Process);
		MR_SAMPLE_START(Refresh, "Refresh");
		gameApp->RefreshView();
		gameApp->mNbFrames++;
		MR_SAMPLE_END(Refresh);

		MR_PRINT_STATS(10);						  // Print and reset profiling statistics every 5 seconds

		LeaveCriticalSection(&lThis->mMutex);
	}

	return 0;
}

MR_GameThread::MR_GameThread(MR_GameApp * pApp)
{
	mGameApp = pApp;
	InitializeCriticalSection(&mMutex);
	mThread = NULL;
	mTerminate = FALSE;
	mPauseLevel = 0;
}

MR_GameThread::~MR_GameThread()
{
	DeleteCriticalSection(&mMutex);
}

MR_GameThread *MR_GameThread::New(MR_GameApp * pApp)
{
	unsigned long lDummyInt;
	MR_GameThread *lReturnValue = new MR_GameThread(pApp);

	lReturnValue->mThread = CreateThread(NULL, 0, Loop, lReturnValue, 0, &lDummyInt);

	if(lReturnValue->mThread == NULL) {
		delete lReturnValue;
		lReturnValue = NULL;
	}
	return lReturnValue;
}

void MR_GameThread::Kill()
{
	mTerminate = TRUE;

	while(mPauseLevel > 0)
		Restart();

	WaitForSingleObject(mThread, 3000);
	delete this;
}

void MR_GameThread::Pause()
{
	if(mPauseLevel == 0)
		EnterCriticalSection(&mMutex);
	mPauseLevel++;
}

void MR_GameThread::Restart()
{
	if(mPauseLevel > 0) {
		mPauseLevel--;
		if(mPauseLevel == 0)
			LeaveCriticalSection(&mMutex);
	}
}

MR_GameApp::MR_GameApp(HINSTANCE pInstance, bool safeMode)
{
	This = this;
	mInstance = pInstance;
	mMainWindow = NULL;
	mBadVideoModeDlg = NULL;
	mMovieWnd = NULL;
	mAccelerators = NULL;
	mVideoBuffer = NULL;
	mObserver1 = NULL;
	mObserver2 = NULL;
	mObserver3 = NULL;
	mObserver4 = NULL;
	highObserver = NULL;
	highConsole = NULL;
	fullscreenTest = NULL;
	mCurrentSession = NULL;
	mGameThread = NULL;

	mCurrentMode = e3DView;

	mClrScrTodo = 2;

	mPaletteChangeAllowed = TRUE;

	controller = NULL;

	// Load the configuration.
	if (!safeMode) {
		LoadRegistry();
		Config::GetInstance()->Load();
		OutputDebugString("Loaded config.\n");
	}

	mServerHasChanged = FALSE;

	mustCheckUpdates = Config::GetInstance()->net.autoUpdates;
}

MR_GameApp::~MR_GameApp()
{
	delete controller;

	Clean();
	MR_DllObjectFactory::Clean(FALSE);
	MR_SoundServer::Close();
	delete mVideoBuffer;
}

void MR_GameApp::Clean()
{
	if(mGameThread != NULL) {
		mGameThread->Kill();
		mGameThread = NULL;
	}
	delete mCurrentSession;
	mCurrentSession = NULL;

	delete highConsole;
	delete highObserver;
	mObserver1->Delete();
	mObserver2->Delete();
	mObserver3->Delete();
	mObserver4->Delete();

	highConsole = NULL;
	highObserver = NULL;
	mObserver1 = NULL;
	mObserver2 = NULL;
	mObserver3 = NULL;
	mObserver4 = NULL;

	MR_DllObjectFactory::Clean(TRUE);

	mClrScrTodo = 2;
	gFirstKDBCall = TRUE;						  // Set to TRUE on each new game

}

void MR_GameApp::LoadRegistry()
{
	Config *cfg = Config::GetInstance();

	char lBuffer[80];
	DWORD lBufferSize = sizeof(lBuffer);

	// Now verify in the registry if this information can not be retrieved
	HKEY lProgramKey;

	int lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\HoverRace.com\\HoverRace",
		0,
		KEY_EXECUTE,
		&lProgramKey);

	if(lError == ERROR_SUCCESS) { // opened key successfully
		MR_UInt8 lControlBuffer[32];
		DWORD lControlBufferSize = sizeof(lControlBuffer);

		/*
		if(RegQueryValueEx(lProgramKey, "Control", 0, NULL, lControlBuffer, &lControlBufferSize) == ERROR_SUCCESS) {
			for (int p = 0, i = 0; p < Config::MAX_PLAYERS; ++p) {
				Config::cfg_controls_t &ctl = cfg->controls[p];
				ctl.motorOn = lControlBuffer[i++];
				ctl.right = lControlBuffer[i++];
				ctl.left = lControlBuffer[i++];
				ctl.jump = lControlBuffer[i++];
				ctl.fire = lControlBuffer[i++];
				ctl.brake = lControlBuffer[i++];
				ctl.weapon = lControlBuffer[i++];
				ctl.lookBack = lControlBuffer[i++];
			}
		}
		*/

		double lVideoSetting[3];
		DWORD lVideoSettingSize = sizeof(lVideoSetting);

		if(RegQueryValueEx(lProgramKey, "VideoColors", 0, NULL, (MR_UInt8 *) lVideoSetting, &lVideoSettingSize) == ERROR_SUCCESS) {
			cfg->video.gamma = lVideoSetting[0];
			cfg->video.contrast = lVideoSetting[1];
			cfg->video.brightness = lVideoSetting[2];
		}

		lBufferSize = sizeof(lBuffer);
		if(RegQueryValueEx(lProgramKey, "Alias", 0, NULL, (MR_UInt8 *) lBuffer, &lBufferSize) == ERROR_SUCCESS)
			cfg->player.nickName = lBuffer;

		BOOL lBool;

		lBufferSize = sizeof(lBool);
		if(RegQueryValueEx(lProgramKey, "DisplayFirstScreen", 0, NULL, (MR_UInt8 *) &lBool, &lBufferSize) == ERROR_SUCCESS)
			cfg->misc.displayFirstScreen = !lBool;

		lBufferSize = sizeof(lBool);
		if(RegQueryValueEx(lProgramKey, "IntroMovie", 0, NULL, (MR_UInt8 *) &lBool, &lBufferSize) == ERROR_SUCCESS)
			cfg->misc.introMovie = !lBool;

		// Get the address of the server (we need a larger buffer)
		{
			char  lServerBuffer[500];
			DWORD lServerBufferSize = sizeof(lServerBuffer);
			if(RegQueryValueEx(lProgramKey, "MainServer", 0, NULL, (MR_UInt8 *) lServerBuffer, &lServerBufferSize) == ERROR_SUCCESS) {
				cfg->net.mainServer = lServerBuffer;
			}
		}
	}
}

BOOL MR_GameApp::IsGameRunning()
{
	BOOL lReturnValue = FALSE;

	if(mCurrentSession != NULL) {
		MR_MainCharacter *lPlayer = mCurrentSession->GetMainCharacter();

		if(lPlayer != NULL) {
			if(!(lPlayer->GetTotalLap() <= lPlayer->GetLap())) {
				lPlayer = mCurrentSession->GetMainCharacter2();

				if(lPlayer == NULL) {
					lReturnValue = TRUE;
				}

				lPlayer = mCurrentSession->GetMainCharacter3();

				if(lPlayer == NULL) {
					lReturnValue = TRUE;
				}

				lPlayer = mCurrentSession->GetMainCharacter4();

				if(lPlayer == NULL) {
					lReturnValue = TRUE;
				}
				else {
					if(lPlayer->GetTotalLap() <= lPlayer->GetLap()) {
						lReturnValue = TRUE;
					}
				}
			}
		}
	}
	return lReturnValue;
}

int MR_GameApp::AskUserToAbortGame()
{
	int lReturnValue = IDOK;

	if(IsGameRunning()) {
		lReturnValue = MessageBoxW(mMainWindow,
			Str::UW(_("Abort current game?")),
			PACKAGE_NAME_L,
			MB_OKCANCEL | MB_ICONWARNING);

		if(lReturnValue == 0) {
			lReturnValue = IDOK;
		}
	}
	return lReturnValue;
}

void MR_GameApp::DisplayHelp()
{

	// first return to window mode
	SetVideoMode(0, 0);

	unsigned int lReturnCode = (int) ShellExecute(mMainWindow,
		NULL,
		"..\\Help\\Help.doc",					  // "..\\Help\\Index.html",
		NULL,
		NULL,
		SW_SHOWNORMAL);

	if(lReturnCode <= 32) {
		MessageBoxW(mMainWindow,
			Str::UW(_("To be able to display HoverRace Help\n"
			"you must have a Word document viewer installed\n"
			"on your system.")),
			PACKAGE_NAME_L, MB_ICONERROR | MB_APPLMODAL | MB_OK);
	}

}

void MR_GameApp::DisplaySite()
{
	// first return to window mode
	SetVideoMode(0, 0);
	OS::OpenLink(HR_WEBSITE);
}

void MR_GameApp::DisplayAbout()
{
	SetVideoMode(0, 0);
	AboutDialog().ShowModal(mInstance, mMainWindow);
	AssignPalette();
}

void MR_GameApp::DisplayPrefs()
{
	const Config *cfg = Config::GetInstance();
	FullscreenTest *ftest =
		new FullscreenTest(
			cfg->video.xResFullscreen,
			cfg->video.yResFullscreen,
			cfg->video.fullscreenMonitor);

	SetVideoMode(0, 0);
	PrefsDialog(This).ShowModal(mInstance, mMainWindow);
	
	if (ftest->ShouldActivateTest(mMainWindow)) {
		DeleteMovieWnd();

		fullscreenTest = ftest;
		SetVideoMode(cfg->video.xResFullscreen,
			cfg->video.yResFullscreen,
			&cfg->video.fullscreenMonitor,
			true);

		if (mVideoBuffer != NULL) {
			while (mVideoBuffer->IsModeSettingInProgress()) {
				Sleep(0);
			}
			mVideoBuffer->AssignPalette();
			fullscreenTest->Render(mVideoBuffer);
		}
	}
	else {
		delete ftest;
	}
}

int MR_GameApp::MainLoop()
{
	MSG lMessage;
	BOOL lEofGame = FALSE;

	while(!lEofGame) {
		WaitMessage();

		while(PeekMessage(&lMessage, NULL, 0, 0, PM_NOREMOVE)) {
			if(GetMessage(&lMessage, NULL, 0, 0)) {
				if(!TranslateAccelerator(mMainWindow, mAccelerators, &lMessage)) {
					TranslateMessage(&lMessage);
					DispatchMessage(&lMessage);
				}
			}
			else {
				lEofGame = TRUE;
			}
		}
	}
	Clean();

	return lMessage.wParam;
}

BOOL MR_GameApp::IsFirstInstance() const
{
	HWND lPrevAppWnd = FindWindowW(MR_APP_CLASS_NAME, NULL);
	HWND lChildAppWnd = NULL;

	// Determine if another window with our class name exists...
	if(lPrevAppWnd != NULL) {
		// if so, does it have any popups?
		lChildAppWnd = GetLastActivePopup(lPrevAppWnd);

		// Bring the main window to the top.
		SetForegroundWindow(lPrevAppWnd);
		// If iconic, restore the main window.
		ShowWindow(lPrevAppWnd, SW_RESTORE);

		// If there was an active popup, bring it along too!
		if((lChildAppWnd != NULL) && (lPrevAppWnd != lChildAppWnd))
			SetForegroundWindow(lChildAppWnd);
	}
	return (lPrevAppWnd == NULL);
}

BOOL MR_GameApp::InitApplication()
{
	BOOL lReturnValue = TRUE;

	WNDCLASSW lWinClass;

	lWinClass.style = CS_DBLCLKS;
	lWinClass.lpfnWndProc = DispatchFunc;
	lWinClass.cbClsExtra = 0;
	lWinClass.cbWndExtra = 0;
	lWinClass.hInstance = mInstance;
	lWinClass.hIcon = LoadIcon(mInstance, MAKEINTRESOURCE(IDI_HOVER_ICON));
	lWinClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	lWinClass.hbrBackground = (HBRUSH) COLOR_APPWORKSPACE + 1;
	//lWinClass.lpszMenuName = MAKEINTRESOURCEW(FIREBALL_MAIN_MENU);
	lWinClass.lpszMenuName = NULL;
	lWinClass.lpszClassName = MR_APP_CLASS_NAME;

	lReturnValue = RegisterClassW(&lWinClass);

	ASSERT(lReturnValue);

	return lReturnValue;
}

BOOL MR_GameApp::CreateMainWindow()
{
	BOOL lReturnValue = TRUE;

	// check our position
	Config *cfg = Config::GetInstance();
	int xPos = cfg->video.xPos;
	int yPos = cfg->video.yPos;
	int xRes = cfg->video.xRes;
	int yRes = cfg->video.yRes;
	int winXRes = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int winYRes = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// verify a valid size
	if(xRes > winXRes)
		xRes = winXRes;
	if(yRes > winYRes)
		yRes = winYRes;

	// verify a valid position
	if(xPos > winXRes)
		xPos = winXRes - xRes;
	if(xPos < 0)
		xPos = 0;
	if(yPos > winYRes)
		yPos = winYRes - yRes;
	if(yPos < 0)
		yPos = 0;

	// attempt to make the main window
	mMainWindow = CreateWindowExW(
		WS_EX_APPWINDOW,
		MR_APP_CLASS_NAME,
		PACKAGE_NAME_L,
		(WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_EX_CLIENTEDGE) & ~WS_MAXIMIZEBOX,
		xPos, yPos,
		xRes, yRes,
		NULL, NULL, mInstance, NULL);

	if(mMainWindow == NULL)
		lReturnValue = FALSE;					  // making of window failed
	else {
		InvalidateRect(mMainWindow, NULL, FALSE);
		UpdateWindow(mMainWindow);
		SetFocus(mMainWindow);
	}

	RefreshTitleBar();
	CreateMainMenu();
	UpdateMenuItems();

	// set up controller
	controller = new HoverRace::Client::Control::Controller(mMainWindow);

	return lReturnValue;
}

#define MENUFMT(ctx, txt, more, key) MenuFmt(ctx "\004" txt, txt, more, key)
static std::string MenuFmt(const char *ctx, const char *txt, bool more,
                           const char *key)
{
#ifdef ENABLE_NLS
	txt = pgettextImpl(ctx, txt);
#endif
	std::string s = txt;
	s.reserve(64);

	if (more) s += "\xe2\x80\xa6";  // Ellipsis in UTF-8.
	if (key != NULL) {
		s += '\t';
		s += key;
	}

	return s;
}

bool MR_GameApp::CreateMainMenu()
{
	HMENU splitScreenMenu = CreatePopupMenu();
	if (splitScreenMenu == NULL) return false;
	if (!AppendMenuW(splitScreenMenu, MF_STRING, ID_GAME_SPLITSCREEN,
		Str::UW(MENUFMT("Menu|File|Split Screen", "&2 Player", true, NULL).c_str()))) return false;
	if (!AppendMenuW(splitScreenMenu, MF_STRING, ID_GAME_SPLITSCREEN3,
		Str::UW(MENUFMT("Menu|File|Split Screen", "&3 Player", true, NULL).c_str()))) return false;
	if (!AppendMenuW(splitScreenMenu, MF_STRING, ID_GAME_SPLITSCREEN4,
		Str::UW(MENUFMT("Menu|File|Split Screen", "&4 Player", true, NULL).c_str()))) return false;

	HMENU netMenu = CreatePopupMenu();
	if (netMenu == NULL) return false;
	if (!AppendMenuW(netMenu, MF_STRING, ID_GAME_NETWORK_CONNECT,
		Str::UW(MENUFMT("Menu|File|Network", "&Connect to server", true, NULL).c_str()))) return false;
	if (!AppendMenuW(netMenu, MF_STRING, ID_GAME_NETWORK_SERVER,
		Str::UW(MENUFMT("Menu|File|Network", "Create a &server", true, NULL).c_str()))) return false;
	if (!AppendMenuW(netMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(netMenu, MF_STRING, ID_GAME_NETWORK_INTERNET,
		Str::UW(MENUFMT("Menu|File|Network", "&Internet Meeting Room", true, "F2").c_str()))) return false;

	HMENU fileMenu = CreatePopupMenu();
	if (fileMenu == NULL) return false;
	if (!AppendMenuW(fileMenu, MF_STRING, ID_GAME_NETWORK_INTERNET,
		Str::UW(MENUFMT("Menu|File", "&Internet", true, "F2").c_str()))) return false;
	if (!AppendMenuW(fileMenu, MF_STRING, ID_GAME_NEW,
		Str::UW(MENUFMT("Menu|File", "&Practice", true, "Ctrl+N").c_str()))) return false;
	if (!AppendMenuW(fileMenu, MF_POPUP | MF_STRING, (UINT_PTR)splitScreenMenu,
		Str::UW(pgettext("Menu|File","&Split Screen")))) return false;
	if (!AppendMenuW(fileMenu, MF_POPUP | MF_STRING, (UINT_PTR)netMenu,
		Str::UW(pgettext("Menu|File","&Network")))) return false;
	if (!AppendMenuW(fileMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(fileMenu, MF_STRING, ID_APP_EXIT,
		Str::UW(MENUFMT("Menu|File", "E&xit", false, "Alt+F4").c_str()))) return false;
	
	HMENU viewMenu = CreatePopupMenu();
	if (viewMenu == NULL) return false;
	if (!AppendMenuW(viewMenu, MF_STRING, ID_VIEW_3DACTION,
		Str::UW(MENUFMT("Menu|View", "&Following Camera", false, "F3").c_str()))) return false;
	if (!AppendMenuW(viewMenu, MF_STRING, ID_VIEW_COCKPIT,
		Str::UW(MENUFMT("Menu|View", "&Cockpit", false, "F4").c_str()))) return false;
	if (!AppendMenuW(viewMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(viewMenu, MF_STRING, ID_VIEW_PLAYERSLIST,
		Str::UW(MENUFMT("Menu|View", "&Players list", false, "F5").c_str()))) return false;
	if (!AppendMenuW(viewMenu, MF_STRING, ID_VIEW_MOREMESSAGES,
		Str::UW(MENUFMT("Menu|View", "More &messages", false, "F6").c_str()))) return false;
	if (!AppendMenuW(viewMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(viewMenu, MF_STRING, ID_SETTING_REFRESHCOLORS,
		Str::UW(MENUFMT("Menu|View", "&Refresh colors", false, "F12").c_str()))) return false;

	HMENU settingMenu = CreatePopupMenu();
	if (settingMenu == NULL) return false;
	if (!AppendMenuW(settingMenu, MF_STRING, ID_SETTING_FULLSCREEN,
		Str::UW(MENUFMT("Menu|Setting", "&Fullscreen", false, "F9").c_str()))) return false;
	if (!AppendMenuW(settingMenu, MF_STRING, ID_SETTING_WINDOW,
		Str::UW(MENUFMT("Menu|Setting", "&Window", false, "Alt+Enter").c_str()))) return false;
	if (!AppendMenuW(settingMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(settingMenu, MF_STRING, ID_SETTING_PROPERTIES,
		Str::UW(MENUFMT("Menu|Setting", "&Preferences", true, NULL).c_str()))) return false;

	HMENU helpMenu = CreatePopupMenu();
	if (helpMenu == NULL) return false;
	if (!AppendMenuW(helpMenu, MF_STRING, ID_HELP_CONTENTS,
		Str::UW(MENUFMT("Menu|Help", "&Contents", false, NULL).c_str()))) return false;
	if (!AppendMenuW(helpMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(helpMenu, MF_STRING, ID_HELP_SITE,
		Str::UW("&HoverRace.com"))) return false;
	if (!AppendMenuW(helpMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(helpMenu, MF_STRING, ID_HELP_UPDATES,
		Str::UW(MENUFMT("Menu|Help", "Check for &Updates", false, NULL).c_str()))) return false;
	if (!AppendMenuW(helpMenu, MF_SEPARATOR, NULL, NULL)) return false;
	if (!AppendMenuW(helpMenu, MF_STRING, ID_APP_ABOUT,
		Str::UW(MENUFMT("Menu|Help", "&About HoverRace", false, NULL).c_str()))) return false;

	HMENU menu = CreateMenu();
	if (menu == NULL) return false;
	if (!AppendMenuW(menu, MF_POPUP | MF_STRING, (UINT_PTR)fileMenu,
		Str::UW(pgettext("Menu","&File")))) return false;
	if (!AppendMenuW(menu, MF_POPUP | MF_STRING, (UINT_PTR)viewMenu,
		Str::UW(pgettext("Menu","&View")))) return false;
	if (!AppendMenuW(menu, MF_POPUP | MF_STRING, (UINT_PTR)settingMenu,
		Str::UW(pgettext("Menu","&Setting")))) return false;
	if (!AppendMenuW(menu, MF_POPUP | MF_STRING, (UINT_PTR)helpMenu,
		Str::UW(pgettext("Menu","&Help")))) return false;

	return SetMenu(mMainWindow, menu) == TRUE;
}

void MR_GameApp::RefreshTitleBar()
{
	Config *cfg = Config::GetInstance();

	if(mMainWindow != NULL) {
		std::ostringstream oss;
		oss << PACKAGE_NAME " " << cfg->GetVersion();
		if (cfg->IsPrerelease()) {
			oss << " (" << pgettext("Version", "testing") << ')';
		}
		if (cfg->runtime.silent) {
			oss << " (" << _("silent mode") << ')';
		}
		SetWindowTextW(mMainWindow, Str::UW(oss.str().c_str()));
	}
}

BOOL MR_GameApp::InitGame()
{
	BOOL lReturnValue = TRUE;
	Config *cfg = Config::GetInstance();

	InitCommonControls();						  // Allow some special and complex controls

	// Load the latest Rich Edit control.
	//TODO: Create Rich Edit controls manually so we always try to
	//      use the latest version.
	if (LoadLibrary("riched20.dll") == NULL) {
		MessageBoxW(mMainWindow,
			Str::UW(_("Unable to initialize: Unable to find suitable rich text DLL.")),
			PACKAGE_NAME_L, MB_ICONERROR);
		return FALSE;
	}

	// Display a Flash window
	// TODO

	// Init needed modules
	MR_InitTrigoTables();
	MR_InitFuzzyModule();
	MR_DllObjectFactory::Init();
	MR_MainCharacter::RegisterFactory();

	// Load accelerators
	mAccelerators = LoadAccelerators(mInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

	lReturnValue = CreateMainWindow();

	if(lReturnValue) {
		mVideoBuffer = new MR_VideoBuffer(mMainWindow,
			cfg->video.gamma, cfg->video.contrast, cfg->video.brightness);

		// Attempt to create the sound service.
		// We do this early so we can notify the user of any errors
		// and let it switch to silent mode.
		if (!SOUNDSERVER_INIT(mMainWindow)) {
			std::string errMsg;
			errMsg += _("There was a problem setting up the sound device.");
			errMsg += "\r\n";
			errMsg += _("Sound will be disabled for this session.");
			errMsg += "\r\n\r\n";
			errMsg += _("If this is your first time running HoverRace, try restarting your computer.");
			errMsg += "\r\n\r\n";
			errMsg += _("The specific error was:");
			errMsg += "\r\n";
			errMsg += MR_SoundServer::GetInitError();
			MessageBoxW(mMainWindow, Str::UW(errMsg.c_str()), PACKAGE_NAME_L, MB_ICONWARNING);
			RefreshTitleBar();  // To add the "silent mode" marker.
		}
		else {
			// Stop the sound service so it doesn't interfere with the
			// intro movie.
			MR_SoundServer::Close();
		}
	}

	// attempt to set the video mode
	if(lReturnValue) {
		if(!mVideoBuffer->SetVideoMode()) {		  // try to set the video mode
			BOOL lSwitchTo256 = FALSE;
			if(MessageBoxW(mMainWindow, 
				Str::UW(_("You can play HoverRace in a Window or Fullscreen;\n"
				"to play in a Window, HoverRace will have to switch to 256 color mode.\n"
				"It is recommended you use Fullscreen mode instead.\n\n"
				"Do you want to play in Window mode and switch to 256 color?")), 
				Str::UW(_("HoverRace")), 
				MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {

				if(mVideoBuffer->TryToSetColorMode(8)) {
					if(mVideoBuffer->SetVideoMode())
						lSwitchTo256 = TRUE;
				}
				if(!lSwitchTo256) {				  // mode switch failed, tell the user
					MessageBoxW(mMainWindow,
						Str::UW(_("HoverRace was unable to switch the video mode. You will need to play in full screen.")), 
						Str::UW(_("HoverRace")),
						MB_OK);
				}
			}

			if(!lSwitchTo256) {					  // load the "Incompatible Video Mode" dialog
				mBadVideoModeDlg = CreateDialogW(mInstance, MAKEINTRESOURCEW(IDD_BAD_MODE3), mMainWindow, BadModeDialogFunc);
			}
		}
	}

	// play the opening movie
	if(lReturnValue && cfg->misc.introMovie) {
		mMovieWnd = MCIWndCreate(
			mMainWindow, mInstance, 
			WS_CHILD | MCIWNDF_NOMENU | MCIWNDF_NOPLAYBAR, 
			cfg->GetMediaPath("Intro.avi").c_str());

		// Fill the client area.
		RECT clientRect;
		GetClientRect(mMainWindow, &clientRect);
		MoveWindow(mMovieWnd, 0, 0,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top,
			TRUE);
		
		MCIWndPlay(mMovieWnd);

	}

	if(lReturnValue)
		OnDisplayChange();

	//if(lReturnValue)
	//{
	// Raise process priority
	// (Tests shows that it is not a good idea. It is not facter and
	//   it cause the animation to be less smooth )
	// SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );
	// SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST /*THREAD_PRIORITY_TIME_CRITICAL*/ );
	//}

	// show "click OK to play on the internet" dialog
	if(lReturnValue && cfg->misc.displayFirstScreen) {
		if (FirstChoiceDialog().ShowModal(mInstance, mMainWindow))
			SendMessage(mMainWindow, WM_COMMAND, ID_GAME_NETWORK_INTERNET, 0);
	}

	return lReturnValue;
}

void MR_GameApp::RefreshView()
{
	static int lColor = 0;

	// Game processing
	if(mVideoBuffer != NULL) {
		if(mVideoBuffer->Lock()) {
			if(mCurrentSession != NULL) {
				MR_SimulationTime lTime = mCurrentSession->GetSimulationTime();

				switch (mCurrentMode) {
					case e3DView:
						if(mClrScrTodo > 0) {
							mClrScrTodo--;
							DrawBackground();
						}

						if(mObserver1 != NULL)
							mObserver1->RenderNormalDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter(), lTime, mCurrentSession->GetBackImage());
						if(mObserver2 != NULL)
							mObserver2->RenderNormalDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter2(), lTime, mCurrentSession->GetBackImage());
						if(mObserver3 != NULL)
							mObserver3->RenderNormalDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter3(), lTime, mCurrentSession->GetBackImage());
						if(mObserver4 != NULL)
							mObserver4->RenderNormalDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter4(), lTime, mCurrentSession->GetBackImage());
						break;

					case eDebugView:
						if(mObserver1 != NULL)
							mObserver1->RenderDebugDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter(), lTime, mCurrentSession->GetBackImage());
						if(mObserver2 != NULL)
							mObserver2->RenderDebugDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter2(), lTime, mCurrentSession->GetBackImage());
						if(mObserver3 != NULL)
							mObserver3->RenderDebugDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter3(), lTime, mCurrentSession->GetBackImage());
						if(mObserver4 != NULL)
							mObserver4->RenderDebugDisplay(mVideoBuffer, mCurrentSession, mCurrentSession->GetMainCharacter4(), lTime, mCurrentSession->GetBackImage());
						break;
				}
				if (highObserver != NULL) {
					highObserver->Render(mVideoBuffer, mCurrentSession);
				}
				if (highConsole != NULL) {
					highConsole->Render(mVideoBuffer);
				}

				mCurrentSession->IncFrameCount();

#ifdef MR_AVI_CAPTURE
				CaptureScreen(mVideoBuffer);
#endif

			} else
			mVideoBuffer->Clear((MR_UInt8) (lColor++));
			mVideoBuffer->Unlock();
		}
	}
	// Sound refresh
	if(mCurrentSession != NULL) {
		if(mObserver1 != NULL)
			mObserver1->PlaySounds(mCurrentSession->GetCurrentLevel(), mCurrentSession->GetMainCharacter());
		if(mObserver2 != NULL)
			mObserver2->PlaySounds(mCurrentSession->GetCurrentLevel(), mCurrentSession->GetMainCharacter2());
		if(mObserver3 != NULL)
			mObserver3->PlaySounds(mCurrentSession->GetCurrentLevel(), mCurrentSession->GetMainCharacter3());
		if(mObserver4 != NULL)
			mObserver4->PlaySounds(mCurrentSession->GetCurrentLevel(), mCurrentSession->GetMainCharacter4());

		MR_SoundServer::ApplyContinuousPlay();
	}
}

void MR_GameApp::OnChar(char c)
{
	if (highConsole != NULL) {
		if (c == '`') {
			highConsole->toggleVisible();
		}
		else if (highConsole->isVisible()) {
			highConsole->OnChar(c);
		}
	}
	else if (mCurrentSession != NULL) {
		mCurrentSession->AddMessageKey(c);
	}
}

/**
 * Read the control state for a specific player.
 * @param playerIdx The player index (0 = first player, 1 = second, etc.).
 * @return The control state.
 */
int MR_GameApp::ReadAsyncInputControllerPlayer(int playerIdx)
{
	int retv = 0;

	// hack in for now; this checks the current state
	Control::ControlState cur = controller->getControlState(playerIdx);

	if (cur.motorOn)  retv |= MR_MainCharacter::eMotorOn;
	if (cur.jump)     retv |= MR_MainCharacter::eJump;
	if (cur.brake)    retv |= MR_MainCharacter::eBreakDirection;
	if (cur.fire)     retv |= MR_MainCharacter::eFire;
	if (cur.weapon)   retv |= MR_MainCharacter::eSelectWeapon;
	if (cur.lookBack) retv |= MR_MainCharacter::eLookBack;
	if (cur.right)    retv |= MR_MainCharacter::eRight;
	if (cur.left)     retv |= MR_MainCharacter::eLeft;

	controller->poll();

	return retv;
}

void MR_GameApp::ReadAsyncInputController()
{
	gFirstKDBResetJoy1 = TRUE;
	gFirstKDBResetJoy2 = TRUE;

	if(mCurrentSession != NULL) {
		if(GetForegroundWindow() == mMainWindow)
		{
			static BOOL lFirstCall = TRUE;
			int lControlState1 = 0;
			int lControlState2 = 0;
			int lControlState3 = 0;
			int lControlState4 = 0;

			lControlState1 = ReadAsyncInputControllerPlayer(0);

			// If we're in multiplayer mode we need to check those keys too
			if(mCurrentSession->GetMainCharacter2() != NULL) {
				lControlState2 = ReadAsyncInputControllerPlayer(1);
			}
			if(mCurrentSession->GetMainCharacter3() != NULL) {
				lControlState3 = ReadAsyncInputControllerPlayer(2);
			}
			if(mCurrentSession->GetMainCharacter4() != NULL) {
				lControlState4 = ReadAsyncInputControllerPlayer(3);
			}

			if(lFirstCall)
				lFirstCall = FALSE;
			else
				mCurrentSession->SetControlState(lControlState1, lControlState2, lControlState3, lControlState4);
		}
	}
}

bool MR_GameApp::SetVideoMode(int pX, int pY, const std::string *monitor, bool testing)
{
	if(mVideoBuffer != NULL) {
		bool lSuccess;
		bool stayPaused = false;

		PauseGameThread();

		mClrScrTodo = 2;

		if(pX == 0) {
			lSuccess = mVideoBuffer->SetVideoMode();
		}
		else {
			GUID *guid = NULL;
			if (monitor != NULL) {
				guid = (GUID*)malloc(sizeof(GUID));
				OS::StringToGuid(*monitor, *guid);
				if (guid->Data1 == 0) {
					OutputDebugString("Invalid monitor GUID: ");
					OutputDebugString(monitor->c_str());
					OutputDebugString("\n");
				}
			}
			lSuccess = mVideoBuffer->SetVideoMode(pX, pY, guid);
			free(guid);
			AssignPalette();

			if (testing) {
				SetTimer(mMainWindow, MRM_RETURN2WINDOWMODE, 1000, NULL);
				stayPaused = true;
			}
		}

		if (!stayPaused) {
			RestartGameThread();
		}

		//OnDisplayChange();
		return lSuccess;
	} else
		return false;
}

void MR_GameApp::PauseGameThread()
{
	if(mGameThread != NULL) {
		mGameThread->Pause();
	}
}

void MR_GameApp::RestartGameThread()
{
	if(mGameThread != NULL) {
		mGameThread->Restart();
	}
}

void MR_GameApp::OnDisplayChange()
{
	// Show or hide movie and display mode warning
	RECT lClientRect;

	if(!IsIconic(mMainWindow)) {
		if(GetClientRect(mMainWindow, &lClientRect)) {

			POINT lUpperLeft = { lClientRect.left, lClientRect.top };
			POINT lLowerRight = { lClientRect.right, lClientRect.bottom };
			RECT lMovieRect;
			RECT lBadModeRect;

			ClientToScreen(mMainWindow, &lUpperLeft);
			ClientToScreen(mMainWindow, &lLowerRight);

			if((mBadVideoModeDlg != NULL) && GetWindowRect(mBadVideoModeDlg, &lBadModeRect)) {
				if((mVideoBuffer != NULL) && !mVideoBuffer->IsWindowMode()) {
					ShowWindow(mBadVideoModeDlg, SW_HIDE);
				}
				else {

					SetWindowPos(mBadVideoModeDlg, HWND_TOP, (( /*lUpperLeft.x+ */ lLowerRight.x) - (lBadModeRect.right - lBadModeRect.left)),
						(( /*lUpperLeft.y+ */ lLowerRight.y) - (lBadModeRect.bottom - lBadModeRect.top)),
						0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
				}
			}

			if(GetWindowRect(mMovieWnd, &lMovieRect)) {
				SetWindowPos(mMovieWnd, HWND_TOP,
					0, 0,
					lClientRect.right - lClientRect.left,
					lClientRect.bottom - lClientRect.top,
					SWP_SHOWWINDOW);
			}
		}
	}
	// Adjust video buffer
	if(mVideoBuffer != NULL) {
		if(IsIconic(mMainWindow)) {

			//PauseGameThread();
			mVideoBuffer->EnterIconMode();
			//RestartGameThread();

		}
		else {
			if(!mVideoBuffer->IsModeSettingInProgress()) {
				if(mVideoBuffer->IsIconMode()) {

					PauseGameThread();
					mClrScrTodo = 2;
					mVideoBuffer->ExitIconMode();
					mVideoBuffer->SetVideoMode();
					RestartGameThread();
					PostMessage(mMainWindow, WM_QUERYNEWPALETTE, 0, 0);

				}
				else {
					if(mVideoBuffer->IsWindowMode()) {

						PauseGameThread();
						mClrScrTodo = 2;
						mVideoBuffer->SetVideoMode();
						RestartGameThread();
					}
				}
			}
		}
	}
}

void MR_GameApp::AssignPalette()
{
	if(mPaletteChangeAllowed) {
		if(This->mGameThread == NULL) {
			if(mMovieWnd != NULL)
				MCIWndRealize(mMovieWnd, FALSE);
		}
		else {
			if(This->mVideoBuffer != NULL)
				This->mVideoBuffer->AssignPalette();
		}
	}
}

void MR_GameApp::DeleteMovieWnd()
{
	if(mMovieWnd != NULL) {

		MCIWndClose(mMovieWnd);
		Sleep(1000);
		MCIWndDestroy(mMovieWnd);
		mMovieWnd = NULL;
	}
}

void MR_GameApp::NewLocalSession()
{
	bool lSuccess = true;

	// Verify is user acknowledge
	if(AskUserToAbortGame() != IDOK)
		return;

	// Delete the current session
	Clean();

	// Prompt the user for a track name
	std::string lCurrentTrack;
	int lNbLap;
	char lGameOpts;

	lSuccess = MR_SelectTrack(mMainWindow, lCurrentTrack, lNbLap, lGameOpts);

	if(lSuccess) {
		DeleteMovieWnd();
		SOUNDSERVER_INIT(mMainWindow);
		mObserver1 = MR_Observer::New();
		highObserver = new HighObserver();
		if (Config::GetInstance()->runtime.enableConsole) {
			highConsole = new HighConsole();
		}

		// Create the new session
		MR_ClientSession *lCurrentSession = new MR_ClientSession();

		// Load the selected track
		if(lSuccess) {
			MR_RecordFile *lTrackFile = MR_TrackOpen(mMainWindow, lCurrentTrack.c_str());
			lSuccess = (lCurrentSession->LoadNew(lCurrentTrack.c_str(), lTrackFile, lNbLap, lGameOpts, mVideoBuffer) != FALSE);
		}
		// Create the main character
		if(lSuccess)
			lCurrentSession->SetSimulationTime(-6000);

		// Create the main character
		if(lSuccess)
			lSuccess = (lCurrentSession->CreateMainCharacter() != FALSE);

		if(lSuccess) {
			mCurrentSession = lCurrentSession;
			mGameThread = MR_GameThread::New(this);

			if(mGameThread == NULL) {
				mCurrentSession = NULL;
				delete lCurrentSession;
			}
		}										  // it failed, abort
		else {
			// Clean everything
			Clean();
			delete lCurrentSession;
		}
	}

	AssignPalette();
}

void MR_GameApp::NewSplitSession(int pSplitPlayers)
{
	bool lSuccess = true;

	// Verify is user acknowledge
	if(AskUserToAbortGame() != IDOK)
		return;

	// Delete the current session
	Clean();

	// Prompt the user for a maze name
	std::string lCurrentTrack;
	int lNbLap;
	char lGameOpts;

	lSuccess = MR_SelectTrack(mMainWindow, lCurrentTrack, lNbLap, lGameOpts);

	if(lSuccess) {
		// Create the new session
		DeleteMovieWnd();
		SOUNDSERVER_INIT(mMainWindow);

		mObserver1 = MR_Observer::New();
		mObserver2 = MR_Observer::New();
		if(pSplitPlayers > 2)
			mObserver3 = MR_Observer::New();
		if(pSplitPlayers > 3)
			mObserver4 = MR_Observer::New();

		if(pSplitPlayers == 2) {
			mObserver1->SetSplitMode(MR_Observer::eUpperSplit);
			mObserver2->SetSplitMode(MR_Observer::eLowerSplit);
		}
		if(pSplitPlayers == 3) {
			mObserver1->SetSplitMode(MR_Observer::eUpperLeftSplit);
			mObserver2->SetSplitMode(MR_Observer::eUpperRightSplit);
			mObserver3->SetSplitMode(MR_Observer::eLowerLeftSplit);
		}
		if(pSplitPlayers == 4) {
			mObserver1->SetSplitMode(MR_Observer::eUpperLeftSplit);
			mObserver2->SetSplitMode(MR_Observer::eUpperRightSplit);
			mObserver3->SetSplitMode(MR_Observer::eLowerLeftSplit);
			mObserver4->SetSplitMode(MR_Observer::eLowerRightSplit);
		}

		highObserver = new HighObserver();

		MR_ClientSession *lCurrentSession = new MR_ClientSession;

		// Load the selected maze
		if(lSuccess) {
			MR_RecordFile *lTrackFile = MR_TrackOpen(mMainWindow, lCurrentTrack.c_str());
			lSuccess = (lCurrentSession->LoadNew(lCurrentTrack.c_str(), lTrackFile, lNbLap, lGameOpts, mVideoBuffer) != FALSE);
		}

		if(lSuccess) {
			lCurrentSession->SetSimulationTime(-8000);

			// Create the main character2
			lSuccess = (lCurrentSession->CreateMainCharacter() != FALSE);
		}

		if(lSuccess) {
			lSuccess = (lCurrentSession->CreateMainCharacter2() != FALSE);
			if(pSplitPlayers > 2)
				lSuccess = (lCurrentSession->CreateMainCharacter3() != FALSE);
			if(pSplitPlayers > 3)
				lSuccess = (lCurrentSession->CreateMainCharacter4() != FALSE);
		}

		if(!lSuccess) {
			// Clean everytings
			Clean();
			delete lCurrentSession;
		}
		else {
			mCurrentSession = lCurrentSession;
			mGameThread = MR_GameThread::New(this);

			if(mGameThread == NULL) {
				mCurrentSession = NULL;
				delete lCurrentSession;
			}
		}
	}

	AssignPalette();
}

void MR_GameApp::NewNetworkSession(BOOL pServer)
{
	bool lSuccess = true;
	MR_NetworkSession *lCurrentSession = NULL;
	Config *cfg = Config::GetInstance();

	// Verify is user acknowledge
	if(AskUserToAbortGame() != IDOK)
		return;

	// Delete the current session
	Clean();

	std::string lCurrentTrack;
	int lNbLap;
	char lGameOpts;
	// Prompt the user for a maze name fbm extensions

	if(pServer) {
		lSuccess = MR_SelectTrack(mMainWindow, lCurrentTrack, lNbLap, lGameOpts);

		DeleteMovieWnd();
		SOUNDSERVER_INIT(mMainWindow);
		lCurrentSession = new MR_NetworkSession(FALSE, -1, -1, mMainWindow);
	}
	else {
		DeleteMovieWnd();
		SOUNDSERVER_INIT(mMainWindow);

		lCurrentSession = new MR_NetworkSession(FALSE, -1, -1, mMainWindow);
		lCurrentSession->SetPlayerName(cfg->player.nickName.c_str());

		CString lTrack;
		lSuccess = (lCurrentSession->PreConnectToServer(mMainWindow, lTrack) != FALSE);
		lCurrentTrack = static_cast<const char*>(lTrack);

		if(cfg->player.nickName != lCurrentSession->GetPlayerName()) {
			cfg->player.nickName = lCurrentSession->GetPlayerName();
			cfg->Save();
		}
		// Extract the lap count from the track name and gameplay options
		// From the end of the string find the two last space
		int lSpaceCount = 0;

		lNbLap = 5;								  // Default
		lGameOpts = 0;

		for(int lCounter = lCurrentTrack.length() - 1; lCounter >= 0; lCounter--) {
			if(lCurrentTrack[lCounter] == ' ') {
				lSpaceCount++;

				if(lSpaceCount == 1) { 
					/* extract crafts allowed */
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 1] == 'B') ? OPT_ALLOW_BASIC : 0;
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 2] == '2') ? OPT_ALLOW_BI : 0;
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 3] == 'C') ? OPT_ALLOW_CX : 0;
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 4] == 'E') ? OPT_ALLOW_EON : 0;
				}

				if(lSpaceCount == 2) {
					/* extract game options */
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 1] == 'W') ? OPT_ALLOW_WEAPONS : 0;
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 2] == 'M') ? OPT_ALLOW_MINES : 0;
					lGameOpts |= ((lCurrentTrack.c_str())[lCounter + 3] == 'C') ? OPT_ALLOW_CANS : 0;
				}

				if(lSpaceCount == 5) {
					lNbLap = atoi(lCurrentTrack.c_str() + lCounter + 1);

					if(lNbLap < 1)
						lNbLap = 5;

					//lCurrentTrack = CString(lCurrentTrack, lCounter);
					lCurrentTrack.resize(lCounter);
					break;
				}
			}
		}
	}

	MR_RecordFile *lTrackFile;
	if(lSuccess) {
		mObserver1 = MR_Observer::New();
		highObserver = new HighObserver();

		// Load the track
		lTrackFile = MR_TrackOpen(mMainWindow, lCurrentTrack.c_str());
		if (lTrackFile == NULL) {
			lSuccess = TrackDownloadDialog(lCurrentTrack).ShowModal(mInstance, mMainWindow);
			if (lSuccess) {
				lTrackFile = MR_TrackOpen(mMainWindow, lCurrentTrack.c_str());
				if (lTrackFile == NULL) {
					lSuccess = FALSE;
				}
			}
		}
	}

	if(lSuccess) {
		lSuccess = (lCurrentSession->LoadNew(lCurrentTrack.c_str(), lTrackFile, lNbLap, lGameOpts, mVideoBuffer) != FALSE);
	}

	if(lSuccess) {
		if(pServer) {
			CString lNameBuffer;

			lNameBuffer.Format("%s %d %s; options %c%c%c, %c%c%c%c", lCurrentTrack.c_str(), lNbLap, lNbLap > 1 ? "laps" : "lap", 
				(lGameOpts & OPT_ALLOW_WEAPONS) ? 'W' : '_',
				(lGameOpts & OPT_ALLOW_MINES)   ? 'M' : '_',
				(lGameOpts & OPT_ALLOW_CANS)    ? 'C' : '_',
				(lGameOpts & OPT_ALLOW_BASIC)   ? 'B' : '_',
				(lGameOpts & OPT_ALLOW_BI)		? '2' : '_',
				(lGameOpts & OPT_ALLOW_CX)		? 'C' : '_',
				(lGameOpts & OPT_ALLOW_EON)		? 'E' : '_');

			// Create a net server
			lCurrentSession->SetPlayerName(cfg->player.nickName.c_str());

			lSuccess = (lCurrentSession->WaitConnections(mMainWindow, lNameBuffer) != FALSE);
			if(cfg->player.nickName != lCurrentSession->GetPlayerName()) {
				cfg->player.nickName = lCurrentSession->GetPlayerName();
				cfg->Save();
			}
		} else
			lSuccess = (lCurrentSession->ConnectToServer(mMainWindow) != FALSE);
	}

	if(lSuccess) {
												  // start in 13 seconds
		lCurrentSession->SetSimulationTime(-13000);
		lSuccess = (lCurrentSession->CreateMainCharacter() != FALSE);
	}

	if(!lSuccess) {
		// Clean everytings
		Clean();
		delete lCurrentSession;
	}
	else {
		if(GetActiveWindow() != mMainWindow) {
			FLASHWINFO lFlash;
			lFlash.cbSize = sizeof(lFlash);
			lFlash.hwnd = mMainWindow;
			lFlash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
			lFlash.uCount = 5;
			lFlash.dwTimeout = 0;

			FlashWindowEx(&lFlash);
		}

		mCurrentSession = lCurrentSession;
		mGameThread = MR_GameThread::New(this);

		if(mGameThread == NULL) {
			mCurrentSession = NULL;
			delete lCurrentSession;
		}
	}

	AssignPalette();
}

/**
 * Reload the control settings.
 * @return The new control settings.
 */
HoverRace::Client::Control::Controller *MR_GameApp::ReloadController()
{
	delete controller;
	return (controller = new HoverRace::Client::Control::Controller(This->mMainWindow));
}

void MR_GameApp::NewInternetSession()
{
	BOOL lSuccess = TRUE;
	MR_NetworkSession *lCurrentSession = NULL;
	Config *cfg = Config::GetInstance();
	MR_InternetRoom lInternetRoom(cfg->net.mainServer, mustCheckUpdates);

	if(mustCheckUpdates)
		mustCheckUpdates = false; // must make sure we only check once

	// Verify is user acknowledge
	if(AskUserToAbortGame() != IDOK)
		return;

	// Delete the current session
	Clean();
	DeleteMovieWnd();
	SOUNDSERVER_INIT(mMainWindow);

	lCurrentSession = new MR_NetworkSession(TRUE, -1, -1, mMainWindow);

	if(lSuccess) {
		lCurrentSession->SetPlayerName(cfg->player.nickName.c_str());

		lSuccess = lInternetRoom.DisplayChatRoom(mMainWindow, lCurrentSession, mVideoBuffer, mServerHasChanged);
		lCurrentSession->SetRoomList(lInternetRoom.GetRoomList());
		mServerHasChanged = FALSE;

		if(cfg->player.nickName != lCurrentSession->GetPlayerName()) {
			cfg->player.nickName = lCurrentSession->GetPlayerName();
			cfg->Save();
		}
	}

	if(lSuccess) {
		// start in 20 seconds (this time may be readjusted by the server)
		lCurrentSession->SetSimulationTime(-20000);
	}

	if(lSuccess) {
		mObserver1 = MR_Observer::New();
		highObserver = new HighObserver();
		lSuccess = lCurrentSession->CreateMainCharacter();
	}

	if(!lSuccess) {
		// Clean everytings
		Clean();
		delete lCurrentSession;
	}
	else {
		mCurrentSession = lCurrentSession;
		mGameThread = MR_GameThread::New(this);

		if(mGameThread == NULL) {
			mCurrentSession = NULL;
			delete lCurrentSession;
		}
	}
	AssignPalette();

}

void MR_GameApp::DrawBackground()
{

	MR_UInt8 *lDest = mVideoBuffer->GetBuffer();
	int lXRes = mVideoBuffer->GetXRes();
	int lYRes = mVideoBuffer->GetYRes();
	int lDestLineStep = mVideoBuffer->GetLineLen() - lXRes;

	int lColorIndex;

	for(int lY = 0; lY < lYRes; lY++) {
		lColorIndex = lY;
		for(int lX = 0; lX < lXRes; lX++) {
			*lDest = (lColorIndex & 16) ? 11 : 11;

			lColorIndex++;
			lDest++;
		}
		lDest += lDestLineStep;
	}
}

void MR_GameApp::UpdateMenuItems()
{
	// Only update the menu if the fullscreen resolution actually changed.
	static int curX = 0;
	static int curY = 0;
	Config *cfg = Config::GetInstance();
	int xres = cfg->video.xResFullscreen;
	int yres = cfg->video.yResFullscreen;
	if (xres == curX && yres == curY) {
		return;
	}
	else {
		curX = xres;
		curY = yres;
	}

	MENUITEMINFOW lMenuInfo;
	memset(&lMenuInfo, 0, sizeof(lMenuInfo));
	lMenuInfo.cbSize = sizeof(lMenuInfo);

	std::ostringstream oss;
	oss.imbue(OS::stdLocale);
	oss << pgettext("Menu|Setting", "&Fullscreen");
	if (xres > 0 && yres > 0) {
		oss << " (" << xres << 'x' << yres << ')';
	}
	oss << "\tF9";
	wchar_t *ws = Str::Utf8ToWide(oss.str().c_str());

	lMenuInfo.fMask = MIIM_STATE | MIIM_STRING;
	lMenuInfo.fState = MFS_ENABLED;
	lMenuInfo.dwTypeData = ws;
	lMenuInfo.cch = wcslen(ws);

	HMENU lMenu = GetMenu(mMainWindow);
	SetMenuItemInfoW(lMenu, ID_SETTING_FULLSCREEN, FALSE, &lMenuInfo);

	OS::Free(ws);
}

LRESULT CALLBACK MR_GameApp::DispatchFunc(HWND pWindow, UINT pMsgId, WPARAM pWParam, LPARAM pLParam)
{
	// Catch environment modification events
	// TRACE( "MESSAGE %d W %d L %d\n", pMsgId, pWParam, pLParam );

	switch (pMsgId) {
		// Catch environment modification events
		/*
		   case WM_SHOWWINDOW:
		   if( pWParam && pLParam == SW_PARENTOPENING )
			   {
			   if( This->mVideoBuffer != NULL )
			   {
			   This->mVideoBuffer->ExitIconMode();
			   }
			   }
			   break;
		 */

		case WM_DISPLAYCHANGE:
			break;

		case WM_SIZE:
		case WM_MOVE:
			This->OnDisplayChange();
			break;

		case WM_QUERYNEWPALETTE:
			This->AssignPalette();
			return TRUE;

		case WM_PALETTECHANGED:
			if(This->mMovieWnd != NULL) {
				MCIWndRealize(This->mMovieWnd, TRUE);
			}
			break;

		case WM_ENTERMENULOOP:
			This->SetVideoMode(0, 0);
			This->UpdateMenuItems();
			break;

		case WM_TIMER:
			switch (pWParam) {
				case MRM_RETURN2WINDOWMODE:
					if (This->fullscreenTest == NULL) {
						KillTimer(pWindow, MRM_RETURN2WINDOWMODE);
					}
					else if (This->fullscreenTest->TickTimer()) {
						KillTimer(pWindow, MRM_RETURN2WINDOWMODE);
						This->SetVideoMode(0, 0);
						SetWindowPos(This->mMainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

						// Patch, sometime the palette is lost??
						if(GetFocus() != NULL) {
							This->AssignPalette();
						}

						delete This->fullscreenTest;
						This->fullscreenTest = NULL;

						This->RestartGameThread();
					}
					else {
						// Re-apply the palette before rendering.
						// (it doesn't always take when we first switch mode).
						This->mVideoBuffer->AssignPalette();

						This->fullscreenTest->Render(This->mVideoBuffer);
					}

					return 0;
			}
			break;

			/* 
			   case WM_STYLECHANGING:
			   if( This->mVideoBuffer != NULL )
				   {
				   STYLESTRUCT* lStyle = (LPSTYLESTRUCT)pLParam;
	
				   if( pWParam == GWL_EXSTYLE )
				   {
				   ASSERT( FALSE );
				   // if( This->mVideoBuffer->IsWindowMode() )
				   if( lStyle->styleNew & WS_EX_TOPMOST )
				   {
				   SetWindowPos( pWindow, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
				   ASSERT( FALSE );
				   // return 0;
				   }
				   // else
				   {
	
				   // lStyle->styleNew &= ~(WS_EX_DLGMODALFRAME|WS_EX_WINDOWEDGE);
				   return 0;
				   }
				   }
				   else if( pWParam == GWL_STYLE )
				   {
				   lStyle->styleNew &= ~(WS_THICKFRAME);
				   return 0;
				   }
				   }
				   break;;
			 */

			/*
			   case WM_SYSCOMMAND:
			   switch( pWParam )
				   {
				   case SC_RESTORE:
				   if( (This->mVideoBuffer != NULL)&&!This->mVideoBuffer->IsWindowMode() )
				   {
				   This->SetVideoMode( 0, 0 );
				   return 0;
				   }
				   }
				   break;
			 */

		case WM_ACTIVATEAPP:
			//TRACE("WM_ACTIVATE %d\n", pWParam);

			if(pWParam && (This->mVideoBuffer != NULL) && (This->mMainWindow == GetForegroundWindow())) {
				if(!This->mVideoBuffer->IsModeSettingInProgress()) {
					if(This->mVideoBuffer->IsWindowMode()) {
						//TRACE("SetMode\n");

						This->SetVideoMode(0, 0);
						This->AssignPalette();
						return 0;
					}
				}
			}

			break;

		case WM_SETCURSOR:
			if((This->mVideoBuffer != NULL) && !This->mVideoBuffer->IsWindowMode()) {
				SetCursor(NULL);
				return TRUE;
			}
			break;

			// Menu options
		case WM_COMMAND:
			switch (LOWORD(pWParam)) {
				// Game control
				case ID_APP_EXIT:
					PostMessage(This->mMainWindow, WM_CLOSE, 0, 0);
					break;

				case ID_GAME_PAUSE:
					This->SetVideoMode(0, 0);
					return 0;

				case ID_GAME_NEW:
					This->SetVideoMode(0, 0);
					This->NewLocalSession();
					return 0;

				case ID_GAME_SPLITSCREEN:
					This->SetVideoMode(0, 0);
					This->NewSplitSession(2);
					return 0;

				case ID_GAME_SPLITSCREEN3:
					This->SetVideoMode(0, 0);
					This->NewSplitSession(3);
					return 0;

				case ID_GAME_SPLITSCREEN4:
					This->SetVideoMode(0, 0);
					This->NewSplitSession(4);
					return 0;

				case ID_GAME_NETWORK_SERVER:
					This->SetVideoMode(0, 0);
					This->NewNetworkSession(TRUE);
					return 0;

				case ID_GAME_NETWORK_CONNECT:
					This->SetVideoMode(0, 0);
					This->NewNetworkSession(FALSE);
					return 0;

				case ID_GAME_NETWORK_INTERNET:
					This->SetVideoMode(0, 0);
					// Registration key code checking removed, no longer necessary
					This->NewInternetSession();
					return 0;

				case ID_SETTING_REFRESHCOLORS:
					if(GetFocus() != NULL)
						This->AssignPalette();
					break;

				// Video mode setting
				case ID_SETTING_FULLSCREEN:
					{
						Config *cfg = Config::GetInstance();
						This->SetVideoMode(cfg->video.xResFullscreen, cfg->video.yResFullscreen, &cfg->video.fullscreenMonitor);
					}
					return 0;

				case ID_SETTING_PROPERTIES:
					This->DisplayPrefs();
					break;

				case ID_SETTING_WINDOW:
					This->SetVideoMode(0, 0);
					return 0;

				case ID_VIEW_3DACTION:
					This->mCurrentMode = e3DView;
					if(This->mObserver1 != NULL) {
						This->mObserver1->SetCockpitView(FALSE);
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->SetCockpitView(FALSE);
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->SetCockpitView(FALSE);
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->SetCockpitView(FALSE);
					}
					return 0;

				case ID_VIEW_COCKPIT:
					This->mCurrentMode = e3DView;
					if(This->mObserver1 != NULL) {
						This->mObserver1->SetCockpitView(TRUE);
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->SetCockpitView(TRUE);
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->SetCockpitView(TRUE);
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->SetCockpitView(TRUE);
					}
					return 0;

				case ID_VIEW_DEBUG:
					This->mCurrentMode = eDebugView;
					return 0;

					/* DISABLED BY AUSTIN: No need for the +/- to be used.

					   case ID_VIEW_LARGER:
					   if( This->mObserver1 != NULL )
						   {
						   This->mObserver1->ReduceMargin();
						   }
						   if( This->mObserver2 != NULL )
						   {
						   This->mObserver2->ReduceMargin();
						   }
						   if( This->mObserver3 != NULL )
						   {
						   This->mObserver3->ReduceMargin();
						   }
						   if( This->mObserver4 != NULL )
						   {
						   This->mObserver4->ReduceMargin();
						   }
						   return 0;
	
						   case ID_VIEW_SMALLER:
						   if( This->mObserver1 != NULL )
						   {
						   This->mObserver1->EnlargeMargin();
						   }
						   if( This->mObserver2 != NULL )
						   {
						   This->mObserver2->EnlargeMargin();
						   }
						   if( This->mObserver3 != NULL )
						   {
						   This->mObserver3->EnlargeMargin();
						   }
						   if( This->mObserver4 != NULL )
						   {
						   This->mObserver4->EnlargeMargin();
						   }
						   This->mClrScrTodo = 2;
						   return 0;
						 */
	
					case ID_VIEW_PLAYERSLIST:
						if(This->mObserver1 != NULL) {
							This->mObserver1->PlayersListPageDn();
						}
						if(This->mObserver2 != NULL) {
							This->mObserver2->PlayersListPageDn();
						}
						if(This->mObserver3 != NULL) {
							This->mObserver3->PlayersListPageDn();
						}
						if(This->mObserver4 != NULL) {
							This->mObserver4->PlayersListPageDn();
						}
						return 0;
	
					case ID_VIEW_MOREMESSAGES:
						if(This->mObserver1 != NULL) {
							This->mObserver1->MoreMessages();
						}
						if(This->mObserver2 != NULL) {
							This->mObserver2->MoreMessages();
						}
						if(This->mObserver3 != NULL) {
							This->mObserver3->MoreMessages();
						}
						if(This->mObserver4 != NULL) {
							This->mObserver4->MoreMessages();
						}
						return 0;
	
				case ID_HELP_CONTENTS:
					This->DisplayHelp();
					break;

				case ID_HELP_SITE:
					This->DisplaySite();
					break;

				case ID_HELP_UPDATES:
					// don't check if we are a homebuilt version
					if(Config::GetInstance()->GetBuild() != 0) {
						CheckUpdateServerDialog(Config::GetInstance()->net.updateServer).ShowModal(NULL, This->mMainWindow);
					} else {
						MessageBoxW(This->mMainWindow,
							Str::UW(_("This build of HoverRace is not official; updates are not available.")),
							PACKAGE_NAME_L,
							MB_ICONEXCLAMATION);
					}
					break;

				case ID_APP_ABOUT:
					This->DisplayAbout();
					break;

			}
			break;

		case WM_CHAR:
			This->OnChar((char)pWParam);
			return 0;

		case WM_KEYDOWN:
			switch (pWParam) {
				// Camera control
				case VK_HOME:
					if(This->mObserver1 != NULL) {
						This->mObserver1->Home();
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->Home();
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->Home();
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->Home();
					}
					return 0;

				case VK_PRIOR:
					if(This->mObserver1 != NULL) {
						This->mObserver1->Scroll(1);
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->Scroll(1);
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->Scroll(1);
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->Scroll(1);
					}
					return 0;

				case VK_NEXT:
					if(This->mObserver1 != NULL) {
						This->mObserver1->Scroll(-1);
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->Scroll(-1);
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->Scroll(-1);
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->Scroll(-1);
					}
					return 0;

				case VK_INSERT:
					if(This->mObserver1 != NULL) {
						This->mObserver1->ZoomIn();
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->ZoomIn();
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->ZoomIn();
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->ZoomIn();
					}
					return 0;

				case VK_DELETE:
					if(This->mObserver1 != NULL) {
						This->mObserver1->ZoomOut();
					}
					if(This->mObserver2 != NULL) {
						This->mObserver2->ZoomOut();
					}
					if(This->mObserver3 != NULL) {
						This->mObserver3->ZoomOut();
					}
					if(This->mObserver4 != NULL) {
						This->mObserver4->ZoomOut();
					}
					return 0;

					// Debug keys
					/*
					   case VK_F8:
					   #ifdef MR_AVI_CAPTURE
					   InitCapture( "demo.avi", This->mVideoBuffer );
					   #endif
					   This->mNbFrames = 0;
					   This->mNbFramesStartingTime = time( NULL );
					   return 0;

					   case VK_F9:
					   #ifdef MR_AVI_CAPTURE
					   CloseCapture();
					   #endif

					   TRACE( "Refresh rate = %d f/s( %d/%d)\n",
					   (int)((double)This->mNbFrames/(double(time(NULL)-This->mNbFramesStartingTime))),
					   This->mNbFrames,
					   (time(NULL)-This->mNbFramesStartingTime) );
					   return 0;
					 */

			}
			break;

		case WM_PAINT:
			// Nothing to paint (all done by video or DirectX)
			PAINTSTRUCT lPs;
			BeginPaint(pWindow, &lPs);
			EndPaint(pWindow, &lPs);
			return 0;

		case WM_CLOSE:
			if(This->IsGameRunning()) {
				This->SetVideoMode(0, 0);
				if(This->AskUserToAbortGame() != IDOK) {
					return 0;
				}
			}
			This->Clean();
			delete This->mVideoBuffer;
			This->mVideoBuffer = NULL;
			// save resolution information
			{
				Config *cfg = Config::GetInstance();
				RECT size = {0};
				GetWindowRect(This->mMainWindow, &size);
				cfg->video.xPos = size.left;
				cfg->video.yPos = size.top;
				cfg->video.xRes = size.right - size.left;
				cfg->video.yRes = size.bottom - size.top;
				cfg->Save();
			}
			DestroyWindow(This->mMainWindow);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

	}

	// Default return value
	return DefWindowProcW(pWindow, pMsgId, pWParam, pLParam);
}

BOOL CALLBACK MR_GameApp::BadModeDialogFunc(HWND pWindow, UINT pMsgId, WPARAM pWParam, LPARAM pLParam)
{
	// theoretically this dialog should never be shown
	switch (pMsgId) {
		// Catch environment modification events
		case WM_INITDIALOG:
			// i18n
			SetDlgItemTextW(pWindow, IDC_STATIC, Str::UW(_("Incompatible video mode. Select a compatible video mode by pressing F8 or F9 once a game is started")));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(pWParam)) {
				// Game control
				case IDC_FULL_MENU_MODE:
					if(This->mGameThread == NULL) {
						// This->SetVideoMode( 320, 200 );
						// This->mVideoBuffer->PushMenuMode();
					}
					else {
						This->SetVideoMode(320, 200);
						// This->mVideoBuffer->PushMenuMode();
					}
					break;
			}

	}

	return FALSE;
}

// Microsoft bitmap stuff
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPSTR lpString, HPALETTE FAR * lphPalette)
{
	HRSRC hRsrc;
	HGLOBAL hGlobal;
	HBITMAP hBitmapFinal = NULL;
	LPBITMAPINFOHEADER lpbi;
	HDC hdc;
	int iNumColors;

	if(hRsrc = FindResource(hInstance, lpString, RT_BITMAP)) {
		hGlobal = LoadResource(hInstance, hRsrc);
		lpbi = (LPBITMAPINFOHEADER) LockResource(hGlobal);

		hdc = GetDC(NULL);
		*lphPalette = CreateDIBPalette((LPBITMAPINFO) lpbi, &iNumColors);
		if(*lphPalette) {
			SelectPalette(hdc, *lphPalette, FALSE);
			RealizePalette(hdc);
		}

		hBitmapFinal = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER) lpbi, (LONG) CBM_INIT, (LPSTR) lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD), (LPBITMAPINFO) lpbi, DIB_RGB_COLORS);

		ReleaseDC(NULL, hdc);
		UnlockResource(hGlobal);
		FreeResource(hGlobal);
	}
	return (hBitmapFinal);
}

HPALETTE CreateDIBPalette(LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
	LPBITMAPINFOHEADER lpbi;
	LPLOGPALETTE lpPal;
	HANDLE hLogPal;
	HPALETTE hPal = NULL;
	int i;

	lpbi = (LPBITMAPINFOHEADER) lpbmi;
	if(lpbi->biBitCount <= 8)
		*lpiNumColors = (1 << lpbi->biBitCount);
	else
		*lpiNumColors = 0;						  // No palette needed for 24 BPP DIB

	if(*lpiNumColors) {
		hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * (*lpiNumColors));
		lpPal = (LPLOGPALETTE) GlobalLock(hLogPal);
		lpPal->palVersion = 0x300;
		lpPal->palNumEntries = *lpiNumColors;

		for(i = 0; i < *lpiNumColors; i++) {
			lpPal->palPalEntry[i].peRed = lpbmi->bmiColors[i].rgbRed;
			lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
			lpPal->palPalEntry[i].peBlue = lpbmi->bmiColors[i].rgbBlue;
			lpPal->palPalEntry[i].peFlags = 0;
		}
		hPal = CreatePalette(lpPal);
		GlobalUnlock(hLogPal);
		GlobalFree(hLogPal);
	}
	return hPal;
}

/////////////////////////////////   AVI SECTION  //////////////////////////////////////

#ifdef MR_AVI_CAPTURE

void InitCapture(const char *pFileName, MR_VideoBuffer * pVideoBuffer)
{

	AVISTREAMINFO lStreamInfo;

	CloseCapture();
	AVIFileInit();

	gCaptureFrameNo = 0;

	// Create new AVI file.
	AVIFileOpen(&gCaptureFile, pFileName, OF_WRITE | OF_CREATE, NULL);

	// Set parameters for the new stream
	lStreamInfo.fccType = streamtypeVIDEO;
	lStreamInfo.fccHandler = NULL;
	lStreamInfo.dwFlags = 0;
	lStreamInfo.dwCaps = 0;
	lStreamInfo.wPriority = 0;
	lStreamInfo.wLanguage = 0;
	lStreamInfo.dwScale = 1;
	lStreamInfo.dwRate = gCaptureFrameRate;
	lStreamInfo.dwStart = 0;
	lStreamInfo.dwLength = 30;					  // ?
	lStreamInfo.dwInitialFrames = 0;
	lStreamInfo.dwSuggestedBufferSize = pVideoBuffer->GetXRes() * pVideoBuffer->GetYRes() * 4;
	lStreamInfo.dwQuality = -1;
	lStreamInfo.dwSampleSize = pVideoBuffer->GetXRes() * pVideoBuffer->GetYRes();
	SetRect(&lStreamInfo.rcFrame, 0, 0, pVideoBuffer->GetXRes(), pVideoBuffer->GetYRes());
	lStreamInfo.dwEditCount = 0;
	lStreamInfo.dwFormatChangeCount = 0;
	strcpy(lStreamInfo.szName, "(C)GrokkSoft 1996");

	// Create a stream.
	AVIFileCreateStream(gCaptureFile, &gCaptureStream, &lStreamInfo);

	// Set format of new stream.
	int lInfoSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (MR_NB_COLORS - MR_RESERVED_COLORS);

	BITMAPINFO *lBitmapInfo = (BITMAPINFO *) new char[lInfoSize];

	lBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lBitmapInfo->bmiHeader.biWidth = pVideoBuffer->GetXRes();
	lBitmapInfo->bmiHeader.biHeight = pVideoBuffer->GetYRes();
	lBitmapInfo->bmiHeader.biPlanes = 1;
	lBitmapInfo->bmiHeader.biBitCount = 8;
	lBitmapInfo->bmiHeader.biCompression = BI_RGB;
	lBitmapInfo->bmiHeader.biSizeImage = pVideoBuffer->GetXRes() * pVideoBuffer->GetYRes();
	lBitmapInfo->bmiHeader.biXPelsPerMeter = 2048;
	lBitmapInfo->bmiHeader.biYPelsPerMeter = 2048;
	lBitmapInfo->bmiHeader.biClrUsed = MR_NB_COLORS - MR_RESERVED_COLORS;
	lBitmapInfo->bmiHeader.biClrImportant = 0;

	// Create the palette
	PALETTEENTRY *lOurEntries = MR_GetColors(0.75, 0.75, 0.05);

	for(int lCounter = 0; lCounter < MR_NB_COLORS - MR_RESERVED_COLORS; lCounter++) {
		lBitmapInfo->bmiColors[lCounter].rgbRed = lOurEntries[lCounter].peRed;
		lBitmapInfo->bmiColors[lCounter].rgbGreen = lOurEntries[lCounter].peGreen;
		lBitmapInfo->bmiColors[lCounter].rgbBlue = lOurEntries[lCounter].peBlue;
		lBitmapInfo->bmiColors[lCounter].rgbReserved = 0;
	};
	delete lOurEntries;

	/*
	   for( lCounter = 0; lCounter<MR_RESERVED_COLORS; lCounter++ )
	   {
	   lBitmapInfo->bmiColors[ lCounter ].rgbRed   = 1;
	   lBitmapInfo->bmiColors[ lCounter ].rgbGreen = lCounter;
	   lBitmapInfo->bmiColors[ lCounter ].rgbBlue  = 0;
	   lBitmapInfo->bmiColors[ lCounter ].rgbReserved = 0;
	   };
	 */

	AVIStreamSetFormat(gCaptureStream, 0, lBitmapInfo, lInfoSize);

	delete[](char *) lBitmapInfo;
}

void CloseCapture()
{
	if(gCaptureStream != NULL) {
		AVIStreamRelease(gCaptureStream);
		AVIFileRelease(gCaptureFile);

		gCaptureStream = NULL;
		gCaptureFile = NULL;

		AVIFileExit();

	}
}

void CaptureScreen(MR_VideoBuffer * pVideoBuffer)
{

	if(gCaptureStream != NULL) {
		// Create a ttemporaary buffer for the image
		int lXRes = pVideoBuffer->GetXRes();
		int lYRes = pVideoBuffer->GetYRes();

		MR_UInt8 *lBuffer = new MR_UInt8[lXRes * lYRes];

		MR_UInt8 *lDest = lBuffer;

		for(int lY = 0; lY < lYRes; lY++) {
			MR_UInt8 *lSrc = pVideoBuffer->GetBuffer() + (lYRes - lY - 1) * pVideoBuffer->GetLineLen();

			for(int lX = 0; lX < lXRes; lX++) {
				*lDest = *lSrc - MR_RESERVED_COLORS;
				lDest++;
				lSrc++;
			}
		}

		// Append the bitmap to the AVI file
		AVIStreamWrite(gCaptureStream, gCaptureFrameNo++, 1, lBuffer, lXRes * lYRes, AVIIF_KEYFRAME, NULL, NULL);

		delete[]lBuffer;

	}
}
#endif
