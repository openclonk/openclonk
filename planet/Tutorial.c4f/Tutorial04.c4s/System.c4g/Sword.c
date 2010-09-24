// Sword notifies straw target that it is hit.

#appendto Sword

func CheckStrike(iTime)
{
	//if(iTime < 20) return;
	var  offset_x=10;
	var offset_y=0;
	if(Contained()->GetDir() == DIR_Left) offset_x*=-1;
	
	if(!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y=10;
	
	var width=8;
	var height=20;
	var slowedVelocity=GetWeaponSlow(Contained());
	var found=false;
	var angle=0;
	
	var doBash=Abs(Contained()->GetXDir()) > 5 || Abs(Contained()->GetYDir()) > 5;
	if(!doBash) doBash=Contained()->GetContact(-1) & CNAT_Bottom;
	
	if(doBash)
	{
		if(Contained()->GetDir() == DIR_Left)
			angle=-(Max(5, Abs(Contained()->GetXDir())));
		else angle=(Max(5, Abs(Contained()->GetXDir())));
	}
	
	for(var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height), Find_OCF(OCF_Alive), Find_NoContainer(), Find_Exclude(Contained()), Find_Layer(GetObjectLayer())))
	{
		// check for second hit
		var effect_name=Format("HasBeenHitBySwordEffect%d", magic_number);
		var sword_name=Format("HasBeenHitBySword%d", this->ObjectNumber());
		var first=true;
		if(GetEffect(effect_name, obj))
		{
			//Log("already hit");
			continue;
		} else
		{
			//Log("first hit by this effect");
			AddEffect(effect_name, obj, 1, 60 /* arbitrary */, 0, 0);
			
			if(GetEffect(sword_name, obj))
			{
				//Log("successive hit");
				first=false;
			}
			else
			{
				//Log("first hit overall");
				AddEffect(sword_name, obj, 1, 40, 0, 0);
			}
		}
		
		found=true;
		
		/*if(iTime < 20)
		{
			DoWeaponSlow(obj, 800);
			continue;
		}*/
		
		/*var velocity=GetRelativeVelocity(Contained(), obj) * 2;
		velocity+= slowedVelocity / 10;
		velocity=velocity*3;*/
		//if(velocity > 300) velocity=300;
		
		var shield=ApplyShieldFactor(Contained(), obj, damage);
		if(shield == 100)
			continue;
		// fixed damage for now, not taking velocity into account
		var damage=(((100-shield)*(125 * 1000) / 100) / 15);
		obj->DoEnergy(-damage, true, 0, Contained()->GetOwner());
				
		
		if(offset_y)
			ApplyWeaponBash(obj, 100, 0);
		else
			if(!first)
				ApplyWeaponBash(obj, damage/50, Angle(0, 0, angle, -10));
		else
			if(!offset_y)
				DoWeaponSlow(obj, 300);
		
		
		// particle
		var x=-1;
		var p="Slice2";
		if(Contained()->GetDir() == DIR_Right)
		{
			x=1;
			p="Slice1";
		} 

		CreateParticle(p, AbsX(obj->GetX())+RandomX(-1,1), AbsY(obj->GetY())+RandomX(-1,1), 0, 0, 100, RGB(255,255,255), obj);
		obj->~OnSwordHit();
	}
	if(found)
	{
		/*if(iTime < 20)
		{
			DoWeaponSlow(Contained(), 1000);
		}
		else*/
		{
			this->Sound(Format("WeaponHit%d.ogg", 1+Random(3)), false);
			//if(doBash)
			//	DoWeaponSlow(Contained(), 2000);
			//this->StopWeaponHitCheckEffect(Contained());
		}
	}
}