/**
	Ambience sounds
	
	Use this object to play various ambience sounds.
	
	@author Marky
*/

/* -- Internals and engine callbacks -- */

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;


private func Construction()
{
	CreateEffect(FxAmbience, 1, 30);
}

local FxAmbience = new Effect
{
	Construction = func()
	{
		this.Chance = 13;
		this.Condition = nil;
		this.SoundName = nil;
		this.SoundOpts = nil;
		this.Wait = 0;
	},

	Timer = func(int time)
	{
		if (time < this.Wait || !this.SoundName)
			return FX_OK;
	
		// Condition for making the sound must be fulfilled
		if (!this.Condition || this.Target->Call(this.Condition, this))
		{
			if (Random(100) < this.Chance)
			{
				this.Target->Sound(this.SoundName, this.SoundOpts);
			}
		}
		return FX_OK;
	},
};


/* -- Interface -- */

/**
 Sets the chance for sounds to occur.
 
 @par change Once every 'interval' frames, the object tries
             playing a sound - but only 'chance'% of the
             time.
             Passing 'nil' will reset the interval
             to the default value of 13%, which is roughly
             1/8 times.

 @return object This object, so that you can issue further
                function calls conveniently.
 */
public func SetChance(int chance)
{
	var fx = GetEffect("FxAmbience", this);
	if (fx)
	{
		fx.Chance = chance ?? 13;
	}
	return this;
}


/**
 Sets an icon, so that you can distinguish the sounds in the editor view.
 */
public func SetIcon(id icon)
{
	SetGraphics(nil, icon, 1, GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(1000, 0, GetDefWidth() * 300, 0, 1000, GetDefHeight() * 200, 1);
	return this;
}


/**
 Sets the interval for sounds.
 
 @par interval The interval between sounds playing, in frames.
               Passing 'nil' will reset the interval
               to the default value of 30 frames.
               
 @return object This object, so that you can issue further
                function calls conveniently.
 */
public func SetInterval(int interval)
{
	var fx = GetEffect("FxAmbience", this);
	if (fx)
	{
		fx.Interval = interval ?? 30;
	}
	return this;
}


/**
 Sets the sound options for the sound.
 
 @par options A proplist with options for the Sound() function.
              
              For example, SetOptions({volume = 50}) will play
              the sounds at 50% volume.

 @return object This object, so that you can issue further
                function calls conveniently.
 */
public func SetOptions(proplist options)
{
	var fx = GetEffect("FxAmbience", this);
	if (fx)
	{
		fx.SoundOptions = options;
	}
	return this;
}


/**
 Sets the sound that should be played.
 
 @par name The sound name, as if played by the Sound() function.     
 @return object This object, so that you can issue further
                function calls conveniently.
 */
public func SetSound(string name)
{
	var fx = GetEffect("FxAmbience", this);
	if (fx)
	{
		fx.SoundName = name;
	}
	return this;
}


/**
 Sets a condition when the sound should be played.
 
 @par condition A function or function name for a condition.
                This function is called by the ambience sound
                object, so it either has to be a global function
                or one that the ambience sound object has.
                Sounds are played only if the condition returns true.
                
                For example, SetCondition(Global.IsNight) will result
                in sounds playing only at night.
                
 @return object This object, so that you can issue further
                function calls conveniently.
 */
public func SetCondition(condition)
{
	var fx = GetEffect("FxAmbience", this);
	if (fx)
	{
		fx.Condition = condition;
	}
	return this;
}


/**
 Wait at least this many frames before the (next) sound starts playing.
 
 @par frames The amount of frames to wait.
 @return object This object, so that you can issue further
                function calls conveniently.
 */
public func Wait(int frames)
{
	var fx = GetEffect("FxAmbience", this);
	if (fx)
	{
		fx.Time = 0;
		fx.Wait = frames;
	}
	return this;
}
