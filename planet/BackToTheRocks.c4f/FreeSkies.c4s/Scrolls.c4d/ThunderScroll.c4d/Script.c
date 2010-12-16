/*--
	Scroll: Thunder
	Author: Mimmo

	Call down a devastating storm down from above.
--*/


public func ControlUse(object pClonk)
{
	Sound("Blast3");
	Exit(-3,-GetY());
	var x;
	var wdt = 15;
	var y = [];
	var targets=[];
	for(var i = (x-wdt); i < (wdt*2); i++ )
	{
		
		while(!GBackSolid(i,y[i+wdt]) && y[i+wdt] < LandscapeHeight())
			y[i+wdt]++;
	}
	
	for(var i = (x-wdt); i < (wdt*2); i++ )
	{
		if(!(i%3))
			for(var k=0; k<y[i+wdt]; k+=12+Random(6))
			{
				CreateParticle("Air",i,k,RandomX(-1,1),RandomX(-1,1),20+Random(41),RGB(255-Random(100),255-Random(50),255-Random(10)),nil,Random(2));
				if(!Random(4)) 
					CreateParticle("AirIntake",i,k,RandomX(-1,1),RandomX(-5,1),40+Random(21),RGB(255-Random(100),255-Random(50),255-Random(10)),nil,Random(2));
			}
		
		for(var l=0; l<3; l++)
			CreateParticle("AirIntake",i,y[i+wdt]-l-2,i+RandomX(-10,10),-10+Random(6),30+Random(30),RGB(255-Random(50),255-Random(30),255-Random(5)),nil,Random(2));
		
		for(var t in FindObjects(Find_Or(Find_And(Find_ID(Clonk),Find_OCF(OCF_Alive)), Find_ID(TargetBalloon)),Find_OnLine(i,-0,i,y[i+wdt])))
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
			var arw=CreateObject(Arrow,0,0,pClonk->GetOwner());
			t->OnProjectileHit(arw);
			arw->RemoveObject();
		}
		else
		{
			if(t!=pClonk)
			{
				t->DoEnergy(-10,0,0,pClonk->GetOwner());
				t->Fling((t->GetX()-pClonk->GetX())/5,-4);
			}
		}
	}
	
	RemoveObject();
	return 1;
}





local Name = "$Name$";
local Description = "$Description$";
