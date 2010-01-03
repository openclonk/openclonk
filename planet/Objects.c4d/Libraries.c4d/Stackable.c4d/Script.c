
public func IsStackable() { return true; }

public func StackCount() { return 1; }

public func Stack(object obj) { }

public func TakeObject() { return this; }

// TODO: notify hud

public func UpdatePicture()
{
	var one = StackCount()%10;
	var ten = (StackCount()/10)%10;
	var hun = (StackCount()/100)%10;
	
	if(hun > 0)
	{
		SetGraphics(Format("%d",hun),NUMB,1,GFXOV_MODE_Picture);
		SetObjDrawTransform(250,0,0,0,250,0,0,1);
	}
	if(ten > 0)
	{
		SetGraphics(Format("%d",ten),NUMB,2,GFXOV_MODE_Picture);
		SetObjDrawTransform(250,0,0,0,250,0,0,2);
	}
	SetGraphics(Format("%d",hun),NUMB,3,GFXOV_MODE_Picture);
	SetObjDrawTransform(250,0,0,0,250,0,0,3);
	
}

protected func RejectEntrance(object into)
{
	return TryPutInto(into);
}

private func TryPutInto( object into )
{
	var contents = FindObjects(Find_Container(into));

	// first check if stackable can be put into object with extrea slot
	for(var content in contents)
	{
		if(!content) continue;
		if(content->~HasExtraSlot())
			if(TryPutInto(content))
				return true;
	}
	// then check this object
	for(var content in contents)
	{
		if(!content) continue;
		if(content->~IsStackable())
			if(content->~GetID() == GetID())
				if(content->Stack(this))
				{
					RemoveObject();
					return true;
				}
	}
	return false;
}