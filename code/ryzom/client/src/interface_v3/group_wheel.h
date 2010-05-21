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



#ifndef RZ_GROUP_CONTAINER_H
#define RZ_GROUP_CONTAINER_H

#include "interface_group.h"



// Special group to handle the mouse wheel message
class CInterfaceGroupWheel : public CInterfaceGroup
{
public:
	/// Constructor
	CInterfaceGroupWheel(const TCtorParam &param);
	/// Coming from CInterfaceElement
	virtual bool parse(xmlNodePtr cur, CInterfaceGroup * parentGroup);
	virtual bool handleEvent (const CEventDescriptor &event);
private:
	IActionHandler *_AHWheelUp;
	CStringShared	_AHWheelUpParams;
	IActionHandler *_AHWheelDown;
	CStringShared	_AHWheelDownParams;
};

#endif
