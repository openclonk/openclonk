/*--
		Helpers.c
		Authors:
		
		Some useful helper functions.
--*/


global func MessageWindow(string msg, int for_plr, id icon, string caption)
{
	// Get icon.
	if (!icon)
		icon = GetID();
	// Get caption.
	if (!caption)
		caption = GetName();
	// Create msg window (menu).
	var cursor = GetCursor(for_plr);
	if (!cursor->CreateMenu(icon, cursor, 0, caption, 0, 2))
		return false;
	cursor->AddMenuItem(caption, nil, nil, 0, 0, msg);
	return true;
}

global func RemoveAll(p)
{
	var cnt;
	if (GetType(p) == C4V_PropList) p = Find_ID(p); // RemoveAll(ID) shortcut
	for (var obj in FindObjects(p, ...))
	{
		if (obj)
		{
			obj->RemoveObject();
			cnt++;
		}
	}
	return cnt;
}

global func SetBit(int old_val, int bit_nr, bool bit)
{
	if (GetBit(old_val, bit_nr) != (bit != 0))
		return ToggleBit(old_val, bit_nr);
	return old_val;
}

global func GetBit(int value, int bit_nr)
{
	return (value & (1 << bit_nr)) != 0;
}

global func ToggleBit(int old_val, int bit_nr)
{
	return old_val ^ (1 << bit_nr);
}

global func DrawParticleLine (string particle, int x0, int y0, int x1, int y1, int prtdist, int a, int b0, int b1, int ydir)
{
	// Right parameters?
	if (!prtdist)
		return 0;
	// Calculate required number of particles.
	var prtnum = Max(Distance(x0, y0, x1, y1) / prtdist, 2);
	var i = prtnum;
	// Create particles.
	while (i >= 0)
	{
		var i1, i2, b;
		i2 = i * 256 / prtnum;
		i1 = 256 - i2;

		b = ((b0 & 16711935) * i1 + (b1 & 16711935) * i2) >> 8 & 16711935
			| ((b0 >> 8 & 16711935) * i1 + (b1 >> 8 & 16711935) * i2) & -16711936;
		if (!b && (b0 | b1))
			b++;
		CreateParticle(particle, x0 + (x1 - x0) * i / prtnum, y0 + (y1 - y0) * i-- / prtnum, 0, ydir, a, b);
	}
	// Succes, return number of created particles.
	return prtnum;
}

// Searches for an object, which can be obtained with the Acquire command.
global func GetAvailableObject (id def, object obj)
{
	var crit = Find_And(Find_OCF(OCF_Available),
		Find_InRect(-500, -250, 1000, 500),
		Find_ID(def),
		Find_OCF(OCF_Fullcon),
		Find_Not(Find_OCF(OCF_OnFire)),
		Find_Func("GetAvailableObjectCheck", GetOwner()),
		Find_Not(Find_Container(obj)));
	if (!obj)
		SetLength(crit, GetLength(crit) - 1);
	return FindObject(crit, Sort_Distance());
}

global func GetAvailableObjectCheck(int plr)
{
	// Object is not connected to anything (for line construction kits)
	if (FindObject (Find_ActionTarget(this), Find_Procedure(DFA_CONNECT)))
		return false;
	// Not chosen by another friendly clonk
	if (GetEffect("IntNotAvailable", this) && !Hostile(plr, EffectVar(0, this, GetEffect("IntNotAvailable", this))->GetOwner()))
		return false;
	return true;
}

// Sets the MaxEnergy value of an object and does the necessary callbacks.
global func SetMaxEnergy(int value)
{
	if (!this)
		return;
	value *= 1000;
	var old_maxenergy = this.MaxEnergy;
	this.MaxEnergy = value;
	// Change current energy percentage wise and implicit callback.
	DoEnergy(GetEnergy() * (value - old_maxenergy) / old_maxenergy);
	return;
}

// Returns the MaxEnergy value of an object.
global func GetMaxEnergy()
{
	if (!this)
		return;
	return this.MaxEnergy / 1000;
}

// Sets the MaxBreath value of an object and does the necessary callbacks.
global func SetMaxBreath(int value)
{
	if (!this)
		return;
	var old_maxbreath = this.MaxBreath;
	this.MaxBreath = value;
	// Change current breath percentage wise and implicit callback.
	DoBreath(GetBreath() * (value - old_maxbreath) / old_maxbreath);
	return;
}

// Returns the MaxBreath value of an object.
global func GetMaxBreath()
{
	if (!this)
		return;
	return this.MaxBreath;
}

