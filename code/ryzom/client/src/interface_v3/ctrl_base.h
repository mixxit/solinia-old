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



#ifndef RZ_CTRL_BASE_H
#define RZ_CTRL_BASE_H

#include "nel/misc/types_nl.h"
#include "view_base.h"
#include "event_descriptor.h"

class CCtrlBase : public CViewBase
{
public:

	// Tooltip mode
	enum	TToolTipParentType
	{
		TTMouse= 0,		// The tooltip is displayed relatively to the mouse when it appears
		TTCtrl= 1,		// The tooltip is displayed relatively to the ctrl it comes from when it apeears
		TTWindow= 2,	// The tooltip is displayed relatively to the window where the control lies.
		TTSpecialWindow= 3,	// The tooltip is displayed relatively to a special user window

		NumToolTipParentRef
	};

public:

	/// Constructor
	CCtrlBase(const TCtorParam &param) : CViewBase(param)
	{
		_ToolTipInstant= true;
		_ToolTipParent= TTCtrl;
		// see interface.txt for meaning of auto
		_ToolTipParentPosRef= Hotspot_TTAuto;
		_ToolTipPosRef= Hotspot_TTAuto;
	}

	/// Destructor
	virtual ~CCtrlBase();

	// special parse
	virtual bool		parse (xmlNodePtr cur, CInterfaceGroup *parentGroup);


	/// Handle all events (implemented by derived classes) (return true to signal event handled)
	virtual bool		handleEvent (const CEventDescriptor &event);

	virtual CCtrlBase	*getSubCtrl (sint32 /* x */, sint32 /* y */) { return this; }

	/// Debug
	virtual uint32		getMemory() { return (uint32)(sizeof(*this)+_Id.size()); }


	/// Get the ContextHelp for this control. Default is to return _ContextHelp
	virtual void		getContextHelp(ucstring &help) const {help= _ContextHelp;}
	// Get the name of the context help window. Default to "context_help"
	virtual std::string getContextHelpWindowName() const;
	/// Get the ContextHelp ActionHandler. If "", noop
	const std::string	&getContextHelpActionHandler() const {return _OnContextHelp;}
	/// Get the ContextHelp ActionHandler Params
	const std::string	&getContextHelpAHParams() const {return _OnContextHelpParams;}
	/// true if both are empty
	bool				emptyContextHelp() const;
	// Should return true if the context help should be displayed instantly
	bool				wantInstantContextHelp() const { return _ToolTipInstant; }
	/// Set true if ToolTip should be displayed instantly
	void				setInstantContextHelp(bool instant) { _ToolTipInstant = instant;}

	/** If ctrl has a non rectangle shape, perform further test to know
	  * if control should be taken in account for context help
	  */
	virtual bool		preciseHitTest(sint32 /* x */, sint32 /* y */) const { return true; }


	/// return the type of anchor for the tooltip of this control
	TToolTipParentType	getToolTipParent() const { return _ToolTipParent;}
	const std::string	&getToolTipSpecialParent() const {return _ToolTipSpecialParent.toString();}
	/// Set the type of anchor for the tooltip of this control
	void				setToolTipParent(TToolTipParentType type)  { _ToolTipParent = type; }
	void				setToolTipSpecialParent(const std::string &parent)  { _ToolTipSpecialParent = parent; }
	/// Get the ToolTip pos references (parent relevant only if getToolTipParent()!=TTMouse)
	THotSpot			getToolTipParentPosRef() const { return _ToolTipParentPosRef;}
	THotSpot			getToolTipPosRef() const { return _ToolTipPosRef;}
	THotSpot			getToolTipParentPosRefAlt() const { return _ToolTipParentPosRefAlt;}
	THotSpot			getToolTipPosRefAlt() const { return _ToolTipPosRefAlt;}
	/// Set the ToolTip pos references (parent relevant only if getToolTipParent()!=TTMouse)
	void				setToolTipParentPosRef(THotSpot pos) { _ToolTipParentPosRef = pos;}
	void				setToolTipPosRef(THotSpot pos) { _ToolTipPosRef = pos;}

	/// replace the default contextHelp
	ucstring			getDefaultContextHelp() const {return _ContextHelp;}
	void				setDefaultContextHelp(const ucstring &help) {_ContextHelp= help;}
	void				setOnContextHelp(const std::string &help) {_OnContextHelp= help;}
	void				setOnContextHelpAHParams(const std::string &p) {_OnContextHelpParams= p;}



	// called when this element or a son has been captured
	virtual	void		elementCaptured(CCtrlBase * /* capturedElement */) {}

	virtual bool isCtrl() const { return true; }

	// Made for CtrlResizer to take the precedence over son controls.
	virtual uint		getDeltaDepth() const { return 0; }

	// true if this ctrl is capturable (true by default, false for tooltip)
	virtual	bool		isCapturable() const {return true;}

	// from CInterfaceElement
	virtual void		visit(CInterfaceElementVisitor *visitor);

	/** test if virtual desktop change is possible while this element is captured by the mouse
	  * Useful for resizers
	  */
	virtual bool		canChangeVirtualDesktop() const { return true; }

	// called when keyboard capture has been lost
	virtual void		onKeyboardCaptureLost() {}

	REFLECT_EXPORT_START(CCtrlBase, CViewBase)
		REFLECT_UCSTRING("tooltip", getDefaultContextHelp, setDefaultContextHelp);
	REFLECT_EXPORT_END

	// special for mouse over : return true and fill the name of the cursor to display
	virtual bool getMouseOverShape(std::string &/* texName */, uint8 &/* rot */, NLMISC::CRGBA &/* col */) { return false; }

	virtual void serial(NLMISC::IStream &f);
protected:
	// This is the ContextHelp filled by default in parse()
	ucstring			_ContextHelp;
	CStringShared		_OnContextHelp;
	CStringShared		_OnContextHelpParams;
	CStringShared		_ToolTipSpecialParent;
	TToolTipParentType	_ToolTipParent;
	bool				_ToolTipInstant		    : 1;
	THotSpot			_ToolTipParentPosRef    : 6;
	THotSpot			_ToolTipPosRef          : 6;
	THotSpot			_ToolTipParentPosRefAlt : 6;
	THotSpot			_ToolTipPosRefAlt       : 6;
protected:
	void convertTooltipHotSpot(const char *prop, THotSpot &parentHS, THotSpot &childHS);
};


#endif // RZ_VIEW_BASE_H

/* End of ctrl_base.h */
