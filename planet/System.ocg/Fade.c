global func FadeOut(int time, bool del)
{
	//if there is an existing effect, remove it
	if(GetEffect("ObjFade", this)) RemoveEffect("ObjFade", this);
	
	//add the effect itself
	var effect = AddEffect("ObjFade", this, 1,1);
	effect.fadeTime = time;
	effect.delete = del; //delete the object when fade-out is done
	effect.FadeOut = true;
	return effect;
}

global func FadeIn(int time)
{
	//if there is an existing effect, remove it
	if(GetEffect("ObjFade", this)) RemoveEffect("ObjFade", this);
	
	//add the effect itself
	var effect = AddEffect("ObjFade", this, 1,1);
	effect.fadeTime = time;
	return effect;
}

global func FxObjFadeTimer(object target, proplist effect, int timer)
{
	//Is the fade timer up?
	if(timer >= effect.fadeTime){
		//delete object at end if specified
		if(effect.delete) target->RemoveObject();
		else{
			if(effect.FadeOut){
				//Callback to object when alpha is completely transparent
				target->~OnFadeDisappear();
				target->SetObjAlpha(0);
			}
			else{
				//Callback to object when alpha is fully opaque
				target->~OnFadeAppear();
				target->SetObjAlpha(255);
			}
		}
		return -1;
	}
	
	//find out what the alpha should be
	var alpha = (timer * 1000 / effect.fadeTime) * 255 / 1000;
	
	//Does the object fade out or in?
	if(effect.FadeOut) alpha = 255 - alpha;
	
	//shade object's alpha to match time
	target->SetObjAlpha(alpha);
}