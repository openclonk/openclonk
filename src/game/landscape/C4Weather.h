/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Controls temperature, wind, and natural disasters */

#ifndef INC_C4Weather
#define INC_C4Weather

#include <C4Landscape.h>
class C4Weather
  {
  public:
    C4Weather();
	  ~C4Weather();
  public:
    int32_t Season,YearSpeed,SeasonDelay;
    int32_t Wind,TargetWind;
    int32_t Temperature,TemperatureRange,Climate;
    int32_t MeteoriteLevel,VolcanoLevel,EarthquakeLevel,LightningLevel;
		int32_t NoGamma;
  public:
	  void Default();
		void Clear();
    void Execute();
	  void SetClimate(int32_t iClimate);
	  void SetSeason(int32_t iSeason);
	  void SetTemperature(int32_t iTemperature);
    void Init(bool fScenario);
	  void SetWind(int32_t iWind);
		int32_t GetWind(int32_t x, int32_t y);
		int32_t GetTemperature();
	  int32_t GetSeason();
	  int32_t GetClimate();
		bool LaunchLightning(int32_t x, int32_t y, int32_t xdir, int32_t xrange, int32_t ydir, int32_t yrange, bool fDoGamma);
		bool LaunchVolcano(int32_t mat, int32_t x, int32_t y, int32_t size);
	  bool LaunchEarthquake(int32_t iX, int32_t iY);
	  bool LaunchCloud(int32_t iX, int32_t iY, int32_t iWidth, int32_t iStrength, const char *szPrecipitation);
		void SetSeasonGamma();		// set gamma adjustment for season
    void CompileFunc(StdCompiler *pComp);
  };
extern C4Weather Weather;

inline int32_t GBackWind(int32_t x, int32_t y)
  {
  return GBackIFT(x, y) ? 0: ::Weather.Wind;
  }

#endif
