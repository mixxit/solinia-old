/*
	Petite documentation sur le fonctionnement du plugin

1) Le WorldEditor se connecte au plugin (config dans WorldEditorPlugin.cfg)

2) Le plugin se connecte au World_Editor_Service

3) Le World_Editor_Service (config dans WorldEditorService.cfg) envoie des paquets contenant au maximum 100 joueurs.
Ces joueurs sont soit nouveaux, soit leurs coordonn�es ont chang�.

4) Les informations des joueurs sont envoy�es (voir onIdle) sous forme de primitives "player":

Voici la d�finition de la primitive "player", dans WORLD_EDITOR_SCRIPT.XML

<PRIMITIVE CLASS_NAME="root" TYPE="node" AUTO_INIT="true" DELETABLE="true">
		<DYNAMIC_CHILD CLASS_NAME="player"/>
</PRIMITIVE>

<!-- player node -->
<PRIMITIVE CLASS_NAME="player" TYPE="point" R="255" G="0" B="0" A="128" AUTO_INIT="true" DELETABLE="false" LINK_BROTHERS="false" SHOW_ARROW="false">
<PARAMETER NAME= "x"				TYPE="string"		VISIBLE="true"> <DEFAULT_VALUE VALUE=	"0"			/> </PARAMETER>
<PARAMETER NAME= "y"				TYPE="string"		VISIBLE="true"> <DEFAULT_VALUE VALUE=	"0"			/> </PARAMETER>
</PRIMITIVE>

5) World_Editor_Service utilise un cache pour stocker les d�placements des joueurs, afin de n'envoyer que ce qui bouge

6) World_Editor_Plugin utilise aussi un cache avec un pointeur sur les Primitives, afin de ne modifier dans WE que ce qui bouge

*/
#include "stdafx.h"
#include "plugin.h"
#include "resource.h"
#include "nel/misc/path.h"
#include "LoadDialog.h"
#include "nel/sound/u_listener.h"

using namespace NLMISC;
using namespace NLLIGO;
using namespace NLSOUND;
using namespace NLNET;
using namespace std;


class Player
{
public:
	string _name;
	float _x, _y;
	const IPrimitive *primitive;
	// Constructor
	Player::Player()
	{
		primitive = NULL;
	}

	Player::Player(string name, float x, float y)
	{
		primitive = NULL;
		_name = name;
		_x = x;
		_y = y;
	}
	/// Destructor
	Player::~Player()
	{
	}
};
	
vector<Player> StackPlayers;

map<string, Player>	StoredPlayers;


extern "C"
{
	void *createPlugin()
	{
		return new CPlugin();
	}
}

CPlugin::CPlugin()
	: _PluginAccess(0)
{
	m_Initialized = false;
	_Client = NULL;
	StackPlayers.clear();
}

CPlugin::~CPlugin()
{
	delete _Client;

	if (m_Initialized)
	{
		_PluginAccess->deleteRootPluginPrimitive();
		m_Initialized = false;
	}
}

void CPlugin::progress (float progressValue)
{
	/*char tmp[1024];
	sprintf(tmp, "Initializing mixer : %i%%", int(progressValue * 100));
	LoadDlg->Message = tmp;
	LoadDlg->UpdateData(FALSE);
	LoadDlg->RedrawWindow();*/
}

// ***************************************************************************
void serverSentPos (CMessage &msgin, TSockId from, CCallbackNetBase &netbase)
{
	// Called when the server sent a POS message
	uint32 count;

	msgin.serial(count);
	for (uint i = 0;i < count;++i)
	{
		string name;
		msgin.serial(name);
		
		float x, y;
		msgin.serial(x);
		msgin.serial(y);
		// we received name, x, y, we save it in the stack
		StackPlayers.push_back (Player(name, x, y));
	}
}

// ***************************************************************************
void serverSentInfo (CMessage &msgin, TSockId from, CCallbackNetBase &netbase)
{
	// Called when the server sent a INFO message
	string text;
	msgin.serial(text);
	printf("%s\n", text.c_str());
}

// ***************************************************************************
// All messages handled by this server
#define NB_CB 2
TCallbackItem CallbackArray[NB_CB] =
{
	{ "POS", serverSentPos },
	{ "INFO", serverSentInfo },
};

void CPlugin::init(IPluginAccess *pluginAccess)
{
	CLoadDialog			*LoadDlg;
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	AfxEnableControlContainer();
	_PluginAccess = pluginAccess;
//#undef new
	LoadDlg = new CLoadDialog;
//#define new NL_NEW
	LoadDlg->Create(IDD_DIALOG_LOAD);
	LoadDlg->Message = "Connecting...";
	LoadDlg->UpdateData(FALSE);
	//LoadDlg->Invalidate();
	LoadDlg->ShowWindow(SW_SHOW);
	//LoadDlg->RedrawWindow();

	// connect to the server here...
	
	// Read the host where to connect in the client.cfg file
	CConfigFile ConfigFile;	
	ConfigFile.load ("client.cfg");
	string LSHost(ConfigFile.getVar("LSHost").asString());
	
	// Init and Connect the client to the server located on port 3333
	_Client = new CCallbackClient();
	_Client->addCallbackArray (CallbackArray, NB_CB);
	
	//printf("Please wait connecting...\n");
	try
	{
		CInetAddress addr(LSHost+":48888");
		_Client->connect(addr);
	}
	catch(ESocket &e)
	{
		MessageBox (NULL, e.what(), "Error", MB_ICONERROR|MB_OK);
		delete LoadDlg;
		LoadDlg = 0;
		return;
	}
	delete LoadDlg;
	LoadDlg = 0;
	
	if (!_Client->connected())
	{
		MessageBox (NULL, "The connection failed", "Error", MB_ICONERROR|MB_OK);
		return;
	}

//#undef new
	_DialogFlag = new CDialogFlags(/*_Mixer*/);
//#define new NL_NEW
	// open the dialog flag window
	_DialogFlag->Create(IDD_DIALOG_FLAGS, CWnd::FromHandle(_PluginAccess->getMainWindow()->m_hWnd));
	_DialogFlag->init(this);
	_DialogFlag->ShowWindow(TRUE);

	//
	{
		CMessage msg;
		msg.setType("WINDOW");
		float xmin = 0;
		float xmax = 10000;
		float ymin = 0;
		float ymax = 10000;
		msg.serial (xmin);
		msg.serial (ymin);
		msg.serial (xmax);
		msg.serial (ymax);
		_Client->send(msg);
	}
}

/// The current region has changed.
void CPlugin::primitiveChanged(const NLLIGO::IPrimitive *root)
{
}

/// The listener has been moved on the map.
void CPlugin::positionMoved(const NLMISC::CVector &position)
{
}

void CPlugin::lostPositionControl()
{
}

void CPlugin::onIdle()
{
	if (_Client && _Client->connected())
	{
		// during the first call, we create a Root primitive to store all players
		if (!m_Initialized)
		{
			m_Initialized = true;
			_PluginAccess->createRootPluginPrimitive();
		}
		else
		{
			// first, we receive the stack of messages, which is composed of players informations
			_Client->update();
			// now, we insert the players into the GUI
			for (uint i = 0; i < StackPlayers.size(); ++i)
			{
				map<string, Player>::const_iterator ite;
				ite = StoredPlayers.find(StackPlayers[i]._name);
				if (ite == StoredPlayers.end())
				{
					// if the player doesn't exist in the static list, we add it
					//StoredPlayers.insert(map<string, Player>::value_type(StackPlayers[i]._name, StackPlayers[i]));
					StoredPlayers[StackPlayers[i]._name] = StackPlayers[i];
					std::vector<CPrimitiveClass::CInitParameters> Parameters;

					Parameters.push_back (CPrimitiveClass::CInitParameters ());
					CPrimitiveClass::CInitParameters &param1 = Parameters.back ();
					param1.DefaultValue.resize (1);
					param1.Name = "x";
					param1.DefaultValue[0].GenID = false;
					param1.DefaultValue[0].Name = toString(StackPlayers[i]._x);

					Parameters.push_back (CPrimitiveClass::CInitParameters ());
					CPrimitiveClass::CInitParameters &param2 = Parameters.back ();
					param2.DefaultValue.resize (1);
					param2 = Parameters.back ();
					param2.Name = "y";
					param2.DefaultValue[0].GenID = false;
					param2.DefaultValue[0].Name = toString(StackPlayers[i]._y);

					// create a player in WorldEditor
					StoredPlayers[StackPlayers[i]._name].primitive = _PluginAccess->createPluginPrimitive("player",StackPlayers[i]._name.c_str(),NLMISC::CVector(StackPlayers[i]._x,StackPlayers[i]._y,0),0,Parameters);
				}
				else
				{
					// the player has moved, we need to change its coordinates
					_PluginAccess->movePluginPrimitive(StoredPlayers[StackPlayers[i]._name].primitive, CVector(StackPlayers[i]._x, StackPlayers[i]._y, 0));
				}
			}
			// we clear the stack
			StackPlayers.clear();
		}
	}
}
