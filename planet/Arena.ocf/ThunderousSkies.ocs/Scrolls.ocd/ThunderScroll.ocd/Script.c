/*--
	Scroll: Thunder
	Author: Mimmo

	Call down a devastating storm down from above.
--*/


public func ControlUse(object pClonk)
{
	Sound("Fire::Blast3*");
	Exit(0,-GetY());
	AddEffect("ThunderStrike",nil, 100, 1, nil, this->GetID(),pClonk->GetOwner(),this->GetX()-5);
	RemoveObject();
	return 1;
}
global func FxThunderStrikeStart(pTarget, effect, iTemp, owner, x)
{
	if (iTemp) return;
	effect.owner = owner;
	effect.x = x;
	
	effect.particles_air = 
	{
		Prototype = Particles_Air(),
		Size = PV_Random(1, 5)
	};
}
global func FxThunderStrikeTimer(pTarget, effect, iEffectTime)
{
	var move = effect.x;

	if (iEffectTime>36)
	{
		var particles_lightning =
		{
			Size = 4000,
			BlitMode = GFX_BLIT_Additive,
			Alpha = PV_Linear(255, 0)
		};
		var owner = effect.owner;
		var x = 0;
		var wdt = 18;
		var y = [];
		var targets=[];
		for (var i = (x-wdt); i < (wdt*2); i++ )
		{
			
			while (!GBackSolid(i + move, y[i + wdt]) && y[i + wdt] < LandscapeHeight())
				y[i + wdt]++;
	
		}
		
		for (var i = (x-wdt); i < (wdt*2); i++ )
		{
			var particles = Particles_ElectroSpark1();
			if (Random(2))
				particles = Particles_ElectroSpark2();
			if (!(i%5))
				for (var k = 0; k<y[i + wdt]; k += 10 + Random(5))
				{	
					CreateParticle("ElectroSpark", i + move, k, PV_Random(-12, 12), PV_Random(-40, -10), PV_Random(20, 40), Particles_ElectroSpark1(), 3);
				}
			
			for (var l = 0; l<3; l++)
				CreateParticle("ElectroSpark", i + move, y[i + wdt]-l-2, PV_Random(-20, 20), PV_Random(-20, -30), PV_Random(10, 20), particles, 3);

			if (i%3 == 0)
				CreateParticle("LightningStrike", i + move, y[i + wdt]-32, 0, 0, PV_Random(3, 10), particles_lightning);
			for (var t in FindObjects(Find_Or(Find_And(Find_ID(Clonk),Find_OCF(OCF_Alive)), Find_ID(TargetBalloon)),Find_OnLine(i + move,-0, i + move, y[i + wdt])))
			{
				var add = true;
				for (var j = 0; j<GetLength(targets); j++)
					if (targets[j] == t) add = false;
				if (add) targets[GetLength(targets)] = t;
			}
		}
		
		for (var t in targets)
		{
			if (t->GetID() == TargetBalloon)
			{
				var arw = CreateObjectAbove(Arrow, 0, 0, owner);
				t->OnProjectileHit(arw);
				arw->RemoveObject();
			}
			else
			{
				if (t->GetOwner() != owner)
				{
					t->DoEnergy(-15, 0, 0, owner);
					t->Fling(BoundBy((t->GetX()-(move))/4,-3, 3),-6);
				}
			}
		}
		return -1;
	}
	else if (iEffectTime<4)
	{
		if (iEffectTime%3)
		{
			for (var y = 0; !GBackSolid(move + 5, y) && y < LandscapeHeight(); y += Random(4) + 3)
			{
				var add = Random((iEffectTime*5))*((Random(2)*2) -1);
				CreateParticle("Air", move + 5+add, y, PV_Random(-2, 2), PV_Random(-10, -5), PV_Random(5, 20), effect.particles_air);
			}
		}
	}
}




local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
