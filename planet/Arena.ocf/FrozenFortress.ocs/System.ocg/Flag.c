/*--
	Flag Mesh
	Author: Mimmo_O
		
	Special flag graphics.
--*/


#appendto Goal_Flag

public func SetTeam(int to_team)
{
	if (to_team == 1) 
		SetMeshMaterial("FlagLeft");
	else if (to_team == 2)
		SetMeshMaterial("FlagRight");
	return _inherited(to_team);
}
