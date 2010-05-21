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

#include "group_table.h"
#include "interface_manager.h"
#include "interface_element.h"
#include "../client_chat_manager.h"
#include "view_bitmap.h"
#include "view_text_id.h"
#include "group_container.h"

#include "nel/misc/i_xml.h"
#include "nel/misc/i18n.h"

#include "game_share/xml_auto_ptr.h"

using namespace std;
using namespace NLMISC;

extern CClientChatManager ChatMngr;
extern bool DebugUICell;

// ----------------------------------------------------------------------------
CGroupCell::CGroupCell(const TCtorParam &param)
: CInterfaceGroup(param),
	BgColor (0,0,0,0)

{
	NewLine = false;
	TableRatio = 0.f;
	WidthWanted = 0;
	Height = 0;
	Group = new CInterfaceGroup(CViewBase::TCtorParam());
	Align = Left;
	VAlign = Top;
	LeftMargin = 0;
	NoWrap = false;
	IgnoreMaxWidth = false;
	IgnoreMinWidth = false;
	AddChildW = false;
	setEnclosedGroupDefaultParams();
	addGroup (Group);
}

// ----------------------------------------------------------------------------
void CGroupCell::setEnclosedGroupDefaultParams()
{
	nlassert(Group);
	// Cells are moved and resized by the table
	Group->setParent(this);
	Group->setParentPos(this);
	Group->setPosRef(Hotspot_TL);
	Group->setParentPosRef(Hotspot_TL);
	Group->setResizeFromChildW(false);
	Group->setResizeFromChildH(true);
}

// ----------------------------------------------------------------------------
bool CGroupCell::parse(xmlNodePtr cur, CInterfaceGroup * parentGroup, uint columnIndex, uint rowIndex)
{
	CXMLAutoPtr ptr;
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"id");
	if (!ptr)
	{
		// no id was given, so create one
		std::string newId = NLMISC::toString("cell_%d_%d", (int) columnIndex, (int) rowIndex);
		xmlSetProp(cur, (const xmlChar*) "id", (const xmlChar*) newId.c_str());
	}
	// parse enclosed group
	if (!Group->parse(cur, this)) return false;
	Group->setId(parentGroup->getId() + Group->getId());
	setEnclosedGroupDefaultParams();
	// parse the children
	bool ok = CInterfaceManager::getInstance()->parseGroupChildren(cur, Group, false);
	if (!ok) return false;
	// align
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"align" );
	if (ptr)
	{
		if (strcmp((const char *) ptr, "left") == 0)
		{
			Align = Left;
		}
		else if (strcmp((const char *) ptr, "right") == 0)
		{
			Align = Right;
		}
		else if (strcmp((const char *) ptr, "center") == 0)
		{
			Align = Center;
		}
		else
		{
			nlwarning("<CGroupCell::parse> Unknwown or unsupported align mode : %s", (const char *) ptr);
		}
	}
	// v-align
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"valign" );
	if (ptr)
	{
		if (strcmp((const char *) ptr, "top") == 0)
		{
			VAlign = Top;
		}
		else if (strcmp((const char *) ptr, "middle") == 0)
		{
			VAlign = Middle;
		}
		else if (strcmp((const char *) ptr, "bottom") == 0)
		{
			VAlign = Bottom;
		}
		else
		{
			nlwarning("<CGroupCell::parse> Unknwown or unsupported valign mode : %s", (const char *) ptr);
		}
	}
	// left margin
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"left_margin" );
	if (ptr)
	{
		fromString((const char*)ptr, LeftMargin);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"nowrap" );
	if (ptr)
	{
		NoWrap = convertBool(ptr);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"bgcolor" );
	if (ptr)
	{
		BgColor = convertColor(ptr);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"width" );
	if (ptr)
	{
		convertPixelsOrRatio((const char *) ptr, WidthWanted, TableRatio);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"height" );
	if (ptr)
	{
		fromString((const char*)ptr, Height);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"ignore_max_width" );
	if (ptr)
	{
		IgnoreMaxWidth = convertBool(ptr);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"ignore_min_width" );
	if (ptr)
	{
		IgnoreMinWidth = convertBool(ptr);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"add_child_w" );
	if (ptr)
	{
		AddChildW = convertBool(ptr);
	}

	return true;
}


// ----------------------------------------------------------------------------
void CGroupCell::draw ()
{
	CInterfaceManager *pIM = CInterfaceManager::getInstance();
	if (DebugUICell)
	{
		// Draw cell
		CViewRenderer &rVR = pIM->getViewRenderer();
		rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal, _WReal, 1, 0, false, rVR.getBlankTextureId(), CRGBA(0,255,255,255) );
		rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal, 1, _HReal, 0, false, rVR.getBlankTextureId(), CRGBA(0,255,255,255) );
		rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal+_HReal-1, _WReal, 1, 0, false, rVR.getBlankTextureId(), CRGBA(0,255,255,255) );
		rVR.drawRotFlipBitmap (_RenderLayer, _XReal+_WReal-1, _YReal, 1, _HReal, 0, false, rVR.getBlankTextureId(), CRGBA(0,255,255,255) );
	}

	// Draw the background
	if (BgColor.A != 0)
	{
		CViewRenderer &rVR = pIM->getViewRenderer();
		CRGBA finalColor;
		finalColor.modulateFromColor (BgColor, pIM->getGlobalColor());

		// Get the parent table
		if (getParent ())
		{
			CGroupTable *table = static_cast<CGroupTable*> (getParent ());
			finalColor.A = (uint8) (((uint16) table->CurrentAlpha * (uint16) finalColor.A) >> 8);
		}

		rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal, _WReal, _HReal, 0, false, rVR.getBlankTextureId(), finalColor);
	}

	CInterfaceGroup::draw ();
}

// ----------------------------------------------------------------------------
sint32 CGroupCell::getMaxUsedW() const
{
	return Group->getMaxUsedW();
}

// ------------------------------------------------------------------------------------------------
sint32 CGroupCell::getMinUsedW() const
{
	return Group->getMinUsedW();
}

// ----------------------------------------------------------------------------
NLMISC_REGISTER_OBJECT(CViewBase, CGroupTable, std::string, "table");

CGroupTable::CGroupTable(const TCtorParam &param)
:	CInterfaceGroup(param),
	BgColor(0,0,0,255)
{
	_ContentValidated = false;
	TableRatio = 0.f;
	ForceWidthMin = 0;
	Border=0;
	CellPadding=0;
	CellSpacing=0;
	ContinuousUpdate = false;
}

// ----------------------------------------------------------------------------
void CGroupTable::addChild (CGroupCell* child)
{
	// Cell empty ?
	if (_Cells.empty())
		// New line
		child->NewLine = true;

	// Cells are moved and resized by the table
	child->setParent(this);
	child->setParentPos(this);
	child->setPosRef(Hotspot_TL);
	child->setParentPosRef(Hotspot_TL);
	child->setResizeFromChildW(false);

	// Child resize H for cells
	child->setResizeFromChildH(false);

/*	// New line ?
	if (child->NewLine)
	{
		// First element
		if (_Cells.empty())
		{
			child->setParentPos(NULL);
			child->setPosRef(Hotspot_TL);
			child->setParentPosRef(Hotspot_TL);
		}
		// New line
		else
		{
			// Look for previous new line
			uint i;
			for (i=_Cells.size()-1; i>0; i--)
				if (_Cells[i]->NewLine)
					break;

			child->setParentPos(_Cells[i]);
			child->setPosRef(Hotspot_TL);
			child->setParentPosRef(Hotspot_BL);
		}
	}
	else
	{
		nlassert(!_Cells.empty());
		child->setParentPos(_Cells.back());
		child->setPosRef(Hotspot_TL);
		child->setParentPosRef(Hotspot_TR);
	}*/

	// Add the child
	addGroup(child);
	_Cells.push_back(child);
	invalidateContent();
}

// ----------------------------------------------------------------------------
CGroupTable::~CGroupTable()
{
/*	uint i;
	for (i=0; i<_Cells.size(); i++)
		delete _Cells[i];
	_Cells.clear ();*/
}



// ----------------------------------------------------------------------------
void CGroupTable::updateCoords()
{
	if (_Parent)
	{
		if (ContinuousUpdate)
		{
			sint parentWidth = std::min(_Parent->getMaxWReal(), _Parent->getWReal());
			if (_LastParentW != (sint) parentWidth)
			{
				_LastParentW = parentWidth;
				_ContentValidated = false;
			}
		}
		if (!_ContentValidated)
		{
			// Update W and XReal
			CInterfaceElement::updateCoords();

			// *** For each children, update coords, gets min and max values
			// *** Get the column sizes, we need to know min for the table

			uint i;
			uint column = 0;
			_Columns.clear ();
			for (i=0; i<_Cells.size(); i++)
			{
				// Update coords to get XReal
				CGroupCell *cell = _Cells[i];
				cell->checkCoords();
				cell->updateCoords();

				sint32 additionnalWidth = 0;
				if (cell->AddChildW)
				{
					sint width, height;
					cell->Group->evalChildrenBBox(true, false, width, height);
					additionnalWidth = (sint32) width;
				}

				// Get width min and max
				if( !cell->IgnoreMaxWidth)
				{
					cell->WidthMax = cell->getMaxUsedW() + cell->LeftMargin;
				}
				else
				{
					cell->WidthMax = cell->WidthWanted + additionnalWidth + cell->LeftMargin;
				}
				sint32 cellWidth;
				if(!cell->IgnoreMinWidth)
				{
					cellWidth = cell->NoWrap ? cell->WidthMax : cell->getMinUsedW() + cell->LeftMargin;
				}
				else
				{
					cellWidth = cell->NoWrap ? cell->WidthMax : cell->LeftMargin;
				}

				// New cell ?
				if (cell->NewLine)
					column = 0;

				// Resize the array
				if (column>=_Columns.size())
					_Columns.resize(column+1);

				// Update sizes
				if (cellWidth > _Columns[column].Width)
					_Columns[column].Width = cellWidth;
				if (cell->WidthMax > _Columns[column].WidthMax)
					_Columns[column].WidthMax = cell->WidthMax;
				if (cell->TableRatio > _Columns[column].TableRatio)
					_Columns[column].TableRatio = cell->TableRatio;
				if (cell->WidthWanted + additionnalWidth > _Columns[column].WidthWanted)
					_Columns[column].WidthWanted = cell->WidthWanted + additionnalWidth;
				if (cell->Height > _Columns[column].Height)
					_Columns[column].Height = cell->Height;

				if (_Columns[column].WidthWanted + additionnalWidth)
					_Columns[column].WidthMax = _Columns[column].WidthWanted + additionnalWidth;
				if (_Columns[column].WidthWanted > _Columns[column].Width)
					_Columns[column].Width = _Columns[column].WidthWanted;

				// Next column
				column++;
			}

			// Width of cells and table borders
			sint32 borderWidth = 2*Border + ((sint32)_Columns.size()+1) * CellSpacing + ((sint32)_Columns.size()*2) * CellPadding;

			// Get the width
			sint32 tableWidthMax = ForceWidthMin?ForceWidthMin:_LastParentW; // getWReal();
			sint32 tableWidthMin = std::max(ForceWidthMin, (sint32)((float)tableWidthMax*TableRatio));
			tableWidthMax = std::max ((sint32)0, tableWidthMax-borderWidth);
			tableWidthMin = std::max ((sint32)0, tableWidthMin-borderWidth);

			// Get the width of the table and normalize percent of the cell (sum of TableRatio must == 1)
			sint32 tableWidth = 0;
			sint32 tableMaxContentWidth = 0;
			float ratio = 1;
			for (i=0; i<_Columns.size(); i++)
			{
				tableWidth += _Columns[i].Width;
				tableMaxContentWidth += _Columns[i].WidthMax;
				_Columns[i].TableRatio = std::min(_Columns[i].TableRatio, ratio);
				ratio -= _Columns[i].TableRatio;
			}

			// Eval table size with all percent cells resized
			sint32 tableWidthSizeAfterPercent = tableWidth;
			for (i=0; i<_Columns.size(); i++)
			{
				if (_Columns[i].TableRatio > 0)
				{
					// Size of the cell with its percent
					sint32 me = (sint32)((float)_Columns[i].Width / _Columns[i].TableRatio);

					tableWidthSizeAfterPercent = std::min(tableWidthMax, std::max(tableWidthSizeAfterPercent, me));
				}
			}
			for (i=0; i<_Columns.size(); i++)
			{
				if (_Columns[i].TableRatio > 0)
				{
					// Size of the other cells
					sint32 cellSize;
					if (_Columns[i].TableRatio < 1.f)
						cellSize = (sint32)((float)tableWidthSizeAfterPercent*_Columns[i].TableRatio) + 1;
					else
						cellSize = 10000;

					sint32 diff = std::min(tableWidthMax, (std::max(_Columns[i].Width, cellSize) - _Columns[i].Width)+tableWidth) - tableWidth;
					tableWidth += diff;
					_Columns[i].Width += diff;
					// tableMaxContentWidth = std::max(tableMaxContentWidth, (sint32)((float)_Columns[i].WidthMax / _Columns[i].TableRatio));
				}
			}
			sint32 space = tableWidthMax - tableWidth;

			// Normalize percent window
			for (i=0; i<_Columns.size(); i++)
			{
				// Width for the cell
				sint32 diff = (sint32)((float)tableWidth*_Columns[i].TableRatio) - _Columns[i].Width;
				diff = std::min(diff, space);
				if (diff > 0)
				{
					_Columns[i].Width += diff;
					tableWidth += diff;
					space -= diff;
				}
			}

			// Ok, now percent window are nomralized
			// Evaluate space to put in windows
			sint32 finalWidth = std::max(tableWidthMin, std::min(std::max(tableWidth, tableMaxContentWidth), tableWidthMax));
			space = finalWidth - tableWidth;
			if (space > 0)
			{
				// First give to percent cells
				for (i=0; i<_Columns.size(); i++)
				{
					// Width for the cell
					sint32 dif = (sint32)((float)space*_Columns[i].TableRatio);
					_Columns[i].Width += dif;
					tableWidth += dif;
				}

				// Some space ?
				space = finalWidth - tableWidth;

				if (space > 0)
				{
					// Then add in wanted Width cells
					for (i=0; i<_Columns.size(); i++)
					{
						// Width for the cell
						if (_Columns[i].Width < _Columns[i].WidthWanted)
						{
							sint32 dif = std::min(space, _Columns[i].WidthWanted-_Columns[i].Width);
							_Columns[i].Width += dif;
							space -= dif;
						}
					}

					if (space > 0)
					{
						// All cells with sizewanted are full
						// Distribute remaining space in resizable cells that have a WidthMax
						sint32 sumDeltaWidth = 0;
						for (i=0; i<_Columns.size(); i++)
						{
							if ((_Columns[i].TableRatio == 0) && (_Columns[i].WidthWanted == 0))
								sumDeltaWidth += std::max ((sint32)0, _Columns[i].WidthMax - _Columns[i].Width);
						}
						if (sumDeltaWidth)
						{
							sint32 toDistribute = space;
							for (i=0; i<_Columns.size(); i++)
							{
								if ((_Columns[i].TableRatio == 0) && (_Columns[i].WidthWanted == 0))
								{
									sint32 marge = std::max ((sint32)0, _Columns[i].WidthMax - _Columns[i].Width);
									sint32 cellPart = std::min(marge, std::min(space, 1 + marge * toDistribute / sumDeltaWidth));
									if (cellPart)
									{
										_Columns[i].Width += cellPart;
										space -= cellPart;
									}
								}
							}
						}

						if (space > 0)
						{
							// All cells with sizemax are full
							// Distribute remaining space in others resizable cells
							sumDeltaWidth = 0;
							for (i=0; i<_Columns.size(); i++)
							{
								if ((_Columns[i].TableRatio == 0) && (_Columns[i].WidthWanted == 0))
									sumDeltaWidth++;
							}
							if (sumDeltaWidth)
							{
								sint32 toDistribute = space;
								for (i=0; i<_Columns.size(); i++)
								{
									if ((_Columns[i].TableRatio == 0) && (_Columns[i].WidthWanted == 0))
									{
										sint32 cellPart = std::min(space, 1 + toDistribute / sumDeltaWidth);
										_Columns[i].Width += cellPart;
										space -= cellPart;
									}
								}
							}

							if (space > 0)
							{
								// No cells with sizemax not sizewanted, resize size wanted
								// Distribute remaining space in others resizable cells
								sumDeltaWidth = 0;
								for (i=0; i<_Columns.size(); i++)
								{
									if ((_Columns[i].TableRatio == 0) && (_Columns[i].WidthWanted != 0))
										sumDeltaWidth++;
								}
								if (sumDeltaWidth)
								{
									sint32 toDistribute = space;
									for (i=0; i<_Columns.size(); i++)
									{
										if ((_Columns[i].TableRatio == 0) && (_Columns[i].WidthWanted != 0))
										{
											sint32 cellPart = std::min(space, 1 + toDistribute / sumDeltaWidth);
											_Columns[i].Width += cellPart;
											space -= cellPart;
										}
									}
								}
							}
						}
					}
				}
			}

			// *** Now we know each column width, resize cells and get the height for each row

			column = 0;
			sint32 row = 0;
			sint32 currentX = Border + CellSpacing + CellPadding;
			_Rows.clear ();
			for (i=0; i<_Cells.size(); i++)
			{
				CGroupCell *cell = _Cells[i];
				// New cell ?
				if (cell->NewLine)
				{
					column = 0;
					currentX = Border + CellSpacing + CellPadding;
					_Rows.push_back(CRow());
				}

				// Set the x and width

				// Check align
				sint32 alignmentX = 0;
				sint32 widthReduceX = 0;
				if (cell->WidthMax < _Columns[column].Width)
				{
					switch (cell->Align)
					{
					case CGroupCell::Center:
						alignmentX = (_Columns[column].Width - cell->WidthMax) / 2;
						widthReduceX = alignmentX * 2;
						break;
					case CGroupCell::Right:
						alignmentX = _Columns[column].Width - cell->WidthMax;
						widthReduceX = alignmentX;
						break;
					}
				}

				cell->setX(currentX);
				cell->setW(_Columns[column].Width);

				cell->Group->setX(alignmentX+cell->LeftMargin);
				cell->Group->setW(_Columns[column].Width - widthReduceX);
				cell->Group->CInterfaceElement::updateCoords();

				// Update coords to get H
				cell->Group->checkCoords();
				cell->Group->updateCoords();

				// Resize the row array
				_Rows.back().Height = std::max(cell->Height, std::max(_Rows.back().Height, (sint32)cell->Group->getH()));

				// Next column
				currentX += _Columns[column].Width + 2*CellPadding + CellSpacing;
				column ++;
			}

			// Set cell Y
			row = 0;
			sint32 currentY = -(Border + CellSpacing + CellPadding);
			for (i=0; i<_Cells.size(); i++)
			{
				// New cell ?
				CGroupCell *cell = _Cells[i];
				if ((i != 0) && (cell->NewLine))
				{
					if (_Rows[row].Height != 0)
					{
						currentY -= _Rows[row].Height + 2*CellPadding + CellSpacing;
					}
					row++;
				}

				// Check align
				sint32 alignmentY = 0;
				if ((sint32)cell->Group->getH() < _Rows[row].Height)
				{
					switch (cell->VAlign)
					{
					case CGroupCell::Middle:
						alignmentY = (_Rows[row].Height - (sint32)cell->Group->getH()) / 2;
						break;
					case CGroupCell::Bottom:
						alignmentY = _Rows[row].Height - (sint32)cell->Group->getH();
						break;
					}
				}

				cell->setY(currentY);
				cell->setH (_Rows[row].Height);
				cell->Group->setY(-alignmentY);
			}

			// Resize the table
			setW(finalWidth+borderWidth-_LastParentW);
			if (!_Rows.empty())
				currentY -= _Rows[row].Height + CellPadding + CellSpacing + Border;
			setH(-currentY);

			// All done
		}
	}


	CInterfaceGroup::updateCoords();




	// Validated
	_ContentValidated = true;
}

// ----------------------------------------------------------------------------
void CGroupTable::checkCoords ()
{
	if (_Parent != NULL)
	{
		sint parentWidth = std::min(_Parent->getMaxWReal(), _Parent->getWReal());
		if (_LastParentW != (sint) parentWidth)
		{
			if (ContinuousUpdate)
			{
				_LastParentW = parentWidth;
				invalidateContent();
			}
			else
			{
				CInterfaceManager *pIM = CInterfaceManager::getInstance();
				CCtrlBase *pCB = pIM->getCapturePointerLeft();
				if (pCB != NULL)
				{
					CCtrlResizer *pCR = dynamic_cast<CCtrlResizer*>(pCB);
					if (pCR != NULL)
					{
						// We are resizing !!!!
					}
					else
					{
						_LastParentW = parentWidth;
						 invalidateContent();
					}
				}
				else
				{
					_LastParentW = parentWidth;
					invalidateContent();
				}
			}
		}
	}
	CInterfaceGroup::checkCoords();
}

// ----------------------------------------------------------------------------
void CGroupTable::onInvalidateContent()
{
	_ContentValidated = false;
	invalidateCoords();
}

// ----------------------------------------------------------------------------
sint32	CGroupTable::getMaxUsedW() const
{
	uint i;
	uint column = 0;
	vector<sint32> columns;
	columns.clear ();
	for (i=0; i<_Cells.size(); i++)
	{
		// Update coords to get XReal
		CGroupCell *cell = _Cells[i];
		cell->checkCoords();
		cell->updateCoords();

		// Get width min and max
		sint32 cellWidthMax = cell->getMaxUsedW();

		// New cell ?
		if (cell->NewLine)
			column = 0;

		// Resize the array
		if (column>=columns.size())
			columns.resize(column+1, 0);

		// Update sizes
		if (cellWidthMax > columns[column])
			columns[column] = cellWidthMax;
		if (cell->WidthWanted)
			columns[column] = cell->WidthWanted;

		// Hack to force a table with ratio to be large
		sint32 cellRatio = (sint32)ceil(cell->TableRatio * 1024.f);
		if (cellRatio > columns[column])
			columns[column] = cellRatio;

		// Next column
		column++;
	}

	// Sum
	sint32 maxWidth = 0;
	for (i=0; i<columns.size(); i++)
		maxWidth += columns[i];

	maxWidth += 2*Border + ((sint32)columns.size()+1) * CellSpacing + ((sint32)columns.size()*2) * CellPadding;

	return maxWidth;
}

// ----------------------------------------------------------------------------
sint32	CGroupTable::getMinUsedW() const
{
	uint i;
	uint column = 0;
	vector<sint32> columns;
	columns.clear ();
	for (i=0; i<_Cells.size(); i++)
	{
		// Update coords to get XReal
		CGroupCell *cell = _Cells[i];
		cell->checkCoords();
		cell->updateCoords();

		// Get width min and max
		sint32 cellWidthMin = cell->getMinUsedW();

		// New cell ?
		if (cell->NewLine)
			column = 0;

		// Resize the array
		if (column>=columns.size())
			columns.resize(column+1, 0);

		// Update sizes
		if (cellWidthMin > columns[column])
			columns[column] = cellWidthMin;
		if (cell->WidthWanted)
			columns[column] = cell->WidthWanted;

		// Next column
		column++;
	}

	// Sum
	sint32 maxWidth = 0;
	for (i=0; i<columns.size(); i++)
		maxWidth += columns[i];

	maxWidth += 2*Border + ((sint32)columns.size()+1) * CellSpacing + ((sint32)columns.size()*2) * CellPadding;

	return maxWidth;
}

// ----------------------------------------------------------------------------
void CGroupTable::draw ()
{
	// search a parent container
	CInterfaceGroup *gr = getParent();
	while (gr)
	{
		if (gr->isGroupContainer())
		{
			CGroupContainer *gc = static_cast<CGroupContainer *>(gr);
			CurrentAlpha = gc->getCurrentContainerAlpha();
			break;
		}
		gr = gr->getParent();
	}

	// Not found ?
	if (gr == NULL)
		CurrentAlpha = 255;

	if (!_Columns.empty() && !_Rows.empty() && BgColor.A)
	{
		sint32 border = Border + CellSpacing + CellPadding;
		if (border)
		{
			CInterfaceManager *pIM = CInterfaceManager::getInstance();
			CRGBA finalColor;
			finalColor.modulateFromColor (BgColor, pIM->getGlobalColor());
			finalColor.A = CurrentAlpha;

			// Draw the top and bottom lines
			CViewRenderer &rVR = pIM->getViewRenderer();
			rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal, _WReal, border, 0, false, rVR.getBlankTextureId(), finalColor);
			rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal-border+_HReal, _WReal, border, 0, false, rVR.getBlankTextureId(), finalColor);

			// Draw the left and right lines
			sint32 insideHeight = std::max((sint32)0, (sint32)_HReal - (sint32)2*border);
			rVR.drawRotFlipBitmap (_RenderLayer, _XReal, _YReal+border, border, insideHeight, 0, false, rVR.getBlankTextureId(), finalColor);
			rVR.drawRotFlipBitmap (_RenderLayer, _XReal+_WReal-border, _YReal+border, border, insideHeight, 0, false, rVR.getBlankTextureId(), finalColor);

			// Draw the inside borders
			sint32 insideWidth = 2*CellPadding + CellSpacing;
			if (insideWidth)
			{
				// Draw the inside verticals
				uint i;
				sint32 x = _XReal + _Columns[0].Width + border;
				for (i=1; i<_Columns.size(); i++)
				{
					rVR.drawRotFlipBitmap (_RenderLayer, x, _YReal+border, insideWidth, insideHeight, 0, false, rVR.getBlankTextureId(), finalColor);
					x += _Columns[i].Width + insideWidth;
				}

				// Draw the inside horizontals
				sint32 y = _YReal + _HReal - border - _Rows[0].Height;
				if (_Rows[0].Height != 0)
				{
					y -= insideWidth;
				}
				for (i=1; i<_Rows.size(); i++)
				{
					uint j;
					x = _XReal + border;
					if (_Rows[i].Height != 0)
					{
						for (j=0; j<_Columns.size(); j++)
						{
							rVR.drawRotFlipBitmap (_RenderLayer, x, y, _Columns[j].Width, insideWidth, 0, false, rVR.getBlankTextureId(), finalColor);
							x += _Columns[j].Width + insideWidth;
						}
						y -= _Rows[i].Height+ insideWidth;
					}
				}
			}
		}
	}

	CInterfaceGroup::draw ();
}

// ------------------------------------------------------------------------------------------------
bool CGroupTable::parse (xmlNodePtr cur, CInterfaceGroup * parentGroup)
{
	if (!CInterfaceGroup::parse(cur, parentGroup)) return false;
	//
	CXMLAutoPtr ptr;
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"border" );
	if (ptr)
	{
		fromString((const char*)ptr, Border);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"cellpadding" );
	if (ptr)
	{
		fromString((const char*)ptr, CellPadding);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"cellspacing" );
	if (ptr)
	{
		fromString((const char*)ptr, CellSpacing);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"bgcolor" );
	if (ptr)
	{
		BgColor = convertColor((const char *) ptr);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"width" );
	if (ptr)
	{
		convertPixelsOrRatio(ptr, ForceWidthMin, TableRatio);
	}
	//
	ptr = (char*) xmlGetProp( cur, (xmlChar*)"continuous_update" );
	if (ptr)
	{
		ContinuousUpdate = convertBool(ptr);
	}
	// parse cells
	uint row = 0;
	xmlNodePtr currRow = cur->children;
	while (currRow != NULL)
	{
		// look for 'TR' markup
		if (strcmp((char*)currRow->name,"TR") == 0)
		{
			// found a row, parse cells inside
			xmlNodePtr currCol = currRow->children;
			bool newLine = true;
			uint column = 0;
			while (currCol != NULL)
			{
				// look for 'TR' markup
				if (strcmp((char*)currCol->name,"TD") == 0)
				{
					CGroupCell *cell = new CGroupCell(CViewBase::TCtorParam());
					if (cell->parse(currCol, this, column, row))
					{
						cell->NewLine = newLine;
						newLine = false;
						addChild(cell);
					}
					else
					{
						delete cell;
					}
					++ column;
				}
				currCol = currCol->next;
			}
			++ row;
		}
		currRow = currRow->next;
	}

	return true;
}

