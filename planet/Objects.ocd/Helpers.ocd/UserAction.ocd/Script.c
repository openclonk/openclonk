/* User action execution handler */
// Handles actions set in editor e.g. for dialogues, switches, etc.
// An object is sometimes needed to show a menu or start a timer, so this definition is created whenever a user action is run

local Name = "UserAction";
local Plane = 0;

/* UserAction definition */

// Base classes for EditorProps using actions 
local Evaluator;

// EditorProps for generic user action callbacks
local Prop, PropProgressMode, PropParallel;

// Base props for action execution conditions
local ActionEvaluation ;

// Proplist containing callback function. Indexed by option names.
local EvaluatorCallbacks;

// Proplist containing option definitions. Indexed by option names.
local EvaluatorDefs;

// Call this definition early (but after EditorBase) to allow EditorProp initialization
local DefinitionPriority = 99;

// Localized group names
local GroupNames = { Structure="$Structure$", Game="$Game$", Ambience="$Ambience$", Disasters="$Disasters$" };

// Storage for global user variables
static g_UserAction_global_vars;

// Localized evaluator names
local EvaluatorTypeNames = {
	Action = "$UserAction$",
	Color = "$UserColor$",
	Object = "$UserObject$",
	ObjectList = "$UserObjectList$",
	Definition = "$UserDefinition$",
	Player = "$UserPlayer$",
	PlayerList = "$UserPlayerList$",
	Boolean = "$UserBoolean$",
	Integer = "$UserInteger$",
	String = "$UserString$",
	Position = "$UserPosition$",
	Offset = "$UserOffset$"
};

// All evaluator types (unfortunately, EvaluatorReturnTypes->GetProperties() does not work)
local EvaluatorTypes = ["Action", "Object", "ObjectList", "Definition", "Player", "PlayerList", "Boolean", "Integer", "Color", "String", "Position", "Offset", "Any"];

// Evaluator return types
local EvaluatorReturnTypes = {
	Action = C4V_Nil,
	Object = C4V_C4Object,
	ObjectList = [C4V_C4Object],
	Definition = C4V_Def,
	Player = C4V_Int,
	PlayerList = [C4V_Int],
	Boolean = C4V_Bool,
	Integer = C4V_Int,
	String = C4V_String,
	Position = [C4V_Int, 2],
	Offset = [C4V_Int, 2],
	Any = C4V_Nil
};

// Array of specialized object evaluators
local object_evaluators;

func Definition(def)
{
	// Typed evaluator base definitions
	Evaluator = {};
	Evaluator.Action = { Name="$UserAction$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	Evaluator.Object = { Name="$UserObject$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	Evaluator.ObjectList = { Name="$UserObjectList$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	Evaluator.Definition = { Name="$UserDefinition$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	Evaluator.Player = { Name="$UserPlayer$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$Noone$", Priority = 100 } ] };
	Evaluator.PlayerList = { Name="$UserPlayerList$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$Noone$", Priority = 100 } ] };
	Evaluator.Boolean = { Name="$UserBoolean$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	Evaluator.Integer = { Name="$UserInteger$", Type="enum", OptionKey="Function", Sorted = true, Options = [ {Name="0", Priority = 100 } ] };
	Evaluator.Color = { Name="$UserColor$", Type="enum", OptionKey="Function", Sorted = true, Options = [ {Name="$Default$", Priority = 100 } ] };
	Evaluator.String = { Name="$UserString$", Type="enum", OptionKey="Function", Sorted = true, Options = [ {Name="($EmptyString$)", Priority = 100 } ] };
	Evaluator.Position = { Name="$UserPosition$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$Here$", Priority = 100 } ] };
	Evaluator.Offset = { Name="$UserOffset$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	Evaluator.Any = { Name="$UserAny$", Type="enum", OptionKey="Function", Sorted = true, Options = [ { Name="$None$", Priority = 100 } ] };
	EvaluatorCallbacks = {};
	EvaluatorDefs = {};
	// Object constant evaluator may be needed early be evaluators referencing filtered objects
	AddEvaluator("Object", nil, ["$ConstantObject$", ""], "$ConstantObjectHelp$", "object_constant", [def, def.EvalConstant], { Value = nil }, { Type="object", Name="$Value$" });
		// Action evaluators
	AddEvaluator("Action", "$Sequence$", "$Sequence$", "$SequenceHelp$", "sequence", [def, def.EvalAct_Sequence], { Actions=[] }, { Type="proplist", DescendPath="Actions", HideFullName = true, Display="{{Actions}}", EditorProps = {
		Actions = { Name="$Actions$", Type="array", Elements = Evaluator.Action },
		} } );
	AddEvaluator("Action", "$Sequence$", "$Goto$", "$GotoHelp$", "goto", [def, def.EvalAct_Goto], { Index={Function="int_constant", Value = 0} }, new Evaluator.Integer { Name="$Index$" }, "Index");
	AddEvaluator("Action", "$Sequence$", "$StopSequence$", "$StopSequenceHelp$", "stop_sequence", [def, def.EvalAct_StopSequence]);
	AddEvaluator("Action", "$Sequence$", "$SuspendSequence$", "$SuspendSequenceHelp$", "suspend_sequence", [def, def.EvalAct_SuspendSequence]);
	AddEvaluator("Action", "$Sequence$", "$Wait$", "$WaitHelp$", "wait", [def, def.EvalAct_Wait], { Time = 60 }, { Type="proplist", Display="{{Time}}", EditorProps = {
		Time = { Name="$Time$", Type="int", Min = 1 }
		} } );
	AddEvaluator("Action", "$Sequence$", "$WaitForcondition$", "$WaitForConditionHelp$", "wait_condition", [def, def.EvalAct_WaitCondition], { Interval = 20 }, { Type="proplist", Display="{{Condition}}", EditorProps = {
		Interval = { Name="$CheckInterval$", Type="int", Min = 1 },
		Condition = new Evaluator.Boolean { Name="$Condition$", EditorHelp="$WaitConditionHelp$", Priority = 60 }
		} } );
	AddEvaluator("Action", "$Ambience$", "$Sound$", "$SoundHelp$", "sound", [def, def.EvalAct_Sound], { Pitch={Function="int_constant", Value = 0}, Volume={Function="int_constant", Value = 100}, TargetPlayers={Function="all_players"} }, { Type="proplist", Display="{{Sound}}", EditorProps = {
		Sound = { Name="$SoundName$", EditorHelp="$SoundNameHelp$", Type="sound", AllowEditing = true, Priority = 100 },
		Pitch = new Evaluator.Integer { Name="$SoundPitch$", EditorHelp="$SoundPitchHelp$" },
		Volume = new Evaluator.Integer { Name="$SoundVolume$", EditorHelp="$SoundVolumeHelp$" },
		Loop = { Name="$SoundLoop$", EditorHelp="$SoundLoopHelp$", Type="enum", Options=[
			{ Name="$SoundLoopNone$" },
			{ Name="$SoundLoopOn$", Value=+1 },
			{ Name="$SoundLoopOff$", Value=-1 }
			] },
		TargetPlayers = new Evaluator.PlayerList { EditorHelp="$SoundTargetPlayersHelp$" },
		SourceObject = new Evaluator.Object { Name="$SoundSourceObject$", EditorHelp="$SoundSourceObjectHelp$", EmptyName="$Global$" }
		} } );
	AddEvaluator("Action", "$Object$", "$CreateObject$", "$CreateObjectHelp$", "create_object", [def, def.EvalAct_CreateObject], { SpeedX={Function="int_constant", Value = 0},SpeedY={Function="int_constant", Value = 0},SpeedR={Function="int_constant", Value = 0},Rotation={Function="int_constant", Value = 0} }, { Type="proplist", Display="{{ID}}", EditorProps = {
		ID = new Evaluator.Definition { EditorHelp="$CreateObjectDefinitionHelp$", Priority = 100 },
		Position = new Evaluator.Position { EditorHelp="$CreateObjectPositionHelp$" },
		CreateAbove = { Name="$CreateObjectCreationOffset$", EditorHelp="$CreateObjectCreationOffsetHelp$", Type="enum", Options=[
			{ Name="$Center$" },
			{ Name="$Bottom$", Value = true }
			]},
		Owner = new Evaluator.Player { Name="$Owner$", EditorHelp="$CreateObjectOwnerHelp$" },
		Container = new Evaluator.Object { Name="$Container$", EditorHelp="$CreateObjectContainerHelp$" },
		SpeedX = new Evaluator.Integer { Name="$SpeedX$", EditorHelp="$CreateObjectSpeedXHelp$" },
		SpeedY = new Evaluator.Integer { Name="$SpeedY$", EditorHelp="$CreateObjectSpeedYHelp$" },
		Rotation = new Evaluator.Integer { Name="$Rotation$", EditorHelp="$CreateObjectRotationHelp$" },
		SpeedR = new Evaluator.Integer { Name="$SpeedR$", EditorHelp="$CreateObjectSpeedRHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$CastObjects$", "$CastObjectsHelp$", "cast_objects", [def, def.EvalAct_CastObjects], { Amount={Function="int_constant", Value = 8},Speed={Function="int_constant", Value = 20},AngleDeviation={Function="int_constant", Value = 360} }, { Type="proplist", Display="{{Amount}}x{{ID}}", EditorProps = {
		ID = new Evaluator.Definition { EditorHelp="$CastObjectsDefinitionHelp$", Priority = 100 },
		Position = new Evaluator.Position { EditorHelp="$CastObjectsPositionHelp$" },
		Amount = new Evaluator.Integer { Name="$Amount$", EditorHelp="$CastObjectsAmountHelp$" },
		Speed = new Evaluator.Integer { Name="$Speed$", EditorHelp="$CastObjectsSpeedHelp$" },
		MeanAngle = new Evaluator.Integer { Name="$MeanAngle$", EditorHelp="$CastObjectsMeanAngleHelp$" },
		AngleDeviation = new Evaluator.Integer { Name="$AngleDeviation$", EditorHelp="$CastObjectsAngleDeviationHelp$" },
		Owner = new Evaluator.Player { Name="$Owner$", EditorHelp="$CastObjectsOwnerHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$RemoveObject$", "$RemoveObjectHelp$", "remove_object", [def, def.EvalAct_RemoveObject], { }, { Type="proplist", Display="{{Object}}", EditorProps = {
		Object = new Evaluator.Object { EditorHelp="$RemoveObjectObject$", Priority = 100 },
		EjectContents = { Name="$EjectContents$", EditorHelp="$EjectContentsHelp$", Type="enum", Options=[
			{ Name="$EjectContentsNo$" },
			{ Name="$EjectContentsYes$", Value = true }
			] },
		} } );
	AddEvaluator("Action", "$Object$", "$SetPosition$", "$SetPositionHelp$", "set_position", [def, def.EvalAct_SetPosition], { Object={ Function="triggering_clonk" }, Position={ Function="position_constant_rel" } }, { Type="proplist", Display="({{Object}}->{{Position}})", EditorProps = {
		Object = new Evaluator.Object { Name="$Object$", EditorHelp="$SetPositionObjectHelp$" },
		Position = new Evaluator.Position { Name="$Position$", EditorHelp="$SetPositionPositionHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$Fling$", "$FlingHelp$", "fling", [def, def.EvalAct_Fling], { Object={ Function="triggering_clonk" }, SpeedX={ Function="int_constant", Value = 0 }, SpeedY={ Function="int_constant", Value=-20 }, AddSpeed={ Function="bool_constant", Value = false } }, { Type="proplist", Display="({{Object}}, {{SpeedX}}, {{SpeedY}})", EditorProps = {
		Object = new Evaluator.Object { Name="$Object$", EditorHelp="$FlingObjectHelp$" },
		SpeedX = new Evaluator.Integer { Name="$SpeedX$", EditorHelp="$FlingSpeedXHelp$" },
		SpeedY = new Evaluator.Integer { Name="$SpeedY$", EditorHelp="$FlingSpeedYHelp$" },
		AddSpeed = new Evaluator.Boolean { Name="$AddSpeedY$", EditorHelp="$FlingAddSpeedHelp$" },
		} } );
	AddEvaluator("Action", "$Object$", "$EnterObject$", "$EnterObjectHelp$", "enter_object", [def, def.EvalAct_EnterObject], { }, { Type="proplist", Display="{{Object}} -> {{Container}}", EditorProps = {
		Object = new Evaluator.Object { EditorHelp="$EnterObjectObjectHelp$", Priority = 90 },
		Container = new Evaluator.Object { Name="$Container$", EditorHelp="$EnterObjectContainerHelp$", Priority = 80 },
		CollectionCheck = { Name="$CollectionCheck$", EditorHelp="$CollectionCheckHelp$", Type="enum", Options=[
			{ Name="$CollectionCheckIgnore$" },
			{ Name="$CollectionCheckCheck$", Value="check" },
			{ Name="$CollectionCheckExit$", Value="exit" }
			] }
		} } );
	AddEvaluator("Action", "$Object$", "$ExitObject$", "$ExitObjectHelp$", "exit_object", [def, def.EvalAct_ExitObject], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Action", "$Object$", "$SetVisibility$", "$SetVisibilityHelp$", "set_visibility", [def, def.EvalAct_SetVisibility], { Object={ Function="triggering_clonk" }, Visibility = VIS_All }, { Type="proplist", Display="({{Object}}, {{Visibility}})", EditorProps = {
		Object = new Evaluator.Object { Name="$Object$" },
		Visibility = { Name="$Visibility$", Type="enum", Options = [{ Name="$Visible$", Value = VIS_All }, { Name="$Invisible$", Value = VIS_None }] }
		} } );
	AddEvaluator("Action", "$Object$", "$SetVincibility$", "$SetVincibilityHelp$", "set_vincibility", [def, def.EvalAct_SetVincibility], { Object={ Function="triggering_clonk" }, Vincibility = false }, { Type="proplist", Display="({{Object}}, {{Vincibility}})", EditorProps = {
		Object = new Evaluator.Object { Name="$Object$" },
		Vincibility = { Name="$Vincibility$", Type="enum", Options = [{ Name="$Invincible$", Value = false }, { Name="$Vincible$", Value = true }] }
		} } );
	AddEvaluator("Action", "Clonk", "$DoEnergy$", "$DoEnergyHelp$", "do_energy", [def, def.EvalAct_ObjectCallInt, Global.DoEnergy], { Object={ Function="triggering_clonk" } }, { Type="proplist", Display="({{Object}}, {{Value}})", EditorProps = {
		Object = new Evaluator.Object { Name="$Object$", EditorHelp="$DoEnergyObjectHelp$" },
		Value = new Evaluator.Integer { Name="$ValueChange$", EditorHelp="$DoEnergyValueChangeHelp$" }
		} } );
	AddEvaluator("Action", "Clonk", "$SetDirection$", "$SetDirectionHelp$", "set_direction", [def, def.EvalAct_SetDirection], { Object={ Function="triggering_clonk" }, Direction = DIR_Left }, { Type="proplist", Display="({{Object}}, {{Direction}})", EditorProps = {
		Object = GetObjectEvaluator("IsClonk", "$Clonk$"),
		Direction = { Name="$Direction$", Type="enum", Options=[{ Name="$Left$", Value = DIR_Left }, { Name="$Right$", Value = DIR_Right }] }
		} } );
	AddEvaluator("Action", "Ambience", "$CastParticles$", "$CastParticlesHelp$", "cast_particles", [def, def.EvalAct_CastParticles], {
			Name="StarFlash",
			Amount={Function="int_constant", Value = 8},
			Speed={Function="int_constant", Value = 20},
			Lifetime={Function="int_constant", Value = 100},
			Size={Function="int_constant", Value = 10},
			SizeEnd={Function="int_constant", Value = 1},
			Color={Function="color_constant", Value = 0xffff},
			BlitMode = 0,
			Gravity={Function="int_constant", Value = 100},
			FadeOut = true,
			CollisionFunc="bounce"
		}, { Type="proplist", Display="{{Amount}}x{{Name}}", EditorProps = {
		Name = { Name="$ParticleName$", EditorHelp="$ParticleNameHelp$", Type="enum", Priority = 50, Sorted = true, Options = [
			{ Name="$Dust$", Value="Dust" },
			{ Name="$Flash$", Value="Flash" },
			{ Name="$Magic$", Value="Magic" },
			{ Name="$Smoke$", Value="Smoke" },
			{ Name="$Sphere$", Value="Sphere" },
			{ Name="$StarFlash$", Value="StarFlash" },
			{ Name="$StarSpark$", Value="StarSpark" },
			{ Name="$Fire$", Value="MagicFire" },
			{ Name="$Ring$", Value="MagicRing" }
			] },
		Position = new Evaluator.Position { EditorHelp="$CastObjectsPositionHelp$" },
		Amount = new Evaluator.Integer { Name="$Amount$", EditorHelp="$CastParticlesAmountHelp$" },
		Speed = new Evaluator.Integer { Name="$Speed$", EditorHelp="$CastParticlesSpeedHelp$" },
		Lifetime = new Evaluator.Integer { Name="$Lifetime$", EditorHelp="$CastParticlesLifetimeHelp$" },
		Size = new Evaluator.Integer { Name="$Size$", EditorHelp="$CastParticlesSizeHelp$" },
		SizeEnd = new Evaluator.Integer { Name="$SizeEnd$", EditorHelp="$CastParticlesSizeEndHelp$" },
		Color = new Evaluator.Color { Name="$Color$", EditorHelp="$CastParticlesColorHelp$" },
		BlitMode = { Name="$BlitMode$", EditorHelp="$ParticleBlitModeHelp$", Type="enum", Options = [
			{ Name="$Normal$", Value = 0 },
			{ Name="$Additive$", Value = GFX_BLIT_Additive },
			{ Name="$Mod2$", Value = GFX_BLIT_Mod2 }
			] },
		Gravity = new Evaluator.Integer { Name="$Gravity$", EditorHelp="$ParticleGravityHelp$" },
		FadeOut = { Name="$FadeOut$", EditorHelp="$ParticleFadeOutHelp$", Type="bool" },
		CollisionFunc = { Name="$CollisionFunc$", EditorHelp="$ParticleCollisionFuncHelp$", Type="enum", Options = [
			{ Value="pass", Name="$Pass$" },
			{ Value="stop", Name="$Stop$" },
			{ Value="bounce", Name="$Bounce$" },
			{ Value="die", Name="$Die$" }
			] }
		} } );
	AddEvaluator("Action", "$Player$", "$DoWealth$", "$DoWealthHelp$", "do_wealth", [def, def.EvalAct_DoWealth], { Player={ Function="triggering_player" }, DoSound={ Function="bool_constant", Value = true } }, { Type="proplist", Display="({{Player}}, {{Change}})", EditorProps = {
		Player = Evaluator.Player,
		Change = new Evaluator.Integer { Name="$Change$", EditorHelp="$DoWealthChangeHelp$" },
		DoSound = new Evaluator.Boolean { Name="$Sound$", EditorHelp="$DoWealthSoundHelp$", Priority=-1 }
		} } );
	AddEvaluator("Action", "$Player$", "$PlrKnowledge$", "$PlrKnowledgeHelp$", "plr_knowledge", [def, def.EvalAct_PlrKnowledge], { Players={ Function="triggering_player_list" }, ID={ Function="def_constant" } }, { Type="proplist", Display="({{Players}}, {{ID}})", EditorProps = {
		Players = Evaluator.PlayerList,
		ID = Evaluator.Definition
		} } );
	AddEvaluator("Action", "$Player$", "$SetPlrView$", "$SetPlrViewHelp$", "plr_view", [def, def.EvalAct_PlrView], { Players={ Function="triggering_player_list" }, Target={ Function="action_object" } }, { Type="proplist", Display="({{Players}}, {{Target}})", EditorProps = {
		Players = Evaluator.PlayerList,
		Target = new Evaluator.Object { Name="$Target$", EditorHelp="$PlrViewTargetHelp$" },
		Immediate = { Name="$ScrollMode$", EditorHelp="$SetPlrViewScrollModeHelp$", Type="enum", Priority=-10, Options = [
			{ Name="$Smooth$" },
			{ Value = true, Name="$Immediate$" }
			] }
		} } );
	AddEvaluator("Action", "$Script$", "$ConditionalAction$", "$ConditionalActionHelp$", "if", [def, def.EvalAct_If], { }, { Type="proplist", Display="if ({{Condition}}) {{TrueEvaluator}} else {{FalseEvaluator}}", EditorProps = {
		Condition = new Evaluator.Boolean { Name="$Condition$", EditorHelp="$IfConditionHelp$", Priority = 60 },
		TrueEvaluator = new Evaluator.Action { Name="$TrueEvaluator$", EditorHelp="$TrueEvaluatorHelp$", Priority = 50 },
		FalseEvaluator = new Evaluator.Action { Name="$FalseEvaluator$", EditorHelp="$FalseEvaluatorHelp$", Priority = 30 }
		} } );
	AddEvaluator("Action", "$Script$", "$SetVariable$", "$SetVariableHelp$", "set_variable", [def, def.EvalAct_SetVariable], { VariableName={ Function="string_constant", Value="" } }, { Type="proplist", Display="{{Context}}::{{VariableName}}={{Value}}", EditorProps = {
		Context = new Evaluator.Object { Name="$Context$", EditorHelp="$VariableContextHelp$", EmptyName="$Global$" },
		VariableName = new Evaluator.String { Name="$VariableName$", EditorHelp="$VariableNameHelp$" },
		Value = new Evaluator.Any { Name="$Value$", EditorHelp="$SetVariableValueHelp$" }
		} } );
	AddEvaluator("Action", "$Script$", "$ForInteger$", "$ForIntegerHelp$", "for_int", [def, def.EvalAct_For, def.EvalAct_For_IntRange], { Start={ Function="int_constant", Value = 1}, End={ Function="int_constant", Value = 10}, Step={ Function="int_constant", Value = 1} }, { Type="proplist", HideFullName = true, Display="for ({{Start}}:{{Step}}:{{End}}) {{Action}}", EditorProps = {
		Action = new Evaluator.Action { Name="$UserAction$", EditorHelp="$ForActionHelp$", Priority = 100 },
		Start = new Evaluator.Integer { Name="$Start$", EditorHelp="$ForStartHelp$", Priority = 90 },
		End = new Evaluator.Integer { Name="$End$", EditorHelp="$ForEndHelp$", Priority = 80 },
		Step = new Evaluator.Integer { Name="$Step$", EditorHelp="$ForStepHelp$", Priority = 70 }
		} } );
	AddEvaluator("Action", "$Script$", "$ForPlayer$", "$ForPlayerHelp$", "for_player", [def, def.EvalAct_For, def.EvalAct_For_PlayerList], { Players={ Function="all_players" } }, { Type="proplist", HideFullName = true, Display="for ({{Players}}) {{Action}}", EditorProps = {
		Action = new Evaluator.Action { Name="$UserAction$", EditorHelp="$ForActionHelp$", Priority = 100 },
		Players = new Evaluator.PlayerList { EditorHelp="$ForPlayersHelp$" }
		} } );
	AddEvaluator("Action", "$Script$", "$ForObject$", "$ForObjectHelp$", "for_object", [def, def.EvalAct_For, def.EvalAct_For_ObjectList], { }, { Type="proplist", HideFullName = true, Display="for ({{Objects}}) {{Action}}", EditorProps = {
		Action = new Evaluator.Action { Name="$UserAction$", EditorHelp="$ForActionHelp$", Priority = 100 },
		Objects = new Evaluator.ObjectList { EditorHelp="$ForObjectsHelp$" }
		} } );
	AddEvaluator("Action", "$Script$", "$Log$", "$LogHelp$", "log", [def, def.EvalAct_Log], { }, new Evaluator.String { Name="$LogMessage$", EditorHelp="$LogMessageHelp$" }, "Message");
	AddEvaluator("Action", "$Script$", "$Comment$", "$CommentHelp$", "comment", [def, def.EvalAct_Nop], { Comment="" }, { Name="$Comment$", EditorHelp="$CommentHelp$", Type="string" }, "Comment");
	AddEvaluator("Action", "Game", "$GameOver$", "$GameOverHelp$", "game_over", [def, def.EvalAct_GameOver]);
	// Object evaluators
	AddEvaluator("Object", nil, "$ActionObject$", "$ActionObjectHelp$", "action_object", [def, def.EvalContextValue, "action_object"]);
	AddEvaluator("Object", nil, "$TriggerClonk$", "$TriggerClonkHelp$", "triggering_clonk", [def, def.EvalContextValue, "triggering_clonk"]);
	AddEvaluator("Object", nil, "$TriggerObject$", "$TriggerObjectHelp$", "triggering_object", [def, def.EvalContextValue, "triggering_object"]);
	AddEvaluator("Object", nil, "$IteratedObject$", "$IteratedObjectHelp$", "iterated_object", [def, def.EvalContextValue, "for_object"]);
	AddEvaluator("Object", nil, "$LastCreatedObject$", "$LastCreatedObjectHelp$", "last_created_object", [def, def.EvalContextValue, "last_created_object"]);
	var find_object_in_area_delegate = { Type="proplist", Display="{{ID}}", EditorProps = {
		ID = new Evaluator.Definition { Name="$ID$", EditorHelp="$FindObjectsIDHelp$", EmptyName="$Any$", Priority = 51 },
		Area = { Name="$SearchArea$", EditorHelp="$SearchAreaHelp$", Type="enum", OptionKey="Function", Priority = 41, Options=[
			{ Name="$SearchAreaWholeMap$", EditorHelp="$SearchAreaWholeMapHelp$" },
			{ Name="$SearchAreaInRect$", EditorHelp="$SearchAreaInRectHelp$", Value={ Function="InRect" }, DefaultValueFunction = def.GetDefaultRect, ValueKey="Area", Delegate={ Type="rect", Name="$Rectangle$", Relative = false, Color = 0xffff00 } },
			{ Name="$SearchAreaAtRect$", EditorHelp="$SearchAreaAtRectHelp$", Value={ Function="AtRect" }, DefaultValueFunction = def.GetDefaultRect, ValueKey="Area", Delegate={ Type="rect", Name="$Rectangle$", Relative = false, Color = 0xffff80 } },
			{ Name="$SearchAreaCircle$", EditorHelp="$SearchAreaCircleHelp$", Value={ Function="Circle" }, DefaultValueFunction = def.GetDefaultCircle, ValueKey="Area", Delegate={ Type="circle", Name="$Circle$", Relative = false, CanMoveCenter = true, Color = 0xff00ff } },
			{ Name="$SearchAreaNearPosition$", EditorHelp="$SearchAreaNearPositionHelp$", Value={ Function="NearPosition", Parameters={Radius = 25} }, ValueKey="Parameters", Delegate={ Type="proplist", Display="({{Position}}, {{Radius}})", EditorProps = {
				Position = new Evaluator.Position { EditorHelp="$SearchAreaNearPositionPositionHelp$"},
				Radius = { Type="circle", Relative = true, Name="$Radius$", Color = 0xff80ff }
				} } }
			] },
		AllowContained = { Name="$AllowContained$", EditorHelp="$AllowContainedHelp$", Type="bool", Priority = 31 }
		} };
	var find_object_in_container_delegate = { Type="proplist", Display="{{ID}} in {{Container}}", EditorProps = {
		ID = new Evaluator.Definition { Name="$ID$", EditorHelp="$FindObjectsIDHelp$", EmptyName="$Any$" },
		Container = new Evaluator.Object { Name="$Container$", EditorHelp="FindObjectsContainerHelp" }
		} };
	AddEvaluator("Object", nil, "$FindObjectInArea$", "$FindObjectInAreaHelp$", "find_object_in_area", [def, def.EvalObjList_FindObjectsInArea, true], {}, find_object_in_area_delegate);
	AddEvaluator("Object", nil, "$FindObjectInContainer$", "$FindObjectInContainerHelp$", "find_object_in_container", [def, def.EvalObjList_FindObjectInContainer], {}, find_object_in_container_delegate);
	// Object list evaluators
	AddEvaluator("ObjectList", nil, "$FindObjectsInArea$", "$FindObjectsInAreaHelp$", "find_objects_in_area", [def, def.EvalObjList_FindObjectsInArea], {}, find_object_in_area_delegate);
	AddEvaluator("ObjectList", nil, "$FindObjectsInContainer$", "$FindObjectsInContainerHelp$", "find_objects_in_container", [def, def.EvalObjList_FindObjectsInContainer], {}, find_object_in_container_delegate);
	// Definition evaluators
	AddEvaluator("Definition", nil, ["$Constant$", ""], "$ConstantHelp$", "def_constant", [def, def.EvalConstant], { Value = nil }, { Type="def", Name="$Value$" });
	AddEvaluator("Definition", nil, "$TypeOfObject$", "$TypeOfObjectHelp$", "type_of_object", [def, def.EvalObjProp, Global.GetID], { }, new Evaluator.Object { }, "Object");
	// Player evaluators
	AddEvaluator("Player", nil, "$TriggeringPlayer$", "$TriggeringPlayerHelp$", "triggering_player", [def, def.EvalContextValue, "triggering_player"]);
	AddEvaluator("Player", nil, "$OwnerOfObject$", "$OwnerOfObjectHelp$", "owner", [def, def.EvalObjProp, Global.GetOwner], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Player", nil, "$ControllerOfObject$", "$ControllerOfObjectHelp$", "owner", [def, def.EvalObjProp, Global.GetController], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Player", nil, "$IteratedPlayer$", "$IteratedPlayerHelp$", "iterated_player", [def, def.EvalContextValue, "for_player"]);
	// Player list evaluators
	AddEvaluator("PlayerList", nil, "$TriggeringPlayer$", "$TriggeringPlayerHelp$", "triggering_player_list", [def, def.EvalPlrList_Single, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$AllPlayers$", "$AllPlayersHelp$", "all_players", [def, def.EvalPlrList_All]);
	// Boolean (condition) evaluators
	AddEvaluator("Boolean", nil, ["$Constant$", ""], "$ConstantHelp$", "bool_constant", [def, def.EvalConstant], { Value = true }, { Type="bool", Name="$Value$" });
	AddEvaluator("Boolean", "$Comparison$", "$CompareInteger$", "$ComparisonHelp$", "compare_int", [def, def.EvalComparison, "Integer"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", EditorProps = {
		LeftOperand = new Evaluator.Integer { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority = 44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority = 43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			{ Name="<", EditorHelp="$LessThanHelp$", Value="lt" },
			{ Name=">", EditorHelp="$GreaterThanHelp$", Value="gt" },
			{ Name="<=", EditorHelp="$LessOrEqualHelp$", Value="le" },
			{ Name=">=", EditorHelp="$GreaterOrEqualHelp$", Value="ge" }
			] },
		RightOperand = new Evaluator.Integer { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority = 42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareBoolean$", "$ComparisonHelp$", "compare_bool", [def, def.EvalComparison, "Boolean"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", EditorProps = {
		LeftOperand = new Evaluator.Object { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority = 44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority = 43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Object { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority = 42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareObject$", "$ComparisonHelp$", "compare_object", [def, def.EvalComparison, "Object"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", EditorProps = {
		LeftOperand = new Evaluator.Object { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority = 44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority = 43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Object { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority = 42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareString$", "$ComparisonHelp$", "compare_string", [def, def.EvalComparison, "String"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", EditorProps = {
		LeftOperand = new Evaluator.String { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority = 44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority = 43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.String { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority = 42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareDefinition$", "$ComparisonHelp$", "compare_definition", [def, def.EvalComparison, "Definition"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", EditorProps = {
		LeftOperand = new Evaluator.Definition { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority = 44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority = 43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Definition { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority = 42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$ComparePlayer$", "$ComparisonHelp$", "compare_player", [def, def.EvalComparison, "Player"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", EditorProps = {
		LeftOperand = new Evaluator.Player { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority = 44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority = 43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Player { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority = 42 }
		} } );
	AddEvaluator("Boolean", "$Logic$", "$Not$", "$NotHelp$", "not", [def, def.EvalBool_Not], { }, new Evaluator.Boolean { }, "Operand");
	AddEvaluator("Boolean", "$Logic$", "$And$", "$AndHelp$", "and", [def, def.EvalBool_And], { Operands=[] }, { Type="proplist", DescendPath="Operands", Display="{{Operands}}", EditorProps = {
		Operands = { Name="$Operands$", Type="array", Elements = Evaluator.Boolean }
		} } );
	AddEvaluator("Boolean", "$Logic$", "$Or$", "$OrHelp$", "or", [def, def.EvalBool_Or], { Operands=[] }, { Type="proplist", DescendPath="Operands", Display="{{Operands}}", EditorProps = {
		Operands = { Name="$Operands$", Type="array", Elements = Evaluator.Boolean }
		} } );
	AddEvaluator("Boolean", nil, "$ObjectExists$", "$ObjectExistsHelp$", "object_exists", [def, def.EvalBool_ObjectExists], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Boolean", nil, "$ObjectAlive$", "$ObjectAliveHelp$", "object_alive", [def, def.EvalBool_ObjectAlive], { }, new Evaluator.Object { }, "Object");
	// Integer evaluators
	AddEvaluator("Integer", nil, ["$Constant$", ""], "$ConstantHelp$", "int_constant", [def, def.EvalConstant], { Value = 0 }, { Type="int", Name="$Value$" });
	var arithmetic_delegate = { Type="proplist", HideFullName = true, EditorProps = {
		LeftOperand = new Evaluator.Integer { Name="$LeftOperand$", EditorHelp="$LeftArithmeticOperandHelp$", Priority = 44 },
		RightOperand = new Evaluator.Integer { Name="$RightOperand$", EditorHelp="$RightArithmeticOperandHelp$", Priority = 42 }
		} };
	AddEvaluator("Integer", "$Arithmetic$", "$Sum$ (+)", "$SumHelp$", "add", [def, def.EvalInt_Add], { }, new arithmetic_delegate { Display="{{LeftOperand}}+{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Sub$ (-)", "$SumHelp$", "subtract", [def, def.EvalInt_Sub], { }, new arithmetic_delegate { Display="{{LeftOperand}}-{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Mul$ (*)", "$MulHelp$", "multiply", [def, def.EvalInt_Mul], { }, new arithmetic_delegate { Display="{{LeftOperand}}*{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Div$ (/)", "$DivHelp$", "divide", [def, def.EvalInt_Div], { }, new arithmetic_delegate { Display="{{LeftOperand}}/{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Mod$ (%)", "$ModHelp$", "modulo", [def, def.EvalInt_Mod], { }, new arithmetic_delegate { Display="{{LeftOperand}}%{{RightOperand}}" });
	AddEvaluator("Integer", nil, "$Random$", "$RandomIntHelp$", "int_random", [def, def.EvalIntRandom], { Min={Function="int_constant", Value = 0}, Max={Function="int_constant", Value = 99} }, { Type="proplist", HideFullName = true, Display="Rnd({{Min}}..{{Max}})", EditorProps = {
		Min = new Evaluator.Integer { Name="$Min$", EditorHelp="$RandomMinHelp$", Priority = 51 },
		Max = new Evaluator.Integer { Name="$Max$", EditorHelp="$RandomMaxHelp$", Priority = 21 }
		} } );
	AddEvaluator("Integer", nil, "$Distance$", "$DistanceHelp$", "distance", [def, def.EvalInt_Distance], { }, { Type="proplist", HideFullName = true, Display="d({{PositionA}}..{{PositionB}})", EditorProps = {
		PositionA = new Evaluator.Position { Name="$PositionA$", EditorHelp="$PositionAHelp$" },
		PositionB = new Evaluator.Position { Name="$PositionB$", EditorHelp="$PositionBHelp$" }
		} } );
	AddEvaluator("Integer", nil, "$NumberOfObjects$", "$NumberOfObjectsHelp$", "object_count", [def, def.EvalCount, "ObjectList"], { }, new Evaluator.ObjectList { }, "Array");
	AddEvaluator("Integer", nil, "$NumberOfPlayers$", "$NumberOfPlayersHelp$", "player_count", [def, def.EvalCount, "PlayerList"], { }, new Evaluator.PlayerList { }, "Array");
	AddEvaluator("Integer", nil, "$PlayerWealth$", "$PlayerWealthHelp$", "player_wealth", [def, def.EvalInt_Wealth], { }, new Evaluator.Player { }, "Player");
	AddEvaluator("Integer", nil, "$ClonkEnergy$", "$ClonkEnergyHelp$", "clonk_energy", [def, def.EvalObjProp, Global.GetEnergy], { }, GetObjectEvaluator("IsClonk", "$Clonk$"), "Object");
	AddEvaluator("Integer", nil, "$ObjectMass$", "$ObjectMassHelp$", "object_mass", [def, def.EvalObjProp, Global.GetMass], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Integer", nil, "$ObjectSpeed$", "$ObjectSpeedHelp$", "object_speed", [def, def.EvalObjProp, Global.GetSpeed], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Integer", nil, "$PositionX$", "$PositionXHelp$", "position_x", [def, def.EvalInt_PosCoord, 0], { }, new Evaluator.Position { }, "Position");
	AddEvaluator("Integer", nil, "$PositionY$", "$PositionYHelp$", "position_y", [def, def.EvalInt_PosCoord, 1], { }, new Evaluator.Position { }, "Position");
	AddEvaluator("Integer", nil, "$IteratedInteger$", "$IteratedIntegerHelp$", "iterated_int", [def, def.EvalContextValue, "for_int"]);
	// String evaluators
	AddEvaluator("String", nil, ["$Constant$", ""], "$ConstantHelp$", "string_constant", [def, def.EvalStringConstant], { Value="" }, { Type="string", Name="$Value$", Translatable = true });
	AddEvaluator("String", nil, ["$ValueToString$", ""], "$ValueToStringHelp$", "value_to_string", [def, def.EvalStr_ValueToString], { }, new Evaluator.Any { });
	AddEvaluator("String", nil, "$Concat$", "$ConcatHelp$", "string_concat", [def, def.EvalStr_Concat], { Substrings=[] }, { Type="proplist", HideFullName = true, DescendPath="Substrings", Display="{{Substrings}}", EditorProps = {
		Substrings = { Name="$Substrings$", Type="array", Elements = Evaluator.String }
		} } );
	// Color evaluators
	AddEvaluator("Color", nil, ["$Constant$", ""], "$ConstantHelp$", "color_constant", [def, def.EvalConstant], { Value = 0xffffff }, { Type="color", Name="$Value$" });
	AddEvaluator("Color", nil, "$RandomColor$", "$RandomColorHelp$", "random_color", [def, def.EvalClr_Random], { ColorA={ Function="color_constant", Value = 0 }, ColorB={ Function="color_constant", Value = 0xffffff } }, { Type="proplist", Display="({{ColorA}}..{{ColorB}})", EditorProps = {
		ColorA = new Evaluator.Color { Name="$ColorA$" },
		ColorB = new Evaluator.Color { Name="$ColorB$" }
		} } );
	AddEvaluator("Color", nil, "$PlayerColor$", "$PlayerColorHelp$", "player_color", [def, def.EvalClr_PlayerColor], { Player={ Function="triggering_player" } }, new Evaluator.Player { }, "Player");
	AddEvaluator("Color", nil, "$RGB$", "$RGBHelp$", "rgb_color", [def, def.EvalClr_RGB], { R={ Function="int_constant", Value = 255 }, G={ Function="int_constant", Value = 255 }, B={ Function="int_constant", Value = 255 } }, { Type="proplist", Display="({{R}}, {{G}}, {{B}})", EditorProps = {
		R = new Evaluator.Integer { Name="$Red$", Priority = 51 },
		G = new Evaluator.Integer { Name="$Green$", Priority = 41 },
		B = new Evaluator.Integer { Name="$Blue$", Priority = 31 }
		} } );
	// Position evaluators
	AddEvaluator("Position", nil, ["$ConstantPositionAbsolute$", ""], "$ConstantPositionAbsoluteHelp$", "position_constant", [def, def.EvalConstant], def.GetDefaultPosition, { Type="point", Name="$Position$", Relative = false, Color = 0xff2000 });
	AddEvaluator("Position", nil, ["$ConstantPositionRelative$", "+"], "$ConstantPositionRelativeHelp$", "position_constant_rel", [def, def.EvalPositionRelative], { Value=[0, 0] }, { Type="point", Name="$Position$", Relative = true, Color = 0xff0050 });
	AddEvaluator("Position", nil, "$Coordinates$", "$CoordinatesHelp$", "position_coordinates", [def, def.EvalCoordinates], def.GetDefaultCoordinates, { Type="proplist", HideFullName = true, Display="({{X}},{{Y}})", EditorProps = {
		X = new Evaluator.Integer { Name="X", EditorHelp="$PosXHelp$" },
		Y = new Evaluator.Integer { Name="Y", EditorHelp="$PosYHelp$" }
		} } );
	AddEvaluator("Position", nil, "$PositionOffset$", "$PositionOffsetHelp$", "position_offset", [def, def.EvalPositionOffset], { }, { Type="proplist", HideFullName = true, Display="{{Position}}+{{Offset}}", EditorProps = {
		Position = new Evaluator.Position { EditorHelp="$PositionOffsetPositionHelp$", Priority = 51 },
		Offset = new Evaluator.Offset { EditorHelp="$PositionOffsetOffsetHelp$", Priority = 21 }
		} } );
	AddEvaluator("Position", nil, "$ObjectPosition$", "$ObjectPositionHelp$", "object_position", [def, def.EvalPositionObject], { Object={Function="triggering_object"} }, new Evaluator.Object { EditorHelp="$ObjectPositionObjectHelp$" }, "Object");
	AddEvaluator("Position", nil, "$LastUsePosition$", "$LastUsePositionHelp$", "use_position", [def, def.EvalPos_LastUse]);
	AddEvaluator("Position", "$RandomPosition$", "$RandomRectAbs$", "$RandomRectAbsHelp$", "random_pos_rect_abs", [def, def.EvalPos_RandomRect, false], def.GetDefaultRect, { Type="rect", Name="$Rectangle$", Relative = false, Color = 0xffff00 }, "Area");
	AddEvaluator("Position", "$RandomPosition$", "$RandomRectRel$", "$RandomRectRelHelp$", "random_pos_rect_rel", [def, def.EvalPos_RandomRect, true], { Area=[-30,-30, 60, 60] }, { Type="rect", Name="$Rectangle$", Relative = true, Color = 0x00ffff }, "Area");
	AddEvaluator("Position", "$RandomPosition$", "$RandomCircleAbs$", "$RandomCircleAbsHelp$", "random_pos_circle_abs", [def, def.EvalPos_RandomCircle, false], def.GetDefaultCircle, { Type="circle", Name="$Circle$", Relative = false, CanMoveCenter = true, Color = 0xff00ff }, "Area");
	AddEvaluator("Position", "$RandomPosition$", "$RandomCircleRel$", "$RandomCircleRelHelp$", "random_pos_circle_rel", [def, def.EvalPos_RandomCircle, true], { Area=[50, 0, 0] }, { Type="circle", Name="$Circle$", Relative = true, CanMoveCenter = true, Color = 0xa000a0 }, "Area");
	// Offset evaluators
	AddEvaluator("Offset", nil, ["$ConstantOffsetRelative$", ""], "$ConstantOffsetRelativeHelp$", "offset_constant", [def, def.EvalConstant], { Value=[0, 0] }, { Type="point", Name="$Position$", Relative = true, Color = 0xff30ff });
	AddEvaluator("Offset", nil, ["$Coordinates$", ""], "$CoordinatesHelp$", "offset_coordinates", [def, def.EvalCoordinates], { Value={X = 0, Y = 0} }, { Type="proplist", HideFullName = true, Display="({{X}},{{Y}})", EditorProps = {
		X = new Evaluator.Integer { Name="X", EditorHelp="$OffXHelp$" },
		Y = new Evaluator.Integer { Name="Y", EditorHelp="$OffYHelp$" },
		} } );
	AddEvaluator("Offset", nil, "$AddOffsets$", "$AddOffsetsHelp$", "add_offsets", [def, def.EvalOffsetAdd], { }, { Type="proplist", HideFullName = true, Display="{{Offset1}}+{{Offset2}}", EditorProps = {
		Offset1 = new Evaluator.Offset { EditorHelp="$AddOffsetOffsetHelp$" },
		Offset2 = new Evaluator.Offset { EditorHelp="$AddOffsetOffsetHelp$" }
		} } );
	AddEvaluator("Offset", nil, "$DiffPositions$", "$DiffPositionsHelp$", "diff_positions", [def, def.EvalOffsetDiff], { }, { Type="proplist", HideFullName = true, Display="{{PositionB}}-{{PositionA}}", EditorProps = {
		PositionA = new Evaluator.Position { Name="$PositionA$", EditorHelp="$PositionAHelp$" },
		PositionB = new Evaluator.Position { Name="$PositionB$", EditorHelp="$PositionBHelp$" }
		} } );
	AddEvaluator("Offset", nil, "$RandomOffRectRel$", "$RandomRectRelHelp$", "random_off_rect_rel", [def, def.EvalPos_RandomRect, "rect", false], { Area=[-30,-30, 60, 60] }, { Type="rect", Name="$Rectangle$", Relative = true, Color = 0x00ffff }, "Area");
	AddEvaluator("Offset", nil, "$RandomOffCircleRel$", "$RandomCircleRelHelp$", "random_off_circle_rel", [def, def.EvalPos_RandomCircle, "circle", false], { Area=[0, 0, 50] }, { Type="circle", Name="$Circle$", Relative = true, CanMoveCenter = true, Color = 0xa000a0 }, "Area");
	// Script evaluators
	var variable_delegate = { Type="proplist", HideFullName = true, Display="{{Context}}::{{VariableName}}", EditorProps = {
		Context = new Evaluator.Object { Name="$Context$", EditorHelp="$VariableContextHelp$", EmptyName="$Global$" },
		VariableName = new Evaluator.String { Name="$VariableName$", EditorHelp="$VariableNameHelp$", Priority = 50 } } };
	for (var eval_type in EvaluatorTypes)
	{
		var data_type = EvaluatorReturnTypes[eval_type];
		var group = nil;
		if (eval_type != "Action")
		{
			AddEvaluator(eval_type, nil, "$Variable$", "$VariableHelp$", Format("%s_variable", eval_type), [def, def.EvalVariable, data_type], { VariableName={ Function="string_constant", Value="" } }, variable_delegate);
			AddEvaluator(eval_type, nil, "$ConditionalValue$", "$ConditionalValueHelp$", Format("%s_conditional", eval_type), [def, def.EvalConditionalValue, eval_type], { }, { Type="proplist", HideFullName = true, Display="{{Condition}} ? {{TrueEvaluator}} : {{FalseEvaluator}}", EditorProps = {
				Condition = new Evaluator.Boolean { Name="$Condition$", EditorHelp="$IfConditionValueHelp$", Priority = 60 },
				TrueEvaluator = new Evaluator[eval_type] { Name="$TrueEvaluatorValue$", EditorHelp="$TrueEvaluatorValueHelp$", Priority = 50 },
				FalseEvaluator = new Evaluator[eval_type] { Name="$FalseEvaluatorValue$", EditorHelp="$FalseEvaluatorValueHelp$", Priority = 30 }
			} } );
		}
		else
		{
			group = "$Script$";
		}
		AddEvaluator(eval_type, group, "$Script$", "$ScriptHelp$", Format("%s_script", eval_type), [def, def.EvalScript, data_type], { }, { Type="proplist", HideFullName = true, Display="{{Context}}::{{Script}}", EditorProps = {
			Context = new Evaluator.Object { Name="$Context$", EditorHelp="$VariableContextHelp$", EmptyName="$Global$" },
			Script = new Evaluator.String { Name="$ScriptCommand$", EditorHelp="$ScriptCommandHelp$" } } });
	}
	// User action editor props
	Prop = Evaluator.Action;
	PropProgressMode = { Name="$UserActionProgressMode$", EditorHelp="$UserActionProgressModeHelp$", Type="enum", Options = [ { Name="$Session$", Value="session" }, { Name="$PerPlayer$", Value="player" }, { Name="$Global$" } ] };
	PropParallel = { Name="$ParallelAction$", EditorHelp="$ParallelActionHelp$", Type="bool" };
	return true;
}

public func GetObjectEvaluator(filter_def, name, help)
{
	// Create copy of the Evaluator.Object delegate, but with the object_constant proplist replaced by a version with filter_def
	var object_options = Evaluator.Object.Options[:];
	// Need to copy the option Value field to ensure it is owned by the correct parent.
	// Otherwise it would be assigned by reference in the editor
	var const_option = new EvaluatorDefs["object_constant"] { Value = { Function="object_constant", Value = nil } };
	const_option.Delegate = new const_option.Delegate { Filter = filter_def };
	object_options[const_option.OptionIndex] = const_option;
	var new_evaluator = new Evaluator.Object { Name = name, Options = object_options, EditorHelp = help };
	if (!object_evaluators) object_evaluators = [];
	object_evaluators[GetLength(object_evaluators)] = new_evaluator;
	return new_evaluator;
}

private func CopyProplist(p)
{
	// Create copy of p to ensure unique global reference
	if (GetType(p) != C4V_PropList) return p;
	var r = {};
	for (var k in GetProperties(p)) r[k] = p[k];
	return r;
}

public func AddEvaluator(string eval_type, string group, name, string help, string identifier, callback_data, default_val, proplist delegate, string delegate_storage_key)
{
	// Add an evaluator for one of the data types. Evaluators allow users to write small action sequences and scripts in the editor using dropdown lists.
	// eval_type: Return type of the evaluator (Action, Object, Boolean, Player, etc. as defined in UserAction.Evaluator)
	// group [optional] Localized name of sub-group for larger enums (i.e. actions)
	// name: Localized name as it appears in the dropdown list of evaluators. May also be an array [name, short_name].
	// identifier: Unique identifier that is used to store this action in savegames and look up the action def. Identifiers must be unique among all data types.
	// callback_data: Array of [definition, definition.Function, parameter (optional)]. Function to be called when this evaluator is called
	// default_val [optional]: Default value to be set when this evaluator is selected. Must be a proplist. Should contain values for all properties in the delegate
	// delegate: Parameters for this evaluator
	// Copy all value evaluations
	if (eval_type != "Action" && eval_type != "Any" && callback_data[1] != UserAction.EvalVariable)
	{
		var any_group;
		if (group) any_group = Format("%s/%s", EvaluatorTypeNames[eval_type], group); else any_group = EvaluatorTypeNames[eval_type];
		AddEvaluator("Any", any_group, name, help, identifier, callback_data, CopyProplist(default_val), delegate, delegate_storage_key);
	}
	// Dissect parameters
	if (group) group = GroupNames[group] ?? group; // resolve localized group name
	var short_name;
	if (GetType(name) == C4V_Array)
	{
		short_name = name[1];
		name = name[0];
	}
	else if (delegate && delegate.HideFullName)
	{
		// Some proplist delegates provide their own display string and need not show the option name
		short_name = "";
	}
	if (!default_val) default_val = {};
	var default_get;
	if (GetType(default_val) == C4V_Function)
	{
		default_get = default_val;
		default_val = Call(default_get, nil, {Function = identifier});
	}
	default_val.Function = identifier;
	var action_def = { Name = name, ShortName = short_name, EditorHelp = help, Group = group, Value = default_val, Delegate = delegate, DefaultValueFunction = default_get }, n;
	if (delegate)
	{
		if (delegate.EditorProps || delegate.Elements)
		{
			// Proplist of array parameter for this evaluator: Descend path title should be name
			delegate.Name = name;
			var child_delegate = delegate;
			if (delegate.DescendPath) child_delegate = delegate.EditorProps[delegate.DescendPath];
			if (!child_delegate.EditorHelp) child_delegate.EditorHelp = help;
		}
		else
		{
			// Any other parameter type: Store in value
			action_def.ValueKey = delegate_storage_key ?? "Value";
		}
	}
	// Constant has higher priority
	if (callback_data[1] == UserAction.EvalConstant)
	{
		action_def.Priority = 50;
	}
	Evaluator[eval_type].Options[n = GetLength(Evaluator[eval_type].Options)] = action_def;
	action_def.OptionIndex = n;
	// Remember lookup table through identifier (ignore duplicates)
	if (eval_type != "Any" || !group)
	{
		EvaluatorCallbacks[identifier] = callback_data;
		EvaluatorDefs[identifier] = action_def;
	}
	// Copy any object evaluators to existing evaluator lists
	if (eval_type == "Object" && object_evaluators)
		for (var obj_eval in object_evaluators)
			obj_eval.Options[GetLength(obj_eval.Options)] = action_def;
	return action_def;
}

public func EvaluateValue(string eval_type, proplist props, proplist context)
{
	//Log("EvaluateValue %v %v %v", eval_type, props, context);
	if (!props) return nil;
	// Finish any hold-action
	if (context.hold == props)
	{
		context.hold = nil;
		return context.hold_result;
	}
	// Not on hold: Perform evaluation
	var cb = EvaluatorCallbacks[props.Function];
	/*var rval = cb[0]->Call(cb[1], props, context, cb[2]);
	Log("%v <- EvaluateValue %v %v %v", rval, eval_type, props, context);
	return rval;*/
	return cb[0]->Call(cb[1], props, context, cb[2]);
}

public func EvaluateAction(proplist props, object action_object, object triggering_object, int triggering_player, string progress_mode, bool allow_parallel, finish_callback, array position)
{
	// No action
	if (!props) if (finish_callback) return action_object->Call(finish_callback); else return;
	// Determine context
	var context;
	if (!progress_mode)
	{
		if (!(context = props._context))
			props._context = context = CreateObject(UserAction);
	}
	else if (progress_mode == "player")
	{
		if (!props._contexts) props._contexts = [];
		var plr_id = GetPlayerID(triggering_player);
		if (!(context = props._contexts[plr_id]))
			props._contexts[plr_id] = context = CreateObject(UserAction);
	}
	else // if (progress_mode == "session")
	{
		// Temporary context
		context = CreateObject(UserAction);
		context.temp = true;
	}
	// Prevent duplicate parallel execution
	if (!allow_parallel && (context.hold && !context.suspended)) return false;
	// Init context settings
	context->InitContext(action_object, triggering_player, triggering_object, props, finish_callback, position);
	// Execute action
	EvaluateValue("Action", props, context);
	FinishAction(context);
	return true;
}

public func EvaluateCondition(proplist props, object action_object, object triggering_object, int triggering_player)
{
	// Build temp context
	var context = CreateObject(UserAction);
	context.temp = true;
	// Init context settings
	context->InitContext(action_object, triggering_player, triggering_object, props);
	// Execute condition evaluator
	var result = EvaluateValue("Boolean", props, context);
	// Cleanup
	if (context) context->RemoveObject();
	// Done
	return result;
}

public func EvaluateString(proplist props, object context)
{
	return EvaluateValue("String", props, context) ?? "";
}

private func EvaluatePosition(proplist props, object context)
{
	// Execute position evaluator; fall back to position of action object
	var position = EvaluateValue("Position", props, context);
	if (!position)
	{
		if (context.action_object) position = [context.action_object->GetX(), context.action_object->GetY()];
		else position = [0, 0];
	}
	return position;
}

private func EvaluateOffset(proplist props, object context)
{
	// Execute offset evaluator; fall back to [0, 0]
	return  EvaluateValue("Offset", props, context) ?? [0, 0];
}

private func EvaluatePlayer(proplist props, object context)
{
	// Execute player evaluator; nil means NO_OWNER
	var plr = EvaluateValue("Player", props, context);
	if (!GetType(plr)) plr = NO_OWNER;
	return plr;
}

private func ResumeAction(proplist context, proplist resume_props)
{
	//Log("ResumeAction %v %v", context, resume_props);
	// Resume only if on hold for the same entry
	if (context.hold != resume_props) return;
	// Not if owning object is dead
	if (!context.action_object) return;
	// Resume action
	EvaluateValue("Action", context.root_action, context);
	// Cleanup action object (unless it ran into another hold)
	FinishAction(context);
}

private func FinishAction(proplist context)
{
	// Cleanup action object (unless it's kept around for callbacks or to store sequence progress)
	// Note that context.root_action.contexts is checked to kill session-contexts that try to suspend
	// There would be no way to resume so just kill the context
	if (!context.hold || context.temp)
	{
		if (context.action_object && context.finish_callback) context.action_object->Call(context.finish_callback, context);
		context->RemoveObject();
	}
}

private func EvalConstant(proplist props, proplist context) { return props.Value; }
private func EvalStringConstant(proplist props, proplist context) { return GetTranslatedString(props.Value); }
private func EvalContextValue(proplist props, proplist context, string name) { return context[name]; }
private func EvalObj_ActionObject(proplist props, proplist context) { return context.action_object; }
private func EvalObj_TriggeringObject(proplist props, proplist context) { return context.triggering_object; }
private func EvalObj_TriggeringClonk(proplist props, proplist context) { return context.triggering_clonk; }
private func EvalObj_LastCreatedObject(proplist props, proplist context) { return context.last_created_object; }
private func EvalPlr_Trigger(proplist props, proplist context) { return context.triggering_player; }
private func EvalPlrList_Single(proplist props, proplist context, fn) { return [Call(fn, props, context)]; }

private func EvalCount(proplist props, proplist context, data_type)
{
	var list = EvaluateValue(data_type, props.Array, context);
	if (list) return GetLength(list); else return 0;
}

private func EvalObjList_FindObjectsInArea(proplist props, proplist context, bool find_one)
{
	var params = Find_And(), np = 1;
	// Resolve area parameter
	if (props.Area)
	{
		var area = props.Area.Area;
		var fn = props.Area.Function;
		var area_criterion;
		if (fn == "InRect")
			area_criterion = Find_InRect(area[0], area[1], area[2], area[3]);
		else if (fn == "AtRect")
			area_criterion = Find_AtRect(area[0], area[1], area[2], area[3]);
		else if (fn == "Circle")
			area_criterion = Find_Distance(area[0], area[1], area[2]);
		else if (fn == "NearPosition")
		{
			var pos_params = props.Area.Parameters;
			var pos = EvaluatePosition(pos_params.Position, context);
			area_criterion = Find_Distance(pos_params.Radius, pos[0], pos[1]);
		}
		if (area_criterion) params[np++] = area_criterion;
	}
	// Other parameters
	var idobj = EvaluateValue("Definition", props.ID, context);
	if (idobj) params[np++] = Find_ID(idobj);
	if (!props.AllowContained) params[np++] = Find_NoContainer();
	// Find objects
	var result = FindObjects(params);
	if (find_one) return result[0]; else return result;
}

private func EvalObjList_FindObjectInContainer(proplist props, proplist context)
{
	var container = EvaluateValue("Object", props.Container, context);
	var idobj = EvaluateValue("Definition", props.ID, context);
	if (!container) return;
	if (idobj)
		return container->FindContents(idobj);
	else
		return container->Contents();
}

private func EvalObjList_FindObjectsInContainer(proplist props, proplist context)
{
	// Return array of all objects contained in container
	var container = EvaluateValue("Object", props.Container, context);
	var idobj = EvaluateValue("Definition", props.ID, context);
	if (!container) return; // nil is treated as empty list
	var i, n = container->ContentsCount(idobj), j;
	if (!n) return;
	var result = CreateArray(n), obj;
	while ((obj = container->Contents(i++)))
		if (!idobj || obj->GetID() == idobj)
			result[j++] = obj;
	return result;
}

private func EvalPlrList_All(proplist props, proplist context, fn)
{
	var n = GetPlayerCount(C4PT_User);
	var result = CreateArray(n);
	for (var i = 0; i<n; ++i) result[i] = GetPlayerByIndex(i);
	return result;
}

private func EvalComparison(proplist props, proplist context, data_type)
{
	var left = EvaluateValue(data_type, props.LeftOperand, context);
	var right = EvaluateValue(data_type, props.RightOperand, context);
	if (!left) left = nil; // 0 ==nil
	if (!right) right = nil; // 0 == nil
	var op = props.Operator;
	if (!op)
		return left == right;
	else if (op == "ne")
		return left != right;
	else if (op == "lt")
		return left < right;
	else if (op == "gt")
		return left > right;
	else if (op == "le")
		return left <= right;
	else if (op == "ge")
		return left >= right;
}

private func EvalBool_Not(proplist props, proplist context) { return !EvaluateValue("Boolean", props.Operand, context); }

private func EvalBool_And(proplist props, proplist context)
{
	for (var cond in props.Operands)
		if (!EvaluateValue("Boolean", cond, context))
			return false;
	return true;
}

private func EvalBool_Or(proplist props, proplist context)
{
	for (var cond in props.Operands)
		if (EvaluateValue("Boolean", cond, context))
			return true;
	return false;
}

private func EvalBool_ObjectExists(proplist props, proplist context) { return !!EvaluateValue("Object", props.Object, context); }

private func EvalBool_ObjectAlive(proplist props, proplist context)
{
	var obj = EvaluateValue("Object", props.Object, context);
	return obj && obj->GetAlive();
}

private func EvalAct_Sequence(proplist props, proplist context)
{
	// Sequence execution: Iterate over actions until one action puts the context on hold
	var n = GetLength(props.Actions), sid = props._sequence_id;
	if (!sid) sid = props._sequence_id = Format("%d", ++UserAction_SequenceIDs);
	for (var progress = context.action_data[sid] ?? 0; progress < n; ++progress)
	{
		//Log("Sequence progress exec %v %v", progress, context.hold);
		// goto preparations
		context.sequence_had_goto[sid] = false;
		context.last_sequence = props;
		// Evaluate next sequence step
		EvaluateValue("Action", props.Actions[progress], context);
		if (context.hold || context.suspended)
		{
			// Execution on hold (i.e. wait action). Stop execution for now
			if (!context.hold) progress = 0; // No hold specified: Stop with sequence reset
			context.action_data[sid] = progress;
			return;
		}
		// Apply jump in the sequence
		if (context.sequence_had_goto[sid]) progress = context.action_data[sid] - 1;
	}
	// Sequence finished
	context.last_sequence = nil;
	// Reset for next execution.
	context.action_data[sid] = 0;
}

private func EvalAct_Goto(proplist props, proplist context)
{
	// Apply goto by jumping in most recently executed sequence
	if (context.last_sequence)
	{
		var index = props.Index;
		if (GetType(index) != C4V_Int) index = EvaluateValue("Integer", index, context); // compatibility
		context.action_data[context.last_sequence._sequence_id] = index;
		context.sequence_had_goto[context.last_sequence._sequence_id] = true;
	}
}

private func EvalAct_StopSequence(proplist props, proplist context)
{
	// Stop: Suspend without hold props, which causes all sequences to reset
	context.hold = nil;
	context.suspended = true;
}

private func EvalAct_SuspendSequence(proplist props, proplist context)
{
	// Suspend: Remember hold position and stop action execution
	context.hold = props;
	context.suspended = true;
}

private func EvalAct_For_IntRange(proplist props, proplist context)
{
	// Create list with range of integers
	// Both start and end inclusive
	var start = EvaluateValue("Integer", props.Start, context);
	var end = EvaluateValue("Integer", props.End, context);
	var step = EvaluateValue("Integer", props.Step, context);
	if (!step) return [];
	var d = end - start;
	if (d * step < 0) return []; // wrong direction
	var n = (end - start) / step + 1;
	var list = CreateArray(n), i;
	for (var v = start; v <= end; v += step) list[i++] = v;
	return list;
}

private func EvalAct_For_ObjectList(proplist props, proplist context)
{
	return EvaluateValue("ObjectList", props.Objects, context) ?? [];
}

private func EvalAct_For_PlayerList(proplist props, proplist context)
{
	return EvaluateValue("PlayerList", props.Players, context) ?? [];
}

private func EvalAct_For(proplist props, proplist context, list_function)
{
	// For: Iterate over elements until one action puts the context on hold
	// list_info: [list_function, current_item_field]
	var sid = props._sequence_id;
	if (!sid) sid = props._sequence_id = Format("%d", ++UserAction_SequenceIDs);
	// Get list data
	var list, progress;
	if (!context.action_data[sid])
	{
		list = Call(list_function, props, context);
	}
	else
	{
		list = context.action_data[sid][0];
		progress = context.action_data[sid][1];
		context.action_data[sid] = nil;
	}
	var n = GetLength(list);
	for (; progress < n; ++progress)
	{
		// Get iterated item
		var curr_value = list[progress];
		if (!GetType(curr_value)) continue; // Ignore nil (deleted objects)
		context[props.Function] = curr_value;
		// Evaluate next sequence step
		EvaluateValue("Action", props.Action, context);
		if (context.hold || context.suspended)
		{
			// Execution on hold (i.e. wait action). Stop execution for now
			if (context.hold) context.action_data[sid] = [list, progress];
			return;
		}
	}
}

private func EvalAct_Wait(proplist props, proplist context)
{
	// Wait for specified number of frames
	context.hold = props;
	ScheduleCall(context, UserAction.ResumeAction, props.Time, 1, context, props);
}

private func EvalAct_WaitCondition(proplist props, proplist context)
{
	// Poll condition and resume when it's met.
	var cond = props.Condition;
	// Invalid condition?
	if (!cond) return;
	// Condition fulfilled?
	if (EvaluateValue("Boolean", cond, context)) return;	
	// Re-check condition periodically
	context.hold = props;
	ScheduleCall(context, UserAction.EvalAct_WaitConditionRe, props.Interval, 0x7fffffff, props, context);
}

private func EvalAct_WaitConditionRe(proplist props, proplist context)
{
	if (UserAction->EvaluateValue("Boolean", props.Condition, context))
	{
		ClearScheduleCall(context, UserAction.EvalAct_WaitConditionRe);
		UserAction->ResumeAction(context, props);
	}
}

private func EvalAct_Sound(proplist props, proplist context)
{
	if (!props.Sound) return;
	var sound_context;
	if (props.SourceObject)
	{
		sound_context = EvaluateValue("Object", props.SourceObject, context);
		if (!sound_context) return;
	}
	else
	{
		sound_context = Global;
	}
	var volume = EvaluateValue("Integer", props.Volume, context);
	var pitch = EvaluateValue("Integer", props.Pitch, context);
	if (props.TargetPlayers == "all_players")
	{
		sound_context->Sound(props.Sound, true, volume, nil, props.Loop, nil, pitch);
	}
	else
	{
		for (var plr in EvaluateValue("PlayerList", props.TargetPlayers, context))
		{
			sound_context->Sound(props.Sound, false, volume, plr, props.Loop, nil, pitch);
		}
	}
}

private func EvalAct_CreateObject(proplist props, proplist context)
{
	// Create a new object
	var create_id = EvaluateValue("Definition", props.ID, context);
	if (!create_id) return;
	var owner = EvaluatePlayer(props.Owner, context);
	var container = EvaluateValue("Object", props.Container, context);
	var obj;
	if (container)
	{
		// Contained object
		obj = container->CreateContents(create_id);
		if (obj) obj->SetOwner(owner);
	}
	else
	{
		// Uncontained object
		var position = EvaluatePosition(props.Position, context);
		var speed_x = EvaluateValue("Integer", props.SpeedX, context);
		var speed_y = EvaluateValue("Integer", props.SpeedY, context);
		var rotation = EvaluateValue("Integer", props.Rotation, context);
		var speed_r = EvaluateValue("Integer", props.SpeedR, context);
		if (props.CreateAbove)
			obj = Global->CreateObjectAbove(create_id, position[0], position[1], owner);
		else
			obj = Global->CreateObject(create_id, position[0], position[1], owner);
		// Default speed and rotation
		if (obj) obj->SetXDir(speed_x);
		if (obj) obj->SetYDir(speed_y);
		if (obj) obj->SetR(rotation);
		if (obj) obj->SetRDir(speed_r, 5); // Internal rdir value is in five degrees frame
	}
	// Remember object for later access
	context.last_created_object = obj;
}

private func EvalAct_CastObjects(proplist props, proplist context)
{
	// Cast objects in multiple directions
	var create_id = EvaluateValue("Definition", props.ID, context);
	if (!create_id) return;
	var owner = EvaluatePlayer(props.Owner, context);
	var amount = EvaluateValue("Integer", props.Amount, context);
	var speed = EvaluateValue("Integer", props.Speed, context);
	var mean_angle = EvaluateValue("Integer", props.MeanAngle, context);
	var angle_deviation = EvaluateValue("Integer", props.AngleDeviation, context);
	var position = EvaluatePosition(props.Position, context);
	var objects = CastObjects(create_id, amount, speed, position[0], position[1], mean_angle, angle_deviation);
	for (var obj in objects)
		obj->SetOwner(owner);
	context.last_casted_objects = objects;
}

private func EvalAct_CastParticles(proplist props, proplist context)
{
	var particle_name = props.Name;
	var amount = EvaluateValue("Integer", props.Amount, context);
	var speed = EvaluateValue("Integer", props.Speed, context);
	var size_start = EvaluateValue("Integer", props.Size, context);
	var size_end = EvaluateValue("Integer", props.SizeEnd, context);
	var lifetime = EvaluateValue("Integer", props.Lifetime, context);
	if (lifetime <= 0) return;
	var position = EvaluatePosition(props.Position, context);
	var color = (EvaluateValue("Color", props.Color, context) ?? 0xffffff) | 0xff000000;
	var fadeout = props.FadeOut;
	var gravity = EvaluateValue("Integer", props.Gravity, context);
	var collision_func = props.CollisionFunc;
	var prototype = 
	{
		BlitMode = props.BlitMode,
		Size = PV_Linear(size_start, size_end),
		Rotation = PV_Direction(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		CollisionVertex = 500
	};
	if (fadeout) prototype.Alpha = PV_Linear(255, 0); else prototype.Alpha = 255;
	if (gravity) prototype.ForceY = PV_Gravity(gravity);
	if (collision_func == "pass")
	{
		prototype.CollisionDensity = 9999;
	}
	else if (collision_func == "bounce")
	{
		prototype.OnCollision = PC_Bounce(500);
	}
	else if (collision_func == "die")
	{
		prototype.OnCollision = PC_Die();
	}
	else if (collision_func == "stop")
	{
		prototype.OnCollision = PC_Stop();
	}
	CreateParticle(particle_name, position[0], position[1], PV_Random(-speed, speed), PV_Random(-speed, speed), lifetime, prototype, amount);
}

private func EvalAct_RemoveObject(proplist props, proplist context)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return;
	obj->RemoveObject(props.EjectContents);
}

private func EvalAct_SetPosition(proplist props, proplist context)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return;
	var pos = EvaluatePosition(props.Position, context);
	obj->SetPosition(pos[0], pos[1]);
}

private func EvalAct_Fling(proplist props, proplist context)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return;
	var vx = EvaluateValue("Integer", props.SpeedX, context);
	var vy = EvaluateValue("Integer", props.SpeedY, context);
	var add_speed = EvaluateValue("Boolean", props.AddSpeed, context);
	obj->Fling(vx, vy, 10, add_speed);
}

private func EvalAct_EnterObject(proplist props, proplist context)
{
	var object = EvaluateValue("Object", props.Object, context);
	var container = EvaluateValue("Object", props.Container, context);
	if (!container || !object) return;
	// Enter either with a check (Collect) or just force-Enter
	if (!props.CollectionCheck)
	{
		object->Enter(container);
	}
	else
	{
		if (!container->Collect(object, true))
		{
			if (props.CollectionCheck == "exit")
			{
				object->SetPosition(container->GetX(), container->GetY());
			}
		}
	}
}

private func EvalAct_ExitObject(proplist props, proplist context)
{
	var object = EvaluateValue("Object", props.Object, context);
	if (object) object->Exit();
}

private func EvalAct_SetDirection(proplist props, proplist context)
{
	var object = EvaluateValue("Object", props.Object, context);
	if (object) object->SetDir(props.Direction);
}

private func EvalAct_SetVisibility(proplist props, proplist context)
{
	var object = EvaluateValue("Object", props.Object, context);
	if (object) object.Visibility = props.Visibility;
}

private func EvalAct_SetVincibility(proplist props, proplist context)
{
	var object = EvaluateValue("Object", props.Object, context);
	if (object) object->SetInvincibility(!props.Vincibility);
}

private func EvalAct_DoWealth(proplist props, proplist context)
{
	var player = EvaluatePlayer(props.Player, context);
	var change = EvaluateValue("Integer", props.Change, context);
	if (player != NO_OWNER && change)
	{
		SetWealth(player, GetWealth(player) + change);
		var do_sound = EvaluateValue("Boolean", props.DoSound, context);
		if (do_sound)
		{
			if (change < 0) Sound("UI::Cash*", true, nil, player); else Sound("UI::UnCash*", true, nil, player);
		}
	}
}

private func EvalAct_PlrKnowledge(proplist props, proplist context)
{
	var players = EvaluateValue("PlayerList", props.Players, context) ?? [];
	var def = EvaluateValue("Definition", props.ID, context);
	if (!def) return;
	for (var plr in players) SetPlrKnowledge(plr, def);
}

private func EvalAct_PlrView(proplist props, proplist context)
{
	var players = EvaluateValue("PlayerList", props.Players, context) ?? [];
	var target = EvaluateValue("Object", props.Target, context);
	var immediate = props.Immediate;
	if (!target) return;
	for (var plr in players) SetPlrView(plr, target, immediate);
}

private func EvalAct_ObjectCallInt(proplist props, proplist context, func call_fn)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return;
	var parameter = EvaluateValue("Integer", props.Value, context);
	obj->Call(call_fn, parameter);
}

private func EvalAct_If(proplist props, proplist context)
{
	// Do evaluation on first pass. After that, take context value.
	var sid = props._sequence_id;
	if (!sid) sid = props._sequence_id = Format("%d", ++UserAction_SequenceIDs);
	var cond = context.action_data[sid] ?? !!EvaluateValue("Boolean", props.Condition, context);
	if (cond)
		EvaluateValue("Action", props.TrueEvaluator, context);
	else
		EvaluateValue("Action", props.FalseEvaluator, context);
	// Only keep conditional value within a held action
	if (context.hold) context.action_data[sid] = cond;
}

private func EvalConditionalValue(proplist props, proplist context, eval_type)
{
	// Return value by condition
	if (EvaluateValue("Boolean", props.Condition, context))
		return EvaluateValue(eval_type, props.TrueEvaluator, context);
	else
		return EvaluateValue(eval_type, props.FalseEvaluator, context);
}

private func EvalAct_Log(proplist props, proplist context)
{
	Log(EvaluateString(props.Message, context));
}

private func EvalAct_Nop(proplist props, proplist context) {}

private func GetVariableContext(proplist props, proplist context)
{
	// Resolve context for variable. Global or object context.
	var var_context = EvaluateValue("Object", props, context);
	if (!var_context)
	{
		if (!g_UserAction_global_vars) g_UserAction_global_vars = {};
		var_context = g_UserAction_global_vars;
	}
	else
	{
		if (!var_context.user_variables) var_context.user_variables = {};
		var_context = var_context.user_variables;
	}
	return var_context;
}

private func EvalAct_SetVariable(proplist props, proplist context)
{
	// Assign variable
	var var_context = GetVariableContext(props.Context, context);
	var var_name = StringToIdentifier(EvaluateString(props.VariableName, context));
	var value = EvaluateValue("Any", props.Value, context);
	var_context[var_name] = value;
}

private func EvalVariable(proplist props, proplist context, expected_type)
{
	// Get variable value
	var var_context = GetVariableContext(props.Context, context);
	var var_name = StringToIdentifier(EvaluateString(props.VariableName, context));
	var value = var_context[var_name];
	// Check type (except for C4V_Nil which means "Any" here)
	var val_type = GetType(value);
	if (val_type != expected_type && expected_type)
	{
		// Array types have special checking
		if (GetType(expected_type) == C4V_Array)
		{
			var expected_len = expected_type[1];
			var valid;
			if (val_type == C4V_Array)
			{
				// Check required length
				if (!expected_len || GetLength(value) == expected_len)
				{
					// Check data type of contents
					var subtype = expected_type[0];
					valid = true;
					for (var v in value) if (v && GetType(v) != subtype) valid = false;
				}
			}
			// Invalid value for expected array: Return default array.
			// All nil is OK because there is no string list type and nil converts to 0.
			if (!valid) return CreateArray(expected_len);
		}
		else
		{
			// Type not matching. Construct default.
			if (expected_type == C4V_Int) return 0;
			if (expected_type == C4V_Bool) return !!value; // This conversion is OK
			if (expected_type == C4V_String) return "";
			return nil;
		}
	}
	// Value OK. Return it.
	return value;
}

private func EvalScript(proplist props, proplist context)
{
	var script_context;
	if (props.Context)
	{
		if (!(script_context = EvaluateValue("Object", props.Context, context))) return;
	}
	else
	{
		script_context = Global;
	}
	var script = EvaluateString(props.Script, context);
	return script_context->eval(script, true);
}

private func EvalAct_GameOver(proplist props, proplist context) { GameOver(); }

private func GetDefaultPosition(object target_object)
{
	// Default position for constant absolute position evaluator: Use selected object position
	var value;
	if (target_object)
		value = [target_object->GetX(), target_object->GetY()];
	else
		value = [0, 0];
	return { Function="position_constant", Value = value };
}

private func GetDefaultCoordinates(object target_object)
{
	// Default position for constant absolute position evaluator: Use selected object position
	var value;
	if (target_object)
		value = {X={Function="int_constant", Value = target_object->GetX()}, Y={Function="int_constant", Value = target_object->GetY()}};
	else
		value = {X = 0, Y = 0};
	value.Function="position_coordinates";
	return value;
}

private func GetDefaultRect(object target_object, proplist props)
{
	// Default rectangle around target object
	var r;
	if (target_object) r = [target_object->GetX()-30, target_object->GetY()-30, 60, 60]; else r = [100, 100, 100, 100];
	return { Function = props.Function, Area = r };
}

private func GetDefaultCircle(object target_object, proplist props)
{
	// Default circle around target object
	var r;
	if (target_object) r = [50, target_object->GetX(), target_object->GetY()]; else r = [50, 100, 100];
	return { Function = props.Function, Area = r };
}

private func GetDefaultSearchRect(object target_object)
{
	// Default rectangle around target object
	var r;
	if (target_object) r = [target_object->GetX()-30, target_object->GetY()-30, 60, 60]; else r = [100, 100, 100, 100];
	return { Function="Rect", Area = r };
}

private func GetDefaultSearchCircle(object target_object)
{
	// Default circle around target object
	var r;
	if (target_object) r = [50, target_object->GetX(), target_object->GetY()]; else r = [50, 100, 100];
	return { Function="Circle", Area = r };
}

private func EvalIntRandom(proplist props, proplist context)
{
	// Random value between min and max. Also allow them to be swapped.
	var min = EvaluateValue("Integer", props.Min, context);
	var max = EvaluateValue("Integer", props.Max, context);
	var rmin = Min(min, max);
	return Random(Max(max, min)-rmin) + rmin;
}

private func EvalPositionRelative(proplist props, proplist context)
{
	// Return position relative to action_object
	if (context.action_object)
		return [props.Value[0] + context.action_object->GetX(), props.Value[1] + context.action_object->GetY()];
	else
		return props.Value;
}

private func EvalCoordinates(proplist props, proplist context)
{
	// Coordinate evaluation: Make array [X, Y]
	return [EvaluateValue("Integer", props.X, context), EvaluateValue("Integer", props.Y, context)];
}

private func EvalPositionOffset(proplist props, proplist context)
{
	var p = EvaluatePosition(props.Position, context);
	var o = EvaluateOffset(props.Offset, context);
	return [p[0]+o[0], p[1]+o[1]];
}

private func EvalPositionObject(proplist props, proplist context)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (obj) return [obj->GetX(), obj->GetY()];
	return [0, 0]; // undefined object: Position is 0/0 default
}

private func EvalPos_LastUse(proplist props, proplist context) { return context.position; }

private func EvalPos_RandomRect(proplist props, proplist context, bool relative)
{
	// Constant random distribution in rectangle
	var a = props.Area;
	var rval = [a[0] + Random(a[2]), a[1] + Random(a[3])];
	// Apply relative offset
	if (relative && context.action_object)
	{
		rval[0] += context.action_object->GetX(); 
		rval[1] += context.action_object->GetY(); 
	}
	return rval;
}

private func EvalPos_RandomCircle(proplist props, proplist context, bool relative)
{
	// Constant random distribution in circle
	var a = props.Area;
	var r = a[0];
	r = Sqrt(Random(r*r));
	var ang = Random(360);
	var x = Sin(ang, r), y = Cos(ang, r);
	var rval = [a[1]+x, a[2]+y];
	// Apply relative offset
	if (relative && context.action_object)
	{
		rval[0] += context.action_object->GetX(); 
		rval[1] += context.action_object->GetY(); 
	}
	return rval;
}

private func EvalOffsetAdd(proplist props, proplist context)
{
	var o1 = EvaluateOffset(props.Offset1, context);
	var o2 = EvaluateOffset(props.Offset2, context);
	return [o1[0]+o2[0], o1[1]+o2[1]];
}

private func EvalOffsetDiff(proplist props, proplist context)
{
	var pA = EvaluatePosition(props.PositionA, context);
	var pB = EvaluatePosition(props.PositionB, context);
	return [pB[0]-pA[0], pB[1]-pA[1]];
}

private func EvalInt_Add(proplist props, proplist context) { return EvaluateValue("Integer", props.LeftOperand, context) + EvaluateValue("Integer", props.RightOperand, context); }
private func EvalInt_Sub(proplist props, proplist context) { return EvaluateValue("Integer", props.LeftOperand, context) - EvaluateValue("Integer", props.RightOperand, context); }
private func EvalInt_Mul(proplist props, proplist context) { return EvaluateValue("Integer", props.LeftOperand, context) * EvaluateValue("Integer", props.RightOperand, context); }

private func EvalInt_Div(proplist props, proplist context)
{
	var a = EvaluateValue("Integer", props.LeftOperand, context), b = EvaluateValue("Integer", props.RightOperand, context);
	return a/(b+!b);
}

private func EvalInt_Mod(proplist props, proplist context)
{
	var a = EvaluateValue("Integer", props.LeftOperand, context), b = EvaluateValue("Integer", props.RightOperand, context);
	return a%(b+!b);
}

private func EvalInt_Distance(proplist props, proplist context)
{
	var pA = EvaluatePosition(props.PositionA, context);
	var pB = EvaluatePosition(props.PositionB, context);
	return Distance(pA[0], pA[1], pB[0], pB[1]);
}

private func EvalInt_Wealth(proplist props, proplist context) { return GetWealth(EvaluatePlayer(props.Player, context)); }

private func EvalInt_PosCoord(proplist props, proplist context, int idx) { return EvaluatePosition(props.Position, context)[idx]; }

private func EvalClr_PlayerColor(proplist props, proplist context) { return GetPlayerColor(EvaluatePlayer(props.Player, context)); }

private func EvalClr_RGB(proplist props, proplist context)
{
	var R = EvaluateValue("Integer", props.R, context);
	var G = EvaluateValue("Integer", props.G, context);
	var B = EvaluateValue("Integer", props.B, context);
	return RGB(R, G, B);
}

private func EvalClr_Random(proplist props, proplist context)
{
	var a = EvaluateValue("Color", props.ColorA, context);
	var b = EvaluateValue("Color", props.ColorB, context);
	var result;
	for (var i = 0; i<3; ++i)
	{
		var ca = (a>>(i*8)) & 0xff;
		var cb = (b>>(i*8)) & 0xff;
		result |= ((Random(Abs(ca-cb + 1)) + Min(ca, cb)) << (i*8));
	}
	return result;
}

private func EvalStr_ValueToString(proplist props, proplist context)
{
	return Format("%v", EvaluateValue("Any", props.Value, context));
}

private func EvalStr_Concat(proplist props, proplist context)
{
	var result="";
	for (var s in props.Substrings) result = Format("%s%s", result, EvaluateString(s, context));
	return result;
}

private func EvalObjProp(proplist props, proplist context, prop_fn)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return nil;
	return obj->Call(prop_fn);
}



/* Context instance */

// Proplist holding progress in each sequence
local action_data, sequence_had_goto;
static UserAction_SequenceIDs;

// Set to innermost sequence (for goto)
local last_sequence;

// If this action is paused and will be resumed by a callback or by re-execution of the action, this property is set to the props of the holding action
local hold;

// Set to true if action is on hold but waiting for re-execution
local suspended;

// Return value if a value-providing evaluator is held
local hold_result;

public func Initialize()
{
	action_data = {};
	sequence_had_goto = {};
	return true;
}

public func InitContext(object action_object, int triggering_player, object triggering_object, proplist props, finish_callback, position)
{
	// Determine triggering player + objects
	var triggering_clonk;
	// Triggering player unknown? Try fallback to the controller of the triggering object
	if (!GetType(triggering_player) && triggering_object)
	{
		triggering_player = triggering_object->GetController();
	}
	// Triggering clonk is the selected clonk of the triggering player
	if (GetType(triggering_player))
	{
		triggering_clonk = GetCursor(triggering_player);;
		if (!triggering_clonk) triggering_clonk = GetCrew(triggering_player);
	}
	// Triggering object / Triggering player clonk fallbacks
	if (!triggering_object)
		triggering_object = triggering_clonk;
	else if (triggering_object->~IsClonk())
		triggering_clonk = triggering_object;
	// Position default
	if (!position && triggering_object)
		position = [triggering_object->GetX(), triggering_object->GetY()];
	// Init context settings
	this.action_object = action_object;
	this.triggering_object = triggering_object;
	this.triggering_clonk = triggering_clonk;
	this.triggering_player = triggering_player;
	this.position = position;
	this.root_action = props;
	this.suspended = false;
	this.finish_callback = finish_callback;
	return true;
}

public func MenuOK(proplist menu_id, object clonk)
{
	// Pressed 'Next' in dialogue: Resume in user action
	UserAction->ResumeAction(this, this.hold);
}

public func MenuSelectOption(int index)
{
	// Selected an option in dialogue: Resume at specified position in innermost sequence
	if (!hold || !hold.Options) return;
	var opt = this.hold.Options[index];
	if (opt && last_sequence)
	{
		action_data[last_sequence._sequence_id] = opt._goto;
		hold = nil;
	}
	UserAction->ResumeAction(this, hold);
}

public func SaveScenarioObject(props) { return false; } // temp. don't save.
