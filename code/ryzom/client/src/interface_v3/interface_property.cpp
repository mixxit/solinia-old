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

#include "interface_property.h"
#include "interface_manager.h"
#include "nel/misc/rgba.h"

using namespace NLMISC;
using namespace std;

bool CInterfaceProperty::link( CCDBNodeLeaf *dbNode )
{
	_VolatileValue = dbNode;
	return (dbNode != NULL);
}

bool CInterfaceProperty::link( CCDBNodeBranch *dbNode, const string &leafId, CCDBNodeLeaf *defaultLeaf )
{
	// no branch => default leaf
	if( !dbNode )
	{
		_VolatileValue = defaultLeaf;
		return false;
	}

	// get the leaf
	_VolatileValue = dbNode->getLeaf( leafId.c_str(), false );
	if( _VolatileValue )
		return true;

	// default
	_VolatileValue = defaultLeaf;
	return false;
}

bool CInterfaceProperty::link (const char *DBProp)
{
	_VolatileValue = CInterfaceManager::getInstance()->getDbProp(DBProp, false);
	if (_VolatileValue == NULL)
	{

		nlinfo("prop not created : %s", DBProp);
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(DBProp);
		return false;
	}
	return true;
}


// *****************
// sint64 operations
// *****************


void CInterfaceProperty::readSInt64(const char * ptr,const string& id)
{
	string str (ptr);
	//the value is volatile, and a database entry is created
	if ( isdigit(*ptr) || *ptr=='-')
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		sint64 i;
		fromString(ptr, i);
		_VolatileValue->setValue64( i );
	}
	//the value is volatile and points to a db entry created elsewhere
	else
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(str);
	}
}




// ****************
// float operations
// ****************


// ----------------------------------------------------------------------------
void CInterfaceProperty::readDouble(const char * ptr,const string& id)
{
	string str (ptr);
	if ( isdigit(*ptr) || *ptr=='-')
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		double buf;
		fromString(ptr, buf);
		sint64 i = *(sint64*)&buf;
		_VolatileValue->setValue64( i );
	}
	else
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(str);
	}
}

// *****************
// sint32 operations
// *****************


// ----------------------------------------------------------------------------
void CInterfaceProperty::readSInt32 (const char *ptr, const string& id)
{
	string str (ptr);
	//the value is volatile, and a database entry is created
	if ( isdigit(*ptr) || *ptr=='-')
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		sint32 i;
		fromString(ptr, i);
		_VolatileValue->setValue32( i );
	}
	//the value is volatile and points to a db entry created elsewhere
	else
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(str);
	}
}



// *****************
// rgba operations
// *****************
CRGBA CInterfaceProperty::getRGBA() const
{
	CRGBA rgba;
	sint64 buf = getSInt64();
	rgba.R = (sint8) (buf &255);
	rgba.G = (sint8) ((buf>>8)&255 );
	rgba.B = (sint8) ((buf>>16)&255);
	rgba.A = (sint8) ((buf>>24)&255);
	return rgba;
}


void CInterfaceProperty::setRGBA (const CRGBA& value)
{
	setSInt64(  (value.R )+   (((sint32)value.G)<<8) + (((sint32)value.B)<<16) + (((sint32)value.A)<<24));
}


void CInterfaceProperty::readRGBA (const char *value,const string& id)
{
	string str (value);
	if (isdigit(*value))
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		int r=0, g=0, b=0, a=255;
		sscanf (value, "%d %d %d %d", &r, &g, &b, &a);
		clamp (r, 0, 255);
		clamp (g, 0, 255);
		clamp (b, 0, 255);
		clamp (a, 0, 255);
		sint64 val = r+(g<<8)+(b<<16)+(a<<24);
		setSInt64(val);
		CRGBA rgba = getRGBA();
	}
	else
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(str);
	}
}


void CInterfaceProperty::readHotSpot (const char *ptr,const string& id)
{
	string str(ptr);
	if ( !strcmp(ptr,"TL") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64((sint64)Hotspot_TL );
	}
	else if ( !strcmp(ptr,"TM") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_TM );
	}
	else if ( !strcmp(ptr,"TR") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_TR );
	}
	else if ( !strcmp(ptr,"ML") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_ML );
	}
	else if ( !strcmp(ptr,"MM") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_MM );
	}
	else if ( !strcmp(ptr,"MR") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_MR );
	}
	else if ( !strcmp(ptr,"BL") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_BL );
	}
	else if ( !strcmp(ptr,"BM") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_BM );
	}
	else if ( !strcmp(ptr,"BR") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue64( (sint64)Hotspot_BR );
	}

	else
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(str);
}


void CInterfaceProperty::setBool(bool value)
{
	_VolatileValue->setValue8 (value);
}

bool CInterfaceProperty::getBool() const
{
	return _VolatileValue->getValue8() != 0 ? true : false;
}

void CInterfaceProperty::readBool (const char* value,const string& id)
{
	string str (value);
	if ( !strcmp(value,"true") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue8( (sint8)true );
	}
	else if ( !strcmp(value,"false") )
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		_VolatileValue->setValue8( (sint8)false );
	}
	else if ( isdigit(*value) || *value=='-')
	{
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(id);
		sint8 value8;
		fromString(value, value8);
		_VolatileValue->setValue8( value8 );
	}
	else
		_VolatileValue = CInterfaceManager::getInstance()->getDbProp(str);
}


// ***************************************************************************
void	CInterfaceProperty::swap32(CInterfaceProperty &o)
{
	CCDBNodeLeaf	*a= getNodePtr();
	CCDBNodeLeaf	*b= o.getNodePtr();
	if(!a || !b)
		return;
	sint32	val= a->getValue32();
	a->setValue32(b->getValue32());
	b->setValue32(val);
}

