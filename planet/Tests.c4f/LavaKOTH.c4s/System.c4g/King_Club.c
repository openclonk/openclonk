#appendto Club

local king_size;

public func MakeKingSize() { king_size = true;  SetMeshMaterial("KingClub",0);}
public func MakeNormalSize() { king_size = false;  SetMeshMaterial("Club",0);}
public func Departure() { MakeNormalSize(); }

func DoStrike(clonk, angle)
{
	var x=Sin(angle, 7);
	var y=-Cos(angle, 7);
	var found=false;
	for(var obj in FindObjects(Find_Distance(7, x, y), Find_Or(Find_OCF(OCF_Alive), Find_Category(C4D_Object)), Find_Exclude(clonk), Find_NoContainer(), Find_Layer(GetObjectLayer())))
	{
		if(obj->Stuck()) continue;
		
		var en=Format("CannotBeHitTwiceBy%d", this->ObjectNumber());
		if(GetEffect(en, obj)) continue;
		
		if(obj->GetOCF() & OCF_Alive)
		{
			var damage=5*1000;
			if(king_size) damage+=3000;
			var f=ApplyShieldFactor(clonk, obj, damage);
			ApplyWeaponBash(obj, 200, angle);
			obj->DoEnergy(-damage, true, FX_Call_EngGetPunched, clonk->GetOwner());
		}
		else
		{
			var div=100;
			if(obj->GetContact(-1)) div*=10;
			var k=1;
			if(king_size) k=3;
			// mass factor
			var fac1=10000/Max(2, obj->GetMass());
			var fac2=BoundBy(10-Abs(obj->GetDefCoreVal("Width", "DefCore")-obj->GetDefCoreVal("Height", "DefCore")), 1, 10);
			var speed=(2000 * fac1 * fac2) / 10 / 1000;
			obj->SetXDir(((obj->GetXDir(100) + Sin(angle, speed)) * k) / 2, div);
			obj->SetYDir(((obj->GetYDir(100) - Cos(angle, speed)) * k) / 2, div);
		}
		AddEffect(en, obj, 1, 15, 0);
		found=true;
		break;
	}
	
	if(found)
		RemoveEffect("DuringClubShoot", clonk);
}

