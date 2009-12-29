/*
	Standard rank definition
	Author: Newton

	This is a ressource object in which the rank graphics and the rank names
	are stored. Any (crew) object can freely define which rank graphics it
	wants to use by defining the function func RanksID() { return [myrankid] }.
	Any other ranks definition should include this object
*/

global func GetRankName(int rank)
{
	var idRank;

	// extra rank definition
	if(this)
		idRank = this->~RanksID();
	if(!idRank)
		idRank = RANK;
		
	return DefinitionCall(idRank,"RankName",rank);
}

public func RankName(int rank, int extra)
{
	if(rank < 0) return nil;
	var rrank = rank % RegularRankCount();
	var erank = (rank / RegularRankCount() % ExtraRankCount());

	// here you see the limits of C4Script
	// r=Format("$Rank%d",rrank); is not possible
	var r,e;
	if(rrank==0) r="$Rank0$";
	else if(rrank==1) r="$Rank1$";
	else if(rrank==2) r="$Rank2$";
	else if(rrank==3) r="$Rank3$";
	else if(rrank==4) r="$Rank4$";
	else if(rrank==5) r="$Rank5$";
	else if(rrank==6) r="$Rank6$";
	else if(rrank==7) r="$Rank7$";
	else if(rrank==8) r="$Rank8$";
	else if(rrank==9) r="$Rank9$";
	else if(rrank==10) r="$Rank10$";
	else if(rrank==11) r="$Rank11$";
	else if(rrank==12) r="$Rank12$";
	else if(rrank==13) r="$Rank13$";
	else if(rrank==14) r="$Rank14$";
	else if(rrank==15) r="$Rank15$";
	else if(rrank==16) r="$Rank16$";
	else if(rrank==17) r="$Rank17$";
	else if(rrank==18) r="$Rank18$";
	else if(rrank==19) r="$Rank19$";
	else if(rrank==20) r="$Rank20$";
	else if(rrank==21) r="$Rank21$";
	else if(rrank==22) r="$Rank22$";
	else if(rrank==23) r="$Rank23$";
	else if(rrank==24) r="$Rank24$";
	
	if(erank==1) e = Format("$RankExtra1$",r);
	else if(erank==2) e = Format("$RankExtra2$",r);
	else if(erank==3) e = Format("$RankExtra3$",r);
	else if(erank==4) e = Format("$RankExtra4$",r);
	
	if(erank) return e;
	else return r;
}

public func RegularRankCount() { return 25; }
public func ExtraRankCount() { return 4; }

func Definition(def) {
  def["ActMap"] = {

    Rank0       = {  Prototype = Action,  Name = "Rank0",       X = 0, Y = 0, Wdt = 16, Hgt = 16 },
    Rank1       = {  Prototype = Action,  Name = "Rank1",       X = 16, Y = 0, Wdt = 16, Hgt = 16 },
    Rank2       = {  Prototype = Action,  Name = "Rank2",       X = 32, Y = 0, Wdt = 16, Hgt = 16 },
    Rank3       = {  Prototype = Action,  Name = "Rank3",       X = 48, Y = 0, Wdt = 16, Hgt = 16 },
    Rank4       = {  Prototype = Action,  Name = "Rank4",       X = 64, Y = 0, Wdt = 16, Hgt = 16 },
    Rank5       = {  Prototype = Action,  Name = "Rank5",       X = 80, Y = 0, Wdt = 16, Hgt = 16 },
    Rank6       = {  Prototype = Action,  Name = "Rank6",       X = 96, Y = 0, Wdt = 16, Hgt = 16 },
    Rank7       = {  Prototype = Action,  Name = "Rank7",       X = 112, Y = 0, Wdt = 16, Hgt = 16 },
    Rank8       = {  Prototype = Action,  Name = "Rank8",       X = 128, Y = 0, Wdt = 16, Hgt = 16 },
    Rank9       = {  Prototype = Action,  Name = "Rank9",       X = 144, Y = 0, Wdt = 16, Hgt = 16 },
    Rank10      = {  Prototype = Action,  Name = "Rank10",      X = 160, Y = 0, Wdt = 16, Hgt = 16 },
    Rank11      = {  Prototype = Action,  Name = "Rank11",      X = 176, Y = 0, Wdt = 16, Hgt = 16 },
    Rank12      = {  Prototype = Action,  Name = "Rank12",      X = 192, Y = 0, Wdt = 16, Hgt = 16 },
    Rank13      = {  Prototype = Action,  Name = "Rank13",      X = 208, Y = 0, Wdt = 16, Hgt = 16 },
    Rank14      = {  Prototype = Action,  Name = "Rank14",      X = 224, Y = 0, Wdt = 16, Hgt = 16 },
    Rank15      = {  Prototype = Action,  Name = "Rank15",      X = 240, Y = 0, Wdt = 16, Hgt = 16 },
    Rank16      = {  Prototype = Action,  Name = "Rank16",      X = 256, Y = 0, Wdt = 16, Hgt = 16 },
    Rank17      = {  Prototype = Action,  Name = "Rank17",      X = 272, Y = 0, Wdt = 16, Hgt = 16 },
    Rank18      = {  Prototype = Action,  Name = "Rank18",      X = 288, Y = 0, Wdt = 16, Hgt = 16 },
    Rank19      = {  Prototype = Action,  Name = "Rank19",      X = 304, Y = 0, Wdt = 16, Hgt = 16 },
    Rank20      = {  Prototype = Action,  Name = "Rank20",      X = 320, Y = 0, Wdt = 16, Hgt = 16 },
    Rank21      = {  Prototype = Action,  Name = "Rank21",      X = 336, Y = 0, Wdt = 16, Hgt = 16 },
    Rank22      = {  Prototype = Action,  Name = "Rank22",      X = 352, Y = 0, Wdt = 16, Hgt = 16 },
    Rank23      = {  Prototype = Action,  Name = "Rank23",      X = 368, Y = 0, Wdt = 16, Hgt = 16 },
    Rank24      = {  Prototype = Action,  Name = "Rank24",      X = 384, Y = 0, Wdt = 16, Hgt = 16 },
    RankExtra1  = {  Prototype = Action,  Name = "RankExtra1",  X = 400, Y = 0, Wdt = 16, Hgt = 16 },
    RankExtra2  = {  Prototype = Action,  Name = "RankExtra2",  X = 416, Y = 0, Wdt = 16, Hgt = 16 },
    RankExtra3  = {  Prototype = Action,  Name = "RankExtra3",  X = 432, Y = 0, Wdt = 16, Hgt = 16 },
    RankExtra4  = {  Prototype = Action,  Name = "RankExtra4",  X = 448, Y = 0, Wdt = 16, Hgt = 16 }

  };
}