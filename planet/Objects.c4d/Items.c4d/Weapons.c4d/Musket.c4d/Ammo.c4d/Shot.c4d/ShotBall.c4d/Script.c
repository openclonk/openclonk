/*-- Shot Ball --*/

#strict 2

protected func Hit()
{
	RemoveEffect("HitCheck",this);
	Schedule("RemoveObject()", 50);
	SetVelocity(Random(359));
}

public func AffectShotBall(object shooter)
{
	AddEffect("HitCheck", this, 1,1, nil,nil,shooter);
}

private func HitObject(object pVictim)
{
	Punch(pVictim,7);
	if(pVictim->GetOCF() & OCF_Living)
	{
		if(Random(3)==1) SetAction("Tumble");
	}
	RemoveObject();
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}