// NeL - MMORPG Framework <http://dev.ryzom.com/projects/nel/>
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

#include "stdmisc.h"

#include <map>

#include "nel/misc/string_mapper.h"

using namespace std;

namespace NLMISC
{

CStringMapper	CStringMapper::_GlobalMapper;


// ****************************************************************************
CStringMapper::CStringMapper()
{
	_EmptyId = new string;
	*_EmptyId = "";
}

// ****************************************************************************
CStringMapper *CStringMapper::createLocalMapper()
{
	return new CStringMapper;
}

// ****************************************************************************
TStringId CStringMapper::localMap(const std::string &str)
{
	if (str.size() == 0)
		return 0;

	CAutoFastMutex	automutex(&_Mutex);

	string *pStr = new string;
	*pStr = str;

	std::set<string*,CCharComp>::iterator it = _StringTable.find(pStr);

	if (it == _StringTable.end())
	{
		_StringTable.insert(pStr);
	}
	else
	{
		delete pStr;
		pStr = (*it);
	}
	return (TStringId)pStr;
}

// ***************************************************************************
void CStringMapper::localSerialString(NLMISC::IStream &f, TStringId &id)
{
	std::string	str;
	if(f.isReading())
	{
		f.serial(str);
		id= localMap(str);
	}
	else
	{
		str= localUnmap(id);
		f.serial(str);
	}
}

// ****************************************************************************
void CStringMapper::localClear()
{
	CAutoFastMutex	automutex(&_Mutex);

	std::set<string*,CCharComp>::iterator it = _StringTable.begin();
	while (it != _StringTable.end())
	{
		string *ptrTmp = (*it);
		delete ptrTmp;
		it++;
	}
	_StringTable.clear();
	delete _EmptyId;
}

// ****************************************************************************
// CStaticStringMapper
// ****************************************************************************

// ****************************************************************************
TSStringId CStaticStringMapper::add(const std::string &str)
{
	nlassert(!_MemoryCompressed);
	std::map<std::string, TSStringId>::iterator it = _TempStringTable.find(str);
	if (it == _TempStringTable.end())
	{
		_TempStringTable.insert(pair<string,TSStringId>(str,_IdCounter));
		_TempIdTable.insert(pair<TSStringId,string>(_IdCounter,str));
		_IdCounter++;
		return _IdCounter-1;
	}
	else
	{
		return it->second;
	}
}

// ****************************************************************************
bool CStaticStringMapper::isAdded(const std::string &str) const
{
	nlassert(!_MemoryCompressed);
	return _TempStringTable.count(str) != 0;
}

// ****************************************************************************
void CStaticStringMapper::memoryUncompress()
{
	std::map<std::string, TSStringId>	tempStringTable;
	std::map<TSStringId, std::string>	tempIdTable;
	for(uint k = 0; k < _IdToStr.size(); ++k)
	{
		tempStringTable[_IdToStr[k]] = (TSStringId) k;
		tempIdTable[(TSStringId) k] = _IdToStr[k];
	}
	delete [] _AllStrings;
	_AllStrings = NULL;
	contReset(_IdToStr);
	_TempStringTable.swap(tempStringTable);
	_TempIdTable.swap(tempIdTable);
	_MemoryCompressed = false;
}

// ****************************************************************************
void CStaticStringMapper::memoryCompress()
{
	_MemoryCompressed = true;
	std::map<TSStringId, std::string>::iterator it = _TempIdTable.begin();

	uint nTotalSize = 0;
	uint32 nNbStrings = 0;
	while (it != _TempIdTable.end())
	{
		nTotalSize += (uint)it->second.size() + 1;
		nNbStrings++;
		it++;
	}

	_AllStrings = new char[nTotalSize];
	_IdToStr.resize(nNbStrings);
	nNbStrings = 0;
	nTotalSize = 0;
	it = _TempIdTable.begin();
	while (it != _TempIdTable.end())
	{
		strcpy(_AllStrings + nTotalSize, it->second.c_str());
		_IdToStr[nNbStrings] = _AllStrings + nTotalSize;
		nTotalSize += (uint)it->second.size() + 1;
		nNbStrings++;
		it++;
	}
	contReset(_TempStringTable);
	contReset(_TempIdTable);
}

// ****************************************************************************
const char *CStaticStringMapper::get(TSStringId stringId)
{
	if (_MemoryCompressed)
	{
		nlassert(stringId < _IdToStr.size());
		return _IdToStr[stringId];
	}
	else
	{
		std::map<TSStringId, std::string>::iterator it = _TempIdTable.find(stringId);
		if (it != _TempIdTable.end())
			return it->second.c_str();
		else
			return NULL;
	}
}

// ****************************************************************************
void CStaticStringMapper::clear()
{
	contReset(_TempStringTable);
	contReset(_TempIdTable);
	delete [] _AllStrings;
	contReset(_IdToStr);

	_IdCounter = 0;
	_AllStrings = NULL;
	_MemoryCompressed = false;
	add("");
}

// ****************************************************************************
void CStaticStringMapper::serial(IStream &f, TSStringId &strId) throw(EStream)
{
	std::string tmp;
	if (f.isReading())
	{
		f.serial(tmp);
		strId = add(tmp);
	}
	else
	{
		tmp = get(strId);
		f.serial(tmp);
	}
}

// ****************************************************************************
void CStaticStringMapper::serial(IStream &f, std::vector<TSStringId> &strIdVect) throw(EStream)
{
	std::vector<std::string> vsTmp;
	std::string sTmp;
	// Serialize class components.
	if (f.isReading())
	{
		f.serialCont(vsTmp);
		strIdVect.resize(vsTmp.size());
		for(uint i = 0; i < vsTmp.size(); ++i)
			strIdVect[i] = add(vsTmp[i]);
			}
	else
	{
		vsTmp.resize(strIdVect.size());
		for (uint i = 0; i < vsTmp.size(); ++i)
			vsTmp[i] = get(strIdVect[i]);
		f.serialCont(vsTmp);

	}
}



} // namespace NLMISC
