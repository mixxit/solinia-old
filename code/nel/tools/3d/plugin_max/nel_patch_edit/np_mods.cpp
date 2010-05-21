/**********************************************************************
 *<
	FILE: mods.cpp

	DESCRIPTION:   DLL implementation of modifiers

	CREATED BY: Rolf Berteig (based on prim.cpp)

	HISTORY: created 30 January 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "stdafx.h"
#include "editpat.h"

#include <nel/misc/debug.h>

HINSTANCE hInstance;
int controlsInit = FALSE;

using namespace NLMISC;

/** public functions **/
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);
		
		if (!controlsInit) 
		{
			controlsInit = TRUE;
			
			// jaguar controls
			InitCustomControls(hInstance);

#ifdef OLD3DCONTROLS
			// initialize 3D controls
			Ctl3dRegister(hinstDLL);
			Ctl3dAutoSubclass(hinstDLL);
#endif
			
			// initialize Chicago controls
			InitCommonControls();
		}
	}
	
	return TRUE;
}


//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------
__declspec( dllexport ) const TCHAR *LibDescription() 
{ 
	return "NeL Patch Edit"; 
}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS
__declspec( dllexport ) int LibNumberClasses()
{ 
	return 1;
}

__declspec( dllexport ) ClassDesc *LibClassDesc(int i) 
{
	switch(i) 
	{
		case 0: return GetEditPatchModDesc();
		default: return NULL;
	}
}

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion()
{ 
	return VERSION_3DSMAX;
}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

BOOL CALLBACK DefaultSOTProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	IObjParam *ip = (IObjParam*)GetWindowLong(hWnd,GWL_USERDATA);

	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLong(hWnd,GWL_USERDATA,lParam);
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			if (ip) ip->RollupMouseMessage(hWnd,msg,wParam,lParam);
			return FALSE;

		default:
			return FALSE;
	}
	return TRUE;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
}
