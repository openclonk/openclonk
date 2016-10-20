/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Controls temperature, wind, and natural disasters */

#include "C4Include.h"
#include "landscape/C4Weather.h"

#include "object/C4Object.h"
#include "lib/C4Random.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Game.h"
#include "platform/C4SoundSystem.h"
#include "graphics/C4Draw.h"

C4Weather::C4Weather()
{
	Default();
}

C4Weather::~C4Weather()
{
	Clear();
}

void C4Weather::Init(bool fScenario)
{
	if (fScenario)
	{
		// Season
		Season=Game.C4S.Weather.StartSeason.Evaluate();
		YearSpeed=Game.C4S.Weather.YearSpeed.Evaluate();
		// Temperature
		Climate=100-Game.C4S.Weather.Climate.Evaluate()-50;
		Temperature=Climate;
		// Wind
		Wind=TargetWind=Game.C4S.Weather.Wind.Evaluate();
		// gamma?
		NoGamma=Game.C4S.Weather.NoGamma;
	}
	// set gamma
	SetSeasonGamma();
}

void C4Weather::Execute()
{
	// Season
	if (!::Game.iTick35)
	{
		SeasonDelay+=YearSpeed;
		if (SeasonDelay>=200)
		{
			SeasonDelay=0;
			Season++;
			if (Season>Game.C4S.Weather.StartSeason.Max)
				Season=Game.C4S.Weather.StartSeason.Min;
			SetSeasonGamma();
		}
	}
	// Temperature
	if (!::Game.iTick35)
	{
		int32_t iTemperature = Climate - fixtoi(Cos(itofix(36 * Season, 10)), TemperatureRange);
		if (Temperature<iTemperature) Temperature++;
		else if (Temperature>iTemperature) Temperature--;
	}
	// Wind
	if (!::Game.iTick1000)
		TargetWind=Game.C4S.Weather.Wind.Evaluate();
	if (!::Game.iTick10)
		Wind=Clamp<int32_t>(Wind+Sign(TargetWind-Wind),
		                      Game.C4S.Weather.Wind.Min,
		                      Game.C4S.Weather.Wind.Max);
}

void C4Weather::Clear()
{

}

int32_t C4Weather::GetWind(int32_t x, int32_t y)
{
	if (Landscape.GetBackPix(x,y) != 0) return 0;
	return Wind;
}

int32_t C4Weather::GetTemperature()
{
	return Temperature;
}

void C4Weather::Default()
{
	Season=0; YearSpeed=0; SeasonDelay=0;
	Wind=TargetWind=0;
	Temperature=Climate=0;
	TemperatureRange=30;
	NoGamma=true;
}

void C4Weather::SetWind(int32_t iWind)
{
	Wind=Clamp<int32_t>(iWind,-100,+100);
	TargetWind=Clamp<int32_t>(iWind,-100,+100);
}

void C4Weather::SetTemperature(int32_t iTemperature)
{
	Temperature = Clamp<int32_t>(iTemperature,-100,100);
	SetSeasonGamma();
}

void C4Weather::SetSeason(int32_t iSeason)
{
	Season = Clamp<int32_t>(iSeason,0,100);
	SetSeasonGamma();
}

int32_t C4Weather::GetSeason()
{
	return Season;
}

void C4Weather::SetClimate(int32_t iClimate)
{
	Climate = Clamp<int32_t>(iClimate,-50,+50);
	SetSeasonGamma();
}

int32_t C4Weather::GetClimate()
{
	return Climate;
}

static float SeasonColors[4][3] =
{
	{ 0.90f, 0.90f, 1.00f }, // winter: slightly blue; blued out by temperature
	{ 1.00f, 1.05f, 0.90f }, // spring: green to yellow
	{ 1.00f, 1.00f, 1.00f }, // summer: regular ramp
	{ 1.00f, 0.95f, 0.90f }  // fall:   dark, brown ramp
};

void C4Weather::SetSeasonGamma()
{
	if (NoGamma) return;
	// get season num and offset
	int32_t iSeason1=(Season/25)%4; int32_t iSeason2=(iSeason1+1)%4;
	int32_t iSeasonOff1=Clamp(Season%25, 5, 19)-5; int32_t iSeasonOff2=15-iSeasonOff1;
	float gamma[3] = { 0.0f, 0.0f, 0.0f };
	// interpolate between season colors
	for (int32_t iChan=0; iChan<3; iChan+=8)
	{
		float c1 = SeasonColors[iSeason1][iChan],
		      c2 = SeasonColors[iSeason2][iChan];
		gamma[iChan] = (c1*iSeasonOff2 + c2*iSeasonOff1) / 15;
	}
	// apply gamma ramp
	pDraw->SetGamma(gamma[0], gamma[1], gamma[2], C4GRI_SEASON);
}

void C4Weather::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Season,           "Season",                0));
	pComp->Value(mkNamingAdapt(YearSpeed,        "YearSpeed",             0));
	pComp->Value(mkNamingAdapt(SeasonDelay,      "SeasonDelay",           0));
	pComp->Value(mkNamingAdapt(Wind,             "Wind",                  0));
	pComp->Value(mkNamingAdapt(TargetWind,       "TargetWind",            0));
	pComp->Value(mkNamingAdapt(Temperature,      "Temperature",           0));
	pComp->Value(mkNamingAdapt(TemperatureRange, "TemperatureRange",      30));
	pComp->Value(mkNamingAdapt(Climate,          "Climate",               0));
	pComp->Value(mkNamingAdapt(NoGamma,          "NoGamma",               0));

	int32_t gamma[C4MaxGammaRamps*3],
	        gammaDefaults[C4MaxGammaRamps*3];
	for (int32_t i=0; i<C4MaxGammaRamps; ++i)
	{
		gammaDefaults[i*3+0] = 100;
		gammaDefaults[i*3+1] = 100;
		gammaDefaults[i*3+2] = 100;
		gamma[i*3+0] = int(pDraw->gamma[i][0] * 100.0f);
		gamma[i*3+1] = int(pDraw->gamma[i][1] * 100.0f);
		gamma[i*3+2] = int(pDraw->gamma[i][2] * 100.0f);
	}
	pComp->Value(mkNamingAdapt(mkArrayAdaptM(gamma), "Gamma", gammaDefaults));
	for (int32_t i=0; i<C4MaxGammaRamps; ++i)
	{
		pDraw->gamma[i][0] = float(gamma[i*3+0]) / 100.0f;
		pDraw->gamma[i][1] = float(gamma[i*3+1]) / 100.0f;
		pDraw->gamma[i][2] = float(gamma[i*3+2]) / 100.0f;
	}
}

C4Weather Weather;
