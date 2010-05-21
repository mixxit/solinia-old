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

#include "std_afx.h"
#include "nel_export.h"
#include "nel/misc/file.h"
#include "nel/3d/shape.h"
#include "nel/3d/animation.h"
#include "nel/3d/skeleton_shape.h"
#include "nel/3d/vegetable_shape.h"
#include "nel/3d/lod_character_shape.h"
#include "../nel_mesh_lib/export_nel.h"
#include "../nel_mesh_lib/export_appdata.h"


using namespace NL3D;
using namespace NLMISC;

// --------------------------------------------------

bool CNelExport::exportMesh (const char *sPath, INode& node, TimeValue time)
{
	// Result to return
	bool bRet=false;

	// Eval the object a time
	ObjectState os = node.EvalWorldState(time);

	// Object exist ?
	if (os.obj)
	{
		// Skeleton shape
		CSkeletonShape *skeletonShape=NULL;
		TInodePtrInt *mapIdPtr=NULL;
		TInodePtrInt mapId;

		// If model skinned ?
		if (CExportNel::isSkin (node))
		{
			// Create a skeleton
			INode *skeletonRoot=CExportNel::getSkeletonRootBone (node);

			// Skeleton exist ?
			if (skeletonRoot)
			{
				// Build a skeleton
				skeletonShape=new CSkeletonShape();

				// Add skeleton bind pos info
				CExportNel::mapBoneBindPos boneBindPos;
				CExportNel::addSkeletonBindPos (node, boneBindPos);

				// Build the skeleton based on the bind pos information
				_ExportNel->buildSkeletonShape (*skeletonShape, *skeletonRoot, &boneBindPos, mapId, time);

				// Set the pointer to not NULL
				mapIdPtr=&mapId;

				// Erase the skeleton
				if (skeletonShape)
					delete skeletonShape;
			}
		}

		DWORD t = timeGetTime();
		if (InfoLog)
			InfoLog->display("Beg buildShape %s \n", node.GetName());
		// Export in mesh format
		IShape*	pShape=_ExportNel->buildShape (node, time, mapIdPtr, true);
		if (InfoLog)
			InfoLog->display("End buildShape in %d ms \n", timeGetTime()-t);

		// Conversion success ?
		if (pShape)
		{
			// Open a file
			COFile file;
			if (file.open (sPath))
			{
				try
				{
					// Create a streamable shape
					CShapeStream shapeStream (pShape);
					
					// Serial the shape
					shapeStream.serial (file);

					// All is good
					bRet=true;
				}
				catch (...)
				{
				}
			}

			// Delete the pointer
			delete pShape;
		}
	}
	return bRet;
}

// --------------------------------------------------

bool CNelExport::exportVegetable (const char *sPath, INode& node, TimeValue time)
{
	bool bRet=false;

	// Build a vegetable
	NL3D::CVegetableShape vegetable;
	if (_ExportNel->buildVegetableShape (vegetable, node, time))
	{
		// Open a file
		COFile file;
		if (file.open (sPath))
		{
			try
			{
				// Serial the shape
				vegetable.serial (file);

				// All is good
				bRet=true;
			}
			catch (Exception &e)
			{
				// Message box
				const char *message = e.what();
				_ExportNel->outputErrorMessage ("Error during vegetable serialisation");
			}
		}
	}
	return bRet;
}

// --------------------------------------------------

bool CNelExport::exportAnim (const char *sPath, std::vector<INode*>& vectNode, TimeValue time, bool scene)
{
	// Result to return
	bool bRet=false;

	// Create an animation file
	CAnimation animFile;

	// For each node to export
	for (uint n=0; n<vectNode.size(); n++)
	{				
		// Get name
		std::string nodeName="";

		// Get NEL3D_APPDATA_EXPORT_ANIMATION_PREFIXE_NAME
		int prefixe = CExportNel::getScriptAppData (vectNode[n], NEL3D_APPDATA_EXPORT_ANIMATION_PREFIXE_NAME, 0);
		
		// Set the name only if it is a scene animation
		if (scene || prefixe)
		{
			// try to get the prefix from the appData if present. If not, takes it from the node name
			nodeName = CExportNel::getScriptAppData (vectNode[n], NEL3D_APPDATA_INSTANCE_NAME, "");
			if (nodeName == "") // not found ?
			{
				nodeName=CExportNel::getName (*vectNode[n]);
			}
			nodeName+=".";
		}

		// Is a root ?
		bool root = vectNode[n]->GetParentNode () == _Ip->GetRootNode();

		// Add animation
		_ExportNel->addAnimation (animFile, *vectNode[n], nodeName.c_str(), root);		
	}

	if (vectNode.size())
	{
		// Open a file
		COFile file;
		if (file.open (sPath))
		{
			try
			{
				// Serial the animation
				animFile.serial (file);

				// All is good
				bRet=true;
			}
			catch (Exception& e)
			{
				if (_ErrorInDialog)
					MessageBox (NULL, e.what(), "NeL export", MB_OK|MB_ICONEXCLAMATION);
				else
					nlwarning ("ERROR : %s", e.what ());
			}
		}
		else
		{
			if (_ErrorInDialog)
				MessageBox (NULL, "Can't open the file for writing.", "NeL export", MB_OK|MB_ICONEXCLAMATION);
			else
				nlwarning ("ERROR : Can't open the file (%s) for writing", sPath);
		}
	}
	return bRet;
}

// --------------------------------------------------

bool CNelExport::exportSkeleton	(const char *sPath, INode* pNode, TimeValue time)
{
	// Result to return
	bool bRet=false;

	// Build the skeleton format
	CSkeletonShape *skeletonShape=new CSkeletonShape();
	TInodePtrInt mapId;
	_ExportNel->buildSkeletonShape (*skeletonShape, *pNode, NULL, mapId, time);

	// Open a file
	COFile file;
	if (file.open (sPath))
	{
		try
		{
			// Create a streamable shape
			CShapeStream shapeStream (skeletonShape);
			
			// Serial the shape
			shapeStream.serial (file);

			// All is good
			bRet=true;
		}
		catch (Exception &e)
		{
			nlwarning (e.what());
		}
	}

	// Delete the pointer
	delete skeletonShape;

	return bRet;
}

// --------------------------------------------------

bool CNelExport::exportLodCharacter (const char *sPath, INode& node, TimeValue time)
{
	// Result to return
	bool bRet=false;

	// Eval the object a time
	ObjectState os = node.EvalWorldState(time);

	// Object exist ?
	if (os.obj)
	{
		// Skeleton shape
		CSkeletonShape *skeletonShape=NULL;
		TInodePtrInt *mapIdPtr=NULL;
		TInodePtrInt mapId;

		// If model skinned ?
		if (CExportNel::isSkin (node))
		{
			// Create a skeleton
			INode *skeletonRoot=CExportNel::getSkeletonRootBone (node);

			// Skeleton exist ?
			if (skeletonRoot)
			{
				// Build a skeleton
				skeletonShape=new CSkeletonShape();

				// Add skeleton bind pos info
				CExportNel::mapBoneBindPos boneBindPos;
				CExportNel::addSkeletonBindPos (node, boneBindPos);

				// Build the skeleton based on the bind pos information
				_ExportNel->buildSkeletonShape (*skeletonShape, *skeletonRoot, &boneBindPos, mapId, time);

				// Set the pointer to not NULL
				mapIdPtr=&mapId;

				// Erase the skeleton
				if (skeletonShape)
					delete skeletonShape;
			}
		}

		// Conversion success ?
		CLodCharacterShapeBuild		lodBuild;
		if (_ExportNel->buildLodCharacter (lodBuild, node, time, mapIdPtr) )
		{
			// Open a file
			COFile file;
			if (file.open (sPath))
			{
				try
				{
					// Serial the shape
					lodBuild.serial (file);

					// All is good
					bRet=true;
				}
				catch (...)
				{
				}
			}
		}
	}
	return bRet;
}
