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
#include "group_scrolltext.h"
#include "group_list.h"
#include "view_text.h"
#include "ctrl_scroll.h"
#include "ctrl_button.h"
#include "action_handler.h"

#include "../time_client.h"
#include "nel/misc/i18n.h"

NLMISC_REGISTER_OBJECT(CViewBase, CGroupScrollText, std::string, "scroll_text");

//========================================================================
CGroupScrollText::CGroupScrollText(const TCtorParam &param) :
										CInterfaceGroup(param),
										_List(NULL),
										_ScrollBar(NULL),
										_Settuped(false),
										_InvertScrollBar(true),
										_ListHeight(0),
										_ButtonAdd(NULL),
										_ButtonSub(NULL),
										_StartHeight(0),
										_EllapsedTime(0)
{
}


//========================================================================
CGroupScrollText::~CGroupScrollText()
{
}

//========================================================================
bool CGroupScrollText::parse(xmlNodePtr cur,CInterfaceGroup *parentGroup)
{
	if(!CInterfaceGroup::parse(cur, parentGroup))
		return false;
	CXMLAutoPtr ptr;

	// invert scroll bar?
	ptr = xmlGetProp (cur, (xmlChar*)"invert_scroll_bar");
	if(ptr)		_InvertScrollBar= convertBool(ptr);

	return true;
}

//========================================================================
void CGroupScrollText::updateCoords()
{
	CInterfaceGroup::updateCoords();
	if (!_Settuped) setup();
	updateScrollBar();
	// re-update of scrollbar
	if(_ScrollBar)
		_ScrollBar->updateCoords();
}

//========================================================================
void CGroupScrollText::updateScrollBar()
{
	if (_List && _ScrollBar)
	{
		if (_List->getHReal() < _List->getMaxHReal())
		{
			_ScrollBar->setActive(false);
		}
		else
		{
			_ScrollBar->setActive(true);
		}
	}
}

//========================================================================
void CGroupScrollText::checkCoords ()
{
	// update scrollbar if necessary
	if (_List)
	{
		if (_List->getH() != _ListHeight) // see if the scrollbar should be updated
		{
			invalidateCoords();
			_ListHeight = _List->getH();
		}
	}
	CInterfaceGroup::checkCoords();
}

//========================================================================
void CGroupScrollText::draw()
{
	CInterfaceGroup::draw();
}

//========================================================================
void CGroupScrollText::clearViews()
{
	CInterfaceGroup::clearViews();
}

//========================================================================
bool CGroupScrollText::handleEvent(const CEventDescriptor &event)
{
	if (event.getType() == CEventDescriptor::mouse)
	{
		const CEventDescriptorMouse &eventDesc = (const CEventDescriptorMouse &)event;
		//
		if (_List && _ScrollBar)
		{
			if (eventDesc.getEventTypeExtended() == CEventDescriptorMouse::mousewheel)
			{
				if (isIn(eventDesc.getX(), eventDesc.getY()))
				{
					sint32 h = _List->getMaxHReal() / 2;
					if (h == 0) h = 1;
					_ScrollBar->moveTargetY(- eventDesc.getWheel() * h);
					return true;
				}
			}
		}
	}
	if (CInterfaceGroup::handleEvent(event)) return true;
	return false;
}

//========================================================================
void CGroupScrollText::setup()
{
	// bind to the controls
	_ScrollBar = dynamic_cast<CCtrlScroll *>(CInterfaceGroup::getCtrl("scroll_bar"));
	_ButtonAdd = dynamic_cast<CCtrlBaseButton *>(CInterfaceGroup::getCtrl("button_add"));
	_ButtonSub = dynamic_cast<CCtrlBaseButton *>(CInterfaceGroup::getCtrl("button_sub"));
	_List	   = dynamic_cast<CGroupList *>(CInterfaceGroup::getGroup("text_list"));

	if(_ScrollBar == NULL)
		nlwarning("<setup> scroll bar 'scroll_bar' missing or bad type.(%s)",this->_Id.c_str());
	// Add and sub button are not required
	/*
	if(buttonAdd == NULL)
		nlwarning("Interface: CGroupScrollText: button 'button_add' missing or bad type");
	if(buttonSub == NULL)
		nlwarning("Interface: CGroupScrollText: button 'button_sub' missing or bad type");
	*/
	if(_List == NULL)
		nlwarning("<setup> group list 'text_list' missing or bad type");

	// actions
	if (_ButtonAdd) _ButtonAdd->setActionOnClockTick("gst_add");
	if (_ButtonSub) _ButtonSub->setActionOnClockTick("gst_sub");

	// bind the scrollbar to the list
	if (_ScrollBar)
	{
		_ScrollBar->setTarget(_List);
		//_ScrollBar->setInverted(_InvertScrollBar);
	}
	_Settuped = true;
}

//========================================================================
void CGroupScrollText::elementCaptured(CCtrlBase *capturedElement)
{
	if (capturedElement == _ButtonAdd || capturedElement == _ButtonSub)
	{
		// reset the counters for increase
		_EllapsedTime = 0;
		_StartHeight = getH();
	}
}

// ***************************************************************************
// ***************************************************************************
// Actions Handlers
// ***************************************************************************
// ***************************************************************************


// ***************************************************************************
class CSTUp : public IActionHandler
{
public:
	virtual void execute (CCtrlBase *pCaller, const std::string &/* Params */)
	{
		CGroupScrollText *pST = dynamic_cast<CGroupScrollText*>(pCaller->getParent());
		if (pST == NULL) return;
		if (pST->getList() == NULL) return;
		// get the font height from the text template of the list
		const CViewText *vt = pST->getList()->getTextTemplatePtr();
		if (!vt) return;
		pST->_EllapsedTime += DT64;
//		pST->setH(std::min((sint32) pST->getMaxHeight(), (sint32) (pST->_StartHeight + pST->_EllapsedTime / 9)));
		pST->setH((sint32) (pST->_StartHeight + pST->_EllapsedTime / 9));
		pST->invalidateCoords();
	}
};
REGISTER_ACTION_HANDLER (CSTUp, "gst_add");

// ***************************************************************************
class CSTDown : public IActionHandler
{
public:
	virtual void execute (CCtrlBase *pCaller, const std::string &/* Params */)
	{
		CGroupScrollText *pST = dynamic_cast<CGroupScrollText*>(pCaller->getParent());
		if (pST == NULL) return;
		if (pST->getList() == NULL) return;
		// get the font height from the text template of the list
		const CViewText *vt = pST->getList()->getTextTemplatePtr();
		if (!vt) return;
		pST->_EllapsedTime += DT64;
//		pST->setH(std::max((sint32) pST->getMinHeight(), (sint32) (pST->_StartHeight - pST->_EllapsedTime / 9)));
		pST->setH((sint32) (pST->_StartHeight - pST->_EllapsedTime / 9));
		pST->invalidateCoords();
	}
};
REGISTER_ACTION_HANDLER (CSTDown, "gst_sub");

