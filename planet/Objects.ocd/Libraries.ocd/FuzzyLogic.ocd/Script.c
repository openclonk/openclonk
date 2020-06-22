/**
	FuzzyLogic
	Contains functions to evaluate fuzzy sets.
	Objects using this library first need to declare fuzzy sets with AddSet and fuzzy rules with AddRule
	and can then update the fuzzy values using Fuzzify.
	FuzzyExec can then be called to evaluate the current upates and returns an array of actions based on the AddRule calls.
	
	For an example, see the fish.
	
	
	If you happen to notice anything weird regarding how the data of this library is handled internally, it is probably due to optimizations.
	@author Zapper
	
	
	Objects using this library must call _inherited on:
		- Construction
*/

static const FUZZY_NOT = 1;
static const FUZZY_AND = 2;
static const FUZZY_OR = 3;

/*
	Returns a new empty fuzzy logic object.
*/
public func Init()
{
	return
	{
		rules = [],
		sets = {},
		actions = {},
		// updates[set][subset]=[unused, value, usage_count]
		updates = {},
		
		// both caches help when values change either not at all or only slowly
		// which could happen quite often. (For example the "hunger" of an animal could be at the same value for quite some FuzzyExecs)
		cache_defuz = {}, // optimizes defuzzification
		cache_fuz = {}, // optimizes fuzzification
		DumpCache = FuzzyLogic.FuzzyDumpCache,
		// the interface
		AddSet = FuzzyLogic.AddFuzzySet,
		AddRule = FuzzyLogic.AddFuzzyRule,
		Execute = FuzzyLogic.Execute,
		Fuzzify = FuzzyLogic.Fuzzify,
		And = FuzzyLogic.FuzzyAnd,
		Or = FuzzyLogic.FuzzyOr,
		Not = FuzzyLogic.FuzzyNot
	};
}

/*
	AddFuzzySet("swim", "left", [[-1000, 1], [-500, 1], [0, 0]]);
*/
public func AddFuzzySet(string set, string subset, array data, bool no_warn)
{
	if (!this.sets[set])
		this.sets[set] = {};
	// normaliz data
	// the Y values should normally be 0 or 1 and are normalized to 0 and 1000 here
	for (var item in data)
	{
		if (item[1] != 0 && item[1] != 1 && !no_warn)
			Log("Warning: AddFuzzyAction for %s/%s has a strength value of %d", set, subset, item[1]);
		else
			item[1] *= 1000;
	}
	this.sets[set][subset] = data;
	
	// init cache for that entry
	if (!this.cache_defuz[set])
		this.cache_defuz[set] = {};
	this.cache_defuz[set][subset] = [-1, nil];
	
	if (!this.cache_fuz[set])
		this.cache_fuz[set] = {};
	this.cache_fuz[set] = 0xffffff;
}

public func Execute()
{
	var actions = {};
	// apply all the rules to determine the action values
	for (var rule in this.rules)
	{
		if (!actions[rule[1][0]])
			actions[rule[1][0]] = {};
		var value = ApplyFuzzyRule(rule[0]);
		actions[rule[1][0]][rule[1][1]] = Max(actions[rule[1][0]][rule[1][1]], value);
	}
	
	// now defuzzify the actions
	for (var action in GetProperties(actions))
	{
		var weighted_sum = 0, sum = 0;
		
		for (var subset in GetProperties(actions[action]))
		{
			var strength = actions[action][subset];
			var defuz; // [centroid, weight]
			
			// try the cache
			if (this.cache_defuz[action][subset][0] == strength)
			{
				defuz = this.cache_defuz[action][subset][1];
			}
			else // cache miss
			{
				defuz = Defuzzify(action, subset, strength);
				this.cache_defuz[action][subset] = [strength, defuz];
			}
			
			weighted_sum += defuz[0] * defuz[1];
			sum += defuz[1];
			//Log("Fuzzy exec: action %s/%s: %d -> cx: %d, weight: %d", action, subset, actions[action][subset], defuz[0], defuz[1]);
		}
		
		if (!sum)
			this.actions[action] = 0;
		else
		{
			this.actions[action] = weighted_sum / sum;
			//Log("Action [%s] defuzzifies to %d", action, FuzzyLogic.actions[action]);
		}
	}
	return this.actions;
}

/*
	calculates the centroid of an action set
	returns [centroid-X, weight]
*/
public func Defuzzify(string action, string subset, int strength)
{
	var subset_data = this.sets[action][subset];
	var centroid_data = [];
	for (var i = 0; i <= 1; ++i)
	{
		var data = DefuzzifyGetCentroidFromGraph(subset_data[i], subset_data[i + 1], strength, nil, true, i == 0);
		PushBack(centroid_data, data);
		//Log("     -> %d/%d, %d/%d with Y=%d: cx: %d, weight: %d", subset_data[i][0], subset_data[i][1], subset_data[i + 1][0], subset_data[i + 1][1], strength, data[0], data[1]);
	}
	
	// calculate joint weight from array
	var w_sum = 0, sum = 0;
	for (var centroid in centroid_data)
	{
		w_sum += centroid[0] * centroid[1]; 
		sum += centroid[1];
	}
	if (!sum)
		return [0, 0];
	return [w_sum / sum, sum];
}

/*
	calculates the [centroid, weight] from two points on a line and a fill-value
*/
public func DefuzzifyGetCentroidFromGraph(array coords1, array coords2, int Y, bool is_good_triangle, bool is_main_block, bool is_left_block)
{
	if ((Y == 0) || (coords1[0] == coords2[0]))
		return [0, 0];
		
	// rectangle? well, that's easy!
	// some additional treatment for "main" blocks, so that the weight is distributed more realistically
	if (coords1[1] == coords2[1])
	{
		var centroid_x;
		if (is_main_block)
		{
			if (is_left_block) centroid_x = coords1[0];
			else centroid_x = coords2[0];
		}
		else
			centroid_x = (coords1[0] + coords2[0]) / 2;
		var centroid_weight = (coords2[0] - coords1[0]) * (Y * coords1[1] / 1000);
		return [centroid_x, centroid_weight];
	}
	else
	if (is_good_triangle) // triangle which is already in a "good" normal form meaning Y is the max-Y
	{
		var h = (coords2[0] - coords1[0]) / 2;
		var c = coords1[0];
		if (coords1[1] < coords2[1])
			c = coords2[0];
		var s = (c - coords1[0] - h) / 3;
		var weight = (Max(coords1[1], coords2[1]) * h) / 2;
		return [coords1[0] + h + s, weight];
	}
	else
	{
		// generic triangle? that's a tiny bit harder
		var centroid_x = FuzzyCalcLineIntersectY(coords1, coords2, Y);
		//Log("          -> calc line %d/%d, %d/%d with Y=%d yields X=%d", coords1[0], coords1[1], coords2[0], coords2[1], Y, centroid_x);
		var d1 = DefuzzifyGetCentroidFromGraph([coords1[0], Min(coords1[1], Y)], [centroid_x, Y], Y, true);
		var d2 = DefuzzifyGetCentroidFromGraph([centroid_x, Y], [coords2[0], Min(coords2[1], Y)], Y, true);
		if (d1[1] + d2[1] == 0)
			return [0, 0];
		var center = (d1[0] * d1[1] + d2[0] * d2[1]) / (d1[1] + d2[1]);
		//Log("          -> d1_x: %d, d1_weight: %d, d2_x: %d, d2_weight: %d: centroid: %d", d1[0], d1[1], d2[0], d2[1], center);
		return [center, d1[1] + d2[1]];
	}
}

/*
	For debugging purposes.
*/
private func FuzzyDumpCache()
{
	Log("Fuzzification cache (last values):");
	for (var prop in GetProperties(this.cache_fuz))
	{
		var dump = Format("%s = %d -> ", prop, this.cache_fuz[prop]);
		if (!this.updates[prop])
		{
			dump = Format("%s ..no actions available..", dump);
		}
		else
		{
			for (var subset in GetProperties(this.updates[prop]))
			{
				dump = Format("%s %s (%d)", dump, subset, this.updates[prop][subset][1]);
			}
		}
		Log(dump);
	}
	Log("Defuzzification cache (last categorical actions):");
	for (var prop in GetProperties(this.cache_defuz))
	{
		var action_set = this.cache_defuz[prop];
		var dump = Format("%s: ", prop);
		for (var action in GetProperties(action_set))
		{
			dump = Format("%s %s (%v)", dump, action, action_set[action]);
		}
		Log(dump);
	}
	Log("end dump---------------------------");
}

// returns value between 0 and 1000 for how much the rule fits
private func ApplyFuzzyRule(array rule)
{
	if (rule[0] == FUZZY_AND)
	{
		var v1 = ApplyFuzzyRule(rule[1]);
		if (v1 < 50) return 0;
		var v2 = ApplyFuzzyRule(rule[2]);
		return Min(v1, v2);
	}
	
	if (rule[0] == FUZZY_OR)
	{
		var v1 = ApplyFuzzyRule(rule[1]);
		if (v1 > 950) return 1000;
		var v2 = ApplyFuzzyRule(rule[2]);
		return Max(v1, v2);
	}
	
	if (rule[0] == FUZZY_NOT)
	{
		// reverse all sets in rule
		return 1000 - ApplyFuzzyRule(rule[1]);
	}
	
	// normal rule, value has already been calcuated in FuzzyExec
	return rule[1];
}


/*
	takes "position", 3
	puts position = {left=[nil, 0], middle=[nil, 0.5], right=[nil, 0.9]} into updates (only for actually used subsets, though)
*/
public func Fuzzify(string set, int value)
{
	// does not need to change the updates at all if the value didn't change between two calls
	if (this.cache_fuz[set] == value) return true;
	this.cache_fuz[set] = value;
	
	// no rules for that set?
	if (!this.updates[set]) return true;
	
	for (var subset in GetProperties(this.updates[set]))
	{
		var subset_data = this.sets[set][subset];
		var result_value = nil;
		for (var i = 0; i <= 2; i += 2)
		{
			var X = subset_data[i][0];
			var Y = subset_data[i][1];
			if ((i == 0 && value <= X)
			||  (i == 2 && value >= X))
			{
				result_value = Y;
				break;
			}
		}
		
		// if no result found yet, calculate
		for (var i = 0; (i <= 1) && (result_value == nil); ++i)
		{
			if (subset_data[i][0] <= value && value <= subset_data[i + 1][0])
			{
				result_value = FuzzyCalcLine(subset_data[i], subset_data[i + 1], value);
				//Log("         -> calc line %d/%d, %d/%d value %d: result %d", subset_data[i][0], subset_data[i][1], subset_data[i + 1][0], subset_data[i + 1][1], value, result_value);
				break;
			}
		}
		
		this.updates[set][subset][1] = result_value;
		//Log("Fuzzify %s/%s: %d [from value %d]", set, subset, result_value, value);
	}
}

/*	
	internal helper
	calculates Y value for X on a line between coords1 and coords2
*/	
public func FuzzyCalcLine(array coords1, array coords2, int X)
{
	return 
		(
			(X *  (coords2[1] - coords1[1])
			+ (coords2[0] * coords1[1] - coords1[0] * coords2[1]))
		)
		/ (coords2[0] - coords1[0]);
}

/*
	internal helper
	calculates X for intersection with Y-value of a line
*/
public func FuzzyCalcLineIntersectY(array coords1, array coords2, int Y)
{
	var m = (coords2[1] - coords1[1]) / (coords2[0] - coords1[0]);
	return coords1[0] + ((Y - coords1[1]) / m);
}

// takes "position = left", returns [value = 0, usage_count = 0] for !no_cache and ["position", "left"] for no_cache
public func FuzzyStateStringToArray(state, bool no_cache)
{
	// already an array?
	if (GetType(state) == C4V_Array) return state;
	
	var first = "", second = "";
	var l = GetLength(state);
	var in_first = true;
	for (var i = 0; i < l; ++i)
	{
		var char = GetChar(state, i);
		
		if (char == 32) continue; // ignore spaces
		if (char == 61) // =
		{
			in_first = false;
			continue;
		}
		if (in_first)
			first = Format("%s%c", first, char);
		else second = Format("%s%c", second, char);
	}
	
	// some sanity checks to prevent people from tearing out their hair over non-working rules
	if (GetLength(first) == 0 || GetLength(second) == 0)
		FatalError(Format("FuzzyLogic: Error in rule string: %s [%s=%s]", state, first, second));
	if (!this.sets[first])
		FatalError(Format("FuzzyLogic: Error in rule string: %s [unknown set %s]", state, first));
	if (!this.sets[first][second])
		FatalError(Format("FuzzyLogic: Error in rule string: %s [unknown subset %s]", state, second));
	
	if (no_cache)
		return [first, second];
	
	// see whether that entry already exists in the cache
	if (!this.updates[first])
		this.updates[first] = {};
	if (!this.updates[first][second])
		this.updates[first][second] = [nil, 0, 0];
		
	return this.updates[first][second];
}

public func FuzzyAnd(condA, condB)
{
	if (!condB) return FuzzyStateStringToArray(condA);
	return [FUZZY_AND, FuzzyStateStringToArray(condA), FuzzyAnd(condB, ...)];
}

public func FuzzyOr(condA, condB)
{
	if (!condB) return FuzzyStateStringToArray(condA);
	return [FUZZY_OR, FuzzyStateStringToArray(condA), FuzzyOr(condB, ...)];
}

public func FuzzyNot(condA, int nope)
{
	return [FUZZY_NOT, FuzzyStateStringToArray(condA)];
}

/*
	/condition/ can be "position = left" or FuzzyAnd(FuzzyOr("position = left", "position = right"), FuzzyNot("position = middle"))
	/result/ should be "move = left"
*/
public func AddFuzzyRule(condition, string result)
{
	if (!result)
		FatalError("FuzzyLogic::AddRule needs result string");
	if (GetType(condition) == C4V_String)
		condition = FuzzyStateStringToArray(condition);
	result = FuzzyStateStringToArray(result, true);
	PushBack(this.rules, [condition, result]);
	
	this.actions[result[0]] = 0;
	
	return true;
}
