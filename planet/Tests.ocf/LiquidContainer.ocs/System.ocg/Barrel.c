#appendto Barrel

func Test3_IsLiquidContainerForMaterial(string liquid)
{
	return liquid == "Water"
	    || liquid == "Lava"
	    || liquid == "123"
	    || liquid == "#24942fwijvri";
}
