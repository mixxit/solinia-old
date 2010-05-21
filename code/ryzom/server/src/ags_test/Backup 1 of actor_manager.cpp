/** \file actor_manager.cpp
 * 
 * the actor manager for AgS_Test
 *
 * $Id: actor_manager.cpp,v 1.34 2004/06/14 12:34:17 brigand Exp $
 * $Log: actor_manager.cpp,v $
 * Revision 1.34  2004/06/14 12:34:17  brigand
 * CHANGED : updated to new PDS people enum
 *
 * Revision 1.33  2004/03/01 19:18:37  lecroart
 * REMOVED: bad headers
 *
 * Revision 1.32  2004/01/13 18:47:07  cado
 * ADDED: Mirror tests
 *
 * Revision 1.31  2003/10/21 08:48:19  legros
 * ADDED: multi target commands
 *
 * Revision 1.30  2003/10/02 16:13:15  legros
 * no message
 *
 * Revision 1.29  2003/10/01 14:29:32  legros
 * no message
 *
 * Revision 1.28  2003/09/30 16:03:15  legros
 * CHANGED: ags test is now up to date...
 *
 * Revision 1.27  2003/07/16 15:51:19  cado
 * CHANGED: First-pass to re-adapt to mirror system V1.4
 *
 * Revision 1.26  2003/07/16 14:34:06  boucher
 * no message
 *
 * Revision 1.25  2003/02/26 10:24:19  cado
 * CHANGED: changed all properties to new mirror, no more old mirror
 *
 * Revision 1.24  2003/02/24 10:44:44  cado
 * ADDED: Adapted for new mirror system
 *
 * Revision 1.23  2003/01/28 20:36:51  miller
 * Changed header comments from 'Nel Network Services' to Ryzom
 *
 * Revision 1.22  2002/11/29 10:17:50  portier
 * #ADDED: ia_player.id
 *
 * Revision 1.21.2.1  2003/01/03 18:18:02  cado
 * ADDED: Integration with new mirror system
 *
 * Revision 1.21  2002/11/15 16:22:42  fleury
 * CHANGED : the OPS has been replaced by the EGS
 *
 * Revision 1.20  2002/08/30 08:47:34  miller
 * another quick test (non destructive)
 *
 * Revision 1.19  2002/08/30 08:46:09  miller
 * quick test (non destructive)
 *
 */



// Nel
//#include "nel/net/service.h"


// Local headers
#include "position_generator.h"
#include "actor_manager.h"
#include "sheets.h"
#include <nel/net/unified_network.h>
#include "game_share/people.h"
#include "game_share/synchronised_message.h"

#include "nel/misc/variable.h"

#include <set>

using namespace NLMISC;
using namespace NLNET;
using namespace std;


namespace AGS_TEST
{

//-------------------------------------------------------------------------
// Some global variables
extern uint32 GlobalActorCount;
extern uint32 GlobalActorUpdates;
extern uint32 GlobalActorMoves;

CEntityId	LastActorId;

NLMISC_DYNVARIABLE(string, LastActorId, "Last created actor")
{
	if (get)
		*pointer = LastActorId.toString();
	else
		LastActorId.fromString((*pointer).c_str());
}

//-------------------------------------------------------------------------
// Some function prototypes

static void cbServiceUp( const string& serviceName, uint16 serviceId, void * );
static void cbServiceDown( const string& serviceName, uint16 serviceId, void * );

//------------------------------------------------------------------------
// static data for CActorManager

std::vector<CActor*>		CActorManager::_actors;
std::vector<CActorGroup*>	CActorManager::_actorGroups;
int							CActorManager::_nextActorID=0;
std::set<uint>				CActorManager::_visionHandlingServices;
CActorGroup					*CActorManager::_defaultActorGroup;

//------------------------------------------------------------------------
// basic functionality for CActorManager singleton

void CActorManager::init()
{
	static bool first_time=true;

	if (first_time)
	{
		// Init callback for service up / down
		CUnifiedNetwork::getInstance()->setServiceUpCallback ("*", cbServiceUp, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback("*", cbServiceDown, NULL);
		first_time=false;
	}
	else
		release();

	_nextActorID=0;
/*
	CPositionGenerator::setPattern(std::string("grid"));
	CPositionGenerator::setPosition(17981,-33200);
	CPositionGenerator::setSpacing(2000);
*/
	_defaultActorGroup = newActorGroup("defaultGroup");
}

void CActorManager::update()
{
	set< pair<sint32, sint32> >	iaZones;

	std::vector<CActorGroup*>::iterator	itg;
	for (itg=_actorGroups.begin(); itg!=_actorGroups.end(); ++itg)
		(*itg)->update();


	// iterate through all known actors, calling their update
	uint16 positionChangeCount=0;
	uint16 updateCount=0;
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		bool updated= (*it)->update();
		if (updated)
			updateCount++;
/*
		if ((*it)->positionChanged())
			positionChangeCount++;

		sint32	zx = (*it)->getX()/160000;
		sint32	zy = -(*it)->getY()/160000;

		iaZones.insert(make_pair<sint32,sint32>(zx, zy));
*/
	}


	// for all entities, request vision zones
	/*CMessage	msg("AGENT_VISON");
	sint64		fakeId = 0;
	msg.serial(fakeId);
	sint32		pos = msg.getPos();
	set< pair<sint32, sint32> >::iterator	itz;
	for (itz=iaZones.begin(); itz!=iaZones.end(); ++itz)
	{
		sint32	x = (*itz).first,
				y = (*itz).second;
		msg.serial(x);
		msg.serial(y);
	}
	if (msg.getPos() > pos)
		CUnifiedNetwork::getInstance()->send("GPMS", msg);*/
/*
	// build the position changed message and send it to GPMS
	if (positionChangeCount>0)
	{
		NLNET::CMessage gpmPosMsg("UPDATE_ENTITIES_POSITIONS");
		gpmPosMsg.serial(positionChangeCount);
		for (it=_actors.begin(); it!=_actors.end(); ++it)
			(*it)->addPositionChangesToMessage(gpmPosMsg);

		sendMessageViaMirror( "GPMS", gpmPosMsg );
	}
*/
	GlobalActorUpdates=updateCount;
	//GlobalActorMoves=positionChangeCount;
}

void CActorManager::release()
{
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		(*it)->removeFromOtherServices();
	}
	_actors.clear();
}

// Callback called at service connexion
static void cbServiceUp( const string& serviceName, uint16 serviceId, void * )
{
	/*if (serviceName==std::string("EGS"))
		CActorManager::reconnectEGS((uint8)serviceId);

	if (serviceName==std::string("IOS"))
		CActorManager::reconnectIOS((uint8)serviceId);

	if (serviceName==std::string("GPMS"))
		CActorManager::reconnectGPMS((uint8)serviceId);*/

	/*CMessage	reqVision("ASK_VISION_ZONE");
	CUnifiedNetwork::getInstance()->send(serviceId, reqVision);*/
}

//
static void cbServiceDown( const string& serviceName, uint16 serviceId, void * )
{
	CActorManager::removeVisionService(serviceId);
}

//---------------------------------------------------------------------------------
// methods for dealing with tardy connection of a key service
/*void CActorManager::reconnectEGS(uint8 serviceId)
{
}

void CActorManager::reconnectIOS(uint8 serviceId)
{
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		(*it)->addToIOS(serviceId);
	}
}

void CActorManager::reconnectGPMS(uint8 serviceId)
{
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		(*it)->addToGPMS(serviceId);
	}
}*/


//---------------------------------------------------------------------------------
// CActorManager methods for creating and killing actors

// method for adding a new actor to the scene
CActor *CActorManager::newActor(const std::string &type, const std::string &name)
{
	LastActorId = CEntityId::Unknown;

	if (getActor(name)!=NULL)
	{
		nlinfo("Actor already exists: %s",name.c_str());
		return NULL;
	}

	CSheetId				sheetId(type);
	const CSheets::CSheet	*sheet=CSheets::lookup(sheetId);
	if (!sheet)
	{
		nlwarning("ERROR: Can't find static type data for '%s'(%d) for entity %s", type.c_str(), sheetId.asInt(), name.c_str());
		return NULL;
	}

	// create a new actor record and COPY it into the actor vector

//	EGSPD::CPeople::TPeople p_type = EGSPD::CPeople::fromString( type );
	bool success = false;
	CActor *aNewActor;
	if ( sheet->isNpc /*EGSPD::CPeople::isPlayableRace( p_type )*/ )
		aNewActor = new CActor(type,name,CEntityId(RYZOMID::npc,_nextActorID++),success);
	else
		aNewActor = new CActor(type,name,CEntityId(RYZOMID::creature,_nextActorID++),success);

	if ( ! success )
	{
		if (aNewActor)
			delete aNewActor;
		return NULL;
	}

	LastActorId = aNewActor->_id;

	int x,y;
	CPositionGenerator::next(x,y);
	aNewActor->setPos(x, y, 0);
	aNewActor->setAngle(0);
	_actors.push_back(aNewActor);

	// get hold of a pointer of the copy of the actor in the actor vector
	CActor *theActor=getActor(name);

	// add the actor to the GPMS, the IOS, etc
	if (theActor!=0)
		theActor->addToOtherServices();
/*
	CMessage	msgMode("SET_MODE");
	TDataSetRow		index = CMirrors::DataSet->getDataSetRow(theActor->_id);
	msgMode.serial(index);
	MBEHAV::TMode	mode(MBEHAV::NORMAL, x, y);
	msgMode.serial(mode);
	sendMessageViaMirror("EGS", msgMode);
*/
	CMirrors::initSheet( aNewActor->getSid(), sheetId ); // initialize out the sheet
	// Let the position & angle & tick be sent ("UPDATE_ENTITIES_POSITIONS") in update
	//aNewActor->initMode();

	MBEHAV::TMode	mode(MBEHAV::NORMAL, x, y);
	theActor->_mode = mode.RawModeAndParam;

	aNewActor->display();
	theActor->setGroup(_defaultActorGroup);

	GlobalActorCount++;
	return theActor;
}

// method for retrievng a pointer to a named actor
// returns NULL if the actor doesn't exist
CActor *CActorManager::getActor(const std::string &name)
{
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		if ((*it)->getName()==name)
			return (*it);
	}

	return 0;
}

// method for retrievng a pointer to given actor
CActor *CActorManager::getActor(unsigned index)
{
	if (index>=_actors.size())
		return NULL;
	else
		return (_actors[index]);
}

// method for retrievng a pointer to given actor
CActor *CActorManager::getActor(const NLMISC::CEntityId &id)
{
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		if ((*it)->getSid()==id)
			return (*it);
	}

	return NULL;
}

void   CActorManager::killActor(const std::string &name)
{
	std::vector<CActor*>::iterator it;
	for (it=_actors.begin(); it!=_actors.end(); ++it)
	{
		if ((*it)->getName()==name)
		{
			// remove from net refferences
			(*it)->removeFromOtherServices();

			// remove from vision xrefs
			std::vector<CActor*>::iterator it2;
			for (it2=_actors.begin(); it2!=_actors.end(); ++it2)
			{
				(*it2)->removeRefs(*it);
			}

			for (int i=_actorGroups.size();i--;)
				_actorGroups[i]->removeActor(*it);

			// remove from container
			delete *it;
			_actors.erase(it);
			GlobalActorCount--;
			return;
		}
	}
}

//------------------------------------------------------------------------
// basic functionality for actor group management

// method for adding a new actorGroup to the scene
CActorGroup *CActorManager::newActorGroup(const std::string &name)
{
	if (!getActorGroup(name))
	{
		// create a new actorGroup record and COPY it into the actorGroup vector
		CActorGroup *aNewActorGroup = new CActorGroup(name);
		_actorGroups.push_back(aNewActorGroup);
		aNewActorGroup->display();
	}

	// get hold of a pointer of the copy of the actorGroup in the actorGroup vector
	return getActorGroup(name);
}

// method for retrievng a pointer to a named actorGroup
// returns NULL if the actorGroup doesn't exist
CActorGroup *CActorManager::getActorGroup(const std::string &name)
{
	std::vector<CActorGroup*>::iterator it;
	for (it=_actorGroups.begin(); it!=_actorGroups.end(); ++it)
	{
		if ((*it)->getName()==name)
			return (*it);
	}

	return NULL;
}

// method for retrievng a pointer to given actorGroup
CActorGroup *CActorManager::getActorGroup(unsigned index)
{
	if (index>=_actorGroups.size())
		return NULL;
	else
		return (_actorGroups[index]);
}

void   CActorManager::removeActorGroup(const std::string &name)
{
	std::vector<CActorGroup*>::iterator it;
	for (it=_actorGroups.begin(); it!=_actorGroups.end(); ++it)
	{
		if ((*it)->getName()==name)
		{
			// remove all actors from this group into default group
			uint	i;
			for (i=0; i<(*it)->actorCount(); ++i)
				(*(*it))[i]->setGroup(_defaultActorGroup);

			// remove from container
			delete (*it);
			_actorGroups.erase(it);
			return;
		}
	}
}

//
void	CActorManager::setActorsToGroup(const std::string &sourceActorGroup, const std::string &destActorGroup)
{
	CActorGroup	*srcGroup = getActorGroup(sourceActorGroup);
	if (srcGroup == NULL)
	{
		nlwarning("source actor group '%s' is unknown, abort setActorsToGroup(%s, %s)", sourceActorGroup.c_str(), sourceActorGroup.c_str(), destActorGroup.c_str());
		return;
	}

	CActorGroup	*destGroup = getActorGroup(destActorGroup);
	if (destGroup == NULL)
	{
		nlwarning("destination actor group '%s' is unknown, abort setActorsToGroup(%s, %s)", destActorGroup.c_str(), sourceActorGroup.c_str(), destActorGroup.c_str());
		return;
	}

	uint	i;
	for (i=0; i<srcGroup->actorCount(); ++i)
		(*srcGroup)[i]->setGroup(destGroup);
}


} // end of namespace AGS_TEST
