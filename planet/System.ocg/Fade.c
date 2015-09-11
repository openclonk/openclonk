/**
	Fade.c
	Function to fade in and out objects.
	
*/

global func FadeOut(int time, bool del)
{
	// If there is an existing effect, remove it.
	if (GetEffect("ObjFade", this)) 
		RemoveEffect("ObjFade", this);
	
	// Add the effect itself.
	var effect = AddEffect("ObjFade", this, 1, 1);
	effect.fade_time = time;
	effect.fade_out = true;
	// Delete the object when fade-out is done.
	effect.delete = del;
	return effect;
}

global func FadeIn(int time)
{
	// If there is an existing effect, remove it.
	if (GetEffect("ObjFade", this)) 
		RemoveEffect("ObjFade", this);
	
	// Add the effect itself.
	var effect = AddEffect("ObjFade", this, 1, 1);
	effect.fade_time = time;
	return effect;
}

global func FxObjFadeTimer(object target, proplist effect, int timer)
{
	// Is the fade timer up?
	if (timer >= effect.fade_time)
	{
		// Delete object at end if specified.
		if (effect.delete) 
			target->RemoveObject();
		else
		{
			if (effect.fade_out)
			{
				// Callback to object when alpha is completely transparent.
				target->~OnFadeDisappear();
				target->SetObjAlpha(0);
			}
			else
			{
				// Callback to object when alpha is fully opaque.
				target->~OnFadeAppear();
				target->SetObjAlpha(255);
			}
		}
		return FX_Execute_Kill;
	}
	
	// Find out what the alpha should be.
	var alpha = (timer * 1000 / effect.fade_time) * 255 / 1000;
	
	// Does the object fade out or in?
	if (effect.fade_out) 
		alpha = 255 - alpha;
	
	// Shade object's alpha to match time.
	target->SetObjAlpha(alpha);
	return FX_OK;
}