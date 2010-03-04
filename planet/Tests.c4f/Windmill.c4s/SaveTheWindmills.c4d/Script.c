/*-- Save the windmills --*/

#include Library_Goal

local score;

func Initialize()
{
	score = 0;
}

public func IsFulfilled()
{
	return false;
}

public func Activate(int byplr)
{
	MessageWindow(GetDesc(), byplr);
	return;
}

public func IncShotScore()
{
	score++;
	NotifyHUD();
}

public func GetShortDescription(int plr)
{
	return Format("$ShotScore$",score);
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}