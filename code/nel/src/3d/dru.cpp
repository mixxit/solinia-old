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

#include "std3d.h"

#include "nel/misc/types_nl.h"
#include "nel/misc/dynloadlib.h"

#include "nel/3d/dru.h"
#include "nel/3d/driver.h"
#include "nel/3d/material.h"
#include "nel/3d/vertex_buffer.h"
#include "nel/3d/index_buffer.h"


#ifdef NL_OS_WINDOWS
#	define NOMINMAX
#	include <windows.h>
#else // NL_OS_WINDOWS
#	include <dlfcn.h>
#endif // NL_OS_WINDOWS

using namespace NLMISC;
using namespace std;

namespace NL3D
{


typedef IDriver* (*IDRV_CREATE_PROC)(void);
const char *IDRV_CREATE_PROC_NAME = "NL3D_createIDriverInstance";

typedef uint32 (*IDRV_VERSION_PROC)(void);
const char *IDRV_VERSION_PROC_NAME = "NL3D_interfaceVersion";

#ifdef NL_STATIC
extern IDriver* createGlDriverInstance ();

#ifdef NL_OS_WINDOWS
extern IDriver* createD3DDriverInstance ();
#endif

#endif

// ***************************************************************************
IDriver		*CDRU::createGlDriver() throw (EDru)
{
#ifdef NL_STATIC

	return createGlDriverInstance ();

#else

	IDRV_CREATE_PROC	createDriver = NULL;
	IDRV_VERSION_PROC	versionDriver = NULL;

//#ifdef NL_OS_WINDOWS

	// WINDOWS code.
//	HINSTANCE			hInst;

//	hInst=LoadLibrary(NL3D_GL_DLL_NAME);
	CLibrary	driverLib;

//	if (!hInst)
	if (!driverLib.loadLibrary(NL3D_GL_DLL_NAME, true, true, false))
	{
		throw EDruOpenglDriverNotFound();
	}

//	char buffer[1024], *ptr;
//	SearchPath (NULL, NL3D_GL_DLL_NAME, NULL, 1023, buffer, &ptr);
	nlinfo ("Using the library '"NL3D_GL_DLL_NAME"' that is in the directory: '%s'", driverLib.getLibFileName().c_str());

//	createDriver = (IDRV_CREATE_PROC) GetProcAddress (hInst, IDRV_CREATE_PROC_NAME);
	createDriver = (IDRV_CREATE_PROC) driverLib.getSymbolAddress(IDRV_CREATE_PROC_NAME);
	if (createDriver == NULL)
	{
		throw EDruOpenglDriverCorrupted();
	}

//	versionDriver = (IDRV_VERSION_PROC) GetProcAddress (hInst, IDRV_VERSION_PROC_NAME);
	versionDriver = (IDRV_VERSION_PROC) driverLib.getSymbolAddress(IDRV_VERSION_PROC_NAME);
	if (versionDriver != NULL)
	{
		if (versionDriver()<IDriver::InterfaceVersion)
			throw EDruOpenglDriverOldVersion();
		else if (versionDriver()>IDriver::InterfaceVersion)
			throw EDruOpenglDriverUnknownVersion();
	}

//#elif defined (NL_OS_UNIX)
//
//	void *handle = dlopen(NL3D_GL_DLL_NAME, RTLD_NOW);
//
//	if (handle == NULL)
//	{
//		nlwarning ("when loading dynamic library '%s': %s", NL3D_GL_DLL_NAME, dlerror());
//		throw EDruOpenglDriverNotFound();
//	}
//
//	/* Not ANSI. Might produce a warning */
//	createDriver = (IDRV_CREATE_PROC) dlsym (handle, IDRV_CREATE_PROC_NAME);
//	if (createDriver == NULL)
//	{
//		nlwarning ("when getting function in dynamic library '%s': %s", NL3D_GL_DLL_NAME, dlerror());
//		throw EDruOpenglDriverCorrupted();
//	}
//
//	versionDriver = (IDRV_VERSION_PROC) dlsym (handle, IDRV_VERSION_PROC_NAME);
//	if (versionDriver != NULL)
//	{
//		if (versionDriver()<IDriver::InterfaceVersion)
//			throw EDruOpenglDriverOldVersion();
//		else if (versionDriver()>IDriver::InterfaceVersion)
//			throw EDruOpenglDriverUnknownVersion();
//	}
//
//#else // NL_OS_UNIX
//#error "Dynamic DLL loading not implemented!"
//#endif // NL_OS_UNIX

	IDriver		*ret= createDriver();
	if (ret == NULL)
	{
		throw EDruOpenglDriverCantCreateDriver();
	}
	return ret;

#endif
}

// ***************************************************************************

#ifdef NL_OS_WINDOWS

IDriver		*CDRU::createD3DDriver() throw (EDru)
{
#ifdef NL_STATIC

	return createD3DDriverInstance ();

#else

	IDRV_CREATE_PROC	createDriver = NULL;
	IDRV_VERSION_PROC	versionDriver = NULL;

	// WINDOWS code.
//	HINSTANCE			hInst;

//	hInst=LoadLibrary(NL3D_D3D_DLL_NAME);

	CLibrary driverLib;

//	if (!hInst)
	if (!driverLib.loadLibrary(NL3D_D3D_DLL_NAME, true, true, false))
	{
		throw EDruDirect3dDriverNotFound();
	}

//	char buffer[1024], *ptr;
//	SearchPath (NULL, NL3D_D3D_DLL_NAME, NULL, 1023, buffer, &ptr);
	nlinfo ("Using the library '"NL3D_D3D_DLL_NAME"' that is in the directory: '%s'", driverLib.getLibFileName().c_str());

//	createDriver = (IDRV_CREATE_PROC) GetProcAddress (hInst, IDRV_CREATE_PROC_NAME);
	createDriver = (IDRV_CREATE_PROC) driverLib.getSymbolAddress(IDRV_CREATE_PROC_NAME);
	if (createDriver == NULL)
	{
		throw EDruDirect3dDriverCorrupted();
	}

//	versionDriver = (IDRV_VERSION_PROC) GetProcAddress (hInst, IDRV_VERSION_PROC_NAME);
	versionDriver = (IDRV_VERSION_PROC) driverLib.getSymbolAddress(IDRV_VERSION_PROC_NAME);
	if (versionDriver != NULL)
	{
		if (versionDriver()<IDriver::InterfaceVersion)
			throw EDruDirect3dDriverOldVersion();
		else if (versionDriver()>IDriver::InterfaceVersion)
			throw EDruDirect3dDriverUnknownVersion();
	}

	IDriver		*ret= createDriver();
	if (ret == NULL)
	{
		throw EDruDirect3dDriverCantCreateDriver();
	}
	return ret;

#endif
}
#endif // NL_OS_WINDOWS

// ***************************************************************************

void	CDRU::drawBitmap (float x, float y, float width, float height, ITexture& texture, IDriver& driver, CViewport viewport, bool blend)
{
	CMatrix mtx;
	mtx.identity();
	driver.setupViewport (viewport);
	driver.setupViewMatrix (mtx);
	driver.setupModelMatrix (mtx);
	driver.setFrustum (0.f, 1.f, 0.f, 1.f, -1.f, 1.f, false);

	static CMaterial mat;
	mat.initUnlit ();
	mat.setTexture (0, &texture);
	mat.setBlend(blend);
	mat.setZFunc(CMaterial::always);

	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawBitmap");
	vb.setVertexFormat (CVertexBuffer::PositionFlag|CVertexBuffer::TexCoord0Flag);
	vb.setNumVertices (4);
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		vba.setVertexCoord (0, CVector (x, 0, y));
		vba.setVertexCoord (1, CVector (x+width, 0, y));
		vba.setVertexCoord (2, CVector (x+width, 0, y+height));
		vba.setVertexCoord (3, CVector (x, 0, y+height));
		vba.setTexCoord (0, 0, 0.f, 1.f);
		vba.setTexCoord (1, 0, 1.f, 1.f);
		vba.setTexCoord (2, 0, 1.f, 0.f);
		vba.setTexCoord (3, 0, 0.f, 0.f);
	}
	driver.activeVertexBuffer(vb);

	static CIndexBuffer pb;
	if (pb.getName().empty()) NL_SET_IB_NAME(pb, "CDRU::drawBitmap");
	pb.setFormat(NL_DEFAULT_INDEX_BUFFER_FORMAT);
	pb.setNumIndexes (6);
	{
		CIndexBufferReadWrite iba;
		pb.lock (iba);
		iba.setTri (0, 0, 1, 2);
		iba.setTri (3, 2, 3, 0);
	}

	driver.activeIndexBuffer(pb);
	driver.renderTriangles(mat, 0, 2);
}


// ***************************************************************************
void	CDRU::drawLine (float x0, float y0, float x1, float y1, IDriver& driver, CRGBA col, CViewport viewport)
{
	CMatrix mtx;
	mtx.identity();
	driver.setupViewport (viewport);
	driver.setupViewMatrix (mtx);
	driver.setupModelMatrix (mtx);
	driver.setFrustum (0.f, 1.f, 0.f, 1.f, -1.f, 1.f, false);

	static CMaterial mat;
	mat.initUnlit ();
	mat.setSrcBlend(CMaterial::srcalpha);
	mat.setDstBlend(CMaterial::invsrcalpha);
	mat.setBlend(true);
	mat.setColor(col);
	mat.setZFunc (CMaterial::always);

	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawLine");
	vb.setVertexFormat (CVertexBuffer::PositionFlag);
	vb.setNumVertices (2);
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		vba.setVertexCoord (0, CVector (x0, 0, y0));
		vba.setVertexCoord (1, CVector (x1, 0, y1));
	}
	driver.activeVertexBuffer(vb);

	static CIndexBuffer pb;
	if (pb.getName().empty()) NL_SET_IB_NAME(pb, "CDRU::drawLine");
	pb.setFormat(NL_DEFAULT_INDEX_BUFFER_FORMAT);
	pb.setNumIndexes (2);
	{
		CIndexBufferReadWrite iba;
		pb.lock (iba);
		iba.setLine (0, 0, 1);
	}

	driver.activeIndexBuffer(pb);
	driver.renderLines(mat, 0, 1);
}


// ***************************************************************************
void	CDRU::drawTriangle (float x0, float y0, float x1, float y1, float x2, float y2, IDriver& driver, CRGBA col, CViewport viewport)
{
	CMatrix mtx;
	mtx.identity();
	driver.setupViewport (viewport);
	driver.setupViewMatrix (mtx);
	driver.setupModelMatrix (mtx);
	driver.setFrustum (0.f, 1.f, 0.f, 1.f, -1.f, 1.f, false);

	static CMaterial mat;
	mat.initUnlit();
	mat.setSrcBlend(CMaterial::srcalpha);
	mat.setDstBlend(CMaterial::invsrcalpha);
	mat.setBlend(true);
	mat.setColor(col);
	mat.setZFunc (CMaterial::always);

	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawTriangle");
	vb.setVertexFormat (CVertexBuffer::PositionFlag);
	vb.setNumVertices (3);
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		vba.setVertexCoord (0, CVector (x0, 0, y0));
		vba.setVertexCoord (1, CVector (x1, 0, y1));
		vba.setVertexCoord (2, CVector (x2, 0, y2));
	}
	driver.activeVertexBuffer(vb);

	static CIndexBuffer pb;
	if (pb.getName().empty()) NL_SET_IB_NAME(pb, "CDRU::drawTriangle");
	pb.setFormat(NL_DEFAULT_INDEX_BUFFER_FORMAT);
	pb.setNumIndexes (3);
	{
		CIndexBufferReadWrite iba;
		pb.lock (iba);
		iba.setTri (0, 0, 1, 2);
	}

	driver.activeIndexBuffer(pb);
	driver.renderTriangles(mat, 0, 1);
}



// ***************************************************************************
void	CDRU::drawQuad (float x0, float y0, float x1, float y1, IDriver& driver, CRGBA col, CViewport viewport)
{
	CMatrix mtx;
	mtx.identity();
	driver.setupViewport (viewport);
	driver.setupViewMatrix (mtx);
	driver.setupModelMatrix (mtx);
	driver.setFrustum (0.f, 1.f, 0.f, 1.f, -1.f, 1.f, false);

	static CMaterial mat;
	mat.initUnlit();
	mat.setSrcBlend(CMaterial::srcalpha);
	mat.setDstBlend(CMaterial::invsrcalpha);
	mat.setBlend(true);
	mat.setColor(col);
	mat.setZFunc (CMaterial::always);

	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawQuad");
	vb.setVertexFormat (CVertexBuffer::PositionFlag);
	vb.setNumVertices (4);
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		vba.setVertexCoord (0, CVector (x0, 0, y0));
		vba.setVertexCoord (1, CVector (x1, 0, y0));
		vba.setVertexCoord (2, CVector (x1, 0, y1));
		vba.setVertexCoord (3, CVector (x0, 0, y1));
	}

	driver.activeVertexBuffer(vb);
	driver.renderRawQuads(mat, 0, 1);
}


// ***************************************************************************
void	CDRU::drawQuad (float xcenter, float ycenter, float radius, IDriver& driver, CRGBA col, CViewport viewport)
{
	CMatrix mtx;
	mtx.identity();
	driver.setupViewport (viewport);
	driver.setupViewMatrix (mtx);
	driver.setupModelMatrix (mtx);
	driver.setFrustum (0.f, 1.f, 0.f, 1.f, -1.f, 1.f, false);

	static CMaterial mat;
	mat.initUnlit();
	mat.setSrcBlend(CMaterial::srcalpha);
	mat.setDstBlend(CMaterial::invsrcalpha);
	mat.setBlend(true);
	mat.setColor(col);
	mat.setZFunc (CMaterial::always);

	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawQuad");
	vb.setVertexFormat (CVertexBuffer::PositionFlag);
	vb.setNumVertices (4);
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		vba.setVertexCoord (0, CVector (xcenter-radius, 0, ycenter-radius));
		vba.setVertexCoord (1, CVector (xcenter+radius, 0, ycenter-radius));
		vba.setVertexCoord (2, CVector (xcenter+radius, 0, ycenter+radius));
		vba.setVertexCoord (3, CVector (xcenter-radius, 0, ycenter+radius));
	}

	driver.activeVertexBuffer(vb);
	driver.renderRawQuads(mat, 0, 1);
}


// ***************************************************************************
void	CDRU::drawWiredQuad (float x0, float y0, float x1, float y1, IDriver& driver, CRGBA col, CViewport viewport)
{
	// v-left
	CDRU::drawLine(x0,y0,x0,y1 ,driver,col,viewport);
	// v-right
	CDRU::drawLine(x1,y0,x1,y1 ,driver,col,viewport);
	// h-up
	CDRU::drawLine(x0,y1,x1,y1,driver,col,viewport);
	// h-bottom
	CDRU::drawLine(x0,y0,x1,y0,driver,col,viewport);
}


// ***************************************************************************
void	CDRU::drawWiredQuad (float xcenter, float ycenter, float radius, IDriver& driver, CRGBA col, CViewport viewport)
{
	// v-left
	CDRU::drawLine(xcenter-radius,ycenter-radius,xcenter-radius,ycenter+radius,driver,col,viewport);
	// v-right
	CDRU::drawLine(xcenter+radius,ycenter-radius,xcenter+radius,ycenter+radius,driver,col,viewport);
	// h-up
	CDRU::drawLine(xcenter-radius,ycenter+radius,xcenter+radius,ycenter+radius,driver,col,viewport);
	// h-bottom
	CDRU::drawLine(xcenter-radius,ycenter-radius,xcenter+radius,ycenter-radius,driver,col,viewport);
}


// ***************************************************************************
void			CDRU::drawTrianglesUnlit(const NLMISC::CTriangleUV	*trilist, sint ntris, CMaterial &mat, IDriver& driver)
{
	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawTrianglesUnlit");
	vb.setVertexFormat (CVertexBuffer::PositionFlag | CVertexBuffer::TexCoord0Flag);
	vb.setNumVertices (ntris*3);

	static	CIndexBuffer pb;
	pb.setFormat(NL_DEFAULT_INDEX_BUFFER_FORMAT);
	pb.setNumIndexes(ntris*3);
	if (pb.getFormat() == CIndexBuffer::Indices16)
	{
		nlassert(ntris * 3 <= 0xffff);
	}
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		CIndexBufferReadWrite iba;
		pb.lock (iba);
		for(sint i=0;i<ntris;i++)
		{
			vba.setVertexCoord (i*3+0, trilist[i].V0);
			vba.setVertexCoord (i*3+1, trilist[i].V1);
			vba.setVertexCoord (i*3+2, trilist[i].V2);
			vba.setTexCoord (i*3+0, 0, trilist[i].Uv0);
			vba.setTexCoord (i*3+1, 0, trilist[i].Uv1);
			vba.setTexCoord (i*3+2, 0, trilist[i].Uv2);
			iba.setTri(i*3, i*3+0, i*3+1, i*3+2);
		}
	}

	driver.activeVertexBuffer(vb);
	driver.activeIndexBuffer(pb);
	driver.renderTriangles(mat, 0, ntris);
}


// ***************************************************************************
void			CDRU::drawTrianglesUnlit(const std::vector<NLMISC::CTriangleUV> &trilist, CMaterial &mat, IDriver& driver)
{
	if(trilist.size()==0)
		return;

	CDRU::drawTrianglesUnlit( &(*trilist.begin()), (uint)trilist.size(), mat, driver);
}


// ***************************************************************************
void			CDRU::drawLinesUnlit(const NLMISC::CLine	*linelist, sint nlines, CMaterial &mat, IDriver& driver)
{
	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawLinesUnlit");
	vb.setVertexFormat (CVertexBuffer::PositionFlag);
	vb.setNumVertices (nlines*2);

	static	CIndexBuffer pb;
	pb.setFormat(NL_DEFAULT_INDEX_BUFFER_FORMAT);
	pb.setNumIndexes(nlines*2);


	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		CIndexBufferReadWrite iba;
		pb.lock (iba);
		for(sint i=0;i<nlines;i++)
		{
			vba.setVertexCoord (i*2+0, linelist[i].V0);
			vba.setVertexCoord (i*2+1, linelist[i].V1);
			iba.setLine(i*2, i*2+0, i*2+1);
		}
	}

	driver.activeVertexBuffer(vb);
	driver.activeIndexBuffer(pb);
	driver.renderLines(mat, 0, nlines);
}
// ***************************************************************************
void			CDRU::drawLinesUnlit(const std::vector<NLMISC::CLine> &linelist, CMaterial &mat, IDriver& driver)
{
	if(linelist.size()==0)
		return;
	CDRU::drawLinesUnlit( &(*linelist.begin()), (sint)linelist.size(), mat, driver);
}
// ***************************************************************************
void			CDRU::drawLine(const CVector &a, const CVector &b, CRGBA color, IDriver& driver)
{
	static	NLMISC::CLine		line;
	static	CMaterial	mat;
	static	bool		inited= false;

	// Setup material.
	if(!inited)
	{
		inited= true;
		mat.initUnlit();
	}
	mat.setColor(color);


	line.V0= a;
	line.V1= b;
	CDRU::drawLinesUnlit(&line, 1, mat, driver);
}
// ***************************************************************************
void			CDRU::drawQuad (float x0, float y0, float x1, float y1, CRGBA col0, CRGBA col1, CRGBA col2, CRGBA col3, IDriver& driver, CViewport viewport)
{
	CMatrix mtx;
	mtx.identity();
	driver.setupViewport (viewport);
	driver.setupViewMatrix (mtx);
	driver.setupModelMatrix (mtx);
	driver.setFrustum (0.f, 1.f, 0.f, 1.f, -1.f, 1.f, false);

	static CMaterial mat;
	mat.initUnlit();
	mat.setSrcBlend(CMaterial::srcalpha);
	mat.setDstBlend(CMaterial::invsrcalpha);
	mat.setBlend(true);
	mat.setZFunc (CMaterial::always);

	static CVertexBuffer vb;
	if (vb.getName().empty()) vb.setName("CDRU::drawQuad");
	vb.setVertexFormat (CVertexBuffer::PositionFlag|CVertexBuffer::PrimaryColorFlag);
	vb.setNumVertices (4);
	{
		CVertexBufferReadWrite vba;
		vb.lock (vba);
		vba.setVertexCoord (0, CVector (x0, 0, y0));
		vba.setColor (0, col0);
		vba.setVertexCoord (1, CVector (x1, 0, y0));
		vba.setColor (1, col1);
		vba.setVertexCoord (2, CVector (x1, 0, y1));
		vba.setColor (2, col2);
		vba.setVertexCoord (3, CVector (x0, 0, y1));
		vba.setColor (3, col3);
	}

	driver.activeVertexBuffer(vb);
	driver.renderRawQuads(mat, 0, 1);
}

// ***************************************************************************
void			CDRU::drawWiredBox(const CVector &corner, const CVector &vi, const CVector &vj, const CVector &vk, CRGBA color, IDriver& drv)
{
	CVector		p0= corner;
	CVector		p1= p0 + vi + vj + vk;
	drawLine(p0, p0+vi, color, drv);
	drawLine(p0, p0+vj, color, drv);
	drawLine(p0, p0+vk, color, drv);
	drawLine(p1, p1-vi, color, drv);
	drawLine(p1, p1-vj, color, drv);
	drawLine(p1, p1-vk, color, drv);
	drawLine(p0+vi, p0+vi+vj, color, drv);
	drawLine(p0+vi, p0+vi+vk, color, drv);
	drawLine(p0+vj, p0+vj+vi, color, drv);
	drawLine(p0+vj, p0+vj+vk, color, drv);
	drawLine(p0+vk, p0+vk+vi, color, drv);
	drawLine(p0+vk, p0+vk+vj, color, drv);
}


} // NL3D
