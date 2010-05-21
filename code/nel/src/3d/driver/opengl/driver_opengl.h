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

#ifndef NL_DRIVER_OPENGL_H
#define NL_DRIVER_OPENGL_H


#include "nel/misc/types_nl.h"

//#define NL_PROFILE_DRIVER_OGL
#ifdef NL_PROFILE_DRIVER_OGL
#	define H_AUTO_OGL(label) H_AUTO(label)
#else
#	define H_AUTO_OGL(label)
#endif

#ifdef NL_OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <windows.h>
#else // NL_OS_UNIX
#	define GLX_GLXEXT_PROTOTYPES
#	include <GL/glx.h>
#	ifdef XF86VIDMODE
#		include <X11/extensions/xf86vmode.h>
#	endif //XF86VIDMODE
#endif // NL_OS_UNIX

#include <GL/gl.h>

#include "driver_opengl_extension.h"

#include "nel/3d/driver.h"
#include "nel/3d/material.h"
#include "nel/3d/shader.h"
#include "nel/3d/vertex_buffer.h"
#include "nel/misc/matrix.h"
#include "nel/misc/smart_ptr.h"
#include "nel/misc/rgba.h"
#include "nel/misc/event_emitter.h"
#include "nel/misc/bit_set.h"
#include "nel/misc/hierarchical_timer.h"
#include "nel/3d/ptr_set.h"
#include "nel/misc/heap_memory.h"
#include "nel/misc/event_emitter_multi.h"
#include "driver_opengl_states.h"
#include "nel/3d/texture_cube.h"
#include "nel/3d/vertex_program_parse.h"
#include "nel/3d/viewport.h"
#include "nel/3d/scissor.h"
#include "nel/3d/light.h"
#include "nel/misc/time_nl.h"
#include "nel/3d/occlusion_query.h"


#ifdef NL_OS_WINDOWS
#include "nel/misc/win_event_emitter.h"
#elif defined (NL_OS_UNIX)
#include "unix_event_emitter.h"
#endif // NL_OS_UNIX


// For optimisation consideration, allow 256 lightmaps at max.
#define	NL3D_DRV_MAX_LIGHTMAP		256


void displayGLError(GLenum error);

/*
#define CHECK_GL_ERROR { \
	GLenum error = glGetError(); \
	if (error != GL_NO_ERROR)\
	{\
		displayGLError(error);\
		nlassert(0);\
	}\
}
*/

#define UNSUPPORTED_INDEX_OFFSET_MSG "Unsupported by driver, check IDriver::supportIndexOffset."

namespace NL3D {

using NLMISC::CMatrix;
using NLMISC::CVector;


class	CDriverGL;
class	IVertexArrayRange;
class	IVertexBufferHardGL;
class   COcclusionQueryGL;

typedef std::list<COcclusionQueryGL *> TOcclusionQueryList;

// ***************************************************************************
class COcclusionQueryGL : public IOcclusionQuery
{
public:
	GLuint							ID;				// id of gl object
	NLMISC::CRefPtr<CDriverGL>		Driver;			// owner driver
	TOcclusionQueryList::iterator   Iterator;		// iterator in owner driver list of queries
	TOcclusionType					OcclusionType;  // current type of occlusion
	uint							VisibleCount;	// number of samples that passed the test
	// From IOcclusionQuery
	virtual void begin();
	virtual void end();
	virtual TOcclusionType getOcclusionType();
	virtual uint getVisibleCount();
};

// ***************************************************************************
class CTextureDrvInfosGL : public ITextureDrvInfos
{
public:
	/*
		ANY DATA ADDED HERE MUST BE SWAPPED IN swapTextureHandle() !!
	*/

	// The GL Id.
	GLuint					ID;
	// Is the internal format of the texture is a compressed one?
	bool					Compressed;
	// Is the internal format of the texture has mipmaps?
	bool					MipMap;

	// This is the computed size of what memory this texture take.
	uint32					TextureMemory;
	// This is the owner driver.
	CDriverGL				*_Driver;

	// enum to use for this texture (GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_NV..)
	GLenum					TextureMode;

	// FBO Id
	GLuint					FBOId;

	// depth stencil FBO id
	GLuint					DepthFBOId;
	GLuint					StencilFBOId;

	bool					InitFBO;
	bool					AttachDepthStencil;
	bool					UsePackedDepthStencil;

	// The current wrap modes assigned to the texture.
	ITexture::TWrapMode		WrapS;
	ITexture::TWrapMode		WrapT;
	ITexture::TMagFilter	MagFilter;
	ITexture::TMinFilter	MinFilter;

	// The gl id is auto created here.
	CTextureDrvInfosGL(IDriver *drv, ItTexDrvInfoPtrMap it, CDriverGL *drvGl, bool isRectangleTexture);
	// The gl id is auto deleted here.
	~CTextureDrvInfosGL();
	// For Debug info. return the memory cost of this texture
	virtual uint	getTextureMemoryUsed() const {return TextureMemory;}

	bool					initFrameBufferObject(ITexture * tex);
	bool					activeFrameBufferObject(ITexture * tex);
};


// ***************************************************************************
class CVBDrvInfosGL : public IVBDrvInfos
{
public:
	CVBDrvInfosGL(CDriverGL *drv, ItVBDrvInfoPtrList it, CVertexBuffer *vb);

	// Verex buffer hard ?
	IVertexBufferHardGL		*_VBHard;
	class CDriverGL			*_DriverGL;
	uint8					*_SystemMemory;


	// From IVBDrvInfos
	virtual ~CVBDrvInfosGL();
	virtual uint8	*lock (uint first, uint last, bool readOnly);
	virtual void	unlock (uint first, uint last);
};


// ***************************************************************************
class CShaderGL : public IMaterialDrvInfos
{
public:
	GLenum		SrcBlend;
	GLenum		DstBlend;
	GLenum		ZComp;

	GLfloat		Emissive[4];
	GLfloat		Ambient[4];
	GLfloat		Diffuse[4];
	GLfloat		Specular[4];
	// For fast comp.
	uint32		PackedEmissive;
	uint32		PackedAmbient;
	uint32		PackedDiffuse;
	uint32		PackedSpecular;

	// The supported Shader type.
	CMaterial::TShader	SupportedShader;

	CShaderGL(IDriver *drv, ItMatDrvInfoPtrList it) : IMaterialDrvInfos(drv, it) {}
};


// ***************************************************************************
/// Info for the last VertexBuffer setuped (iether normal or hard).
class	CVertexBufferInfo
{
public:
	uint16					VertexFormat;
	uint16					VertexSize;
	uint32					NumVertices;
	CVertexBuffer::TType	Type[CVertexBuffer::NumValue];
	uint8					UVRouting[CVertexBuffer::MaxStage];

	// NB: ptrs are invalid if VertexFormat does not support the compoennt. must test VertexFormat, not the ptr.
	void					*ValuePtr[CVertexBuffer::NumValue];


	enum TVBMode { TVBModeNone = 0, SysMem, HwNVIDIA, HwARB, HwATI }; // standard VBs, or Hard VBs using different extensions

	// Kind of vb
	TVBMode					VBMode;
	// the handle of ATI or ARB vertex object
	uint					VertexObjectId;

	CVertexBufferInfo()
	{
		VBMode = TVBModeNone;
	}

	void		setupVertexBuffer(CVertexBuffer &vb);
	void		setupVertexBufferHard(IVertexBufferHardGL &vb);
};



// ***************************************************************************
/// Info for the last IndexBuffer setuped (iether normal or hard).
class	CIndexBufferInfo
{
public:
	const void				*_Values;
	CIndexBuffer::TFormat    _Format;

	CIndexBufferInfo ();
	void					setupIndexBuffer(CIndexBuffer &vb);
};



// ***************************************************************************
class CDriverGL : public IDriver
{
public:

	// Some constants
	enum { MaxLight=8 };

							CDriverGL();
	virtual					~CDriverGL();

	virtual	bool			isLost() const { return false; } // there's no notion of 'lost device" in OpenGL

	virtual bool			init (uint windowIcon = 0, emptyProc exitFunc = 0);

	virtual void			disableHardwareVertexProgram();
	virtual void			disableHardwareVertexArrayAGP();
	virtual void			disableHardwareTextureShader();

	virtual bool			setDisplay(void* wnd, const GfxMode& mode, bool show, bool resizeable) throw(EBadDisplay);
	virtual bool			setMode(const GfxMode& mode);
	virtual bool			getModes(std::vector<GfxMode> &modes);
	virtual bool			getCurrentScreenMode(GfxMode &mode);
	virtual void			beginDialogMode();
	virtual void			endDialogMode();

	/// Set the title of the NeL window
	virtual void			setWindowTitle(const ucstring &title);

	/// Set the position of the NeL window
	virtual void			setWindowPos(uint32 x, uint32 y);

	/// Show or hide the NeL window
	virtual void			showWindow(bool show);

	virtual void*			getDisplay()
	{
#ifdef NL_OS_WINDOWS
		return (void*)_hWnd;
#else // NL_OS_WINDOWS
		return NULL;
#endif // NL_OS_WINDOWS
	}

	virtual uint32			getAvailableVertexAGPMemory ();
	virtual uint32			getAvailableVertexVRAMMemory ();

	virtual emptyProc		getWindowProc();

	virtual bool			activate();

	virtual	uint			getNbTextureStages() const;

	virtual bool			isTextureExist(const ITexture&tex);

	virtual NLMISC::IEventEmitter	*getEventEmitter() { return&_EventEmitter; };

	virtual bool			clear2D(CRGBA rgba);

	virtual bool			clearZBuffer(float zval=1);
	virtual bool			clearStencilBuffer(float stencilval=0);
	virtual void			setColorMask (bool bRed, bool bGreen, bool bBlue, bool bAlpha);
	virtual void			setDepthRange(float znear, float zfar);
	virtual	void			getDepthRange(float &znear, float &zfar) const;

	virtual bool			setupTexture (ITexture& tex);

	virtual bool			setupTextureEx (ITexture& tex, bool bUpload, bool &bAllUploaded, bool bMustRecreateSharedTexture= false);
	virtual bool			uploadTexture (ITexture& tex, NLMISC::CRect& rect, uint8 nNumMipMap);
	virtual bool			uploadTextureCube (ITexture& tex, NLMISC::CRect& rect, uint8 nNumMipMap, uint8 nNumFace);

	virtual void			forceDXTCCompression(bool dxtcComp);

	virtual void			forceTextureResize(uint divisor);

	virtual void			forceNativeFragmentPrograms(bool nativeOnly);

	/// Setup texture env functions. Used by setupMaterial
	void				setTextureEnvFunction(uint stage, CMaterial& mat);

	/// setup the texture matrix for a given number of stages (starting from 0)
	void      setupUserTextureMatrix(uint numStages, CMaterial& mat);

	/// disable all texture matrix
	void      disableUserTextureMatrix();

	/// For objects with caustics, setup the first texture (which actually is the one from the material)
	/*static inline void		setupCausticsFirstTex(const CMaterial &mat);

	/// For objects with caustics, setup the caustic texture itself
	static inline void		setupCausticsSecondTex(uint stage);*/

	virtual bool			setupMaterial(CMaterial& mat);

	virtual void			startSpecularBatch();
	virtual void			endSpecularBatch();

	virtual void			setFrustum(float left, float right, float bottom, float top, float znear, float zfar, bool perspective = true);
	virtual	void			setFrustumMatrix(CMatrix &frust);
	virtual	CMatrix			getFrustumMatrix();
	virtual float			getClipSpaceZMin() const { return -1.f; }

	virtual void			setupViewMatrix(const CMatrix& mtx);

	virtual void			setupViewMatrixEx(const CMatrix& mtx, const CVector &cameraPos);

	virtual void			setupModelMatrix(const CMatrix& mtx);

	virtual CMatrix			getViewMatrix() const;

	virtual bool			activeShader(CShader *shd);

	virtual	void			forceNormalize(bool normalize)
	{
		_ForceNormalize= normalize;
		// if ForceNormalize, must enable GLNormalize now.
		if(normalize)
			enableGlNormalize(true);
	}

	virtual	bool			isForceNormalize() const
	{
		return _ForceNormalize;
	}

	virtual void			getNumPerStageConstant(uint &lightedMaterial, uint &unlightedMaterial) const;

	virtual	bool			supportVertexBufferHard() const;

	virtual bool			supportVolatileVertexBuffer() const;

	virtual	bool			supportCloudRenderSinglePass() const;

	virtual bool			supportIndexOffset() const { return false; /* feature only supported in D3D for now */ }


	virtual	bool			slowUnlockVertexBufferHard() const;

	virtual	uint			getMaxVerticesByVertexBufferHard() const;

	virtual	bool			initVertexBufferHard(uint agpMem, uint vramMem);

	virtual bool			activeVertexBuffer(CVertexBuffer& VB);

	virtual bool			activeIndexBuffer(CIndexBuffer& IB);

	virtual	void			mapTextureStageToUV(uint stage, uint uv);

	virtual bool			renderLines(CMaterial& mat, uint32 firstIndex, uint32 nlines);
	virtual bool			renderTriangles(CMaterial& Mat, uint32 firstIndex, uint32 ntris);
	virtual bool			renderSimpleTriangles(uint32 firstTri, uint32 ntris);
	virtual bool			renderRawPoints(CMaterial& Mat, uint32 startIndex, uint32 numPoints);
	virtual bool			renderRawLines(CMaterial& Mat, uint32 startIndex, uint32 numLines);
	virtual bool			renderRawTriangles(CMaterial& Mat, uint32 startIndex, uint32 numTris);
	virtual bool			renderRawQuads(CMaterial& Mat, uint32 startIndex, uint32 numQuads);
	//
	virtual bool			renderLinesWithIndexOffset(CMaterial& /* mat */, uint32 /* firstIndex */, uint32 /* nlines */, uint /* indexOffset */) { nlassertex(0, (UNSUPPORTED_INDEX_OFFSET_MSG)); return false; }
	virtual bool			renderTrianglesWithIndexOffset(CMaterial& /* mat */, uint32 /* firstIndex */, uint32 /* ntris */, uint /* indexOffset */) { nlassertex(0, (UNSUPPORTED_INDEX_OFFSET_MSG)); return false; }
	virtual bool			renderSimpleTrianglesWithIndexOffset(uint32 /* firstIndex */, uint32 /* ntris */, uint /* indexOffset */) { nlassertex(0, (UNSUPPORTED_INDEX_OFFSET_MSG)); return false; }

	virtual bool			swapBuffers();

	virtual void			setSwapVBLInterval(uint interval);

	virtual uint			getSwapVBLInterval();

	virtual	void			profileRenderedPrimitives(CPrimitiveProfile &pIn, CPrimitiveProfile &pOut);

	virtual	uint32			profileAllocatedTextureMemory();

	virtual	uint32			profileSetupedMaterials() const;

	virtual	uint32			profileSetupedModelMatrix() const;

	void					enableUsedTextureMemorySum (bool enable);

	uint32					getUsedTextureMemory() const;

	virtual	void			startProfileVBHardLock();

	virtual	void			endProfileVBHardLock(std::vector<std::string> &result);

	virtual	void			profileVBHardAllocation(std::vector<std::string> &result);

	virtual	void			startProfileIBLock();

	virtual	void			endProfileIBLock(std::vector<std::string> &result);

	virtual	void			profileIBAllocation(std::vector<std::string> &result);

	virtual bool			release();

	virtual TMessageBoxId	systemMessageBox (const char* message, const char* title, TMessageBoxType type=okType, TMessageBoxIcon icon=noIcon);

	virtual void			setupScissor (const class CScissor& scissor);

	virtual void			setupViewport (const class CViewport& viewport);

	virtual	void			getViewport(CViewport &viewport);


	virtual uint32			getImplementationVersion () const
	{
		return ReleaseVersion;
	}

	virtual const char*		getDriverInformation ()
	{
		return "Opengl 1.2 NeL Driver";
	}

	virtual const char*		getVideocardInformation ();

	virtual bool			isActive ();

	virtual uint8			getBitPerPixel ();

	virtual void			showCursor (bool b);

	// between 0.0 and 1.0
	virtual void			setMousePos(float x, float y);

	virtual void			setCapture (bool b);

	virtual NLMISC::IMouseDevice			*enableLowLevelMouse(bool enable, bool exclusive);

	virtual NLMISC::IKeyboardDevice			*enableLowLevelKeyboard(bool enable);

	virtual NLMISC::IInputDeviceManager		*getLowLevelInputDeviceManager();

	virtual uint							 getDoubleClickDelay(bool hardwareMouse);

	virtual void			getWindowSize (uint32 &width, uint32 &height);

	virtual void			getWindowPos (uint32 &x, uint32 &y);

	virtual void			getBuffer (CBitmap &bitmap);

	virtual void			getZBuffer (std::vector<float>  &zbuffer);

	virtual void			getBufferPart (CBitmap &bitmap, NLMISC::CRect &rect);

	// copy the first texture in a second one of different dimensions
	virtual bool			stretchRect(ITexture * srcText, NLMISC::CRect &srcRect, ITexture * destText, NLMISC::CRect &destRect);

	// return true if driver support Bloom effect.
	virtual	bool			supportBloomEffect() const;

	// return true if driver support non-power of two textures
	virtual	bool			supportNonPowerOfTwoTextures() const;

	virtual bool			activeFrameBufferObject(ITexture * tex);

	virtual void			getZBufferPart (std::vector<float>  &zbuffer, NLMISC::CRect &rect);

	virtual bool			setRenderTarget (ITexture *tex, uint32 x, uint32 y, uint32 width, uint32 height,
												uint32 mipmapLevel, uint32 cubeFace);

	virtual bool			copyTargetToTexture (ITexture *tex, uint32 offsetx, uint32 offsety, uint32 x, uint32 y,
													uint32 width, uint32 height, uint32 mipmapLevel);

	virtual bool			getRenderTargetSize (uint32 &width, uint32 &height);


	virtual bool			fillBuffer (CBitmap &bitmap);

	virtual void			setPolygonMode (TPolygonMode mode);

	virtual uint			getMaxLight () const;

	virtual void			setLight (uint8 num, const CLight& light);

	virtual void			enableLight (uint8 num, bool enable=true);

	virtual void			setPerPixelLightingLight(CRGBA diffuse, CRGBA specular, float shininess);

	virtual void			setLightMapDynamicLight (bool enable, const CLight& light);

	virtual void			setAmbientColor (CRGBA color);

	/// \name Fog support.
	// @{
	virtual	bool			fogEnabled();
	virtual	void			enableFog(bool enable);
	/// setup fog parameters. fog must enabled to see result. start and end are in [0,1] range.
	virtual	void			setupFog(float start, float end, CRGBA color);
	virtual	float			getFogStart() const;
	virtual	float			getFogEnd() const;
	virtual	CRGBA			getFogColor() const;
	// @}

	/// \name texture addressing modes
	// @{
	virtual bool supportTextureShaders() const;

	virtual bool isWaterShaderSupported() const;

	virtual bool isTextureAddrModeSupported(CMaterial::TTexAddressingMode mode) const;

	virtual void setMatrix2DForTextureOffsetAddrMode(const uint stage, const float mat[4]);
	// @}

	/// \name EMBM support
	// @{
		virtual bool supportEMBM() const;
		virtual bool isEMBMSupportedAtStage(uint stage) const;
		virtual void setEMBMMatrix(const uint stage, const float mat[4]);
	// @}

	virtual bool supportPerPixelLighting(bool specular) const;


	/// \name Misc
	// @{
	virtual	bool			supportBlendConstantColor() const;
	virtual	void			setBlendConstantColor(NLMISC::CRGBA col);
	virtual	NLMISC::CRGBA	getBlendConstantColor() const;
	virtual bool			setMonitorColorProperties (const CMonitorColorProperties &properties);
	virtual	void			finish();
	virtual	void			flush();
	virtual	void			enablePolygonSmoothing(bool smooth);
	virtual	bool			isPolygonSmoothingEnabled() const;
	// @}


	virtual void			swapTextureHandle(ITexture &tex0, ITexture &tex1);

	virtual	uint			getTextureHandle(const ITexture&tex);

	/// \name Material multipass.
	/**	NB: setupMaterial() must be called before thoses methods.
	 *  NB: This is intended to be use with the rendering of simple primitives.
	 *  NB: Other render calls performs the needed setup automatically
	 */
	// @{
	/// init multipass for _CurrentMaterial. return number of pass required to render this material.
	virtual sint			beginMaterialMultiPass() { 	return beginMultiPass(); }
	/// active the ith pass of this material.
	virtual void			setupMaterialPass(uint pass) { 	setupPass(pass); }
	/// end multipass for this material.
	virtual void			endMaterialMultiPass() { 	endMultiPass(); }
	// @}

	/// Adaptor information
	virtual uint			getNumAdapter() const;
	virtual bool			getAdapter(uint adapter, CAdapter &desc) const;
	virtual bool			setAdapter(uint adapter);

	virtual CVertexBuffer::TVertexColorType getVertexColorFormat() const;

	// Bench
	virtual void startBench (bool wantStandardDeviation = false, bool quick = false, bool reset = true);
	virtual void endBench ();
	virtual void displayBench (class NLMISC::CLog *log);

	virtual bool			supportOcclusionQuery() const;
	virtual IOcclusionQuery *createOcclusionQuery();
	virtual void			deleteOcclusionQuery(IOcclusionQuery *oq);

	// Test whether this device supports the frame buffer object mecanism
	virtual bool			supportTextureRectangle() const;
	virtual bool			supportFrameBufferObject() const;
	virtual bool			supportPackedDepthStencil() const;

	virtual uint64			getSwapBufferCounter() const { return _SwapBufferCounter; }

	virtual void			setCullMode(TCullMode cullMode);
	virtual	TCullMode       getCullMode() const;

	virtual void			enableStencilTest(bool enable);
	virtual bool			isStencilTestEnabled() const;
	virtual void			stencilFunc(TStencilFunc stencilFunc, int ref, uint mask);
	virtual void			stencilOp(TStencilOp fail, TStencilOp zfail, TStencilOp zpass);
	virtual void			stencilMask(uint mask);



private:
	virtual class IVertexBufferHardGL	*createVertexBufferHard(uint size, uint numVertices, CVertexBuffer::TPreferredMemory vbType, CVertexBuffer *vb);
	friend class					CTextureDrvInfosGL;
	friend class					CVertexProgamDrvInfosGL;


private:
	// Version of the driver. Not the interface version!! Increment when implementation of the driver change.
	static const uint32		ReleaseVersion;

	bool						_FullScreen;
	bool						_OffScreen;
	uint						_Interval;

	sint32						_WindowWidth, _WindowHeight, _WindowX, _WindowY;

#ifdef NL_OS_WINDOWS

	friend static bool GlWndProc(CDriverGL *driver, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HWND						_hWnd;
	HDC							_hDC;
	PIXELFORMATDESCRIPTOR		_pfd;
    HGLRC						_hRC;
	static uint					_Registered;
	DEVMODE						_OldScreenMode;
	NLMISC::CEventEmitterMulti	_EventEmitter; // this can contains a win emitter and eventually a direct input emitter
	bool						_DestroyWindow;

	// Off-screen rendering in Dib section
	HPBUFFERARB					_PBuffer;

#elif defined (NL_OS_UNIX)

	Display						*dpy;
	GLXContext					ctx;
	Window						win;
	Cursor						cursor;
	NLMISC::CUnixEventEmitter	_EventEmitter;

#ifdef XF86VIDMODE
	int						_OldDotClock;   // old dotclock
	XF86VidModeModeLine		_OldScreenMode;	// old modeline
	int						_OldX, _OldY;   //Viewport settings
#endif //XF86VIDMODE

#endif // NL_OS_UNIX

	bool					_Initialized;

	/// \name Driver Caps.
	// @{
	// OpenGL extensions Extensions.
	CGlExtensions			_Extensions;
	// @}


	// Depth of the driver in Bit Per Pixel
	uint8					_Depth;

	// The forceNormalize() state.
	bool					_ForceNormalize;

	// To know if light setup has been changed from last render() ( any call to setupViewMatrix() or setLight() ).
	bool					_LightSetupDirty;

	// To know if the modelview matrix setup has been changed from last render() (any call to setupViewMatrix() / setupModelMatrix() ).
	bool					_ModelViewMatrixDirty;

	// To know if the projection matrix has been changed
	bool					_ProjMatDirty;

	// Mirror the gl projection matrix when _ProjMatDirty = false
	NLMISC::CMatrix			_GLProjMat;

	// Ored of _LightSetupDirty and _ModelViewMatrixDirty
	bool					_RenderSetupDirty;

	// Backup znear and zfar
	float					_OODeltaZ;

	// Current View matrix, in NEL basis. This is the parameter passed in setupViewMatrix*().
	CMatrix					_UserViewMtx;
	// Current (OpenGL basis) View matrix.
	// NB: if setuped with setupViewMatrixEx(), _ViewMtx.Pos()==(0,0,0)
	CMatrix					_ViewMtx;
	// Matrix used for specular
	CMatrix					_SpecularTexMtx;
	// Precision ZBuffer: The Current cameraPosition, to remove from each model Position.
	CVector					_PZBCameraPos;


	// Current computed (OpenGL basis) ModelView matrix.
	// NB: This matrix have already substracted the _PZBCameraPos
	// Hence this matrix represent the Exact eye-space basis (only _ViewMtx is a bit tricky).
	CMatrix					_ModelViewMatrix;

	// Fog.
	bool					_FogEnabled;
	float					_FogEnd, _FogStart;
	GLfloat					_CurrentFogColor[4];



	// current viewport and scissor
	CViewport				_CurrViewport;
	CScissor				_CurrScissor;

	// viewport before call to setRenderTarget, if BFO extension is supported
	CViewport				_OldViewport;

	bool					_RenderTargetFBO;


	// Num lights return by GL_MAX_LIGHTS
	uint						_MaxDriverLight;
	// real mirror of GL state
	uint						_LightMode[MaxLight];				// Light mode.
	CVector						_WorldLightPos[MaxLight];			// World position of the lights.
	CVector						_WorldLightDirection[MaxLight];		// World direction of the lights.
	bool						_LightDirty[MaxLight];				// Light that need a View position setup in refreshRenderSetup().
	// For Lightmap Dynamic Lighting
	CLight						_LightMapDynamicLight;
	bool						_LightMapDynamicLightEnabled;
	bool						_LightMapDynamicLightDirty;
	// this is the backup of standard lighting (cause GL states may be modified by Lightmap Dynamic Lighting)
	CLight						_UserLight0;
	bool						_UserLightEnable[MaxLight];

	//\name description of the per pixel light
	// @{
		void checkForPerPixelLightingSupport();
		bool						_SupportPerPixelShader;
		bool						_SupportPerPixelShaderNoSpec;
		float						_PPLExponent;
		NLMISC::CRGBA				_PPLightDiffuseColor;
		NLMISC::CRGBA				_PPLightSpecularColor;
	// @}



	/// \name Prec settings, for optimisation.
	// @{

	// Special Texture environnements.
	enum	CTexEnvSpecial {
		TexEnvSpecialDisabled= 0,
		TexEnvSpecialLightMap,
		TexEnvSpecialSpecularStage1,
		TexEnvSpecialSpecularStage1NoText,
		TexEnvSpecialPPLStage0,
		TexEnvSpecialPPLStage2,
		TexEnvSpecialCloudStage0,
		TexEnvSpecialCloudStage1,
	};

	// NB: CRefPtr are not used for mem/spped optimisation. setupMaterial() and setupTexture() reset those states.
	CMaterial*				_CurrentMaterial;
	CMaterial::TShader		_CurrentMaterialSupportedShader;

	/* NB : this pointers handles the caching of glBindTexture() and setTextureMode() calls.
	*/
	ITexture*				_CurrentTexture[IDRV_MAT_MAXTEXTURES];

	CTextureDrvInfosGL*		_CurrentTextureInfoGL[IDRV_MAT_MAXTEXTURES];
	CMaterial::CTexEnv		_CurrentTexEnv[IDRV_MAT_MAXTEXTURES];
	// Special Texture Environnement.
	CTexEnvSpecial			_CurrentTexEnvSpecial[IDRV_MAT_MAXTEXTURES];
	// Texture addressing mode
	GLenum					_CurrentTexAddrMode[IDRV_MAT_MAXTEXTURES];
	// Reset texture shaders to their initial state if they are used
	void					resetTextureShaders();
	/** set texture shaders from stage 0 to stage IDRV_MAT_MAXTEXTURES - 1
	  * textures are needed to setup the right kind of shader (cubic or 2d texture)
	  */
	void					setTextureShaders(const uint8 *addressingModes, const NLMISC::CSmartPtr<ITexture> *textures);
	// activation of texture shaders
	bool					_NVTextureShaderEnabled;
	// Which stages support EMBM
	bool					_StageSupportEMBM[IDRV_MAT_MAXTEXTURES];

	// Prec settings for material.
	CDriverGLStates			_DriverGLStates;
	// Optim: To not test change in Materials states if just texture has changed. Very useful for landscape.
	uint32					_MaterialAllTextureTouchedFlag;

	// @}

	bool					_CurrentGlNormalize;

private:
	// Get the proj matrix setupped in GL
	void					refreshProjMatrixFromGL();

	bool					setupVertexBuffer(CVertexBuffer& VB);
	// Activate Texture Environnement. Do it with caching.
	bool					activateTexture(uint stage, ITexture *tex);
	// NB: this test _CurrentTexEnv[] and _CurrentTexEnvSpecial[].
	void					activateTexEnvMode(uint stage, const CMaterial::CTexEnv  &env);
	void					activateTexEnvColor(uint stage, const CMaterial::CTexEnv  &env);
	// Force Activate Texture Environnement. no caching here. TexEnvSpecial is disabled.
	void					forceActivateTexEnvMode(uint stage, const CMaterial::CTexEnv  &env);
	void					activateTexEnvColor(uint stage, NLMISC::CRGBA col);
	void					forceActivateTexEnvColor(uint stage, NLMISC::CRGBA col)
	{
		static	const float	OO255= 1.0f/255;
		_CurrentTexEnv[stage].ConstantColor= col;
		// Setup the gl cte color.
		_DriverGLStates.activeTextureARB(stage);
		GLfloat		glcol[4];
		glcol[0]= col.R*OO255;
		glcol[1]= col.G*OO255;
		glcol[2]= col.B*OO255;
		glcol[3]= col.A*OO255;
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, glcol);
	}
	void					forceActivateTexEnvColor(uint stage, const CMaterial::CTexEnv  &env)
	{
		H_AUTO_OGL(CDriverGL_forceActivateTexEnvColor)
		forceActivateTexEnvColor(stage, env.ConstantColor);
	}


	/// nv texture shaders. Should be used only if this caps is present!
	void					enableNVTextureShader(bool enabled);
	// check nv texture shader consistency
	void			verifyNVTextureShaderConfig();

	// Called by doRefreshRenderSetup(). set _LightSetupDirty to false
	void					cleanLightSetup ();

	// According to extensions, retrieve GL tex format of the texture.
	GLint					getGlTextureFormat(ITexture& tex, bool &compressed);


	// Clip the wanted rectangle with window. return true if rect is not NULL.
	bool					clipRect(NLMISC::CRect &rect);

	// Copy the frame buffer to a texture
	void					copyFrameBufferToTexture(ITexture *tex,
		                                             uint32 level,
													 uint32 offsetx,
													 uint32 offsety,
													 uint32 x,
													 uint32 y,
													 uint32 width,
													 uint32 height,
													 uint   cubeFace = 0
													);
	// is this texture a rectangle texture ?
	virtual bool			isTextureRectangle(ITexture * tex) const;

	/// \name Material multipass.
	/**	NB: setupMaterial() must be called before thoses methods.
	 */
	// @{
	/// init multipass for _CurrentMaterial. return number of pass required to render this material.
	 sint			beginMultiPass();
	/// active the ith pass of this material.
	 void			setupPass(uint pass);
	/// end multipass for this material.
	 void			endMultiPass();
	// @}

	/// LastVB for UV setup.
	CVertexBufferInfo		_LastVB;
	CIndexBufferInfo		_LastIB;

	/// setup a texture stage with an UV from VB.
	void			setupUVPtr(uint stage, CVertexBufferInfo &VB, uint uvId);


	/// \name Lightmap.
	// @{
	void			computeLightMapInfos(const CMaterial &mat);
	sint			beginLightMapMultiPass();
	void			setupLightMapPass(uint pass);
	void			endLightMapMultiPass();

	/// Temp Variables computed in beginLightMapMultiPass(). Reused in setupLightMapPass().
	uint			_NLightMaps;
	uint			_NLightMapPerPass;
	uint			_NLightMapPass;
	bool			_LightMapNoMulAddFallBack;
	// This array is the LUT from lmapId in [0, _NLightMaps[, to original lightmap id in material.
	std::vector<uint>		_LightMapLUT;

	// last stage env.
	CMaterial::CTexEnv	_LightMapLastStageEnv;

	// Caching
	bool			_LastVertexSetupIsLightMap;
	sint8			_LightMapUVMap[IDRV_MAT_MAXTEXTURES];

	// restore std vertex Setup.
	void			resetLightMapVertexSetup();

	// @}

	/// \name Specular.
	// @{
	sint			beginSpecularMultiPass();
	void			setupSpecularPass(uint pass);
	void			endSpecularMultiPass();
	void			setupSpecularBegin();
	void			setupSpecularEnd();
	bool			_SpecularBatchOn;
	// @}


	/// \name Water
	// @{
	sint			beginWaterMultiPass();
	void			setupWaterPass(uint pass);
	void			endWaterMultiPass();
	// @}

	/// \name Per pixel lighting
	// @{
	// per pixel lighting with specular
	sint			beginPPLMultiPass();
	void			setupPPLPass(uint pass);
	void			endPPLMultiPass();

	// per pixel lighting, no specular
	sint			beginPPLNoSpecMultiPass();
	void			setupPPLNoSpecPass(uint pass);
	void			endPPLNoSpecMultiPass();

	typedef NLMISC::CSmartPtr<CTextureCube> TSPTextureCube;
	typedef std::vector<TSPTextureCube> TTexCubeVect;
	TTexCubeVect   _SpecularTextureCubes; // the cube maps used to compute the specular lighting.


	/// get (and if necessary, build) a cube map used for specular lighting. The range for exponent is limited, and only the best fit is used
	CTextureCube   *getSpecularCubeMap(uint exp);

	// get (and if necessary, build) the cube map used for diffuse lighting
	CTextureCube   *getDiffuseCubeMap() { return getSpecularCubeMap(1); }



	// @}

	/// \name Caustics
	// @{
	/*sint			beginCausticsMultiPass(const CMaterial &mat);
	void			setupCausticsPass(const CMaterial &mat, uint pass);
	void			endCausticsMultiPass(const CMaterial &mat);*/
	// @}

	/// \name Cloud Shader
	sint			beginCloudMultiPass ();
	void			setupCloudPass (uint pass);
	void			endCloudMultiPass ();
	// @}


	/// setup GL arrays, with a vb info.
	void			setupGlArrays(CVertexBufferInfo &vb);

	/// Tools fct used by setupGLArrays
	void			setupGlArraysStd(CVertexBufferInfo &vb);
	void			setupGlArraysForNVVertexProgram(CVertexBufferInfo &vb);
	void			setupGlArraysForARBVertexProgram(CVertexBufferInfo &vb);
	void			setupGlArraysForEXTVertexShader(CVertexBufferInfo &vb);
	void			toggleGlArraysForNVVertexProgram();
	void			toggleGlArraysForARBVertexProgram();
	void			toggleGlArraysForEXTVertexShader();

	/// Test/activate normalisation of normal.
	void			enableGlNormalize(bool normalize)
	{
		if(_CurrentGlNormalize!=normalize)
		{
			_CurrentGlNormalize= normalize;
			if(normalize)
				glEnable(GL_NORMALIZE);
			else
				glDisable(GL_NORMALIZE);
		}
	}

	// refresh matrixes and lights.
	void			refreshRenderSetup()
	{
		// check if something to change.
		if(_RenderSetupDirty)
		{
			doRefreshRenderSetup();
		}
	}
	void			doRefreshRenderSetup();

	void			setLightInternal(uint8 num, const CLight& light);
	void			enableLightInternal(uint8 num, bool enable);
	void			setupLightMapDynamicLighting(bool enable);


	/// \name VertexBufferHard
	// @{
	CPtrSet<IVertexBufferHardGL>	_VertexBufferHardSet;
	friend class					CVertexArrayRangeNVidia;
	friend class					CVertexBufferHardGLNVidia;
	friend class					CVertexArrayRangeATI;
	friend class					CVertexArrayRangeARB;
	friend class					CVertexBufferHardARB;
	friend class					CVertexBufferHardGLATI;
	friend class					CVertexArrayRangeMapObjectATI;
	friend class					CVertexBufferHardGLMapObjectATI;
	friend class					CVBDrvInfosGL;

	// The VertexArrayRange activated.
	IVertexArrayRange				*_CurrentVertexArrayRange;
	// The VertexBufferHardGL activated.
	IVertexBufferHardGL				*_CurrentVertexBufferHard;
	// current setup passed to nglVertexArrayRangeNV()
	void							*_NVCurrentVARPtr;
	uint32							_NVCurrentVARSize;

	// Info got from ATI or NVidia extension.
	bool							_SupportVBHard;
	bool							_SlowUnlockVBHard;
	uint32							_MaxVerticesByVBHard;

	// The AGP VertexArrayRange.
	IVertexArrayRange				*_AGPVertexArrayRange;
	// The VRAM VertexArrayRange.
	IVertexArrayRange				*_VRAMVertexArrayRange;


	void							resetVertexArrayRange();

	void							fenceOnCurVBHardIfNeeded(IVertexBufferHardGL *newVBHard);


	// @}


	/// \name Profiling
	// @{
	CPrimitiveProfile									_PrimitiveProfileIn;
	CPrimitiveProfile									_PrimitiveProfileOut;
	uint32												_AllocatedTextureMemory;
	uint32												_NbSetupMaterialCall;
	uint32												_NbSetupModelMatrixCall;
	bool												_SumTextureMemoryUsed;
	std::set<CTextureDrvInfosGL*>						_TextureUsed;
	uint							computeMipMapMemoryUsage(uint w, uint h, GLint glfmt) const;

	// VBHard Lock Profiling
	struct	CVBHardProfile
	{
		NLMISC::CRefPtr<CVertexBuffer>			VBHard;
		NLMISC::TTicks							AccumTime;
		// true if the VBHard was not always the same for the same chronogical place.
		bool									Change;
		CVBHardProfile()
		{
			AccumTime= 0;
			Change= false;
		}
	};
	// The Profiles in chronogical order.
	bool												_VBHardProfiling;
	std::vector<CVBHardProfile>							_VBHardProfiles;
	uint												_CurVBHardLockCount;
	uint												_NumVBHardProfileFrame;
	void							appendVBHardLockProfile(NLMISC::TTicks time, CVertexBuffer *vb);

	// @}

	/// \name Vertex program interface
	// @{

	bool			isVertexProgramSupported () const;
	bool			isVertexProgramEmulated () const;
	bool			activeVertexProgram (CVertexProgram *program);
	void			setConstant (uint index, float, float, float, float);
	void			setConstant (uint index, double, double, double, double);
	void			setConstant (uint indexStart, const NLMISC::CVector& value);
	void			setConstant (uint indexStart, const NLMISC::CVectorD& value);
	void			setConstant (uint index, uint num, const float *src);
	void			setConstant (uint index, uint num, const double *src);
	void			setConstantMatrix (uint index, IDriver::TMatrix matrix, IDriver::TTransform transform);
	void			setConstantFog (uint index);
	void			enableVertexProgramDoubleSidedColor(bool doubleSided);
	bool		    supportVertexProgramDoubleSidedColor() const;

	virtual	bool			supportMADOperator() const ;


	// @}

	/// \name Vertex program implementation
	// @{
		bool activeNVVertexProgram (CVertexProgram *program);
		bool activeARBVertexProgram (CVertexProgram *program);
		bool activeEXTVertexShader (CVertexProgram *program);
	//@}



	/// \fallback for material shaders
	// @{
		/// test whether the given shader is supported, and gives back a supported shader
		CMaterial::TShader	getSupportedShader(CMaterial::TShader shader);
	// @}

	bool			isVertexProgramEnabled () const
	{
		// Don't use glIsEnabled, too slow.
		return _VertexProgramEnabled;
	}

	// Track state of activeVertexProgram()
	bool							_VertexProgramEnabled;
	// Say if last setupGlArrays() was a VertexProgram setup.
	bool							_LastSetupGLArrayVertexProgram;

	// The last vertex program that was setupped
	NLMISC::CRefPtr<CVertexProgram> _LastSetuppedVP;

	bool							_ForceDXTCCompression;
	/// Divisor for textureResize (power).
	uint							_ForceTextureResizePower;
	// enforcement of native fragment program check
	bool							_ForceNativeFragmentPrograms;


	// user texture matrix
	NLMISC::CMatrix		_UserTexMat[IDRV_MAT_MAXTEXTURES];
	uint				_UserTexMatEnabled; // bitm ask for user texture coords
	//NLMISC::CMatrix		_UserTexMat[IDRV_MAT_MAXTEXTURES];

	// Static const
	static const uint NumCoordinatesType[CVertexBuffer::NumType];
	static const uint GLType[CVertexBuffer::NumType];
	static const uint GLVertexAttribIndex[CVertexBuffer::NumValue];
	static const bool GLTypeIsIntegral[CVertexBuffer::NumType];
	static const uint GLMatrix[IDriver::NumMatrix];
	static const uint GLTransform[IDriver::NumTransform];

	/// \name Caustics shaders
	// @{
		NLMISC::CSmartPtr<CTextureCube>	_CausticCubeMap; // a cube map used for the rendering of caustics
		static void initCausticCubeMap();
	// @}

	/// Same as getNbTextureStages(), but faster because inline, and not virtual!!
	uint			inlGetNumTextStages() const { return _Extensions.NbTextureStages; }


	NLMISC::CRGBA					_CurrentBlendConstantColor;

	/// \name EXTVertexShader specifics.
	// @{
			// Variants offset used for :
			// Secondary color
			// Fog Coords
			// Skin Weight
			// Palette Skin
			// This mean that they must have 4 components
			public:
				enum EEVSVariants { EVSSecondaryColorVariant = 0, EVSFogCoordsVariant = 1, EVSSkinWeightVariant = 2, EVSPaletteSkinVariant = 3, EVSNumVariants };
			private:
			// Handle for standard gl arrays
			GLuint _EVSPositionHandle;
			GLuint _EVSNormalHandle;
			GLuint _EVSColorHandle;
			GLuint _EVSTexHandle[8];
			// Handle of the first constant c[0]. In vertex program we have 96 constant c[0] .. c[95]
			GLuint _EVSConstantHandle;
			// number of constant
			static const uint _EVSNumConstant;
			//
			bool   setupEXTVertexShader(const CVPParser::TProgram &program, GLuint id, uint variants[EVSNumVariants], uint16 &usedInputRegisters);
			bool   setupARBVertexProgram (const CVPParser::TProgram &parsedProgram, GLuint id, bool &specularWritten);
			//
	// @}

	// init EMBM settings (set each stage to modify the next)
	void	initEMBM();



	// Monitor color parameters backup
#ifdef WIN32
	bool							_NeedToRestaureGammaRamp;
	uint16							_GammaRampBackuped[3*256];
#endif


	/// \fragment shaders
	// @{

			GLuint ATIWaterShaderHandleNoDiffuseMap; // water support on R200
			GLuint ATIWaterShaderHandle; // water support on R200
			GLuint ATICloudShaderHandle; // cloud support for R200 and more

			GLuint ARBWaterShader[4]; // water support on R300, NV30 & the like


			void   initFragmentShaders();
			void   deleteFragmentShaders();
			void   deleteARBFragmentPrograms();

			void   setupWaterPassR200(const CMaterial &mat);
			void   setupWaterPassARB(const CMaterial &mat);
			void   setupWaterPassNV20(const CMaterial &mat);
	// @}


	bool				_PolygonSmooth;

	// driver version for ATI hardware (0 if unknown)
	uint				_ATIDriverVersion;
	bool                _ATIFogRangeFixed;

	void		retrieveATIDriverVersion();

	/// \Render to texture
	// @{
	CSmartPtr<ITexture>		_TextureTarget;
	uint32					_TextureTargetLevel;
	uint32					_TextureTargetX;
	uint32					_TextureTargetY;
	uint32					_TextureTargetWidth;
	uint32					_TextureTargetHeight;
	bool					_TextureTargetUpload;
	uint					_TextureTargetCubeFace;
	// @}
	// misc
public:
	static GLenum NLCubeFaceToGLCubeFace[6];
	static CMaterial::CTexEnv	_TexEnvReplace;
	// occlusion query
	TOcclusionQueryList			_OcclusionQueryList;
	COcclusionQueryGL			*_CurrentOcclusionQuery;
protected:
	// is the window active ,
	bool					_WndActive;
	uint64					_SwapBufferCounter;
public:
	void incrementResetCounter() { ++_ResetCounter; }
	bool isWndActive() const { return _WndActive; }
	const IVertexBufferHardGL	*getCurrentVertexBufferHard() const { return _CurrentVertexBufferHard; }
	// For debug : dump list of mapped buffers
	#ifdef NL_DEBUG
		void dumpMappedBuffers();
	#endif

	emptyProc ExitFunc;

	// tmp for debug
	void checkTextureOn() const;
private:
	/** Bind a texture at stage 0 for the good texture mode(2d or cube)
	  * Parameters / part of the texture are ready to be changed in the gl after that
	  * _CurrentTexture & _CurrentTextureInfoGL are not modified !
	  */
	inline void bindTextureWithMode(ITexture &tex);
	/** Force to set clamp & wrap mode for the given texture
	  * Setup is done for texture currently bind to the gl, so calling bindTextureWithMode is necessary
	  */
	inline void setupTextureBasicParameters(ITexture &tex);

};

// ***************************************************************************
class CVertexProgamDrvInfosGL : public IVertexProgramDrvInfos
{
public:
	// The GL Id.
	GLuint					ID;

	// ARB_vertex_program specific -> must know if specular part is written
	bool					SpecularWritten;

	/**  EXTVertexShader specific
	  *  handle of allocated variants
	  */
	GLuint					Variants[CDriverGL::EVSNumVariants];
	/** EXTVertexShader specific
	  * Used input registers.
	  * This allow to activate only the gl arrays that are needed by a given shader.
	  */
	uint16					UsedVertexComponents;


	// The gl id is auto created here.
	CVertexProgamDrvInfosGL (CDriverGL *drv, ItVtxPrgDrvInfoPtrList it);
};

#ifdef NL_OS_MAC
	// Specific mac functions
	extern bool getMacModes(std::vector<GfxMode> &modes);
#endif

} // NL3D

#endif // NL_DRIVER_OPENGL_H
