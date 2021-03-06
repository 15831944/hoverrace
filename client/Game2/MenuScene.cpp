
#include "StdAfx.h"

#include "../../engine/Util/Config.h"
#include "../../engine/VideoServices/VideoBuffer.h"
#include "../../engine/VideoServices/Sprite.h"
#include "../../engine/Model/TrackEntry.h"
#include "../../engine/Model/TrackList.h"
#include "../../engine/Model/Track.h"
#include "../../engine/Model/TrackFileCommon.h"
#include "../../engine/Parcel/TrackBundle.h"
#include "../../engine/Parcel/RecordFile.h"
#include "../../engine/MainCharacter/MainCharacterRenderer.h"

#include "Rulebook.h"
#include "Control/Controller.h"

#include <string>

#include "MenuScene.h"

using HoverRace::Util::Config;
using HoverRace::Model::TrackEntry;
using HoverRace::Model::TrackList;
using HoverRace::MainCharacter::MainCharacterRenderer;
using namespace HoverRace::Parcel;
using namespace HoverRace::VideoServices;

namespace HoverRace {
namespace Client {


//Really bad, basic menu
MenuScene::MenuScene(ClientApp *client, VideoServices::VideoBuffer *videoBuf) :
	SUPER(),
	client(client), videoBuf(videoBuf), menuSelection(0),
	selectedPlayers(1), menuHeading(""), curTrackMap(NULL),
	selectedLaps(0), selectedTrack("")
{
	Util::ObjectFromFactoryId baseFontId = { 1, 1000 };
	baseFont = (ObjFac1::SpriteHandle *) Util::DllObjectFactory::CreateObject(baseFontId);
	
	Util::ObjectFromFactoryId mainCharacterRendererId = { 1, 100 };
	MainCharacterRenderer* mainCharacterRenderer = (MainCharacterRenderer *) Util::DllObjectFactory::CreateObject(mainCharacterRendererId);
	
	bumpSound = mainCharacterRenderer->GetBumpSound();
	lapSound = mainCharacterRenderer->GetLineCrossingSound();
	
	Control::InputEventController *controller = client->GetController();
	controller->ClearActionMap();
	controller->AddMenuMap(this);
	
	StartMainMenu();
}

MenuScene::~MenuScene()
{
	delete baseFont;
}

void MenuScene::StartMainMenu()
{
	menuHeading = "HoverRace";
	
	menuOptions.clear();
	menuOptions.push_back("Local Play");
	menuOptions.push_back("Online Play");
	menuOptions.push_back("Controls");
	menuOptions.push_back("Quit");
	
	menuState = MAINMENU;
	drawMode = BASICMENU;
	
	menuSelection = 0;
}

void MenuScene::StartSelectPlayers()
{
	menuHeading = "Num Players";
	
	menuOptions.clear();
	menuOptions.push_back("1");
	menuOptions.push_back("2");
	menuOptions.push_back("3");
	menuOptions.push_back("4");
	
	menuState = PLAYERSSELECT;
	drawMode = BASICMENU;
	
	menuSelection = 0;
}

void MenuScene::StartSelectTrack()
{
	menuHeading = "Track";
	
	Model::TrackList trackList;
	trackList.Reload(Config::GetInstance()->GetTrackBundle());
	
	menuOptions.clear();
	trackDescriptions.clear();
	BOOST_FOREACH(TrackEntry *ent, trackList) {
		menuOptions.push_back(ent->name);
		trackDescriptions.push_back(ent->description);
	}
	
	menuState = TRACKSELECT;
	drawMode = TRACKMENU;
	
	menuSelection = 0;
	
	LoadTrackMap(menuOptions[menuSelection]);
}

void MenuScene::StartSelectLaps()
{
	menuHeading = "Laps";
	
	menuOptions.clear();
	menuOptions.push_back("1");
	menuOptions.push_back("2");
	menuOptions.push_back("3");
	menuOptions.push_back("4");
	menuOptions.push_back("5");
	menuOptions.push_back("6");
	menuOptions.push_back("7");
	menuOptions.push_back("8");
	
	menuState = LAPSSELECT;
	drawMode = BASICMENU;
	
	menuSelection = 0;
}

void MenuScene::LoadTrackMap(std::string trackName)
{
	if (curTrackMap != NULL) delete curTrackMap;
	
	std::string curTrackFile = trackName+Config::TRACK_EXT;
	Model::TrackPtr track = Config::GetInstance()->GetTrackBundle()->OpenTrack(curTrackFile);
	Parcel::RecordFilePtr trackRecord = track->GetRecordFile();
	if (trackRecord->GetNbRecords() >= 4) {
		trackRecord->SelectRecord(3);
		
		ObjStreamPtr archivePtr(trackRecord->StreamIn());
		ObjStream &lArchive = *archivePtr;

		int lX0;
		int lX1;
		int lY0;
		int lY1;
		
		curTrackMap = new VideoServices::Sprite;

		lArchive >> lX0;
		lArchive >> lX1;
		lArchive >> lY0;
		lArchive >> lY1;

		curTrackMap->Serialize(lArchive);
	} else {
		curTrackMap = NULL;
	}
	
}

void MenuScene::IncrementSelection(int amount) {
	if (amount == 0) return;
	
	menuSelection += amount;
	menuSelection %= menuOptions.size();
	if (menuSelection < 0) menuSelection += menuOptions.size();
	
	
	if (menuState == TRACKSELECT) {
		LoadTrackMap(menuOptions[menuSelection]);
	}
	
	SoundServer::Play(bumpSound);
}

void MenuScene::Select() {
	SoundServer::Play(lapSound);
	
	if (menuState == MAINMENU) {
		if (menuSelection == 0) {
			StartSelectPlayers();
		}
	} else if (menuState == PLAYERSSELECT) {
		selectedPlayers = menuSelection+1;
		StartSelectTrack();
	} else if (menuState == TRACKSELECT) {
		selectedTrack = menuOptions[menuSelection]+Config::TRACK_EXT;
		StartSelectLaps();
	} else if (menuState == LAPSSELECT) {
		selectedLaps = menuSelection+1;
		
		RulebookPtr rules = boost::make_shared<Rulebook>(selectedTrack, selectedLaps, 0x7f);
		
		if (selectedPlayers == 1) {
			client->NewLocalSession(rules);
		} else {
			client->NewSplitSession(selectedPlayers, rules);
		}
	}
}

void MenuScene::Advance(Util::OS::timestamp_t tick)
{
}

void MenuScene::Render()
{
	
	viewPort.Setup(videoBuf, 0, 0, videoBuf->GetXRes(), videoBuf->GetYRes());
	viewPort.Clear(0);
	
	const Sprite *font = baseFont->GetSprite();
	
	//blit heading
	int titleScaling = 1;
	int XMargin = 10;
	int YMargin = 10;
	font->StrBlt(XMargin, YMargin, Ascii2Simple(menuHeading), &viewPort, Sprite::eLeft, Sprite::eTop, titleScaling);
	
	int entryY = YMargin+font->GetItemHeight()/titleScaling+20;
	
	if (drawMode == BASICMENU) {
		int scaling = 2;
		int lineSpace = font->GetItemHeight() / scaling + 10;
	
		for (int i=0; i<menuOptions.size(); i++) {
			if (i == menuSelection)  {
				font->StrBlt(XMargin, entryY+lineSpace*i, Ascii2Simple("-"), &viewPort, Sprite::eLeft, Sprite::eTop, scaling);
			}
			font->StrBlt(XMargin+font->GetItemWidth()*2/scaling, entryY+lineSpace*i,
			             Ascii2Simple(menuOptions[i].c_str()), &viewPort, Sprite::eLeft, Sprite::eTop, scaling);
		}
	} else if (drawMode == TRACKMENU) {
		
		//blit track name
		int trackNameScaling = 2;
		std::stringstream trackName;
		trackName << (menuSelection+1) << ". " << menuOptions[menuSelection];
		font->StrBlt(XMargin, entryY, Ascii2Simple(trackName.str().c_str()), &viewPort, Sprite::eLeft, Sprite::eTop, trackNameScaling);
		
		//blit track map
		int mapScaling = 1;
		int contentY = entryY+font->GetItemHeight()/trackNameScaling + 10;
		int descriptionX = XMargin;
		if (curTrackMap != NULL) {
			curTrackMap->Blt(XMargin, contentY, &viewPort, Sprite::eLeft, Sprite::eTop, 0, mapScaling);
			descriptionX += curTrackMap->GetItemWidth()/mapScaling + 20;
		}
		
		//blit track description
		int descriptionScaling = 3;
		
		std::string description = trackDescriptions[menuSelection];
		int lineWidth = (viewPort.GetXRes()-descriptionX)/(font->GetItemWidth()/descriptionScaling);
		int lineHeight = font->GetItemHeight()/descriptionScaling + 5;
		
		for (int i=0, curLineStart=0; curLineStart < description.length(); i++) {
			size_t nextNewLinePos = description.find('\n', curLineStart);
			int curLineLength;
			if (nextNewLinePos != std::string::npos && nextNewLinePos < curLineStart+lineWidth) {
				curLineLength = nextNewLinePos-curLineStart;
			} else {
				curLineLength = lineWidth;
			}
			std::string line = description.substr(curLineStart, curLineLength);
			font->StrBlt(descriptionX, contentY+lineHeight*i,
			             Ascii2Simple(line.c_str()), &viewPort,
			             Sprite::eLeft, Sprite::eTop, descriptionScaling);
			
			curLineStart += curLineLength+1;
		}
		
		
		
	}
	
}


}  // namespace HoverScript
}  // namespace Client