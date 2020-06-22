/**
	Library_Inventory.c
	Global functions that belong to Libraries.ocd/Inventory.ocd.
	
	@author
*/

// overload function for objects with Inventory.ocd
// documented in /docs/sdk/script/fn
global func ShiftContents(bool shift_back, id target_id)
{
	if (this && (this.HandObjects > 0))
	{
		// special handling for only one hand: just move the hand to next item
		// always move hand 0
		
		// move to target ID?
		if (target_id)
		{
			for (var pos = 0; pos < this.MaxContentsCount; ++pos)
			{
				var obj = this.inventory.objects[pos];
				if (!obj) continue;
				if (obj->GetID() == target_id)
				{
					this->SetHandItemPos(0, pos);
					return true;
				}
			}
			return false;
		}
		// otherwise, move in direction
		var move_dir = 1;
		if (shift_back) move_dir = -1;
		var current_pos = this->GetHandItemPos(0);
		for (var i = this.MaxContentsCount; i > 0; --i)
		{
			current_pos += move_dir;
			if (current_pos < 0) current_pos = this.MaxContentsCount + current_pos;
			else current_pos = current_pos % this.MaxContentsCount;
			
			// is there an object at the slot?
			if (!this.inventory.objects[current_pos]) continue;
			this->SetHandItemPos(0, current_pos);
			return true;
		}
		return false;
	}
	return _inherited(shift_back, target_id, ...);
}
