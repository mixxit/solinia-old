// Ryzom - MMORPG Framework <http://dev.ryzom.com/projects/ryzom/>
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


#include "stdpch.h"

//
// Includes
//

#include <string>
#include <queue>

#include <nel/misc/types_nl.h>
#include <nel/misc/debug.h>

#include <nel/net/udp_sock.h>

#include "action.h"

//
// Using
//

using namespace std;

using namespace NLMISC;
using namespace NLNET;

namespace CLFECOMMON {

//
// Classes
//

const CAction::TValue	NullValue = (CAction::TValue)0;

CAction::CAction ()
{
	Timeout = 100;
	Code = 0;
	PropertyCode = 0;
	Slot = INVALID_SLOT;
	_Priority = 1;
}

}