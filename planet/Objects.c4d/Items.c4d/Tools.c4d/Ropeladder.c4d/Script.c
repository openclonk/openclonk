/*
	Ropeladder
	Author: Randrian

	A ladder consisting of ropesegments. The physics is done completly intern with point masses.
	Verlet integration and simple stick constraints are used for the rope physics.
	The segments are an extra object, which handels the graphics and the detection of the ladder by the clonk.
	Strings are drawn by the engine, which is a bit buggy cause the engine enforces it's own line handling.
*/

local particles;

static const MaxParticles = 15;

static Iterations = 3;

local segments;

local TestArray;

static const Precision = 100;

protected func Initialize()
{Iterations = 10;
TestArray = [[0, 1], [1, 0], [1, 1], [0, 2], [1, 2], [2, 0], [2, 1], [2, 2], [0, 3], [1, 3], [2, 3], [3, 0], [3, 1], [3, 2], [0, 4], [1, 4], [2, 4], [3, 3], [4, 0], [4, 1], [4, 2], [0, 5], [1, 5], [2, 5], [3, 4], [3, 5], [4, 3], [4, 4], [5, 0], [5, 1], [5, 2], [5, 3], [0, 6], [1, 6], [2, 6], [3, 6], [4, 5], [5, 4], [6, 0], [6, 1], [6, 2], [6, 3], [0, 7], [1, 7], [2, 7], [3, 7], [4, 6], [5, 5], [5, 6], [6, 4], [6, 5], [7, 0], [7, 1], [7, 2], [7, 3], [0, 8], [1, 8], [2, 8], [3, 8], [4, 7], [4, 8], [5, 7], [6, 6], [7, 4], [7, 5], [8, 0], [8, 1], [8, 2], [8, 3], [8, 4], [0, 9], [1, 9], [2, 9], [3, 9], [4, 9], [5, 8], [6, 7], [7, 6], [7, 7], [8, 5], [9, 0], [9, 1], [9, 2], [9, 3], [9, 4]];

	segments = CreateArray(MaxParticles);
	for(var i = 0; i < MaxParticles; i++)
	{
		segments[i] = CreateObject(Ropeladder_Segment);
		segments[i]->SetMaster(this, i);
		if(i > 0)
		{
			segments[i]->SetNextLadder(segments[i-1]);
			segments[i-1]->SetPreviousLadder(segments[i]);
		}
	}
//  SetAction("Connect", segments[0], segments[GetLength(segments)-1]);
  SetVertexXY(0, GetX(), GetY());
  SetVertexXY(1, GetX(), GetY());
	particles = CreateArray(MaxParticles);
	for(var i = 0; i < MaxParticles; i++)
		particles[i] = [[ (GetX()+i*1)*Precision, GetY()*Precision], [ (GetX()+i*1)*Precision, GetY()*Precision], [0,1*Precision], 1]; // Pos, Oldpos, acceleration (gravity), mass
	particles[0][2] = [0,0];
	particles[0][3] = 0;
	SetProperty("LineColors", [RGB(100,60)]);
	SetPosition(GetX(), GetY()-10);
//	Message("@!", this);
	Verlet(1);
}

// Verlet integration step
func Verlet(fFirst)
{
	var fTimeStep = 1;
  for(var i = 1; i < MaxParticles; i++)
	{
    var x = particles[i][0];
    var temp = x;
    var oldx = particles[i][1];
    var a = particles[i][2];

		var object_speed = [0,0];
		if(!fFirst && i)
		{
			object_speed[0] = segments[i]->GetX()-GetPartX(i);
			object_speed[1] = segments[i]->GetY()-GetPartY(i);
//			if(object_speed[0]+object_speed[1] != 0)
//				Log("%v", object_speed);
		}
		particles[i][0][0] += x[0]-oldx[0]+a[0]*fTimeStep*fTimeStep;//+object_speed[0]*Precision;
		particles[i][0][1] += x[1]-oldx[1]+a[1]*fTimeStep*fTimeStep;//+object_speed[1]*Precision;
		particles[i][1] = temp;
  }
}

func UpdateLines()
{
	var fTimeStep = 1;
  for(var i=0; i < MaxParticles; i++)
	{
		if(i >= GetVertexNum()) AddVertex();

		SetVertexXY(i, GetPartX(i)-2, GetPartY(i)+1);

		segments[i]->SetPosition(GetPartX(i), GetPartY(i));
  }
	for(var i=0; i < MaxParticles; i++)
	{
		if(i+MaxParticles >= GetVertexNum()) AddVertex();
		SetVertexXY(i+MaxParticles, GetPartX(MaxParticles-i-1)+2, GetPartY(MaxParticles-i-1)-1);
  }
	while(GetVertexNum()>MaxParticles*2) RemoveVertex(GetVertexNum()-1);
}
/*
GetCursor()->CreateObject(Test_Rope, 40, -100)
*/

func LogSpeed()
{
	var array = [];
	for(var i=0; i < MaxParticles; i++)
	{
		var x = particles[i][0];
    var oldx = particles[i][1];
		array[GetLength(array)] = Distance(x[0]-oldx[0], x[1]-oldx[1]);
	}
	Log("%v", array);
}

func GetPartX(index) { return (particles[index][0][0]+Precision/2)/Precision; }
func GetPartY(index) { return (particles[index][0][1]+Precision/2)/Precision; }

public func OnLadderGrab(clonk, index)
{
	if(index == 0) return;
	particles[index][0][0] += BoundBy(clonk->GetXDir(), -50, 50)*Precision;
}

public func OnLadderClimb(clonk, index)
{
	if(index > 2 && index < MaxParticles-3)
	{
		particles[index-2][0][0] -= 1*Precision*(-1+2*clonk->GetDir());
		particles[index+2][0][0] += 1*Precision*(-1+2*clonk->GetDir());
	}
	else if(index > 2 && index < MaxParticles-2)
	{
		particles[index-2][0][0] -= 1*Precision*(-1+2*clonk->GetDir());
		particles[index+1][0][0] += 1*Precision*(-1+2*clonk->GetDir());
	}
//	particles[index][0][1] += BoundBy(clonk->GetYDir(), -50, 50)*Precision;
}

public func GetLadderData(index, &startx, &starty, &endx, &endy, &angle)
{
	startx = GetPartX(index);
	starty = GetPartY(index);
	if(index == 0)
	{
		endx = startx;
		endy = starty-5;
		angle = Angle(GetPartX(2), GetPartY(2), GetPartX(0), GetPartY(0));
		return true;
	}
	if(index == MaxParticles-1)
	{
		angle = Angle(GetPartX(MaxParticles-1), GetPartY(MaxParticles-1), GetPartX(MaxParticles-3), GetPartY(MaxParticles-3));
	}
	else
		angle = Angle(GetPartX(index+1), GetPartY(index+1), GetPartX(index-1), GetPartY(index-1));
	endx = GetPartX(index-1);
	endy = GetPartY(index-1);
	return true;
}

// This function should accumulate forces for each particle
func AccumulateForces()
{
// All particles are influenced by gravity
//   for(var i=0; i < MaxParticles; i++) particles[i][2] = [0,1];//m_vGravity;
}
// Here constraints should be satisfied
func SatisfyConstraints() {
	for(var j=0; j < Iterations; j++)
	{
  for(var i=0; i < MaxParticles-1; i++)
	{
		// Keep length
		var restlength = 5*Precision;
    var x1 = particles[i][0];
    var x2 = particles[i+1][0];
		var invmass1 = particles[i][3];
		var invmass2 = particles[i+1][3];
    var delta = Vec_Sub(x2,x1);
    var deltalength = Sqrt(Vec_Dot(delta,delta));
    var diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
    particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
    particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
	}
/*	for(var i=0; i < MaxParticles-1; i++)
	{
		// Keep length
		var restlength = 5*Precision;
    var x1 = particles[i][0];
    var x2 = particles[i+1][0];
		var invmass1 = 0;//particles[i][3];
		var invmass2 = 1;//particles[i+1][3];
    var delta = Vec_Sub(x2,x1);
    var deltalength = Sqrt(Vec_Dot(delta,delta));
    var diff = (deltalength-restlength)*1000/(deltalength*(invmass1+invmass2));
    particles[i][0]   = Vec_Add(x1, Vec_Div(Vec_Mul(delta, invmass1*diff), 1000));
    particles[i+1][0] = Vec_Sub(x2, Vec_Div(Vec_Mul(delta, invmass2*diff), 1000));
	}*/
	for(var i=0; i < MaxParticles; i++)
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
			//Log("%d %d %d", i, xdir, ydir);
			for(var pos in TestArray)
			{
				if(!GBackSolid(GetPartX(i)-GetX()+xdir*pos[0], GetPartY(i)-GetY()+ydir*pos[1]))
				{//Log("Move %d %d", xdir*pos[0], ydir*pos[1]);
					particles[i][0][0] = (GetPartX(i)+xdir*pos[0])*Precision;
					particles[i][0][1] = (GetPartY(i)+ydir*pos[1])*Precision;
					//Log("%d", GBackSolid(GetPartX(i)-GetX(), GetPartY(i)-GetY()));
					break;
				}
			}
		}
	}
	}

}

func LogArray()
{var array = [];
	for(var dist = 1; dist < 10; dist++)
			for(var x = 0; x < 10; x++)
			for(var y = 0; y < 10; y++)
			{
				if(Distance(0,0,x,y) != dist) continue;//Log("%d %d %d < %d", x, y, Distance(0,0,x,y), dist);
				array[GetLength(array)] = [x,y];
			}
	Log("%v", array);
}


func TimeStep() {
   AccumulateForces();
   Verlet();
	 SatisfyConstraints();
	 UpdateLines();
}

func Vec_Sub(array x, array y) { return [x[0]-y[0], x[1]-y[1]]; }
func Vec_Add(array x, array y) { return [x[0]+y[0], x[1]+y[1]]; }
func Vec_Mul(array x, int   i) { return [x[0]*i,	  x[1]*i];    }
func Vec_Div(array x, int   i) { return [x[0]/i,	  x[1]/i];    }
func Vec_Dot(array x, array y) { return x[0]*y[0]+x[1]*y[1];    }

func Definition(def) {
  SetProperty("ActMap", {
Connect = {
Prototype = Action,
Name = "Connect",
Length = 0,
Delay = 0,
Procedure = DFA_CONNECT,
NextAction = "Connect",
},  }, def);
  SetProperty("Name", "$Name$", def);
}