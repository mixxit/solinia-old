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



#ifndef NL_DBVIEW_DIGIT_H
#define NL_DBVIEW_DIGIT_H

#include "nel/misc/types_nl.h"

#include "view_base.h"


// ***************************************************************************
/**
 * A number displayed with special bitmaps
 * \author Lionel Berenguier
 * \author Nevrax France
 * \date 2002
 */
class CDBViewDigit : public CViewBase
{
public:

	/// Constructor
	CDBViewDigit(const TCtorParam &param);

	virtual bool parse (xmlNodePtr cur, CInterfaceGroup * parentGroup);
	virtual void draw ();
	virtual void updateCoords();

protected:
	CInterfaceProperty		_Number;
	sint32					_Cache;
	sint32					_NumDigit;
	NLMISC::CRGBA			_Color;
	// space between each digit
	sint32					_WSpace;
	// The texture digit for the current number
	sint32					_DigitId[10];
	uint					_DivBase;


};


#endif // NL_DBVIEW_DIGIT_H

/* End of dbview_digit.h */
