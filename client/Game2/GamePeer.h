
// GamePeer.h
// Scripting peer for system-level control of the game.
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

#pragma once

#include <luabind/luabind.hpp>
#include <luabind/object.hpp>

#include "../../engine/Script/Handlers.h"

class MR_GameApp;
namespace HoverRace {
	namespace Client {
		class ConfigPeer;
		class Rulebook;
		typedef boost::shared_ptr<Rulebook> RulebookPtr;
		class SessionPeer;
		typedef boost::shared_ptr<SessionPeer> SessionPeerPtr;
	}
	namespace Script {
		class Core;
	}
}

namespace HoverRace {
namespace Client {

/**
 * Scripting peer for system-level control of the game.
 * @author Michael Imamura
 */
class GamePeer {
	public:
		GamePeer(Script::Core *scripting, MR_GameApp *gameApp);
		virtual ~GamePeer();

	public:
		static void Register(Script::Core *scripting);

	public:
		void OnInit();
		void OnSessionStart(SessionPeerPtr sessionPeer);

		RulebookPtr RequestedNewSession();

	public:
		ConfigPeer *LGetConfig();

		bool LIsInitialized();

		void LOnInit(const luabind::object &fn);
		void LOnInit(const std::string &name, const luabind::object &fn);

		void LOnSessionStart(const luabind::object &fn);
		void LOnSessionStart(const std::string &name, const luabind::object &fn);

		void LStartPractice(const std::string &track);
		void LStartPractice(const std::string &track, const luabind::object &rules);

	private:
		Script::Core *scripting;
		MR_GameApp *gameApp;
		bool initialized;
		Script::Handlers onInit;
		Script::Handlers onSessionStart;
		RulebookPtr deferredStart;
};

}  // namespace Client
}  // namespace HoverRace
