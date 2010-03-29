
// Method.cpp
// Help text for a method in the scripting API.
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

#include "Method.h"

namespace HoverRace {
namespace Script {
namespace Help {

/**
 * Constructor.
 * @param name The name of the method.
 */
Method::Method(const std::string &name) :
	name(name)
{
}

Method::~Method()
{
}

const std::string &Method::GetName() const
{
	return name;
}

}  // namespace Help
}  // namespace Script
}  // namespace HoverRace
