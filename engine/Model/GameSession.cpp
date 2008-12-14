// GameSession.cpp
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

#include <Mmsystem.h>

#include "GameSession.h"
#include "FreeElementMovingHelper.h"

#define MR_SIMULATION_SLICE             15
#define MR_MINIMUM_SIMULATION_SLICE     10

MR_GameSession::MR_GameSession(BOOL pAllowRendering)
{
	mAllowRendering = pAllowRendering;
	mCurrentMazeFile = NULL;
	mCurrentLevel = NULL;
	mCurrentLevelNumber = -1;

	mSimulationTime = -3000;					  // 3 sec countdown
}

MR_GameSession::~MR_GameSession()
{
	Clean();
}

BOOL MR_GameSession::LoadLevel(int pLevel)
{
	ASSERT(mCurrentMazeFile != NULL);

	BOOL lReturnValue = TRUE;

	// delete the current level
	delete mCurrentLevel;
	mCurrentLevelNumber = -1;

	// Load the new level
	if(pLevel < mCurrentMazeFile->GetNbRecords()) {
		mCurrentLevel = new MR_Level(mAllowRendering);
		mCurrentMazeFile->SelectRecord(pLevel);

		CArchive lArchive(mCurrentMazeFile, CArchive::load);

		mCurrentLevel->Serialize(lArchive);

		mCurrentLevelNumber = pLevel;
	} else
	lReturnValue = FALSE;

	return lReturnValue;
}

void MR_GameSession::Clean()
{
	delete mCurrentMazeFile;
	mCurrentMazeFile = NULL;

	delete mCurrentLevel;
	mCurrentLevel = NULL;

	mCurrentMazeName = "";
	mCurrentLevelNumber = -1;
}

BOOL MR_GameSession::LoadNew(const char *pTitle, MR_RecordFile * pMazeFile)
{
	BOOL lReturnValue = FALSE;

	Clean();
	if(pMazeFile != NULL) {
		mTitle = pTitle;
		mCurrentMazeFile = pMazeFile;
		lReturnValue = LoadLevel(1);

		if(!lReturnValue)
			Clean();
	}

	return lReturnValue;
}

void MR_GameSession::SetSimulationTime(MR_SimulationTime pTime)
{
	mSimulationTime = pTime;
	mLastSimulateCallTime = timeGetTime();
}

MR_SimulationTime MR_GameSession::GetSimulationTime() const
{
	return mSimulationTime;
} void MR_GameSession::Simulate()
{
	ASSERT(mCurrentLevel != NULL);

	DWORD lSimulateCallTime = timeGetTime();
	MR_SimulationTime lTimeToSimulate;

	// Determine the duration of the simulation step
	lTimeToSimulate = lSimulateCallTime - mLastSimulateCallTime;

	if(lTimeToSimulate < 0)
		lTimeToSimulate = 0;

	/*
	   if( (lTimeToSimulate < 0 )||(lTimeToSimulate>500) )
	   {
	   // There is someting wrong with the calculated duration
	   // This is probably because the game have been paused
	   // (or the refresh rate is extrenemly slow, less than 2 frames sec)

	   // Use a relativly small time step
	   lTimeToSimulate = 20; // 2/100 of second
	   }
	 */

	while(lTimeToSimulate >= MR_SIMULATION_SLICE) {
		SimulateFreeElems(mSimulationTime < 0 ? 0 : MR_SIMULATION_SLICE);
		lTimeToSimulate -= MR_SIMULATION_SLICE;
		mSimulationTime += MR_SIMULATION_SLICE;
	}

	if(lTimeToSimulate >= MR_MINIMUM_SIMULATION_SLICE) {
		SimulateFreeElems(mSimulationTime < 0 ? 0 : lTimeToSimulate);
		mSimulationTime += lTimeToSimulate;
		lTimeToSimulate = 0;
	}

	SimulateSurfaceElems(lSimulateCallTime - lTimeToSimulate - mLastSimulateCallTime);

	mLastSimulateCallTime = lSimulateCallTime - lTimeToSimulate;
}

void MR_GameSession::SimulateLateElement(MR_FreeElementHandle pElement, MR_SimulationTime pDuration, int pRoom)
{
	ASSERT(mCurrentLevel != NULL);

	MR_SimulationTime lTimeToSimulate = pDuration;

	if(lTimeToSimulate < 0) {
		return;
	}

	if(mSimulationTime <= lTimeToSimulate) {
		// can't backtrack below 0
		return;
	}
	// clock back
	MR_SimulationTime lOriginalTime = mSimulationTime;
	mSimulationTime -= lTimeToSimulate;

	while((pRoom >= 0) && (lTimeToSimulate >= MR_SIMULATION_SLICE)) {
		pRoom = SimulateOneFreeElem(MR_SIMULATION_SLICE, pElement, pRoom);
		lTimeToSimulate -= MR_SIMULATION_SLICE;
		mSimulationTime += MR_SIMULATION_SLICE;
	}

	if((pRoom >= 0) && (lTimeToSimulate >= MR_MINIMUM_SIMULATION_SLICE)) {
		pRoom = SimulateOneFreeElem(lTimeToSimulate, pElement, pRoom);
		SimulateFreeElems(mSimulationTime < 0 ? 0 : lTimeToSimulate);
	}
	// return to good time
	mSimulationTime = lOriginalTime;
}

void MR_GameSession::SimulateSurfaceElems(MR_SimulationTime /*pTimeToSimulate */ )
{
	// Give the control to each surface so they can update there state

	// TODO but it's a big job that will slow the simulation (find a better way)

}

int MR_GameSession::SimulateOneFreeElem(MR_SimulationTime pTimeToSimulate, MR_FreeElementHandle pElementHandle, int pRoom)
{
	BOOL lDeleteElem = FALSE;
	MR_FreeElement *lElement = mCurrentLevel->GetFreeElement(pElementHandle);

	// Ask the element to simulate its movement
	int lNewRoom = lElement->Simulate(pTimeToSimulate, mCurrentLevel, pRoom);
	int lReturnValue = lNewRoom;

	if(lNewRoom == MR_Level::eMustBeDeleted) {
		lDeleteElem = TRUE;
		lNewRoom = pRoom;
	}

	if(pRoom != lNewRoom)
		mCurrentLevel->MoveElement(pElementHandle, lNewRoom);

	// Compute interaction of the element with the environment
	const MR_ShapeInterface *lContactShape = lElement->GetGivingContactEffectShape();

	if(lContactShape != NULL) {
		// Compute contact with structural elements
		MR_FixedFastArray < int, 20 > lVisitedRooms;
		MR_RoomContactSpec lSpec;

		// Do the contact treatement for that room
		mCurrentLevel->GetRoomContact(lNewRoom, lContactShape, lSpec);

		ComputeShapeContactEffects(lNewRoom, lElement, lSpec, &lVisitedRooms, 1, pTimeToSimulate);
	}

	if(lDeleteElem)
		mCurrentLevel->DeleteElement(pElementHandle);
	return lReturnValue;
}

void MR_GameSession::SimulateFreeElems(MR_SimulationTime pTimeToSimulate)
{
	// Do the simulation
	int lRoomIndex;

	// Interaction objects collection

	// Simulate element by element
	for(lRoomIndex = -1; lRoomIndex < mCurrentLevel->GetRoomCount(); lRoomIndex++) {
		// Simulate FreeElements
		MR_FreeElementHandle lElementHandle = mCurrentLevel->GetFirstFreeElement(lRoomIndex);

		while(lElementHandle != NULL) {
			MR_FreeElementHandle lNext = MR_Level::GetNextFreeElement(lElementHandle);
			SimulateOneFreeElem(pTimeToSimulate, lElementHandle, lRoomIndex);
			lElementHandle = lNext;
		}
	}
	mCurrentLevel->FlushPermElementPosCache();
}

void MR_GameSession::ComputeShapeContactEffects(int pCurrentRoom, MR_FreeElement * pActor, const MR_RoomContactSpec & pLastSpec, MR_FastArrayBase < int >*pVisitedRooms, int pMaxDepth, MR_SimulationTime pDuration)
{
	int lCounter;
	MR_ContactSpec lSpec;

	const MR_ShapeInterface *lActorShape = pActor->GetGivingContactEffectShape();

	BOOL lValidDirection;
	MR_Angle lDirectionAngle;

	pVisitedRooms->Add(pCurrentRoom);

	// Compute contact with features

	// Not correctly done. If a collision is detected, only the floor of the
	// feature is assumed to be touched (directio is ok but wrong elementis selected
	int lNbFeatures = mCurrentLevel->GetFeatureCount(pCurrentRoom);

	for(lCounter = 0; lCounter < lNbFeatures; lCounter++) {
		int lFeatureId = mCurrentLevel->GetFeature(pCurrentRoom, lCounter);

		if(mCurrentLevel->GetFeatureContact(lFeatureId, lActorShape, lSpec)) {
			MR_SurfaceElement *lFloor = mCurrentLevel->GetFeatureTopElement(lFeatureId);

			// Ok Compute the directiion of the collision
			if(lSpec.mZMax <= lActorShape->ZMin()) {
				lValidDirection = FALSE;
			}
			else if(lSpec.mZMin >= lActorShape->ZMax()) {
				lValidDirection = FALSE;
			}
			else {
				lValidDirection = mCurrentLevel->GetFeatureContactOrientation(lFeatureId, lActorShape, lDirectionAngle);
			}

			// const MR_ContactEffectList* lActorEffectList    = pActor->GetEffectList();
			const MR_ContactEffectList *lObstacleEffectList = lFloor->GetEffectList();

			// Apply feature effects to the actor
			pActor->ApplyEffects(lObstacleEffectList, mSimulationTime, pDuration, lValidDirection, lDirectionAngle, lSpec.mZMin, lSpec.mZMax, mCurrentLevel);

			// Apply actor effects to the feature
			// if( lValidDirection )
			//{
			//   lDirectionAngle = MR_NORMALIZE_ANGLE( lDirectionAngle+MR_PI );
			//   lFloor->ApplyEffects( lActorEffectList, mSimulationTime, pDuration, lValidDirection, lDirectionAngle );
			//}
		}
	}

	// Compute interaction with room actors
	MR_FreeElementHandle lObstacleHandle = mCurrentLevel->GetFirstFreeElement(pCurrentRoom);

	while(lObstacleHandle != NULL) {
		MR_FreeElement *lObstacleElem = MR_Level::GetFreeElement(lObstacleHandle);

		if(lObstacleElem != pActor) {

			if(MR_DetectActorContact(lActorShape, lObstacleElem->GetReceivingContactEffectShape(), lSpec)) {
				// Ok Compute the directiion of the collision
				if(lSpec.mZMax <= lActorShape->ZMin()) {
					lValidDirection = FALSE;
				}
				else if(lSpec.mZMin >= lActorShape->ZMax()) {
					lValidDirection = FALSE;
				}
				else {
					lValidDirection = MR_GetActorForceLongitude(lActorShape, lObstacleElem->GetReceivingContactEffectShape(), lDirectionAngle);
				}

				const MR_ContactEffectList *lActorEffectList = pActor->GetEffectList();
				const MR_ContactEffectList *lObstacleEffectList = lObstacleElem->GetEffectList();

				// Apply feature effects to the actor
				pActor->ApplyEffects(lObstacleEffectList, mSimulationTime, pDuration, lValidDirection, lDirectionAngle, lSpec.mZMin, lSpec.mZMax, mCurrentLevel);

				// Apply actor effects to the feature
				lDirectionAngle = MR_NORMALIZE_ANGLE(lDirectionAngle + MR_PI);
				lObstacleElem->ApplyEffects(lActorEffectList, mSimulationTime, pDuration, lValidDirection, lDirectionAngle, lSpec.mZMin, lSpec.mZMax, mCurrentLevel);

			}
		}
		lObstacleHandle = MR_Level::GetNextFreeElement(lObstacleHandle);
	}

	// Compute interaction with touched walls
	// at the same time compute interaction with other rooms

	for(lCounter = 0; lCounter < pLastSpec.mNbWallContact; lCounter++) {
		// Verify that the wall is a wall on all its height

		MR_Int32 lContactTop = lActorShape->ZMax();
		MR_Int32 lContactBottom = lActorShape->ZMin();

		int lNeighbor = mCurrentLevel->GetNeighbor(pCurrentRoom,
			pLastSpec.mWallContact[lCounter]);

		if(lNeighbor != -1) {
			if(!pVisitedRooms->Contains(lNeighbor)) {
				MR_RoomContactSpec lSpec;

				// Recursively call this function

				mCurrentLevel->GetRoomContact(lNeighbor, lActorShape, lSpec);

				/*
				   if( lSpec.mDistanceFromFloor < 0 )
				   {
				   lNeighbor = -1;
				   lContactTop = lActorShape->ZMin() - lSpec.mDistanceFromFloor;
				   }

				   if( lSpec.mDistanceFromCeiling < 0 )
				   {
				   lNeighbor = -1;
				   lContactBottom = lActorShape->ZMax()-lSpec.mDistanceFromCeiling;
				   }

				   if( lNeighbor != -1 )
				 */
				if((lSpec.mDistanceFromFloor < 0) || (lSpec.mDistanceFromCeiling < 0)) {
					lNeighbor = -1;
				}
				else {
					ComputeShapeContactEffects(lNeighbor, pActor, lSpec, pVisitedRooms, pMaxDepth - 1, pDuration);
				}
			}
		}

		if(lNeighbor == -1) {
			// Apply the effect of the wall
			if(mCurrentLevel->GetRoomWallContactOrientation(pCurrentRoom, pLastSpec.mWallContact[lCounter], lActorShape, lDirectionAngle)) {
				MR_SurfaceElement *lWall = mCurrentLevel->GetRoomWallElement(pCurrentRoom, pLastSpec.mWallContact[lCounter]);
				// const MR_ContactEffectList* lActorEffectList    = pActor->GetEffectList();
				const MR_ContactEffectList *lWallEffectList = lWall->GetEffectList();

				// Apply feature effects to the actor
				pActor->ApplyEffects(lWallEffectList, mSimulationTime, pDuration, TRUE, lDirectionAngle, lContactBottom, lContactTop, mCurrentLevel);

				// Apply actor effects to the feature
				// lDirectionAngle = MR_NORMALIZE_ANGLE( lDirectionAngle+MR_PI );
				// lWall->ApplyEffects( lActorEffectList, mSimulationTime, pDuration, TRUE, lDirectionAngle );
			}
		}
	}

}

MR_Level *MR_GameSession::GetCurrentLevel()
{
	return mCurrentLevel;
}

const char *MR_GameSession::GetTitle() const
{
	return mTitle;
} MR_RecordFile *MR_GameSession::GetCurrentMazeFile()
{
	return mCurrentMazeFile;
}