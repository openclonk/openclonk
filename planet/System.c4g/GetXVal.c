/*--
		GetXVal.c
		Authors:
		
		Some functions which request information from the GetXVal family.
		Attention: These functions directly access internal values of the engine.
		The usage might, under certain circumstances, lead to desynchronisation in
		either network games or save games. The most dangerous functions are marked
		with //(!).
--*/


// GetDefCoreVal
global func GetDefCategory()    { return GetDefCoreVal("Category", "DefCore"); }
global func GetDefWidth()       { return GetDefCoreVal("Width", "DefCore"); }
global func GetDefHeight()      { return GetDefCoreVal("Height", "DefCore"); }
global func GetDefOffset(nr)   { return GetDefCoreVal("Offset", "DefCore", nr); }
global func GetDefValue()       { return GetDefCoreVal("Value", "DefCore"); }
global func GetDefMass()        { return GetDefCoreVal("Mass", "DefCore"); }
global func GetDefComponents(nr) { return GetDefCoreVal("Components", "DefCore", nr); }
global func GetDefCollection(nr) { return GetDefCoreVal("Collection", "DefCore", nr); }
global func GetDefFireTop()     { return GetDefCoreVal("FireTop", "DefCore"); }
global func GetDefPlacement()   { return GetDefCoreVal("Placement", "DefCore"); }
global func GetDefContactIncinerate() { return GetDefCoreVal("ContactIncinerate", "DefCore"); }
global func GetDefBlastIncinerate() { return GetDefCoreVal("BlastIncinerate", "DefCore"); }
global func GetDefBurnTo()      { return GetDefCoreVal("BurnTo", "DefCore"); }
global func GetDefBase()        { return GetDefCoreVal("Base", "DefCore"); }
global func GetDefLine()        { return GetDefCoreVal("Line", "DefCore"); }
global func GetDefLineConnect() { return GetDefCoreVal("Exclusive", "DefCore"); }
global func GetDefPrey()        { return GetDefCoreVal("Prey", "DefCore"); }
global func GetDefEdible()      { return GetDefCoreVal("Edible", "DefCore"); }
global func GetDefCrewMember()  { return GetDefCoreVal("Prey", "DefCore"); }
global func GetDefGrowth()      { return GetDefCoreVal("Growth", "DefCore"); }
global func GetDefRebuy()       { return GetDefCoreVal("Rebuy", "DefCore"); }
global func GetDefConstruction(){ return GetDefCoreVal("Construction", "DefCore"); }
global func GetDefConstructTo() { return GetDefCoreVal("ConstructTo", "DefCore"); }
global func GetDefGrab()        { return GetDefCoreVal("Grab", "DefCore"); }
global func GetDefGrabPutGet()  { return GetDefCoreVal("GrabPutGet", "DefCore"); }
global func GetDefCollectible() { return GetDefCoreVal("Collectible", "DefCore"); }
global func GetDefRotate()      { return GetDefCoreVal("Rotate", "DefCore"); }
global func GetDefChop()        { return GetDefCoreVal("Chop", "DefCore"); }
global func GetDefFloat()       { return GetDefCoreVal("Float", "DefCore"); }
global func GetDefContainBlast(){ return GetDefCoreVal("ContainBlast", "DefCore"); }
global func GetDefHorizontalFix() { return GetDefCoreVal("HorizontalFix", "DefCore"); }
global func GetDefBorderBound() { return GetDefCoreVal("BorderBound", "DefCore"); }
global func GetDefLiftTop()     { return GetDefCoreVal("LiftTop", "DefCore"); }
global func GetDefUprightAttach() { return GetDefCoreVal("UprightAttach", "DefCore"); }
global func GetDefStretchGrowth() { return GetDefCoreVal("StretchGrowth", "DefCore"); }
global func GetDefBasement()    { return GetDefCoreVal("Basement", "DefCore"); }
global func GetDefNoBurnDecay() { return GetDefCoreVal("NoBurnDecay", "DefCore"); }
global func GetDefIncompleteActivity()  { return GetDefCoreVal("IncompleteActivity", "DefCore"); }
global func GetDefAttractLightning()  { return GetDefCoreVal("AttractLightning", "DefCore"); }
global func GetDefOversize()    { return GetDefCoreVal("Oversize", "DefCore"); }
global func GetDefFragile()     { return GetDefCoreVal("Fragile", "DefCore"); }
global func GetDefExplosive()   { return GetDefCoreVal("Explosive", "DefCore"); }
global func GetDefProjectile()  { return GetDefCoreVal("Projectile", "DefCore"); }
global func GetDefNoPushEnter() { return GetDefCoreVal("NoPushEnter", "DefCore"); }
global func GetDefVehicleControl()  { return GetDefCoreVal("VehicleControl", "DefCore"); }
global func GetDefNoComponentMass() { return GetDefCoreVal("NoComponentMass", "DefCore"); }
global func GetDefClosedContainer() { return GetDefCoreVal("ClosedContainer", "DefCore"); }
global func GetDefSilentCommands() { return GetDefCoreVal("SilentCommands", "DefCore"); }
global func GetDefNoComponentMass() { return GetDefCoreVal("NoComponentMass", "DefCore"); }
global func GetDefNoBurnDamage(){ return GetDefCoreVal("NoBurnDamage", "DefCore"); }
global func GetDefTemporaryCrew() { return GetDefCoreVal("TemporaryCrew", "DefCore"); }
global func GetDefSmokeRate()   { return GetDefCoreVal("SmokeRate", "DefCore"); }
global func GetDefNoBreath()    { return GetDefCoreVal("NoBreath", "DefCore"); }
global func GetDefConSizeOff()  { return GetDefCoreVal("ConSizeOff", "DefCore"); }
global func GetDefNoSell()      { return GetDefCoreVal("NoSell", "DefCore"); }

// GetObjectVal
global func GetObjOwnMass()    { return GetObjectVal("OwnMass", 0); }
global func GetObjFixX()       { return GetObjectVal("FixX", 0); }
global func GetObjFixY()       { return GetObjectVal("FixY", 0); }
global func GetObjWidth()      { return GetObjectVal("Width", 0); }
global func GetObjHeight()     { return GetObjectVal("Height", 0); }
global func GetObjFireTop()    { return GetObjectVal("FireTop", 0); }
global func GetObjMobile()     { return GetObjectVal("Mobile", 0); }
global func GetObjOnFire()     { return GetObjectVal("OnFire", 0); }
global func GetObjInLiquid()   { return GetObjectVal("InLiquid", 0); }
global func GetObjEntranceStatus() { return GetObjectVal("EntranceStatus", 0); }
global func GetObjPhysicalTemporary() { return GetObjectVal("PhysicalTemporary", 0); }
global func GetObjNeedEnergy() { return GetObjectVal("NeedEnergy", 0); }
global func GetObjActionTime() { return GetObjectVal("ActionTime", 0); }
global func GetObjActionData() { return GetObjectVal("ActionData", 0); }
global func GetObjPhaseDelay() { return GetObjectVal("PhaseDelay", 0); }
global func GetObjActionTarget1() { return GetObjectVal("ActionTarget1", 0); }
global func GetObjActionTarget2() { return GetObjectVal("ActionTarget2", 0); }
global func GetObjPlrViewRange() { return GetObjectVal("PlrViewRange", 0); }

// GetPlayerVal
global func GetPlrClientNr(int plr)   { return GetPlayerVal("AtClient", 0, plr); }
global func GetPlrClientName(int plr) { return GetPlayerVal("AtClientName", 0, plr); }
//global func GetPlrColor(int plr)      { return GetPlayerVal("ColorDw", 0, plr); } - use GetPlayerColor
global func GetPlrViewX(int plr)      { return GetPlayerVal("ViewX", 0, plr); } //(!)
global func GetPlrViewY(int plr)      { return GetPlayerVal("ViewY", 0, plr); } //(!)
global func GetPlrFogOfWar(int plr)   { return GetPlayerVal("FogOfWar", 0, plr); } //(!)
global func GetPlrWealth(int plr)     { return GetPlayerVal("Wealth", 0, plr); }
global func GetPlrPoints(int plr)     { return GetPlayerVal("Points", 0, plr); }
global func GetPlrValue(int plr)      { return GetPlayerVal("Value", 0, plr); }
global func GetPlrInitialValue(int plr) { return GetPlayerVal("InitialValue", 0, plr); }
global func GetPlrValueGain(int plr)  { return GetPlayerVal("ValueGain", 0, plr); }
global func GetPlrObjectsOwned(int plr) { return GetPlayerVal("GetPlrObjectsOwned", 0, plr); }

// GetScenarioVal
global func GetScenTitle()     { return GetScenarioVal("Title", "Head"); } //(!)
global func GetScenVersion()   { return GetScenarioVal("Version", "Head"); } //(!)
global func GetScenMaxPlayer() { return GetScenarioVal("MaxPlayer", "Head"); }//(!)
global func GetScenSaveGame()  { return GetScenarioVal("SaveGame", "Head"); } //(!)
global func GetScenReplay()    { return GetScenarioVal("Replay", "Head"); } //(!)
global func GetScenFilm()      { return GetScenarioVal("Film", "Head"); } //(!)
global func GetScenMissionAccess() { return GetScenarioVal("MissionAccess", "Head"); } //(!)
global func GetScenNetworkGame() { return GetScenarioVal("NetworkGame", "Head"); } //(!)
global func GetScenNetworkClients() { return GetScenarioVal("StartupPlayerCount", "Head"); } //(!)
global func GetScenNoSky()     { return GetScenarioVal("NoSky", "Landscape"); }
global func GetScenBottomOpen() { return GetScenarioVal("BottomOpen", "Landscape"); }
global func GetScenTopOpen()   { return GetScenarioVal("TopOpen", "Landscape"); }
global func GetScenLeftOpen()  { return GetScenarioVal("LeftOpen", "Landscape"); }
global func GetScenRightOpen() { return GetScenarioVal("RightOpen", "Landscape"); }
global func GetScenMapWidth()  { return GetScenarioVal("MapWidth", "Landscape"); } //(!)
global func GetScenMapHeight() { return GetScenarioVal("MapHeight", "Landscape"); } //(!)
global func GetScenMapZoom()   { return GetScenarioVal("MapZoom", "Landscape"); } //(!)
global func GetScenClimate()   { return GetScenarioVal("Climate", "Weather"); }
global func GetScenYearSpeed() { return GetScenarioVal("YearSpeed", "Weather"); }
global func GetScenRain()      { return GetScenarioVal("Rain", "Weather"); }
global func GetScenWind()      { return GetScenarioVal("Wind", "Weather"); }
global func GetScenPrecipitation() { return GetScenarioVal("Precipitation", "Weather"); }

// GetObjectInfoCoreVal
global func GetObjCoreName()      { return GetObjectInfoCoreVal("Name", "ObjectInfo"); }
global func GetObjCoreDeathMessage() { return GetObjectInfoCoreVal("DeathMessage", "ObjectInfo"); }
global func GetObjCoreRank()      { return GetObjectInfoCoreVal("Rank", "ObjectInfo"); }
global func GetObjCoreRankName()  { return GetObjectInfoCoreVal("RankName", "ObjectInfo"); }
global func GetObjCoreTypeName()  { return GetObjectInfoCoreVal("TypeName", "ObjectInfo"); }
global func GetObjCoreParticipation() { return GetObjectInfoCoreVal("Participation", "ObjectInfo"); }
global func GetObjCoreExperience() { return GetObjectInfoCoreVal("Experience", "ObjectInfo"); }
global func GetObjCoreRounds()    { return GetObjectInfoCoreVal("Rounds", "ObjectInfo"); }
global func GetObjCoreDeathCount() { return GetObjectInfoCoreVal("DeathCount", "ObjectInfo"); }
global func GetObjCoreBirthday()  { return GetObjectInfoCoreVal("Birthday", "ObjectInfo"); }
global func GetObjCoreTotalPlayingTime() { return GetObjectInfoCoreVal("TotalPlayingTime", "ObjectInfo"); }
global func GetObjCoreRounds()    { return GetObjectInfoCoreVal("Rounds", "ObjectInfo"); }
global func GetAge()              { return GetObjectInfoCoreVal("Age", "ObjectInfo"); }

// GetPlayerInfoCoreVal
global func GetPlrCoreName(int plr)      { return GetPlayerInfoCoreVal("Name", "Player", plr); }
global func GetPlrCoreComment(int plr)   { return GetPlayerInfoCoreVal("Comment", "Player", plr); }
global func GetPlrCoreRank(int plr)      { return GetPlayerInfoCoreVal("Rank", "Player", plr); }
global func GetPlrCoreRankName(int plr)  { return GetPlayerInfoCoreVal("RankName", "Player", plr); }
global func GetPlrCoreScore(int plr)     { return GetPlayerInfoCoreVal("Score", "Player", plr); }
global func GetPlrCoreRounds(int plr)    { return GetPlayerInfoCoreVal("Rounds", "Player", plr); }
global func GetPlrCoreRoundsLost(int plr) { return GetPlayerInfoCoreVal("RoundsLost", "Player", plr); }
global func GetPlrCoreTotalPlayingTime(int plr) { return GetPlayerInfoCoreVal("TotalPlayingTime", "Player", plr); }
global func GetPlrCoreColor(int plr)     { return GetPlayerInfoCoreVal("Color", "Preferences", plr); }
global func GetPlrCoreControl(int plr)   { return GetPlayerInfoCoreVal("Control", "Preferences", plr); }
global func GetPlrCorePosition(int plr)  { return GetPlayerInfoCoreVal("Position", "Preferences", plr); }
global func GetPlrCoreJumpAndRunControl(int plr)    { return GetPlayerInfoCoreVal("AutoStopControl", "Preferences", plr); }
global func GetPlrLRTitle(int plr)       { return GetPlayerInfoCoreVal("Title", "LastRound", plr); }
global func GetPlrLRDate(int plr)        { return GetPlayerInfoCoreVal("Date", "LastRound", plr); }
global func GetPlrLRDuration(int plr)    { return GetPlayerInfoCoreVal("Duration", "LastRound", plr); }
global func GetPlrLRWon(int plr)         { return GetPlayerInfoCoreVal("Won", "LastRound", plr); }
global func GetPlrLRScore(int plr)       { return GetPlayerInfoCoreVal("Score", "LastRound", plr); }
global func GetPlrLRFinalScore(int plr)  { return GetPlayerInfoCoreVal("FinalScore", "LastRound", plr); }
global func GetPlrLRTotalScore(int plr)  { return GetPlayerInfoCoreVal("TotalScore", "LastRound", plr); }
global func GetPlrLRBonus(int plr)       { return GetPlayerInfoCoreVal("Bonus", "LastRound", plr); }
global func GetPlrLRLevel(int plr)       { return GetPlayerInfoCoreVal("Level", "LastRound", plr); }
