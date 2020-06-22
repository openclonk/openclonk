/** 
	Tutorial Arrow
	Arrow to show important stuff to the player, can be created by using one of the global functions.
	
	@author Sven2, Maikel
*/

// Remove arrow if it was showing a target object which got removed.
protected func AttachTargetLost() { RemoveObject(); }
// Remove arrow if target object entered a container.
protected func Entrance() { RemoveObject(); }

static const ARROW_DEFAULT_ANGLE = 135;
static const ARROW_DEFAULT_DIST = 16;
static const ARROW_INCREASE_DIST = 8;

/*-- Arrow Creation --*/

protected func Construction()
{
	// Remove FoW
	SetLightRange(80);
}

/** Removes all tutorial arrows.
*/
global func TutArrowClear()
{
	for (var arrow in FindObjects(Find_ID(TutorialArrow)))
	{
		arrow->RemoveObject();
	}
	return;
}

/** Creates an arrow to indicate a position.
* @param x X-coordinate of the position.
* @param y Y-coordinate of the position.
* @param angle angle at which the arrow should be drawn, standard \c 135 degrees.
* @param dist distance of the arrow to the position, standard 16 pixels.
* @return the arrow created.
*/
global func TutArrowShowPos(int x, int y, int angle, int dist)
{
	var arrow = CreateObject(TutorialArrow, x, y, NO_OWNER);
	if (!arrow)
	{
		return;
	}
	// Display bouncing arrow, corrected for arrow size.
	angle = angle ?? ARROW_DEFAULT_ANGLE;
	dist = dist ?? ARROW_DEFAULT_DIST;
	dist += ARROW_INCREASE_DIST;
	x -= Sin(angle, dist);
	y += Cos(angle, dist);
	arrow->SetAction("Show");
	arrow->SetPosition(x, y);
	arrow->SetR(angle);
	return arrow;
}

/** Creates an arrow to indicate the target.
* @param target target object which should be indicated by the arrow.
* @param angle angle at which the arrow should be drawn, standard \c 135 degrees.
* @param dist distance of the arrow to the target object, standard 16 pixels.
* @return the arrow created.
*/
global func TutArrowShowTarget(object target, int angle, int dist)
{
	if (!target)
	{
		return;	
	}
	var arrow = CreateObject(TutorialArrow, target->GetX(), target->GetY(), NO_OWNER);
	if (!arrow)
	{
		return;
	}
	// Display spinning arrow, corrected for arrow size.
	angle = angle ?? ARROW_DEFAULT_ANGLE;
	dist = dist ?? ARROW_DEFAULT_DIST;
	dist += ARROW_INCREASE_DIST;
	arrow->SetAction("Attach", target);
	arrow->SetR(angle);
	arrow->SetVertex(0, VTX_Y, -dist, VTX_SetPermanentUpd);
	return arrow;
}

/** Creates an arrow to indicate a GUI position.
* @param x X-coordinate of the GUI position.
* @param y Y-coordinate of the GUI position.
* @param angle angle at which the arrow should be drawn, standard \c 135 degrees.
* @param dist distance of the arrow to the position, standard 16 pixels.
* @return the arrow created.
*/
global func TutArrowShowGUIPos(int x, int y, int angle, int dist)
{
	var arrow = CreateObject(TutorialArrow, x, y, NO_OWNER);
	arrow.MeshTransformation = Trans_Scale(3600);
	if (!arrow) 
	{
		return;
	}
	// Change arrow category to C4D_Gui.
	arrow->SetCategory(C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax);
	arrow.Plane = 2500;
	// Display bouncing arrow, corrected for arrow size.
	dist += ARROW_INCREASE_DIST;
	x -= Sin(angle, dist);
	y += Cos(angle, dist);
	arrow->SetAction("Show");
	arrow->SetPosition(x, y);
	arrow->SetR(angle);
	return arrow;
}

/** Creates an arrow to indicate the target.
* @param target GUI object which should be indicated by the arrow.
* @param angle angle at which the arrow should be drawn, standard \c 135 degrees.
* @param dist distance of the arrow to the target object, automatically corrects for GUI object's size.
* @return the arrow created.
*/
global func TutArrowShowGUITarget(object target, int angle, int dist)
{
	if (!target)
	{
		return;	
	}
	var arrow = CreateObject(TutorialArrow, target->GetX(), target->GetY(), NO_OWNER);
	arrow.MeshTransformation = Trans_Scale(3600);
	if (!arrow)
	{
		return;
	}
	// Change arrow category to C4D_Gui and set correct plane.
	arrow->SetCategory(C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax);
	arrow.Plane = target.Plane;
	// Display spinning arrow, corrected for GUI and arrow size.
	angle = angle ?? ARROW_DEFAULT_ANGLE;
	dist = dist ?? ARROW_DEFAULT_DIST;
	dist += ARROW_INCREASE_DIST;
	dist += target->GetID()->GetDefHeight() / 2;
	arrow->SetAction("Attach", target);
	arrow->SetR(angle);
	arrow->SetVertex(0, VTX_Y, -dist, VTX_SetPermanentUpd);
	return arrow;
}

/*-- Proplist --*/

local Name = "$Name$";
local MeshTransformation = [1400, 0, 0, 0, 0, 1400, 0, 0, 0, 0, 1400, 0];
local Parallaxity = [0, 0];
local Plane = 375;
local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		Length = 20,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 32,
		Hgt = 32,
		NextAction = "Attach",
		Animation = "Spin",
	},
	Show = {
		Prototype = Action,
		Name = "Show",
		Procedure = DFA_FLOAT,
		Length = 20,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 32,
		Hgt = 32,
		NextAction = "Show",
		Animation = "Bounce",
	},
};
