/*
	Ropeladder
	Author: Randrian

	A ladder consisting of ropesegments. The physics is done completly intern with point masses.
	Verlet integration and simple stick constraints are used for the rope physics.
	The segments are an extra object, which handels the graphics and the detection of the ladder by the clonk.
*/

static const Ladder_MaxParticles = 15;
static const Ladder_Iterations = 10;
static const Ladder_Precision = 100;

local particles;
local segments;
local TestArray;

public func ControlUse(object clonk, int x, int y)
{
	Exit(0, 10);
	// Unroll dir
	var dir = -1;
	if(x > 0) dir = 1;
	Unroll(dir);

	return true;
}

protected func Unroll(dir)
{
	SetCategory(C4D_StaticBack);
	SetAction("Hanging");
	SetProperty("Collectible", 0);

//	TestArray = [[0,1],[0,2],[0,3],[0,4],[0,5],[0,6],[0,7],[0,8],[0,9]];
	TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	segments = CreateArray(Ladder_MaxParticles);
	for(var i = Ladder_MaxParticles-1; i >= 0; i--)
	{
		segments[i] = CreateObject(Ropeladder_Segment);
		if(i == 0) segments[i]->SetGraphics("NoRope");
	}
	for(var i = 0; i < Ladder_MaxParticles; i++)
	{
		segments[i]->SetMaster(this, i);
		if(i > 0)
		{
			segments[i]->SetNextLadder(segments[i-1]);
			segments[i-1]->SetPreviousLadder(segments[i]);
		}
	}
	particles = CreateArray(Ladder_MaxParticles);
	for(var i = 0; i < Ladder_MaxParticles; i++)
		particles[i] = [[ (GetX()+i*dir)*Ladder_Precision, GetY()*Ladder_Precision], [ (GetX()+i*1)*Ladder_Precision, GetY()*Ladder_Precision], [0,1*Ladder_Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass
	particles[0][2] = [0,0];
	particles[0][3] = 0;
	SetPosition(GetX(), GetY()-10);
	Verlet(1);

	AddEffect("IntHang", this, 1, 1, this);
}

func FxIntHangTimer() { TimeStep(); }

// Verlet integration step
func Verlet(fFirst)
{
	var fTimeStep = 1;
  for(var i = 1; i < Ladder_MaxParticles; i++)
	{
    var x = particles[i][0];
    var temp = x;
    var oldx = particles[i][1];
    var a = particles[i][2];

		// Verlet step, get speed out of distance moved relativ to the last position
		particles[i][0][0] += x[0]-oldx[0]+a[0]*fTimeStep*fTimeStep;
		particles[i][0][1] += x[1]-oldx[1]+a[1]*fTimeStep*fTimeStep;
		particles[i][1] = temp;
  }
}

func UpdateLines()
{
	var fTimeStep = 1;
  for(var i=0; i < Ladder_MaxParticles; i++)
	{
		segments[i]->SetPosition(GetPartX(i), GetPartY(i));

		var angle;
		if(i >= 1)
			angle = Angle(particles[i][0][0], particles[i][0][1], particles[i-1][0][0], particles[i-1][0][1]);
		SetSegmentTransform(segments[i], -angle, particles[i][0][0]*10-GetPartX(i)*1000,particles[i][0][1]*10-GetPartY(i)*1000 );
//		segments[i]->SetR(angle);
  }
}

func SetSegmentTransform(obj, int r, int xoff, int yoff) {
  var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
  // set matrix values
  obj->SetObjDrawTransform (
    +fcos, +fsin, xoff, //(1000-fcos)*xoff - fsin*yoff,
    -fsin, +fcos, yoff, //(1000-fcos)*yoff + fsin*xoff,
  );
}

func LogSpeed()
{
	// Helperfunction for Debugpurpose
	var array = [];
	for(var i=0; i < Ladder_MaxParticles; i++)
	{
		var x = particles[i][0];
    var oldx = particles[i][1];
		array[GetLength(array)] = Distance(x[0]-oldx[0], x[1]-oldx[1]);
	}
	Log("%v", array);
}

func GetPartX(index) { return (particles[index][0][0]+Ladder_Precision/2)/Ladder_Precision; }
func GetPartY(index) { return (particles[index][0][1]+Ladder_Precision/2)/Ladder_Precision; }

public func OnLadderGrab(clonk, index)
{
	// Do some speed when the clonk jumps on the ladder
	if(index == 0) return;
	particles[index][0][0] += BoundBy(clonk->GetXDir(), -50, 50)*Ladder_Precision;
}

public func OnLadderClimb(clonk, index)
{
	// The clonk drags on the upper segments and pushes on the lower ones
	if(index > 2 && index < Ladder_MaxParticles-3)
	{
		particles[index-2][0][0] -= 1*Ladder_Precision*(-1+2*clonk->GetDir());
		particles[index+2][0][0] += 1*Ladder_Precision*(-1+2*clonk->GetDir());
	}
	else if(index > 2 && index < Ladder_MaxParticles-2)
	{
		particles[index-2][0][0] -= 1*Ladder_Precision*(-1+2*clonk->GetDir());
		particles[index+1][0][0] += 1*Ladder_Precision*(-1+2*clonk->GetDir());
	}
}

public func GetLadderData(index, &startx, &starty, &endx, &endy, &angle)
{
	startx = particles[index][0][0]*10;
	starty = particles[index][0][1]*10;
	if(index == 0)
	{
		endx = startx;
		endy = starty-5000;
		angle = Angle(particles[2][0][0], particles[2][0][1], particles[0][0][0], particles[0][0][1]);
		return true;
	}
	if(index == Ladder_MaxParticles-1)
	{
		angle = Angle(particles[index][0][0], particles[index][0][1], particles[index-2][0][0], particles[index-2][0][1]);
	}
	else
		angle = Angle(particles[index+1][0][0], particles[index+1][0][1], particles[index-1][0][0], particles[index-1][0][1]);
	endx = particles[index-1][0][0]*10;
	endy = particles[index-1][0][1]*10;
	return true;
}

func SatisfyConstraints()
{
	for(var j=0; j < Ladder_Iterations; j++)
	{
		// Satisfy all stick constraints (move the particles to fit the length)
		for(var i=0; i < Ladder_MaxParticles-1; i++)
		{
			// Keep length
			var restlength = 5*Ladder_Precision; // normal length between laddersegments
			// Get coordinates and inverse masses
			var x1 = particles[i][0];
			var x2 = particles[i+1][0];
			var invmass1 = particles[i][3];
			var invmass2 = particles[i+1][3];
			// calculate difference
			var delta = Vec_Sub(x2,x1);
			var deltalength = Sqrt(Vec_Dot(delta,delta));
			var diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
			// Set new positions
			particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
			particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
		}
		for(var i=0; i < Ladder_MaxParticles; i++)
		{
			// Don't touch ground
			if(GBackSolid(GetPartX(i)-GetX(), GetPartY(i)-GetY()))
			{
				// Moving left?
				var xdir = -1;
				if(particles[i][0][0] < particles[i][1][0])
					xdir = 1;
				var ydir = -1;
				// Moving up?
				if(particles[i][0][1] < particles[i][1][1])
					ydir = 1;
				// Look for all possible places where the particle could move (from nearest to farest)
//				for(var y = 0; y < 10; y++)
//					for(var x = 0; x < 10; x++)
				for(var pos in TestArray)
				{//var pos = [x,y];
					if(!GBackSolid(GetPartX(i)-GetX()+xdir*pos[0], GetPartY(i)-GetY()+ydir*pos[1]))
					{
						particles[i][0][0] = (GetPartX(i)+xdir*pos[0])*Ladder_Precision;
						particles[i][0][1] = (GetPartY(i)+ydir*pos[1])*Ladder_Precision;
						break;
					}
				}
			}
		}
	}
}

func LogArray()
{
	// Helperfunction which has created "TestArray"
	var array = [];
	for(var dist = 1; dist < 10; dist++)
		for(var x = 0; x < 10; x++)
			for(var y = 0; y < 10; y++)
			{
				if(Distance(0,0,x,y) != dist) continue;
				array[GetLength(array)] = [x,y];
			}
	Log("%v", array);
}

func TimeStep() {
	Verlet();
	SatisfyConstraints();
	UpdateLines();
}

// Some vector math
func Vec_Sub(array x, array y) { return [x[0]-y[0], x[1]-y[1]]; }
func Vec_Add(array x, array y) { return [x[0]+y[0], x[1]+y[1]]; }
func Vec_Mul(array x, int   i) { return [x[0]*i,	  x[1]*i];    }
func Vec_Div(array x, int   i) { return [x[0]/i,	  x[1]/i];    }
func Vec_Dot(array x, array y) { return x[0]*y[0]+x[1]*y[1];    }

func Definition(def) {
	 SetProperty("ActMap", {

Hanging = {
	Prototype = Action,
	Name = "Hanging"
},}, def);
  SetProperty("Name", "$Name$", def);
}