/**
	Diamond Socket
	Contains a valuable diamond that can be freed using a pickaxe.
*/

local Name = "$Name$";
local Description = "$Description$";

local attached_mesh;
local last_free;
local mat_color;

public func Place(int amount, proplist area, proplist settings)
{
	area = area ?? Shape->LandscapeRectangle();
	var diamond_sockets = [];
	var failsafe = amount * 100;
	while ((amount > 0) && (--failsafe > 0))
	{
		// select cluster
		var c_size = Min(RandomX(4, 6), amount);
		if (settings != nil && settings.cluster_size != nil)
			c_size = Min(settings.cluster_size, amount);
		
		// look for random in-earth position
		var failsafe2 = 500;
		var pt = {};
		while (--failsafe2)
		{
			if (!area->GetRandomPoint(pt)) break;
			
			if (!GBackSolid(pt.x, pt.y)) continue;
			
			// must be diggable
			var mat = GetMaterial(pt.x, pt.y);
			if (!GetMaterialVal("DigFree","Material",mat))
				continue;
			break;
		}
		
		if (failsafe2 <= 0) continue;
		
		// color of cluster
		var hsl = HSL(Random(255), 50, 150);
		var r2 = (hsl >> 16) & 0xff;
		var g2 = (hsl >> 8) & 0xff;
		var b2 = (hsl >> 0) & 0xff;
		
		// we have a position now!
		failsafe2 = 50;
		var i = 0;
		while ((i < c_size) && (--failsafe2 > 0))
		{
			var mx = pt.x + RandomX(-20, 20);
			var my = pt.y + RandomX(-20, 20);
			if (!GBackSolid(mx, my)) continue;
			var mat = GetMaterial(mx, my);
			if (!GetMaterialVal("DigFree","Material",mat)) continue;
			if (FindObject(Find_Distance(8, mx, my), Find_ID(this))) continue;
			
			++i;
			var socket = CreateObject(this, mx, my, NO_OWNER);
			socket->Set(r2, g2, b2);
			PushBack(diamond_sockets, socket);
		}
		
		amount -= i;
	}
	return diamond_sockets;
}

public func Construction()
{
	var tex = GetTexture(0, 0);
	if (tex)
	{
		mat_color = GetAverageTextureColor(tex);
	}
	else
	{
		mat_color = RGB(100, 50, 5);
	}
	
	last_free = false;
	
	var contents = CreateContents(Diamond);
	contents->SetObjectBlitMode(GFX_BLIT_Mod2);
	this.MeshTransformation = Trans_Rotate(90, 1, 0, 0);
	attached_mesh = AttachMesh(contents, "main", "main", Trans_Mul(Trans_Rotate(Random(360), 1, 0, 0)));
	
	SetR(Random(360));
	Hide();
	AddTimer("CheckFree", 100 + Random(10));
	
	// Random color for now. The Place() function will create groups of the same color.
	Set(Random(255), Random(255), Random(255));
}

public func Set(int r, int g, int b)
{
	var contents = Contents(0);
	if (contents)
		contents->SetClrModulation(RGB(r, g, b), 0);
}

public func DugOut(object clonk)
{
	// For instant appearance
	return CheckFree(true);
}

private func CheckFree(bool by_dig_free)
{	
	if (GBackSolid() == last_free)
	{
		last_free = !last_free;
		if (!last_free)
		{
			if (!Contents()) return RemoveObject();
			Hide();
		}
		else
		{
			// Look what's appearing behind the dust!
			DugOutDust();
			// One shiny burst because MuchSparkle is a bit thin sometims
			var particles = 
			{
				Size = PV_Step(5, 5),
				Stretch = PV_Step(2),
				BlitMode = GFX_BLIT_Additive,
				Alpha = PV_Linear(255, 0),
				Rotation = Random(360)
			};
			CreateParticle("StarSpark", 0, 0, 0, 0, 10, particles);
			// And some extra sparkling in the future
			AddEffect("MuchSparkle", this, 1, 1, this);
			Show();
			// Also, some sound (delayed for audibility on visibility change)
			if (by_dig_free) ScheduleCall(this, Global.Sound, 1, 1, "Objects::DiamondDigOut");
		}
	}
	
	if (!Random(20))
		Sparkle();
	if (last_free && !Random(3))
		Sparkle();
	return true;
}

private func FxMuchSparkleTimer(target, fx, time)
{
	if (time > Random(90)) return -1;
	if (Random(3)) return;
	Sparkle();
}

private func Sparkle()
{
	var r = Random(360);
	var particles = 
	{
		Size = PV_Step(1, 1, 0),
		Stretch = PV_Step(2),
		BlitMode = GFX_BLIT_Additive,
		Alpha = PV_Linear(255, 0),
		Rotation = r
	};
	var x = RandomX(-3, 3);
	var y = RandomX(-3, 3);
	CreateParticle("StarSpark", x, y, 0, 0, 10 + Random(20), particles);
	CreateParticle("StarSpark", x, y, 0, 0, 10 + Random(20), {Prototype = particles, Rotation = r + 90});
}

private func DugOutDust()
{
	var particles =
	{
		Prototype = Particles_Colored(Particles_Dust(), mat_color),
		ForceY = PV_Gravity(100),
		DampingX = 750, DampingY = 750,
		Size = PV_KeyFrames(0, 0, 2, 100, 10, 1000, 4),
		Alpha = PV_KeyFrames(0, 0, 0, 100, 100, 1000, 0),
		OnCollision = PC_Die()
	};

	CreateParticle("Dust", 0, 0, PV_Random(-40, 40), PV_Random(-40, 40), PV_Random(10, 60), particles, 40 + Random(20));
}

public func OnHitByPickaxe()
{
	var c = Contents();
	if (!c) return;
	if (Random(3)) return;
	
	Sound("Hits::Materials::Rock::RockHit*");
	DetachMesh(attached_mesh);
	c->Exit(0, 0, 0, RandomX(-3, 3), RandomX(-3, -1), RandomX(-20, 20));
}

public func CanBeHitByPickaxe()
{
	return !!Contents();
}


/* Hidden socket state: Still show (but semi-transparent) in editor */

private func Hide()
{
	this.Visibility = VIS_Editor;
	SetClrModulation(0x80ffffff);
}

private func Show()
{
	this.Visibility = VIS_All;
	SetClrModulation(0xffffffff);
}


/* Scenario saving */

public func SaveScenarioObject(props, ...)
{
	if (!inherited(props, ...)) return false;
	// Ignore some properties set in construction
	props->Remove("Visibility");
	props->Remove("ClrModulation");
	props->Remove("R");
	// Special saving of empty sockets
	if (!Contents())
	{
		props->AddCall("Diamond", this, "RemoveDiamond");
	}
	return true;
}

public func RemoveDiamond()
{
	// Remove any contained diamond (for scenario saving of empty sockets)
	var diamond = FindContents(Diamond);
	if (diamond) return diamond->RemoveObject();
}
