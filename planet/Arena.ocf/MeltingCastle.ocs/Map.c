/*
	Melting Castle
	Team melee to melt each others castles
	
	@authors Sven2
*/

// Only half of the actual map is present in the map file and Objects.c.
// Create a mirror for the missing half here.
public func InitializeMap(proplist map)
{
	// Not when editing
	if (EDIT_MAP) return true;
	// Mirror map
	var old_map = Duplicate();
	Resize(this.Wdt*2, this.Hgt);
	Blit(old_map);
	Blit({Algo = MAPALGO_Scale, OffX = old_map.Wdt, X=-100, Op = old_map});
	return true;
}
