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
#include "weather_function_params_sheet_base.h"
//
#include "nel/georges/u_form_elm.h"
#include "nel/georges/u_form.h"
#include "nel/georges/u_form_loader.h"



//=======================================================================
CWeatherFunctionParamsSheetBase::CWeatherFunctionParamsSheetBase()
:							DayLenght(24),
							CycleLenght(25),
							MinThunderPeriod(1.f),
							ThunderLenght(0.5f),
							CloudWindSpeedFactor(1.f),
							CloudMinSpeed(0.f)
{
}

//=======================================================================
void CWeatherFunctionParamsSheetBase::readGeorges(const NLGEORGES::UForm *form, const NLMISC::CSheetId &/* sheetId */)
{
	build( form->getRootNode() );
}


//=======================================================================
void CWeatherFunctionParamsSheetBase::build(const NLGEORGES::UFormElm &item)
{
	item.getValueByName(DayLenght, "DayNumHours");
	item.getValueByName(CycleLenght, "CycleLenght");
	item.getValueByName(MinThunderPeriod, "MinThunderPeriod");
	item.getValueByName(ThunderLenght, "ThunderLenght");
	item.getValueByName(CloudWindSpeedFactor, "CloudWindSpeedFactor");
	item.getValueByName(CloudMinSpeed, "CloudMinSpeed");
}

//=======================================================================
void CWeatherFunctionParamsSheetBase::serial(class NLMISC::IStream &f) throw(NLMISC::EStream)
{
	f.serial(DayLenght);
	f.serial(CycleLenght);
	f.serial(MinThunderPeriod);
	f.serial(ThunderLenght);
	f.serial(CloudWindSpeedFactor);
	f.serial(CloudMinSpeed);
}

//================================================================
void CWeatherFunctionParamsSheetBase::build(const std::string &sheetName)
{
	NLGEORGES::UFormLoader *loader = NLGEORGES::UFormLoader::createLoader();
	NLMISC::CSmartPtr<NLGEORGES::UForm> form = loader->loadForm(sheetName.c_str());
	if (form)
	{
		build(form->getRootNode());
	}
	else
	{
		nlwarning("Can't load form %s", sheetName.c_str ());
	}
	NLGEORGES::UFormLoader::releaseLoader(loader);
}

