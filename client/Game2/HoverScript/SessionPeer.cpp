
// SessionPeer.cpp
// Scripting peer for a game session.
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

#include "../../engine/Script/Core.h"
#include "../ClientSession.h"

#include "SessionPeer.h"

namespace HoverRace {
namespace Client {
namespace HoverScript {

SessionPeer::SessionPeer(Script::Core *scripting, ClientSession *session) :
	SUPER(scripting, "Session"), session(session)
{
}

SessionPeer::~SessionPeer()
{
}

/**
 * Register this peer in an environment.
 */
void SessionPeer::Register(Script::Core *scripting)
{
	using namespace luabind;
	lua_State *L = scripting->GetState();

	module(L) [
		class_<SessionPeer,SUPER>("Session")
			.def("get_num_players", &SessionPeer::LGetNumPlayers)
	];
}

/**
 * Signal that the session has ended.
 * This will detach this peer from the session.
 */
void SessionPeer::OnSessionEnd()
{
	session = NULL;
}

/**
 * Verify that the session is still active.
 * If the session is no longer active, then a Lua error will be triggered and
 * this function will not return.
 */
void SessionPeer::VerifySession() const
{
	if (session == NULL) {
		luaL_error(scripting->GetState(), "Session has ended.");
	}
}

int SessionPeer::LGetNumPlayers() const
{
	VerifySession();
	return session->GetNbPlayers();
}

}  // namespace HoverScript
}  // namespace Client
}  // namespace HoverRace
