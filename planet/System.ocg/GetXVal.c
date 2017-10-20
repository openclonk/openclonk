/**
	GetXVal.c
	Some functions which request information from the GetXVal family.
	Attention: These functions directly access internal values of the engine.
	The usage might, under certain circumstances, lead to desynchronisation in
	either network games or save games. The most dangerous functions are marked
	with //(!).
	These functions are also very slow. Values should be cached if possible.

	@author 
*/

// documented in /docs/sdk/script/fn
global func GetActMapVal(string entry, string action, id def, int num)
{
	if (!def)
		def = GetID();
	if (entry == "Facet")
		entry = ["X", "Y", "Wdt", "Hgt", "OffX", "OffY"][num];
	return GetProperty(entry, GetProperty(action, def));
}

// GetDefCoreVal
global func GetDefCategory()    { return GetDefCoreVal("Category", "DefCore"); }
global func GetDefOffset(nr)   { return GetDefCoreVal("Offset", "DefCore", nr); }
global func GetDefValue()       { return GetDefCoreVal("Value", "DefCore"); }
global func GetDefMass()        { return GetDefCoreVal("Mass", "DefCore"); }
global func GetDefComponents(nr) { return GetDefCoreVal("Components", "DefCore", nr); }
global func GetDefCollection(nr) { return GetDefCoreVal("Collection", "DefCore", nr); }
global func GetDefColorByOwner(){ return GetDefCoreVal("ColorByOwner", "DefCore"); }
global func GetDefLine()        { return GetDefCoreVal("Line", "DefCore"); }
global func GetDefCrewMember()  { return GetDefCoreVal("CrewMember", "DefCore"); }
global func GetDefConstruction(){ return GetDefCoreVal("Construction", "DefCore"); }
global func GetDefGrabPutGet()  { return GetDefCoreVal("GrabPutGet", "DefCore"); }
global func GetDefRotate()      { return GetDefCoreVal("Rotate", "DefCore"); }
global func GetDefFloat()       { return GetDefCoreVal("Float", "DefCore"); }
global func GetDefHorizontalFix() { return GetDefCoreVal("HorizontalFix", "DefCore"); }
global func GetDefLiftTop()     { return GetDefCoreVal("LiftTop", "DefCore"); }
global func GetDefUprightAttach() { return GetDefCoreVal("UprightAttach", "DefCore"); }
global func GetDefStretchGrowth() { return GetDefCoreVal("StretchGrowth", "DefCore"); }
global func GetDefIncompleteActivity()  { return GetDefCoreVal("IncompleteActivity", "DefCore"); }
global func GetDefOversize()    { return GetDefCoreVal("Oversize", "DefCore"); }
global func GetDefFragile()     { return GetDefCoreVal("Fragile", "DefCore"); }
global func GetDefProjectile()  { return GetDefCoreVal("Projectile", "DefCore"); }
global func GetDefNoPushEnter() { return GetDefCoreVal("NoPushEnter", "DefCore"); }
global func GetDefVehicleControl()  { return GetDefCoreVal("VehicleControl", "DefCore"); }
global func GetDefNoMassFromContents() { return GetDefCoreVal("NoMassFromContents", "DefCore"); }
global func GetDefClosedContainer() { return GetDefCoreVal("ClosedContainer", "DefCore"); }
global func GetDefSilentCommands() { return GetDefCoreVal("SilentCommands", "DefCore"); }
global func GetDefTemporaryCrew() { return GetDefCoreVal("TemporaryCrew", "DefCore"); }
global func GetDefNoBreath()    { return GetDefCoreVal("NoBreath", "DefCore"); }
global func GetDefConSizeOff()  { return GetDefCoreVal("ConSizeOff", "DefCore"); }

// GetObjectVal
global func GetObjOwnMass()    { return GetObjectVal("OwnMass", nil); }
global func GetObjFixX()       { return GetObjectVal("FixX", nil); }
global func GetObjFixY()       { return GetObjectVal("FixY", nil); }
global func GetObjWidth()      { return GetObjectVal("Width", nil); }
global func GetObjHeight()     { return GetObjectVal("Height", nil); }
global func GetObjMobile()     { return GetObjectVal("Mobile", nil); }
global func GetObjOnFire()     { return GetObjectVal("OnFire", nil); }
global func GetObjInLiquid()   { return GetObjectVal("InLiquid", nil); }
global func GetObjEntranceStatus() { return GetObjectVal("EntranceStatus", nil); }
global func GetObjPhysicalTemporary() { return GetObjectVal("PhysicalTemporary", nil); }
global func GetObjNeedEnergy() { return GetObjectVal("NeedEnergy", nil); }
global func GetObjActionTime() { return GetObjectVal("ActionTime", nil); }
global func GetObjActionData() { return GetObjectVal("ActionData", nil); }
global func GetObjPhaseDelay() { return GetObjectVal("PhaseDelay", nil); }
global func GetObjActionTarget1() { return GetObjectVal("ActionTarget1", nil); }
global func GetObjActionTarget2() { return GetObjectVal("ActionTarget2", nil); }
global func GetObjPlrViewRange() { return GetObjectVal("PlrViewRange", nil); }

// GetPlayerVal
global func GetPlrClientNr(int plr)   { return GetPlayerVal("AtClient", nil, plr); }
global func GetPlrClientName(int plr) { return GetPlayerVal("AtClientName", nil, plr); }
//global func GetPlrColor(int plr)      { return GetPlayerVal("ColorDw", 0, plr); } - use GetPlayerColor
global func GetPlrViewX(int plr)      { return GetPlayerVal("ViewX", nil, plr); } //(!)
global func GetPlrViewY(int plr)      { return GetPlayerVal("ViewY", nil, plr); } //(!)
global func GetPlrFogOfWar(int plr)   { return GetPlayerVal("FogOfWar", nil, plr); } //(!)
global func GetPlrWealth(int plr)     { return GetPlayerVal("Wealth", nil, plr); }
global func GetPlrPoints(int plr)     { return GetPlayerVal("Points", nil, plr); }
global func GetPlrValue(int plr)      { return GetPlayerVal("Value", nil, plr); }
global func GetPlrInitialValue(int plr) { return GetPlayerVal("InitialValue", nil, plr); }
global func GetPlrValueGain(int plr)  { return GetPlayerVal("ValueGain", nil, plr); }
global func GetPlrObjectsOwned(int plr) { return GetPlayerVal("GetPlrObjectsOwned", nil, plr); }

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
global func GetScenBottomOpen() { return GetScenarioVal("BottomOpen", "Landscape"); }
global func GetScenTopOpen()   { return GetScenarioVal("TopOpen", "Landscape"); }
global func GetScenLeftOpen()  { return GetScenarioVal("LeftOpen", "Landscape"); }
global func GetScenRightOpen() { return GetScenarioVal("RightOpen", "Landscape"); }
global func GetScenMapWidth()  { return GetScenarioVal("MapWidth", "Landscape"); } //(!)
global func GetScenMapHeight() { return GetScenarioVal("MapHeight", "Landscape"); } //(!)
global func GetScenMapZoom()   { return GetScenarioVal("MapZoom", "Landscape"); } //(!)
global func GetScenClimate()   { return GetScenarioVal("Climate", "Weather"); }
global func GetScenYearSpeed() { return GetScenarioVal("YearSpeed", "Weather"); }
global func GetScenWind()      { return GetScenarioVal("Wind", "Weather"); }

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
