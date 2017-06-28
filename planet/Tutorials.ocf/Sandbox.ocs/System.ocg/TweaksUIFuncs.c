
#appendto Clonk

func TweaksUI_SetInvincibility(newValue)
{
	if (newValue == true)
		Log("$TweakInvincible_Activated$", this->GetName());
	else
		Log("$TweakInvincible_Deactivated$", this->GetName());
	
	return this->SetInvincibility(newValue);
}