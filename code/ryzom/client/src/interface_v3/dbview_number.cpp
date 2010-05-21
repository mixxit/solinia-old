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

#include "dbview_number.h"
#include "interface_manager.h"
#include "game_share/xml_auto_ptr.h"

using namespace std;
using namespace NL3D;
using namespace NLMISC;


NLMISC_REGISTER_OBJECT(CViewBase, CDBViewNumber, std::string, "text_number");

// ***************************************************************************
CDBViewNumber::CDBViewNumber(const TCtorParam &param)
	:CViewText(param)
{
	_Positive = false;
	_Cache= 0;
	setText(ucstring("0"));
	_Divisor = 1;
	_Modulo = 0;
}


// ***************************************************************************
bool CDBViewNumber::parse (xmlNodePtr cur, CInterfaceGroup * parentGroup)
{
	if(!CViewText::parse(cur, parentGroup))
		return false;

	// link to the db
	CXMLAutoPtr ptr;
	ptr = xmlGetProp (cur, (xmlChar*)"value");
	if ( ptr )
	{
		// Yoyo: verify doesn't entered a direct number :). MUST BE A CORRECT DATABASE ENTRY
		const	char	*serverDb= "SERVER:";
		const	char	*localDb= "LOCAL:";
		const	char	*uiDb= "UI:";
		if( strncmp((const char*)ptr, serverDb, strlen(serverDb))==0 ||
			strncmp((const char*)ptr, localDb, strlen(localDb))==0 ||
			strncmp((const char*)ptr, uiDb, strlen(uiDb))==0 )
		{
			// OK? => Link.
			_Number.link ( ptr );
		}
		else
		{
			nlinfo ("bad value in %s", _Id.c_str());
			return false;
		}
	}
	else
	{
		nlinfo ("no value in %s", _Id.c_str());
		return false;
	}

	ptr = xmlGetProp (cur, (xmlChar*)"positive");
	if (ptr) _Positive = convertBool(ptr);
	else _Positive = false;

	ptr = xmlGetProp (cur, (xmlChar*)"divisor");
	if (ptr) fromString((const char*)ptr, _Divisor);

	ptr = xmlGetProp (cur, (xmlChar*)"modulo");
	if (ptr) fromString((const char*)ptr, _Modulo);

	ptr = xmlGetProp (cur, (xmlChar*)"suffix");
	if (ptr) _Suffix = (const char*)ptr;

	ptr = xmlGetProp (cur, (xmlChar*)"prefix");
	if (ptr) _Prefix = (const char*)ptr;

	// init cache.
	_Cache= 0;
	setText(ucstring("0"));

	return true;
}

// ***************************************************************************
void CDBViewNumber::checkCoords()
{
	// change text
	sint64	val = getVal();
	if (_Cache != val)
	{
		_Cache= val;
		if (_Positive) setText(val >= 0 ? ((string)_Prefix)+toString(val)+(string)_Suffix : "?");
		else setText( ((string)_Prefix)+toString(val)+(string)_Suffix );
	}
}

// ***************************************************************************
void CDBViewNumber::draw ()
{
	// parent call
	CViewText::draw();
}

