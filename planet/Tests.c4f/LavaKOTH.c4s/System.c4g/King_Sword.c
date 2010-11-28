#appendto Sword

local king_size;

public func MakeKingSize() { king_size = true; SetMeshMaterial("KingSword2",1); }
public func MakeNormalSize() { king_size = false; SetMeshMaterial("Sword2",1); }
public func Departure() { MakeNormalSize(); }


func CheckStrike(iTime)
{

	
	//if(iTime < 20) return;
	var  offset_x=7;
	var offset_y=0;
	if(Contained()->GetDir() == DIR_Left) offset_x*=-1;
	
	if(!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y=10;
	
	var width=10;
	var height=20;
	var angle=0;
	
	var doBash=Abs(Contained()->GetXDir()) > 5 || Abs(Contained()->GetYDir()) > 5;
	if(!doBash) doBash=Contained()->GetContact(-1) & CNAT_Bottom;
	
	if(doBash)
	{
		if(Contained()->GetDir() == DIR_Left)
			angle=-(Max(5, Abs(Contained()->GetXDir())));
		else angle=(Max(5, Abs(Contained()->GetXDir())));
	}
	
	for(var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height),
							   Find_NoContainer(),
							   Find_Exclude(Contained()),
							   Find_Layer(GetObjectLayer())))
	{
		if (obj->~IsProjectileTarget(this, Contained()) || obj->GetOCF() & OCF_Alive)
		{
			var effect_name=Format("HasBeenHitBySwordEffect%d", magic_number);
			var sword_name=Format("HasBeenHitBySword%d", this->ObjectNumber());
			var first=true;
			// don't hit objects twice
			if(!GetEffect(effect_name, obj))
			{
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

				if(!king_size)
				{
					// Reduce damage by shield
					var shield=ApplyShieldFactor(Contained(), obj, 0); // damage out of scope?
					if(shield == 100)
						continue;
				}	
				// fixed damage (10)
				var damage=((100-shield)*10*1000 / 100);
				if(king_size) damage+=3000+Random(3000);
				ProjectileHit(obj, damage, ProjectileHit_no_query_catch_blow_callback | ProjectileHit_exact_damage | ProjectileHit_no_on_projectile_hit_callback, FX_Call_EngGetPunched);
				
				// object has not been deleted?
				if(obj)
				{
					if(offset_y)
						ApplyWeaponBash(obj, 100, 0);
					else
						if(!first)
							ApplyWeaponBash(obj, damage/50, Angle(0, 0, angle, -10));
					else
						if(!offset_y)
							DoWeaponSlow(obj, 300);
					
					// Particle effect
					var x=-1;
					var p="Slice2";
					if(Contained()->GetDir() == DIR_Right)
					{
						x=1;
						p="Slice1";
					} 
					CreateParticle(p, AbsX(obj->GetX())+RandomX(-1,1), AbsY(obj->GetY())+RandomX(-1,1), 0, 0, 100, RGB(255,255,255), obj);
				}
				
				// sound and done. We can only hit one target
				Sound(Format("WeaponHit%d.ogg", 1+Random(3)), false);
				break;
			}
		}
	}

}

