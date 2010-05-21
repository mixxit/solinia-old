
#include "stdpch.h"
#include "game_share/power_types.h"
#include "game_share/brick_flags.h"
#include "powers_and_auras.h"
#include "egs_sheets/egs_static_game_item.h"


//-----------------------------------------------------------------------------
// CPowerActivationDate
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CPowerActivationDate::serial(NLMISC::IStream &f) throw(NLMISC::EStream)
{
	f.serial( DeactivationDate );
	f.serial( ActivationDate );
	if (f.isReading())
	{
		std::string tmp;
		f.serial( tmp);
		PowerType = POWERS::toPowerType(tmp);
	}
	else
	{
		std::string tmp = POWERS::toString(PowerType);
		f.serial( tmp);
	}
}

//-----------------------------------------------------------------------------
// CPowerActivationDateVector
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CPowerActivationDateVector::clear()
{
	PowerActivationDates.clear();
}

//-----------------------------------------------------------------------------
void CPowerActivationDateVector::clearConsumable()
{
	for(sint32 i = (sint32)PowerActivationDates.size()-1; i >= 0; --i )
	{
		if (PowerActivationDates[i].ConsumableFamilyId != (uint16)~0)
		{
			PowerActivationDates[i] = PowerActivationDates[PowerActivationDates.size()-1];
			PowerActivationDates.pop_back();
		}
	}
}

//-----------------------------------------------------------------------------
void CPowerActivationDateVector::serial(NLMISC::IStream &f) throw(NLMISC::EStream)
{
	if (f.isReading())
	{
		f.serialCont(PowerActivationDates);
		cleanVector();
	}
	else
	{
		cleanVector();
		f.serialCont(PowerActivationDates);
	}
}

//-----------------------------------------------------------------------------
void CPowerActivationDateVector::cleanVector()
{
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = PowerActivationDates.begin();

	while (it != PowerActivationDates.end())
	{
		if ((*it).ActivationDate <= time)
		{
			// erase returns an iterator that designates the first element remaining beyond any elements removed, or end() if no such element exists.
			it = PowerActivationDates.erase(it);
		}
		else
			++it;
	}
}

//-----------------------------------------------------------------------------
bool CPowerActivationDateVector::isPowerAllowed(POWERS::TPowerType type, uint16 consumableFamilyId, NLMISC::TGameCycle &endDate)
{
	bool result = true;
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = PowerActivationDates.begin();
	
	while (it != PowerActivationDates.end())
	{
		if ( (*it).ActivationDate <= time )
		{
			// erase returns an iterator that designates the first element remaining beyond any elements removed, or end() if no such element exists.
			it = PowerActivationDates.erase(it);
		}
		else
		{
			if ( (*it).PowerType == type || ((*it).ConsumableFamilyId != (uint16)~0 && (*it).ConsumableFamilyId == consumableFamilyId))
			{
				endDate = (*it).ActivationDate;
				result = false;
			}
			++it;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
void CPowerActivationDateVector::writeUsablePowerFlags( uint32 &usablePowerFlags)
{
	usablePowerFlags = 0xffffffff;
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = PowerActivationDates.begin();
	while (it != PowerActivationDates.end())
	{
		if ( (*it).ActivationDate <= time && doNotClear == false )
		{
			// erase returns an iterator that designates the first element remaining beyond any elements removed, or end() if no such element exists.
			it = PowerActivationDates.erase(it);
		}
		else
		{
			usablePowerFlags &= ~(1 << (BRICK_FLAGS::powerTypeToFlag((*it).PowerType) - BRICK_FLAGS::BeginPowerFlags) );
			++it;
		}
	}
}

//-----------------------------------------------------------------------------
void CPowerActivationDateVector::activate()
{
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = PowerActivationDates.begin();
	
	while (it != PowerActivationDates.end())
	{
		(*it).DeactivationDate = time - (*it).DeactivationDate; // the value saved is time length since deactivation, here we transform length into a date
		(*it).ActivationDate += time;
		++it;
	}
	doNotClear = false;
}

//-----------------------------------------------------------------------------
// CAuraActivationDateVector
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CAuraActivationDateVector::clear()
{
	_AuraActivationDates.clear();
	_AuraUsers.clear();
}

//-----------------------------------------------------------------------------
void CAuraActivationDateVector::disableAura(POWERS::TPowerType type, NLMISC::TGameCycle startDate, NLMISC::TGameCycle endDate, const NLMISC::CEntityId &userId)
{
	_AuraActivationDates.push_back( CPowerActivationDate(type,(uint16)~0, startDate, endDate) );
	_AuraUsers.push_back(userId);
}

//-----------------------------------------------------------------------------
void CAuraActivationDateVector::serial(NLMISC::IStream &f) throw(NLMISC::EStream)
{
	if (f.isReading())
	{
		f.serialCont(_AuraActivationDates);						
		_AuraUsers.resize(_AuraActivationDates.size(), NLMISC::CEntityId::Unknown);
		cleanVector();
	}
	else
	{
		cleanVector();
		f.serialCont(_AuraActivationDates);
	}
}

//-----------------------------------------------------------------------------
void CAuraActivationDateVector::cleanVector()
{
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = _AuraActivationDates.begin();
	std::vector <NLMISC::CEntityId>::iterator itUser = _AuraUsers.begin();
	
	while (it != _AuraActivationDates.end())
	{
		if ((*it).ActivationDate <= time)
		{
			// erase returns an iterator that designates the first element remaining beyond any elements removed, or end() if no such element exists.
			it = _AuraActivationDates.erase(it);
			itUser = _AuraUsers.erase(itUser);
		}
		else
		{
			++it;++itUser;
		}
	}
}

//-----------------------------------------------------------------------------
bool CAuraActivationDateVector::isAuraEffective(POWERS::TPowerType type, const NLMISC::CEntityId &user, NLMISC::TGameCycle &endDate)
{
	bool result = true;
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = _AuraActivationDates.begin();
	std::vector <NLMISC::CEntityId>::iterator itUser = _AuraUsers.begin();
	
	while (it != _AuraActivationDates.end())
	{
		if ( (*it).ActivationDate <= time )
		{
			// erase returns an iterator that designates the first element remaining beyond any elements removed, or end() if no such element exists.
			it = _AuraActivationDates.erase(it);
			itUser = _AuraUsers.erase(itUser);
		}
		else
		{
			if ((*it).PowerType == type && (*itUser) != user)
			{
				endDate = (*it).ActivationDate;
				result = false;
			}
			++it;
			++itUser;
		}
	}
	return result;
}


//-----------------------------------------------------------------------------
void CAuraActivationDateVector::activate()
{
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CPowerActivationDate>::iterator it = _AuraActivationDates.begin();
	
	while (it != _AuraActivationDates.end())
	{
		(*it).ActivationDate += time;
		++it;
	}
}

//-----------------------------------------------------------------------------
// CConsumableOverdoseTimerVector
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool CConsumableOverdoseTimerVector::cleanVector()
{
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CConsumableOverdoseTimer>::iterator it = Dates.begin();

	bool entryRemoved = false;

	while (it != Dates.end())
	{
		if ((*it).ActivationDate <= time)
		{
			it = Dates.erase(it);
			entryRemoved = true;
		}
		else
			++it;
	}
	return entryRemoved;
}

//-----------------------------------------------------------------------------
bool CConsumableOverdoseTimerVector::canConsume(uint16 family, NLMISC::TGameCycle &endDate)
{
	bool result = true;
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CConsumableOverdoseTimer>::iterator it = Dates.begin();
	
	while (it != Dates.end())
	{
		if ( (*it).ActivationDate <= time )
			it = Dates.erase(it);
		else
		{
			if ((*it).Family == family)
			{
				endDate = (*it).ActivationDate;
				result = false;
			}
			++it;
		}
	}
	
	return result;
}

//-----------------------------------------------------------------------------
void CConsumableOverdoseTimerVector::activate()
{
	const NLMISC::TGameCycle time = CTickEventHandler::getGameCycle();
	std::vector <CConsumableOverdoseTimer>::iterator it = Dates.begin();
	
	while (it != Dates.end())
	{
		(*it).ActivationDate += time;
		++it;
	}
}
