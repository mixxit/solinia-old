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



#ifndef NL_GROUP_FRAME_H
#define NL_GROUP_FRAME_H

#include "nel/misc/types_nl.h"
#include "interface_group.h"


// ***************************************************************************
/** A Group with a background and a frame displayed
 * \author Lionel Berenguier
 * \author Nevrax France
 * \date 2002
 */
class CGroupFrame : public CInterfaceGroup
{
public:

	/// Constructor
	CGroupFrame(const TCtorParam &param);

	virtual bool parse (xmlNodePtr cur, CInterfaceGroup *parentGroup);
	virtual void draw ();

	void    copyOptionFrom(const CGroupFrame &other);


	void setColorAsString(const std::string & col);
	std::string getColorAsString() const;

	REFLECT_EXPORT_START(CGroupFrame, CInterfaceGroup)
		REFLECT_STRING ("color", getColorAsString, setColorAsString);
	REFLECT_EXPORT_END


	static void resetDisplayTypes() { _DispTypes.clear(); }

// ******************
protected:

	bool			_DisplayFrame;

	NLMISC::CRGBA	_Color;

	uint8			_DispType;

	// Fields Defined in the XML => must not herit them from extends=""
	bool			_DisplayFrameDefined : 1;
	bool			_ColorDefined : 1;
	bool			_DispTypeDefined : 1;

	// Static stuff
	enum
	{
		TextTL= 0,
		TextTM,
		TextTR,
		TextML,
		TextMM,
		TextMR,
		TextBL,
		TextBM,
		TextBR
	};

	struct SDisplayType
	{
		std::string Name;
		sint32	BorderIds[9];
		uint8	TileBorder[9]; // Dont works for TextTL, TextTR, TextBL, TextBR
		sint32	LeftBorder; // enum
		sint32	RightBorder;
		sint32	TopBorder;
		sint32	BottomBorder;

		// -----------------------
		SDisplayType()
		{
			for (uint i = 0; i < 9; ++i)
				TileBorder[i] = 0;
		}
	};
	static std::vector<SDisplayType> _DispTypes;
};


#endif // NL_GROUP_FRAME_H

/* End of group_frame.h */
