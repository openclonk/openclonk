/**
	Animals can use Decay() in their Death() function so they will slowly decay and spawn a few temporary flies.
	The delay parameter specifies the amount of frames before decaying one Con.
*/

global func Decay(int delay)
{
	delay = delay ?? 100;
	AddEffect("Decaying", this, 1, delay);
	if (!GBackSemiSolid())
	{
		var rnd = Random(4);
		for (var i = 0; i < rnd; i++)
		{
			var mos = CreateObject(Mosquito);
			ScheduleCall(mos, "RemoveObject", Max(0, GetCon() - 20) * delay + Random(300));
		}
	}
}

global func FxDecayingTimer(object target, effect fx, int time)
{
	if (target->GetCon() < 20)
		target->RemoveObject();
	else
		target->DoCon(-1);
	return FX_OK;
}
