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

#include "interface_group.h"
#include "interface_property.h"
#include "interface_manager.h"
#include "group_container.h"
#include "../misc.h"
#include "interface_link.h"
#include "game_share/xml_auto_ptr.h"
#include "lua_ihm.h"
#include "nel/misc/mem_stream.h"
//

using namespace std;
using namespace NLMISC;

CStringMapper *_UIStringMapper = NULL;

// ------------------------------------------------------------------------------------------------
CReflectableRefPtrTarget::~CReflectableRefPtrTarget()
{
	CInterfaceManager	*pIM= CInterfaceManager::getInstance();
	CLuaState *lua= pIM->getLuaState();
	if(!lua) return;
	CLuaStackChecker lsc(lua);
	// remove from the lua registry if i'm in
	lua->pushLightUserData((void *) this);
	lua->getTable(LUA_REGISTRYINDEX);
	if (!lua->isNil(-1))
	{
		lua->pop();
		lua->pushLightUserData((void *) this);
		lua->pushNil();
		lua->setTable(LUA_REGISTRYINDEX);
	}
	else
	{
		lua->pop();
	}
}

// ------------------------------------------------------------------------------------------------
CInterfaceElement::~CInterfaceElement()
{
	if (_Links) // remove any link that point to that element
	{
		for(TLinkVect::iterator it = _Links->begin(); it != _Links->end(); ++it)
		{
			(*it)->removeTarget(this);
		}
		delete _Links;
	}
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::parseError(CInterfaceGroup * parentGroup, const char *reason)
{
	string tmp = string("cannot parse view:")+getId()+", parent:"+parentGroup->getId();
	nlinfo(tmp.c_str());
	if (reason)
		nlinfo("reason : %s", reason);
}


void CInterfaceElement::setIdRecurse(const std::string &newID)
{
	std::string baseId = _Parent ? _Parent->getId() : "ui";
	setId(baseId + ":" + newID);
}

// ------------------------------------------------------------------------------------------------
std::string CInterfaceElement::getShortId() const
{
	std::string::size_type last = _Id.find_last_of(':');
	if (last != std::string::npos)
	{
		return _Id.substr(last + 1);
	}
	return _Id;
}

// ------------------------------------------------------------------------------------------------
bool CInterfaceElement::parse(xmlNodePtr cur, CInterfaceGroup * parentGroup)
{
	// parse the basic properties
	CXMLAutoPtr ptr((const char*) xmlGetProp( cur, (xmlChar*)"id" ));
	if (ptr)
	{
		if (parentGroup)
		{
			_Id = ( (CInterfaceElement*)parentGroup )->_Id;
		}
		else
		{
			_Id ="ui";
		}
		_Id += ":"+ string((const char*)ptr);
	}
	else
	{
		nlinfo(" error no id in an element");
		return false;
	}

	ptr = (char*) xmlGetProp( cur, (xmlChar*)"active" );
	_Active = true;
	if (ptr)
	{
		_Active = convertBool(ptr);
	}

	_Parent = parentGroup;

	// parse location. If these properties are not specified, set them to 0
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"x" );
	_X = 0;
	if (ptr) fromString((const char*)ptr, _X);

	ptr = (char*) xmlGetProp( cur, (xmlChar*)"y" );
	_Y = 0;
	if (ptr) fromString((const char*)ptr, _Y);

	ptr = (char*) xmlGetProp( cur, (xmlChar*)"w" );
	_W = 0;
	if (parentGroup != NULL)
		_W = parentGroup->getW();
	if (ptr) fromString((const char*)ptr, _W);

	ptr = (char*) xmlGetProp( cur, (xmlChar*)"h" );
	_H = 0;
	if (parentGroup != NULL)
		_H = parentGroup->getH();
	if (ptr) fromString((const char*)ptr, _H);

	// snapping
//	ptr = (char*) xmlGetProp( cur, (xmlChar*)"snap" );
//	_Snap = 1;
//	if (ptr)
//		fromString((const char*)ptr, _Snap);
//	if (_Snap <= 0)
//	{
//		parseError(parentGroup, "snap must be > 0" );
//		return false;
//	}

	ptr = (char*) xmlGetProp( cur, (xmlChar*) "posref" );
	_ParentPosRef = Hotspot_BL;
	_PosRef = Hotspot_BL;
	if (ptr)
	{
		convertHotSpotCouple(ptr.getDatas(), _ParentPosRef, _PosRef);
	}

	ptr = (char*) xmlGetProp( cur, (xmlChar*)"posparent" );
	if (ptr)
	{
		if (strcmp(ptr,"parent")) // is ptr != "parent"
		{
			string idparent;
			if (parentGroup)
				idparent = parentGroup->getId() +":";
			else
				idparent = "ui:";
			CInterfaceManager::getInstance()->addParentPositionAssociation (this, idparent +  string((const char*)ptr));
		}
	}

	ptr = (char*) xmlGetProp( cur, (xmlChar*)"sizeparent" );
	if (ptr)
	{
		string idparent = ptr;
		idparent = NLMISC::strlwr(idparent);
		if (idparent != "parent")
		{
			if (parentGroup)
				idparent = parentGroup->getId() +":" +  string((const char*)ptr);
			else
				idparent = "ui:" +  string((const char*)ptr);
		}
		else
		{
			if (parentGroup)
				idparent = parentGroup->getId();
		}
		CInterfaceManager::getInstance()->addParentSizeAssociation (this, idparent);
	}

	ptr = (char*) xmlGetProp (cur, (xmlChar*)"sizeref");
	_SizeRef = 0;
	_SizeDivW = 10;
	_SizeDivH = 10;
	if (ptr)
	{
		parseSizeRef(ptr.getDatas());
		sint32 nWhat = 0;
		const char *seekPtr = ptr.getDatas();
		while (*seekPtr != 0)
		{
			if ((*seekPtr=='w')||(*seekPtr=='W'))
			{
				_SizeRef |= 1;
				nWhat = 1;
			}

			if ((*seekPtr=='h')||(*seekPtr=='H'))
			{
				_SizeRef |= 2;
				nWhat = 2;
			}

			if ((*seekPtr>='1')&&(*seekPtr<='9'))
			{
				if (nWhat != 0)
				{
					if (nWhat == 1)
						_SizeDivW = *seekPtr-'0';
					if (nWhat == 2)
						_SizeDivH = *seekPtr-'0';
				}
			}

			++seekPtr;
		}
	}

//	snapSize();

	ptr= (char*) xmlGetProp (cur, (xmlChar*)"global_color");
	if(ptr)
	{
		_ModulateGlobalColor= convertBool(ptr);
	}

	ptr= (char*) xmlGetProp (cur, (xmlChar*)"render_layer");
	if(ptr)		fromString((const char*)ptr, _RenderLayer);

	ptr= (char*) xmlGetProp (cur, (xmlChar*)"avoid_resize_parent");
	if(ptr)		_AvoidResizeParent= convertBool(ptr);

	return true;
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::setSizeRef(const std::string &sizeref)
{
	parseSizeRef(sizeref.c_str());
}

// ------------------------------------------------------------------------------------------------
std::string CInterfaceElement::getSizeRefAsString() const
{
	return "IMPLEMENT ME!";
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::parseSizeRef(const char *sizeRefStr)
{
	parseSizeRef(sizeRefStr, _SizeRef, _SizeDivW, _SizeDivH);
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::parseSizeRef(const char *sizeRefStr, sint32 &sizeRef, sint32 &sizeDivW, sint32 &sizeDivH)
{
	nlassert(sizeRefStr);

	sizeRef = 0;
	sizeDivW = 10;
	sizeDivH = 10;
	sint32 nWhat = 0;
	const char *seekPtr = sizeRefStr;
	while (*seekPtr != 0)
	{
		if ((*seekPtr=='w')||(*seekPtr=='W'))
		{
			sizeRef |= 1;
			nWhat = 1;
		}

		if ((*seekPtr=='h')||(*seekPtr=='H'))
		{
			sizeRef |= 2;
			nWhat = 2;
		}

		if ((*seekPtr>='1')&&(*seekPtr<='9'))
		{
			if (nWhat != 0)
			{
				if (nWhat == 1)
					sizeDivW = *seekPtr-'0';
				if (nWhat == 2)
					sizeDivH = *seekPtr-'0';
			}
		}

		++seekPtr;
	}
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::updateCoords()
{
	_XReal = _X;
	_YReal = _Y;
	_WReal = getW();
	_HReal = getH();

	CInterfaceElement *el = NULL;

	// Modif Pos

	if (_ParentPos != NULL)
		el = _ParentPos;
	else
		el = _Parent;

	if (el == NULL)
		return;

	_XReal += el->_XReal;
	_YReal += el->_YReal;

	THotSpot hsParent = _ParentPosRef;
	if (hsParent &  Hotspot_Mx)
		_YReal += el->_HReal/2;
	if (hsParent & Hotspot_Tx)
		_YReal += el->_HReal;
	if (hsParent & Hotspot_xM)
		_XReal += el->_WReal/2;
	if (hsParent & Hotspot_xR)
		_XReal += el->_WReal;

	// Modif Size

	if (_ParentSize != NULL)
	{
		el = _ParentSize;
	}
	else
	{
		if (_ParentPos != NULL)
			el = _ParentPos;
		else
			el = _Parent;
	}

	if (el == NULL)
		return;

	if (_SizeRef&1)
		_WReal += _SizeDivW * el->_WReal / 10;

	if (_SizeRef&2)
		_HReal += _SizeDivH * el->_HReal / 10;

	THotSpot hs = _PosRef;
	if (hs & Hotspot_Mx)
		_YReal -= _HReal/2;
	if (hs & Hotspot_Tx)
		_YReal -= _HReal;
	if (hs & Hotspot_xM)
		_XReal -= _WReal/2;
	if (hs & Hotspot_xR)
		_XReal -= _WReal;
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::getCorner(sint32 &px, sint32 &py, THotSpot hs)
{
	px = _XReal;
	py = _YReal;
	if (hs & 1) px += _WReal;
	if (hs & 2) px += _WReal >> 1;
	if (hs & 8) py += _HReal;
	if (hs & 16) py += _HReal >> 1;
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::move (sint32 dx, sint32 dy)
{
	_X += dx;
	_Y += dy;
	invalidateCoords();
}



// ------------------------------------------------------------------------------------------------
/*void CInterfaceElement::resizeBR (sint32 sizeW, sint32 sizeH)
{
	uint32 i = i / 0;
	THotSpot hs = _PosRef;

	sint32 dw = sizeW - _W;
	sint32 dh = sizeH - _H;

	sint32 snap = _Snap;
	nlassert(snap > 0);

	if (hs&8) // is top ?
	{
		sint32 newH = dh + _H;
		if (snap > 1)
			newH -= newH % snap;
		_H = newH;
	}
	if (hs&32) // is bottom ?
	{
		sint32 newH = dh + _H;
		if (snap > 1)
			newH -= newH % snap;
		_Y = _H - newH + _Y;
		_H = newH;
	}

	if (hs&1) // is right ?
	{
		sint32 newW = dw + _W;
		if (snap > 1)
			newW -= newW % snap;
		_X = newW - _W + _X;
		_W = newW;
	}
	if (hs&4) // is left ?
	{
		sint32 newW = dw + _W;
		if (snap > 1)
			newW -= newW % snap;
		_W = newW;
	}

	// DO NOT TREAT THE MIDDLE HOTSPOT CASE

	invalidateCoords();
}*/


// ------------------------------------------------------------------------------------------------
/*void CInterfaceElement::snapSize()
{
	sint32 snap = _Snap;
	nlassert(snap > 0);
	if (snap > 1)
	{
		_W = _W - (_W % snap);
		_H = _H - (_H % snap);
	}
}*/


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::setW (sint32 w)
{
	_W = w;
//	sint32 snap = _Snap;
//	nlassert(snap > 0);
//	if (snap > 1)
//	{
//		_W = _W - (_W % snap);
//	}
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::setH (sint32 h)
{
	_H = h;
//	sint32 snap = _Snap;
//	nlassert(snap > 0);
//	if (snap > 1)
//	{
//		_H = _H - (_H % snap);
//	}
}


// ------------------------------------------------------------------------------------------------
CInterfaceGroup* CInterfaceElement::getRootWindow ()
{
	if (_Parent == NULL)
		return NULL;
	if (_Parent->getParent() == NULL)
		return dynamic_cast<CInterfaceGroup*>(this);
	return _Parent->getRootWindow();
}

// ------------------------------------------------------------------------------------------------
uint	CInterfaceElement::getParentDepth() const
{
	uint	depth= 0;
	CInterfaceGroup *parent= _Parent;
	while(parent!=NULL)
	{
		parent= parent->getParent();
		depth++;
	}
	return depth;
}

// ------------------------------------------------------------------------------------------------
bool CInterfaceElement::isActiveThroughParents() const
{
	if(!getActive())
		return false;
	if(_Parent == NULL)
		return false;
	// is it the root window?
	if (_Parent->getParent() == NULL)
		// yes and getActive() is true => the element is visible!
		return true;
	else
		return _Parent->isActiveThroughParents();
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::relativeSInt64Read (CInterfaceProperty &rIP, const string &prop, const char *val,
													   const string &defVal)
{
	if (val == NULL)
	{
		rIP.readSInt64 (defVal.c_str(), _Id+":"+prop);
	}
	else
	{
		if ( isdigit(*val) || *val=='-')
		{
			rIP.readSInt64 (val, _Id+":"+prop);
			return;
		}

		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		sint32 decal = 0;
		if (val[0] == ':')
			decal = 1;
		if (pIM->getDbProp(val+decal, false) != NULL)
		{
			rIP.readSInt64 (val+decal, _Id+":"+prop);
			return;
		}
		else
		{
			string sTmp;
			CInterfaceElement *pIEL = this;

			while (pIEL != NULL)
			{
				sTmp = pIEL->getId()+":"+string(val+decal);
				if (CInterfaceManager::getInstance()->getDbProp(sTmp, false) != NULL)
				{
					rIP.readSInt64 (sTmp.c_str(), _Id+":"+prop);
					return;
				}
				pIEL = pIEL->getParent();
			}

			rIP.readSInt64 (val+decal, _Id+":"+prop);
		}
	}
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::relativeSInt32Read (CInterfaceProperty &rIP, const string &prop, const char *val,
													   const string &defVal)
{
	if (val == NULL)
	{
		rIP.readSInt32 (defVal.c_str(), _Id+":"+prop);
	}
	else
	{
		if ( isdigit(*val) || *val=='-')
		{
			rIP.readSInt32 (val, _Id+":"+prop);
			return;
		}

		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		sint32 decal = 0;
		if (val[0] == ':')
			decal = 1;
		if (pIM->getDbProp(val+decal, false) != NULL)
		{
			rIP.readSInt32 (val+decal, _Id+":"+prop);
			return;
		}
		else
		{
			string sTmp;
			CInterfaceElement *pIEL = this;

			while (pIEL != NULL)
			{
				sTmp = pIEL->getId()+":"+string(val+decal);
				if (CInterfaceManager::getInstance()->getDbProp(sTmp, false) != NULL)
				{
					rIP.readSInt32 (sTmp.c_str(), _Id+":"+prop);
					return;
				}
				pIEL = pIEL->getParent();
			}

			rIP.readSInt32 (val+decal, _Id+":"+prop);
		}
	}
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::relativeBoolRead (CInterfaceProperty &rIP, const string &prop, const char *val,
													   const string &defVal)
{
	if (val == NULL)
	{
		rIP.readBool (defVal.c_str(), _Id+":"+prop);
	}
	else
	{
		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		sint32 decal = 0;
		if (val[0] == ':')
			decal = 1;
		if (pIM->getDbProp(val+decal, false) != NULL)
		{
			rIP.readBool (val+decal, _Id+":"+prop);
			return;
		}
		else
		{
			string sTmp;
			CInterfaceElement *pIEL = this;

			while (pIEL != NULL)
			{
				sTmp = pIEL->getId()+":"+string(val+decal);
				if (CInterfaceManager::getInstance()->getDbProp(sTmp, false) != NULL)
				{
					rIP.readBool (sTmp.c_str(), _Id+":"+prop);
					return;
				}
				pIEL = pIEL->getParent();
			}

			rIP.readBool (val+decal, _Id+":"+prop);
		}
	}
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::relativeRGBARead(CInterfaceProperty &rIP,const std::string &prop,const char *val,const std::string &defVal)
{
	if (val == NULL)
	{
		rIP.readRGBA (defVal.c_str(), _Id+":"+prop);
	}
	else
	{
		if ( isdigit(*val) || *val=='-')
		{
			rIP.readRGBA (val, _Id+":"+prop);
			return;
		}

		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		sint32 decal = 0;
		if (val[0] == ':')
			decal = 1;
		if (pIM->getDbProp(val+decal, false) != NULL)
		{
			rIP.readRGBA (val+decal, _Id+":"+prop);
			return;
		}
		else
		{
			string sTmp;
			CInterfaceElement *pIEL = this;

			while (pIEL != NULL)
			{
				sTmp = pIEL->getId()+":"+string(val+decal);
				if (CInterfaceManager::getInstance()->getDbProp(sTmp, false) != NULL)
				{
					rIP.readRGBA (sTmp.c_str(), _Id+":"+prop);
					return;
				}
				pIEL = pIEL->getParent();
			}

			rIP.readRGBA (val+decal, _Id+":"+prop);
		}
	}
}


// ------------------------------------------------------------------------------------------------
THotSpot CInterfaceElement::convertHotSpot (const char *ptr)
{
	if ( !strnicmp(ptr,"TL",2) )
	{
		return Hotspot_TL;
	}
	else if ( !strnicmp(ptr,"TM",2) )
	{
		return Hotspot_TM;
	}
	else if ( !strnicmp(ptr,"TR",2) )
	{
		return Hotspot_TR;
	}
	else if ( !strnicmp(ptr,"ML",2) )
	{
		return Hotspot_ML;
	}
	else if ( !strnicmp(ptr,"MM",2) )
	{
		return Hotspot_MM;
	}
	else if ( !strnicmp(ptr,"MR",2) )
	{
		return Hotspot_MR;
	}
	else if ( !strnicmp(ptr,"BL",2) )
	{
		return Hotspot_BL;
	}
	else if ( !strnicmp(ptr,"BM",2) )
	{
		return Hotspot_BM;
	}
	else if ( !strnicmp(ptr,"BR",2) )
	{
		return Hotspot_BR;
	}
	else
		return Hotspot_BL;
}

// ------------------------------------------------------------------------------------------------
void		CInterfaceElement::convertHotSpotCouple (const char *ptr, THotSpot &parentPosRef, THotSpot &posRef)
{
	nlassert(ptr);

	// *** first hotspot
	// skip any space or tab
	while(*ptr=='\t' || *ptr==' ')
		ptr++;
	// convert first
	parentPosRef = convertHotSpot (ptr);

	// *** second hotspot
	// must be at least 2 letter and a space
	nlassert(strlen(ptr)>=3);
	ptr+=3;
	// skip any space or tab
	while(*ptr=='\t' || *ptr==' ')
		ptr++;
	// convert second
	posRef = convertHotSpot (ptr);
}

// ------------------------------------------------------------------------------------------------
NLMISC::CRGBA CInterfaceElement::convertColor (const char *ptr)
{
	return stringToRGBA(ptr);
}

// ------------------------------------------------------------------------------------------------
bool			CInterfaceElement::convertBool (const char *ptr)
{
	string	str= ptr;
	NLMISC::strlwr(str);
	return str=="true"?true:false;
}

// ------------------------------------------------------------------------------------------------
NLMISC::CVector CInterfaceElement::convertVector (const char *ptr)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;

	sscanf (ptr, "%f %f %f", &x, &y, &z);

	return CVector(x,y,z);
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::convertPixelsOrRatio(const char *ptr, sint32 &pixels, float &ratio)
{
	std::string value = ptr;
	if (!value.empty())
	{
		if (value[value.size() - 1] == '%')
		{
			value.resize(value.size() - 1);
			fromString(value, ratio);
			ratio /= 100.f;
			clamp(ratio, 0.f, 1.f);
		}
		else
		{
			fromString(value, pixels);
		}
	}
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::addLink(CInterfaceLink *link)
{
	nlassert(link != NULL);
	if (!_Links)
	{
		_Links = new TLinkVect;
	}
	TLinkSmartPtr linkPtr(link);
	TLinkVect::const_iterator it = std::find(_Links->begin(), _Links->end(), linkPtr);
	if (it != _Links->end())
	{
		// Link already appened : this can be the case when a link has several targets property that belong to the same element, in this case, one single ptr in the vector is enough.
		// nlwarning("Link added twice");
	}
	else
	{
		_Links->push_back(linkPtr);
	}
}


// ------------------------------------------------------------------------------------------------
void CInterfaceElement::removeLink(CInterfaceLink *link)
{
	nlassert(link != NULL);
	if (!_Links)
	{
		nlwarning("No link added");
		return;
	}
	TLinkVect::iterator it = std::find(_Links->begin(), _Links->end(), TLinkSmartPtr(link));
	if (it == _Links->end())
	{
		nlwarning("Unknown link");
		return;
	}
	_Links->erase(it); // kill the smart ptr, maybe deleting the link.
	if (_Links->empty())
	{
		delete _Links;
		_Links = NULL;
	}
}


// ------------------------------------------------------------------------------------------------
CInterfaceElement* CInterfaceElement::getMasterGroup() const
{
	if(getParent()==NULL)
		return const_cast<CInterfaceElement*>(this);
	else
		return getParent()->getMasterGroup();
}

// ------------------------------------------------------------------------------------------------
CGroupContainer *CInterfaceElement::getParentContainer()
{
	CInterfaceElement *parent = this;
	while (parent)
	{
		CGroupContainer *gc = dynamic_cast<CGroupContainer *>(parent);
		if (gc) return gc;
		parent = parent->getParent();
	}
	return NULL;
}

// ------------------------------------------------------------------------------------------------
bool CInterfaceElement::isIn(sint x, sint y) const
{
	return  (x >= _XReal) &&
			(x < (_XReal + _WReal))&&
			(y > _YReal) &&
			(y <= (_YReal+ _HReal));
}

// ------------------------------------------------------------------------------------------------
bool CInterfaceElement::isIn(sint x, sint y, uint width, uint height) const
{
	return (x + (sint) width) >= _XReal &&
		   (y + (sint) height) > _YReal &&
		   x < (_XReal + _WReal) &&
		   y <= (_YReal + _HReal);
}

// ------------------------------------------------------------------------------------------------
bool CInterfaceElement::isIn(const CInterfaceElement &other) const
{
	return isIn(other._XReal, other._YReal, other._WReal, other._HReal);
}

// ------------------------------------------------------------------------------------------------
void CInterfaceElement::setActive (bool state)
{
	if (_Active != state)
	{
		_Active = state;
		invalidateCoords();
	}
}


// ***************************************************************************
void		CInterfaceElement::invalidateCoords(uint8 numPass)
{
	// Get the "Root Group" ie the 1st son of the master group of us (eg "ui:interface:rootgroup" )
	CInterfaceGroup		*parent= getParent();
	// if our parent is NULL, then we are the master group (error!)
	if(parent==NULL)
		return;
	// if our grandfather is NULL, then our father is the Master Group => we are the "Root group"
	if(parent->getParent()==NULL)
	{
		parent= dynamic_cast<CInterfaceGroup*>(this);
	}
	else
	{
		// parent is the root group when is grandFather is NULL
		while( parent->getParent()->getParent()!=NULL )
		{
			parent= parent->getParent();
		}
	}

	// invalidate the "root group"
	if(parent)
	{
		uint8	&val= static_cast<CInterfaceElement*>(parent)->_InvalidCoords;
		val= max(val, numPass);
	}
}


// ***************************************************************************
void	CInterfaceElement::checkCoords()
{
}

// ***************************************************************************
bool CInterfaceElement::isSonOf(const CInterfaceElement *other) const
{
	const CInterfaceElement  *currElem = this;
	do
	{
		if (currElem == other) return true;
		currElem = currElem->_Parent;
	}
	while (currElem);
	return false;
}

// ***************************************************************************
void	CInterfaceElement::resetInvalidCoords()
{
	_InvalidCoords= 0;
}

// ***************************************************************************
void CInterfaceElement::updateAllLinks()
{
	if (_Links)
	{
		for(TLinkVect::iterator it = _Links->begin(); it != _Links->end(); ++it)
		{
			(*it)->update();
		}
	}
}

// ***************************************************************************
void CInterfaceElement::copyOptionFrom(const CInterfaceElement &other)
{
	_Active = other._Active;
	_InvalidCoords = other._InvalidCoords;
	_XReal = other._XReal;
	_YReal = other._YReal;
	_WReal = other._WReal;
	_HReal = other._HReal;
	_X = other._X;
	_Y = other._Y;
	_XReal = other._XReal;
	_YReal = other._YReal;
    _PosRef = other._PosRef;
	_ParentPosRef = other._ParentPosRef;
	_SizeRef = other._SizeRef;
	_SizeDivW = other._SizeDivW;
	_SizeDivH = other._SizeDivH;
	_ModulateGlobalColor = other._ModulateGlobalColor;
	_RenderLayer = other._RenderLayer;

}

// ***************************************************************************
void CInterfaceElement::center()
{
	// center the pc
	CInterfaceManager *im = CInterfaceManager::getInstance();
	CViewRenderer &vr = im->getViewRenderer();
	uint32 sw, sh;
	vr.getScreenSize(sw, sh);
	setX(sw / 2 - getWReal() / 2);
	setY(sh / 2 + getHReal() / 2);
}

// ***************************************************************************
void CInterfaceElement::renderWiredQuads(TRenderWired type, const std::string &uiFilter)
{
	CCtrlBase *ctrlBase = dynamic_cast<CCtrlBase*>(this);
	CInterfaceGroup *groupBase = dynamic_cast<CInterfaceGroup*>(this);
	if (
		((type == RenderView) && (ctrlBase==NULL) && (groupBase==NULL)) ||
		((type == RenderCtrl) && (ctrlBase!=NULL) && (groupBase==NULL)) ||
		((type == RenderGroup) && (ctrlBase!=NULL) && (groupBase!=NULL)))
	{
		if (!_Active) return;
		// if there is an uiFilter, the end of _Id must match it
		if (!uiFilter.empty() && (uiFilter.size()>_Id.size() ||
			_Id.compare(_Id.size()-uiFilter.size(),string::npos,uiFilter)!=0)
			)
			return;
		CInterfaceManager *im = CInterfaceManager::getInstance();
		CViewRenderer &vr = im->getViewRenderer();
		vr.drawWiredQuad(_XReal, _YReal, _WReal, _HReal);
		drawHotSpot(_PosRef, CRGBA::Red);
		if (_Parent) _Parent->drawHotSpot(_ParentPosRef, CRGBA::Blue);
	}
}

// ***************************************************************************
void CInterfaceElement::drawHotSpot(THotSpot hs, CRGBA col)
{
	const sint32 radius = 2;
	sint32 px, py;
	//
	if (hs & Hotspot_Bx)
	{
		py = _YReal + radius;
	}
	else if (hs & Hotspot_Mx)
	{
		py = _YReal + _HReal / 2;
	}
	else
	{
		py = _YReal + _HReal - radius;
	}
	//
	if (hs & Hotspot_xL)
	{
		px = _XReal + radius;
	}
	else if (hs & Hotspot_xM)
	{
		px = _XReal + _WReal / 2;
	}
	else
	{
		px = _XReal + _WReal - radius;
	}
	CInterfaceManager *im = CInterfaceManager::getInstance();
	CViewRenderer &vr = im->getViewRenderer();
	vr.drawFilledQuad(px - radius, py - radius, radius * 2, radius * 2, col);

}

// ***************************************************************************
void CInterfaceElement::invalidateContent()
{
	CInterfaceElement *elm = this;
	while (elm)
	{
		// Call back
		elm->onInvalidateContent();

		// Get the parent
		elm = elm->getParent();
	}
}

// ***************************************************************************
void CInterfaceElement::visit(CInterfaceElementVisitor *visitor)
{
	nlassert(visitor);
	visitor->visit(this);
}

// ***************************************************************************
void CInterfaceElement::serialConfig(NLMISC::IStream &f)
{
	if (f.isReading())
	{
		throw NLMISC::ENewerStream(f);
		nlassert(0);
	}
}

// ***************************************************************************
void CInterfaceElement::onFrameUpdateWindowPos(sint dx, sint dy)
{
	_XReal+= dx;
	_YReal+= dy;
}

// ***************************************************************************
void CInterfaceElement::dummySet(sint32 /* value */)
{
	nlwarning("Element can't be written.");
}

// ***************************************************************************
void CInterfaceElement::dummySet(const std::string &/* value */)
{
	nlwarning("Element can't be written.");
}

// ***************************************************************************
int CInterfaceElement::luaUpdateCoords(CLuaState &ls)
{
	CLuaIHM::checkArgCount(ls, "updateCoords", 0);
	updateCoords();
	return 0;
}

// ***************************************************************************
int CInterfaceElement::luaInvalidateCoords(CLuaState &ls)
{
	CLuaIHM::checkArgCount(ls, "updateCoords", 0);
	invalidateCoords();
	return 0;
}

// ***************************************************************************
int CInterfaceElement::luaInvalidateContent(CLuaState &ls)
{
	CLuaIHM::checkArgCount(ls, "invalidateContent", 0);
	invalidateContent();
	return 0;
}

// ***************************************************************************
int CInterfaceElement::luaCenter(CLuaState &ls)
{
	CLuaIHM::checkArgCount(ls, "center", 0);
	center();
	return 0;
}

// ***************************************************************************
int CInterfaceElement::luaSetPosRef(CLuaState &ls)
{
	CLuaIHM::checkArgCount(ls, "setPosRef", 1);
	CLuaIHM::check(ls,   ls.isString(1),    "setPosRef() requires a string in param 1");

	// get hotspot
	THotSpot	newParentPosRef, newPosRef;
	convertHotSpotCouple(ls.toString(1), newParentPosRef, newPosRef);

	// if different from current, set,a nd invalidate coords
	if(newParentPosRef!=getParentPosRef() || newPosRef!=getPosRef())
	{
		setParentPosRef(newParentPosRef);
		setPosRef(newPosRef);
		invalidateCoords();
	}

	return 0;
}

// ***************************************************************************
int CInterfaceElement::luaSetParentPos(CLuaState &ls)
{
	CLuaIHM::checkArgCount(ls, "setParentPos", 1);
	CInterfaceElement *ie = CLuaIHM::getUIOnStack(ls, 1);
	if(ie)
	{
		setParentPos(ie);
	}
	return 0;
}



// ***************************************************************************
CInterfaceElement *CInterfaceElement::clone()
{
	NLMISC::CMemStream dupStream;
	nlassert(!dupStream.isReading());
	CInterfaceGroup *oldParent = _Parent;
	_Parent = NULL;
	CInterfaceElement *oldParentPos = _ParentPos;
	CInterfaceElement *oldParentSize = _ParentSize;
	if (_ParentPos == oldParent) _ParentPos = NULL;
	if (_ParentSize == oldParent) _ParentSize = NULL;
	CInterfaceElement *begunThisCloneWarHas = NULL;
	try
	{
		if (dupStream.isReading())
		{
			dupStream.invert();
		}
		CInterfaceElement *self = this;
		dupStream.serialPolyPtr(self);
		std::vector<uint8> datas(dupStream.length());
		std::copy(dupStream.buffer(), dupStream.buffer() + dupStream.length(), datas.begin());
		dupStream.resetPtrTable();
		dupStream.invert();
		dupStream.fill(&datas[0], (uint32)datas.size());
		dupStream.serialPolyPtr(begunThisCloneWarHas);
	}
	catch(NLMISC::EStream &)
	{
		// no-op -> caller has to handle the failure because NULL will be returned
	}
	//
	_Parent		  = oldParent;
	_ParentPos	  = oldParentPos;
	_ParentSize	  = oldParentSize;
	//
	return begunThisCloneWarHas;
}

// ***************************************************************************
void CInterfaceElement::serial(NLMISC::IStream &f)
{
	f.serialPolyPtr(_Parent);
	f.serial(_Id);
	f.serial(_Active);
	f.serial(_InvalidCoords);
	f.serial(_XReal, _YReal, _WReal, _HReal);
	f.serial(_X, _Y, _W, _H);
	f.serialEnum(_PosRef);
	f.serialEnum(_ParentPosRef);
	_ParentPos.serialPolyPtr(f);
	f.serial(_SizeRef);
	f.serial(_SizeDivW, _SizeDivH);
	_ParentSize.serialPolyPtr(f);
	f.serial(_ModulateGlobalColor);
	f.serial(_RenderLayer);
	f.serial(_AvoidResizeParent);
	nlassert(_Links == NULL); // not supported
}


// ***************************************************************************
void CInterfaceElement::serialAH(NLMISC::IStream &f, IActionHandler *&ah)
{
	std::string ahName;
	if (f.isReading())
	{
		f.serial(ahName);
		ah = CActionHandlerFactoryManager::getInstance()->getActionHandler(ahName);
	}
	else
	{
		ahName = CActionHandlerFactoryManager::getInstance()->getActionHandlerName(ah);
		f.serial(ahName);
	}
}





