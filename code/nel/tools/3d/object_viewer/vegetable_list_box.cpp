// vegetable_list_box.cpp : implementation file
//

#include "std_afx.h"
#include "object_viewer.h"
#include "vegetable_list_box.h"
#include "resource.h"
#include "vegetable_dlg.h"


/////////////////////////////////////////////////////////////////////////////
// CVegetableListBox

CVegetableListBox::CVegetableListBox()
{
	VegetableDlg= NULL;
}

CVegetableListBox::~CVegetableListBox()
{
}


BEGIN_MESSAGE_MAP(CVegetableListBox, CListBox)
	//{{AFX_MSG_MAP(CVegetableListBox)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVegetableListBox message handlers

void CVegetableListBox::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// must be setuped
	nlassert(VegetableDlg);

	// popup menu
	CMenu  menu;
	CMenu* subMenu;
	menu.LoadMenu(IDR_VEGETABLE_MENU);

	// test wether there is an item under that point
	BOOL	bout;
	uint	item  = this->ItemFromPoint(point, bout);
	if (!bout)
	{
		// Change the selection, and force refresh display.
		this->SetCurSel(item);
		VegetableDlg->OnSelchangeListVegetable();

		// Init Show Item.
		if( VegetableDlg->isVegetableVisible(item) )
			menu.CheckMenuItem(ID_MENU_SHOW, MF_CHECKED | MF_BYCOMMAND );
		else
			menu.CheckMenuItem(ID_MENU_SHOW, MF_UNCHECKED | MF_BYCOMMAND );
	}
	else
	{
		// Item not selected, enable only Add and Load.
		menu.EnableMenuItem(ID_MENU_SHOW, MF_GRAYED);
		menu.EnableMenuItem(ID_MENU_SHOWONLY, MF_GRAYED);
		menu.EnableMenuItem(ID_MENU_INSERT, MF_GRAYED);
		menu.EnableMenuItem(ID_MENU_REMOVE, MF_GRAYED);
		menu.EnableMenuItem(ID_MENU_COPY, MF_GRAYED);
		menu.EnableMenuItem(ID_MENU_SAVEVEGETDESC, MF_GRAYED);
	}

	// launch
	subMenu = menu.GetSubMenu(0);
	nlassert(subMenu);
	RECT r;
	GetWindowRect(&r);
	subMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, r.left + point.x, r.top + point.y, this);

	
	// Base behavior
	CListBox::OnRButtonDown(nFlags, point);
}

BOOL CVegetableListBox::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	if (nCode != 0) return CListBox::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

	// must be setuped
	nlassert(VegetableDlg);
	uint	numVegets= VegetableDlg->getNumVegetables();
	// get the vegetable edited, if any
	sint	vegetId= GetCurSel();

	// track 
	switch(nID)
	{
	case	ID_MENU_SHOW:
		{
			if(vegetId!=LB_ERR)
			{
				VegetableDlg->swapShowHideVegetable(vegetId);
				Invalidate();
			}
			break;
		}
	case	ID_MENU_ADD:
		VegetableDlg->OnButtonVegetableAdd();
		break;
	case	ID_MENU_INSERT:
		VegetableDlg->OnButtonVegetableInsert();
		break;
	case	ID_MENU_REMOVE:
		VegetableDlg->OnButtonVegetableRemove();
		break;
	case	ID_MENU_COPY:
		VegetableDlg->OnButtonVegetableCopy();
		break;
	case	ID_MENU_LOADVEGETDESC:
		VegetableDlg->OnButtonVegetableLoadDesc();
		break;
	case	ID_MENU_SAVEVEGETDESC:
		VegetableDlg->OnButtonVegetableSaveDesc();
		break;
	case	ID_MENU_SHOWONLY:
		{
			// Hide all vegetables.
			for(uint i=0; i<numVegets; i++)
			{
				VegetableDlg->setShowHideVegetable(i, false, false);
			}

			// show selected
			if(vegetId!=LB_ERR)
				VegetableDlg->setShowHideVegetable(vegetId, true, false);

			// refresh display.
			Invalidate();
			VegetableDlg->refreshVegetableDisplay();
		}
		break;
	case	ID_MENU_SHOWALL:
		{
			// show all vegetables.
			for(uint i=0; i<numVegets; i++)
			{
				VegetableDlg->setShowHideVegetable(i, true, false);
			}

			// refresh display.
			Invalidate();
			VegetableDlg->refreshVegetableDisplay();
		}
		break;
	};
	
	// Base behavior
	return CListBox::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CVegetableListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	CListBox::MeasureItem(lpMeasureItemStruct);
}

void CVegetableListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	sint	itemID= lpDrawItemStruct->itemID;
	if( itemID!=LB_ERR )
	{
		nlassert(VegetableDlg);

		nlassert(lpDrawItemStruct->CtlType == ODT_LISTBOX);
		CDC dc;

		dc.Attach(lpDrawItemStruct->hDC);

		// Save these value to restore them when done drawing.
		COLORREF crOldTextColor = dc.GetTextColor();
		COLORREF crOldBkColor = dc.GetBkColor();

		// If this item is selected, set the background color 
		// and the text color to appropriate values. Also, erase
		// rect by filling it with the background color.
		if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
		  (lpDrawItemStruct->itemState & ODS_SELECTED))
		{
			dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
			dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
			dc.FillSolidRect(&lpDrawItemStruct->rcItem, 
			 ::GetSysColor(COLOR_HIGHLIGHT));
		}
		else
			dc.FillSolidRect(&lpDrawItemStruct->rcItem, crOldBkColor);

		// Draw the text.
		CString	str;
		GetText( itemID, str);
		if( !str.IsEmpty() )
		{
			// If not visible, change Text color.
			if( !VegetableDlg->isVegetableVisible(itemID) )
			{
				dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT) );
			}

			dc.DrawText(
				(const char*)str,
				str.GetLength(),
				&lpDrawItemStruct->rcItem,
				DT_LEFT|DT_SINGLELINE|DT_VCENTER);
		}

		// Reset the background color and the text color back to their
		// original values.
		dc.SetTextColor(crOldTextColor);
		dc.SetBkColor(crOldBkColor);

		dc.Detach();
	}
}
