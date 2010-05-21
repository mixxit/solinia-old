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



#ifndef NL_GROUP_TABLE_H
#define NL_GROUP_TABLE_H

#include "nel/misc/types_nl.h"
#include "group_frame.h"
#include "view_text.h"
#include "view_link.h"
#include "ctrl_button.h"

/**
  * This group is used to simulate HTML cells.
  * They have specific parameters to be aligned like HTML cells.
  * (Percent of the table size
  */
class CGroupCell: public CInterfaceGroup
{
	friend class CGroupTable;
public:
	CGroupCell(const TCtorParam &param);

	enum TAlign
	{
		Left,
		Center,
		Right
	};

	enum TVAlign
	{
		Top,
		Middle,
		Bottom
	};

	/// \from CInterfaceElement
	virtual void draw();
	virtual sint32 getMaxUsedW() const;
	virtual sint32 getMinUsedW() const;

	// to be called by CGroupTable
	bool parse (xmlNodePtr cur, CInterfaceGroup * parentGroup, uint columnIndex, uint rowIndex);

	// If the cell is a new line. This is the first <td> after a <tr>
	bool	NewLine;
	bool    IgnoreMaxWidth;
	bool    IgnoreMinWidth;
	bool	AddChildW;

	// The table width cell ratio. This is the <td width="50%"> parameter
	float	TableRatio;

	// The Width you want in pixel. This is the <td width="100"> parameter
	sint32	WidthWanted;


	// The min height of the cell
	sint32	Height;

	// Memorize max width
	sint32	WidthMax;

	// The cell color
	NLMISC::CRGBA	BgColor;

	// Alignment
	TAlign	Align;
	TVAlign	VAlign;
	sint32	LeftMargin;

	// The cell group
	CInterfaceGroup	*Group;

	// The cell is nowrap
	bool	NoWrap;
private:
	void setEnclosedGroupDefaultParams();
};

/**
  * This group is used to simulate HTML table. Support "percent of the parent width" sizeRef mode.
  */
class CGroupTable : public CInterfaceGroup
{
public:

	///constructor
	CGroupTable(const TCtorParam &param);

	// dtor
	~CGroupTable();

	// Add a cell in the table
	void addChild (CGroupCell* child);

	// The ratio you want [0 ~1]. This is the <table width="50%"> parameter
	float	TableRatio;

	// The Width you want in pixel. This is the <table width="100"> parameter
	sint32	ForceWidthMin;

	// Table borders
	sint32	Border;
	sint32	CellPadding;
	sint32	CellSpacing;

	// The table color
	NLMISC::CRGBA	BgColor;
	uint8	CurrentAlpha;

	bool    ContinuousUpdate;

protected:

	/// \from CInterfaceElement
	void onInvalidateContent();
	sint32	getMaxUsedW() const;
	sint32	getMinUsedW() const;
	void draw ();

	/**
	 * init  or reset the children element coords. Orverloaded from CInterfaceGroup because we begin with the last inserted element here
	 */
	virtual void updateCoords();

	virtual void checkCoords();

	virtual bool parse (xmlNodePtr cur, CInterfaceGroup * parentGroup);

	// Content validated
	bool	_ContentValidated;

	// Last parent width
	sint32		_LastParentW;

	// Children
	std::vector<CGroupCell *>	_Cells;

	// Table column
	class CColumn
	{
	public:
		CColumn()
		{
			Width = 0;
			WidthMax = 0;
			WidthWanted = 0;
			TableRatio = 0;
			Height = 0;
		}
		sint32	Width;
		sint32	Height;
		sint32	WidthWanted;
		sint32	WidthMax;
		float	TableRatio;
	};

	// Table row
	class CRow
	{
	public:
		CRow()
		{
			Height = 0;
		}
		sint32	Height;
	};

	// Column table
	std::vector<CColumn>	_Columns;

	// Column table
	std::vector<CRow>		_Rows;
};


#endif // NL_GROUP_TABLE_H

/* End of group_table.h */


