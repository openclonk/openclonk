global func Particles_Planks()
{
	return
	{
		Alpha = PV_KeyFrames(0, 0, 255, 900, 255, 1000, 0),
		Size = PV_Random(20, 30),
		Phase = PV_Random(0, 2),
		Rotation = PV_Speed(),
		ForceX = PV_Wind(50),
		ForceY = PV_Gravity(100),
		CollisionVertex = 500,
		OnCollision = PC_Die(),
	};
}