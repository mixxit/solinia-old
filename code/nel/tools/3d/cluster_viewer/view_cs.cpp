// NeL - MMORPG Framework <http://dev.ryzom.com/projects/nel/>
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


// ---------------------------------------------------------------------------
// Includes
// ---------------------------------------------------------------------------


#include "nel/misc/types_nl.h"
#include "nel/misc/file.h"
#include "nel/misc/bitmap.h"

#include "nel/misc/event_server.h"
#include "nel/misc/event_listener.h"
#include "nel/misc/events.h"
#include "nel/misc/path.h"
#include "nel/misc/time_nl.h"
#include "nel/3d/driver.h"
#include "nel/3d/nelu.h"
#include "nel/3d/scene_group.h"
#include "nel/3d/transform_shape.h"
#include "nel/3d/event_mouse_listener.h"
#include "nel/3d/text_context.h"

#ifdef NL_OS_WINDOWS
#	define NOMINMAX
#	include <windows.h>
#endif // NL_OS_WINDOWS

using namespace std;
using namespace NL3D;
using namespace NLMISC;


// ---------------------------------------------------------------------------
struct SDispCS
{
	string Name;
	CInstanceGroup *pIG;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
char *readLine (char*out, char*in)
{
	out[0] = 0;

	while ((*in != '\r') && (*in != '\n') && (*in != 0))
	{
		*out = *in;
		++out;
		++in;
		*out = 0;
	}
	
	while ((*in == '\r') || (*in == '\n'))
		++in;
	return in;
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
CInstanceGroup* LoadInstanceGroup(const char* sFilename)
{
	CIFile file;
	CInstanceGroup *newIG = new CInstanceGroup;

	if( file.open( CPath::lookup( string(sFilename) ) ) )
	{
		try
		{
			// Serial the skeleton
			newIG->serial (file);
			// All is good
		}
		catch (Exception &)
		{
			// Cannot save the file
			delete newIG;
			return NULL;
		}
	}
	return newIG;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void LoadSceneScript (const char *ScriptName, CScene* pScene, vector<SDispCS> &DispCS, CVector &CameraStart)
{
	char nameIG[256];
	float posx, posy, posz;
	float roti, rotj, rotk;
	
	FILE *f = fopen (ScriptName,"rb");
	fseek (f, 0, SEEK_END);
	int file_size = ftell (f);
	fseek (f, 0, SEEK_SET);
	char *file_buf = (char*)malloc(file_size+1);
	fread (file_buf, 1, file_size, f);
	file_buf[file_size] = 0;
	++file_size;
	fclose (f);
	char *buf_ptr = file_buf;
	int nLastNbPlus = 0;
	vector<CInstanceGroup*> pile;
	pile.clear ();

	do 
	{
		char Line[256], *line_ptr;
		int nNbPlus = 0;
		line_ptr = &Line[0];
		buf_ptr = readLine (line_ptr, buf_ptr);

		while ((*line_ptr == '\t') || (*line_ptr == ' ') || (*line_ptr == '+'))
		{
			if (*line_ptr == '+') 
				++nNbPlus;
			++line_ptr;
		}

		if (strlen (line_ptr) == 0)
			break;

		if (*line_ptr == '/')
			continue;

		posx = posy = posz = roti = rotj = rotk = 0.0f;
		sscanf (line_ptr, "%s %f %f %f %f %f %f", nameIG, &posx, &posy, &posz, &roti, &rotj, &rotk);

		if (stricmp (nameIG, "CAMERA_START") == 0)
		{
			CameraStart = CVector(posx, posy, posz);
		}
		else
		{
	 		if (nLastNbPlus >= nNbPlus)
			for (int i = 0; i < ((nLastNbPlus-nNbPlus)+1); ++i)
				if (pile.size() > 0)
					pile.pop_back();
		
			nLastNbPlus = nNbPlus;

			CInstanceGroup *father = pScene->getGlobalInstanceGroup();
			
			if (pile.size() > 0)
				father = pile.back();
		
			CInstanceGroup *ITemp = LoadInstanceGroup (nameIG);
			if (ITemp != NULL)
			{
				SDispCS dcsTemp;
				dcsTemp.Name = "";
				for (sint32 i = 0; i < (1+nNbPlus); ++i)
					dcsTemp.Name += "   ";
				dcsTemp.Name += nameIG;
				dcsTemp.pIG = ITemp;
				DispCS.push_back (dcsTemp);
				
				ITemp->createRoot (*pScene);
				ITemp->setPos (CVector(posx, posy, posz));
				ITemp->setRotQuat (ITemp->getRotQuat() * CQuat(CVector::I, roti));
				ITemp->setRotQuat (ITemp->getRotQuat() * CQuat(CVector::J, rotj));
				ITemp->setRotQuat (ITemp->getRotQuat() * CQuat(CVector::K, rotk));
				ITemp->setClusterSystemForInstances (father);
				ITemp->addToScene (*pScene);
			}
			pile.push_back (ITemp);
		}
	} 
	while (strlen(buf_ptr) > 0);
	free (file_buf);
}


// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

#ifdef NL_OS_WINDOWS
int CALLBACK WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else // NL_OS_WINDOWS
int main(int argc, char **argv)
#endif // NL_OS_WINDOWS
{
	double rGlobalTime = 0;
	double rOldGlobalTime = 0;
	double rDeltaTime = 0;
	double rMoveTime = 0;

	vector<SDispCS> DispCS;

	bool bAutoDetect = true;
	bool bDisplay = true;
	sint32 nSelected = 0;

	SDispCS dcsTemp;


	CNELU::init (800, 600, CViewport(), 32, true);

	CPath::addSearchPath("shapes/");
	CPath::addSearchPath("groups/");
	CPath::addSearchPath("maps/");
	CPath::addSearchPath("lightmaps/");
	CPath::addSearchPath("anims/");
	///// CPath::addSearchPath("fonts/");

	CFontManager FontManager;
	CTextContext TextContext;

	TextContext.init (CNELU::Driver, &FontManager);	
	TextContext.setFontGenerator ("fonts/n019003l.pfb");
	TextContext.setHotSpot (CComputedString::TopLeft);
	TextContext.setColor (CRGBA(255,255,255));
	TextContext.setFontSize (20);

	CEvent3dMouseListener MouseListener;
	MouseListener.addToServer (CNELU::EventServer);
	MouseListener.setFrustrum (CNELU::Camera->getFrustum());
	MouseListener.setMouseMode (CEvent3dMouseListener::firstPerson);
	
	CNELU::Camera->setTransformMode (ITransformable::DirectMatrix);
	// Force to automatically find the cluster system
	CNELU::Camera->setClusterSystem ((CInstanceGroup*)-1); 


	CClipTrav *pClipTrav = &CNELU::Scene->getClipTrav();
	dcsTemp.Name = "Root";
	dcsTemp.pIG = NULL;
	DispCS.push_back (dcsTemp);

	
	// Add all instance that create the scene
	// --------------------------------------	
	// Begining of script reading
	CVector CameraStart;

	LoadSceneScript ("view_cs.txt", CNELU::Scene, DispCS, CameraStart);

	CNELU::Scene->enableLightingSystem(true);
	CMatrix m = MouseListener.getViewMatrix();
	m.setPos (CameraStart);
	MouseListener.setMatrix (m);
	// End of script reading

	// Main loop
	do
	{
		rGlobalTime = CTime::ticksToSecond(CTime::getPerformanceTime());
		rDeltaTime = rGlobalTime - rOldGlobalTime;
		rOldGlobalTime = rGlobalTime;


		CNELU::Scene->animate ((float)rGlobalTime);

		CNELU::EventServer.pump();

		CNELU::clearBuffers (CRGBA(0,0,0));

		CNELU::Scene->render ();

		// -----------------------------------------------------
		// -----------------------------------------------------
		if (bDisplay)
		{
			vector<CCluster*> vCluster;
			DispCS[0].pIG = pClipTrav->RootCluster->Group;
			TextContext.setColor (CRGBA(255,255,255,255));
			if (bAutoDetect)
			{
				TextContext.printfAt (0, 1, "AutoDetect : ON");
				
				pClipTrav->fullSearch (vCluster, pClipTrav->CamPos);

				for (uint32 i = 0; i < DispCS.size(); ++i)
				{
					TextContext.setColor (CRGBA(255,255,255,255));
					for( uint32 j = 0; j < vCluster.size(); ++j)
					{
						if (DispCS[i].pIG == vCluster[j]->Group)
						{
							TextContext.setColor (CRGBA(255,0,0,255));
							break;
						}
					}

					TextContext.printfAt (0, 1-(i+2)*0.028f, DispCS[i].Name.c_str());
				}

			}
			else
			{
				TextContext.printfAt (0, 1, "AutoDetect : OFF");

				CInstanceGroup *pCurIG = CNELU::Camera->getClusterSystem();
				for (uint32 i = 0; i < DispCS.size(); ++i)
				{
					if (DispCS[i].pIG == pCurIG)
						TextContext.setColor (CRGBA(255,0,0,255));
					else
						TextContext.setColor (CRGBA(255,255,255,255));

					TextContext.printfAt (0, 1-(i+2)*0.028f, DispCS[i].Name.c_str());
				}

				TextContext.setColor (CRGBA(255,255,255,255));

				pClipTrav->Accel.select (pClipTrav->CamPos, pClipTrav->CamPos);
				CQuadGrid<CCluster*>::CIterator itAcc = pClipTrav->Accel.begin();
				while (itAcc != pClipTrav->Accel.end())
				{
					CCluster *pCluster = *itAcc;
					if (pCluster->Group == pClipTrav->Camera->getClusterSystem())
					if (pCluster->isIn (pClipTrav->CamPos))
					{
						vCluster.push_back (pCluster);
					}
					++itAcc;
				}
				if ((vCluster.size() == 0) && (DispCS[0].pIG == pCurIG))
				{
					vCluster.push_back (pClipTrav->RootCluster);
				}

			}			

			TextContext.setColor (CRGBA(255,255,255,255));

			string sAllClusters = "";
			for( uint32 j = 0; j < vCluster.size(); ++j)
			{
				sAllClusters += vCluster[j]->Name;
				if (j < (vCluster.size()-1))
					sAllClusters += ",  ";
			}
			TextContext.printfAt (0, 1-0.028f, sAllClusters.c_str());				


		}

		// -----------------------------------------------------
		// -----------------------------------------------------

		CNELU::Driver->swapBuffers ();

		
		// Keys management
		// ---------------

		if (CNELU::AsyncListener.isKeyDown (KeySHIFT))
			MouseListener.setSpeed (10.0f);
		else
			MouseListener.setSpeed (2.0f);

		CNELU::Camera->setMatrix (MouseListener.getViewMatrix());

		if (CNELU::AsyncListener.isKeyPushed(KeyL))
		{
			CNELU::Driver->setPolygonMode (IDriver::Line);
		}

		if (CNELU::AsyncListener.isKeyPushed (KeyP))
		{
			CNELU::Driver->setPolygonMode (IDriver::Filled);
		}

		if (CNELU::AsyncListener.isKeyPushed (KeyTAB))
			bDisplay = !bDisplay;

		if (CNELU::AsyncListener.isKeyPushed (KeyA))
		{
			if (bAutoDetect)
			{
				bAutoDetect = false;
				CNELU::Camera->setClusterSystem (NULL);
				nSelected = 0;
			}
			else
			{
				bAutoDetect = true;
				CNELU::Camera->setClusterSystem ((CInstanceGroup*)-1);
			}
		}

		if (!bAutoDetect)
		{
			if (CNELU::AsyncListener.isKeyPushed (KeyZ))
			{
				nSelected--;
				if(nSelected == -1)
					nSelected = (sint32)DispCS.size()-1;
			}
			if (CNELU::AsyncListener.isKeyPushed (KeyS))
			{
				nSelected++;
				if(nSelected == (sint32)DispCS.size())
					nSelected = 0;
			}
			CNELU::Camera->setClusterSystem (DispCS[nSelected].pIG);
		}

	}
	while (!CNELU::AsyncListener.isKeyPushed (KeyESCAPE));
	return 0;
}
