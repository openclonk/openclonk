/**
	Power
	Cares about power management of a base.
	
	callbacks:
	QueryWaivePowerRequest()
	OnNotEnoughPower()
	OnEnoughPower()
	OnRemovedFromPowerSleepingQueue(): called when the object was removed from the sleeping queue
	
	globals:
	MakePowerConsumer(int amount)
	MakePowerProducer(int amount)
	IsPowerAvailable(int amount)
	
*/

static Library_Power_power_compounds;

// for the helper definitions
local power_links; // producers and consumers
local sleeping_links;
local power_balance; // performance
local neutral; // is "the" neutral helper?

func Initialize()
{
	power_links = [];
	sleeping_links = [];
	power_balance = 0;
	neutral = false;
}


func AddPowerProducer(object p, int a)
{
	return AddPowerLink(p, a);
}

func AddPowerConsumer(object p, int a)
{
	
	// possibly sleeping?
	{	
		for(var i = GetLength(sleeping_links); --i >= 0;)
		{
			var o = sleeping_links[i];
			if(o.obj != p) continue;
			
			// did not affect power balance, we can just remove/change the link
			if(a == 0) // remove
			{
				sleeping_links[i] = sleeping_links[GetLength(sleeping_links) - 1];
				SetLength(sleeping_links, GetLength(sleeping_links) - 1);
				
				// message
				var diff = 0;
				{
					var t = CreateObject(FloatingMessage, o.obj->GetX() - GetX(), o.obj->GetY() - GetY(), NO_OWNER);
					t->SetMessage(Format("%d</c>{{Library_PowerConsumer}}", diff));
					t->SetColor(255, 0, 0);
					t->SetYDir(-10);
					t->FadeOut(4, 8);
				}
				
				o.obj->~OnRemovedFromPowerSleepingQueue();
				return true;
			}
			sleeping_links[i].amount = a;
			return true;
		}
	}
	
	// not asleep
	return AddPowerLink(p, -a);
}

func RemovePowerLink(object p)
{
	return AddPowerLink(p, 0);
}

func AddPowerLink(object p, int a, bool surpress_balance_check)
{
	var n = {obj = p, amount = a};
	
	var before = 0;
	var found = false;
	var first_empty = -1;
	var diff = 0;
	for(var i = GetLength(power_links); --i >= 0;)
	{
		var o = power_links[i];
		if(o == nil) // possible
		{
			first_empty = i;
			continue; 
		}
		
		if(o.obj == nil) // removed from outside
		{
			power_links[i] = nil;
			continue;
		}
		
		if(o.obj != p) continue;
		found = true;
		
		diff = a - o.amount;
		power_balance += diff;
		
		if(a == 0)
		{
			before = power_links[i].amount;
			power_links[i] = nil;
		}
		else power_links[i] = n;
		break;
	}
	
	if(!found)
	{
		// place to insert?
		if(a != 0)
		{
			if(first_empty != -1)
				power_links[first_empty] = n;
			else
				power_links[GetLength(power_links)] = n;
		}
		diff = n.amount;
		power_balance += diff;
	}
	
	diff = n.amount;
	if((diff > 0) || ((a == 0) && (before > 0)))
	{
		var t = CreateObject(FloatingMessage, n.obj->GetX() - GetX(), n.obj->GetY() - GetY(), NO_OWNER);
		t->SetMessage(Format("+%d</c>{{Library_PowerConsumer}}", diff));
		t->SetColor(0, 255, 0);
		t->SetYDir(-10);
		t->FadeOut(4, 8);
	}
	else if((diff < 0) || ((a == 0) && (before < 0)))
	{
		var t = CreateObject(FloatingMessage, n.obj->GetX() - GetX(), n.obj->GetY() - GetY(), NO_OWNER);
		t->SetMessage(Format("%d</c>{{Library_PowerConsumer}}", diff));
		t->SetColor(255, 0, 0);
		t->SetYDir(-10);
		t->FadeOut(4, 8);
	}
	if(n.amount < 0)
		n.obj->~OnEnoughPower(); // might be reverted soon, though
		
	if(!surpress_balance_check)
		CheckPowerBalance();
	return true;
}

func CheckPowerBalance()
{
	// special handling for ownerless links
	// always sleep
	if(neutral)
	{
		for(var i = GetLength(power_links); --i >= 0;)
		{
			var o = power_links[i];
			if(o == nil) continue;
			if(o.amount > 0) continue; // producer
			SleepLink(i);
		}
		return false;
	}
	
	//Message("@Power: %d", power_balance);
	
	if(power_balance >= 0) // alrighty
	{
		// we are good to go
		// we could revive sleeping links at this point
		
		if(GetLength(sleeping_links))
		{
			for(var i = GetLength(sleeping_links); --i >= 0;)
			{
				var o = sleeping_links[i];
				if(o.obj == nil)
				{
					sleeping_links[i] = sleeping_links[GetLength(sleeping_links) - 1];
					SetLength(sleeping_links, GetLength(sleeping_links) - 1);
					continue;
				}
				if(power_balance + o.amount < 0) continue;
				
				// found link to revive!
				UnsleepLink(i);
				
				// can not continue since UnsleepLink changes the array
				return true;
			}
		}
		
		return true;
	}
	
	// something happened
	// something evil
	
	// look for consumer to kick out of system
	// best-fit strategy (or volunteers)
	
	var best_fit = 0xFFFFFF;
	var best_volunteer = 0;
	var best = -1;
	var abs_diff = Abs(power_balance);
	for(var i = GetLength(power_links); --i >= 0;)
	{
		var o = power_links[i];
		if(o == nil) continue;
		if(o.amount > 0) continue; // producer
		
		var d = Abs(((-o.amount) - abs_diff));
		
		var v = o.obj->~QueryWaivePowerRequest();
		
		if(!best_volunteer) // no volunteers yet
		{
			if((d < best_fit) || (best == nil) || (v > 0))
			{
				best_fit = d;
				best = i;
				
				if(v)
					best_volunteer = v;
			}
		}
		else // we already have volunteers
		{
			if(v < best_volunteer) continue;
			best_volunteer = v;
			best = i;
		}
	}
	
	// total blackout? No consumer active anymore?
	if(best == -1)
	{
		return false;
	}
	
	// has object
	SleepLink(best);
	
	// recurse
	// might revive weaker consumer or sleep another one
	CheckPowerBalance();
	
	return false;
}

func SleepLink(int index)
{
	if(index < 0 || index >= GetLength(power_links))
		return FatalError("SleepLink() called without valid index!");
	
	// only consumers, sleeping producers does not make sense...
	var o = power_links[index];
	if(o.amount > 0) return FatalError("SleepLink() trying to sleep producer!");
	
	// delete from list
	power_links[index] = nil;
	power_balance -= o.amount;
	sleeping_links[GetLength(sleeping_links)] = o;
	
	// sadly not enough power anymore
	o.obj->~OnNotEnoughPower();
	
	return true;
}

func UnsleepLink(int index)
{
	if(index < 0 || index >= GetLength(sleeping_links))
		return FatalError("UnsleepLink() called without valid index!");
		
	var o = sleeping_links[index];
	
	// delete
	sleeping_links[index] = sleeping_links[GetLength(sleeping_links) - 1];
	SetLength(sleeping_links, GetLength(sleeping_links) - 1);
	
	return AddPowerLink(o.obj, o.amount); // revives the link
}

// get requested power of nodes that are currently sleeping
public func GetPendingPowerAmount()
{
	var sum = 0;
	for(var i = GetLength(sleeping_links); --i >= 0;)
	{
		sum += -sleeping_links[i].amount;
	}
	return sum;
}

// should always be above zero - otherwise an object would have been deactivated
public func GetPowerBalance()
{
	return power_balance;
}

public func IsPowerAvailable(object obj, int amount)
{
	// ignore object for now
	return power_balance > amount;
}

public func Init()
{
	if(GetType(Library_Power_power_compounds) != C4V_Array)
		Library_Power_power_compounds = [];
}

// static
func GetPowerHelperForObject(object who)
{
	var w;
	while(w = who->~GetActualPowerConsumer())
	{
		if(w == who) break; // nope
		who = w;
	}
	var flag = GetFlagpoleForPosition(who->GetX() - GetX(), who->GetY() - GetY());
	
	var helper = nil;
	if(!flag) // neutral - needs neutral helper
	{
		for(var obj in Library_Power_power_compounds)
		{
			if(!obj.neutral) continue;
			helper = obj;
			break;
		}
		
		if(helper == nil) // not yet created?
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			helper.neutral = true;
			Library_Power_power_compounds[GetLength(Library_Power_power_compounds)] = helper;
		}
		
	} 
	else
	{
		helper=flag->GetPowerHelper();
		
		if(helper == nil)
		{
			helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
			Library_Power_power_compounds[GetLength(Library_Power_power_compounds)] = helper;
			
			// add to all linked flags
			flag->SetPowerHelper(helper);
			for(var f in flag->GetLinkedFlags())
			{
				if(f->GetPowerHelper() != nil) // assert
					FatalError("Flags in compound have different power helper!");
				f->SetPowerHelper(helper);
			}
		}
	}
	
	return helper;
}

global func GetPendingPowerAmount()
{
	if(!this) return 0;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->GetPendingPowerAmount();
}

global func GetCurrentPowerBalance()
{
	if(!this) return 0;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->GetPowerBalance();
}

global func MakePowerProducer(int amount)
{
	if(!this) return false;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->AddPowerProducer(this, amount);
}

global func IsPowerAvailable(int amount)
{
	if(!this) return false;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->IsPowerAvailable(this, amount);
}

global func MakePowerConsumer(int amount)
{
	if(!this) return false;
	Library_Power->Init();
	return (Library_Power->GetPowerHelperForObject(this))->AddPowerConsumer(this, amount);
}

// helper object