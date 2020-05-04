/**
	Creation.c
	Extension of System.ocg/Creation.c
*/

global func PlaceObjectBatches(array item_ids, int n_per_batch, int batch_radius, int n_batches, string in_material)
{
	// place a number (n_batches) of batches of objects of types item_ids. Each batch has n_per_batch objects.
	// fewer batches and/or objects may be placed if no space is found
	in_material = in_material ?? "Earth";
	var n_item_ids = GetLength(item_ids), n_created = 0;
	for (var i = 0; i<n_batches; ++i)
	{
		var loc = FindLocation(Loc_Material(in_material));
		if (loc)
		{
			for (var j = 0; j<n_per_batch; ++j)
			{
				var loc2 = FindLocation(Loc_InRect(loc.x-batch_radius, loc.y-batch_radius, batch_radius*2, batch_radius*2), Loc_Material(in_material));
				if (loc2)
				{
					var obj = CreateObjectAbove(item_ids[Random(n_item_ids)], loc2.x, loc2.y);
					if (obj)
					{
						obj->SetPosition(loc2.x, loc2.y);
						++n_created;
					}
				}
			}
		}
	}
	return n_created;
}

