/* --- GoldSeller --- */

/*
 automatically sells gold and other valuables nearby when in a base area
*/

func AutoSellValuablesRadius(){return 50;}

static const LIBRARY_GOLDSELLER_MinTimer = 10;
static const LIBRARY_GOLDSELLER_MaxTimer = 60;
static const LIBRARY_GOLDSELLER_TimerStep = 10;

func Initialize()
{
	AddEffect("AutoSellValuables", this, 1, 20, this);
	return _inherited(...);
}

func FxAutoSellValuablesStart(t, effect, temp)
{
	if(temp) return;
	effect.target = t;
}

func FxAutoSellValuablesTimer(_, effect, time)
{
	var owner = GetOwner();
	
	if(!GetPlayerName(owner))
	{
		effect.TimerIntervall = LIBRARY_GOLDSELLER_MaxTimer;
		return 1;
	}
	
	var objs = [];
	
	for(var obj in FindObjects(Find_Distance(AutoSellValuablesRadius()), Find_NoContainer(), Find_Func("IsValuable")))
	{
		if(obj->Stuck()) continue;
		if(!IsAllied(owner, obj->GetController())) continue;
		objs[GetLength(objs)] = obj;
	} 
	
	if(!GetLength(objs))
	{
		effect.TimerIntervall = LIBRARY_GOLDSELLER_MaxTimer;
		return 1;
	}
	
	var comp = objs[0];
	var to_remove = [];
	
	for(var obj in objs)
	{
		var d = ObjectDistance(obj, comp);
		if(d > 50) continue;
		to_remove[GetLength(to_remove)] = obj;
	}
	
	// assert: at least one object in to_remove
	var value = 0;
	var fm = CreateObject(FloatingMessage, comp->GetX() - GetX(), comp->GetY() - GetY(), NO_OWNER);
	fm->SetColor(250, 200, 50);
	fm->FadeOut(2, 10);
	fm->SetSpeed(0, -5);
	
	for(var valuable in to_remove)
	{
		if(valuable->~QueryOnSell(valuable->GetController())) continue;
		
		DoWealth(valuable->GetController(), valuable->GetValue());
		
		value += valuable->GetValue();
		
		CreateParticle("Flash", valuable->GetX() - GetX(), valuable->GetY() - GetY(), 0, 0, 10 * Max(5, Max(valuable->GetDefWidth(), valuable->GetDefHeight())), RGB(255,255,50));
		CastParticles("Dust2", 4, 10, valuable->GetX() - GetX(), valuable->GetY() - GetY(), 50, 70, RGB(100, 100, 100), RGB(200,125,125));
		valuable->RemoveObject();
	}
	
	fm->SetMessage(Format("%d</c>{{Icon_Coins}}", value));
	Sound("Cash");
	
	effect.TimerIntervall = BoundBy(effect.TimerIntervall - Random(LIBRARY_GOLDSELLER_TimerStep), LIBRARY_GOLDSELLER_MaxTimer, LIBRARY_GOLDSELLER_MaxTimer);
	return 1;
}
