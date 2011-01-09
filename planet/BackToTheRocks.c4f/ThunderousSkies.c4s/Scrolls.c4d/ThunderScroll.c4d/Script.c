/*--
	Scroll: Thunder
	Author: Mimmo

	Call down a devastating storm down from above.
--*/


public func ControlUse(object pClonk)
{
	Sound("Blast3");
	Exit(0,-GetY());
	AddEffect("ThunderStrike",0,100,1,0,this->GetID(),pClonk->GetOwner(),this->GetX()-5);
	RemoveObject();
	return 1;
}
global func FxThunderStrikeStart(pTarget, iEffectNumber, iTemp, owner, x)
{
	if(iTemp) return;
	EffectVar(0, pTarget, iEffectNumber)=owner;
	EffectVar(1, pTarget, iEffectNumber)=x;
}
global func FxThunderStrikeTimer(pTarget, iEffectNumber, iEffectTime)
{
	var move = EffectVar(1, pTarget, iEffectNumber);

	if(iEffectTime>36)
	{

	var owner = EffectVar(0, pTarget, iEffectNumber);
	var x=0;
	var wdt = 18;
	var y = [];
	var targets=[];
	for(var i = (x-wdt); i < (wdt*2); i++ )
	{
		
		while(!GBackSolid(i+move,y[i+wdt]) && y[i+wdt] < LandscapeHeight())
			y[i+wdt]++;

	}
	
	for(var i = (x-wdt); i < (wdt*2); i++ )
	{
		if(!(i%5))
			for(var k=0; k<y[i+wdt]; k+=10+Random(5))
			{	
					CreateParticle("LightningSpark",i+move,k,RandomX(-12,12),RandomX(-40,-10),40+Random(21),RGB(255-Random(100),255-Random(50),255-Random(10)),nil,Random(2));
			}
		
		for(var l=0; l<3; l++)
			CreateParticle("LightningSpark",i+move,y[i+wdt]-l-2,RandomX(-20,20),-20-Random(10),30+Random(30),RGB(255-Random(50),255-Random(30),255-Random(5)),nil,Random(2));
		if(i%3) CreateParticle("LightningStrike",i+move,y[i+wdt]-l-10,0,0,64,RGBa(255-Random(50),255-Random(30),255-Random(5),255-Random(20)),nil,Random(2));
		
		for(var t in FindObjects(Find_Or(Find_And(Find_ID(Clonk),Find_OCF(OCF_Alive)), Find_ID(TargetBalloon)),Find_OnLine(i+move,-0,i+move,y[i+wdt])))
		{
			var add=true;
			for(var j=0; j<GetLength(targets); j++)
				if(targets[j] == t) add=false;
			if(add) targets[GetLength(targets)] = t;
		}
	}
	
	for(var t in targets)
	{
		if(t->GetID() == TargetBalloon)
		{
			var arw=CreateObject(Arrow,0,0,owner);
			t->OnProjectileHit(arw);
			arw->RemoveObject();
		}
		else
		{
			if(t->GetOwner() != owner)
			{
				t->DoEnergy(-15,0,0,owner);
				t->Fling(BoundBy((t->GetX()-(move))/4,-3,3),-6);
			}
		}
	}
	return -1;
	}
	else if(iEffectTime<4)
	{
		if(iEffectTime%3)
		{
		
			while(!GBackSolid(x+move+5,y) && y < LandscapeHeight())
			{
				var add=Random((iEffectTime*5))*((Random(2)*2) -1);
				CreateParticle("Air",x+move+5+add,y,0,RandomX(-2,1),10+(iEffectTime*5),RGB(255-Random(50),255-Random(30),255-Random(5)));
				y+=Random(4)+3;
			}
		}
	}
}




local Name = "$Name$";
local Description = "$Description$";
