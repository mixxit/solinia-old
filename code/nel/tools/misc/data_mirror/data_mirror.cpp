// data_mirror.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "data_mirror.h"
#include "data_mirrorDlg.h"

using namespace std;
using namespace NLMISC;

std::string				MainDirectory;
std::string				MirrorDirectory;
std::string				LogDirectory;
std::string				IgnoreDirectory;
std::string				CurrentDir;
std::set<string>		IgnoreFiles;
bool					BinaryCompare = false;

/////////////////////////////////////////////////////////////////////////////
// CData_mirrorApp

BEGIN_MESSAGE_MAP(CData_mirrorApp, CWinApp)
	//{{AFX_MSG_MAP(CData_mirrorApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CData_mirrorApp construction

CData_mirrorApp::CData_mirrorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CData_mirrorApp object

CData_mirrorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CData_mirrorApp initialization

BOOL CData_mirrorApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	try
	{
		// Get the module
		CConfigFile	cf;
		string exePath = GetCommandLine ();
		if (exePath.size()>0)
		{
			if (exePath[0] == '\"')
			{
				uint end=exePath.find ('\"', 1);
				if (end != string::npos)
				{
					exePath = exePath.substr (1, end-1);
				}
				else
				{
					nlassert (0);	// no!
				}
			}
			else
			{
				uint end=exePath.find (' ', 1);
				exePath = exePath.substr (0, end);
			}
		}

		// Register it
		string temp = "\"" + exePath;
		temp = temp+"\" \"%1\"";
		nlverify (RegisterDirectoryAppCommand ("NeLDataMirror", "&Sync Mirror", temp.c_str ()));

		// Get the app directory
		string path = NLMISC::CFile::getPath (exePath)+"config.cfg";

		cf.load (path);
		MainDirectory = cf.getVar ("MainDirectory").asString ();
		MainDirectory = strlwr (CPath::standardizePath (MainDirectory));
		
		MirrorDirectory = cf.getVar ("MirrorDirectory").asString ();
		MirrorDirectory = strlwr (CPath::standardizePath (MirrorDirectory));
		
		LogDirectory = cf.getVar ("LogDirectory").asString ();
		LogDirectory = strlwr (CPath::standardizePath (LogDirectory));
		
		IgnoreDirectory = cf.getVar ("IgnoreDirectory").asString ();
		IgnoreDirectory = strlwr (CPath::standardizePath (IgnoreDirectory));
		if (IgnoreDirectory.empty())
			IgnoreDirectory = MainDirectory;

		string sBinaryCompare = cf.getVar ("BinaryCompare").asString ();
		sBinaryCompare = strlwr (sBinaryCompare);
		BinaryCompare = false;
		if ((sBinaryCompare == "true") || (sBinaryCompare=="1"))
			BinaryCompare = true;

		CurrentDir = m_lpCmdLine;
		// Remove 
		if (CurrentDir.size ()>=2)
		{
			if (CurrentDir[0] == '"')
				CurrentDir = CurrentDir.substr (1, CurrentDir.size ()-1);
			if (CurrentDir[CurrentDir.size ()-1] == '"')
				CurrentDir = CurrentDir.substr (0, CurrentDir.size ()-1);
		}
		if (CurrentDir.empty ())
			return FALSE;


		// File or directory ?
		bool directory;
		if (NLMISC::CFile::isDirectory (CurrentDir))
		{
			directory = true;
			CurrentDir = strlwr (CPath::standardizePath (CurrentDir));
		}
		else if (NLMISC::CFile::fileExists (CurrentDir))
		{
			directory = false;
			CurrentDir = strlwr (CPath::standardizePath (NLMISC::CFile::getPath (CurrentDir)));
		}
		else
		{
			MessageBox (NULL, (CurrentDir+" is not a directory nor a file.").c_str (), "NeL Data Mirror", MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}

		// Get relative dir
		if (CurrentDir.substr (0, MainDirectory.size ()) == MainDirectory)
		{
			CurrentDir = CurrentDir.substr (MainDirectory.size (), CurrentDir.size () - MainDirectory.size ());
		}
		else
		{
			if (CurrentDir.substr (0, MirrorDirectory.size ()) == MirrorDirectory)
			{
				CurrentDir = CurrentDir.substr (MirrorDirectory.size (), CurrentDir.size () - MirrorDirectory.size ());
			}
			else
			{
				MessageBox (NULL, (CurrentDir+" is not a sub directory of "+MainDirectory+" or "+MirrorDirectory).c_str (), 
							"NeL Data Mirror", MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}

		/*
		if (CurrentDir.substr (0, MainDirectory.size ()) != MainDirectory)
		{
			MessageBox (NULL, (CurrentDir+" is not a sub directory of "+MainDirectory).c_str (), "NeL Data Mirror", MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		CurrentDir = CurrentDir.substr (MainDirectory.size (), CurrentDir.size () - MainDirectory.size ());
		*/

		// Load ignore list
		FILE *file = fopen ((IgnoreDirectory+"ignore_list.txt").c_str (), "r");
		if (file)
		{
			char line[512];
			while (fgets (line, 512, file))
			{
				// Remove last back
				int n = strlen (line);
				if (n && (line[n-1] == '\n'))
					line[n-1] = 0;
				IgnoreFiles.insert (line);
			}
		}
	}
	catch (Exception &e)
	{
		MessageBox (NULL, e.what (), "NeL Data Mirror", MB_OK|MB_ICONEXCLAMATION);
	}

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CData_mirrorDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

// Register an application command
bool RegisterDirectoryAppCommand (const char *appName, const char *command, const char *app)
{
	// Create the app key
	HKEY hKey;
	if (RegCreateKey (HKEY_CLASSES_ROOT, "Directory", &hKey) == ERROR_SUCCESS)
	{
		// Create the icon
		char tmp[512];
		smprintf (tmp, 512, "shell\\%s", appName);
		if (RegCreateKey (hKey, tmp, &hKey) == ERROR_SUCCESS)
		{
			// Set the description
			RegSetValue (hKey, "", REG_SZ, command, strlen (command));
			if (RegCreateKey (hKey, "command", &hKey) == ERROR_SUCCESS)
			{
				// Set the description
				RegSetValue (hKey, "", REG_SZ, app, strlen (app));
				return true;
			}
		}
	}
	return false;
}
