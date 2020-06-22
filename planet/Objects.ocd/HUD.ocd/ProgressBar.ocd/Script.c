/**
	GUI_ProgressBar
	Provides the interface used by different progress bars.
*/

local Name = "$Name$";
local Description = "$Description$";

// creates a progress bar of the given ID and returns it
global func CreateProgressBar(
	ID /* ID of the progress bar */
	, int max /* maximum value of the progress bar */
	, int current /* starting value of the progress bar */
	, int time_out /* time in frames after which the progress bar closes itself when not receiving updates, might be nil */
	, int owner /* owner of the progress bar */
	, proplist offset /* proplist {x = ?, y = ?} that specifies the offset of the progress bar relative to the calling object */
	, int visibility /* visibility mask for the progress bar, f.e.: VIS_Owner | VIS_Allies */
	, proplist data /* proplist with extra data that is passed to the progress bar */
	)
{
	owner = owner ?? NO_OWNER;
	visibility = visibility ?? VIS_All;
	offset = offset ?? {x = 0, y = 0};
	max = max ?? 100;
	current = current ?? 0;
	data = data ?? {};
	
	if (!ID)
		FatalError("CreateProgressBar called without valid ID");
		
	var obj = CreateObject(ID, AbsX(0), AbsY(0), owner);
	obj->Init(this, max, current, time_out, offset, visibility, data);
	
	return obj;
}

// closes the progress bar and usually removes it
func Close(){return _inherited(...);}

// sets the value of the progress bar, updates the progress bar
func SetValue(int to){return _inherited(to, ...);}

// changes the value of the progress bar by the specified amount, usually calls SetValue
func DoValue(int change){return _inherited(change, ...);}

// changes the offset {x = ?, y = ?}  of the progress bar relative to the attached object (or global)
func SetOffset(proplist offset){return _inherited(offset, ...);}

// makes the progress bar 100% parallax
func SetParallax(){return _inherited(...);}

// sets the Plane property of the progress bar
func SetPlane(int to) {return _inherited(...);}

// makes the object a HUD element by setting parallaxity and the category C4D_StaticBack | C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax
func MakeHUDElement() {return _inherited(...);}

// called once on creation by CreateProgressBar on the new bar
func Init(object to /* object to attach the bar to */
		, int maximum /* maximum value of the progress bar (100%) */
		, int current /* starting value of the progress bar (0 <= current <= maximum*/
		, int timeout /* time in frames after which the progress bar should Close itself when not receiving updates, might be nil */
		, proplist offset /* proplist with properties "x" and "y" that specifies the offset of the bar relative to the target object, the progress bar might provide standard values */
		, proplist data /* proplist with additional data the progress bar can use */
		)
{return _inherited(to, maximum, current, timeout, offset, data, ...);}

// updates the visuals of the progress bar
func Update(){return _inherited(...);}
