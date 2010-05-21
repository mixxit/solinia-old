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

#include <windows.h>
#include <locale.h>
#include <windowsx.h>
#include <commctrl.h>
#include <max.h>
#include <iparamb2.h>
#include <istdplug.h>
#include <iparamm2.h>
#include <utilapi.h>
#include <shlobj.h>
#undef STRICT
#include <MaxScrpt/maxscrpt.h>
#include <MaxScrpt/3dmath.h>
#include <MaxScrpt/numbers.h>
#include <MaxScrpt/maxclses.h>
#include <MaxScrpt/streams.h>
#include <MaxScrpt/mstime.h>
#include <MaxScrpt/maxobj.h>
#include <MaxScrpt/parser.h>
#include <stdmat.h>
#include <MaxScrpt/definsfn.h>
#include <animtbl.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "nel/misc/bsphere.h"
