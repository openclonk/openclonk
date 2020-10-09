local ActMap=
{
	Attach = 
	{
		Prototype = Action,
		Name="Attach",
		Procedure = DFA_ATTACH,
		NextAction="Be",
		Length = 1,
		FacetBase = 1,
		AbortCall = "AttachTargetLost"
	}
};