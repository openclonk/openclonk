/*-- Arrow target --*/

#strict 2

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func QueryCatchBlow(obj)
{
//	obj->Schedule("RemoveObject", 1);
	DoFireworks();
	return 1;
}

func DoFireworks(int speed)
{
	var color = HSL(Random(8)*32,255,127);

	if(!speed) speed = 12;
	for(var i=0; i<36; ++i)
	{
		var oangle = Random(70);
		var num = AddEffect("Firework", nil, 300, 1, nil, BOOM, Cos(oangle,speed), i*10+Random(5), GetX(), GetY());
		EffectVar(4,nil,num) = color;
	}

	for(var i=0; i<16; ++i)
	{
		CreateParticle("ExploSmoke",RandomX(-80,80),RandomX(-80,80),0,0,RandomX(500,700),RGBa(255,255,255,90));
	}
	CastParticles("Spark",60,190,0,0,40,70,color,color);

	CreateParticle("Flash",0,0,0,0,3500,color | (200 & 255)<<24);

	Explode(30);
}