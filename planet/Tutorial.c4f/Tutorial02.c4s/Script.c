/*-- 
		Tutorial 2
		Author: Maikel

		In this tutorial the player will be familiarized with crew selection, backpack control and some tools.
--*/

static chest_1;	// Contains firestones to be shot with the catapult, and dynamite to blast through rock.
static chest_2; // Contains some firestones and other stuff.
static cp_clonk1;
static cp_clonk2;
static cp_grapple;
static cp_finish;

protected func Initialize()
{
	var chest;
	// Flint chest.
	chest = CreateObject(Chest, 340, 420, NO_OWNER);
	chest->CreateContents(Firestone);
	chest->CreateContents(Firestone);
	chest->CreateContents(Firestone);
	chest->CreateContents(DynamiteBox);
	
	// Rope ladder chest.
	chest = CreateObject(Chest, 655, 202, NO_OWNER);
	chest->CreateContents(Ropeladder);
	chest->CreateContents(Ropeladder);
	chest->CreateContents(Ropeladder);
	chest->CreateContents(Firestone);
	chest->CreateContents(Firestone);
	
	// Checkpoints.
	var goal = FindObject(Find_ID(Goal_Tutorial));
	if (!goal) //{ GameOver(); FatalError("Goal missing - no definitions loaded?"); }
		goal = CreateObject(Goal_Tutorial);
	cp_clonk1 = goal->AddCheckpoint(150, 358);
	cp_clonk2 = goal->AddCheckpoint(560, 120);
	cp_grapple = goal->AddCheckpoint(820, 390, "Grapple");
	cp_grapple->SetBaseGraphics(GrappleBow);
	
	cp_finish = goal->SetFinishpoint(1450, 390);
	goal->DisableDirectionHelp();
	
	// Scriptcounter.
	ScriptGo(true);
	return;
}

public func Checkpoint_Grapple(int plr, object cp)
{
	// Clonk only needs grapple bows.
	var clonk = FindObject(Find_Owner(plr), Find_OCF(OCF_CrewMember), Sort_Distance(cp->GetX(), cp->GetY()));
	if (clonk)
	{
		for (var content in FindObjects(Find_Container(clonk)))
			content->RemoveObject();
		clonk->CreateContents(GrappleBow);
		clonk->CreateContents(GrappleBow);
	}
	return;
}

func PlrHasSpawned(int plr, object clonk, object cp)
{
	if (!clonk) return;
	var shovel = clonk->FindContents(Shovel);
	if (shovel) 
		shovel->RemoveObject();
	if (cp == cp_grapple)
	{
		clonk->CreateContents(GrappleBow);
		clonk->CreateContents(GrappleBow);
	}
	return true;
}

protected func InitializePlayer(int plr)
{
	// First clonk.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(150, 358);
	// Give shovel and grapple bow.
	if (clonk->Contents())
		clonk->Contents()->RemoveObject();
	// Second clonk.
	var clonk = GetCrew(plr, 1);
	clonk->SetPosition(565, 110);
	// Give grapple bow and flints, catapult later.
	if (clonk->Contents())
		clonk->Contents()->RemoveObject();
}



/*-- Messages --*/

// Intro: Clonk selection
func Script1()
{
	TutMsg("@$MsgTutWelcome$");
}

func Script20()
{
	TutMsg("@$MsgTutClonks$");
}

func Script40()
{
	TutMsg("@$MsgTutSelection$");
}

// Part two: backpack control
func Script60()
{
	if (GetCursor(0) == GetCrew(0, 1))
		goto(59);
	else
		TutMsg("@$MsgTut04$");
}

func Script80()
{
	TutMsg("@$MsgTut05$");
}

func Script100()
{
	TutMsg("@$MsgTut06$");
}

func Script120()
{
	TutMsg("@$MsgTut07$");
}

// Part Y: Ropeladder
func Script140()
{
	// Check rock free.
	if (GetPathLength(440, 110, 520, 110) == 0)
		goto(139);
	else
		TutMsg("@$MsgTutRockBlasted$");
}

func Script160()
{
	TutMsg("@$MsgTutSelectOther$");
}

func Script180()
{
	if (GetCursor(0) == GetCrew(0, 0))
		goto(179);
	else
		TutMsg("@$MsgTutSelectedOther$");
}

func Script200()
{
	TutMsg("@$MsgTutRopeladder$");
	TutArrowShowPos(655, 200);
	TutArrowShowPos(440, 140);
}

func Script220()
{
	if (!FindObject(Find_Distance(20, 440, 140), Find_OCF(OCF_CrewMember)))
		goto(219);
	else
	{
		TutMsg("@$MsgTutUnfold$");
		TutArrowClear();
		TutArrowClear();
	}
}

// Part XX: Dynamite Box

func Script240()
{
	if (!FindObject(Find_Func("IsLadder"), Find_Distance(40, 425, 185)))
		goto(239);
	else
	{
		TutMsg("@$MsgTutUpwards$");
		TutArrowShowPos(600, 295);
	}
}

func Script260()
{
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(20, 600, 295)))
		if (FindObject(Find_ID(DynamiteBox), Find_Container(clonk)))
		{
			TutMsg("@$MsgTutBroughtDyna$");
			TutArrowClear();
			return;
		}
	goto(259);
}

func Script280()
{
	TutMsg("@$MsgTutPlaceDyna$");
	TutArrowShowPos(640, 320);
}

func Script300()
{
	if (ObjectCount(Find_ID(Dynamite), Find_Distance(40, 640, 320)) >= 5)
	{
		TutMsg("@$MsgTutDetonate$");
		TutArrowClear();	
		return;
	}
	if (GetPathLength(620, 290, 680, 350) > 0)
	{
		goto(319);
		TutArrowClear();
	}
	goto(299);
}

func Script320()
{
	if (GetPathLength(620, 290, 680, 350) == 0)
		goto(319);

	TutMsg("@$MsgTutSmokeCleared$");
	TutArrowShowPos(820, 390);
}

// Part Z: Grapple Bow

func Script340()
{
	if (!FindObject(Find_Distance(20, 820, 390), Find_OCF(OCF_CrewMember)))
		goto(339);
	else
		TutMsg("@$MsgTutGrappleBows$");
}

func Script360()
{
	TutMsg("@$MsgTutSwingAcross$");
}

