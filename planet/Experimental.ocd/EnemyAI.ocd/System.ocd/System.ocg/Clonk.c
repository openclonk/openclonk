// Editor tweak

#appendto Clonk

func EditCursorSelection()
{
	var ai = S2AI->GetAI(this);
	if (ai) Call(S2AI.EditCursorSelection, ai);
	return _inherited(...);
}

func EditCursorDeselection()
{
	var ai = S2AI->GetAI(this);
	if (ai) Call(S2AI.EditCursorDeselection, ai);
	return _inherited(...);
}