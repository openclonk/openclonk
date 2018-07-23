/**
	Scoreboard
	Provides an additional abstraction layer on top of the scoreboard-functions
	
	Attention: Do not include this. Use the function as specified with f.e. Scoreboard->Init(...)
*/

// do not manipulate externally
static Scoreboard_keys; // [{key = ?, title = ?, ..}, ..]
static Scoreboard_data; // [{key = value, ..}, ..]
static Scoreboard_unique_ids;
static Scoreboard_title_set;

static const Scoreboard_Y_title = SBRD_Caption;
static const Scoreboard_X_title = SBRD_Caption;

// wrapper for Scoreboard->NewEntry, adds a new entry for a player with the tagged player name as the title
public func NewPlayerEntry(int plr)
{
	return Scoreboard->NewEntry(GetPlayerID(plr), GetTaggedPlayerName(plr));
}

public func RemovePlayerEntry(int plr)
{
	return Scoreboard->RemoveEntry(GetPlayerID(plr));
}

// adds a new entry (row) to the scoreboard, will return the ID of the added entry
// the parameter new_id might be nil in which case a unique new ID is chosen
public func NewEntry(
	int new_id /* unique ID for the new entry, can be nil */
	, string new_title /* text for the first column */
	)
{
	// not initialized yet? Then init empty.
	if (!Scoreboard_keys) Init([]);
	// check for duplicates
	if(new_id)
	{
		var index = -1;
		for(var i = 0; i < GetLength(Scoreboard_data); ++i)
		{
			if(Scoreboard_data[i].ID != new_id) continue;
			index = i;
			break;
		}
		
		// duplicate?
		if(index != -1) return nil;
	}
	else // no ID provided, get new one
	{
		new_id = ++Scoreboard_unique_ids;
	}
	
	PushBack(Scoreboard_data, {ID = new_id, title = new_title});
	Scoreboard->Update(new_id);
	return new_id;
}

// removes an entry (row) from the scoreboard
public func RemoveEntry(
	int remove_id /* unique ID for the new entry, can be nil */
	)
{
	var i, len = GetLength(Scoreboard_data);
	for(i = 0; i < len; ++i) if(Scoreboard_data[i].ID == remove_id) break;
	if (i == len) return false; // not found
	RemoveArrayIndex(Scoreboard_data, i);
	// Clear all columns
	for (var col in Scoreboard_keys) SetScoreboardData(remove_id, col.index);
	SetScoreboardData(remove_id, Scoreboard_X_title);
	return true;
}

// sets a value for a specific key of a scoreboard entry for a player and updates it
// that entry must have been created earlier with ScoreboarNewPlayerEntry or manually with Scoreboard->NewEntry
public func SetPlayerData(
	int plr /* player number for the player entry to manipulate */
	, string key /* name of the key assign value to */
	, to /* value to assign, might be ID, string, int or bool */
	, int sort_parameter /* parameter used for sorting. if nil, 'to' is used if possible*/
	)
{
	var ID = GetPlayerID(plr);
	return Scoreboard->SetData(ID, key, to, sort_parameter);
}

// sets a value for a specific key of any entry in the scoreboard and updates it
// the entry must have been created with Scoreboard->NewEntry earlier
public func SetData(
	int ID /* ID of the entry to manipulate */
	, string key /* name of the key to assign value to */
	, to /* new value of the key, might be int, string bool or id */
	, int sort_parameter /* parameter that is used for sorting of the entry. if nil, 'to' is used if possible*/
	)
{
	var index = -1;
	for(var i = 0; i < GetLength(Scoreboard_data); ++i)
	{
		if(Scoreboard_data[i].ID == ID)
		{
			index = i;
			break;
		}
	}
	if(index == -1) return;
	
	Scoreboard_data[index][key] = to;
	if(sort_parameter)
		Scoreboard_data[index][Format("%s_", key)] = sort_parameter;
	Scoreboard->Update(ID);
}

// updates the display of the scoreboard
// there is usually no need to call it manually since Scoreboard->SetData calls Update
public func Update(
	int ID /* ID of the entry to update */
	, int index /* optional, internal index. leave at nil.*/
	)
{
	if(index == nil)
	{
		for(var i = 0; i < GetLength(Scoreboard_data); ++i)
		{
			if(Scoreboard_data[i].ID != ID) continue;
			index = i;
			break;
		}
		if(index == nil) return;
	}
	
	var data = Scoreboard_data[index];
	
	// title - playername f.e.
	SetScoreboardData(data.ID, Scoreboard_X_title, data.title, -1);
	
	var len = GetLength(Scoreboard_keys);
	for(var i = 0; i < len; ++i)
	{
		var col = Scoreboard_keys[i];
		if(!col.key) continue;
		if(col.key == "title") continue; // already set
		
		// aquire value
		var value = col.default;
		if(data[col.key] != nil) value = data[col.key];
		
		// get sorting parameter
		var sort = data[Format("%s_", col.key)];
		if(sort == nil)
			if(GetType(value) == C4V_Int) sort = value;
		
		// the column provides a conditional function
		if(col.conditional)
		{
			value = col->conditional(value);
			
			// overwrite old sort?
			if(GetType(value) == C4V_Int) sort = value;
		}
		
		if(value == nil) value = "";
		else if(GetType(value) == C4V_Int) {value = Format("%d", value);}
		else if(GetType(value) == C4V_Def) value = Format("{{%i}}", value);
		else if(GetType(value) == C4V_Bool) {sort = value; if(value) value = "X"; else value = "";}

		SetScoreboardData(data.ID, col.index, value, sort);
	}
	UpdateSort();
	return true;
}

public func UpdateSort()
{
	// do sorting for fields neccessary
	var len = GetLength(Scoreboard_keys);
	for(var i = len-1; i >= 0; --i)
	{
		var col = Scoreboard_keys[i];
		if(!col.key) continue;
		if(!col.sorted) continue;
		
		SortScoreboard(col.index, col.desc);
	}
	return true;
}

// updates the whole scoreboard
public func UpdateAll()
{
	// for every row, do update
	var len = GetLength(Scoreboard_data);
	for(var i = 0; i < len; ++i)
	{
		Scoreboard->Update(Scoreboard_data[i].ID, i);
	}
	return true;
}

// remove a column from the scoreboard
public func RemoveColumn(string key)
{
	var col, data;
	if (!key) FatalError("Scoreboard::RemoveColumn: Key required!");
	// find key
	for (col in Scoreboard_keys) if (col.key == key) break;
	if (!col || col.key != key) return false; // key not found. fail but not fatal.
	// for every row, remove data for key
	for (data in Scoreboard_data)
	{
		data[key] = data[Format("%s_", key)] = nil;
		SetScoreboardData(data.ID, col.index);
	}
	// resort
	SortScoreboard(col.index, col.desc);
	return true;
}

// initializes the scoreboard with certain columns and attributes
// can be called multiple times and scales
// values must be an array of proplists where each entry stands for one column
// the entries can have the following attributes:
// "key" is required and used later for Scoreboard->SetData
// {key (string), priority (int), sorted (bool), desc (bool), title (string), default (any)}
// one row with key = "title" is necessary and will be automatically added if left out and not existent
public func Init(array values /* see description */)
{
	// first call?
	if(Scoreboard_unique_ids == nil)
	{
		Scoreboard_unique_ids = 0xffffff;
		Scoreboard_keys = [];
		Scoreboard_data = [];
		
		if(!Scoreboard_title_set)
			Scoreboard->SetTitle("Scoreboard");
	}
	
	// merge arrays
	for(var val in values)
	{
		if(val == nil) continue;
		
		var index = nil;
		for(var i = GetLength(Scoreboard_keys)-1; i >= 0; --i)
		{
			if(Scoreboard_keys[i].key != val.key) continue;
			index = i;
			break;
		}
		if(index == nil)
			PushBack(Scoreboard_keys, val); // new entry
		else Scoreboard_keys[index] = val; // overwrite
	}
	
	// title property set?
	var found_title = false;
	for(var col in Scoreboard_keys)
	{
		if(col.key != "title") continue;
		found_title = true;
		break;
	}
	
	// if not, add
	if(!found_title)
		PushBack(Scoreboard_keys, {key = "title", title = ""});
	
	
	// sort, selection sort
	var len = GetLength(Scoreboard_keys);
	for(var x = 0; x < len - 1; ++x)
	{
		var max = -1, max_val = nil;
		for(var i = x; i < len; ++i)
		{
			var data = Scoreboard_keys[i]; 
			if(max_val == nil || (data.priority > max_val))
			{
				max = i;
				max_val = data.priority;
			}
		}
		if(max == -1) break;
		
		var t = Scoreboard_keys[x];
		Scoreboard_keys[x] = Scoreboard_keys[max];
		Scoreboard_keys[max] = t;
	}
	
	// assign indices to scoreboard data, they are now sorted - also create scoreboard
	var len = GetLength(Scoreboard_keys);
	for(var i = 0; i < len; ++i)
	{
		Scoreboard_keys[i].index = i;
		
		if(Scoreboard_keys[i].key == "title") // title has special index and no headline
		{
			Scoreboard_keys[i].index = Scoreboard_X_title;
			// don't set headline for title (first) column, because that would change the scoreboard title
			continue;
		}
		
		// check title
		if(GetType(Scoreboard_keys[i].title) == C4V_Def)
			Scoreboard_keys[i].title = Format("{{%i}}", Scoreboard_keys[i].title);
					
		var data = -0xffffff;
		if(Scoreboard_keys[i].desc) data = 0xffffff;
		SetScoreboardData(Scoreboard_Y_title, Scoreboard_keys[i].index, Scoreboard_keys[i].title, data);
	}
	
	
	// setup scoreboard
	Scoreboard->UpdateAll();
	
	// show scoreboard, only initially - after that it's the player's choice
	DoScoreboardShow(1); 
}

// sets the title of the scoreboard, standard is "Scoreboard"
public func SetTitle(string title)
{
	SetScoreboardData(SBRD_Caption, SBRD_Caption, title);
	Scoreboard_title_set = true;
}
