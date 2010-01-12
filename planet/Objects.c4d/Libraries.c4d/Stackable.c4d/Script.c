local count;

public func IsStackable() { return true; }
public func GetStackCount() { return Max(1,count); }
public func MaxStackCount() { return 20; }

public func Stack(object obj)
{
	if(obj->GetID() != GetID())	return 0;
	
	var howmany = Min(obj->GetStackCount(),MaxStackCount()-GetStackCount());
	
	SetStackCount(count+howmany);
	return howmany;
}

public func SetStackCount(int amount)
{
	count = amount;
	UpdatePicture();
	UpdateMass();
}

public func TakeObject()
{
	if(count == 1)
	{
		Exit();
		return this;
	}
	else if(count > 1)
	{
		SetStackCount(count-1);
		return CreateObject(GetID(),0,0,GetOwner());
	}
}

public func UpdatePicture()
{
	var one = GetStackCount()%10;
	var ten = (GetStackCount()/10)%10;
	var hun = (GetStackCount()/100)%10;
	
	if(hun > 0)
	{
		SetGraphics(Format("%d",hun),NUMB,1,GFXOV_MODE_Picture);
		SetObjDrawTransform(400,0,-19000,0,400,+10000, 1);
	}
	if(ten > 0)
	{
		SetGraphics(Format("%d",ten),NUMB,2,GFXOV_MODE_Picture);
		SetObjDrawTransform(400,0,-12000,0,400,+10000, 2);
	}
	SetGraphics(Format("%d",hun),NUMB,3,GFXOV_MODE_Picture);
	SetObjDrawTransform(400,0,-5000,0,400,+10000, 3);
	
}

public func UpdateMass()
{
	SetMass(GetID()->GetMass()*Max(GetStackCount(),1));
}

protected func RejectEntrance(object into)
{
	return TryPutInto(into);
}

/* Value */

public func CalcValue(object pInBase, int iForPlayer)
{
  // Je nach Anzahl
  return(GetID()->GetValue()*Max(GetStackCount(),1));
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
		var howmany = 0;
		if(!content) continue;
		if(content->~IsStackable())
			if(content->GetID() == GetID())
				if(howmany = content->Stack(this))
				{
					count -= howmany;
					if(count <= 0)
					{
						RemoveObject();
						return true;
					}
				}
	}
	
	UpdatePicture();
	UpdateMass();
	
	return false;
}
