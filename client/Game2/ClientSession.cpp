// ClientSession.cpp
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

#include "../../engine/MainCharacter/MainCharacter.h"
#include "../../engine/VideoServices/VideoBuffer.h"
#include "../../compilers/MazeCompiler/TrackCommonStuff.h"

#include "ClientSession.h"

using HoverRace::Util::OS;

namespace HoverRace {
namespace Client {

ClientSession::ClientSession() :
	mSession(TRUE),
	frameCount(0), lastTimestamp(0), fps(0.0)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		mainCharacter[i] = NULL;
	}
	mBackImage = NULL;
	mMap = NULL;
	mNbLap = 1;
	mGameOpts = 0;

	InitializeCriticalSection(&mChatMutex);
}

ClientSession::~ClientSession()
{
	delete[]mBackImage;
	delete mMap;

	DeleteCriticalSection(&mChatMutex);
}

void ClientSession::Process(int pSpeedFactor)
{
	mSession.Simulate();
}

void ClientSession::ReadLevelAttrib(MR_RecordFile * pRecordFile, MR_VideoBuffer * pVideo)
{
	// Read level background palette
	if((pVideo != NULL) && (pRecordFile->GetNbRecords() >= 3)) {
		pRecordFile->SelectRecord(2);

		{
			CArchive lArchive(pRecordFile, CArchive::load | CArchive::bNoFlushOnDelete);

			int lImageType;

			lArchive >> lImageType;

			if(lImageType == MR_RAWBITMAP) {
				MR_UInt8 *lPalette = new MR_UInt8[MR_BACK_COLORS * 3];

				if(mBackImage == NULL) {
					mBackImage = new MR_UInt8[MR_BACK_X_RES * MR_BACK_Y_RES];
				}

				lArchive.Read(lPalette, MR_BACK_COLORS * 3);
				lArchive.Read(mBackImage, MR_BACK_X_RES * MR_BACK_Y_RES);

				pVideo->SetBackPalette(lPalette);
			}
		}
	}
	// Read map section
	if(pRecordFile->GetNbRecords() >= 4) {
		pRecordFile->SelectRecord(3);
		{
			CArchive lArchive(pRecordFile, CArchive::load | CArchive::bNoFlushOnDelete);

			int lX0;
			int lX1;
			int lY0;
			int lY1;

			MR_Sprite *lMapSprite = new MR_Sprite;

			lArchive >> lX0;
			lArchive >> lX1;
			lArchive >> lY0;
			lArchive >> lY1;

			lMapSprite->Serialize(lArchive);

			SetMap(lMapSprite, lX0, lY0, lX1, lY1);
		}
	}
	// Read level midi stream
	if(pRecordFile->GetNbRecords() >= 5) {
		pRecordFile->SelectRecord(4);
		{
			// TODO
		}
	}
}

BOOL ClientSession::LoadNew(const char *pTitle, MR_RecordFile *pMazeFile,
                            int pNbLap, char pGameOpts, MR_VideoBuffer *pVideo)
{
	BOOL lReturnValue;
	mNbLap = pNbLap;
	mGameOpts = pGameOpts;
	lReturnValue = mSession.LoadNew(pTitle, pMazeFile, pGameOpts);

	if(lReturnValue) {
		ReadLevelAttrib(pMazeFile, pVideo);
	}

	return lReturnValue;
}

const MR_UInt8 *ClientSession::GetBackImage() const
{
	return mBackImage;
}

// Main character control and interrogation

bool ClientSession::CreateMainCharacter(int i)
{
	ASSERT(mainCharacter[i] == NULL);

	MR_Level *curLevel = mSession.GetCurrentLevel();
	ASSERT(curLevel != NULL);

	MR_MainCharacter *ch = mainCharacter[i] = MR_MainCharacter::New(mNbLap, mGameOpts);

	int startingRoom = curLevel->GetStartingRoom(i);
	ch->mRoom = startingRoom;
	ch->mPosition = curLevel->GetStartingPos(i);
	ch->SetOrientation(curLevel->GetStartingOrientation(i));
	ch->SetHoverId(i);

	curLevel->InsertElement(ch, startingRoom);

	return true;
}

/** @deprecated Use CreateMainCharacter(int) instead. */
BOOL ClientSession::CreateMainCharacter()
{
	return CreateMainCharacter(0);
}

/** @deprecated Use CreateMainCharacter(int) instead. */
BOOL ClientSession::CreateMainCharacter2()
{
	return CreateMainCharacter(1);
}

/** @deprecated Use CreateMainCharacter(int) instead. */
BOOL ClientSession::CreateMainCharacter3()
{
	return CreateMainCharacter(2);
}

/** @deprecated Use CreateMainCharacter(int) instead. */
BOOL ClientSession::CreateMainCharacter4()
{
	return CreateMainCharacter(3);
}

/** @deprecated Use GetPlayer(int) instead. */
MR_MainCharacter *ClientSession::GetMainCharacter() const
{
	return const_cast<MR_MainCharacter*>(GetPlayer(0));
}

/** @deprecated Use GetPlayer(int) instead. */
MR_MainCharacter *ClientSession::GetMainCharacter2() const
{
	return const_cast<MR_MainCharacter*>(GetPlayer(1));
}

/** @deprecated Use GetPlayer(int) instead. */
MR_MainCharacter *ClientSession::GetMainCharacter3() const
{
	return const_cast<MR_MainCharacter*>(GetPlayer(2));
}

/** @deprecated Use GetPlayer(int) instead. */
MR_MainCharacter *ClientSession::GetMainCharacter4() const
{
	return const_cast<MR_MainCharacter*>(GetPlayer(3));
}

void ClientSession::SetSimulationTime(MR_SimulationTime pTime)
{
	mSession.SetSimulationTime(pTime);
}

MR_SimulationTime ClientSession::GetSimulationTime() const
{
	return mSession.GetSimulationTime();
}

void ClientSession::SetControlState(int pState1, int pState2, int pState3, int pState4)
{
	if(mainCharacter[0] != NULL) {
		mainCharacter[0]->SetControlState(pState1, mSession.GetSimulationTime());
	}

	if(mainCharacter[1] != NULL) {
		mainCharacter[1]->SetControlState(pState2, mSession.GetSimulationTime());
	}

	if(mainCharacter[2] != NULL) {
		mainCharacter[2]->SetControlState(pState3, mSession.GetSimulationTime());
	}

	if(mainCharacter[3] != NULL) {
		mainCharacter[3]->SetControlState(pState4, mSession.GetSimulationTime());
	}
}

const MR_Level *ClientSession::GetCurrentLevel() const
{
	MR_GameSession *lSession = (MR_GameSession *) & mSession;

	return lSession->GetCurrentLevel();
}

int ClientSession::ResultAvaillable() const
{
	return 0;
}

/** @deprecated */
void ClientSession::GetResult(int, const char *&pPlayerName, int &, BOOL &,
                              int &, MR_SimulationTime &, MR_SimulationTime &) const
{
	pPlayerName = "?";
	ASSERT(FALSE);
}

/** @deprecated */
void ClientSession::GetHitResult(int pPosition, const char *&pPlayerName,
                                 int &pId, BOOL & pConnected, int &pNbHitOther, int &pNbHitHimself) const
{
	pPlayerName = "?";
	ASSERT(FALSE);
}

int ClientSession::GetNbPlayers() const
{
	int retv = 0;

	for (int i = 0; i < MAX_PLAYERS; ++i)
		if (mainCharacter[i] != NULL)
			++retv;

	return retv;
}

int ClientSession::GetRank(const MR_MainCharacter *pPlayer) const
{
	int lReturnValue = 1;

	MR_MainCharacter *mMainCharacter1 = mainCharacter[0];
	MR_MainCharacter *mMainCharacter2 = mainCharacter[1];
	MR_MainCharacter *mMainCharacter3 = mainCharacter[2];
	MR_MainCharacter *mMainCharacter4 = mainCharacter[3];

	if(mMainCharacter1 != NULL) {
		if(pPlayer == mMainCharacter1) {
			if(mMainCharacter2->HasFinish()) {
				if(mMainCharacter2->GetTotalTime() < mMainCharacter1->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter3->HasFinish()) {
				if(mMainCharacter3->GetTotalTime() < mMainCharacter1->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter4->HasFinish()) {
				if(mMainCharacter4->GetTotalTime() < mMainCharacter1->GetTotalTime()) {
					lReturnValue++;
				}
			}
		}
		if(pPlayer == mMainCharacter2) {
			lReturnValue = 1;

			if(mMainCharacter1->HasFinish()) {
				if(mMainCharacter1->GetTotalTime() < mMainCharacter2->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter3->HasFinish()) {
				if(mMainCharacter3->GetTotalTime() < mMainCharacter2->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter4->HasFinish()) {
				if(mMainCharacter4->GetTotalTime() < mMainCharacter2->GetTotalTime()) {
					lReturnValue++;
				}
			}
		}
		if(pPlayer == mMainCharacter3) {
			lReturnValue = 1;

			if(mMainCharacter1->HasFinish()) {
				if(mMainCharacter1->GetTotalTime() < mMainCharacter3->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter2->HasFinish()) {
				if(mMainCharacter2->GetTotalTime() < mMainCharacter3->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter4->HasFinish()) {
				if(mMainCharacter4->GetTotalTime() < mMainCharacter3->GetTotalTime()) {
					lReturnValue++;
				}
			}
		}
		if(pPlayer == mMainCharacter4) {
			lReturnValue = 1;

			if(mMainCharacter1->HasFinish()) {
				if(mMainCharacter1->GetTotalTime() < mMainCharacter4->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter2->HasFinish()) {
				if(mMainCharacter2->GetTotalTime() < mMainCharacter4->GetTotalTime()) {
					lReturnValue++;
				}
			}
			if(mMainCharacter3->HasFinish()) {
				if(mMainCharacter3->GetTotalTime() < mMainCharacter4->GetTotalTime()) {
					lReturnValue++;
				}
			}
		}
	}
	return lReturnValue;
}

void ClientSession::SetMap(MR_Sprite * pMap, int pX0, int pY0, int pX1, int pY1)
{
	delete mMap;

	mMap = pMap;

	mX0Map = pX0;
	mY0Map = pY0;
	mWidthMap = pX1 - pX0;
	mHeightMap = pY1 - pY0;

	mWidthSprite = mMap->GetItemWidth();
	mHeightSprite = mMap->GetItemHeight();

}

const MR_Sprite *ClientSession::GetMap() const
{
	return mMap;
}

void ClientSession::ConvertMapCoordinate(int &pX, int &pY, int pRatio) const
{
	pX = (pX - mX0Map) * mWidthSprite / (mWidthMap * pRatio);
	pY = (mHeightSprite - 1 - (pY - mY0Map) * mHeightSprite / mHeightMap) / pRatio;
}

const MR_MainCharacter *ClientSession::GetPlayer(int i) const
{
	ASSERT(i >= 0 && i < MAX_PLAYERS);
	return mainCharacter[i];
}

void ClientSession::AddMessageKey(char /*pKey */ )
{

}

void ClientSession::GetCurrentMessage(char *pDest) const
{
	pDest[0] = 0;
}

BOOL ClientSession::GetMessageStack(int pLevel, char *pDest, int pExpiration) const
{
	BOOL lReturnValue = FALSE;

	if(pLevel < CHAT_MESSAGE_STACK) {
		EnterCriticalSection(&((ClientSession *) this)->mChatMutex);

		if(((mMessageStack[pLevel].mCreationTime + pExpiration) > time(NULL)) &&
			(mMessageStack[pLevel].mBuffer.length() > 0))
		{
			lReturnValue = TRUE;
			strcpy(pDest, mMessageStack[pLevel].mBuffer.c_str());
		}
		LeaveCriticalSection(&((ClientSession *) this)->mChatMutex);
	}

	return lReturnValue;
}

void ClientSession::AddMessage(const char *pMessage)
{
	EnterCriticalSection(&mChatMutex);

	for(int lCounter = CHAT_MESSAGE_STACK - 1; lCounter > 0; lCounter--) {
		mMessageStack[lCounter] = mMessageStack[lCounter - 1];
	}

	mMessageStack[0].mCreationTime = time(NULL);

	mMessageStack[0].mBuffer = Ascii2Simple(pMessage);

	LeaveCriticalSection(&mChatMutex);

}

/**
 * Increment the frame counter for stats purposes.
 * This should be called once after rendering each frame.
 */
void ClientSession::IncFrameCount()
{
	// Don't start counting until the first frame.
	if (lastTimestamp == 0) lastTimestamp = OS::Time();

	++frameCount;
	OS::timestamp_t curTimestamp = OS::Time();
	OS::timestamp_t diff = OS::TimeDiff(curTimestamp, lastTimestamp);

	if (diff > 1000) {
		fps = ((double)frameCount) / (diff / 1000.0);
		lastTimestamp = curTimestamp;
		frameCount = 0;
		/*
		OutputDebugString(boost::str(boost::format("FPS: %0.2f\n") % fps).c_str());
		*/
	}
}

/**
 * Retrieve the current framerate.
 * The framerate is calculated about once per second, as long as IncFrameCount
 * is called.
 * @return The framerate in frames per second (may be zero if a second
 *         hasn't passed yet).
 */
double ClientSession::GetCurrentFramerate() const
{
	return fps;
}

}  // namespace Client
}  // namespace HoverRace
