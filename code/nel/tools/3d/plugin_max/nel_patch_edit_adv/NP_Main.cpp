#include "stdafx.h"
#include "editpat.h"

// ------------------------------------------------------------------------------------------------------------------------------------------------------

#define DBGWELD_DUMPx
#define DBGWELD_ACTIONx
#define DBG_NAMEDSELSx

#define PROMPT_TIME	2000

BOOL CALLBACK PatchSelectDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PatchOpsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PatchObjSurfDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PatchSurfDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PatchTileDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PatchEdgeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void CancelEditPatchModes(IObjParam *ip);

// ------------------------------------------------------------------------------------------------------------------------------------------------------

// This function checks the current command mode and resets it to CID_OBJMOVE if
// it's one of our command modes
void CancelEditPatchModes(IObjParam *ip) 
{
	switch (ip->GetCommandMode()->ID())
	{
	case CID_STDPICK:
		ip->SetStdCommandMode(CID_OBJMOVE);
		break;
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

// This gets rid of two-step modes, like booleans.  This is necessary because
// the first step, which activates the mode button, validates the selection set.
// If the selection set changes, the mode must be turned off because the new
// selection set may not be valid for the mode.
void Cancel2StepPatchModes(IObjParam *ip) 
{
	//	switch(ip->GetCommandMode()->ID()) {
	//		case CID_BOOLEAN:
	//			ip->SetStdCommandMode( CID_OBJMOVE );
	//			break;
	//		}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------
// IPatchSelect and IPatchOps interfaces   (JBW 2/2/99)
void* EditPatchMod::GetInterface(ULONG id) 
{
	switch (id)
	{
		case I_PATCHSELECT: 
			return (IPatchSelect*)this;
		case I_PATCHSELECTDATA: 
			return (IPatchSelectData*)this;
		case I_PATCHOPS: 
			return (IPatchOps*)this;
		case I_SUBMTLAPI: 
			return (ISubMtlAPI*)this;
	}
	return Modifier::GetInterface(id);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

void EditPatchMod::StartCommandMode(patchCommandMode mode)
{
	switch (mode)
	{
		case PcmAttach:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_ATTACH, 0);
			break;
		case PcmExtrude:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_EP_EXTRUDE, 0);
			break;
		case PcmBevel:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_EP_BEVEL, 0);
			break;
		case PcmBind:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_BIND, 0);
			break;
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

void EditPatchMod::ButtonOp(patchButtonOp opcode)
{
	switch (opcode)
	{
		case PopUnbind:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_UNBIND, 0);
			break;
		case PopHide:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_HIDE, 0);
			break;
		case PopUnhideAll:
			if (hOpsPanel != NULL)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_UNHIDE, 0);
			break;
		case PopWeld:
			if (hOpsPanel != NULL && GetSubobjectLevel() == PO_VERTEX)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_WELD, 0);
			break;
		case PopDelete:
			if (hOpsPanel != NULL && GetSubobjectLevel() >= PO_VERTEX)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_PATCH_DELETE, 0);
			break;
		case PopSubdivide:
			if (hOpsPanel != NULL && GetSubobjectLevel() >= PO_EDGE && GetSubobjectLevel() != PO_TILE)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_SUBDIVIDE, 0);
			break;
		case PopAddTri:
			if (hOpsPanel != NULL && GetSubobjectLevel() == PO_EDGE)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_ADDTRI, 0);
			break;
		case PopAddQuad:
			if (hOpsPanel != NULL && GetSubobjectLevel() == PO_EDGE)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_ADDQUAD, 0);
			break;
		case PopDetach:
			if (hOpsPanel != NULL && GetSubobjectLevel() == PO_PATCH)
				PostMessage(hOpsPanel, WM_COMMAND, IDC_DETACH, 0);
			break;
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

void EditPatchMod::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	this->ip = ip;
	
	patchUIValid = FALSE;
	CreatePatchDataTempData();

	hSelectPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EDPATCH_SELECT),
		PatchSelectDlgProc, GetString(IDS_TH_SELECTION), (LPARAM)this, rsSel ? 0 : APPENDROLL_CLOSED);
	hOpsPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EDPATCH_OPS),
		PatchOpsDlgProc, GetString(IDS_TH_GEOMETRY), (LPARAM) this, rsOps ? 0 : APPENDROLL_CLOSED);
		/* watje 3 - 18 - 99
		if (selLevel == EP_OBJECT)
		{
		hSurfPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EDPATCH_SURF_OBJ),
		PatchObjSurfDlgProc, GetString(IDS_TH_SURFACEPROPERTIES), (LPARAM) this, rsSurf ? 0 : APPENDROLL_CLOSED);
		}
		else
	*/
	if (selLevel == EP_PATCH)
	{
		hSurfPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EDPATCH_SURF),
			PatchSurfDlgProc, GetString(IDS_TH_SURFACEPROPERTIES), (LPARAM) this, rsSurf ? 0 : APPENDROLL_CLOSED);
	}
	else
		hSurfPanel = NULL;

	if (selLevel == EP_TILE)
	{
		hTilePanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EDPATCH_TILE),
			PatchTileDlgProc, "Tile Properties", (LPARAM) this, rsTile ? 0 : APPENDROLL_CLOSED);
	}
	else
		hTilePanel = NULL;
	
	if (selLevel == EP_EDGE)
	{
		hEdgePanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_EDPATCH_EDGE),
			PatchEdgeDlgProc, "Edge Properties", (LPARAM) this, rsEdge ? 0 : APPENDROLL_CLOSED);
	}
	else
		hEdgePanel = NULL;
	
	// Create sub object editing modes.
	moveMode        = new MoveModBoxCMode(this, ip);
	rotMode         = new RotateModBoxCMode(this, ip);
	uscaleMode      = new UScaleModBoxCMode(this, ip);
	nuscaleMode     = new NUScaleModBoxCMode(this, ip);
	squashMode      = new SquashModBoxCMode(this, ip);
	selectMode      = new SelectModBoxCMode(this, ip);
	extrudeMode		= new EPM_ExtrudeCMode(this, ip);
	bevelMode		= new EPM_BevelCMode(this, ip);
	bindMode		= new EPM_BindCMode(this, ip);
	
	
	// Add our sub object type
	TSTR type1(GetString(IDS_TH_VERTEX));
	TSTR type2(GetString(IDS_TH_EDGE));
	TSTR type3(GetString(IDS_TH_PATCH));
	TSTR type4("Tile");
	const TCHAR *ptype[] = { type1, type2, type3 , type4 };
	ip->RegisterSubObjectTypes(ptype, 4);
	
	// Restore the selection level.
	ip->SetSubObjectLevel(selLevel);
	
	// Disable show end result.
	ip->EnableShowEndResult(FALSE);
	
	// Setup named selection sets	
	SetupNamedSelDropDown();
	
	// Update selection UI display
	SelectionChanged();
	
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);	
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

void EditPatchMod::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (hSelectPanel)
	{
		rsSel = IsRollupPanelOpen(hSelectPanel);
		ip->DeleteRollupPage(hSelectPanel);
		hSelectPanel = NULL;
	}
	if (hOpsPanel)
	{
		rsOps = IsRollupPanelOpen(hOpsPanel);
		ip->DeleteRollupPage(hOpsPanel);
		hOpsPanel = NULL;
	}
	if (hSurfPanel)
	{
		rsSurf = IsRollupPanelOpen(hSurfPanel);
		ip->DeleteRollupPage(hSurfPanel);
		hSurfPanel = NULL;
	}
	if (hTilePanel)
	{
		rsTile = IsRollupPanelOpen(hTilePanel);
		ip->DeleteRollupPage(hTilePanel);
		hTilePanel = NULL;
	}
	if (hEdgePanel)
	{
		rsEdge = IsRollupPanelOpen(hEdgePanel);
		ip->DeleteRollupPage(hEdgePanel);
		hEdgePanel = NULL;
	}
	
	// Enable show end result
	ip->EnableShowEndResult(TRUE);
	
	CancelEditPatchModes(ip);

	if (ip->GetCommandMode()->ID()==CID_EP_PAINT)
		ip->SetStdCommandMode(CID_OBJMOVE);
	
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
	ClearAFlag(A_MOD_BEING_EDITED);
	
	DeletePatchDataTempData();
	this->ip = NULL;
	
	//	if ( ip->GetCommandMode()->ID() == CID_EP_EXTRUDE ) ip->SetStdCommandMode( CID_OBJMOVE );
	//	if ( ip->GetCommandMode()->ID() == CID_EP_BEVEL ) ip->SetStdCommandMode( CID_OBJMOVE );
	
	ip->DeleteMode(moveMode);
	ip->DeleteMode(rotMode);
	ip->DeleteMode(uscaleMode);
	ip->DeleteMode(nuscaleMode);
	ip->DeleteMode(squashMode);
	ip->DeleteMode(selectMode);
	ip->DeleteMode(extrudeMode);
	ip->DeleteMode(bevelMode);
	ip->DeleteMode(bindMode);
	
	if (moveMode)
		delete moveMode;
	moveMode = NULL;
	if (rotMode)
		delete rotMode;
	rotMode = NULL;
	if (uscaleMode)
		delete uscaleMode;
	uscaleMode = NULL;
	if (nuscaleMode)
		delete nuscaleMode;
	nuscaleMode = NULL;
	if (squashMode)
		delete squashMode;
	squashMode = NULL;
	if (selectMode)
		delete selectMode;
	selectMode = NULL;
	
	if (extrudeMode)
		delete extrudeMode;
	extrudeMode = NULL;
	
	if (bevelMode)
		delete bevelMode;
	bevelMode = NULL;
	if (bindMode)
		delete bindMode;
	bindMode = NULL;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

EditPatchMod::EditPatchMod()
{
	selLevel = EP_OBJECT;
	displayLattice = TRUE;
	displaySurface = TRUE;
	propagate = TRUE;
	meshSteps = 5;
	transitionType = 1;
	channelModified = EDITPAT_CHANNELS;
	// 3-18-99 to suport render steps and removal of the mental tesselator
	meshStepsRender = 5;
	showInterior = TRUE;
	
	namedSelNeedsFixup = FALSE;
	includeMeshes=false;
	//	meshAdaptive = FALSE;	// Future use (Not used now)
}

EditPatchMod::~EditPatchMod()
{
	ClearSetNames();
}
