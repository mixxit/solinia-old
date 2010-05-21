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



#ifndef NL_DBVIEW_NUMBER_H
#define NL_DBVIEW_NUMBER_H

#include "nel/misc/types_nl.h"

#include "view_text.h"


// ***************************************************************************
/**
 * Display a text from a database number
 * \author Lionel Berenguier
 * \author Nevrax France
 * \date 2002
 */
class CDBViewNumber : public CViewText
{
public:

	/// Constructor
	CDBViewNumber(const TCtorParam &param);

	virtual bool parse (xmlNodePtr cur, CInterfaceGroup * parentGroup);
	virtual void checkCoords();
	virtual void draw ();

	void link (const std::string &dbprop)
	{
		_Number.link (dbprop.c_str());
	}

protected:

	sint64 getVal() { if (_Modulo == 0) return (_Number.getSInt64() / _Divisor);
								else	return (_Number.getSInt64() / _Divisor)%_Modulo; }

protected:

	CInterfaceProperty		_Number;
	sint64					_Cache;
	bool                    _Positive; // only positive values are displayed
	sint64					_Divisor, _Modulo;
	// string to append to the value (eg: meters)
	CStringShared			_Suffix;
	CStringShared			_Prefix;
};


#endif // NL_DBVIEW_NUMBER_H

/* End of dbview_number.h */
