/**
	Pipe

	Author: ST-DDT, Marky

	The pipe has several states:
	- neutral
	- source (green line)
	- drain (red line)

	By default a pipe is neutral, and stays neutral if you connect it to a liquid tank.
	- Connected neutral pipes cannot be connected to another (neutral) liquid tank
	  (this would not make any sense, or maybe it does if you want to have a liquid
	   tank array, but this would lead to all kinds of complications and special
	   cases eventually).

	However, a pipe can be connected to a liquid transferring structure (pump), too.
	- Neutral pipes can be connected to a pump. They then become a drain or source pipe.
	- Connected neutral pipes can be connected to a pump. See above.
	- Source pipes can be connected to liquid tanks only.
	- Drain pipes can be connected to liquid tanks only.

	Logic for object use from inventory and interaction menu:
	- Both know the using Clonk (interaction menu: the container of the pipe)
	- The user may want to connect a drain pipe before connecting a source pipe
	- The user may want to connect a neutral pipe
	=> separate functions are necessary	
*/

static const PIPE_STATE_Neutral = nil;
static const PIPE_STATE_Source = "Source";
static const PIPE_STATE_Drain = "Drain";
static const PIPE_STATE_Air = "Air";

local pipe_state = nil;

local pipe_line;

local ApertureOffsetX = 0;
local ApertureOffsetY = 3;

/* ---------- Callbacks ---------- */

protected func Hit()
{
	Sound("Hits::GeneralHit?");
}

private func Destruction()
{
	// Remove the line first, so that it does not provoke errors on destruction.
	// Actually there is an ill-defined state where line contains the pipe and is
	// removed. Then line = GetConnectedLine() causes an error, instead use the
	// slower find object variant.
	var line = FindObject(Find_Func("IsConnectedTo", this));
	if (line)
		line->RemoveObject();
	return;
}


public func IsToolProduct() { return true;}

public func OnPipeLineRemoval()
{
	SetNeutralPipe();
	OnPipeLengthChange();
	return;
}

public func OnPipeLengthChange()
{
	// Update usage bar for a possible carrier (the clonk).
	var carrier = Contained();
	if (carrier)
		carrier->~OnInventoryChange();
	return;
}

// Display the line length bar over the pipe icon.
public func GetInventoryIconOverlay()
{
	var line = GetConnectedLine();
	if (!line) return;

	var percentage = 100 * line->GetPipeLength() / line.PipeMaxLength;
	var red = percentage * 255 / 100;
	var green = 255 - red;
	// Overlay a usage bar.
	var overlay = 
	{
		Bottom = "0.75em",
		Margin = ["0.1em", "0.25em"],
		BackgroundColor = RGB(0, 0, 0),
		margin = 
		{
			Margin = "0.05em",
			bar = 
			{
				BackgroundColor = RGB(red, green, 0),
				Right = Format("%d%%", percentage),
			}
		}
	};
	return overlay;
}

public func CanBeStackedWith(object other)
{
	// Do not stack source/drain/unused pipes
	return _inherited(other) && (pipe_state == other.pipe_state);
}


/**
 The pump calls this function to prevent clogging of the intake.
 Cycles through several aperture offset indices.
 */
public func HasAperture() { return true; }
 
public func CycleApertureOffset()
{
	// Cycle in three steps of three px each through X and Y
	// covering a 3x3 grid on points -3,0,+3
	ApertureOffsetX = (ApertureOffsetX + 6) % 9 - 3;
	if (!ApertureOffsetX) ApertureOffsetY = (ApertureOffsetY + 6) % 9 - 3;
	return true;
}

/**
  Container dies: Drop connected pipes so they don't
  draw huge lines over the landscape
 */
func IsDroppedOnDeath(object clonk)
{
	return !!GetConnectedLine();
}


/* ---------- Pipe States ---------- */


public func IsNeutralPipe() { return pipe_state == PIPE_STATE_Neutral; }
public func IsDrainPipe() { return pipe_state == PIPE_STATE_Drain; }
public func IsSourcePipe() { return pipe_state == PIPE_STATE_Source; }
public func IsAirPipe() { return pipe_state == PIPE_STATE_Air; }

public func GetPipeState() { return pipe_state; }

public func SetNeutralPipe()
{
	pipe_state = PIPE_STATE_Neutral;

	SetGraphics("", nil, GFX_Overlay, GFXOV_MODE_Picture);
	Description = "$Description$";
	Name = "$Name$";

	var line = GetConnectedLine();
	if (line)
		line->SetNeutral();
}

public func SetDrainPipe()
{
	pipe_state = PIPE_STATE_Drain;
	
	SetGraphics("Drain", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
	SetObjDrawTransform(1000, 0, 0, 0, 1000, 10000, GFX_Overlay);
	Description = "$DescriptionDrain$";
	Name = "$NameDrain$";

	var line = GetConnectedLine();
	if (line)
		line->SetDrain();
}

public func SetSourcePipe()
{
	pipe_state = PIPE_STATE_Source;

	SetGraphics("Source", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
	SetObjDrawTransform(1000, 0, 0, 0, 1000, 10000, GFX_Overlay);
	Description = "$DescriptionSource$";
	Name = "$NameSource$";

	var line = GetConnectedLine();
	if (line)
		line->SetSource();
}

public func SetAirPipe()
{
	pipe_state = PIPE_STATE_Air;

	SetGraphics("Air", Pipe, GFX_Overlay, GFXOV_MODE_Picture);
	SetObjDrawTransform(1000, 0, 0, 0, 1000, 10000, GFX_Overlay);
	Description = "$DescriptionAir$";
	Name = "$NameAir$";

	var line = GetConnectedLine();
	if (line)
		line->SetAir();
}

/* ---------- Pipe Connection ---------- */


func ConnectPipeTo(object target, string specific_pipe_state, bool block_cutting)
{
	if (!target || target->~QueryConnectPipe(this)) return false;
	AddLineConnectionTo(target, block_cutting);
	target->OnPipeConnect(this, specific_pipe_state);
	Sound("Objects::Connect");
	return true;
}

/* ---------- Line Connection ---------- */


public func SetPipeLine(to_line)
{
	pipe_line = to_line;
}

/**
 Finds a line that is connected to this pipe kit.
 @return object the pipe, or nil if nothing was found.
 */
public func GetConnectedLine()
{
	return pipe_line;
}


/**
 Connects a line to an object.

 The pipe kit will connect the line to the target object and itself first.
 Otherwise, if the pipe kit already has a line, it connects that line to the target.

 Note: Reports a fatal error if the line would be connected to more than two targets
       at the same time.
      
 @par target the target object
 */
func AddLineConnectionTo(object target, bool block_cutting)
{
	var line = GetConnectedLine();
	if (line)
	{
		if (line->IsConnectedTo(this, true))
		{
			line->SwitchConnection(this, target);
			SetPipeLine(line);
			line.BlockPipeCutting = block_cutting;
			ScheduleCall(this, this.Enter, 1, nil, line); // delayed entrance, so that the message is still displayed above the clonk
			return line;
		}
		else
		{
			FatalError("This line is connected to two objects already!");
		}
	}
	else
	{
		return CreateLine(target, block_cutting);
	}
}


/**
 Cuts the connection between the line and an object.
 
 Note: Reports a fatal error if the target was not
       connected to the line.
 
 @par target the target object
 */
func CutLineConnection(object target)
{
	var line = GetConnectedLine();
	if (!line) return;

	// connected only to the kit and a structure
	if (line->IsConnectedTo(this, true)) 
	{
		target->OnPipeDisconnect(this);
		line->RemoveObject();
	}
	// connected to the target and another structure
	else if (line->IsConnectedTo(target, true))
	{
		target->OnPipeDisconnect(this);
		Exit(); // the kit was inside the line at this point.
		SetPosition(target->GetX(), target->GetY() + target->GetBottom() - this->GetBottom());
		line->SwitchConnection(target, this);
		SetPipeLine(line);
	}
	else
	{
		FatalError(Format("An object %v is trying to cut the pipe connection, but only objects %v and %v may request a disconnect", target, line->GetActionTarget(0), line->GetActionTarget(1)));
	}
}

// Returns whether the cutting pipe is blocked.
public func QueryCutLineConnection(object target)
{
	var line = GetConnectedLine();
	if (!line)
		return false;
	return line.BlockPipeCutting;
}

/**
 Creates a new pipe line that is connected to this pipe kit.
 @par target the target object.
 @return object the line that was created
 */
func CreateLine(object target, bool block_cutting)
{
	// Create and connect pipe line.
	pipe_line = CreateObject(PipeLine, 0, 0, NO_OWNER);
	pipe_line->SetActionTargets(this, target);
	pipe_line->SetPipeKit(this);
	pipe_line.BlockPipeCutting = block_cutting;
	return pipe_line;
}


/** Will connect liquid line to building at the clonk's position. */
protected func ControlUse(object clonk, int x, int y)
{
	var target = FindObject(Find_AtPoint(), Find_Func("CanConnectPipe"));
	if (target)
	{
		ConnectPipeTo(target);
	}
	return true;
}


/**
 Displays a message at top-level container of this object.
 @par message the message
 */
func Report(string message)
{
	var reporter = this;
	var next = Contained();
	
	while(next)
	{
		reporter = next;
		next = reporter->Contained();
	}
	
	reporter->Message(message, ...);
}


/*-- Saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (pipe_line) props->AddCall("PipeLine", this, "SetPipeLine", pipe_line);
	if (IsNeutralPipe()) props->AddCall("PipeStateNeutral", this, "SetNeutralPipe");
	else if (IsDrainPipe()) props->AddCall("PipeStateDrain", this, "SetDrainPipe");
	else if (IsSourcePipe()) props->AddCall("PipeStateSource", this, "SetSourcePipe");
	else if (IsAirPipe()) props->AddCall("PipeStateAir", this, "SetAirPipe");
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Metal = 1};
