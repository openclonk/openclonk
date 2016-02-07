/**
	Animals can use Decay() in their Death() function so they will slowly decay and spawn a few temporary flies.
*/

global func Decay()
{
	AddEffect("Decaying", this, 1, 500);
	if (!GBackSemiSolid())
	{
		var rnd = Random(4);
		for (var i = 0; i < rnd; i++)
		{
			var mos = CreateObject(Mosquito);
			ScheduleCall(mos, "RemoveObject", 9000 + Random(300));
		}
	}
}

global func FxDecayingTimer(object target)
{
	if (target->GetCon() < 20)
		target->RemoveObject();
	else
		target->DoCon(-5);
	return true;
}
