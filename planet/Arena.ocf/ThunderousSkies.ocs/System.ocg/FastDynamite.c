/* Everyone wins! */

#appendto Dynamite

local fast;

func Initialize()
{
	fast=0;
	_inherited();
}

func MakeFast(int f) { fast=f; }

func FuseTime() { return 140 - fast; }
