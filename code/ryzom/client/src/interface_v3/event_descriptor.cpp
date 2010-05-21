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

#include "event_descriptor.h"
#include "nel/misc/events.h"

void CEventDescriptorKey::init(const NLMISC::CEventKey &ev)
{
	_CtrlState = (ev.Button & NLMISC::ctrlKeyButton) != 0;
	_ShiftState = (ev.Button & NLMISC::shiftKeyButton) != 0;
	_AltState = (ev.Button & NLMISC::altKeyButton) != 0;
	if (ev == NLMISC::EventCharId)
	{
		_KeyEvent = keychar;
		_Char = ((const NLMISC::CEventChar &) ev).Char;
	}
	else if (ev == NLMISC::EventKeyUpId)
	{
		_KeyEvent = keyup;
		_Key = ((const NLMISC::CEventKeyUp &) ev).Key;
	}
	else if (ev == NLMISC::EventKeyDownId)
	{
		_KeyEvent = keydown;
		_Key = ((const NLMISC::CEventKeyDown &) ev).Key;
	}
	else
	{
		_KeyEvent = unknown;
	}
}

