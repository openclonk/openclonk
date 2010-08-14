/*
	ObjectFade Rule
	Author: Caesar

	Removes unused objects. Like Hazard-Arena.
	
	TODO:
	Use ChangeEffect properly to save calls
*/

local fade_time;

func Initialize() {
	if(ObjectCount(Find_ID(Rule_ObjectFade), Find_Exclude(this))) {
		FindObject(Find_ID(Rule_ObjectFade), Find_Exclude(this))->LocalN("fade_time") += 36;
		return RemoveObject();
	}
	fade_time = 18; // 18, because the timer will check once per second, so it's aproximately a second.
}

func Timer() {
	for(var fade in FindObjects(Find_Category(C4D_Object), Find_NoContainer(), Find_Not(Find_OCF(OCF_HitSpeed1)))) {
		if(fade->GetXDir() || fade->GetYDir() || fade->GetEffect("IntFadeOut", fade)) continue;
		fade->AddEffect("IntFadeOut", fade, 1, /*fade_time * 2/3*/1, this, Rule_ObjectFade);
	}
}

public func FxIntFadeOutStart(object target, int num) {
	EffectVar(0, target, num) = target->GetX();
	EffectVar(1, target, num) = target->GetY();
	EffectVar(3, target, num) = target->GetClrModulation() & 0x00ffffff; //Safe pure rgb
	EffectVar(4, target, num) = target->GetClrModulation() >> 24 & 255; //Safe alpha
}

public func FxIntFadeOutTimer(object target, int num, int time) {
	if(time < fade_time * 2/3) return;
	//Log("%d,%d,%08x,%08x => %08x", fade_time, time, EffectVar(4, target, num), EffectVar(3, target, num), ((fade_time - time) * EffectVar(4, target, num) / (fade_time/3)));
	if(!(target->Contained()) 
		&& EffectVar(0, target, num) == target->GetX()
		&& EffectVar(1, target, num) == target->GetY()
	) {
		if(time >= fade_time) {
			target->RemoveObject();
			return -1;
		}
	} else {
		target->SetClrModulation(EffectVar(4, target, num) << 24 | EffectVar(3, target, num));
		return -1;
	}
	EffectVar(0, target, num) = target->GetX();
	EffectVar(1, target, num) = target->GetY();
	/*if(!EffectVar(2, target, num)) {
		EffectVar(2, target, num) = true;
		ChangeEffect(0, target, num, 0, 1);
	}*/
	target->SetClrModulation(((fade_time - time) * EffectVar(4, target, num) / (fade_time/3)) << 24 | EffectVar(3, target, num));
}

public func FxIntFadeOutTimerEffect(string new_effect_name) {
	if(new_effect_name == "IntFadeOut")
		return -1;
}

func Definition(def) {
	SetProperty("Name", "Object Fade", def);
}