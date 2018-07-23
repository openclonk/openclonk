/*--
	Crystal communicator
	Author: Sven2

	Shining structure built from gems and metal
--*/

#include Library_Structure

local top_face, base_face;

public func IsCrystalCommunicator() { return !base_face; }

/* Construction */

public func SetConstructionSiteOverlay(object site, int dir, object stick, object component_obj)
{
	// Play component-specific sound for adding stuff to the site
	if (component_obj && !component_obj->GetDefFragile()) component_obj->~Hit();
	// Construction site graphics by provided metal
	var metal_completion = site->ContentsCount(Metal) * 3 / Max(GetComponent(Metal, nil), 1);
	site->SetGraphics(["Site0", "Site1", "Site2", nil][metal_completion], CrystalCommunicator, 1, GFXOV_MODE_Base);
	site->SetGraphics(nil, nil, 2);
	site->SetObjDrawTransform(1000, 0,0,0, 1000, site->GetObjHeight()/2 * -1000, 1);
	// Add graphics of contained gems
	UpdateGemOverlays(site, [1, 3, 7, 12][metal_completion]);
	return true;
}

public func DoConstructionEffects(object site)
{
	// Grab all gems from site
	GrabContents(site);
	var metal;
	while (metal = FindContents(Metal)) metal->RemoveObject();
	// Site is done immediately
	SetCon(100);
	// Create TopFace overlay
	top_face = CreateObjectAbove(GetID(),0,35,GetOwner());
	top_face.Plane = this.Plane + 10;
	top_face->SetGraphics("TopFace");
	top_face->SetAction("Attach", this);
	top_face.base_face = this;
	// Transfer gem overlays
	this.gem_overlays = site.gem_overlays;
	UpdateGemOverlays(this, 13, true);
	// Construction done. Remove site.
	site->RemoveObject(site);
	// Start finals effect
	return true;
}


/* Gem overlays */

static const CrystalCommunicator_GemsX = [ 15,440,336,221,121,298,220, 50, 14,333,129, 77],
             CrystalCommunicator_GemsY = [255, 75,100, 84, 44,107, 15,130,107,153,149,106],
             CrystalCommunicator_GemsZ = [  5,  3,  4,  0,  4,  0,  5,  2,  3,  1,  1,  4],
             CrystalCommunicator_GemCount = 12;

private func UpdateGemOverlays(object obj, int max_overlays, bool refresh_existing)
{
	// Add overlays for any gems that have not yet been added
	var gem_overlay_index = 3;
	if (!obj.gem_overlays) obj.gem_overlays = [];
	var n_overlays = GetLength(obj.gem_overlays);
	var i;
	// Remove overlays of gems that have left
	for (i=0; i<n_overlays; ++i)
		if (!obj.gem_overlays[i] || obj.gem_overlays[i]->Contained() != obj)
		{
			obj->SetGraphics(nil, nil, gem_overlay_index+i);
			if (obj.top_face) obj.top_face->SetGraphics(nil, nil, gem_overlay_index+i);
			obj.gem_overlays[i] = nil;
		}
	// Add new overlays
	for (var gem in FindObjects(Find_Container(obj), Find_Func("GetGemColor")))
	{
		// Already displayed?
		i = GetIndexOf(obj.gem_overlays, gem);
		if (i>=0)
		{
			if (!refresh_existing) continue;
		}
		else
		{
			// Find a spot for this gem
			i = GetIndexOf(obj.gem_overlays, nil);
			if (i<0) i=n_overlays;
			// No free space?
			if (i == max_overlays) if (refresh_existing) continue; else break;
		}
		// Add overlay
		var gem_gfx = gem.graphics_index;
		if (gem_gfx) gem_gfx = Format("%d", gem_gfx+1); else gem_gfx = nil;
		var x = CrystalCommunicator_GemsX[i];
		var y = CrystalCommunicator_GemsY[i];
		var z = CrystalCommunicator_GemsZ[i];
		var size = z*100+500;
		var off_y;
		if (obj == this) off_y = 35000; else off_y = 70000;
		var gem_target;
		if (obj.top_face && z>=3) gem_target = obj.top_face; else gem_target = obj;
		gem_target->SetGraphics(gem_gfx, gem->GetID(), gem_overlay_index+i, GFXOV_MODE_Base);
		gem_target->SetObjDrawTransform(size,0,x*200-45000, 0,size,y*200-off_y, gem_overlay_index+i);
		if (z<3) gem_target->SetClrModulation(0xffb0b0b0, gem_overlay_index+i);
		// Remember in list
		obj.gem_overlays[i] = gem;
		n_overlays = GetLength(obj.gem_overlays);
	}
	// Make sure a glitter effect is there
	if (!GetEffect("IntGemGlitter", obj)) AddEffect("IntGemGlitter", obj, 1, 12, nil, CrystalCommunicator);
	return true;
}

private func FxIntGemGlitterTimer(target)
{
	// Glitter at random gem position
	if (Random(2))
	{
		var i = Random(12);
		var gem = target.gem_overlays[i];
		if (gem)
		{
			var x = CrystalCommunicator_GemsX[i]/5 - 45;
			var y = CrystalCommunicator_GemsY[i]/5 - 35;
			if (target->GetID() == ConstructionSite) y -= 35;
			var size = CrystalCommunicator_GemsZ[i]*4 + 10;
			var sparkle_fx = GetEffect("Sparkle", gem);
			sparkle_fx.Size = PV_KeyFrames(1, 0, 0, 500, size, 1000, 0); // modifying value directly in gem, assuming gem won't leave this structure any more
			if (sparkle_fx && sparkle_fx.particles)
				target->CreateParticle("MagicRing", x, y, 0, 0, size, sparkle_fx.particles, 1);
		}
	}
}



/* End sequence */

local ruby_particle, amethyst_particle, beam_particles, gem_particles;
local flash_particle, small_flash_particle, large_flash_particle;
local time;
local send_code, next_send_time, send_code_pos;

public func StartCommunication()
{
	// forward to main object
	if (base_face) return base_face->StartCommunication();
	// Init particles
	// Gem particles
	beam_particles = CreateArray(CrystalCommunicator_GemCount);
	gem_particles = CreateArray(CrystalCommunicator_GemCount);
	ruby_particle = new Particles_MagicRing() { R = 0xff, G = 0x00, B = 0x30 };
	amethyst_particle = new Particles_MagicRing() { R = 0xa0, G = 0x00, B = 0xff };
	for (var i=0; i<CrystalCommunicator_GemCount; ++i)
	{
		var base;
		if (this.gem_overlays && this.gem_overlays[i])
		{
			if (this.gem_overlays[i]->GetID() == Ruby) base = ruby_particle; else base = amethyst_particle;
		}
		else
		{
			if (i%2) base = ruby_particle; else base = amethyst_particle;
		}
		gem_particles[i] = base;
		var x = CrystalCommunicator_GemsX[i]/5 - 45;
		beam_particles[i] = CreateCirclingParticle(base, 100, 20, Abs(x), x>0);
	}
	// Central flash particles
	flash_particle = {
		Size = PV_KeyFrames(0, 0, 0, 500, 60, 1000, 0),
		R = PV_KeyFrames(0, 750, 255, 1000, 0),
		G = PV_KeyFrames(0, 300, 255, 1000, 0),
		B = PV_KeyFrames(0, 300, 255, 500, 0),
		Rotation = PV_Random(0, 360),
		Alpha = PV_KeyFrames(0, 0, 255, 750, 100, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Attach = ATTACH_Front | ATTACH_MoveRelative,
	};
	large_flash_particle = {
		Size = PV_KeyFrames(0, 0, 0, 500, 200, 1000, 0),
		R = PV_KeyFrames(0, 750, 255, 1000, 0),
		G = PV_KeyFrames(0, 300, 255, 1000, 0),
		B = PV_KeyFrames(0, 300, 255, 500, 0),
		Rotation = PV_Random(0, 360),
		Alpha = PV_KeyFrames(0, 0, 255, 750, 100, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Attach = ATTACH_Front | ATTACH_MoveRelative,
	};
	// Gem flash particle
	small_flash_particle = {
		Size = PV_KeyFrames(0, 0, 0, 500, 20, 1000, 0),
		R = PV_KeyFrames(0, 750, 255, 1000, 0),
		G = PV_KeyFrames(0, 300, 255, 1000, 0),
		B = PV_KeyFrames(0, 300, 255, 500, 0),
		Rotation = PV_Random(0, 360),
		Alpha = PV_KeyFrames(0, 0, 255, 750, 100, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Attach = ATTACH_Front | ATTACH_MoveRelative,
	};
	// Run effects
	Sound("CrystalCommunicator::CrystalCommCharge");
	time = 0;
	AddTimer(this.PreActivity, 5);
}

private func PreActivity()
{
	// Warmup effects
	var i,x,y,z;
	for (i=0; i<CrystalCommunicator_GemCount; ++i)
	{
		x = CrystalCommunicator_GemsX[i]/5 - 45;
		y = CrystalCommunicator_GemsY[i]/5 - 35;
		z = CrystalCommunicator_GemsZ[i];
		var gem_target;
		if (top_face && z>=3) gem_target = top_face; else gem_target = this;
		if (time < 20 || !Random(3))
		{
			if (!(time % 5)) gem_target->CreateParticle("StarFlash", x,y, 0,0, 60+Random(10), small_flash_particle, 1);
		}
		else
			gem_target->CreateParticle("StarFlash", x, y, -x, -y, 10, small_flash_particle, 10);
	}
	if (time == 20) Sound("CrystalCommunicator::CrystalCommBoost");
	if (time > 50)
	{
		RemoveTimer(this.PreActivity);
		time = 0;
		CreateParticle("StarFlash", PV_Random(-12, +12), PV_Random(-12, +12), PV_Random(-10, +10),PV_Random(-10, +10), PV_Random(20, 100), large_flash_particle, 10);
		Sound("CrystalCommunicator::CrystalCommWumm");
		SetAction("Active");
		AddTimer(this.Activity, 1);
	}
	++time;
}

public func StopCommunication()
{
	// forward to main object
	if (base_face) return base_face->StopCommunication();
	// Stop activities
	RemoveTimer(this.PreActivity);
	RemoveTimer(this.Activity);
	SetAction("Idle");
	time = 0;
	return true;
}

private func Activity()
{
	// Send codes
	if (send_code)
	{
		if (next_send_time == time)
		{
			var send_char = GetChar(send_code, send_code_pos++);
			if (!send_char)
			{
				// All sent.
				send_code = nil;
			}
			else
			{
				// Next char to send
				if (send_char == GetChar("."))
				{
					Sound("CrystalCommunicator::CrystalCommToneA");
					next_send_time = time + 13;
				}
				else
				{
					Sound("CrystalCommunicator::CrystalCommToneB");
					next_send_time = time + 27;
				}
			}
		}
		if (next_send_time - time > 10)
		{
			// During tone: Effects
			CreateParticle("StarFlash", PV_Random(-3, +3), 0, 0,-30, 500, flash_particle, 1);
		}
	}
	// Effects
	// Circulate through gems
	var i = time % CrystalCommunicator_GemCount;
	var x = CrystalCommunicator_GemsX[i]/5 - 45, y = CrystalCommunicator_GemsY[i]/5 - 35, z = CrystalCommunicator_GemsZ[i];
	var gem_target;
	if (top_face && z>=3) gem_target = top_face; else gem_target = this;
	// Create ring moving upwards
	if (Abs(x) > 5) CreateParticle("MagicRing", x, y, 0, -Min(time/20,10), 2000, beam_particles[i], 1);
	// Create flash at gem
	gem_target->CreateParticle("StarFlash", x, y, 0, 0, 20+Random(10), gem_particles[i], 1);
	// Create central flash
	if (!(time % 5)) CreateParticle("StarFlash", PV_Random(-6, +6), PV_Random(-6, +6), 0,0, 20+Random(10), flash_particle, 1);
	++time;
}

private func CreateCirclingParticle(proplist prototype, int frames_per_cycle, int num_cycles, int rad, bool start_backmove)
{
	var a = (rad * 10000000) / (2429 * frames_per_cycle * frames_per_cycle);
	var ang0 = (!!start_backmove) * 180;
	var particle = {
		Prototype = prototype,
		Size =   PV_Sin(PV_Linear( ang0,              360*num_cycles),5,8),
		ForceX = PV_Sin(PV_Linear( ang0+90,   ang0+90+360*num_cycles), a, 0),
		ForceY = 0,
		Attach = ATTACH_Front | ATTACH_MoveRelative,
	};
	return particle;
	
}

public func SendCode(string code)
{
	send_code = code;
	next_send_time = time;
	send_code_pos = 0;
	return true;
}


/* Definition data */

public func Definition(proplist def)
{
}

local ActMap = {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 1,
			FacetBase = 1,
			Length = 1,
			Delay = 0,
			NextAction = "Hold"
		},
		Active = {
			Prototype = Action,
			Name = "Active",
			Procedure = DFA_NONE,
			Directions = 1,
			FacetBase = 1,
			Delay = 0,
			Length = 1,
			NextAction = "Active",
			Sound = "CrystalCommunicator::CrystalCommActive",
		},
	};

local Collectible = false;
local Name = "$Name$";
local Description = "$Description$";
local Touchable = 0;
local Plane = 280;
local Components = {Ruby = 6, Amethyst = 6, Metal = 6};
