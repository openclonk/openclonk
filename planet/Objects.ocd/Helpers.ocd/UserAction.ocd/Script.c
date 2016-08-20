/* User action execution handler */
// Handles actions set in editor e.g. for dialogues, switches, etc.
// An object is sometimes needed to show a menu or start a timer, so this definition is created whenever a user action is run

local Name = "UserAction";
local Plane=0;

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
local DefinitionPriority=99;

// Localized group names
local GroupNames = { Structure="$Structure$", Game="$Game$" };

// Storage for global user variables
static g_UserAction_global_vars;

// Localized evaluator names
local EvaluatorTypeNames = {
	Action = "$UserAction$",
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
local EvaluatorTypes = ["Action", "Object", "ObjectList", "Definition", "Player", "PlayerList", "Boolean", "Integer", "String", "Position", "Offset", "Any"];

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

func Definition(def)
{
	// Typed evaluator base definitions
	Evaluator = {};
	Evaluator.Action = { Name="$UserAction$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Object = { Name="$UserObject$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.ObjectList = { Name="$UserObjectList$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Definition = { Name="$UserDefinition$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Player = { Name="$UserPlayer$", Type="enum", OptionKey="Function", Options = [ { Name="$Noone$" } ] };
	Evaluator.PlayerList = { Name="$UserPlayerList$", Type="enum", OptionKey="Function", Options = [ { Name="$Noone$" } ] };
	Evaluator.Boolean = { Name="$UserBoolean$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Integer = { Name="$UserInteger$", Type="enum", OptionKey="Function", Options = [ {Name="0"} ] };
	Evaluator.String = { Name="$UserString$", Type="enum", OptionKey="Function", Options = [ {Name="($EmptyString$)"} ] };
	Evaluator.Position = { Name="$UserPosition$", Type="enum", OptionKey="Function", Options = [ { Name="$Here$" } ] };
	Evaluator.Offset = { Name="$UserOffset$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Any = { Name="$UserAny$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	// Action evaluators
	EvaluatorCallbacks = {};
	EvaluatorDefs = {};
	AddEvaluator("Action", "$Sequence$", "$Sequence$", "$SequenceHelp$", "sequence", [def, def.EvalAct_Sequence], { Actions=[] }, { Type="proplist", DescendPath="Actions", Display="{{Actions}}", EditorProps = {
		Actions = { Name="$Actions$", Type="array", Elements=Evaluator.Action },
		} } );
	AddEvaluator("Action", "$Sequence$", "$Goto$", "$GotoHelp$", "goto", [def, def.EvalAct_Goto], { Index=0 }, { Type="proplist", Display="{{Index}}", EditorProps = {
		Index = { Name="$Index$", Type="int", Min=0 }
		} } );
	AddEvaluator("Action", "$Sequence$", "$StopSequence$", "$StopSequenceHelp$", "stop_sequence", [def, def.EvalAct_StopSequence]);
	AddEvaluator("Action", "$Sequence$", "$SuspendSequence$", "$SuspendSequenceHelp$", "suspend_sequence", [def, def.EvalAct_SuspendSequence]);
	AddEvaluator("Action", "$Sequence$", "$Wait$", "$WaitHelp$", "wait", [def, def.EvalAct_Wait], { Time=60 }, { Type="proplist", Display="{{Time}}", EditorProps = {
		Time = { Name="$Time$", Type="int", Min=1 }
		} } );
	AddEvaluator("Action", "$Ambience$", "$Sound$", "$SoundHelp$", "sound", [def, def.EvalAct_Sound], { Pitch={Function="int_constant", Value=0}, Volume={Function="int_constant", Value=100}, TargetPlayers={Function="all_players"} }, { Type="proplist", Display="{{Sound}}", EditorProps = {
		Sound = { Name="$SoundName$", EditorHelp="$SoundNameHelp$", Type="sound", AllowEditing=true, Priority=100 },
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
	AddEvaluator("Action", "$Object$", "$CreateObject$", "$CreateObjectHelp$", "create_object", [def, def.EvalAct_CreateObject], { SpeedX={Function="int_constant", Value=0},SpeedY={Function="int_constant", Value=0} }, { Type="proplist", Display="{{ID}}", ShowFullName=true, EditorProps = {
		ID = new Evaluator.Definition { EditorHelp="$CreateObjectDefinitionHelp$", Priority=100 },
		Position = new Evaluator.Position { EditorHelp="$CreateObjectPositionHelp$" },
		CreateAbove = { Name="$CreateObjectCreationOffset$", EditorHelp="$CreateObjectCreationOffsetHelp$", Type="enum", Options=[
			{ Name="$Center$" },
			{ Name="$Bottom$", Value=true }
			]},
		Owner = new Evaluator.Player { Name="$Owner$", EditorHelp="$CreateObjectOwnerHelp$" },
		Container = new Evaluator.Object { Name="$Container$", EditorHelp="$CreateObjectContainerHelp$" },
		SpeedX = new Evaluator.Integer { Name="$SpeedX$", EditorHelp="$CreateObjectSpeedXHelp$" },
		SpeedY = new Evaluator.Integer { Name="$SpeedY$", EditorHelp="$CreateObjectSpeedYHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$CastObjects$", "$CastObjectsHelp$", "cast_objects", [def, def.EvalAct_CastObjects], { Amount={Function="int_constant", Value=8},Speed={Function="int_constant", Value=20},AngleDeviation={Function="int_constant", Value=360} }, { Type="proplist", Display="{{Amount}}x{{ID}}", ShowFullName=true, EditorProps = {
		ID = new Evaluator.Definition { EditorHelp="$CastObjectsDefinitionHelp$", Priority=100 },
		Position = new Evaluator.Position { EditorHelp="$CastObjectsPositionHelp$" },
		Amount = new Evaluator.Integer { Name="$Amount$", EditorHelp="$CastObjectsAmountHelp$" },
		Speed = new Evaluator.Integer { Name="$Speed$", EditorHelp="$CastObjectsSpeedHelp$" },
		MeanAngle = new Evaluator.Integer { Name="$MeanAngle$", EditorHelp="$CastObjectsMeanAngleHelp$" },
		AngleDeviation = new Evaluator.Integer { Name="$AngleDeviation$", EditorHelp="$CastObjectsAngleDeviationHelp$" },
		Owner = new Evaluator.Player { Name="$Owner$", EditorHelp="$CastObjectsOwnerHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$RemoveObject$", "$RemoveObjectHelp$", "remove_object", [def, def.EvalAct_RemoveObject], { }, { Type="proplist", Display="{{Object}}", ShowFullName=true, EditorProps = {
		Object = new Evaluator.Object { EditorHelp="$RemoveObjectObject$", Priority=100 },
		EjectContents = { Name="$EjectContents$", EditorHelp="$EjectContentsHelp$", Type="enum", Options=[
			{ Name="$EjectContentsNo$" },
			{ Name="$EjectContentsYes$", Value=true }
			] },
		} } );
	AddEvaluator("Action", "$Object$", "$SetPosition$", "$SetPositionHelp$", "set_position", [def, def.EvalAct_SetPosition], { Object={ Function="triggering_clonk" }, Position={ Function="position_constant_rel" } }, { Type="proplist", Display="({{Object}}->{{Position}})", ShowFullName=true, EditorProps = {
		Object = new Evaluator.Object { Name="$Object$", EditorHelp="$SetPositionObjectHelp$" },
		Position = new Evaluator.Position { Name="$Position$", EditorHelp="$SetPositionPositionHelp$" }
		} } );
	AddEvaluator("Action", "Clonk", "$DoEnergy$", "$DoEnergyHelp$", "do_energy", [def, def.EvalAct_ObjectCallInt, Global.DoEnergy], { Object={ Function="triggering_clonk" } }, { Type="proplist", Display="({{Object}}, {{Value}})", ShowFullName=true, EditorProps = {
		Object = new Evaluator.Object { Name="$Object$", EditorHelp="$DoEnergyObjectHelp$" },
		Value = new Evaluator.Integer { Name="$ValueChange$", EditorHelp="$DoEnergyValueChangeHelp$" }
		} } );
	AddEvaluator("Action", "$Script$", "$ConditionalAction$", "$ConditionalActionHelp$", "if", [def, def.EvalAct_If, "Action"], { }, { Type="proplist", Display="if({{Condition}}) {{Action}} else {{ElseAction}}", EditorProps = {
		Condition = new Evaluator.Boolean { Name="$Condition$", EditorHelp="$IfConditionHelp$", Priority=60 },
		TrueEvaluator = new Evaluator.Action { Name="$TrueEvaluator$", EditorHelp="$TrueEvaluatorHelp$", Priority=50 },
		FalseEvaluator = new Evaluator.Action { Name="$FalseEvaluator$", EditorHelp="$FalseEvaluatorHelp$", Priority=30 }
		} } );
	AddEvaluator("Action", "$Script$", "$SetVariable$", "$SetVariableHelp$", "set_variable", [def, def.EvalAct_SetVariable], { }, { Type="proplist", Display="{{Context}}::{{VariableName}}={{Value}}", EditorProps = {
		Context = new Evaluator.Object { Name="$Context$", EditorHelp="$VariableContextHelp$", EmptyName="$Global$" },
		VariableName = new Evaluator.String { Name="$VariableName$", EditorHelp="$VariableNameHelp$" },
		Value = new Evaluator.Any { Name="$Value$", EditorHelp="$SetVariableValueHelp$" }
		} } );
	AddEvaluator("Action", "$Script$", "$Log$", "$LogHelp$", "log", [def, def.EvalAct_Log], { }, { Type="proplist", Display="{{Message}}", ShowFullName=true, EditorProps = {
		Message = new Evaluator.String { Name="$LogMessage$", EditorHelp="$LogMessageHelp$" },
		} } );
	// Object evaluators
	AddEvaluator("Object", nil, "$ActionObject$", "$ActionObjectHelp$", "action_object", [def, def.EvalObj_ActionObject]);
	AddEvaluator("Object", nil, "$TriggerClonk$", "$TriggerClonkHelp$", "triggering_clonk", [def, def.EvalObj_TriggeringClonk]);
	AddEvaluator("Object", nil, "$TriggerObject$", "$TriggerObjectHelp$", "triggering_object", [def, def.EvalObj_TriggeringObject]);
	AddEvaluator("Object", nil, ["$ConstantObject$", ""], "$ConstantObjectHelp$", "object_constant", [def, def.EvalConstant], { Value=nil }, { Type="object", Name="$Value$" });
	AddEvaluator("Object", nil, "$LastCreatedObject$", "$LastCreatedObjectHelp$", "last_created_object", [def, def.EvalObj_LastCreatedObject]);
	var find_object_in_area_delegate = { Type="proplist", Display="{{ID}}", ShowFullName=true, EditorProps = {
		ID = new Evaluator.Definition { Name="$ID$", EditorHelp="$FindObjectsIDHelp$", EmptyName="$Any$", Priority=51 },
		Area = { Name="$SearchArea$", EditorHelp="$SearchAreaHelp$", Type="enum", OptionKey="Function", Priority=41, Options=[
			{ Name="$SearchAreaWholeMap$", EditorHelp="$SearchAreaWholeMapHelp$" },
			{ Name="$SearchAreaInRect$", EditorHelp="$SearchAreaInRectHelp$", Value={ Function="InRect" }, Get=def.GetDefaultRect, ValueKey="Area", Delegate={ Type="rect", Name="$Rectangle$", Relative=false, Color=0xffff00 } },
			{ Name="$SearchAreaAtRect$", EditorHelp="$SearchAreaAtRectHelp$", Value={ Function="AtRect" }, Get=def.GetDefaultRect, ValueKey="Area", Delegate={ Type="rect", Name="$Rectangle$", Relative=false, Color=0xffff80 } },
			{ Name="$SearchAreaCircle$", EditorHelp="$SearchAreaCircleHelp$", Value={ Function="Circle" }, Get=def.GetDefaultCircle, ValueKey="Area", Delegate={ Type="circle", Name="$Circle$", Relative=false, CanMoveCenter=true, Color=0xff00ff } },
			{ Name="$SearchAreaNearPosition$", EditorHelp="$SearchAreaNearPositionHelp$", Value={ Function="NearPosition", Parameters={Radius=25} }, ValueKey="Parameters", Delegate={ Type="proplist", Display="{{Object}}", ShowFullName=true, EditorProps = {
				Position = new Evaluator.Position { EditorHelp="$SearchAreaNearPositionPositionHelp$"},
				Radius = { Type="circle", Relative=true, Name="$Radius$", Color=0xff80ff }
				} } }
			] },
		AllowContained = { Name="$AllowContained$", EditorHelp="$AllowContainedHelp$", Type="bool", Priority=31 }
		} };
	var find_object_in_container_delegate = { Type="proplist", Display="{{ID}} in {{Container}}", ShowFullName=true, EditorProps = {
		ID = new Evaluator.Definition { Name="$ID$", EditorHelp="$FindObjectsIDHelp$", EmptyName="$Any$" },
		Container = new Evaluator.Object { Name="$Container$", EditorHelp="FindObjectsContainerHelp" }
		} };
	AddEvaluator("Object", nil, "$FindObjectInArea$", "$FindObjectInAreaHelp$", "find_object_in_area", [def, def.EvalObjList_FindObjectsInArea, true], {}, find_object_in_area_delegate);
	AddEvaluator("Object", nil, "$FindObjectInContainer$", "$FindObjectInContainerHelp$", "find_object_in_container", [def, def.EvalObjList_FindObjectInContainer], {}, find_object_in_container_delegate);
	// Object list evaluators
	AddEvaluator("ObjectList", nil, "$FindObjectsInArea$", "$FindObjectsInAreaHelp$", "find_objects_in_area", [def, def.EvalObjList_FindObjectsInArea], {}, find_object_in_area_delegate);
	AddEvaluator("ObjectList", nil, "$FindObjectsInContainer$", "$FindObjectsInContainerHelp$", "find_objects_in_container", [def, def.EvalObjList_FindObjectsInContainer], {}, find_object_in_container_delegate);
	// Definition evaluators
	AddEvaluator("Definition", nil, ["$Constant$", ""], "$ConstantHelp$", "def_constant", [def, def.EvalConstant], { Value=nil }, { Type="def", Name="$Value$" });
	AddEvaluator("Definition", nil, "$TypeOfObject$", "$TypeOfObjectHelp$", "type_of_object", [def, def.EvalObjProp, Global.GetID], { }, new Evaluator.Object { }, "Object");
	// Player evaluators
	AddEvaluator("Player", nil, "$TriggeringPlayer$", "$TriggeringPlayerHelp$", "triggering_player", [def, def.EvalPlr_Trigger]);
	AddEvaluator("Player", nil, "$OwnerOfObject$", "$OwnerOfObjectHelp$", "owner", [def, def.EvalObjProp, Global.GetOwner], { }, new Evaluator.Object { }, "Object");
	AddEvaluator("Player", nil, "$ControllerOfObject$", "$ControllerOfObjectHelp$", "owner", [def, def.EvalObjProp, Global.GetController], { }, new Evaluator.Object { }, "Object");
	// Player list evaluators
	AddEvaluator("PlayerList", nil, "$TriggeringPlayer$", "$TriggeringPlayerHelp$", "triggering_player_list", [def, def.EvalPlrList_Single, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$AllPlayers$", "$AllPlayersHelp$", "all_players", [def, def.EvalPlrList_All]);
	// Boolean (condition) evaluators
	AddEvaluator("Boolean", nil, ["$Constant$", ""], "$ConstantHelp$", "bool_constant", [def, def.EvalConstant], { Value=true }, { Type="bool", Name="$Value$" });
	AddEvaluator("Boolean", "$Comparison$", "$CompareInteger$", "$ComparisonHelp$", "compare_int", [def, def.EvalComparison, "Integer"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", ShowFullName=true, EditorProps = {
		LeftOperand = new Evaluator.Integer { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority=44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority=43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			{ Name="<", EditorHelp="$LessThanHelp$", Value="lt" },
			{ Name=">", EditorHelp="$GreaterThanHelp$", Value="gt" },
			{ Name="<=", EditorHelp="$LessOrEqualHelp$", Value="le" },
			{ Name=">=", EditorHelp="$GreaterOrEqualHelp$", Value="ge" }
			] },
		RightOperand = new Evaluator.Integer { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority=42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareBoolean$", "$ComparisonHelp$", "compare_bool", [def, def.EvalComparison, "Boolean"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", ShowFullName=true, EditorProps = {
		LeftOperand = new Evaluator.Object { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority=44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority=43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Object { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority=42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareObject$", "$ComparisonHelp$", "compare_object", [def, def.EvalComparison, "Object"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", ShowFullName=true, EditorProps = {
		LeftOperand = new Evaluator.Object { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority=44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority=43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Object { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority=42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareString$", "$ComparisonHelp$", "compare_string", [def, def.EvalComparison, "String"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", ShowFullName=true, EditorProps = {
		LeftOperand = new Evaluator.String { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority=44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority=43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.String { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority=42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$CompareDefinition$", "$ComparisonHelp$", "compare_definition", [def, def.EvalComparison, "Definition"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", ShowFullName=true, EditorProps = {
		LeftOperand = new Evaluator.Definition { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority=44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority=43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Definition { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority=42 }
		} } );
	AddEvaluator("Boolean", "$Comparison$", "$ComparePlayer$", "$ComparisonHelp$", "compare_player", [def, def.EvalComparison, "Player"], { }, { Type="proplist", Display="{{LeftOperand}}{{Operator}}{{RightOperand}}", ShowFullName=true, EditorProps = {
		LeftOperand = new Evaluator.Player { Name="$LeftOperand$", EditorHelp="$LeftOperandHelp$", Priority=44 },
		Operator = { Type="enum", Name="$Operator$", EditorHelp="$OperatorHelp$", Priority=43, Options = [
			{ Name="==", EditorHelp="$EqualHelp$" },
			{ Name="!=", EditorHelp="$NotEqualHelp$", Value="ne" },
			] },
		RightOperand = new Evaluator.Player { Name="$RightOperand$", EditorHelp="$RightOperandHelp$", Priority=42 }
		} } );
	AddEvaluator("Boolean", "$Logic$", "$Not$", "$NotHelp$", "not", [def, def.EvalBool_Not], { }, new Evaluator.Boolean { }, "Operand");
	AddEvaluator("Boolean", "$Logic$", "$And$", "$AndHelp$", "and", [def, def.EvalBool_And], { Operands=[] }, { Type="proplist", DescendPath="Operands", Display="{{Operands}}", EditorProps = {
		Operands = { Name="$Operands$", Type="array", Elements=Evaluator.Boolean }
		} } );
	AddEvaluator("Boolean", "$Logic$", "$Or$", "$OrHelp$", "or", [def, def.EvalBool_Or], { Operands=[] }, { Type="proplist", DescendPath="Operands", Display="{{Operands}}", EditorProps = {
		Operands = { Name="$Operands$", Type="array", Elements=Evaluator.Boolean }
		} } );
	AddEvaluator("Boolean", nil, "$ObjectExists$", "$ObjectExistsHelp$", "object_exists", [def, def.EvalBool_ObjectExists], { }, new Evaluator.Object { }, "Object");
	// Integer evaluators
	AddEvaluator("Integer", nil, ["$Constant$", ""], "$ConstantHelp$", "int_constant", [def, def.EvalConstant], { Value=0 }, { Type="int", Name="$Value$" });
	var arithmetic_delegate = { Type="proplist", EditorProps = {
		LeftOperand = new Evaluator.Integer { Name="$LeftOperand$", EditorHelp="$LeftArithmeticOperandHelp$", Priority=44 },
		RightOperand = new Evaluator.Integer { Name="$RightOperand$", EditorHelp="$RightArithmeticOperandHelp$", Priority=42 }
		} };
	AddEvaluator("Integer", "$Arithmetic$", "$Sum$ (+)", "$SumHelp$", "add", [def, def.EvalInt_Add], { }, new arithmetic_delegate { Display="{{LeftOperand}}+{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Sub$ (-)", "$SumHelp$", "subtract", [def, def.EvalInt_Sub], { }, new arithmetic_delegate { Display="{{LeftOperand}}-{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Mul$ (*)", "$MulHelp$", "multiply", [def, def.EvalInt_Mul], { }, new arithmetic_delegate { Display="{{LeftOperand}}*{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Div$ (/)", "$DivHelp$", "divide", [def, def.EvalInt_Div], { }, new arithmetic_delegate { Display="{{LeftOperand}}/{{RightOperand}}" });
	AddEvaluator("Integer", "$Arithmetic$", "$Mod$ (%)", "$ModHelp$", "modulo", [def, def.EvalInt_Mod], { }, new arithmetic_delegate { Display="{{LeftOperand}}%{{RightOperand}}" });
	AddEvaluator("Integer", nil, "$Random$", "$RandomIntHelp$", "int_random", [def, def.EvalIntRandom], { Min={Function="int_constant", Value=0}, Max={Function="int_constant", Value=99} }, { Type="proplist", Display="Rnd({{Min}}..{{Max}})", EditorProps = {
		Min = new Evaluator.Integer { Name="$Min$", EditorHelp="$RandomMinHelp$", Priority=51 },
		Max = new Evaluator.Integer { Name="$Max$", EditorHelp="$RandomMaxHelp$", Priority=21 }
		} } );
	AddEvaluator("Integer", nil, "$Distance$", "$DistanceHelp$", "distance", [def, def.EvalInt_Distance], { }, { Type="proplist", Display="d({{PositionA}}..{{PositionB}})", EditorProps = {
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
	// String evaluators
	AddEvaluator("String", nil, ["$Constant$", ""], "$ConstantHelp$", "string_constant", [def, def.EvalConstant], { Value="" }, { Type="string", Name="$Value$" });
	AddEvaluator("String", nil, ["$ValueToString$", ""], "$ValueToStringHelp$", "value_to_string", [def, def.EvalStr_ValueToString], { }, new Evaluator.Any { });
	AddEvaluator("String", nil, "$Concat$", "$ConcatHelp$", "string_concat", [def, def.EvalStr_Concat], { Substrings=[] }, { Type="proplist", DescendPath="Substrings", Display="{{Substrings}}", EditorProps = {
		Substrings = { Name="$Substrings$", Type="array", Elements=Evaluator.String }
		} } );
	// Position evaluators
	AddEvaluator("Position", nil, ["$ConstantPositionAbsolute$", ""], "$ConstantPositionAbsoluteHelp$", "position_constant", [def, def.EvalConstant], def.GetDefaultPosition, { Type="point", Name="$Position$", Relative=false, Color=0xff2000 });
	AddEvaluator("Position", nil, ["$ConstantPositionRelative$", "+"], "$ConstantPositionRelativeHelp$", "position_constant_rel", [def, def.EvalPositionRelative], { Value=[0,0] }, { Type="point", Name="$Position$", Relative=true, Color=0xff0050 });
	AddEvaluator("Position", nil, "$Coordinates$", "$CoordinatesHelp$", "position_coordinates", [def, def.EvalCoordinates], def.GetDefaultCoordinates, { Type="proplist", Display="({{X}},{{Y}})", EditorProps = {
		X = new Evaluator.Integer { Name="X", EditorHelp="$PosXHelp$" },
		Y = new Evaluator.Integer { Name="Y", EditorHelp="$PosYHelp$" }
		} } );
	AddEvaluator("Position", nil, "$PositionOffset$", "$PositionOffsetHelp$", "position_offset", [def, def.EvalPositionOffset], { }, { Type="proplist", Display="{{Position}}+{{Offset}}", EditorProps = {
		Position = new Evaluator.Position { EditorHelp="$PositionOffsetPositionHelp$", Priority=51 },
		Offset = new Evaluator.Offset { EditorHelp="$PositionOffsetOffsetHelp$", Priority=21 }
		} } );
	AddEvaluator("Position", nil, "$ObjectPosition$", "$ObjectPositionHelp$", "object_position", [def, def.EvalPositionObject], { Object={Function="triggering_object"} }, new Evaluator.Object { EditorHelp="$ObjectPositionObjectHelp$" }, "Object");
	AddEvaluator("Position", "$RandomPosition$", "$RandomRectAbs$", "$RandomRectAbsHelp$", "random_pos_rect_abs", [def, def.EvalPos_RandomRect, false], def.GetDefaultRect, { Type="rect", Name="$Rectangle$", Relative=false, Color=0xffff00 }, "Area");
	AddEvaluator("Position", "$RandomPosition$", "$RandomRectRel$", "$RandomRectRelHelp$", "random_pos_rect_rel", [def, def.EvalPos_RandomRect, true], { Area=[-30,-30,60,60] }, { Type="rect", Name="$Rectangle$", Relative=true, Color=0x00ffff }, "Area");
	AddEvaluator("Position", "$RandomPosition$", "$RandomCircleAbs$", "$RandomCircleAbsHelp$", "random_pos_circle_abs", [def, def.EvalPos_RandomCircle, false], def.GetDefaultCircle, { Type="circle", Name="$Circle$", Relative=false, CanMoveCenter=true, Color=0xff00ff }, "Area");
	AddEvaluator("Position", "$RandomPosition$", "$RandomCircleRel$", "$RandomCircleRelHelp$", "random_pos_circle_rel", [def, def.EvalPos_RandomCircle, true], { Area=[50,0,0] }, { Type="circle", Name="$Circle$", Relative=true, CanMoveCenter=true, Color=0xa000a0 }, "Area");
	// Offset evaluators
	AddEvaluator("Offset", nil, ["$ConstantOffsetRelative$", ""], "$ConstantOffsetRelativeHelp$", "offset_constant", [def, def.EvalConstant], { Value=[0,0] }, { Type="point", Name="$Position$", Relative=true, Color=0xff30ff });
	AddEvaluator("Offset", nil, ["$Coordinates$", ""], "$CoordinatesHelp$", "offset_coordinates", [def, def.EvalCoordinates], { Value={X=0,Y=0} }, { Type="proplist", Display="({{X}},{{Y}})", EditorProps = {
		X = new Evaluator.Integer { Name="X", EditorHelp="$OffXHelp$" },
		Y = new Evaluator.Integer { Name="Y", EditorHelp="$OffYHelp$" },
		} } );
	AddEvaluator("Offset", nil, "$AddOffsets$", "$AddOffsetsHelp$", "add_offsets", [def, def.EvalOffsetAdd], { }, { Type="proplist", Display="{{Offset1}}+{{Offset2}}", EditorProps = {
		Offset1 = new Evaluator.Offset { EditorHelp="$AddOffsetOffsetHelp$" },
		Offset2 = new Evaluator.Offset { EditorHelp="$AddOffsetOffsetHelp$" }
		} } );
	AddEvaluator("Offset", nil, "$DiffPositions$", "$DiffPositionsHelp$", "diff_positions", [def, def.EvalOffsetDiff], { }, { Type="proplist", Display="{{PositionB}}-{{PositionA}}", EditorProps = {
		PositionA = new Evaluator.Position { Name="$PositionA$", EditorHelp="$PositionAHelp$" },
		PositionB = new Evaluator.Position { Name="$PositionB$", EditorHelp="$PositionBHelp$" }
		} } );
	AddEvaluator("Offset", nil, "$RandomOffRectRel$", "$RandomRectRelHelp$", "random_off_rect_rel", [def, def.EvalPos_RandomRect, "rect", false], { Area=[-30,-30,60,60] }, { Type="rect", Name="$Rectangle$", Relative=true, Color=0x00ffff }, "Area");
	AddEvaluator("Offset", nil, "$RandomOffCircleRel$", "$RandomCircleRelHelp$", "random_off_circle_rel", [def, def.EvalPos_RandomCircle, "circle", false], { Area=[0,0,50] }, { Type="circle", Name="$Circle$", Relative=true, CanMoveCenter=true, Color=0xa000a0 }, "Area");
	// Script evaluators
	var variable_delegate = { Type="proplist", Display="{{Context}}::{{VariableName}}", EditorProps = {
		Context = new Evaluator.Object { Name="$Context$", EditorHelp="$VariableContextHelp$", EmptyName="$Global$" },
		VariableName = new Evaluator.String { Name="$VariableName$", EditorHelp="$VariableNameHelp$", Priority=50 } } };
	for (var eval_type in EvaluatorTypes)
	{
		var data_type = EvaluatorReturnTypes[eval_type];
		var group = nil;
		if (eval_type != "Action")
		{
			AddEvaluator(eval_type, nil, "$Variable$", "$VariableHelp$", Format("%s_variable", eval_type), [def, def.EvalVariable, data_type], { }, variable_delegate);
		}
		else
		{
			group = "$Script$";
		}
		AddEvaluator(eval_type, group, "$Script$", "$ScriptHelp$", Format("%s_script", eval_type), [def, def.EvalScript, data_type], { }, { Type="proplist", Display="{{Context}}::{{Script}}", EditorProps = {
			Context = new Evaluator.Object { Name="$Context$", EditorHelp="$VariableContextHelp$", EmptyName="$Global$" },
			Script = new Evaluator.String { Name="$ScriptCommand$", EditorHelp="$ScriptCommandHelp$" } } });
	}
	// User action editor props
	Prop = Evaluator.Action;
	PropProgressMode = { Name="$UserActionProgressMode$", EditorHelp="$UserActionProgressModeHelp$", Type="enum", Options = [ { Name="$Session$", Value="session" }, { Name="$Player$", Value="player" }, { Name="$Global$" } ] };
	PropParallel = { Name="$ParallelAction$", EditorHelp="$ParallelActionHelp$", Type="bool" };
	return true;
}

public func GetObjectEvaluator(filter_def, name, help)
{
	// Create copy of the Evaluator.Object delegate, but with the object_constant proplist replaced by a version with filter_def
	var object_options = Evaluator.Object.Options[:];
	var const_option = new EvaluatorDefs["object_constant"] {};
	const_option.Delegate = new const_option.Delegate { Filter=filter_def };
	object_options[const_option.OptionIndex] = const_option;
	return new Evaluator.Object { Name=name, Options=object_options, EditorHelp=help };
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
		AddEvaluator("Any", any_group, name, help, identifier, callback_data, default_val, delegate, delegate_storage_key);
	}
	// Dissect parameters
	if (group) group = GroupNames[group] ?? group; // resolve localized group name
	var short_name;
	if (GetType(name) == C4V_Array)
	{
		short_name = name[1];
		name = name[0];
	}
	else if (delegate && delegate.Type == "proplist" && !delegate.ShowFullName)
	{
		// Proplist delegates provide their own display string and need not show the option name
		short_name = "";
	}
	if (!default_val) default_val = {};
	var default_get;
	if (GetType(default_val) == C4V_Function)
	{
		default_get = default_val;
		default_val = Call(default_get, nil, {Function=identifier});
	}
	default_val.Function = identifier;
	var action_def = { Name=name, ShortName=short_name, EditorHelp=help, Group=group, Value=default_val, Delegate=delegate, Get=default_get }, n;
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
	Evaluator[eval_type].Options[n = GetLength(Evaluator[eval_type].Options)] = action_def;
	action_def.OptionIndex = n;
	// Remember lookup table through identifier (ignore duplicates)
	if (eval_type != "Any")
	{
		EvaluatorCallbacks[identifier] = callback_data;
		EvaluatorDefs[identifier] = action_def;
	}
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

public func EvaluateAction(proplist props, object action_object, object triggering_object, int triggering_player, string progress_mode, bool allow_parallel, finish_callback)
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
		var plr_id;
		if (action_object) plr_id = GetPlayerID(action_object->GetOwner());
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
	context->InitContext(action_object, triggering_player, triggering_object, props, finish_callback);
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

private func EvaluatePosition(proplist props, object context)
{
	// Execute position evaluator; fall back to position of action object
	var position = EvaluateValue("Position", props, context);
	if (!position)
	{
		if (context.action_object) position = [context.action_object->GetX(), context.action_object->GetY()];
		else position = [0,0];
	}
	return position;
}

private func EvaluateOffset(proplist props, object context)
{
	// Execute offset evaluator; fall back to [0, 0]
	return  EvaluateValue("Offset", props, context) ?? [0,0];
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
	var params = Find_And(), np=1;
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
	for (var i=0; i<n; ++i) result[i] = GetPlayerByIndex(i);
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
	for (cond in props.Operands)
		if (!EvaluateValue("Boolean", cond, context))
			return false;
	return true;
}

private func EvalBool_Or(proplist props, proplist context)
{
	for (cond in props.Operands)
		if (EvaluateValue("Boolean", cond, context))
			return true;
	return false;
}

private func EvalBool_ObjectExists(proplist props, proplist context) { return !!EvaluateValue("Object", props.Object, context); }

private func EvalAct_Sequence(proplist props, proplist context)
{
	// Sequence execution: Iterate over actions until one action puts the context on hold
	var n = GetLength(props.Actions), sid = props._sequence_id;
	if (!sid) sid = props._sequence_id = Format("%d", ++UserAction_SequenceIDs);
	for (var progress = context.sequence_progress[sid] ?? 0; progress < n; ++progress)
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
			context.sequence_progress[sid] = progress;
			return;
		}
		// Apply jump in the sequence
		if (context.sequence_had_goto[sid]) progress = context.sequence_progress[sid] - 1;
	}
	// Sequence finished
	context.last_sequence = nil;
	// Reset for next execution.
	context.sequence_progress[sid] = 0;
}

private func EvalAct_Goto(proplist props, proplist context)
{
	// Apply goto by jumping in most recently executed sequence
	if (context.last_sequence)
	{
		context.sequence_progress[context.last_sequence._sequence_id] = props.Index;
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

private func EvalAct_Wait(proplist props, proplist context)
{
	// Wait for specified number of frames
	context.hold = props;
	ScheduleCall(context, UserAction.ResumeAction, props.Time, 1, context, props);
}

private func EvalAct_Sound(proplist props, proplist context)
{
	if (!props.Sound) return;
	var sound_context = props.SourceObject ?? Global;
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
		if (props.CreateAbove)
			obj = Global->CreateObjectAbove(create_id, position[0], position[1], owner);
		else
			obj = Global->CreateObject(create_id, position[0], position[1], owner);
		// Default speed
		if (obj) obj->SetXDir(speed_x);
		if (obj) obj->SetYDir(speed_y);
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
	context.last_casted_objects = CastObjects(create_id, amount, speed, position[0], position[1], mean_angle, angle_deviation);
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

private func EvalAct_ObjectCallInt(proplist props, proplist context, func call_fn)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return;
	var parameter = EvaluateValue("Integer", props.Value, context);
	obj->Call(call_fn, parameter);
}

private func EvalAct_If(proplist props, proplist context, eval_type)
{
	// Do evaluation on first pass. After that, take context value.
	var sid = props._sequence_id;
	if (!sid) sid = props._sequence_id = Format("%d", ++UserAction_SequenceIDs);
	if (context.sequence_progress[sid] = context.sequence_progress[sid] ?? !!EvaluateValue("Boolean", props.Condition, context))
		return EvaluateValue(eval_type, props.TrueEvaluator, context);
	else
		return EvaluateValue(eval_type, props.FalseEvaluator, context);
}

private func EvalAct_Log(proplist props, proplist context)
{
	Log(EvaluateValue("String", props.Message, context) ?? "");
}

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
	var var_name = StringToIdentifier(EvaluateValue("String", props.VariableName, context) ?? "");
	var value = EvaluateValue("Any", props.Value, context);
	var_context[var_name] = value;
}

private func EvalVariable(proplist props, proplist context, expected_type)
{
	// Get variable value
	var var_context = GetVariableContext(props.Context, context);
	var var_name = StringToIdentifier(EvaluateValue("String", props.VariableName, context) ?? "");
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
	var script = EvaluateValue("String", props.Script, context) ?? "";
	return script_context->eval(script, true);
}

private func GetDefaultPosition(object target_object)
{
	// Default position for constant absolute position evaluator: Use selected object position
	var value;
	if (target_object)
		value = [target_object->GetX(), target_object->GetY()];
	else
		value = [0,0];
	return { Function="position_constant", Value=value };
}

private func GetDefaultCoordinates(object target_object)
{
	// Default position for constant absolute position evaluator: Use selected object position
	var value;
	if (target_object)
		value = {X={Function="int_constant", Value=target_object->GetX()}, Y={Function="int_constant", Value=target_object->GetY()}};
	else
		value = {X=0, Y=0};
	value.Function="position_coordinates";
	return value;
}

private func GetDefaultRect(object target_object, proplist props)
{
	// Default rectangle around target object
	var r;
	if (target_object) r = [target_object->GetX()-30, target_object->GetY()-30, 60, 60]; else r = [100,100,100,100];
	return { Function=props.Function, Area=r };
}

private func GetDefaultCircle(object target_object, proplist props)
{
	// Default circle around target object
	var r;
	if (target_object) r = [50, target_object->GetX(), target_object->GetY()]; else r = [50,100,100];
	return { Function=props.Function, Area=r };
}

private func GetDefaultSearchRect(object target_object)
{
	// Default rectangle around target object
	var r;
	if (target_object) r = [target_object->GetX()-30, target_object->GetY()-30, 60, 60]; else r = [100,100,100,100];
	return { Function="Rect", Area=r };
}

private func GetDefaultSearchCircle(object target_object)
{
	// Default circle around target object
	var r;
	if (target_object) r = [50, target_object->GetX(), target_object->GetY()]; else r = [50,100,100];
	return { Function="Circle", Area=r };
}

private func EvalIntRandom(proplist props, proplist context)
{
	// Random value between min and max. Also allow them to be swapped.
	var min = EvaluateValue("Integer", props.Min, context);
	var max = EvaluateValue("Integer", props.Max, context);
	var rmin = Min(min,max);
	return Random(Max(max,min)-rmin) + rmin;
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
	return [0,0]; // undefined object: Position is 0/0 default
}

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

private func EvalStr_ValueToString(proplist props, proplist context)
{
	return Format("%v", EvaluateValue("Any", props.Value, context));
}

private func EvalStr_Concat(proplist props, proplist context)
{
	var result="";
	for (var s in props.Substrings) result = Format("%s%s", result, EvaluateValue("String", s, context) ?? "");
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
local sequence_progress, sequence_had_goto;
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
	sequence_progress = {};
	sequence_had_goto = {};
	return true;
}

public func InitContext(object action_object, int triggering_player, object triggering_object, proplist props, finish_callback)
{
	// Determine triggering player+objects
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
	// Init context settings
	this.action_object = action_object;
	this.triggering_object = triggering_object;
	this.triggering_clonk = triggering_clonk;
	this.triggering_player = triggering_player;
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
		sequence_progress[last_sequence._sequence_id] = opt._goto;
		hold = nil;
	}
	UserAction->ResumeAction(this, hold);
}

public func SaveScenarioObject(props) { return false; } // temp. don't save.
