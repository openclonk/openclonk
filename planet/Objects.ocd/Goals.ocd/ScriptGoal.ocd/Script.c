/*--
		Script goal
		Author: Maikel
		
		The script goal can be fulfilled from other scripts, e.g. a scenario script.
--*/


#include Library_Goal

local fulfilled;

protected func Initialize()
{
	fulfilled = false;
	return inherited(...);
}

public func IsFulfilled()
{
	return fulfilled;
}

public func Fulfill()
{
	fulfilled = true;
	return;
}

public func SetFulfilled(bool to_val)
{
	fulfilled = to_val;
	return;
}

public func GetDescription(int plr)
{
	return GetTranslatedString(this.Description);
}

public func GetPictureDefinition(int plr)
{
	return this.Picture ?? this;
}

public func GetPictureName(int plr)
{
	return this.PictureName ?? "";
}

/* Editor */

local overlay_picture;

public func Definition(def)
{
	// Properties
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.Description = { Name="$PropDescription$", EditorHelp="$PropDescriptionHelp$", Type="string", Save="Description", Translatable=true };
	def.EditorProps.overlay_picture = { Name="$Picture$", EditorHelp="$PictureHelp$", Type="def", Set="SetOverlayPicture", Save="Picture" };
	// User actions
	UserAction->AddEvaluator("Action", "Game", "$SetScriptGoalData$", "$SetScriptGoalDataDesc$", "set_script_goal_data", [def, def.EvalAct_SetData],
			{ Target = { Function="action_object" }, Description = { Function="string_constant", Value="$Description$" }, Fulfilled = { Function="bool_constant", Value=false } },
			{ Type="proplist", Display="{{Description}}, {{OverlayPicture}}, {{Fulfilled}}",
		EditorProps = {
			Target = new UserAction->GetObjectEvaluator("IsScriptGoal", "$Goal$") { Priority=50 },
			Description = new UserAction.Evaluator.String { Name="$PropDescription$", EditorHelp="$PropDescriptionHelp$" },
			OverlayPicture = new UserAction.Evaluator.Definition { Name="$Picture$", EditorHelp="$PictureHelp$" },
			Fulfilled = new UserAction.Evaluator.Boolean { Name="$Fulfilled$", EditorHelp="$FulfilledHelp$" }
		} } );
}

public func SetOverlayPicture(to_picture_def)
{
	// Picutre + Overlay
	overlay_picture = to_picture_def;
	SetGraphics(nil, to_picture_def, 1, GFXOV_MODE_Picture);
	return true;
}

public func IsScriptGoal() { return true; }

private func EvalAct_SetData(props, context)
{
	var target = UserAction->EvaluateValue("Objct", props.Target, context);
	if (!target || !target->~IsScriptGoal()) return;
	target.Description = UserAction->EvaluateString(props.Description, context);
	target->~SetOverlayPicture(UserAction->EvaluateValue("Definition", props.OverlayPicture, context));
	target->~SetFulfilled(UserAction->EvaluateValue("Boolean", props.Fulfilled, context));
}

/*-- Proplist --*/
local Name = "$Name$";
local Description = "$Description$";
