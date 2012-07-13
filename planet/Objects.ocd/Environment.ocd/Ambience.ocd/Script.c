/**
	Ambience
	Cares about the placement of purely visual objects.
	The placement uses categories and thus is forward-compatible.
*/

// this proplist defines the selectable environment objects
// "ID" might be nil or a valid id
// "includes" specifies what objects are created when the selected object is created (no specific order)
// any entry of "Environment_Attributes" might be nil or false instead of a proplist
// nil will log a warning on creating that object and false will silently ignore it
// thus something like Environment_Attributes["Foobar"] = false; will work to disable certain features
static const Environment_Attributes =
{
	All = {
		ID = nil,
		includes = ["Temperate", "Desert"],
	},
	
	Temperate = {
		ID = nil,
		includes = ["Zicadas", "Frogs", "BackgroundBirds"],
	},
	
	Desert = {
		ID = nil,
		includes = ["Zicadas"],
	},
	
	Zicadas = {
		ID = Ambience_Zicadas,
	},
	
	Frogs = {
		ID = nil /* not yet implemented: Environment_Frogs */,
	},
	
	BackgroundBirds = {
		ID = nil /* not yet implemented: Environment_BackgroundBirds */,
	},
};



// provides a simple interface for creation of environment objects and decoration with standard values
// the objects are placed on a best-effort-basis. That means f.e. objects that rely on water will not be placed when there is no water in the landscape.
global func CreateEnvironmentObjects(
	what /* array of strings or single string: what objects will be created, standard: "All" */
	, proplist area /* area where objects will be created, format {x = ??, y = ??, w = ??, h = ??}, standard: whole landscape */
	, int amount_percentage /* what percentage of the standard amount will be created, standard: 100 */
	)
{
/*
// half desert, half temperate - but birds everywhere
CreateEnvironmentObjects(["Desert", "BackgroundBirds"], Rectangle(0, 0, LandscapeWidth()/2, LandscapeHeight()));
CreateEnvironmentObjects("Temperate", Rectangle(LandscapeWidth()/2, 0, LandscapeWidth()/2, LandscapeHeight()));
*/
	what = what ?? "All";
	area = area ?? Rectangle(0, 0, LandscapeWidth(), LandscapeHeight());
	amount_percentage = amount_percentage ?? 100;
	
	// might be a string to allow CreateEnvironmentObjects("All")
	if(GetType(what) != C4V_Array)
		what = [what];
	
	// iteratively find all the objects that are included in the selection
	while(true)
	{
		var changed = false;
		var to_add = [];
		
		// go through every object in the list
		for(var obj in what)
		{
			var p = Environment_Attributes[obj];
			if(p == nil) {Log("Warning: Environment object %s does not exist!", obj);}
			else if(p == false) continue; // disabled by the scenario designer
			
			// add all objects included to the temporary list if existing
			if(!p["includes"]) continue;
			to_add = Concatenate(to_add, p["includes"]);
		}
		
		// add every unique item from the temporary list to the object list
		for(var obj in to_add)
		{
			if(IsValueInArray(what, obj)) continue;
			
			if(!!Environment_Attributes[obj]["includes"])
				changed = true; // found changes, need further checking
			
			PushBack(what, obj);
		}
		
		if(!changed)
			break;
	}
	
	// now create all the selected objects
	for(var obj in what)
	{
		var p, p_id;
		if(!(p = Environment_Attributes[obj])) continue;
		if(!(p_id = p["ID"])) continue;
		
		p_id->Place(amount_percentage, area);
	}
}