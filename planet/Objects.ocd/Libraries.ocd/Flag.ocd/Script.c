local Name = "$Name$";
local Description = "$Description$";

static const LibraryFlag_standard_radius = 200;
static LibraryFlag_flag_list;

local lflag;

global func GetOwnerOfPosition(int x, int y)
{
	if(GetType(LibraryFlag_flag_list) != C4V_Array) return NO_OWNER;
	
	var oldest = nil, oldest_time = 0;
	
	for(var flag in LibraryFlag_flag_list)
	{
		var d = Distance(GetX() + x, GetY() + y, flag->GetX(), flag->GetY());
		if(d > flag->GetFlagRadius()) continue; 
		
		if(oldest == nil || flag->GetFlagConstructionTime() < oldest_time)
		{
			oldest = flag;
			oldest_time = flag->GetFlagConstructionTime();
		}
	}
	
	if(oldest == nil) return NO_OWNER;
	return oldest->GetOwner();
}

func RedrawFlagRadius()
{
	//ClearFlagMarkers();
	
	//var flags = FindObjects(Find_ID(FlagPole),Find_Exclude(target), Find_Not(Find_Owner(GetOwner())), /*Find_Distance(FLAG_DISTANCE*2 + 10,0,0)*/Sort_Func("GetLifeTime"));
	var other_flags = [];
	var i = 0;
	for(var f in LibraryFlag_flag_list)
	{
		//if(f->GetOwner() == GetOwner()) continue;
		if(f == this) continue;
		other_flags[i++] = f;
	}
	// inner border
	var count = Max(5, lflag.radius / 10);
	var marker_index = -1;
	for(var i=0; i<360; i+= 360 / count)
	{
		++marker_index;
		var draw=true;
		var f=nil;
		var x= Sin(i, lflag.radius);
		var y=-Cos(i, lflag.radius);	
		//var inEnemy = false;
		
		for(var f in other_flags)
		{
			if(Distance(GetX()+x,GetY()+y,f->GetX(),f->GetY()) <= f->GetFlagRadius())
			{
				if(f->GetFlagConstructionTime() == GetFlagConstructionTime())
					lflag.construction_time += 1;
					
				if(f->GetFlagConstructionTime() < GetFlagConstructionTime())
				{	
					draw=false;
				}
				//else inEnemy=true;
				if(!Hostile(GetOwner(), f->GetOwner()))
					draw = false;
			}
		}
		
		// check allied flags
		// don't draw inner border
		/*
		if(draw && GetLength(other_flags))
		{
			var own_flags = [];
	
			for(var f in LibraryFlag_flag_list)
			{
				if(f->GetOwner() != GetOwner()) continue;
				if(f == this) continue;
				own_flags[i++] = f;
			}
			
			// sort own flags for age - insert sort okay here since probably 99% sorted already
			for(var fi = 1; fi < GetLength(own_flags); ++fi)
			{
				var ft = own_flags[fi];
				var t = ft->GetFlagConstructionTime();
				for(var ci = fi-1; ci >= 0; ++ci)
				{
					if(own_flags[ci]->GetFlagConstructionTime() >= t) continue;
					
					// right position?
					if(ci == fi-1) break;
					
					// move to the right
					for(var mi = ci+1;mi<fi;++mi)
						own_flags[mi+1] = own_flags[mi];
					own_flags[ci] = ft;
					break;
				}
			}
			
			//own_flags = FindObjects(Find_ID(FlagPole),Find_Exclude(target), Find_Owner(GetOwner()), Find_Distance(FLAG_DISTANCE ,x,y),Sort_Func("GetLifeTime"));	
			if(GetLength(own_flags))
			{
				draw=false;
				for(var f in other_flags)
				{
					for(var e in own_flags)
					{
						if(Distance(GetX()+x,GetY()+y,f->GetX(),f->GetY())<(FLAG_DISTANCE))
						{
							if(GetLifeTime()> f->GetLifeTime() && e->GetLifeTime() < f->GetLifeTime())
							draw=true;
						}		
					}
				}
			}
		} //if(draw)
		*/
		if(!draw)
		{
			if(marker_index < GetLength(lflag.range_markers))
				if(lflag.range_markers[marker_index])
				{
					lflag.range_markers[marker_index]->FadeOut();
					lflag.range_markers[marker_index]->MoveTo(GetX(), GetY(), -lflag.range_markers[marker_index]->GetR());
				}
			continue;
		}
		var marker = lflag.range_markers[marker_index];
		if(!marker)
		{
			marker = CreateObject(GetFlagMarkerID(), 0, 0, GetOwner());
			marker->SetR(Angle(0, 0, x, y));
		}
		marker->FadeIn();
		marker->MoveTo(GetX() + x, GetY() + y, Angle(0, 0, x, y));
		lflag.range_markers[marker_index] = marker;
	}
	
	// uter Border
	/*
	for(var i=0; i<10; i++)
	{
		SetLength(outers,0);
		var f=nil;
		draw=true;
		var x= Sin(i*36 - time, lflag.radius);
		var y=-Cos(i*36 - time, lflag.radius);	
		if(flags)
			for(var f in flags)
			{
				if(Distance(GetX()+x,GetY()+y,f->GetX(),f->GetY())<FLAG_DISTANCE)
				{
					if(f->GetLifeTime() < target->GetLifeTime())
					{	
						outers[GetLength(outers)] = f;	
					}
					else
						draw=false;
				}
			}
		if(outers)
		{
			var newf=nil;
			for(var f in outers)
			{
				var d=0;
				if(f->GetLifeTime()>d)
				{
					d=f->GetLifeTime();
					newf=f;
				}
			}
		}
		if(!newf) continue;
		var fl=nil;
		fl = FindObjects(Find_ID(FlagPole),Find_Exclude(target), Find_Owner(GetOwner()), Find_Distance(FLAG_DISTANCE ,x,y),Sort_Func("GetLifeTime"));
		if(fl[0] && draw && flags)
		{
			draw=false;
			for(var f in flags)
			{
				for(var e in fl)
				{
					if(Distance(GetX()+x,GetY()+y,f->GetX(),f->GetY())<(FLAG_DISTANCE))
					{
						if(GetLifeTime()> f->GetLifeTime() && e->GetLifeTime() < f->GetLifeTime())
						draw=true;
					}		
				}
			}
		}
		if(!draw)	
			continue;
		CreateParticle("Magic",Sin(i*36 - time, FLAG_DISTANCE+1),-Cos(i*36 - time, FLAG_DISTANCE+1),0,0,10+Random(4),newf.color);
	}*/
}

func RefreshOwnershipOfSurrounding()
{
	for(var obj in FindObjects(Find_Distance(lflag.radius), Find_Func("CanBeOwned")))
	{
		var o = GetOwnerOfPosition(AbsX(obj->GetX()), AbsY(obj->GetY()));
		if(obj->GetOwner() == o) continue;
		var old = obj->GetOwner();
		obj->SetOwner(o);
		obj->~OnOwnerChange(old);
	}
}

public func Construction()
{
	if(GetType(LibraryFlag_flag_list) != C4V_Array)
		LibraryFlag_flag_list = [];
	
	if(GetIndexOf(this, LibraryFlag_flag_list) == -1)
		LibraryFlag_flag_list[GetLength(LibraryFlag_flag_list)] = this;
	
	lflag =
	{
		construction_time = FrameCounter(),
		radius = LibraryFlag_standard_radius,
		range_markers = []
	};
	
	// redraw
	RedrawAllFlagRadiuses();
	
	// ownership
	RefreshOwnershipOfSurrounding();
	
	return _inherited(...);
}

public func Destruction()
{
	ClearFlagMarkers();
	
	// remove from global array
	for(var i = 0; i < GetLength(LibraryFlag_flag_list); ++i)
	{
		if(LibraryFlag_flag_list[i] != this) continue;
		LibraryFlag_flag_list[i] = LibraryFlag_flag_list[GetLength(LibraryFlag_flag_list)-1];
		SetLength(LibraryFlag_flag_list, GetLength(LibraryFlag_flag_list)-1);
		break;
	}
	
	// redraw
	RedrawAllFlagRadiuses();
	
	// ownership
	RefreshOwnershipOfSurrounding();
	
	return _inherited(...);
}

private func ClearFlagMarkers()
{
	for(var obj in lflag.range_markers)
		obj->RemoveObject();
	lflag.range_markers = [];
}

public func SetFlagRadius(int to)
{
	lflag.radius = to;
	
	// redraw
	RedrawAllFlagRadiuses();
	
	// ownership
	RefreshOwnershipOfSurrounding();
	
	return true;
}

global func RedrawAllFlagRadiuses()
{
	for(var f in LibraryFlag_flag_list)
		f->RedrawFlagRadius();
}

public func GetFlagRadius(){return lflag.radius;}
public func GetFlagConstructionTime() {return lflag.construction_time;}
public func GetFlagMarkerID(){return LibraryFlag_Marker;}
