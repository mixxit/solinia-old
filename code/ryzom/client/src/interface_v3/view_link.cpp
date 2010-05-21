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

#include "nel/misc/bit_mem_stream.h"
#include "nel/misc/i18n.h"

#include "view_link.h"

using namespace std;
using namespace NLMISC;
using namespace NL3D;

// ***************************************************************************

CViewLink::CViewLink (const TCtorParam &param)
: CViewText(param)
{
	HTML = NULL;
}

// ***************************************************************************

void CViewLink::setHTMLView(CGroupHTML *html)
{
	HTML = html;
}

// ***************************************************************************

