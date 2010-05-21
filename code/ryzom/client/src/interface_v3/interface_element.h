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



#ifndef NL_INTERFACE_ELEMENT_H
#define NL_INTERFACE_ELEMENT_H

#include "nel/misc/types_nl.h"
#include "nel/misc/string_mapper.h"
#include "nel/misc/smart_ptr.h"
#include "interface_property.h"
#include "reflect.h"

// ----------------------------------------------------------------------------
class CInterfaceGroup;

// ----------------------------------------------------------------------------
enum THotSpot
{
	Hotspot_BL = 36,	// 100100,
	Hotspot_BM = 34,	// 100010,
	Hotspot_BR = 33,	// 100001,
	Hotspot_ML = 20,	// 010100,
	Hotspot_MM = 18,	// 010010
	Hotspot_MR = 17,	// 010001
	Hotspot_TL = 12,	// 001100
	Hotspot_TM = 10,	// 001010
	Hotspot_TR = 9,		// 001001
	Hotspot_xR = 1,		// 000001
	Hotspot_xM = 2,		// 000010
	Hotspot_xL = 4,		// 000100
	Hotspot_Bx = 32,	// 100000
	Hotspot_Mx = 16,	// 010000
	Hotspot_Tx = 8,		// 001000
	Hotspot_TTAuto = 0,	// Special For Tooltip PosRef. Auto mode. see CCtrlBase and tooltip info
};


class CInterfaceLink;
class CInterfaceElement;
class CInterfaceGroup;
class CViewBase;
class CCtrlBase;
class CGroupContainer;
class IActionHandler;

/**
 * A visitor to walk a tree of interface elements and apply a teartment on them.
 *
 * For each vsited element, visitElement() is called
 * If the element is a control, then visitCtrl() is called, and then visitElement()
 * If the element is a view, then visitView() is called, and then visitElement()
 * If the element is a group, then visitGoup() is called, and then visitElement()
 *
 * \author Nicolas Vizerie
 * \author Nevrax France
 * \date 2003
 */
class CInterfaceElementVisitor
{
public:
	virtual void visit(CInterfaceElement * /* elem */) {}
	virtual void visitGroup(CInterfaceGroup * /* group */) {}
	virtual void visitView(CViewBase * /* view */) {}
	virtual void visitCtrl(CCtrlBase * /* ctrl */) {}
};


/** Reflectable refcounted object
  * NB nico : added this intermediate class so that the binding from lua to the reflection
  * system that are found in CLuaIHM can be reused for other objects as well
  * NOTE: The class is named 'CReflectableRefPtrTarget' and not 'CReflectableRefCount'
  * because the refcount part is only used for ref pointing in the ui
  */
class CReflectableRefPtrTarget : public CReflectable, public NLMISC::CRefCount
{
public:
	virtual ~CReflectableRefPtrTarget();
};


#define	DECLARE_UI_CLASS(_class_)					\
	virtual std::string	getClassName() {return #_class_;}		\
	static	NLMISC::IClassable	*creator() {return new _class_(CViewBase::TCtorParam());}
#define	REGISTER_UI_CLASS(_class_)  \
	class CRegisterUIClassHelper_##_class_ \
	{ \
	public: \
		CRegisterUIClassHelper_##_class_() \
		{ \
			NLMISC::CClassRegistry::init(); \
			NLMISC::CClassRegistry::registerClass(#_class_, _class_::creator, typeid(_class_).name()); \
		} \
	} RegisterUIClassHelper_##_class_;



/**
 * class describing a localisable interface element, i.e. : an element with coordinates
 * \author Nicolas Brigand
 * \author Nevrax France
 * \date 2002
 */
class CInterfaceElement : public CReflectableRefPtrTarget, public NLMISC::IStreamable
{
public:


	enum EStrech
	{
		none=0,
		width=1,
		height=2
	};

	/// Constructor
	CInterfaceElement()
	{
		_Parent = NULL;

		_XReal = _YReal = _WReal = _HReal = 0;
		_X = _Y = _W = _H = 0;
		//_Snap = 1;

		_PosRef = Hotspot_BL;
		_ParentPosRef = Hotspot_BL;
		_ParentPos = NULL;

		_SizeRef = 0;
		_SizeDivW = 10;
		_SizeDivH = 10;
		_ParentSize = NULL;

		_Links = NULL;
		_Active= true;
		// default to 3 pass
		_InvalidCoords= 3;

		_ModulateGlobalColor= true;
		_RenderLayer= 0;

		_AvoidResizeParent= false;
	}

	// dtor
	virtual ~CInterfaceElement();

	/** Cloning
	  * Cloning is actually performed using a serial / unserial in a memory stream
	  * NB Nico : if too slow, should use a CFastStream version instead, that is designedto work in memory only
	  */
	virtual CInterfaceElement *clone();

	// help to serialize an action handler
	static void serialAH(NLMISC::IStream &f, IActionHandler *&ah);


	/// Parse the element and initalize it
	virtual bool parse (xmlNodePtr cur, CInterfaceGroup *parentGroup);

	/// Debug info on memory
	virtual uint32 getMemory () { return (uint32)(sizeof(*this)+_Id.size()); }

	/// helper: display a parse error with the id of the lement
	void parseError (CInterfaceGroup *parentGroup, const char *reason = NULL);

	/// Accessors : GET
	const std::string& getId() const { return _Id; }
	std::string getShortId() const;
	std::string getIdByValue() const { return _Id; }

	CInterfaceGroup* getParent() const { return _Parent; }

	CInterfaceElement* getParentPos() const { return _ParentPos; }

	CInterfaceElement* getParentSize() const { return _ParentSize; }

	/// Get the master group of this element (recurs call.
	CInterfaceElement* getMasterGroup() const;

	// get a possible group container
	CGroupContainer *getParentContainer();

	bool getActive() const { return _Active; }

	sint32 getX() const { return _X; }

	sint32 getY() const { return _Y; }

	sint32 getW() const { return (_Active?_W:0); }
	sint32 getW(bool bTestActive) const { return (bTestActive?(_Active?_W:0):_W); }

	sint32 getH() const { return (_Active?_H:0); }
	sint32 getH(bool bTestActive) const { return (bTestActive?(_Active?_H:0):_H); }

	/**
	  * Get the max width used by the window.
	  *
	  * The view must return the largest width its content can take if it will be resized at maximum.
	  * Typical use : for a CTextView multiline, it returns the size of the whole string.
	  *
	  * This method is used by the container CCGroupTable that need to know this information about its children in its resizing algorithm.
	  */
	virtual sint32	getMaxUsedW() const { return INT_MAX; };

	/**
	  * Get the min width used by the window.
	  *
	  * The view must return the smallest width its content can take if it will be resized at minimum.
	  * Typical use : for a CTextView multiline without word clipping, it returns the size of the largest word.
	  *
	  * This method is used by the container CCGroupTable that need to know this information about its children in its resizing algorithm.
	  */
	virtual sint32	getMinUsedW() const { return 0; };

	//bool isSnapped() const { return (_Snap>1); }

	//sint32 getSnapping() const { return _Snap; }

	sint32 getXReal() const { return _XReal; }

	sint32 getYReal() const { return _YReal; }

	sint32 getWReal() const { return (_Active?_WReal:0); }

	sint32 getHReal() const { return (_Active?_HReal:0); }

	THotSpot getPosRef () const { return _PosRef; }

	THotSpot getParentPosRef () const { return _ParentPosRef; }

	sint32   getSizeRef() const { return _SizeRef; } // none == 0, w == 1, h == 2, wh == 3

	void	 setSizeRef(sint32 value) { _SizeRef = value; }

	/// Accessors : SET
	void setId (const std::string &newID) { _Id = newID; }

	virtual void setIdRecurse(const std::string &newID);

	void setParent (CInterfaceGroup *pIG) { _Parent = pIG; }

	void setParentPos (CInterfaceElement *pIG) { _ParentPos = pIG; }

	void setParentSize (CInterfaceElement *pIG) { _ParentSize = pIG; }

	virtual void setActive (bool state);

	void setX (sint32 x) { _X = x; }
	void setXAndInvalidateCoords (sint32 x) { _X = x; invalidateCoords(); }

	void setY (sint32 y) { _Y = y; }
	void setYAndInvalidateCoords (sint32 y) { _Y = y; invalidateCoords(); }

	void setW (sint32 w);
	void setWAndInvalidateCoords (sint32 w) { setW(w); invalidateCoords(); }

	void setH (sint32 h);
	void setHAndInvalidateCoords (sint32 h) { setH(h); invalidateCoords(); }

	void setPosRef (THotSpot hs) { _PosRef = hs; }

	void setParentPosRef (THotSpot hs) { _ParentPosRef = hs; }

	// Get the coordinate of a corner on screen
	void getCorner(sint32 &px, sint32 &py, THotSpot hotSpot);

	/** Test if the given coordinates are inside this element
	  */
	bool isIn(sint x, sint y) const;

	/** Test if the given box intersect the element
	  */
	bool isIn(sint x, sint y, uint width, uint height) const;

	/** Test if another interface element intersect this one
	  */
	bool isIn(const CInterfaceElement &other) const;

	/**
	 * get the window containing the element
	 * \return NULL if the element is not on the window, otherwise returns a pointer to the window
	 */
	CInterfaceGroup* getRootWindow();

	/** get the element Depth, ie the number of parent he has (0 if _Parent==NULL)
	 *	\warning slow test. don't take into account CCtrlBase::getDeltaDepth() (since method not virtual)
	 */
	uint	getParentDepth() const;

	/**
	 * true if the element and all its parents are active (recurs up to the root window)
	 */
	bool isActiveThroughParents() const;

	/**
	 * move the element (add dx and dy to its coords)
	 * \param dx : value added to _X
	 * \param dy : value added to _Y
	 */
	virtual void move (sint32 dx, sint32 dy);

	//void resizeBR (sint32 sizex, sint32 sizey);
	//void stopResizeBR();
	//void startResizeBR();

	// Some tools

	void relativeSInt64Read (CInterfaceProperty &rIP, const std::string &prop, const char *val,
							const std::string &defVal);
	void relativeSInt32Read (CInterfaceProperty &rIP, const std::string &prop, const char *val,
							const std::string &defVal);
	void relativeBoolRead (CInterfaceProperty &rIP, const std::string &prop, const char *val,
							const std::string &defVal);
	void relativeRGBARead (CInterfaceProperty &rIP, const std::string &prop, const char *val,
							const std::string &defVal);

	// Parse tools
	static	THotSpot		convertHotSpot (const char *ptr);		//
	static	void			convertHotSpotCouple (const char *ptr, THotSpot &parentPosRef, THotSpot &posRef);
	static	NLMISC::CRGBA	convertColor (const char *ptr);
	static	bool			convertBool (const char *ptr);
	static	NLMISC::CVector convertVector (const char *ptr);
	/** Convert a value that is in the form like width="50" or width="10%"
      * if the value is absolute like '50' then 'pixels' is filled, else
	  * the ratio is remapped to the [0, 1] range and is copied in 'ratio'
	  */
	static	void convertPixelsOrRatio(const char *ptr, sint32 &pixels, float &ratio);

	// add an interface link to that element (kept in a smart ptr)
	void addLink(CInterfaceLink *link);

	// remove a link from that element; There's one less reference on the link (which is referenced by a smart ptr)
	void removeLink(CInterfaceLink *link);

	/** Update all links for this instance and its sons.
	  * Derivers should override this to update their sons.
	  */
	virtual void updateAllLinks();


	/** This allow to force opening an element. By default it just activate the element.
	  * It allow to have different behaviour on more complex containers
	  */
	virtual void forceOpen() { setActive(true); }

	virtual void enableBlink(int /* numBlinks */ = 0) {}
	virtual void disableBlink() {}
	virtual bool getBlink() const { return false; }

	// Options for views to be modulated by interface global color or not. Parsed with "global_color". Default: true
	void		setModulateGlobalColor(bool state) {_ModulateGlobalColor= state;}
	bool		getModulateGlobalColor() const {return _ModulateGlobalColor;}


	void dummySet(sint32 value);
	void dummySet(const std::string &value);

	// lua methods
	int luaUpdateCoords(CLuaState &ls);
	int luaInvalidateCoords(CLuaState &ls);
	int luaInvalidateContent(CLuaState &ls);
	int luaCenter(CLuaState &ls);
	int luaSetPosRef(CLuaState &ls);
	int luaSetParentPos(CLuaState &ls);

	// set sizeref as a string, like "wh", "wh5" ....
	void setSizeRef(const std::string &sizeref);
	std::string getSizeRefAsString() const;

	// export some properties
	REFLECT_EXPORT_START(CInterfaceElement, CReflectable)
		REFLECT_BOOL ("active", getActive, setActive);
		REFLECT_BOOL ("global_color", getModulateGlobalColor, setModulateGlobalColor);
		REFLECT_SINT32 ("x", getX, setXAndInvalidateCoords);
		REFLECT_SINT32 ("y", getY, setYAndInvalidateCoords);
		REFLECT_SINT32 ("w", getW, setWAndInvalidateCoords);
		REFLECT_SINT32 ("h", getH, setHAndInvalidateCoords);
		REFLECT_SINT32 ("x_real", getXReal, dummySet);
		REFLECT_SINT32 ("y_real", getYReal, dummySet);
		REFLECT_SINT32 ("w_real", getWReal, dummySet);
		REFLECT_SINT32 ("h_real", getHReal, dummySet);
		REFLECT_STRING ("id", getIdByValue, dummySet);
		REFLECT_STRING ("sizeref", getSizeRefAsString, setSizeRef);
		REFLECT_LUA_METHOD("updateCoords", luaUpdateCoords);
		REFLECT_LUA_METHOD("invalidateCoords", luaInvalidateCoords);
		REFLECT_LUA_METHOD("invalidateContent", luaInvalidateContent);
		REFLECT_LUA_METHOD("center", luaCenter);
		REFLECT_LUA_METHOD("setPosRef", luaSetPosRef);
		REFLECT_LUA_METHOD("setParentPos", luaSetParentPos);
	REFLECT_EXPORT_END


	/* invalidate coords. set numPass==1 if you are SURE that only XReal/YReal need to be updated
	 * Default 3 is needed for:
	 *	1: update _W/_H and _WReal/_HReal according to Sons
	 *	2: update XReal/YReal (eg: according to Scroll offset)
	 */
	void			invalidateCoords(uint8 numPass= 2);
	uint8			getInvalidCoords() const {return _InvalidCoords;}

	/* Invalidates the content of the window. This method invalidate the content of the window (for CViewText or CGroupTable).
	 * It invalidates the content of the parent too.
	 */
	void			invalidateContent();

	/* This call back is called when the content or the content of a child is been invalidated.
	 */
	virtual void	onInvalidateContent() {}

	// called by interfaceManager for master window only
	void			resetInvalidCoords();

	/// Update the elements coords convert x,y,w,h (parentpos coord) to xreal,yreal,wreal,hreal (BL coord)
	virtual void	updateCoords();
	/// Called each frame before draw to possibly invalidateCoords().
	virtual	void	checkCoords();

	/// Test if this element is son of the given element
	bool			isSonOf(const CInterfaceElement *other) const;

	/// Called after first frame initialised
	virtual void	launch () {}

	void  setRenderLayer(sint8 rl) {_RenderLayer= rl;}
	sint8 getRenderLayer() const { return _RenderLayer; }

	void    copyOptionFrom(const CInterfaceElement &other);

	// center the element in middle of screen
	void	center();

	// for debug only: draw wired quad to see where groups and hotspots are
	enum TRenderWired
	{
		RenderView,
		RenderCtrl,
		RenderGroup
	};
	// if uiFilter is not empty, draw a quad only if the element id match
	virtual void renderWiredQuads(TRenderWired type, const std::string &uiFilter);

	void drawHotSpot(THotSpot hs, NLMISC::CRGBA col);

	// Returns 'true' if that element can be downcasted to a view
	virtual bool isView() const { return false; }

	// Returns 'true' if that element can be downcasted to a ctrl
	virtual bool isCtrl() const { return false; }

	// Returns 'true' if that element can be downcasted to an interface group
	virtual bool isGroup() const { return false; }


	/** This is called before the config loading begins. This is the place to restore default state for config info.
      */
	virtual void onLoadConfig() {}
	/** Tells wether that element wants to save info in a config stream. If this returns true, then serialConfig
	  * is called.
	  */
	virtual bool wantSerialConfig() const { return false; }
	// Serial config info about that element. This is called only if wantSerialConfig() returns true
	virtual void serialConfig(NLMISC::IStream &f);

	// visit the node of the ui tree
	virtual void visit(CInterfaceElementVisitor *visitor);

	/** When user is quitting the interface, this is called. Then the interface config is saved
	  * This is where the element get the opportunity to do some cleanup.
	  */
	virtual void onQuit() {}

	/// Whent an element is added to a CInterfaceGroup via addCtrl, addGroup or addView, this is called after the add.
	virtual	void onAddToGroup() {}

	/** typically used only in conjunction with CGroupInScene. Such groups move every Frames. so
	 *	this function is called on each children elements to move the XReal/Yreal only (with a delta)
	 */
	virtual void	onFrameUpdateWindowPos(sint dx, sint dy);

	/// if true, InterfaceGroup child resize won't take this element into account
	bool	avoidResizeParent() const {return _AvoidResizeParent;}
	void	setAvoidResizeParent(bool state) {_AvoidResizeParent= state;}

	virtual std::string	getClassName()
	{
		nlassert(0); // forgot to implement serial & to register the class ?
		return "";
	}

protected:

	///the parent
	CInterfaceGroup* _Parent;

	///the id of the element
	std::string _Id;

	///is the element active?
	bool	_Active;

	// if 0, don't need updateCoords(), else tell the number of pass needed
	uint8	_InvalidCoords;

	// Real display coords
	sint32 _XReal, _YReal, _WReal, _HReal;

	// Relative coords
	sint32 _X;
	sint32 _Y;
	sint32 _W;
	sint32 _H;

	//sint32 _Snap;

	// position references e.g. : _PosRef=BL, _ParentPosref=MM : the bottom left corner of the element
	// will be placed on the center (middle middle) of the parent window
	THotSpot _PosRef;
	THotSpot _ParentPosRef;
	NLMISC::CRefPtr<CInterfaceElement>	_ParentPos;		// RefPtr in case of group destroyed in a parent group with posref on it

	sint32 _SizeRef; // none == 0, w == 1, h == 2, wh == 3
	sint32 _SizeDivW, _SizeDivH;
	NLMISC::CRefPtr<CInterfaceElement>	_ParentSize;	// RefPtr in case of group destroyed in a parent group with posref on it

	// Friend Class
	friend class CGroupList;
	friend class CGroupParagraph;

	// True if must modulate the global color with the view
	bool		_ModulateGlobalColor;
	// Index of layer to render it.
	sint8		_RenderLayer;

	// Used for CInterfaceGroup ChildResize feature
	bool		_AvoidResizeParent;


	virtual void serial(NLMISC::IStream &f);

	void parseSizeRef(const char *sizeRef);
	void parseSizeRef(const char *sizeRefStr, sint32 &sizeref, sint32 &sizeDivW, sint32 &sizeDivH);

private:
	//void	snapSize();

	typedef NLMISC::CSmartPtr<CInterfaceLink> TLinkSmartPtr;
	typedef std::vector<TLinkSmartPtr> TLinkVect;
	TLinkVect *_Links; // links, or NULL if no link
};

extern NLMISC::CStringMapper *_UIStringMapper;
/**
 * class to compress string usage in the interface
 * \author Matthieu 'Trap' Besson
 * \author Nevrax France
 * \date October 2003
 */
class CStringShared
{

public:
	CStringShared()
	{
		_Id = NLMISC::CStringMapper::emptyId();
	}

	const CStringShared& operator=(const std::string &str)
	{
		_Id = _UIStringMapper->localMap(str);
		return *this;
	}

	const CStringShared& operator=(const CStringShared &str)
	{
		_Id = str._Id;
		return *this;
	}

	const std::string &toString() const
	{
		return _UIStringMapper->localUnmap(_Id);
	}

	operator const std::string &() const
	{
		return _UIStringMapper->localUnmap(_Id);
	}

	bool empty() const
	{
		return _Id == NLMISC::CStringMapper::emptyId();
	}

	static CStringShared emptyString()
	{
		return CStringShared();
	}

	NLMISC::TStringId getStringId() const { return _Id; }

	void serial(NLMISC::IStream &f)
	{
		std::string str;
		if (f.isReading())
		{
			f.serial(str);
			*this = str;
		}
		else
		{
			str = this->toString();
			f.serial(str);
		}
	}

private:

	NLMISC::TStringId	_Id;
};

inline bool operator==(const CStringShared &lhs, const CStringShared &rhs) { return lhs.getStringId() == rhs.getStringId(); }
inline bool operator!=(const CStringShared &lhs, const CStringShared &rhs) { return !(lhs == rhs); }


#endif // NL_INTERFACE_ELEMENT_H

/* End of interface_element.h */
