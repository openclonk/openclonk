/**
    Relaunch Rule
    This rule enables and handles relaunches.
    @author Maikel, Sven2, Fulgen
*/

protected func Initialize()
{
    ScheduleCall(this, this.CheckDescription, 1, 1);
    return true;
}

private func CheckDescription()
{
    // If neutral flagpoles exist, update name and description.
    if(respawn_at_base)
    {
        if(ObjectCount(Find_ID(Flagpole), Find_Func("IsNeutral")))
        {
            SetName("$Name2$");
            this.Description = "$Description2$";
        }
        else
        {
            SetName("$Name3$");
            this.Description = "$Description3$";
        }
    }
    else
    {
        SetName("$Name$");
        this.Description = "$Description$";
    }
    return true;
}

// Determines whether the inventory of the crew member is transfered upon respawn.
local inventory_transfer = false;

// Determines whether a crew member needs to be bought.
local free_crew = false;

//Determines whether the clonk will be respawned at the base
local respawn_at_base = false;

local DefaultRelaunchCount = 5;
local aRelaunches = [];

local ClonkType = Clonk;

local DisableLastWeapon = false;
local LastUsedPlayerWeapons = [];
local RelaunchTime = 36 * 10;
local Hold = false;

public func SetInventoryTransfer(bool transfer)
{
    inventory_transfer = transfer;
    return true;
}

public func GetInventoryTransfer()
{
    return inventory_transfer;
}

public func SetFreeCrew(bool free)
{
    free_crew = free;
    return true;
}

public func GetFreeCrew()
{
    return free_crew;
}

public func SetRespawnDelay(int iDelay)
{
    RelaunchTime = iDelay;
    return this;
}

public func GetRespawnDelay()
{
    return RelaunchTime;
}

public func SetHolding(bool fHold)
{
    Hold = fHold;
    return this;
}

public func GetHolding()
{
    return Hold;
}

public func SetLastWeaponUse(bool fUse)
{
    this.DisableLastWeapon = !fUse;
    return this;
}

public func GetLastWeaponUse()
{
    return DisableLastWeapon;
}

public func SetBaseRespawn(bool fSet)
{
    respawn_at_base = fSet;
    return this;
}

public func GetBaseRespawn()
{
    return respawn_at_base;
}

public func SetDefaultRelaunches(int iRelaunches)
{
    DefaultRelaunchCount = iRelaunches;
}

public func InitializePlayer(int iPlr)
{
    _inherited(iPlr, ...);
    // Scenario script callback.
    aRelaunches[iPlr] = DefaultRelaunchCount;
    GameCallEx("OnPlayerRelaunch", iPlr, false);
    return DoRelaunch(iPlr, nil, nil, true);
}

/*public func OnClonkDeath(int plr, object pClonk, int iKiller)
{
    return RelaunchPlayer(plr, iKiller, pClonk);
}*/

public func RelaunchPlayer(int plr, int killer, object pClonk)
{
    if(plr == nil || plr == NO_OWNER) return Log("NO PlAYER: %d", plr);
    
    if(DefaultRelaunchCount != nil)
    {
        aRelaunches[plr]--;
        if(aRelaunches[plr] < 0)
        {
            EliminatePlayer(plr);
            return;
        }
    }
    
    GameCall("OnPlayerRelaunch", plr, true);
    
    return DoRelaunch(plr, pClonk, nil);
}

public func RespawnAtBase(int plr, object clonk, bool fNoCreation)
{
    // Skip eliminated players, NO_OWNER, etc.
    if (!GetPlayerName(plr)) 
        return; 
    
    // Only respawn a clonk if it is the last crew member.
    if (GetCrewCount(plr) >= 1) 
        return;
    
    // Get the bases at which the clonk can possibly respawn.
    var bases = GetBases(clonk), crew;
    for (var base in bases)
    {
        if (!base)
            continue;
        
        if(fNoCreation)
        {
            crew = clonk ?? GetCrew(plr);
            if(crew)
            {
                crew->SetPosition(base->GetX(), base->GetY() + base->GetDefHeight() / 2);
                break;
            }
   
        }
            
        // If free crew just create a clonk at the base.
        if (free_crew)
        {
            crew = CreateObjectAbove(ClonkType, base->GetX() - GetX(), base->GetY() + base->GetDefHeight() / 2 - GetY(), plr);
            crew->MakeCrewMember(plr);
            SetCursor(plr, crew);
            // Transfer inventory if turned on.
            if (inventory_transfer) TransferInventory(clonk, crew);
            break;
        }
        // Try to buy a crew member at the base.
        var pay_plr = base->GetOwner();
        // Payment in neutral bases by clonk owner.
        if (pay_plr == NO_OWNER) 
            pay_plr = plr;
        crew = base->~DoBuy(ClonkType, plr, pay_plr, clonk);
        if (crew)
        {
            crew->Exit(0, base->GetDefHeight() / 2);
            SetCursor(plr, crew);
            // Transfer inventory if turned on.
            if (inventory_transfer) TransferInventory(clonk, crew);
            break;
        }
    }
    // Respawn delay (+Weapon choice if desired by scenario)
    if (crew && RelaunchTime)
    {
        crew->SetCrewEnabled(false); // will be re-set by relauncher
        
        crew->CreateObject(RelaunchContainer,nil,nil,plr)->StartRelaunch(clonk);
        // But keep view on old corpse because the death might be exciting!
        // And sometimes you want to know why you died (just like in real-life!)
        if(clonk)
        {
            var light = clonk->CreateLight(0, 0, 100, Fx_Light.LGT_Temp, plr, 20, RelaunchTime*36);
            SetCursor(plr, nil);
            SetPlrView(plr, light);
        }
    }
    return true;
}

private func TransferInventory(object from, object to)
{
    // Drop some items that cannot be transferred (such as connected pipes and dynamite igniters)
    var i = from->ContentsCount(), contents;
    while (i--)
        if (contents = from->Contents(i))
            if (contents->~IsDroppedOnDeath(from))
            {
                contents->Exit();
            }
            else
            {
                // The new clonk doesn't burn. To be consistent, also extinguish contents
                contents->Extinguish();
            }
    return to->GrabContents(from);
}

private func GetBases(object clonk)
{
    var plr = clonk->GetOwner();
    // Neutral flagpoles are preferred respawn points, because they are used as the only respawn points in missions.
    var bases = clonk->FindObjects(Find_ID(Flagpole), Find_Func("IsNeutral"), clonk->Sort_Distance());
    // If there are no neutral flagpoles, find closest base owned by the player (or team) and try to buy a clonk.
    if (GetLength(bases) <= 0) 
        bases = clonk->FindObjects(Find_Func("IsBaseBuilding"), Find_Allied(plr), clonk->Sort_Distance());
    return bases;
}

public func DoRelaunch(int iPlr, object pClonk, array position, bool fNoCreation)
{
    if(respawn_at_base) return RespawnAtBase(iPlr, pClonk, fNoCreation);
    if(!GetPlayerName(iPlr)) return Log("NO NAME");
    position = (position ?? GameCall("RelaunchPosition", iPlr, GetPlayerTeam(iPlr))) ?? FindRelaunchPos(iPlr);
    
    var spawn;
    
    if(GetType(position) == C4V_Array)
    {
        if(GetType(position[0]) == C4V_Array)
        {
            spawn = position[Random(GetLength(position))];
        }
        else spawn = position;
    }
    var clonk;
    if(!fNoCreation)
    {
        var clonk = CreateObjectAbove(ClonkType, spawn[0], spawn[1], iPlr);
        if(!clonk) return Log("NO CLONK");
        clonk->MakeCrewMember(iPlr);
    }
    else
    {
        clonk = GetCrew(iPlr);
        if(!clonk) return Log("NO CREW");
    }
    
    if(!GetCursor(iPlr) || GetCursor(iPlr) == pClonk) SetCursor(iPlr, clonk);
    clonk->DoEnergy(100000);
    
    if(RelaunchTime)
    {
        clonk->CreateObject(RelaunchContainer,nil,nil,iPlr)->StartRelaunch(clonk);
    }
    return true;
}

protected func FindRelaunchPos(int plr)
{
    var tx, ty; // Test position.
    for (var i = 0; i < 500; i++)
    {
        tx = Random(LandscapeWidth());
        ty = Random(LandscapeHeight());
        if (GBackSemiSolid(AbsX(tx), AbsY(ty)))
            continue;
        if (GBackSemiSolid(AbsX(tx+5), AbsY(ty+10)))
            continue;
        if (GBackSemiSolid(AbsX(tx+5), AbsY(ty-10)))
            continue;
        if (GBackSemiSolid(AbsX(tx-5), AbsY(ty+10)))
            continue;
        if (GBackSemiSolid(AbsX(tx-5), AbsY(ty-10)))
            continue;
        // Succes.
        return [tx, ty];
    }
    return nil;
}

/*-- Scenario saving --*/

public func SaveScenarioObject(props, ...)
{
    if (!inherited(props, ...)) 
        return false;
    // Custom properties
    props->Remove("Name"); // updated by initialization
    props->Remove("Description"); // updated by initialization
    if (inventory_transfer) 
        props->AddCall("InventoryTransfer", this, "SetInventoryTransfer", inventory_transfer);
    if (free_crew) 
        props->AddCall("FreeCrew", this, "SetFreeCrew", free_crew);
    return true;
}
/*-- Globals --*/

global func SetRelaunchCount(int plr, int value)
{
    if(UnlimitedRelaunches()) return;
    GetRelaunchRule().aRelaunches[plr] = value;
    Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule().aRelaunches[plr]);
    return value;
}

global func GetRelaunchCount(int plr)
{
    return GetRelaunchRule().aRelaunches[plr];
}

global func DoRelaunchCount(int plr, int value)
{
    if(UnlimitedRelaunches()) return;
    GetRelaunchRule().aRelaunches[plr] += value;
    Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule().aRelaunches[plr]);
    return;
}

global func UnlimitedRelaunches()
{
    return GetRelaunchRule().DefaultRelaunchCount == nil;
}

global func GetRelaunchRule()
{
    return FindObject(Find_ID(Rule_Relaunch)) || CreateObject(Rule_Relaunch);
}

/* Editor */

public func Definition(def)
{
    if (!def.EditorProps) def.EditorProps = {};
    def.EditorProps.inventory_transfer = { Name="$InventoryTransfer$", EditorHelp="$InventoryTransferHelp$", Type="bool", Set="SetInventoryTransfer" };
    def.EditorProps.free_crew = { Name="$FreeCrew$", EditorHelp="$FreeCrewHelp$", Type="bool", Set="SetFreeCrew" };
    def.EditorProps.respawn_at_base = {
        Name = "$RespawnAtBase$",
        EditorHelp = "$RespawnAtBaseHelp$",
        Type = "bool",
        Set = "SetBaseRespawn"
    };
    
    def.EditorProps.hold = {
        Name = "$Holding$",
        EditorHelp = "$HoldingHelp$",
        Type = "bool",
        Set = "SetHolding"
    };
    
    def.EditorProps.respawn_delay = {
        Name = "$RespawnDelay$",
        EditorHelp = "$RespawnDelayHelp$",
        Type = "int",
        Set = "SetRespawnDelay"
    };
    
    def.EditorProps.relaunch_count = {
        Name = "$RelaunchCount$",
        EditorHelp = "$RelaunchCountHelp$",
        Type = "int",
        Set = "SetDefaultRelaunches"
    };
}

/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
