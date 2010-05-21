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

#ifndef NL_UNIX_EVENT_EMITTER_H
#define NL_UNIX_EVENT_EMITTER_H

#include "nel/misc/types_nl.h"
#include "nel/misc/event_emitter.h"

#ifdef NL_OS_UNIX

#include <GL/gl.h>
#include <GL/glx.h>

namespace NLMISC {


/**
 * TODO Class description
 * \author Vianney Lecroart
 * \author Nevrax France
 * \date 2000
 */
class CUnixEventEmitter : public IEventEmitter
{
public:

  /// Constructor
  CUnixEventEmitter();

  void init (Display *dpy, Window win);

	/**
	 * sends all events to server
	 * (should call CEventServer method postEvent() )
	 * \param server
	 */
	virtual void submitEvents(CEventServer & server, bool allWindows);

public:
	void processMessage (XEvent &event, CEventServer &server);

 private:
	Display *_dpy;
	Window   _win;
};


} // NLMISC

#endif // NL_OS_UNIX

#endif // NL_UNIX_EVENT_EMITTER_H

/* End of unix_event_emitter.h */
