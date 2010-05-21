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

#include "nel/net/service.h"
#include "nel/misc/hierarchical_timer.h"
#include "nel/misc/log.h"
#include "nel/misc/command.h"

#include "game_share/utils.h"
#include "game_share/msg_client_server.h"

#include "pvp_manager/pvp_manager_2.h"
#include "pvp_manager/pvp_manager.h"
#include "pvp_manager/pvp_faction.h"
#include "pvp_manager/pvp.h"
#include "player_manager/player_manager.h"
#include "player_manager/player.h"
#include "player_manager/character.h"
#include "team_manager/team.h"
#include "team_manager/team_manager.h"
#include "egs_globals.h"
#include "stat_db.h"
#include "admin.h"

using namespace std;
using namespace NLMISC;
using namespace NLNET;

extern CGenericXmlMsgHeaderManager	GenericMsgManager;

CPVPManager2 * CPVPManager2::_Instance = NULL;

CVariable<bool>	DisablePVPDuel("egs", "DisablePVPDuel", "If true PVP duel is disabled", false, 0, true );
CVariable<uint32> FactionChannelHistoricSize("egs", "FactionChannelHistoricSize", "size of history for player faction channel", false, 1000, true );
CVariable<string> FactionChannelModeratorWriteRight("privilege", "FactionChannelModeratorWriteRight", "Privilege that writre in faction channel for moderator", string(":GM:SGM:DEV:"), 0, true);

extern CAdminCommand * findAdminCommand(const std::string & name);

NL_INSTANCE_COUNTER_IMPL(CPVPManager2);

//----------------------------------------------------------------------------
void CPVPManager2::init()
{
	BOMB_IF( _Instance != 0, "CPVPManager2 already allocated", return );
	_Instance = new CPVPManager2();
	BOMB_IF(_Instance == 0, "Can't allocate CPVPManager2 singleton", nlstop );

	///////////////////////////////////////////////////////////////////
	// *** in first step, we just consider PVP Faction and PVP duel ***
	///////////////////////////////////////////////////////////////////
	
	// instantiate pvp faction class
	IPVPInterface * pvpFaction = new CPVPFaction();
	BOMB_IF(pvpFaction == 0, "Can't allocate CPVPFaction", nlstop );
	_Instance->_PVPInterface.push_back(pvpFaction);
	// add war between kami and karavan faction (must be controled by GM tools later
	_Instance->addFactionWar(PVP_CLAN::Kami, PVP_CLAN::Karavan);
	
	// instantiate pvp duel class
	IPVPInterface * pvpDuel = new CPVPDuel();
	BOMB_IF(pvpDuel == 0, "Can't allocate CPVPDuel", nlstop );
	_Instance->_PVPInterface.push_back(pvpDuel);
}

//----------------------------------------------------------------------------
CPVPManager2* CPVPManager2::getInstance()
{
	if(CPVPManager2::_Instance == 0)
	{
		CPVPManager2::init();
	}
	return CPVPManager2::_Instance;
}

//----------------------------------------------------------------------------
void CPVPManager2::release()
{
	if( _Instance != 0 )
	{
		
		for( uint32 i = 0; i < _Instance->_PVPInterface.size(); ++i )
		{
			delete _Instance->_PVPInterface[i];
		}
		_Instance->_PVPInterface.clear();

		delete _Instance;
		_Instance = 0;
	}
}

//----------------------------------------------------------------------------
CPVPManager2::~CPVPManager2()
{
	_FactionWarOccurs.clear();
	_SafeZones.clear();
}

//----------------------------------------------------------------------------
void CPVPManager2::tickUpdate()
{
	SM_STATIC_PARAMS_1(params, STRING_MANAGER::player);
	
	// remove expired duel propositions
	while( !_DuelsAsked.empty() &&  _DuelsAsked.front().ExpirationDate <= CTickEventHandler::getGameCycle()     )
	{
		// close interface
		CMessage msgout( "IMPULSION_ID" );
		CEntityId targetId = getEntityIdFromRow (_DuelsAsked.front().Invited);
		msgout.serial( targetId );
		CBitMemStream bms;
		nlverify ( GenericMsgManager.pushNameToStream( "DUEL:CANCEL_INVITATION", bms) );
		msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
		sendMessageViaMirror( TServiceId(targetId.getDynamicId()), msgout );
		
		// send chat infos
		CEntityId userId = getEntityIdFromRow (_DuelsAsked.front().Invitor);
		params[0].setEIdAIAlias( targetId, CAIAliasTranslator::getInstance()->getAIAlias(targetId) );
		CCharacter::sendDynamicSystemMessage( userId, "DUEL_INVITATION_EXPIRE",params);
		params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
		CCharacter::sendDynamicSystemMessage( targetId, "DUEL_INVITATION_EXPIRE",params);
		// remove duel			
		_DuelsAsked.erase(_DuelsAsked.begin() );
	}
	
	CPVPManager::getInstance()->tickUpdate();
}

//----------------------------------------------------------------------------
TChanID CPVPManager2::getFactionDynChannel( PVP_CLAN::TPVPClan faction )
{
	TMAPFactionChannel::iterator it = _FactionChannel.find( faction );
	if( it != _FactionChannel.end() )
	{
		return (*it).second;
	}
	return DYN_CHAT_INVALID_CHAN;
}

//----------------------------------------------------------------------------
TChanID CPVPManager2::getCharacterChannel(CCharacter * user)
{
//	if( user->getPVPFlag(false) ) // new specs: we not need be tagged for have channel, only allegiance is required.
	{
		PVP_CLAN::TPVPClan faction = user->getAllegiance().first;
		if( faction != PVP_CLAN::Neutral )
		{
			if( isFactionInWar(faction) )
			{
				TMAPFactionChannel::iterator it = _FactionChannel.find(faction);
				if( it != _FactionChannel.end() )
				{
					return (*it).second;
				}
			}
		}
	}
	return DYN_CHAT_INVALID_CHAN;
}

//----------------------------------------------------------------------------
TChanID CPVPManager2::getCharacterRegisteredChannel(CCharacter * user)
{
	map< NLMISC::CEntityId, TChanID >::iterator it = _CharacterChannel.find(user->getId());
	if( it != _CharacterChannel.end() )
	{
		return (*it).second;
	}
	return DYN_CHAT_INVALID_CHAN;
}

//----------------------------------------------------------------------------
void CPVPManager2::addOrRemoveFactionChannel(CCharacter * user, bool b )
{
	TChanID channelHave = getCharacterRegisteredChannel(user);
	TChanID channelMustHave = getCharacterChannel(user);
	if( channelHave != DYN_CHAT_INVALID_CHAN )
	{
		if( channelHave != channelMustHave)
		{
			removeFactionChannelForCharacter(user);
		}
	}

	if( channelMustHave != DYN_CHAT_INVALID_CHAN )
	{
		if(channelMustHave != channelHave )
		{
			addFactionChannelToCharacter(user);
		}
	}
	if( b )
		addRemoveFactionChannelToUserWithPriviledge(user);
}

//----------------------------------------------------------------------------
void CPVPManager2::addFactionChannelToCharacter(CCharacter * user)
{
	TChanID channel = getCharacterChannel(user);
	if( channel != DYN_CHAT_INVALID_CHAN )
	{
		DynChatEGS.addSession( channel, user->getEntityRowId(), true);
		_CharacterChannel.insert( make_pair(user->getId(), channel) );
	}
}

//----------------------------------------------------------------------------
void CPVPManager2::removeFactionChannelForCharacter(CCharacter * user)
{
	TChanID channel = getCharacterRegisteredChannel(user);
	if( channel != DYN_CHAT_INVALID_CHAN )
	{
		DynChatEGS.removeSession(channel, user->getEntityRowId());
		std::map< NLMISC::CEntityId, TChanID >::iterator it = _CharacterChannel.find(user->getId());
		if( it != _CharacterChannel.end() )
		{
			_CharacterChannel.erase(it);
		}
	}
}

//----------------------------------------------------------------------------
void CPVPManager2::addRemoveFactionChannelToUserWithPriviledge(CCharacter * user)
{
	const CAdminCommand * cmd = findAdminCommand("ShowFactionChannels");
	if (!cmd)
	{
		return;
	}
	
	if (!user->havePriv(cmd->Priv))
	{
		return;
	}

	if( user->showFactionChannelsMode() )
	{
		removeFactionChannelForCharacter(user);

		bool writeRight = user->havePriv(FactionChannelModeratorWriteRight);
		for( uint32 i = PVP_CLAN::BeginClans; i <= PVP_CLAN::EndClans; ++i )
		{
			if( isFactionInWar((PVP_CLAN::TPVPClan)i) )
			{
				TMAPFactionChannel::iterator it = _FactionChannel.find((PVP_CLAN::TPVPClan)i);
				if( it != _FactionChannel.end() )
				{
					DynChatEGS.addSession((*it).second, user->getEntityRowId(), writeRight);
				}
			}
		}
	}
	else
	{
		for( uint32 i = PVP_CLAN::BeginClans; i <= PVP_CLAN::EndClans; ++i )
		{
			if( isFactionInWar((PVP_CLAN::TPVPClan)i) )
			{
				TMAPFactionChannel::iterator it = _FactionChannel.find((PVP_CLAN::TPVPClan)i);
				if( it != _FactionChannel.end() )
				{
					DynChatEGS.removeSession((*it).second, user->getEntityRowId());
				}
			}
		}
		addOrRemoveFactionChannel( user, false );
	}
}

//----------------------------------------------------------------------------
void CPVPManager2::playerDisconnects(CCharacter * user)
{
	nlassert(user);
	removeDuelInvitor(user->getId());
	endDuel(user, "DUEL_DISCONNECT", "");

	CPVPManager::getInstance()->playerDisconnects(user);

	removeFactionChannelForCharacter(user);
}

//----------------------------------------------------------------------------
void CPVPManager2::playerDies(CCharacter * user)
{
	nlassert(user);
	endDuel(user, "DUEL_LOST", "DUEL_WON");
}

//----------------------------------------------------------------------------
void CPVPManager2::playerTeleports(CCharacter * user)
{
	nlassert(user);
	endDuel(user, "DUEL_YOU_TELEPORT", "DUEL_TELEPORT");
}

//----------------------------------------------------------------------------
void CPVPManager2::setPVPModeInMirror( const CCharacter * user ) const
{
	nlassert(user);
	uint8 pvpMode = 0;
	
	// faction
	{
		if( user->getPVPFlag(false) )
		{
			if( user->getPvPRecentActionFlag() )
			{
				pvpMode |= PVP_MODE::PvpFactionFlagged;
			}
			else
			{
				pvpMode |= PVP_MODE::PvpFaction;
			}
		}
	}
	// duel
	{
		if( user->getDuelOpponent() != NULL )
		{
			pvpMode |= PVP_MODE::PvpDuel;
		}
	}
	// pvp session (i.e everything else)
	{
		if ( user->getPVPInterface().isValid() )
		{
			CPVPInterface interf = user->getPVPInterface();
			pvpMode |= interf.getPVPSession()->getPVPMode();
		}
	}
	
	CMirrorPropValue<TYPE_PVP_MODE> propPvpMode( TheDataset, user->getEntityRowId(), DSPropertyPVP_MODE );
	propPvpMode = pvpMode;
}

//----------------------------------------------------------------------------
void CPVPManager2::addPVPSafeZone( NLMISC::CSmartPtr<CPVPSafeZone> safeZone )
{
	nlassert( !safeZone.isNull() );

	for (uint i = 0; i < _SafeZones.size(); i++)
	{
		if ( safeZone->getAlias() == _SafeZones[i]->getAlias() )
			return;
	}
	_SafeZones.push_back( safeZone );
}

//----------------------------------------------------------------------------
bool CPVPManager2::inSafeZone(const NLMISC::CVector & v) const
{
	CVector v2D = v;
	v2D.z = 0.0f;
	// safe zones are excluded from the PVP zone
	for (uint i = 0; i < _SafeZones.size(); i++)
	{
		if ( _SafeZones[i]->contains(v2D) )
			return true;
	}
	return false;
}

//----------------------------------------------------------------------------
PVP_RELATION::TPVPRelation CPVPManager2::getPVPRelation( CCharacter * actor, CEntityBase * target, bool curative ) const
{
	PVP_RELATION::TPVPRelation relation = PVP_RELATION::Neutral;

	// init reminders, these help to know if faction pvp recent action flag need to be set
	_Instance->_PVPFactionAllyReminder = false;
	_Instance->_PVPFactionEnemyReminder = false;
	
	// init reminders, these help to know if outpost leaving timer must be reset
	_Instance->_PVPOutpostAllyReminder = false;
	_Instance->_PVPOutpostEnemyReminder = false;
	
	/// temp : until new manager is finished we have to check the char pvp session too
	CCharacter * pTarget = dynamic_cast<CCharacter*>(target);
	if( pTarget )
	{
		if( IsRingShard )
			return relation; // disable PVP on Ring shards (only if target is a CCharacter, because we must let attack NPCs)

		IPVP * pvpSession = pTarget->getPVPInterface().getPVPSession();
		if( pvpSession )
		{
			relation = pvpSession->getPVPRelation( actor, target );
			if( relation == PVP_RELATION::Ennemy )
			{
				pvpSession = actor->getPVPInterface().getPVPSession();
				if( pvpSession )
				{
					if( pvpSession->getPVPRelation( pTarget, actor )  == PVP_RELATION::Ennemy )
					{
						if( CPVPManager2::getInstance()->inSafeZone( pTarget->getPosition() ) && pTarget->getSafeInPvPSafeZone() )
						{
							relation = PVP_RELATION::NeutralPVP;
						} 
						else if( CPVPManager2::getInstance()->inSafeZone( actor->getPosition() ) && actor->getSafeInPvPSafeZone() )
						{
							relation = PVP_RELATION::NeutralPVP;
						}
						else
						{
							return PVP_RELATION::Ennemy;
						}
					}
				}
			}
		}
	}

	PVP_RELATION::TPVPRelation relationTmp = PVP_RELATION::Neutral;
	uint i;
	for( i=0; i<_PVPInterface.size(); ++i )
	{
		relationTmp = _PVPInterface[i]->getPVPRelation( actor, target, curative );
		
		if( relationTmp == PVP_RELATION::Unknown )
			return PVP_RELATION::Unknown;

		// ennemy has the highest priority
		if( relationTmp == PVP_RELATION::Ennemy )
			return PVP_RELATION::Ennemy;
		// neutral pvp
		if( relationTmp == PVP_RELATION::NeutralPVP )
			relation = PVP_RELATION::NeutralPVP;
		// check if ally (neutralpvp has priority over ally)
		if( relationTmp == PVP_RELATION::Ally && relation != PVP_RELATION::NeutralPVP )
			relation = PVP_RELATION::Ally;
	}

	return relation;
}


//----------------------------------------------------------------------------
bool CPVPManager2::isCurativeActionValid( CCharacter * actor, CEntityBase * target, bool checkMode ) const
{
	nlassert(actor);
	nlassert(target);

	PVP_RELATION::TPVPRelation pvpRelation = getPVPRelation( actor, target, true );
	bool actionValid;
	switch( pvpRelation )
	{
		case PVP_RELATION::Ally :
			actionValid = true;
			break;
		case PVP_RELATION::Ennemy : // nb: a faction pvp adversary is not an enemy if he's not flagged so he can be heal
			actionValid = false;
			if( !checkMode )
				CCharacter::sendDynamicSystemMessage(actor->getEntityRowId(), "PVP_CANT_HELP_ENEMY");
			break;
		case PVP_RELATION::Neutral :
			actionValid = true;
			break;
		case PVP_RELATION::NeutralPVP :
			actionValid = false;
			if( !checkMode )
				CCharacter::sendDynamicSystemMessage(actor->getEntityRowId(), "PVP_CANT_HELP_NEUTRAL_PVP");
			break;
		default:
			actionValid = false;
			if( !checkMode )
				CCharacter::sendDynamicSystemMessage(actor->getEntityRowId(), "PVP_CANT_HELP_UNKNOWN");
	}

	if( actionValid && !checkMode )
	{
		if( pvpRelation == PVP_RELATION::Ally )
		{
			CCharacter * pTarget = dynamic_cast<CCharacter*>(target);
			if(pTarget)
				actor->clearSafeInPvPSafeZone();
			// propagate faction pvp flag
			if( _PVPFactionAllyReminder )
			{
				if( pTarget )
				{
					if( pTarget->getPvPRecentActionFlag() )
					{
						actor->setPVPRecentActionFlag();
						TeamManager.pvpHelpOccursInTeam( actor, pTarget );
						actor->pvpActionMade();
						pTarget->pvpActionMade();
					}
				}
			}
		
			// stop outpost leaving timer
			if( _PVPOutpostAllyReminder )
			{
				actor->refreshOutpostLeavingTimer();
			}
		}
	}
	return actionValid;
}


//----------------------------------------------------------------------------
bool CPVPManager2::isOffensiveActionValid( CCharacter * actor, CEntityBase * target, bool checkMode ) const
{
	nlassert(actor);
	nlassert(target);

	// cannot hurt a dead entity
	if ( target->isDead() )
		return false;

	PVP_RELATION::TPVPRelation pvpRelation = getPVPRelation( actor, target );
	bool actionValid;
	switch( pvpRelation )
	{
		case PVP_RELATION::Ally :
			actionValid = false;
			if( !checkMode )
				CCharacter::sendDynamicSystemMessage(actor->getEntityRowId(), "PVP_CANT_ATTACK_ALLY");
			break;
		case PVP_RELATION::Ennemy :
			actionValid = true;
			break;
		case PVP_RELATION::Neutral :
			actionValid = false;
			if( !checkMode )
				CCharacter::sendDynamicSystemMessage(actor->getEntityRowId(), "PVP_CANT_ATTACK_NEUTRAL");
			break;
		case PVP_RELATION::NeutralPVP :
			actionValid = false;
			if( !checkMode )
				CCharacter::sendDynamicSystemMessage(actor->getEntityRowId(), "PVP_CANT_ATTACK_NEUTRAL");
			break;
		default:
			actionValid = false;
	}
	
	if( actionValid && !checkMode )
	{
		CCharacter * pTarget = dynamic_cast<CCharacter*>(target);
		if(pTarget)
			actor->clearSafeInPvPSafeZone();
		if( _PVPFactionEnemyReminder )
		{
			actor->setPVPRecentActionFlag();
			if( pTarget )
			{
				TeamManager.pvpAttackOccursInTeam( actor, pTarget );
				actor->pvpActionMade();
				pTarget->pvpActionMade();
			}
		}

		if( _PVPOutpostEnemyReminder )
		{
			actor->refreshOutpostLeavingTimer();
		}
	}
	
	return actionValid;
}

//----------------------------------------------------------------------------
bool CPVPManager2::canApplyAreaEffect(CCharacter* actor, CEntityBase * areaTarget, bool offensive, bool ignoreMainTarget) const
{
	nlassert(actor);
	nlassert(areaTarget);

	// cannot hurt a dead entity
	if( offensive )
		if ( areaTarget->isDead() )
			return false;

	PVP_RELATION::TPVPRelation pvpRelation = getPVPRelation( actor, areaTarget, !offensive );
	bool actionValid;
	switch( pvpRelation )
	{
		case PVP_RELATION::Ally :
			actionValid = !offensive;
			break;
		case PVP_RELATION::Ennemy :
			actionValid = offensive;
			break;
		case PVP_RELATION::Neutral :
			actionValid = !offensive;
			break;
		case PVP_RELATION::NeutralPVP :
			actionValid = false;
			break;
		default:
			actionValid = offensive;
	}
	
	if( actionValid )
	{
		if( areaTarget->getId().getType() == RYZOMID::player )
		{
			CCharacter * pTarget = dynamic_cast<CCharacter*>(areaTarget);
			if(pTarget)
				actor->clearSafeInPvPSafeZone();
			// set faction flag
			if( offensive && _PVPFactionEnemyReminder )
			{
				if(pTarget)
				{
					actor->setPVPRecentActionFlag();
					TeamManager.pvpAttackOccursInTeam( actor, pTarget );
				}
			}
			// propagate faction flag
			if( !offensive && _PVPFactionAllyReminder )
			{
				if( pTarget )
				{
					if( pTarget->getPvPRecentActionFlag() )
					{
						actor->setPVPRecentActionFlag();
						TeamManager.pvpHelpOccursInTeam( actor, pTarget );
					}
				}
			}
			// stop outpost leaving timer
			if( (offensive && _PVPOutpostEnemyReminder) || (!offensive && _PVPOutpostAllyReminder) )
			{
				actor->refreshOutpostLeavingTimer();
			}
		}
	}
	
	return actionValid;
}

//----------------------------------------------------------------------------
bool CPVPManager2::isTPValid( CCharacter * actor, CGameItemPtr TeleportTicket ) const
{
	uint i;
	for( i=0; i<_PVPInterface.size(); ++i )
	{
		if( _PVPInterface[i]->isTPValid(actor, TeleportTicket) == false )
		{
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------
bool CPVPManager2::isRespawnValid( CCharacter * actor, CCharacterRespawnPoints::TRespawnPoint respawnPoint ) const
{
	uint i;
	for( i=0; i<_PVPInterface.size(); ++i )
	{
		if( !_PVPInterface[i]->isRespawnValid(actor, respawnPoint) )
		{
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------
void CPVPManager2::finalBlowerKillerInPvPFaction( CCharacter * killer, PVP_CLAN::TPVPClan finalBlowerFaction, CCharacter * victimChar ) const
{
	//////////////////////////////////////////////////////
	// *** in first step, we just consider PVP Faction ***
	//////////////////////////////////////////////////////
	_PVPInterface[0]->finalBlowerKillerInPvPFaction(killer, finalBlowerFaction, victimChar );
}

//----------------------------------------------------------------------------
void CPVPManager2::characterKillerInPvPFaction( CCharacter * character, PVP_CLAN::TPVPClan winnerFaction, sint32 factionPoint ) const
{
	//////////////////////////////////////////////////////
	// *** in first step, we just consider PVP Faction ***
	//////////////////////////////////////////////////////
	_PVPInterface[0]->characterKillerInPvPFaction( character, winnerFaction, factionPoint );
}

//----------------------------------------------------------------------------
void CPVPManager2::characterKilledInPvPFaction( CCharacter * character, PVP_CLAN::TPVPClan looserFaction, sint32 factionPoint ) const
{
	//////////////////////////////////////////////////////
	// *** in first step, we just consider PVP Faction ***
	//////////////////////////////////////////////////////
	_PVPInterface[0]->characterKilledInPvPFaction( character, looserFaction, factionPoint );
	character->getRespawnPoints().resetUserDb();
}

//----------------------------------------------------------------------------
bool CPVPManager2::addFactionWar( PVP_CLAN::TPVPClan clan1, PVP_CLAN::TPVPClan clan2 )
{
	uint32 factionWarOccursSize = (uint32)_FactionWarOccurs.size();
	for( uint32 i = 0; i < factionWarOccursSize; ++i )
	{
		if( _FactionWarOccurs[ i ].inPvPFaction( clan1, clan2 ) )
			return false;
	}
	PVP_CLAN::CFactionWar factionWar;
	factionWar.Clan1 = clan1;
	factionWar.Clan2 = clan2;
	_FactionWarOccurs.push_back( factionWar );

//	// create dynamic channel for faction war if not already exist
//	createFactionChannel(clan1);
//	createFactionChannel(clan2);
//
//	// send start of faction war to all clients, and add faction channel to character with concerned allegiance
//	for( CPlayerManager::TMapPlayers::const_iterator it = PlayerManager.getPlayers().begin(); it != PlayerManager.getPlayers().end(); ++it )
//	{
//		CPlayerManager::SCPlayer scPlayer=(*it).second;
//		
//		if (scPlayer.Player)
//		{
//			CCharacter	*activePlayer=scPlayer.Player->getActiveCharacter();
//			if (activePlayer)
//			{
//				CMessage msgout( "IMPULSION_ID" );
//				CEntityId id = activePlayer->getId();
//				msgout.serial( id );
//				CBitMemStream bms;
//				nlverify ( GenericMsgManager.pushNameToStream( "PVP_FACTION:PUSH_FACTION_WAR", bms) );
//				bms.serialEnum( clan1 );
//				bms.serialEnum( clan2 );
//				msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
//				sendMessageViaMirror( NLNET::TServiceId(id.getDynamicId()), msgout );
//
//				// add faction channel to character if needed
//				addFactionChannelToCharacter( activePlayer );
//				addRemoveFactionChannelToUserWithPriviledge( activePlayer );
//			}
//		}
//	}

	return true;
}

/// create the faction chat channel when IOS mirror ready
void CPVPManager2::onIOSMirrorUp()
{
	// for each pvp war, create the related char channel
	for (uint i=0; i<_FactionWarOccurs.size(); ++i)
	{
		PVP_CLAN::TPVPClan clan1 = _FactionWarOccurs[i].Clan1;
		PVP_CLAN::TPVPClan clan2 = _FactionWarOccurs[i].Clan2;

		// create dynamic channel for faction war if not already exist
		createFactionChannel(clan1);
		createFactionChannel(clan2);

		// send start of faction war to all clients, and add faction channel to character with concerned allegiance
		for( CPlayerManager::TMapPlayers::const_iterator it = PlayerManager.getPlayers().begin(); it != PlayerManager.getPlayers().end(); ++it )
		{
			CPlayerManager::SCPlayer scPlayer=(*it).second;
			
			if (scPlayer.Player)
			{
				CCharacter	*activePlayer=scPlayer.Player->getActiveCharacter();
				if (activePlayer)
				{
					CMessage msgout( "IMPULSION_ID" );
					CEntityId id = activePlayer->getId();
					msgout.serial( id );
					CBitMemStream bms;
					nlverify ( GenericMsgManager.pushNameToStream( "PVP_FACTION:PUSH_FACTION_WAR", bms) );
					bms.serialEnum( clan1 );
					bms.serialEnum( clan2 );
					msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
					sendMessageViaMirror( NLNET::TServiceId(id.getDynamicId()), msgout );

					// add faction channel to character if needed
					addFactionChannelToCharacter( activePlayer );
					addRemoveFactionChannelToUserWithPriviledge( activePlayer );
				}
			}
		}
	}
}


//----------------------------------------------------------------------------
bool CPVPManager2::stopFactionWar( PVP_CLAN::TPVPClan clan1, PVP_CLAN::TPVPClan clan2 )
{
	vector< PVP_CLAN::CFactionWar >::iterator it;
	for( it=_FactionWarOccurs.begin(); it!=_FactionWarOccurs.end(); ++it )
	{
		if( (*it).inPvPFaction( clan1, clan2 ) )
		{
			// send end of faction war to all clients, and remove faction channel if needed
			for( CPlayerManager::TMapPlayers::const_iterator it2 = PlayerManager.getPlayers().begin(); it2 != PlayerManager.getPlayers().end(); ++it2 )
			{
				CPlayerManager::SCPlayer scPlayer=(*it2).second;
				
				if (scPlayer.Player)
				{
					CCharacter	*activePlayer=scPlayer.Player->getActiveCharacter();
					if (activePlayer)
					{
						CMessage msgout( "IMPULSION_ID" );
						CEntityId id = activePlayer->getId();
						msgout.serial( id );
						CBitMemStream bms;
						nlverify ( GenericMsgManager.pushNameToStream( "PVP_FACTION:POP_FACTION_WAR", bms) );
						bms.serialEnum( clan1 );
						bms.serialEnum( clan2 );
						msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
						sendMessageViaMirror( NLNET::TServiceId(id.getDynamicId()), msgout );				

						removeFactionChannelForCharacter(activePlayer);
					}
				}
			}

			// erase war
			_FactionWarOccurs.erase(it);
			removeFactionChannel(clan1);
			removeFactionChannel(clan2);
			return true;
		}
	}
	return false;
}

//----------------------------------------------------------------------------
void CPVPManager2::createFactionChannel(PVP_CLAN::TPVPClan clan)
{
	TMAPFactionChannel::iterator it = _FactionChannel.find(clan);
	if( it == _FactionChannel.end() )
	{
		ucstring title;
		string name = NLMISC::strupr( string("Faction_") + PVP_CLAN::toString(clan) );
//		title.fromUtf8(name);
//		TChanID factionChannelId = DynChatEGS.addChan(name, title );
		TChanID factionChannelId = DynChatEGS.addLocalizedChan(name);
		// set historic size of the newly created channel
		DynChatEGS.setHistoricSize( factionChannelId, FactionChannelHistoricSize );

		_FactionChannel.insert( make_pair(clan, factionChannelId) );
	}
}

//----------------------------------------------------------------------------
void CPVPManager2::removeFactionChannel(PVP_CLAN::TPVPClan clan)
{
	for( uint32 i = 0; i < _FactionWarOccurs.size(); ++i )
	{
		if( _FactionWarOccurs[ i ].Clan1 == clan || _FactionWarOccurs[ i ].Clan2 == clan)
			return;
	}

	TMAPFactionChannel::iterator it = _FactionChannel.find( clan );
	if( it != _FactionChannel.end() )
	{
		DynChatEGS.removeChan( (*it).second );
		_FactionChannel.erase(it);
	}
}

//----------------------------------------------------------------------------
bool CPVPManager2::factionWarOccurs( PVP_CLAN::TPVPClan clan1, PVP_CLAN::TPVPClan clan2 ) const
{
	if( clan1 != PVP_CLAN::Neutral && clan2 != PVP_CLAN::Neutral )
	{
		uint32 factionWarOccursSize = (uint32)_FactionWarOccurs.size();
		for( uint32 i = 0; i < factionWarOccursSize; ++i )
		{
			if( _FactionWarOccurs[ i ].inPvPFaction( clan1, clan2 ) )
				return true;
		}
	}
	return false;
}

//----------------------------------------------------------------------------
bool CPVPManager2::factionWarOccurs( pair<PVP_CLAN::TPVPClan, PVP_CLAN::TPVPClan> allegiance1, pair<PVP_CLAN::TPVPClan, PVP_CLAN::TPVPClan> allegiance2 ) const
{
	bool warOccurs = false;
	if( allegiance1.first != PVP_CLAN::Neutral )
	{
		if( allegiance2.first != PVP_CLAN::Neutral && allegiance1.first != allegiance2. first )
			warOccurs |= factionWarOccurs( allegiance1.first, allegiance2.first );
		if( allegiance2.second != PVP_CLAN::Neutral && allegiance1.first != allegiance2.second )
			warOccurs |= factionWarOccurs( allegiance1.first, allegiance2.second );
	}
	if( allegiance1.second != PVP_CLAN::Neutral )
	{
		if( allegiance2.first != PVP_CLAN::Neutral && allegiance1.second != allegiance2.first )
			warOccurs |= factionWarOccurs( allegiance1.second, allegiance2.first );
		if( allegiance2.second != PVP_CLAN::Neutral && allegiance1.second != allegiance2.second )
			warOccurs |= factionWarOccurs( allegiance1.second, allegiance2.second );
	}
	return warOccurs;
}

//----------------------------------------------------------------------------
bool CPVPManager2::isFactionInWar( PVP_CLAN::TPVPClan clan )
{
	uint32 factionWarOccursSize = (uint32)_FactionWarOccurs.size();
	for( uint32 i = 0; i < factionWarOccursSize; ++i )
	{
		if( _FactionWarOccurs[ i ].Clan1 == clan || _FactionWarOccurs[ i ].Clan2 == clan )
			return true;
	}
	return false;
}

//-----------------------------------------------
//	sendFactionWarsToClient
//
//-----------------------------------------------
void CPVPManager2::sendFactionWarsToClient( CCharacter * user )
{
	nlassert(user);

	CMessage msgout( "IMPULSION_ID" );
	CEntityId id = user->getId();
	msgout.serial( id );

	CBitMemStream bms;
	if ( ! GenericMsgManager.pushNameToStream( "PVP_FACTION:FACTION_WARS", bms) )
	{
		nlwarning("<CPVPManager2::sendFactionWarsToClient> Msg name PVP_FACTION:FACTION_WARS not found");
		return;
	}
	
	CFactionWarsMsg factionWarsMsg;
	for( uint i=0; i<_FactionWarOccurs.size(); ++i )
	{
		factionWarsMsg.FactionWarOccurs.push_back( _FactionWarOccurs[i] );
	}
	bms.serial(factionWarsMsg);

	msgout.serialBufferWithSize( (uint8*)bms.buffer(), bms.length() );
	CUnifiedNetwork::getInstance()->send( NLNET::TServiceId(id.getDynamicId()), msgout );
}


//==========================================================================================


//-----------------------------------------------
//	askForDuel
//
//-----------------------------------------------
void CPVPManager2::askForDuel( const NLMISC::CEntityId & userId )
{
	if (DisablePVPDuel.get())
	{
		CCharacter::sendDynamicSystemMessage(userId, "PVP_DUEL_DISABLED");
		return;
	}

	// global chat parmeters
	SM_STATIC_PARAMS_1(params, STRING_MANAGER::player);

	// get protagonists
	CCharacter * user = PlayerManager.getChar( userId );
	if ( !user )
	{
		nlwarning("<CPVPManager2::askForDuel>'%s' is invalid",userId.toString().c_str() );
		return;
	}
	CCharacter * target = PlayerManager.getChar( user->getTarget() );
	if ( !target )
	{
		nlwarning("<CPVPManager2::askForDuel>'%s' has an invalid target",userId.toString().c_str() );
		return;
	}

	// test if duel is allowed in this place
	CMirrorPropValueRO<TYPE_CELL> mirrorCell( TheDataset, user->getEntityRowId(), DSPropertyCELL );
	sint32 cell1 = mirrorCell;	
	CMirrorPropValueRO<TYPE_CELL> mirrorCell2( TheDataset, target->getEntityRowId(), DSPropertyCELL );
	sint32 cell2 = mirrorCell2;	

	if  (cell1 <= -2 || cell2 <= -2 )
	{
		CCharacter::sendDynamicSystemMessage(userId, "DUEL_FORBIDDEN_PLACE");
		return;
	}

	// if target is already in duel then user cannot invite him
	if ( target->getDuelOpponent() != NULL )
	{	
		params[0].setEIdAIAlias( target->getId(), CAIAliasTranslator::getInstance()->getAIAlias(target->getId()) );
		CCharacter::sendDynamicSystemMessage( userId, "DUEL_TARGET_ALREADY_INVITED",params);
		return;
	}

	// remove previous invitation, and check that user is not invited
	bool problem = false;

	TDataSetRow userRow = TheDataset.getDataSetRow( userId );
	for ( std::list< CDuelAsked >::iterator it = _DuelsAsked.begin(); it != _DuelsAsked.end(); )
	{
		if ( (*it).Invitor == userRow )
		{
			// ignore same invitation
			if ( (*it).Invited == target->getEntityRowId() )
				return;
			// user already invited someone
			// send cancel message to the invited
			CMessage msgout( "IMPULSION_ID" );
			CEntityId msgId = getEntityIdFromRow( (*it).Invited );
			msgout.serial( msgId );
			CBitMemStream bms;
			nlverify ( GenericMsgManager.pushNameToStream( "DUEL:CANCEL_INVITATION", bms) );
			msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
			sendMessageViaMirror( NLNET::TServiceId(msgId.getDynamicId()), msgout );

			//send chat infos
			params[0].setEIdAIAlias( msgId, CAIAliasTranslator::getInstance()->getAIAlias(msgId) );
			CCharacter::sendDynamicSystemMessage( userRow, "DUEL_YOU_CANCEL_INVITATION",params);
			params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
			CCharacter::sendDynamicSystemMessage( (*it).Invited, "DUEL_CANCEL_INVITATION",params);

			// remove this proposition
			std::list< CDuelAsked >::iterator itErase = it;
			++it;
			_DuelsAsked.erase(itErase);

		}
		else
		{
			if ( (*it).Invited == user->getEntityRowId() )
			{	
				// user is already invited : he has to accept or refuse first
				CCharacter::sendDynamicSystemMessage( userId, "DUEL_ALREADY_INVITED",params);
				// dont bail out as we can enter in case "if ( (*it).Invitor == userId )"
				problem = true;
			}
			if ( (*it).Invited == target->getEntityRowId() )
			{	
				// target is already invited : he has to accept or refuse first
				params[0].setEIdAIAlias( target->getId(), CAIAliasTranslator::getInstance()->getAIAlias(target->getId()) );
				CCharacter::sendDynamicSystemMessage( userId, "DUEL_TARGET_ALREADY_INVITED",params);
				// dont bail out as we can enter in case "if ( (*it).Invitor == userId )"
				problem = true;
			}
			if ( (*it).Invitor == target->getEntityRowId() )
			{	
				// target is inviting someone
				params[0].setEIdAIAlias( target->getId(), CAIAliasTranslator::getInstance()->getAIAlias(target->getId()) );
				CCharacter::sendDynamicSystemMessage( userId, "DUEL_TARGET_ALREADY_INVITED",params);
				// dont bail out as we can enter in case "if ( (*it).Invitor == userId )"
				problem = true;
			}
			++it;
		}
	}
	// problem occured : bail out
	if ( problem )
		return;
	
	// create a new entry
	CDuelAsked duelAsked;
	duelAsked.Invitor = user->getEntityRowId();
	duelAsked.Invited = target->getEntityRowId();
	duelAsked.ExpirationDate = CTickEventHandler::getGameCycle() + DuelQueryDuration;
	_DuelsAsked.push_front( duelAsked );
	
	// tell invited player
	params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
	uint32 txt = STRING_MANAGER::sendStringToClient( target->getEntityRowId(), "DUEL_INVITATION", params );
	CMessage msgout( "IMPULSION_ID" );
	msgout.serial( const_cast<CEntityId&>(target->getId()) );
	CBitMemStream bms;
	nlverify ( GenericMsgManager.pushNameToStream( "DUEL:INVITATION", bms) );
	bms.serial( txt );
	msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
	sendMessageViaMirror( NLNET::TServiceId(target->getId().getDynamicId()), msgout );

} // askForDuel //


//-----------------------------------------------
//	refuseDuel
//
//-----------------------------------------------
void CPVPManager2::refuseDuel( const NLMISC::CEntityId & userId )
{
	SM_STATIC_PARAMS_1(params, STRING_MANAGER::player);
	params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
	TDataSetRow row = TheDataset.getDataSetRow( userId );
	// find the proposition
	for ( std::list< CDuelAsked >::iterator it = _DuelsAsked.begin(); it != _DuelsAsked.end();++it )
	{
		if ( (*it).Invited == row )
		{
			CCharacter::sendDynamicSystemMessage( (*it).Invitor, "DUEL_REFUSE_INVITATION", params);
			_DuelsAsked.erase(it);
			return;
		}
	}
	
} // refuseDuel //


//-----------------------------------------------
//	abandonDuel
//
//-----------------------------------------------
void CPVPManager2::abandonDuel( const NLMISC::CEntityId & userId )
{
	CCharacter * user = PlayerManager.getChar( userId ); 
	if ( user )
	{
		endDuel(user, "DUEL_YOU_ABANDON", "DUEL_ABANDON");
	}
	
} // abandonDuel //


//-----------------------------------------------
//	acceptDuel
//
//-----------------------------------------------
void CPVPManager2::acceptDuel( const NLMISC::CEntityId & userId )
{
	SM_STATIC_PARAMS_1(params, STRING_MANAGER::player);
	
	CCharacter * invited = PlayerManager.getChar(userId ); 
	if ( !invited )
	{
		nlwarning("<PVP>invalid user %s", userId.toString().c_str() );
		return;
	}
	for ( std::list< CDuelAsked >::iterator it = _DuelsAsked.begin(); it != _DuelsAsked.end(); )
	{
		// we have to find the user as the invited of a duel to start the duel
		if ( (*it).Invited == invited->getEntityRowId() )
		{
			CCharacter * invitor = PlayerManager.getChar( (*it).Invitor );
			if ( !invitor )
			{
				nlwarning("<PVP>invalid invitor %s", getEntityIdFromRow( (*it).Invitor ).toString().c_str() );
				_DuelsAsked.erase(it);
				return;
			}
			
			params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
			CCharacter::sendDynamicSystemMessage( invitor->getId(), "DUEL_ACCEPTED", params);
			params[0].setEIdAIAlias( invitor->getId(), CAIAliasTranslator::getInstance()->getAIAlias(invitor->getId()) );
			CCharacter::sendDynamicSystemMessage( userId, "DUEL_ACCEPTED", params);

			invitor->setDuelOpponent( invited );
			invited->setDuelOpponent( invitor );
			
			std::list< CDuelAsked >::iterator itErase = it;
			++it;
			_DuelsAsked.erase(itErase);

//			invitor->_PropertyDatabase.setProp("USER:IN_DUEL",true );
			CBankAccessor_PLR::getUSER().setIN_DUEL(invitor->_PropertyDatabase, true);
//			invited->_PropertyDatabase.setProp("USER:IN_DUEL",true );
			CBankAccessor_PLR::getUSER().setIN_DUEL(invited->_PropertyDatabase, true);
			invitor->updateTarget();
			invited->updateTarget();
		}
		// if the user proposed a duel, cancel the proposition
		else if ( (*it).Invitor == invited->getEntityRowId() )
		{
			params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
			CCharacter::sendDynamicSystemMessage( (*it).Invitor, "DUEL_CANCEL_INVITATION",params);
			std::list< CDuelAsked >::iterator itErase = it;
			++it;
			_DuelsAsked.erase(itErase);
		}
		else
			++it;
	}

} // acceptDuel //


//-----------------------------------------------
//	removeDuelInvitor
//
//-----------------------------------------------
void CPVPManager2::removeDuelInvitor( const NLMISC::CEntityId & userId )
{
	SM_STATIC_PARAMS_1(params, STRING_MANAGER::player);
	params[0].setEIdAIAlias( userId, CAIAliasTranslator::getInstance()->getAIAlias(userId) );
	TDataSetRow row = TheDataset.getDataSetRow( userId );
	// find invitation, send messages and remove it
	for ( std::list< CDuelAsked >::iterator it = _DuelsAsked.begin(); it != _DuelsAsked.end();++it )
	{
		if ( (*it).Invitor == row )
		{	
			CMessage msgout( "IMPULSION_ID" );
			CEntityId msgId = getEntityIdFromRow ( (*it).Invited );
			msgout.serial( msgId );
			CBitMemStream bms;
			nlverify ( GenericMsgManager.pushNameToStream( "DUEL:CANCEL_INVITATION", bms) );
			msgout.serialBufferWithSize((uint8*)bms.buffer(), bms.length());
			sendMessageViaMirror( NLNET::TServiceId(msgId.getDynamicId()), msgout );
			
			CCharacter::sendDynamicSystemMessage( msgId, "DUEL_CANCEL_INVITATION",params);
			_DuelsAsked.erase(it);
			return;
		}
	}

} // removeDuelInvitor //



//-----------------------------------------------
//	endDuel
//
//-----------------------------------------------
void CPVPManager2::endDuel( CCharacter * user, const string& userTxt, const string& opponentTxt )
{
	CCharacter * duelOpponent = user->getDuelOpponent();
	if ( duelOpponent )
	{
		if( !userTxt.empty() )
			CCharacter::sendDynamicSystemMessage( user->getEntityRowId(), userTxt );
		if( !opponentTxt.empty() )
			CCharacter::sendDynamicSystemMessage( duelOpponent->getEntityRowId(), opponentTxt );
		
//		duelOpponent->_PropertyDatabase.setProp("USER:IN_DUEL",false );
		CBankAccessor_PLR::getUSER().setIN_DUEL(duelOpponent->_PropertyDatabase, false );
	//	duelOpponent->removeAllSpells();  //< Spells should not be removed
	//	duelOpponent->stopAllLinks(1.0f); //< Offensive links are automatically broken when PvP state change
		duelOpponent->setDuelOpponent( NULL );
				
//		user->_PropertyDatabase.setProp("USER:IN_DUEL",false );
		CBankAccessor_PLR::getUSER().setIN_DUEL(user->_PropertyDatabase, false );
	//	user->removeAllSpells();  //< Spells should not be removed
	//	user->stopAllLinks(1.0f); //< Offensive links are automatically broken when PvP state change
		user->setDuelOpponent( NULL );

		CPVPManager2::getInstance()->setPVPModeInMirror( duelOpponent );
		CPVPManager2::getInstance()->setPVPModeInMirror( user );
	}

} // endDuel //





//----------------------------------------------------------------------------
NLMISC_COMMAND(setFactionWar, "Start/stop current wars between faction", "<Faction1 Name> <Faction2 Name> <0/1 for peace or war>")
{
	if (args.size() != 3)
		return false;

	PVP_CLAN::TPVPClan faction1 = PVP_CLAN::fromString( args[ 0 ] );
	PVP_CLAN::TPVPClan faction2 = PVP_CLAN::fromString( args[ 1 ] );
	uint32 war = (uint32)atoi(args[ 2 ].c_str());

	if( faction1 < PVP_CLAN::BeginClans || faction1 > PVP_CLAN::EndClans )
	{
		log.displayNL("Invalid Faction1 name: '%s'", args[0].c_str());
		return false;
	}

	if( faction2 < PVP_CLAN::BeginClans || faction2 > PVP_CLAN::EndClans )
	{
		log.displayNL("Invalid Faction2 name: '%s'", args[1].c_str());
		return false;
	}

	if( war )
	{
		if( CPVPManager2::getInstance()->addFactionWar( faction1, faction2 ) == false )
			log.displayNL("Faction % and faction %s already in war", args[0].c_str(), args[1].c_str());
	}
	else
	{
		if( CPVPManager2::getInstance()->stopFactionWar( faction1, faction2 ) == false )
			log.displayNL("Faction % and faction %s are not in war", args[0].c_str(), args[1].c_str());
	}
	return true;
}
