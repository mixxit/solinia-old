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



#ifndef NL_INTERFACE_PROPERTY_H
#define NL_INTERFACE_PROPERTY_H

#include "nel/misc/types_nl.h"
#include "nel/misc/rgba.h"

#include "../cdb.h"
#include "../cdb_leaf.h"
#include "../cdb_branch.h"
#include "../cdb_synchronised.h"


class CDBNodeProperty;
class IGetProperty;

/**
 * interface property
 * class used to managed all the interface member values
 * As the database contains only sint64, several methods are needed to do the conversion
 * \author Nicolas Brigand
 * \author Nevrax France
 * \date 2002
 */
class CInterfaceProperty
{
public:
	//enum defining a hot spot

	/// Constructor
	CInterfaceProperty()
	{
		_VolatileValue = NULL;
	}

	CCDBNodeLeaf* getNodePtr() const
	{
		return _VolatileValue;
	}

	void setNodePtr(CCDBNodeLeaf *ptr)
	{
		_VolatileValue = ptr;
	}


	bool link (const char *DBProp);
	bool link( CCDBNodeLeaf *dbNode );
	bool link( CCDBNodeBranch *dbNode, const std::string &leafId, CCDBNodeLeaf *defaultLeaf = NULL  );

	/// float operations
	void setDouble (double value) {setSInt64((sint64&) value);}
	double getDouble () const {sint64 i = getSInt64(); return (double &) i;	}
	void readDouble (const char* value, const std::string& id);

	/// sint32 operations
	void setSInt32 (sint32 value) {_VolatileValue->setValue32 (value);}
	sint32 getSInt32 () const {return _VolatileValue->getValue32();}
	void readSInt32(const char* value, const std::string& id);

	/// sint64 operations
	void setSInt64 (sint64 value) {_VolatileValue->setValue64(value);}
	sint64 getSInt64 () const{return _VolatileValue->getValue64();}
	void readSInt64(const char* value, const std::string& id);

	/// CRGBA operations
	void setRGBA (const NLMISC::CRGBA & value);
	NLMISC::CRGBA getRGBA () const;
	void readRGBA (const char* value, const std::string& id);

	/// HotSpot operations

	void readHotSpot (const char* value, const std::string& id);

	/// bool operations
	void setBool (bool value);
	bool getBool () const;
	void readBool (const char* value, const std::string& id);

	// Swap the content of this 2 property (no-op if one is NULL)
	void	swap32(CInterfaceProperty &o);

private:
	/// volatile value of the property (pointer to a leaf of the database)
	CCDBNodeLeaf* _VolatileValue;
};





#endif // NL_INTERFACE_PROPERTY_H

/* End of interface_property.h */
