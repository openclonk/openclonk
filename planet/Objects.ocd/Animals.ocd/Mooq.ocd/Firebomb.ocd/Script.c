/*-- Firebomb --*/

static const FIREBOMB_DAMAGE = 70;

func Initialize() {
    SetAction("Fly");
}

func Hit() {
    var dmg = FIREBOMB_DAMAGE;

    BlastObjects(GetX(), GetY(), dmg/2, nil, GetOwner() + 1);
    BlastObjects(GetX(), GetY(), dmg/2, nil, GetOwner() + 1);

    var blast = {
        R = 255,
        G = 255,
        B = 255,
        Size = (dmg*13)/20, // /10
        Phase = PV_Linear(0, 21),
        DampingX = 1000,
        DampingY = 1000,
        BlitMode = GFX_BLIT_Additive
    };

    CreateParticle("Blast", 0, 0, 0, 0, 21, blast, 1);


    Call("ExplosionEffect", dmg/2, 0, 0, 0);

    RemoveObject();
}

func Sparkle() {
    if (Contained())
        return false;

    var clusterflight = {
        R = 200 + Random(55),
        G = 200 + Random(55),
        B = 200 + Random(55),
        Alpha = PV_Linear(255, 0), //AlphaFade = 4
        Size = 16, //40
        Phase = PV_Linear(0, 9),
        Rotation = PV_Random(360),
        DampingX = 1000,
        DampingY = 1000,
        //Attach = ATTACH_MoveRelative
    };

    CreateParticle("Clusterflight", 0, 0, RandomX(-2, 2),RandomX(-2, 2), 36, clusterflight, 1);
}


local BorderBound = C4D_Border_Sides;
local Plane = 400;

/* Act Map */

local ActMap = {
Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = nil,
    Length = 1,
    Delay = 1,
    NextAction = "Fly",
    FacetBase = 1,
    PhaseCall = "Sparkle",
},
};
