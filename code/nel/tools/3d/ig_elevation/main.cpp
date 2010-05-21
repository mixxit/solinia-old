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

#include <vector>
#include <string>

#include "nel/misc/config_file.h"
#include "nel/misc/file.h"
#include "nel/misc/bitmap.h"
#include "nel/misc/block_memory.h"
#include "nel/misc/i_xml.h"
#include "nel/misc/app_context.h"

#include "nel/../../src/ligo/zone_region.h"

#include "nel/3d/scene_group.h"

#include <windows.h>

// ---------------------------------------------------------------------------

using namespace std;
using namespace NLMISC;
using namespace NLLIGO;
using namespace NL3D;

// ---------------------------------------------------------------------------
// Out a string to the stdout and log.log
void outString (string &sText)
{
	createDebug ();
	InfoLog->displayRaw(sText.c_str());
	//printf ("%s", sText.c_str());
}

// ---------------------------------------------------------------------------
struct SExportOptions
{
	string	InputIGDir;
	string	OutputIGDir;
	float	CellSize;
	string	HeightMapFile1;
	float	ZFactor1;
	string	HeightMapFile2;
	float	ZFactor2;
	string	LandFile;

	// -----------------------------------------------------------------------
	bool load (const string &sFilename)
	{
		FILE * f = fopen (sFilename.c_str(), "rt");
		if (f == NULL)
			return false;
		else 
			fclose (f);
		
		try
		{			
			CConfigFile cf;
		
			cf.load (sFilename);

			// Out
			CConfigFile::CVar &cvOutputIGDir = cf.getVar("OutputIGDir");
			OutputIGDir = cvOutputIGDir.asString();

			// In
			CConfigFile::CVar &cvInputIGDir = cf.getVar("InputIGDir");
			InputIGDir = cvInputIGDir.asString();

			CConfigFile::CVar &cvCellSize = cf.getVar("CellSize");
			CellSize = cvCellSize.asFloat();

			CConfigFile::CVar &cvHeightMapFile1 = cf.getVar("HeightMapFile1");
			HeightMapFile1 = cvHeightMapFile1.asString();

			CConfigFile::CVar &cvZFactor1 = cf.getVar("ZFactor1");
			ZFactor1 = cvZFactor1.asFloat();

			CConfigFile::CVar &cvHeightMapFile2 = cf.getVar("HeightMapFile2");
			HeightMapFile2 = cvHeightMapFile2.asString();

			CConfigFile::CVar &cvZFactor2 = cf.getVar("ZFactor2");
			ZFactor2 = cvZFactor2.asFloat();

			CConfigFile::CVar &cvLandFile = cf.getVar("LandFile");
			LandFile = cvLandFile.asString();
		}
		catch (EConfigFile &e)
		{
			string sTmp = string("ERROR : Error in config file : ") + e.what() + "\n";
			outString (sTmp);
			return false;
		}
		return true;
	}
};


struct CZoneLimits
{
	sint32 _ZoneMinX;
	sint32 _ZoneMaxX;
	sint32 _ZoneMinY;
	sint32 _ZoneMaxY;
};

// ---------------------------------------------------------------------------
void dir (const string &sFilter, vector<string> &sAllFiles, bool bFullPath)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	char sCurDir[MAX_PATH];
	sAllFiles.clear ();
	GetCurrentDirectory (MAX_PATH, sCurDir);
	hFind = FindFirstFile (sFilter.c_str(), &findData);	
	while (hFind != INVALID_HANDLE_VALUE)
	{
		if (!(GetFileAttributes(findData.cFileName)&FILE_ATTRIBUTE_DIRECTORY))
		{
			if (bFullPath)
				sAllFiles.push_back(string(sCurDir) + "\\" + findData.cFileName);
			else
				sAllFiles.push_back(findData.cFileName);
		}
		if (FindNextFile (hFind, &findData) == 0)
			break;
	}
	FindClose (hFind);
}


// ---------------------------------------------------------------------------
CZoneRegion *loadLand (const string &filename)
{
	CZoneRegion *ZoneRegion = NULL;
	try
	{
		CIFile fileIn;
		if (fileIn.open (filename))
		{
			// Xml
			CIXml xml (true);
			nlverify (xml.init (fileIn));

			ZoneRegion = new CZoneRegion;
			ZoneRegion->serial (xml);
		}
		else
		{
			string sTmp = string("Can't open the land file : ") + filename;
			outString (sTmp);
		}
	}
	catch (Exception& e)
	{
		string sTmp = string("Error in land file : ") + e.what();
		outString (sTmp);
	}
	return ZoneRegion;
}


// ***************************************************************************
CInstanceGroup* LoadInstanceGroup (const char* sFilename)
{
	CIFile file;
	CInstanceGroup *newIG = new CInstanceGroup;

	if( file.open( sFilename ) )
	{
		try
		{
			newIG->serial (file);
		}
		catch (Exception &)
		{
			// Cannot save the file
			delete newIG;
			return NULL;
		}
	}
	else
	{
		delete newIG;
		return NULL;
	}
	return newIG;
}

// ***************************************************************************
void SaveInstanceGroup (const char* sFilename, CInstanceGroup *pIG)
{
	COFile file;

	if( file.open( sFilename ) )
	{
		try
		{
			pIG->serial (file);
		}
		catch (Exception &e)
		{
			outString(string(e.what()));
		}
	}
	else
	{
		outString(string("Couldn't create ") + sFilename);
	}
}

/** Get the Z of the height map at the given position
  */
static float  getHeightMapZ(float x, float y, const CZoneLimits &zl, const SExportOptions &options, CBitmap *heightMap1, CBitmap *heightMap2)
{
	float deltaZ = 0.0f, deltaZ2 = 0.0f;
	CRGBAF color;
	sint32 SizeX = zl._ZoneMaxX - zl._ZoneMinX + 1;
	sint32 SizeY = zl._ZoneMaxY - zl._ZoneMinY + 1;

	clamp (x, options.CellSize * zl._ZoneMinX, options.CellSize * (zl._ZoneMaxX + 1));
	clamp (y, options.CellSize * zl._ZoneMinY, options.CellSize * (zl._ZoneMaxY + 1));

	if (heightMap1 != NULL)
	{
		color = heightMap1->getColor (	(x - options.CellSize * zl._ZoneMinX) / (options.CellSize * SizeX), 
										1.0f - ((y - options.CellSize * zl._ZoneMinY) / (options.CellSize * SizeY)));
		color *= 255.f;
		deltaZ = color.A;
		deltaZ = deltaZ - 127.0f; // Median intensity is 127
		deltaZ *= options.ZFactor1;
	}

	if (heightMap2 != NULL)
	{
		color = heightMap2->getColor (	(x - options.CellSize * zl._ZoneMinX) / (options.CellSize * SizeX), 
										1.0f - ((y - options.CellSize * zl._ZoneMinY) / (options.CellSize * SizeY)));
		color *= 255.f;
		deltaZ2 = color.A;
		deltaZ2 = deltaZ2 - 127.0f; // Median intensity is 127
		deltaZ2 *= options.ZFactor2;
	}

	return deltaZ + deltaZ2;	
}

// ---------------------------------------------------------------------------
int main(int nNbArg, char**ppArgs)
{
	if (!NLMISC::INelContext::isContextInitialised())
		new CApplicationContext();

	NL3D_BlockMemoryAssertOnPurge = false;
	char sCurDir[MAX_PATH];
	GetCurrentDirectory (MAX_PATH, sCurDir);
	
	if (nNbArg != 2)
	{
		printf ("Use : ig_elevation configfile.cfg\n");
		printf ("\nExample of config.cfg\n\n");
		printf ("InputIGDir = \"ig_land_max\";\n");
		printf ("OutputIGDir = \"ig_land_max_elev\";\n");
		printf ("CellSize = 160.0;\n");
		printf ("HeightMapFile1 = \"w:/database/landscape/ligo/jungle/big.tga\";\n");
		printf ("ZFactor1 = 1.0;\n");
		printf ("HeightMapFile2 = \"w:/database/landscape/ligo/jungle/noise.tga\";\n");
		printf ("ZFactor2 = 0.5;\n");
		printf ("LandFile = \"w:/matis.land\";\n");

		return -1;
	}

	SExportOptions options;
	if (!options.load(ppArgs[1]))
	{
		return -1;
	}

	// Get all ig files in the input directory and elevate to the z of the double heightmap

	// Load the land
	CZoneRegion *ZoneRegion = loadLand(options.LandFile);

	CZoneLimits zl;
	if (ZoneRegion)
	{
		zl._ZoneMinX = ZoneRegion->getMinX() < 0	? 0		: ZoneRegion->getMinX();
		zl._ZoneMaxX = ZoneRegion->getMaxX() > 255	? 255	: ZoneRegion->getMaxX();
		zl._ZoneMinY = ZoneRegion->getMinY() > 0	? 0		: ZoneRegion->getMinY();
		zl._ZoneMaxY = ZoneRegion->getMaxY() < -255 ? -255	: ZoneRegion->getMaxY();
	}
	else
	{
		nlwarning("A ligo .land file cannot be found");
		zl._ZoneMinX = 0;
		zl._ZoneMaxX = 255;
		zl._ZoneMinY = 0;
		zl._ZoneMaxY = 255;
	}

	// Load the 2 height maps
	CBitmap *HeightMap1 = NULL;
	if (options.HeightMapFile1 != "")
	{
		HeightMap1 = new CBitmap;
		try 
		{
			CIFile inFile;
			if (inFile.open(options.HeightMapFile1))
			{
				HeightMap1->load (inFile);
			}
			else
			{
				outString(string("Couldn't not open " + options.HeightMapFile1 + " : heightmap 1 map ignored"));
				delete HeightMap1;
				HeightMap1 = NULL;
			}
		}
		catch (Exception &e)
		{
			string sTmp = string("Cant load height map : ") + options.HeightMapFile1 + " : " + e.what();
			outString (sTmp);
			delete HeightMap1;
			HeightMap1 = NULL;
		}
	}
	CBitmap *HeightMap2 = NULL;
	if (options.HeightMapFile2 != "")
	{
		HeightMap2 = new CBitmap;
		try 
		{
			CIFile inFile;
			if (inFile.open(options.HeightMapFile2))
			{
				HeightMap2->load (inFile);
			}
			else
			{
				outString(string("Couldn't not open " + options.HeightMapFile2 + " : heightmap 2 map ignored\n"));
				delete HeightMap2;
				HeightMap2 = NULL;
			}
		}
		catch (Exception &e)
		{
			string sTmp = string("Cant load height map : ") + options.HeightMapFile2 + " : " + e.what() + "\n";
			outString (sTmp);
			delete HeightMap2;
			HeightMap1 = NULL;
		}
	}

	// Get all files
	vector<string> vAllFiles;
	SetCurrentDirectory (options.InputIGDir.c_str());
	dir ("*.ig", vAllFiles, false);
	SetCurrentDirectory (sCurDir);

	for (uint32 i = 0; i < vAllFiles.size(); ++i)
	{
		SetCurrentDirectory (options.InputIGDir.c_str());
		CInstanceGroup *pIG = LoadInstanceGroup (vAllFiles[i].c_str());
		SetCurrentDirectory (sCurDir);
		if (pIG != NULL)
		{
			bool realTimeSunContribution = pIG->getRealTimeSunContribution();
			// For all instances !!!
			CVector vGlobalPos;
			CInstanceGroup::TInstanceArray IA;
			vector<CCluster> Clusters;
			vector<CPortal> Portals;
			vector<CPointLightNamed> PLN;
			pIG->retrieve (vGlobalPos, IA, Clusters, Portals, PLN);
			
			if (IA.empty() && PLN.empty() && Portals.empty() && Clusters.empty()) continue;


			uint k;

			

			// elevate instance
			for(k = 0; k < IA.size(); ++k)
			{
				CVector instancePos = vGlobalPos + IA[k].Pos;
				IA[k].Pos.z += getHeightMapZ(instancePos.x, instancePos.y, zl, options, HeightMap1, HeightMap2);
			}

			// lights
			for(k = 0; k < PLN.size(); ++k)
			{
				CVector lightPos = vGlobalPos + PLN[k].getPosition();
				PLN[k].setPosition( PLN[k].getPosition() + getHeightMapZ(lightPos.x, lightPos.y, zl, options, HeightMap1, HeightMap2) * CVector::K);
			}
		

			// portals
			std::vector<CVector> portal;
			for(k = 0; k < Portals.size(); ++k)
			{
				Portals[k].getPoly(portal);
				if (portal.empty())
				{
					nlwarning("Empty portal found");
					continue;
				}
				// compute mean position of the poly
				CVector meanPos(0, 0, 0);
				uint l;
				for(l = 0; l < portal.size(); ++l)
					meanPos += portal[l];
				meanPos /= (float) portal.size();
				meanPos += vGlobalPos;
				float z = getHeightMapZ(meanPos.x, meanPos.y, zl, options, HeightMap1, HeightMap2);
				for(l = 0; l < portal.size(); ++l)
				{
					portal[l].z += z;
				}
				Portals[k].setPoly(portal);
			}

			// clusters
			std::vector<CPlane> volume;
			CMatrix transMatrix;
			for(k = 0; k < Clusters.size(); ++k)
			{
				CVector clusterPos = vGlobalPos + Clusters[k].getBBox().getCenter();
				float z = getHeightMapZ(clusterPos.x, clusterPos.y, zl, options, HeightMap1, HeightMap2);
				transMatrix.setPos(z * CVector::K);
				Clusters[k].applyMatrix(transMatrix);
			}

			CInstanceGroup *pIGout = new CInstanceGroup;
			pIGout->build (vGlobalPos, IA, Clusters, Portals, PLN);
			pIGout->enableRealTimeSunContribution(realTimeSunContribution);


			SetCurrentDirectory (options.OutputIGDir.c_str());
			SaveInstanceGroup (vAllFiles[i].c_str(), pIGout);
			SetCurrentDirectory (sCurDir);
			delete pIG;
		}
	}

	return 1;
}