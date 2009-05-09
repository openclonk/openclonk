
#strict 2

// *** einige Funktionen, die Informationen über die
//     GetXVal-Familie abfragen

// Achtung: Diese Funktionen greifen direkt auf interne Werte der Engine zu. Die Benutzung
//          kann unter Umständen zu Desynchronisation in Netzwerkspielen oder aufgenommenen
//          Spielen führen. Einige besonders gefährliche Funktionen sind im Folgenden mit
//          "/*(!)*/" gekennzeichnet.

// GetDefCoreVal

global func GetDefCategory(id def)    { return GetDefCoreVal("Category", "DefCore", def); }
global func GetDefMaxUserSelect(id def) { return GetDefCoreVal("MaxUserSelect", "DefCore", def); }
global func GetDefWidth(id def)       { return GetDefCoreVal("Width", "DefCore", def); }
global func GetDefHeight(id def)      { return GetDefCoreVal("Height", "DefCore", def); }
global func GetDefOffset(id def,nr)   { return GetDefCoreVal("Offset", "DefCore", def, nr); }
global func GetDefValue(id def)       { return GetDefCoreVal("Value", "DefCore", def); }
global func GetDefMass(id def)        { return GetDefCoreVal("Mass", "DefCore", def); }
global func GetDefComponents(id def, nr) { return GetDefCoreVal("Components", "DefCore", def, nr); }
global func GetDefCollection(id def, nr) { return GetDefCoreVal("Collection", "DefCore", def, nr); }
global func GetDefFireTop(id def)     { return GetDefCoreVal("FireTop", "DefCore", def); }
global func GetDefPlacement(id def)   { return GetDefCoreVal("Placement", "DefCore", def); }
global func GetDefContactIncinerate(id def) { return GetDefCoreVal("ContactIncinerate", "DefCore", def); }
global func GetDefBlastIncinerate(id def) { return GetDefCoreVal("BlastIncinerate", "DefCore", def); }
global func GetDefBurnTo(id def)      { return GetDefCoreVal("BurnTo", "DefCore", def); }
global func GetDefBase(id def)        { return GetDefCoreVal("Base", "DefCore", def); }
global func GetDefLine(id def)        { return GetDefCoreVal("Line", "DefCore", def); }
global func GetDefLineConnect(id def) { return GetDefCoreVal("Exclusive", "DefCore", def); }
global func GetDefPrey(id def)        { return GetDefCoreVal("Prey", "DefCore", def); }
global func GetDefEdible(id def)      { return GetDefCoreVal("Edible", "DefCore", def); }
global func GetDefCrewMember(id def)  { return GetDefCoreVal("Prey", "DefCore", def); }
global func GetDefGrowth(id def)      { return GetDefCoreVal("Growth", "DefCore", def); }
global func GetDefRebuy(id def)       { return GetDefCoreVal("Rebuy", "DefCore", def); }
global func GetDefConstruction(id def){ return GetDefCoreVal("Construction", "DefCore", def); }
global func GetDefConstructTo(id def) { return GetDefCoreVal("ConstructTo", "DefCore", def); }
global func GetDefGrab(id def)        { return GetDefCoreVal("Grab", "DefCore", def); }
global func GetDefGrabPutGet(id def)  { return GetDefCoreVal("GrabPutGet", "DefCore", def); }
global func GetDefCollectible(id def) { return GetDefCoreVal("Collectible", "DefCore", def); }
global func GetDefRotate(id def)      { return GetDefCoreVal("Rotate", "DefCore", def); }
global func GetDefChop(id def)        { return GetDefCoreVal("Chop", "DefCore", def); }
global func GetDefFloat(id def)       { return GetDefCoreVal("Float", "DefCore", def); }
global func GetDefContainBlast(id def){ return GetDefCoreVal("ContainBlast", "DefCore", def); }
global func GetDefHorizontalFix(id def) { return GetDefCoreVal("HorizontalFix", "DefCore", def); }
global func GetDefBorderBound(id def) { return GetDefCoreVal("BorderBound", "DefCore", def); }
global func GetDefLiftTop(id def)     { return GetDefCoreVal("LiftTop", "DefCore", def); }
global func GetDefUprightAttach(id def) { return GetDefCoreVal("UprightAttach", "DefCore", def); }
global func GetDefStretchGrowth(id def) { return GetDefCoreVal("StretchGrowth", "DefCore", def); }
global func GetDefBasement(id def)    { return GetDefCoreVal("Basement", "DefCore", def); }
global func GetDefNoBurnDecay(id def) { return GetDefCoreVal("NoBurnDecay", "DefCore", def); }
global func GetDefIncompleteActivity(id def)  { return GetDefCoreVal("IncompleteActivity", "DefCore", def); }
global func GetDefAttractLightning(id def)  { return GetDefCoreVal("AttractLightning", "DefCore", def); }
global func GetDefOversize(id def)    { return GetDefCoreVal("Oversize", "DefCore", def); }
global func GetDefFragile(id def)     { return GetDefCoreVal("Fragile", "DefCore", def); }
global func GetDefExplosive(id def)   { return GetDefCoreVal("Explosive", "DefCore", def); }
global func GetDefProjectile(id def)  { return GetDefCoreVal("Projectile", "DefCore", def); }
global func GetDefNoPushEnter(id def) { return GetDefCoreVal("NoPushEnter", "DefCore", def); }
global func GetDefVehicleControl(id def)  { return GetDefCoreVal("VehicleControl", "DefCore", def); }
global func GetDefNoComponentMass(id def) { return GetDefCoreVal("NoComponentMass", "DefCore", def); }
global func GetDefClosedContainer(id def) { return GetDefCoreVal("ClosedContainer", "DefCore", def); }
global func GetDefSilentCommands(id def) { return GetDefCoreVal("SilentCommands", "DefCore", def); }
global func GetDefNoComponentMass(id def) { return GetDefCoreVal("NoComponentMass", "DefCore", def); }
global func GetDefNoBurnDamage(id def){ return GetDefCoreVal("NoBurnDamage", "DefCore", def); }
global func GetDefTemporaryCrew(id def) { return GetDefCoreVal("TemporaryCrew", "DefCore", def); }
global func GetDefSmokeRate(id def)   { return GetDefCoreVal("SmokeRate", "DefCore", def); }
global func GetDefNoBreath(id def)    { return GetDefCoreVal("NoBreath", "DefCore", def); }
global func GetDefConSizeOff(id def)  { return GetDefCoreVal("ConSizeOff", "DefCore", def); }
global func GetDefNoSell(id def)      { return GetDefCoreVal("NoSell", "DefCore", def); }
global func GetDefNoFight(id def)     { return GetDefCoreVal("NoFight", "DefCore", def); }

// GetObjectVal

global func GetObjOwnMass(object obj)    { return GetObjectVal("OwnMass", 0, obj); }
global func GetObjFixX(object obj)       { return GetObjectVal("FixX", 0, obj); }
global func GetObjFixY(object obj)       { return GetObjectVal("FixY", 0, obj); }
global func GetObjWidth(object obj)		   { return GetObjectVal("Width", 0, obj); }
global func GetObjHeight(object obj)     { return GetObjectVal("Height", 0, obj); }
global func GetObjFireTop(object obj)    { return GetObjectVal("FireTop", 0, obj); }
global func GetObjMobile(object obj)     { return GetObjectVal("Mobile", 0, obj); }
global func GetObjOnFire(object obj)     { return GetObjectVal("OnFire", 0, obj); }
global func GetObjInLiquid(object obj)   { return GetObjectVal("InLiquid", 0, obj); }
global func GetObjEntranceStatus(object obj) { return GetObjectVal("EntranceStatus", 0, obj); }
global func GetObjPhysicalTemporary(object obj) { return GetObjectVal("PhysicalTemporary", 0, obj); }
global func GetObjNeedEnergy(object obj) { return GetObjectVal("NeedEnergy", 0, obj); }
global func GetObjActionTime(object obj) { return GetObjectVal("ActionTime", 0, obj); }
global func GetObjActionData(object obj) { return GetObjectVal("ActionData", 0, obj); }
global func GetObjPhaseDelay(object obj) { return GetObjectVal("PhaseDelay", 0, obj); }
global func GetObjActionTarget1(object obj) { return GetObjectVal("ActionTarget1", 0, obj); }
global func GetObjActionTarget2(object obj) { return GetObjectVal("ActionTarget2", 0, obj); }
global func GetObjPlrViewRange(object obj) { return GetObjectVal("PlrViewRange", 0, obj); }

// GetPlayerVal

global func GetPlrClientNr(int plr)   { return GetPlayerVal("AtClient", 0, plr); }
global func GetPlrClientName(int plr) { return GetPlayerVal("AtClientName", 0, plr); }
global func GetPlrColor(int plr)      { return GetPlayerVal("Color", 0, plr); }
global func GetPlrViewX(int plr)      { return GetPlayerVal("ViewX", 0, plr); } /*(!)*/
global func GetPlrViewY(int plr)      { return GetPlayerVal("ViewY", 0, plr); } /*(!)*/
global func GetPlrFogOfWar(int plr)   { return GetPlayerVal("FogOfWar", 0, plr); } /*(!)*/
global func GetPlrWealth(int plr)     { return GetPlayerVal("Wealth", 0, plr); }
global func GetPlrPoints(int plr)     { return GetPlayerVal("Points", 0, plr); }
global func GetPlrValue(int plr)      { return GetPlayerVal("Value", 0, plr); }
global func GetPlrInitialValue(int plr) { return GetPlayerVal("InitialValue", 0, plr); }
global func GetPlrValueGain(int plr)  { return GetPlayerVal("ValueGain", 0, plr); }
global func GetPlrObjectsOwned(int plr) { return GetPlayerVal("GetPlrObjectsOwned", 0, plr); }

// GetScenarioVal

global func GetScenTitle()     { return GetScenarioVal("Title", "Head"); } /*(!)*/
global func GetScenVersion()   { return GetScenarioVal("Version", "Head"); } /*(!)*/
global func GetScenMaxPlayer() { return GetScenarioVal("MaxPlayer", "Head"); } /*(!)*/
global func GetScenSaveGame()  { return GetScenarioVal("SaveGame", "Head"); } /*(!)*/
global func GetScenReplay()    { return GetScenarioVal("Replay", "Head"); } /*(!)*/
global func GetScenFilm()      { return GetScenarioVal("Film", "Head"); } /*(!)*/
global func GetScenMissionAccess() { return GetScenarioVal("MissionAccess", "Head"); } /*(!)*/
global func GetScenNetworkGame() { return GetScenarioVal("NetworkGame", "Head"); } /*(!)*/
global func GetScenNetworkClients() { return GetScenarioVal("StartupPlayerCount", "Head"); } /*(!)*/
global func GetScenNoSky()     { return GetScenarioVal("NoSky", "Landscape"); }
global func GetScenBottomOpen() { return GetScenarioVal("BottomOpen", "Landscape"); }
global func GetScenTopOpen()   { return GetScenarioVal("TopOpen", "Landscape"); }
global func GetScenLeftOpen()  { return GetScenarioVal("LeftOpen", "Landscape"); }
global func GetScenRightOpen() { return GetScenarioVal("RightOpen", "Landscape"); }
global func GetScenMapWidth()  { return GetScenarioVal("MapWidth", "Landscape"); } /*(!)*/
global func GetScenMapHeight() { return GetScenarioVal("MapHeight", "Landscape"); } /*(!)*/
global func GetScenMapZoom()   { return GetScenarioVal("MapZoom", "Landscape"); } /*(!)*/
global func GetScenClimate()   { return GetScenarioVal("Climate", "Weather"); }
global func GetScenYearSpeed() { return GetScenarioVal("YearSpeed", "Weather"); }
global func GetScenRain()      { return GetScenarioVal("Rain", "Weather"); }
global func GetScenWind()      { return GetScenarioVal("Wind", "Weather"); }
global func GetScenLightning() { return GetScenarioVal("Lightning", "Weather"); }
global func GetScenPrecipitation() { return GetScenarioVal("Precipitation", "Weather"); }
global func GetScenMeteorite() { return GetScenarioVal("Meteorite", "Disasters"); }
global func GetScenVolcano()   { return GetScenarioVal("Volcano", "Weather"); }
global func GetScenEarthquake() { return GetScenarioVal("Earthquake", "Weather"); }

// GetObjectInfoCoreVal

global func GetObjCoreName(object obj)      { return GetObjectInfoCoreVal("Name", "ObjectInfo", obj); }
global func GetObjCoreDeathMessage(object obj) { return GetObjectInfoCoreVal("DeathMessage", "ObjectInfo", obj); }
global func GetObjCoreRank(object obj)      { return GetObjectInfoCoreVal("Rank", "ObjectInfo", obj); }
global func GetObjCoreRankName(object obj)  { return GetObjectInfoCoreVal("RankName", "ObjectInfo", obj); }
global func GetObjCoreTypeName(object obj)  { return GetObjectInfoCoreVal("TypeName", "ObjectInfo", obj); }
global func GetObjCoreParticipation(object obj) { return GetObjectInfoCoreVal("Participation", "ObjectInfo", obj); }
global func GetObjCoreExperience(object obj) { return GetObjectInfoCoreVal("Experience", "ObjectInfo", obj); }
global func GetObjCoreRounds(object obj)    { return GetObjectInfoCoreVal("Rounds", "ObjectInfo", obj); }
global func GetObjCoreDeathCount(object obj) { return GetObjectInfoCoreVal("DeathCount", "ObjectInfo", obj); }
global func GetObjCoreBirthday(object obj)  { return GetObjectInfoCoreVal("Birthday", "ObjectInfo", obj); }
global func GetObjCoreTotalPlayingTime(object obj) { return GetObjectInfoCoreVal("TotalPlayingTime", "ObjectInfo", obj); }
global func GetObjCoreRounds(object obj)    { return GetObjectInfoCoreVal("Rounds", "ObjectInfo", obj); }
global func GetAge(object obj)              { return GetObjectInfoCoreVal("Age", "ObjectInfo", obj); }

// GetPlayerInfoCoreVal

global func GetPlrCoreName(int plr)	    { return GetPlayerInfoCoreVal("Name", "Player", plr); }
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
