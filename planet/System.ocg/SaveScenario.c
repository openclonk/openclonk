/* Scenario saving functionality */
// Defines script function SaveScenarioObjects, which is called by the
// engine to generate the Objects.c file for scenario saving

// Temp variable used by MakeScenarioSaveName() to store dependency
static save_scenario_obj_dependencies;

// Propert identifier of object creation
static const SAVEOBJ_Creation = "Creation";
static const SAVEOBJ_ContentsCreation = "ContentsCreation";

global func SaveScenarioObjects(f)
{
	// f is a handle to the Objects.c file
	// Prepare props saving object
	var props_prototype = {
		Add = Global.SaveScenP_Add,
		AddSet = Global.SaveScenP_AddSet,
		AddCall = Global.SaveScenP_AddCall,
		Remove = Global.SaveScenP_Remove,
		RemoveCreation = Global.SaveScenP_RemoveCreation,
		Clear = Global.SaveScenP_Clear,
		Buffer2File = Global.SaveScenP_Buffer2File,
		HasData = Global.SaveScenP_HasData,
		HasCreation = Global.SaveScenP_HasCreation,
		HasProps = Global.SaveScenP_HasProps,
		HasProp = Global.SaveScenP_HasProp,
		TakeProps = Global.SaveScenP_TakeProps
	};
	// Write all objects!
	var objs = FindObjects(Find_And()), obj, i;
	var n = GetLength(objs);
	var obj_type, any_written, do_write_file = false;
	// In reverse order (background to foreground)
	for (i=0; i<n/2; ++i) { obj = objs[i]; objs[i] = objs[n-i-1]; objs[n-i-1] = obj; }
	// ...Except player crew
	var ignore_objs = [];
	for (var iplr = 0; iplr < GetPlayerCount(C4PT_User); ++iplr)
		for (var icrew = 0, crew; crew = GetCrew(GetPlayerByIndex(iplr, C4PT_User), icrew); ++icrew)
			ignore_objs[GetLength(ignore_objs)] = crew;
	// Write creation data and properties
	var obj_data = SaveScen_Objects(objs, ignore_objs, props_prototype);
	// Resolve dependencies
	obj_data = SaveScen_ResolveDepends(objs, obj_data);
	// Write scripts to force containers
	obj_data = SaveScen_SetContainers(obj_data);
	// Write header
	FileWrite(f, "/* Automatically created objects file */\n\n");
	// Declare static variables for objects that wish to have them
	for (obj in objs)
		if (obj.StaticSaveVar)
		{
			if (!any_written) FileWrite(f, "static "); else FileWrite(f, ", ");
			FileWrite(f, obj.StaticSaveVar);
			any_written = true;
		}
	if (any_written)
	{
		FileWrite(f, ";\n\n");
		any_written = false;
	}
	// Write all the creation data
	FileWrite(f, "func InitializeObjects()\n{\n");
	any_written = false;
	for (obj in obj_data)
	{
		if (obj_type != obj.o->GetID())
		{
			if (any_written) FileWrite(f, "\n"); // Extra spacing between different object types
			obj_type = obj.o->GetID();
			any_written = false;
		}
		if (obj.o.StaticSaveVar)
		{
			if (obj.props->HasCreation()) FileWrite(f, Format("	%s = ", obj.o.StaticSaveVar));
		}
		else if (obj.write_label)
		{
			FileWrite(f, Format("	var %s", obj.o->MakeScenarioSaveName()));
			if (obj.props->HasCreation()) FileWrite(f, " = "); else FileWrite(f, ";\n");
		}
		else if (obj.props->HasCreation())
		{
			FileWrite(f, "	");
		}
		if (obj.props->~Buffer2File(f)) do_write_file = any_written = true;
	}
	// Write global effects
	any_written = false;
	var fx; i=0;
	while (fx = GetEffect("*", nil, i++))
	{
		var fx_buffer = {Prototype=props_prototype};
		EffectCall(nil, fx, "SaveScen", fx_buffer);
		if (fx_buffer->HasData())
		{
			if (!any_written && do_write_file) FileWrite(f, "	\n");
			any_written = do_write_file = true;
			fx_buffer->~Buffer2File(f);
		}
	}
	// Write footer
	FileWrite(f, "	return true;\n}\n");
	// Done; success. Return true if any objects or effects were written to the file.
	return do_write_file;
}

global func SaveScen_Objects(array objs, array ignore_objs, proplist props_prototype)
{
	// Write all object data into buffers
	var n = GetLength(objs);
	var obj_data = CreateArray(n), obj;
	for (var i=0; i<n; ++i)
	{
		obj = objs[i];
		obj_data[i] = { o=obj, props={Prototype=props_prototype} };
		// Skip objects on ignore list
		if (GetIndexOf(ignore_objs, obj)>=0) continue;
		if (WildcardMatch(Format("%i", obj->GetID()), "GUI_*")) continue; // big bunch of objects that should not be stored.
		// Generate object creation and property strings
		save_scenario_obj_dependencies = [];
		if (!obj->SaveScenarioObject(obj_data[i].props))
		{
			obj_data[i].props->Clear();
			continue;
		}
		if (obj->Contained()) obj->Contained()->MakeScenarioSaveName(); // force container dependency
		if (obj_data[i].props->HasProps()) obj_data[i].write_label = true;
		obj_data[i].dependencies = save_scenario_obj_dependencies;
		obj_data[i].n_dependencies = GetLength(save_scenario_obj_dependencies);
		save_scenario_obj_dependencies = nil;
	}
	return obj_data;
}

global func SaveScen_ResolveDepends(array objs, array obj_data)
{
	// Dependency pointer from obj to obj_data
	var i,j,k,n=GetLength(objs),od;
	for (i=0; i<n; ++i)
	{
		for (j=0; j<obj_data[i].n_dependencies; ++j)
		{
			k = GetIndexOf(objs, obj_data[i].dependencies[j]);
			if (k < 0 || obj_data[i].dependencies[j] == obj_data[i].o) obj_data[i].dependencies[j] = nil; else obj_data[i].dependencies[j] = obj_data[k];
		}
		if (objs[i]->Contained())
		{
			k = GetIndexOf(objs, objs[i]->Contained());
			if (k >= 0) obj_data[i].co = obj_data[k];
		}
	}
	// Resolve dependencies
	k = 0;
	for (i=0; i<n; ++i)
	{
		k = Max(k, i);
		od = obj_data[i];
		while (od.i_dep_resolved < od.n_dependencies)
		{
			var dep = od.dependencies[od.i_dep_resolved++];
			if (dep)
			{
				dep.write_label = true; // All objects that someone else depends on need to be stored in a variable
				j = GetIndexOf(obj_data, dep);
				if (j > i)
				{
					// The dependent object is behind us in the list. This is bad!
					if (!od.props->HasCreation())
					{
						// This is just object properties. Move them behind dependent object creation
						var obj_data_new = CreateArray(n);
						obj_data_new[0:i] = obj_data[0:i];
						obj_data_new[i:j] = obj_data[i+1:j+1];
						obj_data_new[j] = obj_data[i];
						if (j<n-1) obj_data_new[j+1:n] = obj_data[j+1:n];
						obj_data = obj_data_new;
						if (j <= k) --k;
						--i; break;
					}
					else if (j <= k)
					{
						// Circular dependency. Detach object property setting from object creation.
						var obj_data_new = CreateArray(++n);
						obj_data_new[0:j+1] = obj_data[0:j+1];
						obj_data_new[j+1] = { o=od.o, co=od.co, dependencies=od.dependencies, n_dependencies=od.n_dependencies, i_dep_resolved=od.i_dep_resolved, props=od.props->TakeProps() };
						od.n_dependencies = 0; od.props_detached = true;
						if (j<n-1) obj_data_new[j+2:n] = obj_data[j+1:n];
						obj_data = obj_data_new;
						++k; break;
					}
					else
					{
						// No circular dependency. Just move object we depend on in front of us and process its dependencies.
						var obj_data_new = CreateArray(n);
						if (i) obj_data_new[0:i] = obj_data[0:i];
						obj_data_new[i] = obj_data[j];
						obj_data_new[i+1:j+1] = obj_data[i:j];
						if (j<n-1) obj_data_new[j+1:n] = obj_data[j+1:n];
						obj_data = obj_data_new;
						++k; --i; break;
					}
				}
			}
		}
	}
	// Free up all dependency data
	for (od in obj_data) od.dependencies = nil;
	return obj_data;
}

global func SaveScen_SetContainers(array obj_data)
{
	// Ensure that objects are in proper container
	// Replace calls to CreateObject with CreateContents.
	for (var obj in obj_data) if (obj.o->Contained())
	{
		// ignore if container object was not saved
		if (obj.co && obj.co.props->HasCreation())
		{
			// creation and props? Then turn into CreateContents
			if (obj.props->HasProp(SAVEOBJ_ContentsCreation) && !obj.props_detached)
			{
				obj.props->Remove(SAVEOBJ_Creation);
				// Adjust owner if necessery
				if (obj.o->GetOwner() != obj.o->Contained()->GetOwner())
					obj.props->AddCall("Owner", obj.o, "SetOwner", obj.o->GetOwner());
			}
			else if (obj.props.origin)
			{
				// props detached from creation. Use Enter() call to enter container
				// the label must have been written because something depended on the object.
				obj.props.origin->Remove(SAVEOBJ_ContentsCreation);
				obj.props->AddCall("Container", obj.o, "Enter", obj.o->Contained());
			}
		}
		else
		{
			// unsaved container - just create object outside
			obj.props->Remove(SAVEOBJ_ContentsCreation);
		}
	}
	return obj_data;
}

global func MakeScenarioSaveName()
{
	// Get name to be used to store this object in a scenario
	if (!this) FatalError("MakeScenarioSaveName needs definition or object context!");
	// Definitions may just use their regular name
	if (this.Prototype == Global) return Format("%i", this);
	// When the name is queried while properties are built, it means that there is a dependency. Store it.
	if (save_scenario_obj_dependencies && GetIndexOf(save_scenario_obj_dependencies, this)<0) save_scenario_obj_dependencies[GetLength(save_scenario_obj_dependencies)] = this;
	// Build actual name using unique number (unless there's a static save variable name for us)
	return this.StaticSaveVar ?? Format("%i%04d", GetID(), ObjectNumber());
}

global func SaveScenarioObject(props)
{
	// Called in object context: Default object writing procedure
	// Overwrite this method and return false for objects that should not be saved
	// Overwrite and call inherited for objects that add/remove/alter default creation/properties
	var owner_string = "";
	if (GetOwner() != NO_OWNER) owner_string = Format(", %d", GetOwner());
	props->Add(SAVEOBJ_Creation, "CreateObject(%i, %d, %d%s)", GetID(), GetX(), GetDefBottom(), owner_string);
	// Contained creation is added alongside regular creation because it is not yet known if CreateObject+Enter or CreateContents can be used due to dependencies.
	// func SaveScen_SetContainers will take care of removing one of the two creation strings after dependencies have been resolved.
	if (Contained()) props->Add(SAVEOBJ_ContentsCreation, "%s->CreateContents(%i)", Contained()->MakeScenarioSaveName(), GetID());
	// Write some default props every object should save
	var v, is_static = (GetCategory() & C4D_StaticBack) || Contained(), def = GetID();
	v = GetAlive();         if (!v && (GetCategory()&C4D_Living)) props->AddCall("Alive",         this, "Kill", this, true);
	v = GetDir();           if (v)                                props->AddCall("Dir",           this, "SetDir", GetConstantNameByValueSafe(v,"DIR_"));
	v = GetComDir();        if (v)                                props->AddCall("ComDir",        this, "SetComDir", GetConstantNameByValueSafe(v,"COMD_"));
	v = GetCon();           if (v != 100)                         props->AddCall("Con",           this, "SetCon", Max(v,1));
	v = GetCategory();      if (v != def->GetCategory())          props->AddCall("Category",      this, "SetCategory", GetBitmaskNameByValue(v, "C4D_"));
	v = GetR();             if (v)                                props->AddCall("R",             this, "SetR", v);
	v = GetXDir();          if (v && !is_static)                  props->AddCall("XDir",          this, "SetXDir", v);
	v = GetYDir();          if (v && !is_static) if (!Inside(v, 1,12) || !GetContact(-1, CNAT_Bottom))
	                                                              props->AddCall("YDir",          this, "SetYDir", v); // consolidate small YDir for standing objects
	v = GetRDir();          if (v && !is_static)                  props->AddCall("RDir",          this, "SetRDir", v);
	v = GetColor();         if (v && v != 0xffffffff)             props->AddCall("Color",         this, "SetColor", Format("0x%x", v));
	v = GetClrModulation(); if (v && v != 0xffffffff)             props->AddCall("ClrModulation", this, "SetClrModulation", Format("0x%08x", v));
	v = GetObjectBlitMode();if (v)                                props->AddCall("BlitMode",      this, "SetObjectBlitMode", GetBitmaskNameByValue(v & ~GFX_BLIT_Custom, "GFX_BLIT_"));
	v = GetName();          if (v != def->GetName())              props->AddCall("Name",          this, "SetName", Format("%v", v)); // TODO: Escape quotation marks, backslashes, etc. in name
	v = this.MaxEnergy;     if (v != def.MaxEnergy)               props->AddSet ("MaxEnergy",     this, "MaxEnergy", this.MaxEnergy);
	v = GetEnergy();        if (v != def.MaxEnergy/1000)          props->AddCall("Energy",        this, "DoEnergy", v-def.MaxEnergy/1000);
	v = this.Visibility;    if (v != def.Visibility)              props->AddSet ("Visibility",    this, "Visibility", GetBitmaskNameByValue(v, "VIS_"));
	v = this.Plane;         if (v != def.Plane)                   props->AddSet ("Plane",         this, "Plane", v);
	v = this.StaticSaveVar; if (v)                                props->AddSet ("StaticSaveVar", this, "StaticSaveVar", Format("%v", v));
	// update position on objects that had a shape change through rotation because creation at def bottom would incur a vertical offset
	if (GetR() && !Contained())                                   props->AddCall("SetPosition",   this, "SetPosition", GetX(), GetY());
	// Commands: Could store the whole command stack using AppendCommand.
	// However, usually there is one base command and the rest is derived
	// (e.g.: A Get command may lead to multiple MoveTo commands to the
	// target object). So just store the topmost command.
	var command, last_command, i=0;
	while (command = GetCommand(0, i++)) last_command = command;
	if (last_command)
	{
		i-=2;
		props->AddCall("Command", this, "SetCommand", Format("%v", last_command),
				SaveScenarioValue2String(GetCommand(1,i)), // target
				SaveScenarioValue2String(GetCommand(2,i)), // x
				SaveScenarioValue2String(GetCommand(3,i)), // y
				SaveScenarioValue2String(GetCommand(4,i)), // target2
				SaveScenarioValue2String(GetCommand(5,i))); // data
	}
	// Effects
	var fx; i=0;
	while (fx = GetEffect("*", this, i++)) EffectCall(this, fx, "SaveScen", props);
	return true;
}

global func SaveScenarioObjectAction(props)
{
	// Helper function to store action properties
	var v;
	props->AddCall("Action", this, "SetAction", Format("%v", GetAction() ?? "Idle"), GetActionTarget(), GetActionTarget(1));
	if (v = GetPhase()) props->AddCall("Phase", this, "SetPhase", v);
	return true;
}


global func FxFireSaveScen(object obj, proplist fx, proplist props)
{
	// this is burning. Save incineration to scenario.
	props->AddCall("Fire", obj, "Incinerate", fx.strength, fx.caused_by, fx.blasted, fx.incinerating_object);
	return true;
}


/* Helper functions for value formatting */

// Helper function to turn values of several types into a strings to be written to Objects.c
global func SaveScenarioValue2String(v, bitmask_prefix)
{
	var rval;
	if (bitmask_prefix) return GetConstantNameByValueSafe(v, bitmask_prefix);
	if (GetType(v) == C4V_C4Object) return v->MakeScenarioSaveName();
	if (GetType(v) == C4V_Array) // save procedure for arrays: recurse into contents (cannot save arrays pointing into itself that way)
	{
		for (var el in v)
		{
			if (rval) rval = Format("%s,%s", rval, SaveScenarioValue2String(el));
			else rval = SaveScenarioValue2String(el);
		}
		if (rval) rval = Format("[%s]", rval); else rval = "[]";
		return rval;
	}
	if (GetType(v) == C4V_PropList || GetType(v) == C4V_Def) // custom save procedure for some prop lists or definitions
	{
		rval = v->~ToString();
		if (rval) return rval;
	}
	// Otherwise, rely on the default %v formatting
	return Format("%v", v);
}

global func GetBitmaskNameByValue(v, prefix)
{
	// Compose bitmask of names of individual bits
	// e.g. GetBitmaskNameByValue(3, "C4D_") == "C4D_StaticBack|C4D_Structure"
	var s, n=0;
	for (var i=0; i<31 && v; ++i)
	{
		var v2 = 1<<i;
		if (v & v2)
		{
			v &= ~v2;
			var s2 = GetConstantNameByValue(v2, prefix);
			if (s2) s2 = Format("%s%s", prefix, s2); else s2 = Format("%x", v2);
			if (s) s = Format("%s|%s", s, s2); else s = s2;
		}
	}
	return s ?? GetConstantNameByValueSafe(0, prefix);
}

global func GetConstantNameByValueSafe(v, prefix)
{
	// Get constant name by value including prefix. If not found, return number as string
	var s = GetConstantNameByValue(v, prefix);
	if (s) return Format("%s%s", prefix, s); else return Format("%d", v);
}


/* SaveScen_PropList functions */
// I would like to use non-global here, but how can I take a pointer then?

global func SaveScenP_Add(string name, string s)
{
	// apply format parametrers
	s = Format(s, ...);
	// just append to array of strings
	// could build a string using data=Format("%s%s",data,s); here - however, then we'd be limited to some internal buffer sizes
	var new_data = {name=name, s=s};
	if (!this.data) this.data=[new_data]; else this.data[GetLength(this.data)] = new_data;
	return true;
}

global func SaveScenP_Remove(string name)
{
	// return all lines identified by name. return number of lines removed
	var idx, n=0, n_data;
	if (this.data)
	{
		n_data = GetLength(this.data);
		while (true)
		{
			idx = -1;
			while (++idx<n_data) if (this.data[idx].name == name) break;
			if (idx == n_data) break;
			++n;
			if (!--n_data) { this.data=nil; break; }
			if (idx < n_data) this.data[idx] = this.data[n_data];
			SetLength(this.data, n_data);
		}
	}
	return n;
}

global func SaveScenP_RemoveCreation()
{
	// Remove any creation strings
	return this->Remove(SAVEOBJ_ContentsCreation) + this->Remove(SAVEOBJ_Creation);
}

global func SaveScenP_Clear()
{
	// Remove all property and creation data
	this.data = nil;
	return true;
}

global func SaveScenP_AddCall(string name, proplist obj, string set_fn)
{
	// add string of style Obj123->SetFoo(bar, baz, ...)
	// string parameters will not be quoted, so the caller can do some parameter formatting
	// compose parameter string
	var max_pars = 10, last_written = 2, set_pars = "", n_pars = 0;
	for (var i=3; i<max_pars; ++i)
	{
		var par = Par(i);
		var par_type = GetType(par);
		if (par_type)
		{
			while (++last_written < i)
				if (n_pars++)
					set_pars = Format("%s%s", set_pars, ", nil");
				else
					set_pars = "nil";
			if (par_type != C4V_String) par = SaveScenarioValue2String(par);
			if (n_pars++)
				set_pars = Format("%s, %s", set_pars, par);
			else
				set_pars = par;
		}
	}
	// add setter
	return this->Add(name, "%s->%s(%s)", obj->MakeScenarioSaveName(), set_fn, set_pars);
}

global func SaveScenP_AddSet(string name, object obj, string local_name, value)
{
	// add string of style Obj123->local_name = value
	// string parameters will not be quoted, so the caller can do some parameter formatting
	if (GetType(value) != C4V_String) value = SaveScenarioValue2String(value);
	return this->Add(name, "%s.%s = %s", obj->MakeScenarioSaveName(), local_name, value);
}

global func SaveScenP_Buffer2File(f)
{
	// buffer.data is an array of strings to be written to file f
	if (!this.data) return false;
	var i=0, indent = "";
	for (var creation in [true, false])
	{
		for (var v in this.data)
		{
			var v_is_creation = ((v.name == SAVEOBJ_Creation) || (v.name == SAVEOBJ_ContentsCreation));
			if (v_is_creation != creation) continue;
			if (i || !creation) indent = "	";
			FileWrite(f, Format("%s%s;\n", indent, v.s));
			++i;
		}
	}
	return i;
}

global func SaveScenP_HasData()
{
	// Anything added to data array?
	return !!this.data;
}

global func SaveScenP_HasCreation()
{
	// Functions to test whether any creation data has been added
	if (!this.data) return false;
	for (var v in this.data) if ((v.name == SAVEOBJ_Creation) || (v.name == SAVEOBJ_ContentsCreation)) return true;
	return false;
}

global func SaveScenP_HasProps()
{
	// Functions to test whether any property data has been added
	if (!this.data) return false;
	for (var v in this.data) if ((v.name != SAVEOBJ_Creation) && (v.name != SAVEOBJ_ContentsCreation)) return true;
	return false;
}

global func SaveScenP_HasProp(string prop)
{
	// Test if specific prop is present
	if (!this.data) return false;
	for (var v in this.data) if (v.name == prop) return true;
	return false;
}

global func SaveScenP_TakeProps()
{
	// Remove all props from this data and add them to a copy of this
	var result = { Prototype = this.Prototype };
	if (this.data)
	{
		var creation = nil, props = nil;
		for (var v in this.data)
			if ((v.name != SAVEOBJ_Creation) && (v.name != SAVEOBJ_ContentsCreation))
				if (!props) props = [v]; else props[GetLength(props)] = v;
			else
				if (!creation) creation = [v]; else creation[GetLength(creation)] = v;
		this.data = creation;
		result.data = props;
		result.origin = this;
	}
	return result;
}
