// Ryzom - MMORPG Framework <http://dev.ryzom.com/projects/ryzom/>
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

#include "stdpch.h"

//#include <crtdbg.h>

// LibWWW
extern "C"
{
#include "WWWLib.h"			      /* Global Library Include file */
#include "WWWApp.h"
#include "WWWInit.h"
}

#include "../libwww.h"

#include "group_html.h"
#include "group_list.h"
#include "group_container.h"
#include "view_link.h"
#include "ctrl_scroll.h"
#include "ctrl_button.h"
#include "ctrl_text_button.h"
#include "action_handler.h"
#include "group_paragraph.h"
#include "group_editbox.h"
#include "interface_manager.h"
#include "view_bitmap.h"
#include "../actions.h"
#include "dbgroup_combo_box.h"
#include "lua_ihm.h"

#include "../time_client.h"
#include "nel/misc/i18n.h"
#include "nel/misc/md5.h"

using namespace std;
using namespace NLMISC;


// Default timeout to connect a server
#define DEFAULT_RYZOM_CONNECTION_TIMEOUT (10.0)

// Uncomment to see the log about image download
//#define LOG_DL 1

CGroupHTML *CGroupHTML::_ConnectingLock = NULL;
extern CActionsContext ActionsContext;

// Get an url and return the local filename with the path where the url image should be
string CGroupHTML::localImageName(const string &url)
{
	string dest = "cache/";
	dest += getMD5((uint8 *)url.c_str(), (uint32)url.size()).toString();
	dest += ".cache";
	return dest;
}

// Add a image download request in the multi_curl
void CGroupHTML::addImageDownload(const string &url, CViewBase *img)
{
	// Search if we are not already downloading this url.
	for(uint i = 0; i < Curls.size(); i++)
	{
		if(Curls[i].url == url)
		{
#ifdef LOG_DL
			nlwarning("already downloading '%s' img %p", url.c_str(), img);
#endif
			Curls[i].imgs.push_back(img);
			return;
		}
	}

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, true);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	string dest = localImageName(url)+".tmp";
#ifdef LOG_DL
	nlwarning("add to download '%s' dest '%s' img %p", url.c_str(), dest.c_str(), img);
#endif
	// create the local file
	if (NLMISC::CFile::fileExists(dest))
	{
		CFile::setRWAccess(dest);
		NLMISC::CFile::deleteFile(dest.c_str());
	}
	FILE *fp = fopen (dest.c_str(), "wb");
	if (fp == NULL)
	{
		nlwarning("Can't open file '%s' for writing: code=%d '%s'", dest.c_str (), errno, strerror(errno));
		return;
	}
	curl_easy_setopt(curl, CURLOPT_FILE, fp);

	curl_multi_add_handle(MultiCurl, curl);
	Curls.push_back(CImageDownload(curl, url, fp, img));
#ifdef LOG_DL
	nlwarning("adding handle %x, %d curls", curl, Curls.size());
#endif
	RunningCurls++;
}

// Call this evenly to check if an image in downloaded and then display it
void CGroupHTML::checkImageDownload()
{
	//nlassert(_CrtCheckMemory());

	if(RunningCurls == 0)
		return;

	int NewRunningCurls = 0;
	while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(MultiCurl, &NewRunningCurls))
	{
#ifdef LOG_DL
		nlwarning("more to do now %d - %d curls", NewRunningCurls, Curls.size());
#endif
	}
	if(NewRunningCurls < RunningCurls)
	{
		// some download are done, callback them
#ifdef LOG_DL
		nlwarning ("new %d old %d", NewRunningCurls, RunningCurls);
#endif
		// check msg
		CURLMsg *msg;
		int msgs_left;
		while ((msg = curl_multi_info_read(MultiCurl, &msgs_left))) {
			if (msg->msg == CURLMSG_DONE) {
				for (vector<CImageDownload>::iterator it=Curls.begin(); it<Curls.end(); it++)
				{
					if(msg->easy_handle == it->curl)
					{
						CURLcode res = msg->data.result;
						long r;
						curl_easy_getinfo(it->curl, CURLINFO_RESPONSE_CODE, &r);
						fclose(it->fp);

#ifdef LOG_DL
						nlwarning("transfer %x completed with status res %d r %d - %d curls", msg->easy_handle, res, r, Curls.size());
#endif
						curl_easy_cleanup(it->curl);

						string image = localImageName(it->url);

						if(CURLE_OK != res || r < 200 || r >= 300)
						{
							NLMISC::CFile::deleteFile((image+".tmp").c_str());
						}
						else
						{
							string finalUrl;
							CFile::moveFile(image.c_str(), (image+".tmp").c_str());
							if (lookupLocalFile (finalUrl, image.c_str(), false))
							{
								for(uint i = 0; i < it->imgs.size(); i++)
								{
									// don't display image that are not power of 2
									uint32 w, h;
									CBitmap::loadSize (image, w, h);
									if (w == 0 || h == 0 || !NLMISC::isPowerOf2(w) || !NLMISC::isPowerOf2(h))
										image.clear();

									CCtrlButton *btn = dynamic_cast<CCtrlButton*>(it->imgs[i]);
									if(btn)
									{
#ifdef LOG_DL
										nlwarning("refresh new downloading image %d button %p", i, it->imgs[i]);
#endif
										btn->setTexture (image);
										btn->setTexturePushed(image);
										btn->invalidateCoords();
										btn->invalidateContent();
										btn->resetInvalidCoords();
										btn->updateCoords();
										paragraphChange();
									}
									else
									{
										CViewBitmap *btm = dynamic_cast<CViewBitmap*>(it->imgs[i]);
										if(btm)
										{
#ifdef LOG_DL
											nlwarning("refresh new downloading image %d image %p", i, it->imgs[i]);
#endif
											btm->setTexture (image);
											btm->invalidateCoords();
											btm->invalidateContent();
											btm->resetInvalidCoords();
											btm->updateCoords();
											paragraphChange();
										}
									}
								}
							}
						}

						Curls.erase(it);
						break;
					}
				}
			}
		}
	}
	RunningCurls = NewRunningCurls;
}

void CGroupHTML::initImageDownload()
{
#ifdef LOG_DL
	nlwarning("Init Image Download");
#endif
	MultiCurl = curl_multi_init();
	RunningCurls = 0;
/*
// Get current flag
int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
// Turn on leak-checking bit
tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
// Set flag to the new value
_CrtSetDbgFlag( tmpFlag );
*/


	string pathName = "cache";
	if ( ! CFile::isExists( pathName ) )
		CFile::createDirectory( pathName );
}

void CGroupHTML::releaseImageDownload()
{
#ifdef LOG_DL
	nlwarning("Release Image Download");
#endif
	if(MultiCurl)
		curl_multi_cleanup(MultiCurl);
}

/*
void dolibcurltest()
{
	nlwarning("start libcurl test");

	initImageDownload();

	addImageDownload("http://www.ryzom.com/en/");
	addImageDownload("http://www.ryzom.com/fr/");
	addImageDownload("http://www.ryzom.com/de/");

	do {
		checkImageDownload();
		nlwarning("continue to sleep");
		nlSleep(300);
	}
	while(RunningCurls);

	releaseImageDownload();

	nlwarning("end libcurl test");
}
*/


class CGroupListAdaptor : public CInterfaceGroup
{
public:
	CGroupListAdaptor(const TCtorParam &param)
		: CInterfaceGroup(param)
	{}

private:
	void updateCoords()
	{
		if (_Parent)
		{
			// Get the W max from the parent
			_W = std::min(_Parent->getMaxWReal(), _Parent->getWReal());
			_WReal = _W;
		}
		CInterfaceGroup::updateCoords();
	}
};

// ***************************************************************************

template<class A> void popIfNotEmpty(A &vect) { if(!vect.empty()) vect.pop_back(); }

// Data stored in CGroupHTML for libwww.

class CLibWWWData
{
public:
	CLibWWWData ()
	{
		Request = NULL;
		Anchor = NULL;
	}
	HTRequest	*Request;
	HTAnchor	*Anchor;
};

// ***************************************************************************

void CGroupHTML::beginBuild ()
{
	if (_Browsing)
	{
		nlassert (_Connecting);
		nlassert (_ConnectingLock == this);
		_Connecting = false;
		_ConnectingLock = NULL;

		removeContent ();
	}
}


// ***************************************************************************

void CGroupHTML::addText (const char * buf, int len)
{
	if (_Browsing)
	{
		// Build a UTF8 string
		string inputString(buf, buf+len);
//		inputString.resize (len);
//		uint i;
//		for (i=0; i<(uint)len; i++)
//			inputString[i] = buf[i];

		if (_ParsingLua)
		{
			// we are parsing a lua script
			_LuaScript += inputString;
			// no more to do
			return;
		}

		// Build a unicode string
		ucstring inputUCString;
		inputUCString.fromUtf8(inputString);

		// Build the final unicode string
		ucstring tmp;
		tmp.reserve(len);
		uint ucLen = (uint)inputUCString.size();
		//uint ucLenWithoutSpace = 0;
		for (uint i=0; i<ucLen; i++)
		{
			ucchar output;
			bool keep;
			// special treatment for 'nbsp' (which is returned as a discreet space)
			if (inputString.size() == 1 && inputString[0] == 32)
			{
				// this is a nbsp entity
				output = inputUCString[i];
				keep = true;
			}
			else
			{
				// not nbsp, use normal white space removal routine
				keep = translateChar (output, inputUCString[i], (tmp.empty())?0:tmp[tmp.size()-1]);
			}

			if (keep)
			{
				tmp.push_back(output);
/*
				// Break if the string is more than 50 chars long without space
				if (output != ucchar(' '))
				{
					ucLenWithoutSpace++;
					if (ucLenWithoutSpace == 50)
					{
						tmp.push_back(ucchar(' '));
						ucLenWithoutSpace = 0;
					}
				}
				else
				{
					ucLenWithoutSpace = 0;
				}
*/			}
		}

		if (!tmp.empty())
			addString(tmp);
	}
}

// ***************************************************************************

void CGroupHTML::addLink (uint element_number, uint /* attribute_number */, HTChildAnchor *anchor, const BOOL *present, const char **value)
{
	if (_Browsing)
	{
		if (element_number == HTML_A)
		{
			if (present[MY_HTML_A_HREF] && value[MY_HTML_A_HREF])
			{
				string suri = value[MY_HTML_A_HREF];
				if(suri.find("ah:") == 0)
				{
					// in ah: command we don't respect the uri standard so the HTAnchor_address doesn't work correctly
					_Link.push_back (suri);
				}
				else
				{
					HTAnchor * dest = HTAnchor_followMainLink((HTAnchor *) anchor);
					if (dest)
					{
						C3WSmartPtr uri = HTAnchor_address(dest);
						_Link.push_back ((const char*)uri);
					}
					else
					{
						_Link.push_back("");
					}
				}
			}
			else
				_Link.push_back("");
		}
	}
}

// ***************************************************************************

#define getCellsParameters(prefix,inherit) \
{\
	CGroupHTML::CCellParams cellParams; \
	if (!_CellParams.empty() && inherit) \
	{ \
		cellParams = _CellParams.back(); \
	} \
	if (present[prefix##_BGCOLOR] && value[prefix##_BGCOLOR]) \
		cellParams.BgColor = getColor (value[prefix##_BGCOLOR]); \
	if (present[prefix##_L_MARGIN] && value[prefix##_L_MARGIN]) \
		fromString(value[prefix##_L_MARGIN], cellParams.LeftMargin); \
	if (present[prefix##_NOWRAP]) \
		cellParams.NoWrap = true; \
	if (present[prefix##_ALIGN] && value[prefix##_ALIGN]) \
	{ \
		string align = value[prefix##_ALIGN]; \
		align = strlwr(align); \
		if (align == "left") \
			cellParams.Align = CGroupCell::Left; \
		if (align == "center") \
			cellParams.Align = CGroupCell::Center; \
		if (align == "right") \
			cellParams.Align = CGroupCell::Right; \
	} \
	if (present[prefix##_VALIGN] && value[prefix##_VALIGN]) \
	{ \
		string align = value[prefix##_VALIGN]; \
		align = strlwr(align); \
		if (align == "top") \
			cellParams.VAlign = CGroupCell::Top; \
		if (align == "middle") \
			cellParams.VAlign = CGroupCell::Middle; \
		if (align == "bottom") \
			cellParams.VAlign = CGroupCell::Bottom; \
	} \
	_CellParams.push_back (cellParams); \
}


static bool isHexa(char c)
{
	return isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f');
}

static uint8 convertHexa(char c)
{
	return (uint8) (tolower(c) - (isdigit(c) ? '0' : ('a' - 10)));
}

// scan a color component, and return pointer to next position
static const char *scanColorComponent(const char *src, uint8 &intensity)
{
	if (!src) return NULL;
	if (!isHexa(*src)) return NULL;
	uint8 value = convertHexa(*src++) << 4;
	if (!isHexa(*src)) return NULL;
	value += convertHexa(*src++);
	intensity = value;
	return src;
}


class CNameToCol
{
public:
	const char *Name;
	CRGBA Color;
	CNameToCol(const char *name, CRGBA color) : Name(name), Color(color) {}
};

static CNameToCol htmlColorNameToRGBA[] =
{
	CNameToCol("AliceBlue", CRGBA(0xF0, 0xF8, 0xFF)),
	CNameToCol("AntiqueWhite", CRGBA(0xFA, 0xEB, 0xD7)),
	CNameToCol("Aqua", CRGBA(0x00, 0xFF, 0xFF)),
	CNameToCol("Aquamarine", CRGBA(0x7F, 0xFF, 0xD4)),
	CNameToCol("Azure", CRGBA(0xF0, 0xFF, 0xFF)),
	CNameToCol("Beige", CRGBA(0xF5, 0xF5, 0xDC)),
	CNameToCol("Bisque", CRGBA(0xFF, 0xE4, 0xC4)),
	CNameToCol("Black", CRGBA(0x00, 0x00, 0x00)),
	CNameToCol("BlanchedAlmond", CRGBA(0xFF, 0xEB, 0xCD)),
	CNameToCol("Blue", CRGBA(0x00, 0x00, 0xFF)),
	CNameToCol("BlueViolet", CRGBA(0x8A, 0x2B, 0xE2)),
	CNameToCol("Brown", CRGBA(0xA5, 0x2A, 0x2A)),
	CNameToCol("BurlyWood", CRGBA(0xDE, 0xB8, 0x87)),
	CNameToCol("CadetBlue", CRGBA(0x5F, 0x9E, 0xA0)),
	CNameToCol("Chartreuse", CRGBA(0x7F, 0xFF, 0x00)),
	CNameToCol("Chocolate", CRGBA(0xD2, 0x69, 0x1E)),
	CNameToCol("Coral", CRGBA(0xFF, 0x7F, 0x50)),
	CNameToCol("CornflowerBlue", CRGBA(0x64, 0x95, 0xED)),
	CNameToCol("Cornsilk", CRGBA(0xFF, 0xF8, 0xDC)),
	CNameToCol("Crimson", CRGBA(0xDC, 0x14, 0x3C)),
	CNameToCol("Cyan", CRGBA(0x00, 0xFF, 0xFF)),
	CNameToCol("DarkBlue", CRGBA(0x00, 0x00, 0x8B)),
	CNameToCol("DarkCyan", CRGBA(0x00, 0x8B, 0x8B)),
	CNameToCol("DarkGoldenRod", CRGBA(0xB8, 0x86, 0x0B)),
	CNameToCol("DarkGray", CRGBA(0xA9, 0xA9, 0xA9)),
	CNameToCol("DarkGreen", CRGBA(0x00, 0x64, 0x00)),
	CNameToCol("DarkKhaki", CRGBA(0xBD, 0xB7, 0x6B)),
	CNameToCol("DarkMagenta", CRGBA(0x8B, 0x00, 0x8B)),
	CNameToCol("DarkOliveGreen", CRGBA(0x55, 0x6B, 0x2F)),
	CNameToCol("Darkorange", CRGBA(0xFF, 0x8C, 0x00)),
	CNameToCol("DarkOrchid", CRGBA(0x99, 0x32, 0xCC)),
	CNameToCol("DarkRed", CRGBA(0x8B, 0x00, 0x00)),
	CNameToCol("DarkSalmon", CRGBA(0xE9, 0x96, 0x7A)),
	CNameToCol("DarkSeaGreen", CRGBA(0x8F, 0xBC, 0x8F)),
	CNameToCol("DarkSlateBlue", CRGBA(0x48, 0x3D, 0x8B)),
	CNameToCol("DarkSlateGray", CRGBA(0x2F, 0x4F, 0x4F)),
	CNameToCol("DarkTurquoise", CRGBA(0x00, 0xCE, 0xD1)),
	CNameToCol("DarkViolet", CRGBA(0x94, 0x00, 0xD3)),
	CNameToCol("DeepPink", CRGBA(0xFF, 0x14, 0x93)),
	CNameToCol("DeepSkyBlue", CRGBA(0x00, 0xBF, 0xFF)),
	CNameToCol("DimGray", CRGBA(0x69, 0x69, 0x69)),
	CNameToCol("DodgerBlue", CRGBA(0x1E, 0x90, 0xFF)),
	CNameToCol("Feldspar", CRGBA(0xD1, 0x92, 0x75)),
	CNameToCol("FireBrick", CRGBA(0xB2, 0x22, 0x22)),
	CNameToCol("FloralWhite", CRGBA(0xFF, 0xFA, 0xF0)),
	CNameToCol("ForestGreen", CRGBA(0x22, 0x8B, 0x22)),
	CNameToCol("Fuchsia", CRGBA(0xFF, 0x00, 0xFF)),
	CNameToCol("Gainsboro", CRGBA(0xDC, 0xDC, 0xDC)),
	CNameToCol("GhostWhite", CRGBA(0xF8, 0xF8, 0xFF)),
	CNameToCol("Gold", CRGBA(0xFF, 0xD7, 0x00)),
	CNameToCol("GoldenRod", CRGBA(0xDA, 0xA5, 0x20)),
	CNameToCol("Gray", CRGBA(0x80, 0x80, 0x80)),
	CNameToCol("Green", CRGBA(0x00, 0x80, 0x00)),
	CNameToCol("GreenYellow", CRGBA(0xAD, 0xFF, 0x2F)),
	CNameToCol("HoneyDew", CRGBA(0xF0, 0xFF, 0xF0)),
	CNameToCol("HotPink", CRGBA(0xFF, 0x69, 0xB4)),
	CNameToCol("IndianRed ", CRGBA(0xCD, 0x5C, 0x5C)),
	CNameToCol("Indigo  ", CRGBA(0x4B, 0x00, 0x82)),
	CNameToCol("Ivory", CRGBA(0xFF, 0xFF, 0xF0)),
	CNameToCol("Khaki", CRGBA(0xF0, 0xE6, 0x8C)),
	CNameToCol("Lavender", CRGBA(0xE6, 0xE6, 0xFA)),
	CNameToCol("LavenderBlush", CRGBA(0xFF, 0xF0, 0xF5)),
	CNameToCol("LawnGreen", CRGBA(0x7C, 0xFC, 0x00)),
	CNameToCol("LemonChiffon", CRGBA(0xFF, 0xFA, 0xCD)),
	CNameToCol("LightBlue", CRGBA(0xAD, 0xD8, 0xE6)),
	CNameToCol("LightCoral", CRGBA(0xF0, 0x80, 0x80)),
	CNameToCol("LightCyan", CRGBA(0xE0, 0xFF, 0xFF)),
	CNameToCol("LightGoldenRodYellow", CRGBA(0xFA, 0xFA, 0xD2)),
	CNameToCol("LightGrey", CRGBA(0xD3, 0xD3, 0xD3)),
	CNameToCol("LightGreen", CRGBA(0x90, 0xEE, 0x90)),
	CNameToCol("LightPink", CRGBA(0xFF, 0xB6, 0xC1)),
	CNameToCol("LightSalmon", CRGBA(0xFF, 0xA0, 0x7A)),
	CNameToCol("LightSeaGreen", CRGBA(0x20, 0xB2, 0xAA)),
	CNameToCol("LightSkyBlue", CRGBA(0x87, 0xCE, 0xFA)),
	CNameToCol("LightSlateBlue", CRGBA(0x84, 0x70, 0xFF)),
	CNameToCol("LightSlateGray", CRGBA(0x77, 0x88, 0x99)),
	CNameToCol("LightSteelBlue", CRGBA(0xB0, 0xC4, 0xDE)),
	CNameToCol("LightYellow", CRGBA(0xFF, 0xFF, 0xE0)),
	CNameToCol("Lime", CRGBA(0x00, 0xFF, 0x00)),
	CNameToCol("LimeGreen", CRGBA(0x32, 0xCD, 0x32)),
	CNameToCol("Linen", CRGBA(0xFA, 0xF0, 0xE6)),
	CNameToCol("Magenta", CRGBA(0xFF, 0x00, 0xFF)),
	CNameToCol("Maroon", CRGBA(0x80, 0x00, 0x00)),
	CNameToCol("MediumAquaMarine", CRGBA(0x66, 0xCD, 0xAA)),
	CNameToCol("MediumBlue", CRGBA(0x00, 0x00, 0xCD)),
	CNameToCol("MediumOrchid", CRGBA(0xBA, 0x55, 0xD3)),
	CNameToCol("MediumPurple", CRGBA(0x93, 0x70, 0xD8)),
	CNameToCol("MediumSeaGreen", CRGBA(0x3C, 0xB3, 0x71)),
	CNameToCol("MediumSlateBlue", CRGBA(0x7B, 0x68, 0xEE)),
	CNameToCol("MediumSpringGreen", CRGBA(0x00, 0xFA, 0x9A)),
	CNameToCol("MediumTurquoise", CRGBA(0x48, 0xD1, 0xCC)),
	CNameToCol("MediumVioletRed", CRGBA(0xC7, 0x15, 0x85)),
	CNameToCol("MidnightBlue", CRGBA(0x19, 0x19, 0x70)),
	CNameToCol("MintCream", CRGBA(0xF5, 0xFF, 0xFA)),
	CNameToCol("MistyRose", CRGBA(0xFF, 0xE4, 0xE1)),
	CNameToCol("Moccasin", CRGBA(0xFF, 0xE4, 0xB5)),
	CNameToCol("NavajoWhite", CRGBA(0xFF, 0xDE, 0xAD)),
	CNameToCol("Navy", CRGBA(0x00, 0x00, 0x80)),
	CNameToCol("OldLace", CRGBA(0xFD, 0xF5, 0xE6)),
	CNameToCol("Olive", CRGBA(0x80, 0x80, 0x00)),
	CNameToCol("OliveDrab", CRGBA(0x6B, 0x8E, 0x23)),
	CNameToCol("Orange", CRGBA(0xFF, 0xA5, 0x00)),
	CNameToCol("OrangeRed", CRGBA(0xFF, 0x45, 0x00)),
	CNameToCol("Orchid", CRGBA(0xDA, 0x70, 0xD6)),
	CNameToCol("PaleGoldenRod", CRGBA(0xEE, 0xE8, 0xAA)),
	CNameToCol("PaleGreen", CRGBA(0x98, 0xFB, 0x98)),
	CNameToCol("PaleTurquoise", CRGBA(0xAF, 0xEE, 0xEE)),
	CNameToCol("PaleVioletRed", CRGBA(0xD8, 0x70, 0x93)),
	CNameToCol("PapayaWhip", CRGBA(0xFF, 0xEF, 0xD5)),
	CNameToCol("PeachPuff", CRGBA(0xFF, 0xDA, 0xB9)),
	CNameToCol("Peru", CRGBA(0xCD, 0x85, 0x3F)),
	CNameToCol("Pink", CRGBA(0xFF, 0xC0, 0xCB)),
	CNameToCol("Plum", CRGBA(0xDD, 0xA0, 0xDD)),
	CNameToCol("PowderBlue", CRGBA(0xB0, 0xE0, 0xE6)),
	CNameToCol("Purple", CRGBA(0x80, 0x00, 0x80)),
	CNameToCol("Red", CRGBA(0xFF, 0x00, 0x00)),
	CNameToCol("RosyBrown", CRGBA(0xBC, 0x8F, 0x8F)),
	CNameToCol("RoyalBlue", CRGBA(0x41, 0x69, 0xE1)),
	CNameToCol("SaddleBrown", CRGBA(0x8B, 0x45, 0x13)),
	CNameToCol("Salmon", CRGBA(0xFA, 0x80, 0x72)),
	CNameToCol("SandyBrown", CRGBA(0xF4, 0xA4, 0x60)),
	CNameToCol("SeaGreen", CRGBA(0x2E, 0x8B, 0x57)),
	CNameToCol("SeaShell", CRGBA(0xFF, 0xF5, 0xEE)),
	CNameToCol("Sienna", CRGBA(0xA0, 0x52, 0x2D)),
	CNameToCol("Silver", CRGBA(0xC0, 0xC0, 0xC0)),
	CNameToCol("SkyBlue", CRGBA(0x87, 0xCE, 0xEB)),
	CNameToCol("SlateBlue", CRGBA(0x6A, 0x5A, 0xCD)),
	CNameToCol("SlateGray", CRGBA(0x70, 0x80, 0x90)),
	CNameToCol("Snow", CRGBA(0xFF, 0xFA, 0xFA)),
	CNameToCol("SpringGreen", CRGBA(0x00, 0xFF, 0x7F)),
	CNameToCol("SteelBlue", CRGBA(0x46, 0x82, 0xB4)),
	CNameToCol("Tan", CRGBA(0xD2, 0xB4, 0x8C)),
	CNameToCol("Teal", CRGBA(0x00, 0x80, 0x80)),
	CNameToCol("Thistle", CRGBA(0xD8, 0xBF, 0xD8)),
	CNameToCol("Tomato", CRGBA(0xFF, 0x63, 0x47)),
	CNameToCol("Turquoise", CRGBA(0x40, 0xE0, 0xD0)),
	CNameToCol("Violet", CRGBA(0xEE, 0x82, 0xEE)),
	CNameToCol("VioletRed", CRGBA(0xD0, 0x20, 0x90)),
	CNameToCol("Wheat", CRGBA(0xF5, 0xDE, 0xB3)),
	CNameToCol("White", CRGBA(0xFF, 0xFF, 0xFF)),
	CNameToCol("WhiteSmoke", CRGBA(0xF5, 0xF5, 0xF5)),
	CNameToCol("Yellow", CRGBA(0xFF, 0xFF, 0x00)),
	CNameToCol("YellowGreen", CRGBA(0x9A, 0xCD, 0x32))
};

// scan a color from a HTML form (#rrggbb format)
bool scanHTMLColor(const char *src, CRGBA &dest)
{
	if (!src || *src == '\0') return false;
	if (*src == '#')
	{
		++src;
		CRGBA result;
		src = scanColorComponent(src, result.R); if (!src) return false;
		src = scanColorComponent(src, result.G); if (!src) return false;
		src = scanColorComponent(src, result.B); if (!src) return false;
		src = scanColorComponent(src, result.A);
		if (!src)
		{
			// Alpha is optionnal
			result.A = 255;
		}
		dest = result;
		return true;
	}
	else
	{
		// slow but should suffice for now
		for(uint k = 0; k < sizeofarray(htmlColorNameToRGBA); ++k)
		{
			if (nlstricmp(src, htmlColorNameToRGBA[k].Name) == 0)
			{
				dest = htmlColorNameToRGBA[k].Color;
				return true;
			}
		}
		return false;
	}
}

// ***************************************************************************

void CGroupHTML::beginElement (uint element_number, const BOOL *present, const char **value)
{
	if (_Browsing)
	{
		// Paragraph ?
		switch(element_number)
		{
		case HTML_A:
			_TextColor.push_back(LinkColor);
			_GlobalColor.push_back(LinkColorGlobalColor);
			_A.push_back(true);

			// Quick help
			if (present[MY_HTML_A_Z_ACTION_SHORTCUT] && value[MY_HTML_A_Z_ACTION_SHORTCUT])
			{
				// Get the action category
				string category;
				if (present[MY_HTML_A_Z_ACTION_CATEGORY] && value[MY_HTML_A_Z_ACTION_CATEGORY])
					category = value[MY_HTML_A_Z_ACTION_CATEGORY];

				// Get the action params
				string params;
				if (present[MY_HTML_A_Z_ACTION_PARAMS] && value[MY_HTML_A_Z_ACTION_PARAMS])
					params = value[MY_HTML_A_Z_ACTION_PARAMS];

				// Get the action descriptor
				CActionsManager *actionManager = ActionsContext.getActionsManager (category);
				if (actionManager)
				{
					const CActionsManager::TActionComboMap &actionCombo = actionManager->getActionComboMap ();
					CActionsManager::TActionComboMap::const_iterator ite = actionCombo.find (CAction::CName (value[MY_HTML_A_Z_ACTION_SHORTCUT], params.c_str()));
					if (ite != actionCombo.end())
					{
						addString (ite->second.toUCString());
					}
				}
			}
			break;
		case HTML_FONT:
		{
			bool found = false;
			if (present[HTML_FONT_COLOR] && value[HTML_FONT_COLOR])
			{
				CRGBA color;
				if (scanHTMLColor(value[HTML_FONT_COLOR], color))
				{
					_TextColor.push_back(color);
					found = true;
				}
			}
			if (!found)
			{
				_TextColor.push_back(_TextColor.empty() ? CRGBA::White : _TextColor.back());
			}
		}
		break;
		case HTML_BR:
			addString(ucstring ("\n"));
			break;
		case HTML_BODY:
			if (present[HTML_BODY_BGCOLOR] && value[HTML_BODY_BGCOLOR])
			{
				// Get the color
				CRGBA bgColor = getColor (value[HTML_BODY_BGCOLOR]);
				setBackgroundColor (bgColor);
			}
			break;
		case HTML_FORM:
			{
				// Build the form
				CGroupHTML::CForm form;

				// Get the action name
				if (present[HTML_FORM_ACTION] && value[HTML_FORM_ACTION])
				{
					HTParentAnchor *parent = HTAnchor_parent (_LibWWW->Anchor);
					HTChildAnchor *child = HTAnchor_findChildAndLink (parent, "", value[HTML_FORM_ACTION], NULL);
					if (child)
					{
						HTAnchor *mainChild = HTAnchor_followMainLink((HTAnchor *) child);
						if (mainChild)
						{
							C3WSmartPtr uri = HTAnchor_address(mainChild);
							form.Action = (const char*)uri;
						}
					}
					else
					{
						HTAnchor * dest = HTAnchor_findAddress (value[HTML_FORM_ACTION]);
						if (dest)
						{
							C3WSmartPtr uri = HTAnchor_address(dest);
							form.Action = (const char*)uri;
						}
						else
						{
							form.Action = value[HTML_FORM_ACTION];
						}
					}
				}
				_Forms.push_back(form);
			}
			break;
		case HTML_H1:
			newParagraph(PBeginSpace);
			_FontSize.push_back(H1FontSize);
			_TextColor.push_back(H1Color);
			_GlobalColor.push_back(H1ColorGlobalColor);
			break;
		case HTML_H2:
			newParagraph(PBeginSpace);
			_FontSize.push_back(H2FontSize);
			_TextColor.push_back(H2Color);
			_GlobalColor.push_back(H2ColorGlobalColor);
			break;
		case HTML_H3:
			newParagraph(PBeginSpace);
			_FontSize.push_back(H3FontSize);
			_TextColor.push_back(H3Color);
			_GlobalColor.push_back(H3ColorGlobalColor);
			break;
		case HTML_H4:
			newParagraph(PBeginSpace);
			_FontSize.push_back(H4FontSize);
			_TextColor.push_back(H4Color);
			_GlobalColor.push_back(H4ColorGlobalColor);
			break;
		case HTML_H5:
			newParagraph(PBeginSpace);
			_FontSize.push_back(H5FontSize);
			_TextColor.push_back(H5Color);
			_GlobalColor.push_back(H5ColorGlobalColor);
			break;
		case HTML_H6:
			newParagraph(PBeginSpace);
			_FontSize.push_back(H6FontSize);
			_TextColor.push_back(H6Color);
			_GlobalColor.push_back(H6ColorGlobalColor);
			break;
		case HTML_IMG:
			{
				// Get the string name
				if (present[MY_HTML_IMG_SRC] && value[MY_HTML_IMG_SRC])
				{
					// Get the global color name
					bool globalColor = false;
					if (present[MY_HTML_IMG_GLOBAL_COLOR])
						globalColor = true;

					if (getA() && getParent () && getParent ()->getParent())
					{
						// Tooltip
						const char *tooltip = NULL;
						if (present[MY_HTML_IMG_ALT] && value[MY_HTML_IMG_ALT])
							tooltip = value[MY_HTML_IMG_ALT];

						string params = "name=" + getId() + "|url=" + getLink ();
						addButton(CCtrlButton::PushButton, value[MY_HTML_IMG_SRC], value[MY_HTML_IMG_SRC], value[MY_HTML_IMG_SRC],
							"", globalColor, "browse", params.c_str(), tooltip);
					}
					else
					{
						addImage (value[MY_HTML_IMG_SRC], globalColor);
					}
				}
			}
			break;
		case HTML_INPUT:
			// Got one form ?
			if (!(_Forms.empty()))
			{
				// read general property
				string templateName;
				string minWidth;

				// Widget template name
				if (present[MY_HTML_INPUT_Z_BTN_TMPL] && value[MY_HTML_INPUT_Z_BTN_TMPL])
					templateName = value[MY_HTML_INPUT_Z_BTN_TMPL];
				// Input name is the new
				if (present[MY_HTML_INPUT_Z_INPUT_TMPL] && value[MY_HTML_INPUT_Z_INPUT_TMPL])
					templateName = value[MY_HTML_INPUT_Z_INPUT_TMPL];
				// Widget minimal width
				if (present[MY_HTML_INPUT_Z_INPUT_WIDTH] && value[MY_HTML_INPUT_Z_INPUT_WIDTH])
					minWidth = value[MY_HTML_INPUT_Z_INPUT_WIDTH];

				// Get the type
				if (present[MY_HTML_INPUT_TYPE] && value[MY_HTML_INPUT_TYPE])
				{
					// Global color flag
					bool globalColor = false;
					if (present[MY_HTML_INPUT_GLOBAL_COLOR])
						globalColor = true;

					// Tooltip
					const char *tooltip = NULL;
					if (present[MY_HTML_INPUT_ALT] && value[MY_HTML_INPUT_ALT])
						tooltip = value[MY_HTML_INPUT_ALT];

					string type = value[MY_HTML_INPUT_TYPE];
					type = strlwr (type);
					if (type == "image")
					{
						// The submit button
						string name;
						string normal;
						string pushed;
						string over;
						if (present[MY_HTML_INPUT_NAME] && value[MY_HTML_INPUT_NAME])
							name = value[MY_HTML_INPUT_NAME];
						if (present[MY_HTML_INPUT_SRC] && value[MY_HTML_INPUT_SRC])
							normal = value[MY_HTML_INPUT_SRC];

						// Action handler parameters : "name=group_html_id|form=id_of_the_form|submit_button=button_name"
						string param = "name=" + getId() + "|form=" + toString (_Forms.size()-1) + "|submit_button=" + name;

						// Add the ctrl button
						addButton (CCtrlButton::PushButton, name, normal, pushed.empty()?normal:pushed, over,
							globalColor, "html_submit_form", param.c_str(), tooltip);
					}
					if (type == "button" || type == "submit")
					{
						// The submit button
						string name;
						string text;
						string normal;
						string pushed;
						string over;

						string buttonTemplate(!templateName.empty() ? templateName : DefaultButtonGroup );
						if (present[MY_HTML_INPUT_NAME] && value[MY_HTML_INPUT_NAME])
							name = value[MY_HTML_INPUT_NAME];
						if (present[MY_HTML_INPUT_SRC] && value[MY_HTML_INPUT_SRC])
							normal = value[MY_HTML_INPUT_SRC];
						if (present[MY_HTML_INPUT_VALUE] && value[MY_HTML_INPUT_VALUE])
							text = value[MY_HTML_INPUT_VALUE];

						// Action handler parameters : "name=group_html_id|form=id_of_the_form|submit_button=button_name"
						string param = "name=" + getId() + "|form=" + toString (_Forms.size()-1) + "|submit_button=" + name;

						// Add the ctrl button
						if (!_Paragraph)
						{
							newParagraph (0);
							paragraphChange ();
						}

						CInterfaceManager *im = CInterfaceManager::getInstance();
						typedef pair<string, string> TTmplParam;
						vector<TTmplParam> tmplParams;
						tmplParams.push_back(TTmplParam("id", name));
						tmplParams.push_back(TTmplParam("onclick", "html_submit_form"));
						tmplParams.push_back(TTmplParam("onclick_param", param));
						//tmplParams.push_back(TTmplParam("text", text));
						tmplParams.push_back(TTmplParam("active", "true"));
						if (!minWidth.empty())
							tmplParams.push_back(TTmplParam("wmin", minWidth));
						CInterfaceGroup *buttonGroup = im->createGroupInstance(buttonTemplate, _Paragraph->getId(), tmplParams);
						if (buttonGroup)
						{

							// Add the ctrl button
							CCtrlTextButton *ctrlButton = dynamic_cast<CCtrlTextButton*>(buttonGroup->getCtrl("button"));
							if (!ctrlButton) ctrlButton = dynamic_cast<CCtrlTextButton*>(buttonGroup->getCtrl("b"));
							if (ctrlButton)
							{
								ctrlButton->setModulateGlobalColorAll (globalColor);

								// Translate the tooltip
								if (tooltip)
									ctrlButton->setDefaultContextHelp (CI18N::get (tooltip));

								ctrlButton->setText(ucstring::makeFromUtf8(text));
							}
							getParagraph()->addChild (buttonGroup);
							paragraphChange ();
						}

//						addButton (CCtrlTextButton::PushButton, name, normal, pushed.empty()?normal:pushed, over,
//							globalColor, "html_submit_form", param.c_str(), tooltip);
					}
					else if (type == "text")
					{
						// Get the string name
						string name;
						ucstring ucValue;
						uint size = 120;
						if (present[MY_HTML_INPUT_NAME] && value[MY_HTML_INPUT_NAME])
							name = value[MY_HTML_INPUT_NAME];
						if (present[MY_HTML_INPUT_SIZE] && value[MY_HTML_INPUT_SIZE])
							fromString(value[MY_HTML_INPUT_SIZE], size);
						if (present[MY_HTML_INPUT_VALUE] && value[MY_HTML_INPUT_VALUE])
							ucValue.fromUtf8(value[MY_HTML_INPUT_VALUE]);

						string textTemplate(!templateName.empty() ? templateName : DefaultFormTextGroup);
						// Add the editbox
						CInterfaceGroup *textArea = addTextArea (textTemplate, name.c_str (), 1, size/12, false, ucValue);
						if (textArea)
						{
							// Add the text area to the form
							CGroupHTML::CForm::CEntry entry;
							entry.Name = name;
							entry.TextArea = textArea;
							_Forms.back().Entries.push_back (entry);
						}
					}
					else if (type == "checkbox")
					{
						// The submit button
						string name;
						string normal = DefaultCheckBoxBitmapNormal;
						string pushed = DefaultCheckBoxBitmapPushed;
						string over = DefaultCheckBoxBitmapOver;
						bool checked = false;
						if (present[MY_HTML_INPUT_NAME] && value[MY_HTML_INPUT_NAME])
							name = value[MY_HTML_INPUT_NAME];
						if (present[MY_HTML_INPUT_SRC] && value[MY_HTML_INPUT_SRC])
							normal = value[MY_HTML_INPUT_SRC];
						checked = (present[MY_HTML_INPUT_CHECKED] && value[MY_HTML_INPUT_CHECKED]);

						// Add the ctrl button
						CCtrlButton *checkbox = addButton (CCtrlButton::ToggleButton, name, normal, pushed, over,
							globalColor, "", "", tooltip);
						if (checkbox)
						{
							checkbox->setPushed (checked);

							// Add the text area to the form
							CGroupHTML::CForm::CEntry entry;
							entry.Name = name;
							entry.Checkbox = checkbox;
							_Forms.back().Entries.push_back (entry);
						}
					}
					else if (type == "hidden")
					{
						if (present[MY_HTML_INPUT_NAME] && value[MY_HTML_INPUT_NAME])
						{
							// Get the name
							string name = value[MY_HTML_INPUT_NAME];

							// Get the value
							ucstring ucValue;
							if (present[MY_HTML_INPUT_VALUE] && value[MY_HTML_INPUT_VALUE])
								ucValue.fromUtf8(value[MY_HTML_INPUT_VALUE]);

							// Add an entry
							CGroupHTML::CForm::CEntry entry;
							entry.Name = name;
							entry.Value = decodeHTMLEntities(ucValue);
							_Forms.back().Entries.push_back (entry);
						}
					}
				}
			}
			break;
		case HTML_SELECT:
			if (!(_Forms.empty()))
			{
				// A select box

				// read general property
				string templateName;
				string minWidth;

				// Widget template name
				if (present[MY_HTML_INPUT_Z_INPUT_TMPL] && value[MY_HTML_INPUT_Z_INPUT_TMPL])
					templateName = value[MY_HTML_INPUT_Z_INPUT_TMPL];
				// Widget minimal width
				if (present[MY_HTML_INPUT_Z_INPUT_WIDTH] && value[MY_HTML_INPUT_Z_INPUT_WIDTH])
					minWidth = value[MY_HTML_INPUT_Z_INPUT_WIDTH];

				string name;
				if (present[HTML_SELECT_NAME] && value[HTML_SELECT_NAME])
					name = value[HTML_SELECT_NAME];

				string formTemplate = templateName.empty() ? DefaultFormSelectGroup : templateName;
				CDBGroupComboBox *cb = addComboBox(formTemplate, name.c_str());
				CGroupHTML::CForm::CEntry entry;
				entry.Name = name;
				entry.ComboBox = cb;
				_Forms.back().Entries.push_back (entry);
			}
		break;
		case HTML_OPTION:
			// Got one form ?
			if (!(_Forms.empty()))
			{
				if (!_Forms.back().Entries.empty())
				{
					// clear the option string
					_SelectOptionStr.clear();

					std::string optionValue;
					bool	 selected = false;
					if (present[HTML_OPTION_VALUE] && value[HTML_OPTION_VALUE])
						optionValue = value[HTML_OPTION_VALUE];
					if (present[HTML_OPTION_SELECTED] && value[HTML_OPTION_SELECTED])
						selected = nlstricmp(value[HTML_OPTION_SELECTED], "selected") == 0;
					_Forms.back().Entries.back().SelectValues.push_back(optionValue);
					if (selected)
					{
						_Forms.back().Entries.back().InitialSelection = (sint)_Forms.back().Entries.back().SelectValues.size() - 1;
					}

				}
			}
			_SelectOption = true;
		break;
		case HTML_LI:
			if (getUL())
			{
				// First LI ?
				if (!_LI)
				{
					_LI = true;
					newParagraph(ULBeginSpace);
				}
				else
				{
					newParagraph(LIBeginSpace);
				}
				ucstring str;
				str += (ucchar)0x2219;
				str += (ucchar)' ';
				addString (str);
				flushString ();
				getParagraph()->setFirstViewIndent(LIIndent);
			}
			break;
		case HTML_P:
			newParagraph(PBeginSpace);
			break;
		case HTML_PRE:
			_PRE.push_back(true);
			break;
		case HTML_TABLE:
			{
				// Get cells parameters
				getCellsParameters (MY_HTML_TABLE, false);

				CGroupTable *table = new CGroupTable(TCtorParam());
				table->BgColor = _CellParams.back().BgColor;

				if (present[MY_HTML_TABLE_WIDTH] && value[MY_HTML_TABLE_WIDTH])
					getPercentage (table->ForceWidthMin, table->TableRatio, value[MY_HTML_TABLE_WIDTH]);
				if (present[MY_HTML_TABLE_BORDER] && value[MY_HTML_TABLE_BORDER])
					fromString(value[MY_HTML_TABLE_BORDER], table->Border);
				if (present[MY_HTML_TABLE_CELLSPACING] && value[MY_HTML_TABLE_CELLSPACING])
					fromString(value[MY_HTML_TABLE_CELLSPACING], table->CellSpacing);
				if (present[MY_HTML_TABLE_CELLPADDING] && value[MY_HTML_TABLE_CELLPADDING])
					fromString(value[MY_HTML_TABLE_CELLPADDING], table->CellPadding);

				// Table must fit the container size

				addGroup (table, 0);

				_Tables.push_back(table);

				// Add a cell pointer
				_Cells.push_back(NULL);
				_TR.push_back(false);
			}
			break;
		case HTML_TD:
			{
				// Get cells parameters
				getCellsParameters (MY_HTML_TD, true);

				CGroupTable *table = getTable();
				if (table)
				{
					if (!_Cells.empty())
					{
						_Cells.back() = new CGroupCell(CViewBase::TCtorParam());

						// Set the cell parameters
						_Cells.back()->BgColor = _CellParams.back().BgColor;
						_Cells.back()->Align = _CellParams.back().Align;
						_Cells.back()->VAlign = _CellParams.back().VAlign;
						_Cells.back()->LeftMargin = _CellParams.back().LeftMargin;
						_Cells.back()->NoWrap = _CellParams.back().NoWrap;

						float temp;
						if (present[MY_HTML_TD_WIDTH] && value[MY_HTML_TD_WIDTH])
							getPercentage (_Cells.back()->WidthWanted, _Cells.back()->TableRatio, value[MY_HTML_TD_WIDTH]);
						if (present[MY_HTML_TD_HEIGHT] && value[MY_HTML_TD_HEIGHT])
							getPercentage (_Cells.back()->Height, temp, value[MY_HTML_TD_HEIGHT]);

						_Cells.back()->NewLine = getTR();
						table->addChild (_Cells.back());
						newParagraph(TDBeginSpace);

						// Reset TR flag
						if (!_TR.empty())
							_TR.back() = false;
					}
				}
			}
			break;
		case HTML_TEXTAREA:
			// Got one form ?
			if (!(_Forms.empty()))
			{
				// read general property
				string templateName;

				// Widget template name
				if (present[MY_HTML_TEXTAREA_Z_INPUT_TMPL] && value[MY_HTML_TEXTAREA_Z_INPUT_TMPL])
					templateName = value[MY_HTML_TEXTAREA_Z_INPUT_TMPL];

				// Get the string name
				_TextAreaName = "";
				_TextAreaRow = 1;
				_TextAreaCols = 10;
				_TextAreaContent = "";
				if (present[HTML_TEXTAREA_NAME] && value[HTML_TEXTAREA_NAME])
					_TextAreaName = value[HTML_TEXTAREA_NAME];
				if (present[HTML_TEXTAREA_ROWS] && value[HTML_TEXTAREA_ROWS])
					fromString(value[HTML_TEXTAREA_ROWS], _TextAreaRow);
				if (present[HTML_TEXTAREA_COLS] && value[HTML_TEXTAREA_COLS])
					fromString(value[HTML_TEXTAREA_COLS], _TextAreaCols);

				_TextAreaTemplate = !templateName.empty() ? templateName : DefaultFormTextAreaGroup;
				_TextArea = true;
			}
			break;
		case HTML_TITLE:
			{
				if(!_TitlePrefix.empty())
					_TitleString = _TitlePrefix + " - ";
				else
					_TitleString = "";
				_Title = true;
			}
			break;
		case HTML_I:
			{
				_Localize = true;
			}
			break;
		case HTML_TR:
			{
				// Get cells parameters
				getCellsParameters (MY_HTML_TR, true);

				// Set TR flag
				if (!_TR.empty())
					_TR.back() = true;
			}
			break;
		case HTML_UL:
			_Indent += ULIndent;
			_LI = false;
			endParagraph();
			_UL.push_back(true);
			break;
		}
	}
}

// ***************************************************************************

void CGroupHTML::endElement (uint element_number)
{
	if (_Browsing)
	{
		// Paragraph ?
		switch(element_number)
		{
		case HTML_FONT:
			popIfNotEmpty (_TextColor);
		break;
		case HTML_A:
			popIfNotEmpty (_TextColor);
			popIfNotEmpty (_GlobalColor);
			popIfNotEmpty (_A);
			popIfNotEmpty (_Link);
			break;
		case HTML_H1:
		case HTML_H2:
		case HTML_H3:
		case HTML_H4:
		case HTML_H5:
		case HTML_H6:
			popIfNotEmpty (_FontSize);
			popIfNotEmpty (_TextColor);
			popIfNotEmpty (_GlobalColor);
			endParagraph();
			break;
		case HTML_PRE:
			popIfNotEmpty (_PRE);
			break;
		case HTML_TABLE:
			popIfNotEmpty (_CellParams);
			popIfNotEmpty (_TR);
			popIfNotEmpty (_Cells);
			popIfNotEmpty (_Tables);
			endParagraph();
			// Add a cell
			break;
		case HTML_TD:
			popIfNotEmpty (_CellParams);
			if (!_Cells.empty())
				_Cells.back() = NULL;
			break;
		case HTML_TR:
			popIfNotEmpty (_CellParams);
			break;
		case HTML_TEXTAREA:
			{
				// Add the editbox
// 				nlinfo("textarea temp '%s'", _TextAreaTemplate.c_str());
// 				nlinfo("textarea name '%s'", _TextAreaName.c_str());
// 				nlinfo("textarea %d %d", _TextAreaRow, _TextAreaCols);
// 				nlinfo("textarea content '%s'", _TextAreaContent.toUtf8().c_str());
				CInterfaceGroup *textArea = addTextArea (_TextAreaTemplate, _TextAreaName.c_str (), _TextAreaRow, _TextAreaCols, true, _TextAreaContent);
				if (textArea)
				{
					// Add the text area to the form
					CGroupHTML::CForm::CEntry entry;
					entry.Name = _TextAreaName;
					entry.TextArea = textArea;
					_Forms.back().Entries.push_back (entry);
				}
				_TextArea = false;
			}
			break;
		case HTML_TITLE:
			{
				_Title = false;

				// Get the parent container
				setTitle (_TitleString);
			}
			break;
		case HTML_SELECT:
			{
				_SelectOption = false;
				if (!(_Forms.empty()))
				{
					if (!_Forms.back().Entries.empty())
					{
						CDBGroupComboBox *cb = _Forms.back().Entries.back().ComboBox;
						if (cb)
						{
							cb->setSelectionNoTrigger(_Forms.back().Entries.back().InitialSelection);
							cb->setW(cb->evalContentWidth() + 16);
						}
					}
				}
			}
			break;
		case HTML_OPTION:
			{
				// insert the parsed text into the select control
				CDBGroupComboBox *cb = _Forms.back().Entries.back().ComboBox;
				if (cb)
				{
					cb->addText(_SelectOptionStr);
				}
			}
			break;
		case HTML_I:
			{
				_Localize = false;
			}
			break;
		case HTML_UL:
			if (getUL())
			{
				_Indent -= ULIndent;
				_Indent = std::max(_Indent, (uint)0);
				endParagraph();
				popIfNotEmpty (_UL);
			}
			break;
		}
	}
}

// ***************************************************************************
void CGroupHTML::beginUnparsedElement(const char *buffer, int length)
{
	string str(buffer, buffer+length);
	if (stricmp(str.c_str(), "lua") == 0)
	{
		// we receive an embeded lua script
		_ParsingLua = true;
		_LuaScript = "";
	}
}

// ***************************************************************************
void CGroupHTML::endUnparsedElement(const char *buffer, int length)
{
	string str(buffer, buffer+length);
	if (stricmp(str.c_str(), "lua") == 0)
	{
		if (_ParsingLua)
		{
			_ParsingLua = false;
			// execute the embeded lua script
			CInterfaceManager *pIM = CInterfaceManager::getInstance();
			pIM->executeLuaScript(_LuaScript, true);
		}
	}
}


// ***************************************************************************
NLMISC_REGISTER_OBJECT(CViewBase, CGroupHTML, std::string, "html");


// ***************************************************************************
uint32							CGroupHTML::_GroupHtmlUIDPool= 0;
CGroupHTML::TGroupHtmlByUIDMap	CGroupHTML::_GroupHtmlByUID;


// ***************************************************************************
CGroupHTML::CGroupHTML(const TCtorParam &param)
:	CGroupScrollText(param),
	_TimeoutValue(DEFAULT_RYZOM_CONNECTION_TIMEOUT)
{
	// add it to map of group html created
	_GroupHtmlUID= ++_GroupHtmlUIDPool;		// valid assigned Id begin to 1!
	_GroupHtmlByUID[_GroupHtmlUID]= this;

	// init
	_ParsingLua = false;
	_BrowseNextTime = false;
	_PostNextTime = false;
	_Browsing = false;
	_Connecting = false;
	_LibWWW = new CLibWWWData;
	_CurrentViewLink = NULL;
	_CurrentViewImage = NULL;
	_Indent = 0;
	_LI = false;
	_SelectOption = false;
	_GroupListAdaptor = NULL;

	// Register
	CInterfaceManager::getInstance()->registerClockMsgTarget(this);

	// HTML parameters
	BgColor = CRGBA::Black;
	ErrorColor = CRGBA(255, 0, 0);
	LinkColor = CRGBA(0, 0, 255);
	TextColor = CRGBA(255, 255, 255);
	H1Color = CRGBA(255, 255, 255);
	H2Color = CRGBA(255, 255, 255);
	H3Color = CRGBA(255, 255, 255);
	H4Color = CRGBA(255, 255, 255);
	H5Color = CRGBA(255, 255, 255);
	H6Color = CRGBA(255, 255, 255);
	ErrorColorGlobalColor = false;
	LinkColorGlobalColor = false;
	TextColorGlobalColor = false;
	H1ColorGlobalColor = false;
	H2ColorGlobalColor = false;
	H3ColorGlobalColor = false;
	H4ColorGlobalColor = false;
	H5ColorGlobalColor = false;
	H6ColorGlobalColor = false;
	TextFontSize = 9;
	H1FontSize = 18;
	H2FontSize = 15;
	H3FontSize = 12;
	H4FontSize = 9;
	H5FontSize = 9;
	H6FontSize = 9;
	LIBeginSpace = 4;
	ULBeginSpace = 12;
	PBeginSpace	 = 12;
	TDBeginSpace = 0;
	LIIndent = -10;
	ULIndent = 30;
	LineSpaceFontFactor = 0.5f;
	DefaultButtonGroup =			"html_text_button";
	DefaultFormTextGroup =			"edit_box_widget";
	DefaultFormTextAreaGroup =		"edit_box_widget_multiline";
	DefaultFormSelectGroup =		"html_form_select_widget";
	DefaultCheckBoxBitmapNormal =	"checkbox_normal.tga";
	DefaultCheckBoxBitmapPushed =	"checkbox_pushed.tga";
	DefaultCheckBoxBitmapOver =		"checkbox_over.tga";
	DefaultBackgroundBitmapView =	"bg";
	clearContext();

	initImageDownload();
}

// ***************************************************************************

CGroupHTML::~CGroupHTML()
{
	//releaseImageDownload();

	// TestYoyo
	//nlinfo("** CGroupHTML Destroy: %x, %s, uid%d", this, _Id.c_str(), _GroupHtmlUID);

	/*	Erase from map of Group HTML (thus requestTerminated() callback won't be called)
		Do it first, just because don't want requestTerminated() to be called while I'm destroying
		(useless and may be dangerous)
	*/
	_GroupHtmlByUID.erase(_GroupHtmlUID);

	// stop browsing
	stopBrowse (); // NB : we don't call updateRefreshButton here, because :
				   // 1) it is useless,
	               // 2) it crashed before when it called getElementFromId (that didn't work when a master group was being removed...). Btw it should work now
	               //     this is why the call to 'updateRefreshButton' has been removed from stopBrowse

	clearContext();
	delete _LibWWW;
}

// ***************************************************************************

bool CGroupHTML::parse(xmlNodePtr cur,CInterfaceGroup *parentGroup)
{
	nlassert(CInterfaceManager::getInstance()->isClockMsgTarget(this));


	if(!CGroupScrollText::parse(cur, parentGroup))
		return false;

	// TestYoyo
	//nlinfo("** CGroupHTML parsed Ok: %x, %s, %s, uid%d", this, _Id.c_str(), typeid(this).name(), _GroupHtmlUID);

	CXMLAutoPtr ptr;

	// Get the url
	ptr = xmlGetProp (cur, (xmlChar*)"url");
	if (ptr)
		_URL = (const char*)ptr;

	// Bkup default for undo/redo
	_AskedUrl= _URL;

	ptr = xmlGetProp (cur, (xmlChar*)"title_prefix");
	if (ptr)
		_TitlePrefix = CI18N::get((const char*)ptr);

	// Parameters
	ptr = xmlGetProp (cur, (xmlChar*)"background_color");
	if (ptr)
		BgColor = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"error_color");
	if (ptr)
		ErrorColor = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"link_color");
	if (ptr)
		LinkColor = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"text_color");
	if (ptr)
		TextColor = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h1_color");
	if (ptr)
		H1Color = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h2_color");
	if (ptr)
		H2Color = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h3_color");
	if (ptr)
		H3Color = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h4_color");
	if (ptr)
		H4Color = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h5_color");
	if (ptr)
		H5Color = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h6_color");
	if (ptr)
		H6Color = convertColor(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"error_color_global_color");
	if (ptr)
		ErrorColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"link_color_global_color");
	if (ptr)
		LinkColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"text_color_global_color");
	if (ptr)
		TextColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h1_color_global_color");
	if (ptr)
		H1ColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h2_color_global_color");
	if (ptr)
		H2ColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h3_color_global_color");
	if (ptr)
		H3ColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h4_color_global_color");
	if (ptr)
		H4ColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h5_color_global_color");
	if (ptr)
		H5ColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"h6_color_global_color");
	if (ptr)
		H6ColorGlobalColor = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"text_font_size");
	if (ptr)
		fromString((const char*)ptr, TextFontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"h1_font_size");
	if (ptr)
		fromString((const char*)ptr, H1FontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"h2_font_size");
	if (ptr)
		fromString((const char*)ptr, H2FontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"h3_font_size");
	if (ptr)
		fromString((const char*)ptr, H3FontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"h4_font_size");
	if (ptr)
		fromString((const char*)ptr, H4FontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"h5_font_size");
	if (ptr)
		fromString((const char*)ptr, H5FontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"h6_font_size");
	if (ptr)
		fromString((const char*)ptr, H6FontSize);
	ptr = xmlGetProp (cur, (xmlChar*)"td_begin_space");
	if (ptr)
		fromString((const char*)ptr, TDBeginSpace);
	ptr = xmlGetProp (cur, (xmlChar*)"paragraph_begin_space");
	if (ptr)
		fromString((const char*)ptr, PBeginSpace);
	ptr = xmlGetProp (cur, (xmlChar*)"li_begin_space");
	if (ptr)
		fromString((const char*)ptr, LIBeginSpace);
	ptr = xmlGetProp (cur, (xmlChar*)"ul_begin_space");
	if (ptr)
		fromString((const char*)ptr, ULBeginSpace);
	ptr = xmlGetProp (cur, (xmlChar*)"li_indent");
	if (ptr)
		fromString((const char*)ptr, LIIndent);
	ptr = xmlGetProp (cur, (xmlChar*)"ul_indent");
	if (ptr)
		fromString((const char*)ptr, ULIndent);
	ptr = xmlGetProp (cur, (xmlChar*)"multi_line_space_factor");
	if (ptr)
		fromString((const char*)ptr, LineSpaceFontFactor);
	ptr = xmlGetProp (cur, (xmlChar*)"form_text_group");
	if (ptr)
		DefaultFormTextGroup = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"form_text_area_group");
	if (ptr)
		DefaultFormTextAreaGroup = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"form_select_group");
	if (ptr)
		DefaultFormSelectGroup = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"checkbox_bitmap_normal");
	if (ptr)
		DefaultCheckBoxBitmapNormal = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"checkbox_bitmap_pushed");
	if (ptr)
		DefaultCheckBoxBitmapPushed = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"checkbox_bitmap_over");
	if (ptr)
		DefaultCheckBoxBitmapOver = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"background_bitmap_view");
	if (ptr)
		DefaultBackgroundBitmapView = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"home");
	if (ptr)
		Home = (const char*)(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"browse_next_time");
	if (ptr)
		_BrowseNextTime = convertBool(ptr);
	ptr = xmlGetProp (cur, (xmlChar*)"browse_tree");
	if(ptr)
		_BrowseTree = (const char*)ptr;
	ptr = xmlGetProp (cur, (xmlChar*)"browse_undo");
	if(ptr)
		_BrowseUndoButton= (const char*)ptr;
	ptr = xmlGetProp (cur, (xmlChar*)"browse_redo");
	if(ptr)
		_BrowseRedoButton = (const char*)ptr;
	ptr = xmlGetProp (cur, (xmlChar*)"browse_refresh");
	if(ptr)
		_BrowseRefreshButton = (const char*)ptr;
	ptr = xmlGetProp (cur, (xmlChar*)"timeout");
	if(ptr)
		fromString((const char*)ptr, _TimeoutValue);

	return true;
}

// ***************************************************************************

bool CGroupHTML::handleEvent (const CEventDescriptor& eventDesc)
{
	bool traited = CGroupScrollText::handleEvent (eventDesc);

	if (eventDesc.getType() == CEventDescriptor::system)
	{
		const CEventDescriptorSystem &systemEvent = (const CEventDescriptorSystem &) eventDesc;
		if (systemEvent.getEventTypeExtended() == CEventDescriptorSystem::clocktick)
		{
			// Handle now
			handle ();
		}
	}

	return traited;
}

// ***************************************************************************

void CGroupHTML::endParagraph()
{
	// Remove previous paragraph if empty
	if (_Paragraph && (_Paragraph->getNumChildren() == 0))
	{
		_Paragraph->getParent ()->delGroup(_Paragraph);
		_Paragraph = NULL;
	}

	_Paragraph = NULL;

	paragraphChange ();
}

// ***************************************************************************

void CGroupHTML::newParagraph(uint beginSpace)
{
	// Remove previous paragraph if empty
	if (_Paragraph && (_Paragraph->getNumChildren() == 0))
	{
		_Paragraph->getParent ()->delGroup(_Paragraph);
		_Paragraph = NULL;
	}

	// Add a new paragraph
	CGroupParagraph *newParagraph = new CGroupParagraph(CViewBase::TCtorParam());
	newParagraph->setResizeFromChildH(true);

	newParagraph->setBrowseGroup (this);
	newParagraph->setIndent(_Indent);

	// Add to the group
	addGroup (newParagraph, beginSpace);
	_Paragraph = newParagraph;

	paragraphChange ();
}

// ***************************************************************************

void CGroupHTML::browse(const char *url)
{
	// modify undo/redo
	pushUrlUndoRedo(url);

	// do the browse, with no undo/redo
	doBrowse(url);
}

// ***************************************************************************
void CGroupHTML::refresh()
{
	if (!_URL.empty())
		doBrowse(_URL.c_str());
}

// ***************************************************************************
void CGroupHTML::doBrowse(const char *url)
{
	// Stop previous browse
	if (_Browsing)
	{
		// Clear all the context
		clearContext();

		_Browsing = false;
		if (_Connecting)
		{
			nlassert (_ConnectingLock == this);
			_ConnectingLock = NULL;
		}
		else
			nlassert (_ConnectingLock != this);
		_Connecting = false;
//		stopBrowse ();
		updateRefreshButton();

#ifdef LOG_DL
		nlwarning("*** ALREADY BROWSING, break first");
#endif
	}

#ifdef LOG_DL
	nlwarning("Browsing URL : '%s'", url);
#endif

	// go
	_URL = url;
	_BrowseNextTime = true;

	// if a BrowseTree is bound to us, try to select the node that opens this URL (auto-locate)
	if(!_BrowseTree.empty())
	{
		CInterfaceManager	*pIM= CInterfaceManager::getInstance();

		CGroupTree	*groupTree=dynamic_cast<CGroupTree*>(pIM->getElementFromId(_BrowseTree));
		if(groupTree)
		{
			string	nodeId= selectTreeNodeRecurs(groupTree->getRootNode(), url);
			// select the node
			if(!nodeId.empty())
			{
				groupTree->selectNodeById(nodeId);
			}
		}
	}
}

// ***************************************************************************

void CGroupHTML::browseError (const char *msg)
{
	// Get the list group from CGroupScrollText
	removeContent();
	newParagraph(0);
	CViewText *viewText = new CViewText ("", (string("Error : ")+msg).c_str());
	viewText->setColor (ErrorColor);
	viewText->setModulateGlobalColor(ErrorColorGlobalColor);
	getParagraph()->addChild (viewText);
	if(!_TitlePrefix.empty())
		setTitle (_TitlePrefix);

	stopBrowse ();
	updateRefreshButton();
}

// ***************************************************************************

bool CGroupHTML::isBrowsing()
{
	return _Browsing;
}


void CGroupHTML::stopBrowse ()
{
#ifdef LOG_DL
	nlwarning("*** STOP BROWSE");
#endif

	// Clear all the context
	clearContext();

	_Browsing = false;

	if (_Connecting)
	{
		nlassert (_ConnectingLock == this);
		_ConnectingLock = NULL;
	}
	else
		nlassert (_ConnectingLock != this);
	_Connecting = false;

	// Request running ?
	if (_LibWWW->Request)
	{
//		VerifyLibWWW("HTRequest_kill", HTRequest_kill(_LibWWW->Request) == TRUE);
		HTRequest_kill(_LibWWW->Request);
		HTRequest_delete(_LibWWW->Request);
		_LibWWW->Request = NULL;
	}
}

// ***************************************************************************

void CGroupHTML::updateCoords()
{
	CGroupScrollText::updateCoords();
}

// ***************************************************************************

bool CGroupHTML::translateChar(ucchar &output, ucchar input, ucchar lastCharParam) const
{
	// Keep this char ?
	bool keep = true;

	switch (input)
	{
		// Return / tab only in <PRE> mode
	case '\t':
	case '\n':
		{
			// Get the last char
			ucchar lastChar = lastCharParam;
			if (lastChar == 0)
				lastChar = getLastChar();
			keep = ((lastChar != (ucchar)' ') &&
				    (lastChar != 0)) || getPRE() || (_CurrentViewImage && (lastChar == 0));
			if(!getPRE())
				input = ' ';
		}
		break;
	case ' ':
		{
			// Get the last char
			ucchar lastChar = lastCharParam;
			if (lastChar == 0)
				lastChar = getLastChar();
			keep = ((lastChar != (ucchar)' ') &&
				    (lastChar != (ucchar)'\n') &&
				    (lastChar != 0)) || getPRE() || (_CurrentViewImage && (lastChar == 0));
		}
		break;
	case 0xd:
		keep = false;
		break;
	}

	if (keep)
	{
		output = input;
	}

	return keep;
}

// ***************************************************************************

void CGroupHTML::addString(const ucstring &str)
{
	ucstring tmpStr = str;

	if (_Localize)
	{
		string	_str = tmpStr.toString();
		string::size_type	p = _str.find('#');
		if (p == string::npos)
		{
			tmpStr = CI18N::get(_str);
		}
		else
		{
			string	cmd = _str.substr(0, p);
			string	arg = _str.substr(p+1);

			if (cmd == "date")
			{
				uint	year, month, day;
				sscanf(arg.c_str(), "%d/%d/%d", &year, &month, &day);
				tmpStr = CI18N::get( "uiMFIDate");

				year += (year > 70 ? 1900 : 2000);

				strFindReplace(tmpStr, "%year", toString("%d", year) );
				strFindReplace(tmpStr, "%month", CI18N::get(toString("uiMonth%02d", month)) );
				strFindReplace(tmpStr, "%day", toString("%d", day) );
			}
			else
			{
				tmpStr = arg;
			}
		}
	}

	// In title ?
	if (_Title)
	{
		_TitleString += tmpStr;
	}
	else if (_TextArea)
	{
		_TextAreaContent += tmpStr;
	}
	else if (_SelectOption)
	{
		if (!(_Forms.empty()))
		{
			if (!_Forms.back().Entries.empty())
			{
				_SelectOptionStr += tmpStr;
			}
		}
	}
	else
	{
		// In a paragraph ?
		if (!_Paragraph)
		{
			newParagraph (0);
			paragraphChange ();
		}

		// Text added ?
		bool added = false;

		// Number of child in this paragraph
		if (_CurrentViewLink)
		{
			bool skipLine = !_CurrentViewLink->getText().empty() && *(_CurrentViewLink->getText().rbegin()) == (ucchar) '\n';
			// Compatible with current parameters ?
			if (!skipLine &&
				(getTextColor() == _CurrentViewLink->getColor()) &&
				(getFontSize() == (uint)_CurrentViewLink->getFontSize()) &&
				(getLink() == _CurrentViewLink->Link) &&
				(getGlobalColor() == _CurrentViewLink->getModulateGlobalColor()))
			{
				// Concat the text
				_CurrentViewLink->setText(_CurrentViewLink->getText()+tmpStr);
				_CurrentViewLink->invalidateContent();
				added = true;
			}
		}

		// Not added ?
		if (!added)
		{
			CViewLink *newLink = new CViewLink(CViewBase::TCtorParam());
			if (getA())
			{
				newLink->Link = getLink();
				if (!newLink->Link.empty())
				{
					newLink->setHTMLView (this);
					newLink->setUnderlined (true);
				}
			}
			newLink->setText(tmpStr);
			newLink->setColor(getTextColor());
			newLink->setFontSize(getFontSize());
			newLink->setMultiLineSpace((uint)((float)getFontSize()*LineSpaceFontFactor));
			newLink->setMultiLine(true);
			newLink->setModulateGlobalColor(getGlobalColor());
			// newLink->setLineAtBottom (true);

			if (getA() && !newLink->Link.empty())
			{
				getParagraph()->addChildLink(newLink);
			}
			else
			{
				getParagraph()->addChild(newLink);
			}
			paragraphChange ();
		}
	}
}

// ***************************************************************************

void CGroupHTML::addImage(const char *img, bool globalColor)
{
	// In a paragraph ?
	if (_Paragraph)
	{
		string finalUrl;

		//
		// 1/ try to load the image with the old system (local files in bnp)
		//
		string image = CFile::getPath(img) + CFile::getFilenameWithoutExtension(img) + ".tga";
		if (lookupLocalFile (finalUrl, image.c_str(), false))
		{
			// No more text in this text view
			_CurrentViewLink = NULL;

			// Not added ?
			CViewBitmap *newImage = new CViewBitmap (TCtorParam());
			/* todo link in image
			if (getA())
			{
				newImage->Link = getLink();
				newImage->setHTMLView (this);
			}*/
			newImage->setTexture (finalUrl);
			newImage->setModulateGlobalColor(globalColor);

			/* todo link in image
			if (getA())
				getParagraph()->addChildLink(newImage);
			else*/
			getParagraph()->addChild(newImage);
			paragraphChange ();
		} else {

			//
			// 2/ if it doesn't work, try to load the image in cache
			//
			image = localImageName(img);
			if (lookupLocalFile (finalUrl, image.c_str(), false))
			{
				// No more text in this text view
				_CurrentViewLink = NULL;

				// Not added ?
				CViewBitmap *newImage = new CViewBitmap (TCtorParam());
				/* todo link in image
				if (getA())
				{
				newImage->Link = getLink();
				newImage->setHTMLView (this);
				}*/

				// don't display image that are not power of 2
				uint32 w, h;
				CBitmap::loadSize (image, w, h);
				if (w == 0 || h == 0 || !NLMISC::isPowerOf2(w) || !NLMISC::isPowerOf2(h))
					image.clear();

				newImage->setTexture (image);
//				newImage->setTexture (finalUrl);
				newImage->setModulateGlobalColor(globalColor);

				/* todo link in image
				if (getA())
				getParagraph()->addChildLink(newImage);
				else*/
				getParagraph()->addChild(newImage);
				paragraphChange ();
			} else {

				//
				// 3/ if it doesn't work, display a placeholder and ask to dl the image into the cache
				//
				image = "web_del.tga";
				if (lookupLocalFile (finalUrl, image.c_str(), false))
				{
					// No more text in this text view
					_CurrentViewLink = NULL;

					// Not added ?
					CViewBitmap *newImage = new CViewBitmap (TCtorParam());
					/* todo link in image
					if (getA())
					{
					newImage->Link = getLink();
					newImage->setHTMLView (this);
					}*/
					newImage->setTexture (image);
					//				newImage->setTexture (finalUrl);
					newImage->setModulateGlobalColor(globalColor);

					addImageDownload(img, newImage);

					/* todo link in image
					if (getA())
					getParagraph()->addChildLink(newImage);
					else*/
					getParagraph()->addChild(newImage);
					paragraphChange ();
				}
			}
		}
	}
}

// ***************************************************************************

CInterfaceGroup *CGroupHTML::addTextArea(const std::string &templateName, const char *name, uint /* rows */, uint cols, bool multiLine, const ucstring &content)
{
	// In a paragraph ?
	if (!_Paragraph)
	{
		newParagraph (0);
		paragraphChange ();
	}

	// No more text in this text view
	_CurrentViewLink = NULL;

	CInterfaceManager *im = CInterfaceManager::getInstance();
	if (im)
	{
		// Not added ?
		std::vector<std::pair<std::string,std::string> > templateParams;
		templateParams.push_back (std::pair<std::string,std::string> ("w", toString (cols*12)));
		//templateParams.push_back (std::pair<std::string,std::string> ("h", toString (rows*12)));
		templateParams.push_back (std::pair<std::string,std::string> ("id", name));
		templateParams.push_back (std::pair<std::string,std::string> ("prompt", ""));
		templateParams.push_back (std::pair<std::string,std::string> ("multiline", multiLine?"true":"false"));
		templateParams.push_back (std::pair<std::string,std::string> ("want_return", multiLine?"true":"false"));
		templateParams.push_back (std::pair<std::string,std::string> ("enter_recover_focus", "false"));
		templateParams.push_back (std::pair<std::string,std::string> ("max_num_chars", "1024"));
		CInterfaceGroup *textArea = im->createGroupInstance (templateName.c_str(),
			getParagraph()->getId(), templateParams.empty()?NULL:&(templateParams[0]), (uint)templateParams.size());

		// Group created ?
		if (textArea)
		{
			// Set the content
			CGroupEditBox *eb = dynamic_cast<CGroupEditBox*>(textArea->getGroup("eb"));
			if (eb)
				eb->setInputString(decodeHTMLEntities(content));

			textArea->invalidateCoords();
			getParagraph()->addChild (textArea);
			paragraphChange ();

			return textArea;
		}
	}

	// Not group created
	return NULL;
}

// ***************************************************************************
CDBGroupComboBox *CGroupHTML::addComboBox(const std::string &templateName, const char *name)
{
	// In a paragraph ?
	if (!_Paragraph)
	{
		newParagraph (0);
		paragraphChange ();
	}


	CInterfaceManager *im = CInterfaceManager::getInstance();
	if (im)
	{
		// Not added ?
		std::vector<std::pair<std::string,std::string> > templateParams;
		templateParams.push_back (std::pair<std::string,std::string> ("id", name));
		CInterfaceGroup *group = im->createGroupInstance (templateName.c_str(),
			getParagraph()->getId(), templateParams.empty()?NULL:&(templateParams[0]), (uint)templateParams.size());

		// Group created ?
		if (group)
		{
			// Set the content
			CDBGroupComboBox *cb = dynamic_cast<CDBGroupComboBox *>(group);
			if (!cb)
			{
				nlwarning("'%s' template has bad type, combo box expected", templateName.c_str());
				delete cb;
				return NULL;
			}
			else
			{
				getParagraph()->addChild (cb);
				paragraphChange ();
				return cb;
			}
		}
	}

	// Not group created
	return NULL;
}

// ***************************************************************************

CCtrlButton *CGroupHTML::addButton(CCtrlButton::EType type, const std::string &/* name */, const std::string &normalBitmap, const std::string &pushedBitmap,
								  const std::string &overBitmap, bool useGlobalColor, const char *actionHandler, const char *actionHandlerParams,
								  const char *tooltip)
{
	// In a paragraph ?
	if (!_Paragraph)
	{
		newParagraph (0);
		paragraphChange ();
	}

	// Add the ctrl button
	CCtrlButton *ctrlButton = new CCtrlButton(TCtorParam());

	// Load only tga files.. (conversion in dds filename is done in the lookup procedure)
	string normal = normalBitmap.empty()?"":CFile::getPath(normalBitmap) + CFile::getFilenameWithoutExtension(normalBitmap) + ".tga";

	// if the image doesn't exist on local, we check in the cache
//	if(!CFile::fileExists(normal))
	if(!CPath::exists(normal))
	{
		// search in the compressed texture
		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		CViewRenderer &rVR = pIM->getViewRenderer();
		sint32 id = rVR.getTextureIdFromName(normal);
		if(id == -1)
		{
			normal = localImageName(normalBitmap);
			if(!CFile::fileExists(normal))
			{
				normal = "web_del.tga";
				addImageDownload(normalBitmap, ctrlButton);
			}
		}
	}

	string pushed = pushedBitmap.empty()?"":CFile::getPath(pushedBitmap) + CFile::getFilenameWithoutExtension(pushedBitmap) + ".tga";
	// if the image doesn't exist on local, we check in the cache, don't download it because the "normal" will already setuped it
//	if(!CFile::fileExists(pushed))
	if(!CPath::exists(pushed))
	{
		// search in the compressed texture
		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		CViewRenderer &rVR = pIM->getViewRenderer();
		sint32 id = rVR.getTextureIdFromName(pushed);
		if(id == -1)
		{
			pushed = localImageName(pushedBitmap);
		}
	}

	string over = overBitmap.empty()?"":CFile::getPath(overBitmap) + CFile::getFilenameWithoutExtension(overBitmap) + ".tga";

	ctrlButton->setType (type);
	if (!normal.empty())
		ctrlButton->setTexture (normal);
	if (!pushed.empty())
		ctrlButton->setTexturePushed (pushed);
	if (!over.empty())
		ctrlButton->setTextureOver (over);
	ctrlButton->setModulateGlobalColorAll (useGlobalColor);
	ctrlButton->setActionOnLeftClick (actionHandler);
	ctrlButton->setParamsOnLeftClick (actionHandlerParams);

	// Translate the tooltip
	if (tooltip)
		ctrlButton->setDefaultContextHelp (CI18N::get (tooltip));

	getParagraph()->addChild (ctrlButton);
	paragraphChange ();

	return ctrlButton;
}

// ***************************************************************************

void CGroupHTML::flushString()
{
	_CurrentViewLink = NULL;
}

// ***************************************************************************

void CGroupHTML::clearContext()
{
	_Paragraph = NULL;
	_PRE.clear();
	_TextColor.clear();
	_GlobalColor.clear();
	_FontSize.clear();
	_Indent = 0;
	_LI = false;
	_UL.clear();
	_A.clear();
	_Link.clear();
	_Tables.clear();
	_Cells.clear();
	_TR.clear();
	_Forms.clear();
	_CellParams.clear();
	_Title = false;
	_TextArea = false;
	_Localize = false;

	// TR

	paragraphChange ();

	// clear the pointer to the current image download since all the button are deleted
#ifdef LOG_DL
	nlwarning("Clear pointers to %d curls", Curls.size());
#endif
	for(uint i = 0; i < Curls.size(); i++)
	{
		Curls[i].imgs.clear();
	}

}

// ***************************************************************************

ucchar CGroupHTML::getLastChar() const
{
	if (_CurrentViewLink)
	{
		const ucstring &str = _CurrentViewLink->getText();
		if (!str.empty())
			return str[str.length()-1];
	}
	return 0;
}

// ***************************************************************************

void CGroupHTML::paragraphChange ()
{
	_CurrentViewLink = NULL;
	_CurrentViewImage = NULL;
	CGroupParagraph *paragraph = getParagraph();
	if (paragraph)
	{
		// Number of child in this paragraph
		uint numChild = paragraph->getNumChildren();
		if (numChild)
		{
			// Get the last child
			CViewBase *child = paragraph->getChild(numChild-1);

			// Is this a string view ?
			_CurrentViewLink = dynamic_cast<CViewLink*>(child);
			_CurrentViewImage = dynamic_cast<CViewBitmap*>(child);
		}
	}
}

// ***************************************************************************

CInterfaceGroup *CGroupHTML::getCurrentGroup()
{
	if (!_Cells.empty() && _Cells.back())
		return _Cells.back()->Group;
	else
		return _GroupListAdaptor;
}

// ***************************************************************************

void CGroupHTML::addGroup (CInterfaceGroup *group, uint beginSpace)
{
	// Remove previous paragraph if empty
	if (_Paragraph && (_Paragraph->getNumChildren() == 0))
	{
		_Paragraph->getParent ()->delGroup(_Paragraph);
		_Paragraph = NULL;
	}

	group->setSizeRef(CInterfaceElement::width);

	// Compute begin space between paragraph and tables

	// * If first in group, no begin space
	// * If behind a paragraph, take the biggest begin space between the previous paragraph and current one.

	// Pointer on the current paragraph (can be a table too)
	CGroupParagraph *p = dynamic_cast<CGroupParagraph*>(group);

	CInterfaceGroup *parentGroup = CGroupHTML::getCurrentGroup();
	const std::vector<CInterfaceGroup*> &groups = parentGroup->getGroups ();
	group->setParent(parentGroup);
	group->setParentSize(parentGroup);
	if (groups.empty())
	{
		group->setParentPos(parentGroup);
		group->setPosRef(Hotspot_TL);
		group->setParentPosRef(Hotspot_TL);
		beginSpace = 0;
	}
	else
	{
		// Last is a paragraph ?
		group->setParentPos(groups.back());
		group->setPosRef(Hotspot_TL);
		group->setParentPosRef(Hotspot_BL);

		// Begin space for previous paragraph
		CGroupParagraph *previous = dynamic_cast<CGroupParagraph*>(groups.back());
		if (previous)
			beginSpace = std::max(beginSpace, previous->getTopSpace());
	}

	// Set the begin space
	if (p)
		p->setTopSpace(beginSpace);
	else
		group->setY(-(sint32)beginSpace);

	parentGroup->addGroup (group);
}

// ***************************************************************************

void CGroupHTML::setTitle (const ucstring &title)
{
	CInterfaceElement *parent = getParent();
	if (parent)
	{
		if (parent = parent->getParent())
		{
			CGroupContainer *container = dynamic_cast<CGroupContainer*>(parent);
			if (container)
			{
				container->setUCTitle (title);
			}
		}
	}
}

// ***************************************************************************

bool CGroupHTML::lookupLocalFile (string &result, const char *url, bool isUrl)
{
	result = url;
	string tmp;

	// folder used for images cache
	static const string cacheDir = "cache";

	string::size_type protocolPos = strlwr(result).find("://");

	if (protocolPos != string::npos)
	{
		// protocol present, it's an url so file must be searched in cache folder
		// TODO: case of special characters & and ?
		result = cacheDir + result.substr(protocolPos+2);

		// if the file is already cached, use it
		if (CFile::fileExists(result)) tmp = result;
	}
	else
	{
		// Url is a file ?
		if (strlwr(result).find("file:") == 0)
		{
			result = result.substr(5, result.size()-5);
		}

		tmp = CPath::lookup (CFile::getFilename(result), false, false, false);
		if (tmp.empty())
		{
			// try to find in local directory
			tmp = CPath::lookup (result, false, false, true);
		}
	}

	if (!tmp.empty())
	{
		// Normalize the path
		if (isUrl)
			//result = "file:"+strlwr(CPath::standardizePath (CPath::getFullPath (CFile::getPath(result)))+CFile::getFilename(result));*/
			result = "file:/"+tmp;
		else
			result = tmp;
		return true;
	}
	else
	{
		// Is it a texture in the big texture ?
		CInterfaceManager *pIM = CInterfaceManager::getInstance();
		if (pIM->getViewRenderer().getTextureIdFromName (result) >= 0)
		{
			return true;
		}
		else
		{
			// This is not a file in the CPath, let libwww open this URL
			result = url;
			return false;
		}
	}
}

// ***************************************************************************

void CGroupHTML::submitForm (uint formId, const char *submitButtonName)
{
	// Form id valid ?
	if (formId < _Forms.size())
	{
		_PostNextTime = true;
		_PostFormId = formId;
		_PostFormSubmitButton = submitButtonName;
	}
}

// ***************************************************************************

void CGroupHTML::setBackgroundColor (const CRGBA &bgcolor)
{
	// Should have a child named bg
	CViewBase *view = getView (DefaultBackgroundBitmapView);
	if (view)
	{
		CViewBitmap *bitmap = dynamic_cast<CViewBitmap*> (view);
		if (bitmap)
		{
			// Change the background color
			bitmap->setColor (bgcolor);
			bitmap->setModulateGlobalColor(true);
		}
	}
}


struct CButtonFreezer : public CInterfaceElementVisitor
{
	virtual void visitCtrl(CCtrlBase *ctrl)
	{
		CCtrlBaseButton		*textButt = dynamic_cast<CCtrlTextButton *>(ctrl);
		if (textButt)
		{
			textButt->setFrozen(true);
		}
	}
};

// ***************************************************************************

void CGroupHTML::handle ()
{
	H_AUTO(RZ_Interface_Html_handle)

	if (_Connecting)
	{
		nlassert (_ConnectingLock == this);

		// Check timeout if needed
		if (_TimeoutValue != 0 && _ConnectingTimeout <= TimeInSec)
		{
			browseError(("Connection timeout : "+_URL).c_str());
		}
	}
	else
	{
		if (_ConnectingLock == NULL)
		{
			if (_BrowseNextTime)
			{
				// Stop browsing now
				stopBrowse ();
				updateRefreshButton();

				// Home ?
				if (_URL == "home")
					_URL = home();

				string finalUrl;
				bool isLocal = lookupLocalFile (finalUrl, _URL.c_str(), true);

				// Reset the title
				if(_TitlePrefix.empty())
					setTitle (CI18N::get("uiPleaseWait"));
				else
					setTitle (_TitlePrefix + " - " + CI18N::get("uiPleaseWait"));

				// Start connecting
				nlassert (_ConnectingLock == NULL);
				_ConnectingLock = this;
				_Connecting = true;
				_ConnectingTimeout = TimeInSec + _TimeoutValue;


				CButtonFreezer freezer;
				this->visit(&freezer);

				// Browsing
				_Browsing = true;
				updateRefreshButton();

				// Add custom get params
				addHTTPGetParams (finalUrl);

				// Save new url
				_URL = finalUrl;

				// display HTTP query
				//nlinfo("WEB: GET '%s'", finalUrl.c_str());

				// Init LibWWW
				initLibWWW();
				setCurrentDomain(finalUrl);

				// Get the final URL
				C3WSmartPtr uri = HTParse(finalUrl.c_str(), NULL, PARSE_ALL);

				// Create an anchor
#ifdef NL_OS_WINDOWS
				if ((_LibWWW->Anchor = HTAnchor_findAddress(uri)) == NULL)
#else
				// temporarily disable local URL's until LibWWW can be replaced.
				if (isLocal || ((_LibWWW->Anchor = HTAnchor_findAddress(uri)) == NULL))
#endif
				{
					browseError((string("The page address is malformed : ")+(const char*)uri).c_str());
				}
				else
				{
					/* Add our own request terminate handler. Nb: pass as param a UID, not the ptr */
					HTNet_addAfter(requestTerminater, NULL, (void*)_GroupHtmlUID, HT_ALL, HT_FILTER_LAST);

					/* Set the timeout for long we are going to wait for a response */
					HTHost_setEventTimeout(60000);

					/* Start the first request */

					// request = Request_new(app);
					_LibWWW->Request = HTRequest_new();
					HTRequest_setContext(_LibWWW->Request, this);

					// add supported language header
					HTList *langs = HTList_new();
					// set the language code used by the client
					HTLanguage_add(langs, ClientCfg.getHtmlLanguageCode().c_str(), 1.0);
					HTRequest_setLanguage (_LibWWW->Request, langs, 1);

					// get_document(_LibWWW->Request, _LibWWW->Anchor);
					C3WSmartPtr address = HTAnchor_address(_LibWWW->Anchor);
					HTRequest_setAnchor(_LibWWW->Request, _LibWWW->Anchor);
					if (HTLoad(_LibWWW->Request, NO))
					{
					}
					else
					{
						browseError((string("The page cannot be displayed : ")+(const char*)uri).c_str());
					}
				}

				_BrowseNextTime = false;
			}

			if (_PostNextTime)
			{
				/* Create a list to hold the form arguments */
				HTAssocList * formfields = HTAssocList_new();

				// Add text area text
				uint i;

				// Ref the form
				CForm &form = _Forms[_PostFormId];

				// Save new url
				_URL = form.Action;

				for (i=0; i<form.Entries.size(); i++)
				{
					// Text area ?
					bool addEntry = false;
					ucstring entryData;
					if (form.Entries[i].TextArea)
					{
						// Get the edit box view
						CInterfaceGroup *group = form.Entries[i].TextArea->getGroup ("eb");
						if (group)
						{
							// Should be a CGroupEditBox
							CGroupEditBox *editBox = dynamic_cast<CGroupEditBox*>(group);
							if (editBox)
							{
								entryData = editBox->getViewText()->getText();
								addEntry = true;
							}
						}
					}
					else if (form.Entries[i].Checkbox)
					{
						// todo handle unicode POST here
						if (form.Entries[i].Checkbox->getPushed ())
						{
							entryData = ucstring ("on");
							addEntry = true;
						}
					}
					else if (form.Entries[i].ComboBox)
					{
						CDBGroupComboBox *cb = form.Entries[i].ComboBox;
						entryData.fromUtf8(form.Entries[i].SelectValues[cb->getSelection()]);
						addEntry = true;
					}
					// This is a hidden value
					else
					{
						entryData = form.Entries[i].Value;
						addEntry = true;
					}

					// Add this entry
					if (addEntry)
					{
						// Build a utf8 string
						string uft8 = form.Entries[i].Name + "=" + CI18N::encodeUTF8(entryData);

						/* Parse the content and add it to the association list */
						HTParseFormInput(formfields, uft8.c_str());
					}
				}

				// Add the button coordinates
				HTParseFormInput(formfields, (_PostFormSubmitButton + "_x=0").c_str());
				HTParseFormInput(formfields, (_PostFormSubmitButton + "_y=0").c_str());

				// Add custom params
				addHTTPPostParams (formfields);

				// Reset the title
				if(_TitlePrefix.empty())
					setTitle (CI18N::get("uiPleaseWait"));
				else
					setTitle (_TitlePrefix + " - " + CI18N::get("uiPleaseWait"));

				// Stop previous browse
				stopBrowse ();

				// Set timeout
				nlassert (_ConnectingLock == NULL);
				_ConnectingLock = this;
				_Connecting = true;
				_ConnectingTimeout = TimeInSec + _TimeoutValue;

				CButtonFreezer freezer;
				this->visit(&freezer);

				// Browsing
				_Browsing = true;
				updateRefreshButton();

				// display HTTP query with post parameters
				//nlinfo("WEB: POST %s", _URL.c_str());

				// Init LibWWW
				initLibWWW();
				setCurrentDomain(_URL);

				// Get the final URL
				C3WSmartPtr uri = HTParse(_URL.c_str(), NULL, PARSE_ALL);

				// Create an anchor
				if ((_LibWWW->Anchor = HTAnchor_findAddress(uri)) == NULL)
				{
					browseError((string("The page address is malformed : ")+(const char*)uri).c_str());
				}
				else
				{
					/* Add our own request terminate handler. Nb: pass as param a UID, not the ptr */
					HTNet_addAfter(requestTerminater, NULL, (void*)_GroupHtmlUID, HT_ALL, HT_FILTER_LAST);

					/* Start the first request */

					// request = Request_new(app);
					_LibWWW->Request = HTRequest_new();
					HTRequest_setContext(_LibWWW->Request, this);

					/*
					** Dream up a source anchor (an editor can for example use this).
					** After creation we associate the data that we want to post and
					** set some metadata about what the data is. More formats can be found
					** ../src/HTFormat.html
					*/
					/*HTParentAnchor *src = HTTmpAnchor(NULL);
					HTAnchor_setDocument(src, (void*)(data.c_str()));
					HTAnchor_setFormat(src, WWW_PLAINTEXT);*/

					/*
					** If not posting to an HTTP/1.1 server then content length MUST be
					** there. If HTTP/1.1 then it doesn't matter as we just use chunked
					** encoding under the covers
					*/
					// HTAnchor_setLength(src, data.size());

					HTParentAnchor *result =  HTPostFormAnchor (formfields, _LibWWW->Anchor, _LibWWW->Request);
					if (result)
					{
					}
					else
					{
						browseError((string("The page cannot be displayed : ")+(const char*)uri).c_str());
					}

					/* POST the source to the dest */
					/*
					BOOL status = NO;
					status = HTPostAnchor(src, _LibWWW->Anchor, _LibWWW->Request);
					if (status)
					{
					}
					else
					{
						browseError((string("The page cannot be displayed : ")+(const char*)uri).c_str());
					}*/
				}

				_PostNextTime = false;
			}
		}
	}
}

// ***************************************************************************

void CGroupHTML::draw ()
{
	checkImageDownload();
	CGroupScrollText::draw ();
}

// ***************************************************************************

void CGroupHTML::endBuild ()
{
	invalidateCoords();
}

// ***************************************************************************

void CGroupHTML::addHTTPGetParams (string &/* url */)
{
}

// ***************************************************************************

void CGroupHTML::addHTTPPostParams (HTAssocList * /* formfields */)
{
}

// ***************************************************************************

void CGroupHTML::requestTerminated(HTRequest * /* request */)
{
	// set the browser as complete
	_Browsing = false;
	updateRefreshButton();
	// check that the title is set, or reset it (in the case the page
	// does not provide a title)
	if (_TitleString.empty())
	{
		setTitle(_TitlePrefix);
	}
}

// ***************************************************************************

string	CGroupHTML::home ()
{
	return Home;
}

// ***************************************************************************

void CGroupHTML::removeContent ()
{
	// Remove old document
	if (!_GroupListAdaptor)
	{
		_GroupListAdaptor = new CGroupListAdaptor(CViewBase::TCtorParam()); // deleted by the list
		_GroupListAdaptor->setResizeFromChildH(true);
		getList()->addChild (_GroupListAdaptor, true);
	}

	// Group list adaptor not exist ?
	_GroupListAdaptor->clearGroups();
	_GroupListAdaptor->clearControls();
	_GroupListAdaptor->clearViews();
	CInterfaceManager::getInstance()->clearViewUnders();
	CInterfaceManager::getInstance()->clearCtrlsUnders();
	_Paragraph = NULL;

	// Reset default background color
	setBackgroundColor (BgColor);

	paragraphChange ();
}

// ***************************************************************************
const std::string &CGroupHTML::selectTreeNodeRecurs(CGroupTree::SNode *node, const std::string &url)
{
	static std::string	emptyString;
	if(!node)
	{
		return emptyString;
	}

	// if this node match
	if(actionLaunchUrlRecurs(node->AHName, node->AHParams, url))
	{
		return node->Id;
	}
	// fails => look into children
	else
	{
		for(uint i=0;i<node->Children.size();i++)
		{
			const string &childRes= selectTreeNodeRecurs(node->Children[i], url);
			if(!childRes.empty())
				return childRes;
		}

		// none match...
		return emptyString;
	}
}

// ***************************************************************************
bool	CGroupHTML::actionLaunchUrlRecurs(const std::string &ah, const std::string &params, const std::string &url)
{
	CInterfaceManager	*pIM= CInterfaceManager::getInstance();

	// check if this action match
	if( (ah=="launch_help" || ah=="browse") && IActionHandler::getParam (params, "url") == url)
	{
		return true;
	}
	// can be a proc that contains launch_help/browse => look recurs
	else if(ah=="proc")
	{
		const std::string &procName= params;
		// look into this proc
		uint	numActions= pIM->getProcedureNumActions(procName);
		for(uint i=0;i<numActions;i++)
		{
			string	procAh, procParams;
			if(pIM->getProcedureAction(procName, i, procAh, procParams))
			{
				// recurs proc if needed!
				if (actionLaunchUrlRecurs(procAh, procParams, url))
					return true;
			}
		}
	}

	return false;
}

// ***************************************************************************
void	CGroupHTML::clearUndoRedo()
{
	// erase any undo/redo
	_BrowseUndo.clear();
	_BrowseRedo.clear();

	// update buttons validation
	updateUndoRedoButtons();
}

// ***************************************************************************
void	CGroupHTML::pushUrlUndoRedo(const std::string &url)
{
	// if same url, no op
	if(url==_AskedUrl)
		return;

	// erase any redo, push undo, set current
	_BrowseRedo.clear();
	if(!_AskedUrl.empty())
		_BrowseUndo.push_back(_AskedUrl);
	_AskedUrl= url;

	// limit undo
	while(_BrowseUndo.size()>MaxUrlUndoRedo)
		_BrowseUndo.pop_front();

	// update buttons validation
	updateUndoRedoButtons();
}

// ***************************************************************************
void	CGroupHTML::browseUndo()
{
	if(_BrowseUndo.empty())
		return;

	// push to redo, pop undo, and set current
	_BrowseRedo.push_front(_AskedUrl);
	_AskedUrl= _BrowseUndo.back();
	_BrowseUndo.pop_back();

	// update buttons validation
	updateUndoRedoButtons();

	// and then browse the undoed url, with no undo/redo
	doBrowse(_AskedUrl.c_str());
}

// ***************************************************************************
void	CGroupHTML::browseRedo()
{
	if(_BrowseRedo.empty())
		return;

	// push to undo, pop redo, and set current
	_BrowseUndo.push_back(_AskedUrl);
	_AskedUrl= _BrowseRedo.front();
	_BrowseRedo.pop_front();

	// update buttons validation
	updateUndoRedoButtons();

	// and then browse the redoed url, with no undo/redo
	doBrowse(_AskedUrl.c_str());
}

// ***************************************************************************
void	CGroupHTML::updateUndoRedoButtons()
{
	CInterfaceManager	*pIM= CInterfaceManager::getInstance();
	CCtrlBaseButton		*butUndo= dynamic_cast<CCtrlBaseButton *>(pIM->getElementFromId(_BrowseUndoButton));
	CCtrlBaseButton		*butRedo= dynamic_cast<CCtrlBaseButton *>(pIM->getElementFromId(_BrowseRedoButton));

	// gray according to list size
	if(butUndo)
		butUndo->setFrozen(_BrowseUndo.empty());
	if(butRedo)
		butRedo->setFrozen(_BrowseRedo.empty());
}

// ***************************************************************************
void	CGroupHTML::updateRefreshButton()
{
	CInterfaceManager	*pIM= CInterfaceManager::getInstance();
	CCtrlBaseButton		*butRefresh = dynamic_cast<CCtrlBaseButton *>(pIM->getElementFromId(_BrowseRefreshButton));

	bool enabled = !_Browsing && !_Connecting;
	if(butRefresh)
		butRefresh->setFrozen(!enabled);
}

// ***************************************************************************

NLMISC_REGISTER_OBJECT(CViewBase, CGroupHTMLInputOffset, std::string, "html_input_offset");

CGroupHTMLInputOffset::CGroupHTMLInputOffset(const TCtorParam &param)
	: CInterfaceGroup(param),
	Offset(0)
{
}

// ***************************************************************************
bool CGroupHTMLInputOffset::parse(xmlNodePtr cur, CInterfaceGroup *parentGroup)
{
	if (!CInterfaceGroup::parse(cur, parentGroup)) return false;
	CXMLAutoPtr ptr;
	// Get the url
	ptr = xmlGetProp (cur, (xmlChar*)"y_offset");
	if (ptr)
		fromString((const char*)ptr, Offset);
	return true;
}

// ***************************************************************************
int CGroupHTML::luaBrowse(CLuaState &ls)
{
	const char *funcName = "browse";
	CLuaIHM::checkArgCount(ls, funcName, 1);
	CLuaIHM::checkArgType(ls, funcName, 1, LUA_TSTRING);
	browse(ls.toString(1));
	return 0;
}

// ***************************************************************************
int CGroupHTML::luaRefresh(CLuaState &ls)
{
	const char *funcName = "refresh";
	CLuaIHM::checkArgCount(ls, funcName, 0);
	refresh();
	return 0;
}

// ***************************************************************************
void CGroupHTML::setURL(const std::string &url)
{
	browse(url.c_str());
}

// ***************************************************************************
inline bool isDigit(ucchar c, uint base = 16)
{
	if (c>='0' && c<='9') return true;
	if (base != 16) return false;
	if (c>='A' && c<='F') return true;
	if (c>='a' && c<='f') return true;
	return false;
}

// ***************************************************************************
inline ucchar convertHexDigit(ucchar c)
{
	if (c>='0' && c<='9') return c-'0';
	if (c>='A' && c<='F') return c-'A'+10;
	if (c>='a' && c<='f') return c-'a'+10;
	return 0;
}

// ***************************************************************************
ucstring CGroupHTML::decodeHTMLEntities(const ucstring &str)
{
	ucstring result;
	uint last, pos;

	for (uint i=0; i<str.length(); ++i)
	{
		// HTML entity
		if (str[i] == '&' && (str.length()-i) >= 4)
		{
			pos = i+1;

			// unicode character
			if (str[pos] == '#')
			{
				++pos;

				// using decimal by default
				uint base = 10;

				// using hexadecimal if &#x
				if (str[pos] == 'x')
				{
					base = 16;
					++pos;
				}

				// setup "last" to point at the first character following "&#x?[0-9a-f]+"
				for (last = pos; last < str.length(); ++last) if (!isDigit(str[last], base)) break;

				// make sure that at least 1 digit was found
				// and have the terminating ';' to complete the token: "&#x?[0-9a-f]+;"
				if (last == pos || str[last] != ';')
				{
					result += str[i];
					continue;
				}

				ucchar c = 0;

				// convert digits to unicode character
				while (pos<last) c = convertHexDigit(str[pos++]) + c*ucchar(base);

				// append our new character to the result string
				result += c;

				// move 'i' forward to point at the ';' .. the for(...) will increment i to point to next char
				i = last;

				continue;
			}

			// special xml characters
			if (str.substr(i+1,5)==ucstring("quot;"))	{ i+=5; result+='\"'; continue; }
			if (str.substr(i+1,4)==ucstring("amp;"))	{ i+=4; result+='&'; continue; }
			if (str.substr(i+1,3)==ucstring("lt;"))	{ i+=3; result+='<'; continue; }
			if (str.substr(i+1,3)==ucstring("gt;"))	{ i+=3; result+='>'; continue; }
		}

		// all the special cases are catered for... treat this as a normal character
		result += str[i];
	}

	return result;
}
