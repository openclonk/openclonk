// Maximum pipe length depends on difficulty.

#appendto PipeLine

public func Definition(proplist def)
{
	def.PipeMaxLength = def.PipeMaxLength * (12 - 2 * SCENPAR_Difficulty) / 10;
	return;
}
