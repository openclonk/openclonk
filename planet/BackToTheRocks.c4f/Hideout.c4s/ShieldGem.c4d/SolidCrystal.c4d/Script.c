/*-- CrystalShield --*/


protected func Initialize()
{
	AddEffect("Selfdestruction",this,100,4+Random(2),this,this->GetID());
	return;
}

func FxSelfdestructionTimer(object target, int noum, int timer)
{
	CreateParticle("Magic",RandomX(-4,4),RandomX(-4,4),0,0,12+Random(10),target->GetClrModulation());
 	if(timer>175) target->RemoveObject();
 	return 1;
}

