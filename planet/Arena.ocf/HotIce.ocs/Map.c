/**
	Hot Ice
	Ice islands above a lava lake
	
	@authors Sven2
*/

// Called be the engine: draw the complete map here.
public func InitializeMap(proplist map)
{
	// Map type 0: One big island; more small islands above
	// Map type 1: Only many small islands
	var t = SCENPAR_MapType;
	var w = map.Wdt, h=map.Hgt;
	
	// Bottom lava lake
	map->Draw("^DuroLava", nil, [0,h*4/5,w,h/5]);
	
	// Big island
	if (t == 0)
	{
  	var island = { Algo=MAPALGO_Polygon, X=[0,w,w*6/8,w*2/8], Y=[h*4/10,h*4/10,h*7/10,h*7/10] };
  	island = { Algo=MAPALGO_Turbulence, Op=island, Amplitude=[0, 8] };
  	map->Draw("^Ice-ice2", island, [w/10,h*13/20,w*8/10,h*3/20]); 
  }
  
  // Small islands
  var n_islands = [12,37][t];
  while(n_islands--)
  {
    var y = h*2/10 + Random(h*(3+t*2)/10);
    var x = w*1/10 + Random(w*8/10);
    var szx = t*Random(3);
    var szy = 1+t*Random(Random(2));
    map->Draw("^Ice-ice2", nil, [x-szx,y,1+2*szx,szy]);
  }
  
  // Alternate texctures
  var icealt_tex = { Algo=MAPALGO_RndChecker, Wdt=2, Hgt=3 };
  icealt_tex = { Algo=MAPALGO_Turbulence, Op=icealt_tex };
  icealt_tex = { Algo=MAPALGO_And, Op=[Duplicate("Ice"), icealt_tex]};
  map->Draw("^Ice-ice3", icealt_tex);

	// Return true to tell the engine a map has been successfully created.
	return true;
}
