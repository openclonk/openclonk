/*-- Shield --*/

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryTransform(clonk, sec, back)
{
	if(mTrans != nil) return mTrans;
	if(!sec)
	{
		if(back) return Trans_Mul(Trans_Rotate(-90, 0, 0, 1),Trans_Translate(0,0,-400));
		return nil;
	}
	if(back) return Trans_Mul(Trans_Mul(Trans_Rotate(90, 0, 0, 1),Trans_Rotate(180, 0, 1)),Trans_Translate(0,0,-400));
	return Trans_Rotate(180,1,0,0);
}

local mTrans;

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 10000, def);
  SetProperty("PerspectiveTheta", 0, def);
  SetProperty("PerspectivePhi", 0, def);
}