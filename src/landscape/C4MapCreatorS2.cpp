/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
// complex dynamic landscape creator

#include "C4Include.h"
#include "landscape/C4MapCreatorS2.h"

#include "control/C4Record.h"
#include "graphics/CSurface8.h"
#include "landscape/C4Material.h"
#include "landscape/C4Texture.h"
#include "lib/C4Random.h"
#include "script/C4ScriptHost.h"

namespace {
	// node attribute entry for SetField search
	enum C4MCValueType
	{
		C4MCV_None,
		C4MCV_Integer,
		C4MCV_Percent,
		C4MCV_Pixels,
		C4MCV_Material,
		C4MCV_Texture,
		C4MCV_Algorithm,
		C4MCV_Boolean,
		C4MCV_Zoom,
		C4MCV_ScriptFunc
	};

	template<typename T>
	class MemberAdapter {
	public:
		typedef char (T::*OffsetType);

		MemberAdapter(T& object, OffsetType offset)
				: Object(object), Offset(offset)
		{
		}

		template<typename U>
		U& As()
		{
			typedef U (T::*TargetPtrType);
			return Object.*reinterpret_cast<TargetPtrType>(Offset);
		}

	private:
		T& Object;
		OffsetType Offset;
	};

	typedef MemberAdapter<C4MCOverlay>::OffsetType C4MCOverlayOffsetType;

	struct C4MCNodeAttr
	{
		const char* Name; // name of field
		C4MCValueType Type; // type of field
		C4MCOverlayOffsetType Offset; // offset of field in overlay MCOverlay-class
	};

	extern C4MCNodeAttr C4MCOvrlMap[];
}

/* --- C4MCCallbackArray --- */

C4MCCallbackArray::C4MCCallbackArray(C4AulFunc *pSFunc, C4MapCreatorS2 *pMapCreator)
{
	// store fn
	pSF = pSFunc;
	// zero fields
	pMap=nullptr; pNext=nullptr;
	// store and add in map creator
	if ((this->pMapCreator=pMapCreator))
		pMapCreator->CallbackArrays.Add(this);
	// done
}

C4MCCallbackArray::~C4MCCallbackArray()
{
	// clear map, if present
	if (pMap) delete [] pMap;
}

void C4MCCallbackArray::EnablePixel(int32_t iX, int32_t iY)
{
	// array not yet created? then do that now!
	if (!pMap)
	{
		// safety
		if (!pMapCreator) return;
		// get current map size
		C4MCMap *pCurrMap = pMapCreator->pCurrentMap;
		if (!pCurrMap) return;
		iWdt = pCurrMap->Wdt; iHgt = pCurrMap->Hgt;
		// create bitmap
		int32_t iSize=(iWdt*iHgt+7)/8;
		pMap = new BYTE[iSize];
		memset(pMap, 0, iSize);
		// done
	}
	// safety: do not set outside map!
	if (iX<0 || iY<0 || iX>=iWdt || iY>=iHgt) return;
	// set in map
	int32_t iIndex = iX + iY*iWdt;
	pMap[iIndex/8] |= 1<<(iIndex%8);
	// done
}

void C4MCCallbackArray::Execute(int32_t iMapZoom)
{
	// safety
	if (!pSF || !pMap) return;
	// pre-create parset
	C4AulParSet Pars(0, 0, iMapZoom);
	// call all funcs
	int32_t iIndex=iWdt*iHgt;
	while (iIndex--)
		if (pMap[iIndex/8]&(1<<(iIndex%8)))
		{
			// set pars
			Pars[0] = C4VInt((iIndex%iWdt) * iMapZoom - (iMapZoom/2));
			Pars[1] = C4VInt((iIndex/iWdt) * iMapZoom - (iMapZoom/2));
			// call
			pSF->Exec(nullptr, &Pars);
		}
	// done
}



/* --- C4MCCallbackArrayList --- */

void C4MCCallbackArrayList::Add(C4MCCallbackArray *pNewArray)
{
	// add to end
	if (pFirst)
	{
		C4MCCallbackArray *pLast = pFirst;
		while (pLast->pNext) pLast=pLast->pNext;
		pLast->pNext=pNewArray;
	}
	else pFirst=pNewArray;
}

void C4MCCallbackArrayList::Clear()
{
	// remove all arrays
	C4MCCallbackArray *pArray, *pNext=pFirst;
	while ((pArray=pNext))
	{
		pNext=pArray->pNext;
		delete pArray;
	}
	// zero first-field
	pFirst=nullptr;
}

void C4MCCallbackArrayList::Execute(int32_t iMapZoom)
{
	// execute all arrays
	for (C4MCCallbackArray *pArray = pFirst; pArray; pArray=pArray->pNext)
		pArray->Execute(iMapZoom);
}




/* --- C4MCNode --- */

C4MCNode::C4MCNode(C4MCNode *pOwner)
{
	// reg to owner
	Reg2Owner(pOwner);
	// no name
	*Name=0;
}

C4MCNode::C4MCNode(C4MCParser* pParser, C4MCNode *pOwner, C4MCNode &rTemplate, bool fClone)
{
	// Make sure the template is not used recursively within itself
	for(C4MCNode* pParent = pOwner; pParent != nullptr; pParent = pParent->Owner)
		if(pParent == &rTemplate)
			throw C4MCParserErr(pParser, C4MCErr_NoRecTemplate, rTemplate.Name);
	// set owner and stuff
	Reg2Owner(pOwner);
	// copy children from template
	for (C4MCNode *pChild=rTemplate.Child0; pChild; pChild=pChild->Next)
		pChild->clone(pParser, this);
	// no name
	*Name=0;
}

C4MCNode::~C4MCNode()
{
	// clear
	Clear();
	// remove from list
	if (Prev) Prev->Next = Next; else if (Owner) Owner->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Owner) Owner->ChildL = Prev;
}

void C4MCNode::Reg2Owner(C4MCNode *pOwner)
{
	// init list
	Child0=ChildL=nullptr;
	// owner?
	if ((Owner=pOwner))
	{
		// link into it
		if ((Prev = Owner->ChildL))
			Prev->Next = this;
		else
			Owner->Child0 = this;
		Owner->ChildL = this;
		MapCreator=pOwner->MapCreator;
	}
	else
	{
		Prev=nullptr;
		MapCreator=nullptr;
	}
	// we're always last entry
	Next=nullptr;
}

void C4MCNode::Clear()
{
	// delete all children; they'll unreg themselves
	while (Child0) delete Child0;
}

C4MCOverlay *C4MCNode::OwnerOverlay()
{
	for (C4MCNode *pOwnr=Owner; pOwnr; pOwnr=pOwnr->Owner)
		if (C4MCOverlay *pOwnrOvrl=pOwnr->Overlay())
			return pOwnrOvrl;
	// no overlay-owner
	return nullptr;
}

C4MCNode *C4MCNode::GetNodeByName(const char *szName)
{
	// search local list (backwards: last node has highest priority)
	for (C4MCNode *pChild=ChildL; pChild; pChild=pChild->Prev)
		// name match?
		if (SEqual(pChild->Name, szName))
			// yeah, success!
			return pChild;
	// search owner, if present
	if (Owner) return Owner->GetNodeByName(szName);
	// nothing found
	return nullptr;
}

bool C4MCNode::SetField(C4MCParser *pParser, const char *szField, const char *szSVal, int32_t iVal, C4MCTokenType ValType)
{
	// no fields in base class
	return false;
}

int32_t C4MCNode::IntPar(C4MCParser *pParser, const char *szSVal, int32_t iVal, C4MCTokenType ValType)
{
	// check if int32_t
	if (ValType == MCT_INT || ValType == MCT_PERCENT || ValType == MCT_PX)
		return iVal;
	throw C4MCParserErr(pParser, C4MCErr_FieldValInvalid, szSVal);
}

const char *C4MCNode::StrPar(C4MCParser *pParser, const char *szSVal, int32_t iVal, C4MCTokenType ValType)
{
	// check if identifier
	if (ValType != MCT_IDTF)
		throw C4MCParserErr(pParser, C4MCErr_FieldValInvalid, szSVal);
	return szSVal;
}

#define IntPar IntPar(pParser, szSVal, iVal, ValType) // shortcut for checked int32_t param
#define StrPar StrPar(pParser, szSVal, iVal, ValType) // shortcut for checked str param

void C4MCNode::ReEvaluate()
{
	// evaluate ourselves
	Evaluate();
	// evaluate children
	for (C4MCNode *pChild=Child0; pChild; pChild=pChild->Next)
		pChild->ReEvaluate();
}


// overlay

C4MCOverlay::C4MCOverlay(C4MCNode *pOwner) : C4MCNode(pOwner)
{
	// zero members
	X=Y=Wdt=Hgt=OffX=OffY=0;
	Material=MNone;
	*Texture=0;
	Op=MCT_NONE;
	MatClr=0;
	MatClrBkg=0;
	Algorithm=nullptr;
	Sub=false;
	ZoomX=ZoomY=0;
	FixedSeed=Seed=0;
//  Alpha=Beta=0;
	Turbulence=Lambda=Rotate=0;
	Invert=LooseBounds=Group=Mask=false;
	pEvaluateFunc=pDrawFunc=nullptr;
}

C4MCOverlay::C4MCOverlay(C4MCParser* pParser, C4MCNode *pOwner, C4MCOverlay &rTemplate, bool fClone) : C4MCNode(pParser, pOwner, rTemplate, fClone)
{
	// copy fields
	X=rTemplate.X; Y=rTemplate.Y; Wdt=rTemplate.Wdt; Hgt=rTemplate.Hgt;
	RX=rTemplate.RX; RY=rTemplate.RY; RWdt=rTemplate.RWdt; RHgt=rTemplate.RHgt;
	OffX=rTemplate.OffX; OffY=rTemplate.OffY; ROffX=rTemplate.ROffX; ROffY=rTemplate.ROffY;
	Material=rTemplate.Material;
	SCopy(rTemplate.Texture, Texture, C4MaxName);
	Algorithm=rTemplate.Algorithm;
	Sub=rTemplate.Sub;
	ZoomX=rTemplate.ZoomX; ZoomY=rTemplate.ZoomY;
	MatClr=rTemplate.MatClr;
	MatClrBkg=rTemplate.MatClrBkg;
	Seed=rTemplate.Seed;
	Alpha=rTemplate.Alpha; Beta=rTemplate.Beta; Turbulence=rTemplate.Turbulence; Lambda=rTemplate.Lambda;
	Rotate=rTemplate.Rotate;
	Invert=rTemplate.Invert; LooseBounds=rTemplate.LooseBounds; Group=rTemplate.Group; Mask=rTemplate.Mask;
	FixedSeed=rTemplate.FixedSeed;
	pEvaluateFunc=rTemplate.pEvaluateFunc;
	pDrawFunc=rTemplate.pDrawFunc;
	// zero non-template-fields
	if (fClone) Op=rTemplate.Op; else Op=MCT_NONE;
}

void C4MCOverlay::Default()
{
	// default algo
	Algorithm=GetAlgo(C4MC_DefAlgo);
	// no mat (sky) default
	Material=MNone;
	*Texture=0;
	// but if mat is set, assume it sub
	Sub=true;
	// full size
	OffX=OffY=X=Y=0;
	ROffX.Set(0,true); ROffY.Set(0,true); RX.Set(0,true); RY.Set(0,true);
	Wdt=Hgt=C4MC_SizeRes;
	RWdt.Set(C4MC_SizeRes,true); RHgt.Set(C4MC_SizeRes,true);
	// def zoom
	ZoomX=ZoomY=C4MC_ZoomRes;
	// def values
	Alpha.Set(0,false); Beta.Set(0,false); Turbulence=Lambda=Rotate=0; Invert=LooseBounds=Group=Mask=false;
	FixedSeed=0;
	// script funcs
	pEvaluateFunc=pDrawFunc=nullptr;
}

bool C4MCOverlay::SetField(C4MCParser *pParser, const char *szField, const char *szSVal, int32_t iVal, C4MCTokenType ValType)
{
	int32_t iMat; C4MCAlgorithm *pAlgo;
	// inherited fields
	if (C4MCNode::SetField(pParser, szField, szSVal, iVal, ValType)) return true;
	//local fields
	for (C4MCNodeAttr *pAttr=&C4MCOvrlMap[0]; *pAttr->Name; pAttr++)
		if (SEqual(szField, pAttr->Name))
		{
			// field was found, get offset to store in
			MemberAdapter<C4MCOverlay> Target(*this, pAttr->Offset);
			// store according to field type
			switch (pAttr->Type)
			{
			case C4MCV_Integer:
				// simply store
				Target.As<int32_t>() = IntPar;
				break;
			case C4MCV_Percent:
				Target.As<int_bool>().Set(IntPar, ValType == MCT_PERCENT || ValType == MCT_INT);
				break;
			case C4MCV_Pixels:
				Target.As<int_bool>().Set(IntPar, ValType == MCT_PERCENT);
				break;
			case C4MCV_Material:
				// get material by string
				iMat = MapCreator->MatMap->Get(StrPar);
				// check validity
				if (iMat == MNone) throw C4MCParserErr(pParser, C4MCErr_MatNotFound, StrPar);
				// store
				Target.As<int32_t>() = iMat;
				break;
			case C4MCV_Texture:
				// check validity
				if (!MapCreator->TexMap->CheckTexture(StrPar))
					throw C4MCParserErr(pParser, C4MCErr_TexNotFound, StrPar);
				// store
				SCopy(StrPar, Target.As<char [C4M_MaxName + 1]>(), C4M_MaxName);
				break;
			case C4MCV_Algorithm:
				// get algo
				pAlgo=GetAlgo(StrPar);
				// check validity
				if (!pAlgo) throw C4MCParserErr(pParser, C4MCErr_AlgoNotFound, StrPar);
				// store
				Target.As<C4MCAlgorithm *>()=pAlgo;
				break;
			case C4MCV_Boolean:
				// store whether value is not zero
				Target.As<bool>()=IntPar!=0;
				break;
			case C4MCV_Zoom:
				// store calculated zoom
				Target.As<int32_t>()=Clamp<int32_t>(C4MC_ZoomRes-IntPar,1,C4MC_ZoomRes*2);
				break;
			case C4MCV_ScriptFunc:
			{
				// get script func of main script
				C4AulFunc *pSFunc = ::GameScript.ScenPropList._getPropList()->GetFunc(StrPar);
				if (!pSFunc) throw C4MCParserErr(pParser, C4MCErr_SFuncNotFound, StrPar);
				// add to main
				Target.As<C4MCCallbackArray*>() = new C4MCCallbackArray(pSFunc, MapCreator);
			}
			default:
				// TODO
				break;
			}
			// done
			return true;
		}
	// nothing found :(
	return false;
}

C4MCAlgorithm *C4MCOverlay::GetAlgo(const char *szName)
{
	// search map
	for (C4MCAlgorithm *pAlgo = &C4MCAlgoMap[0]; pAlgo->Function; pAlgo++)
		// check name
		if (SEqual(pAlgo->Identifier, szName))
			// success!
			return pAlgo;
	// nothing found
	return nullptr;
}

void C4MCOverlay::Evaluate()
{
	// inherited
	C4MCNode::Evaluate();
	// get mat color
	if (Inside<int32_t>(Material,0,MapCreator->MatMap->Num-1))
	{
		MatClr=MapCreator->TexMap->GetIndexMatTex(MapCreator->MatMap->Map[Material].Name, *Texture ? Texture : nullptr);
		if(MatClr == 0 || !Sub)
			MatClrBkg = 0;
		else
			MatClrBkg = Mat2PixColDefault(MTunnel);
	}
	else
	{
		MatClr=0;
		MatClrBkg=0;
	}

	// calc size
	if (Owner)
	{
		C4MCOverlay *pOwnrOvrl;
		if ((pOwnrOvrl=OwnerOverlay()))
		{
			int32_t iOwnerWdt=pOwnrOvrl->Wdt; int32_t iOwnerHgt=pOwnrOvrl->Hgt;
			X = RX.Evaluate(iOwnerWdt) + pOwnrOvrl->X;
			Y = RY.Evaluate(iOwnerHgt) + pOwnrOvrl->Y;
			Wdt = RWdt.Evaluate(iOwnerWdt);
			Hgt = RHgt.Evaluate(iOwnerHgt);
			OffX = ROffX.Evaluate(iOwnerWdt);
			OffY = ROffY.Evaluate(iOwnerHgt);
		}
	}
	// calc seed
	if (!(Seed=FixedSeed))
	{
		int32_t r1=Random(32768);
		int32_t r2=Random(65536);
		Seed=(r1<<16) | r2;
	}
}

C4MCOverlay *C4MCOverlay::FirstOfChain()
{
	// run backwards until nullptr, non-overlay or overlay without operator is found
	C4MCOverlay *pOvrl=this;
	C4MCOverlay *pPrevO;
	while (pOvrl->Prev)
	{
		if (!(pPrevO=pOvrl->Prev->Overlay())) break;
		if (pPrevO->Op == MCT_NONE) break;
		pOvrl=pPrevO;
	}
	// done
	return pOvrl;
}

bool C4MCOverlay::CheckMask(int32_t iX, int32_t iY)
{
	// bounds match?
	if (!LooseBounds) if (iX<X || iY<Y || iX>=X+Wdt || iY>=Y+Hgt) return false;
	if (Config.General.DebugRec)
	{
		C4RCTrf rc;
		rc.x=iX; rc.y=iY; rc.Rotate=Rotate; rc.Turbulence=Turbulence;
		AddDbgRec(RCT_MCT1, &rc, sizeof(rc));
	}
	C4Real dX=itofix(iX); C4Real dY=itofix(iY);
	// apply turbulence
	if (Turbulence)
	{
		const C4Real Rad2Grad = itofix(3754936, 65536);
		int32_t j=3;
		for (int32_t i=10; i<=Turbulence; i*=10)
		{
			int32_t Seed2; Seed2=Seed;
			for (int32_t l=0; l<Lambda+1; ++l)
			{
				for (C4Real d=itofix(2); d<6; d+=C4REAL10(15))
				{
					dX += Sin(((dX / 7 + itofix(Seed2) / ZoomX + dY) / j + d) * Rad2Grad) * j / 2;
					dY += Cos(((dY / 7 + itofix(Seed2) / ZoomY + dX) / j - d) * Rad2Grad) * j / 2;
				}
				Seed2 = (Seed * (Seed2<<3) + 0x4465) & 0xffff;
			}
			j+=3;
		}
	}
	// apply rotation
	if (Rotate)
	{
		C4Real dXo(dX), dYo(dY);
		dX = dXo*Cos(itofix(Rotate)) - dYo*Sin(itofix(Rotate));
		dY = dYo*Cos(itofix(Rotate)) + dXo*Sin(itofix(Rotate));
	}
	if (Rotate || Turbulence)
		{ iX=fixtoi(dX, ZoomX); iY=fixtoi(dY, ZoomY); }
	else
		{ iX*=ZoomX; iY*=ZoomY; }
	if (Config.General.DebugRec)
	{
		C4RCPos rc2;
		rc2.x=iX; rc2.y=iY;
		AddDbgRec(RCT_MCT2, &rc2, sizeof(rc2));
	}
	// apply offset
	iX-=OffX*ZoomX; iY-=OffY*ZoomY;
	// check bounds, if loose
	if (LooseBounds) if (iX<X*ZoomX || iY<Y*ZoomY || iX>=(X+Wdt)*ZoomX || iY>=(Y+Hgt)*ZoomY) return Invert;
	// query algorithm
	return (Algorithm->Function) (this, iX, iY)^Invert;
}

bool C4MCOverlay::RenderPix(int32_t iX, int32_t iY, BYTE &rPix, BYTE &rPixBkg, C4MCTokenType eLastOp, bool fLastSet, bool fDraw, C4MCOverlay **ppPixelSetOverlay)
{
	// algo match?
	bool SetThis=CheckMask(iX, iY);
	bool DoSet;
	// exec last op
	switch (eLastOp)
	{
	case MCT_AND: // and
		DoSet=SetThis&&fLastSet;
		break;
	case MCT_OR: // or
		DoSet=SetThis||fLastSet;
		break;
	case MCT_XOR: // xor
		DoSet=SetThis^fLastSet;
		break;
	default: // no op
		DoSet=SetThis;
		break;
	}

	// set pix to local value and exec children, if no operator is following
	if ((DoSet && fDraw && Op == MCT_NONE) || Group)
	{
		// groups don't set a pixel value, if they're associated with an operator
		fDraw &= !Group || (Op == MCT_NONE);
		if (fDraw && DoSet && !Mask)
		{
			rPix=MatClr;
			rPixBkg=MatClrBkg;
			if (ppPixelSetOverlay) *ppPixelSetOverlay = this;
		}
		bool fLastSetC=false; eLastOp=MCT_NONE;
		// evaluate children overlays, if this was painted, too
		for (C4MCNode *pChild=Child0; pChild; pChild=pChild->Next)
			if (C4MCOverlay *pOvrl=pChild->Overlay())
			{
				fLastSetC=pOvrl->RenderPix(iX, iY, rPix, rPixBkg, eLastOp, fLastSetC, fDraw, ppPixelSetOverlay);
				if (Group && (pOvrl->Op == MCT_NONE))
					DoSet |= fLastSetC;
				eLastOp=pOvrl->Op;
			}
		// add evaluation-callback
		if (pEvaluateFunc && DoSet && fDraw) pEvaluateFunc->EnablePixel(iX, iY);
	}
	// done
	return DoSet;
}

bool C4MCOverlay::PeekPix(int32_t iX, int32_t iY)
{
	// start with this one
	C4MCOverlay *pOvrl=this; bool fLastSetC=false; C4MCTokenType eLastOp=MCT_NONE; BYTE Crap;
	// loop through op chain
	while (true)
	{
		fLastSetC=pOvrl->RenderPix(iX, iY, Crap, Crap, eLastOp, fLastSetC, false);
		eLastOp=pOvrl->Op;
		if (!pOvrl->Op) break;
		// must be another overlay, since there's an operator
		// hopefully, the preparser will catch all the other crap
		pOvrl=pOvrl->Next->Overlay();
	}
	// return result
	return fLastSetC;
}

// point

C4MCPoint::C4MCPoint(C4MCNode *pOwner) : C4MCNode(pOwner)
{
	// zero members
	X=Y=0;
}

C4MCPoint::C4MCPoint(C4MCParser* pParser, C4MCNode *pOwner, C4MCPoint &rTemplate, bool fClone) : C4MCNode(pParser, pOwner, rTemplate, fClone)
{
	// copy fields
	X=rTemplate.X; Y=rTemplate.Y;
	RX=rTemplate.RX; RY=rTemplate.RY;
}

void C4MCPoint::Default()
{
	X=Y=0;
}

bool C4MCPoint::SetField(C4MCParser *pParser, const char *szField, const char *szSVal, int32_t iVal, C4MCTokenType ValType)
{
	// only explicit %/px
	if (ValType == MCT_INT) return false;
	if (SEqual (szField, "x"))
	{
		RX.Set(IntPar, ValType == MCT_PERCENT);
		return true;
	}
	else if (SEqual (szField, "y"))
	{
		RY.Set(IntPar, ValType == MCT_PERCENT);
		return true;
	}
	return false;
}

void C4MCPoint::Evaluate()
{
	// inherited
	C4MCNode::Evaluate();
	// get mat color
	// calc size
	if (Owner)
	{
		C4MCOverlay *pOwnrOvrl;
		if ((pOwnrOvrl=OwnerOverlay()))
		{
			X = RX.Evaluate(pOwnrOvrl->Wdt) + pOwnrOvrl->X;
			Y = RY.Evaluate(pOwnrOvrl->Hgt) + pOwnrOvrl->Y;
		}
	}
}

// map

C4MCMap::C4MCMap(C4MCNode *pOwner) : C4MCOverlay(pOwner)
{

}

C4MCMap::C4MCMap(C4MCParser* pParser, C4MCNode *pOwner, C4MCMap &rTemplate, bool fClone) : C4MCOverlay(pParser, pOwner, rTemplate, fClone)
{

}

void C4MCMap::Default()
{
	// inherited
	C4MCOverlay::Default();
	// size by landscape def
	MapCreator->Landscape->GetMapSize(Wdt, Hgt, MapCreator->PlayerCount);
}

bool C4MCMap::RenderTo(BYTE *pToBuf, BYTE *pToBufBkg, int32_t iPitch)
{
	// set current render target
	if (MapCreator) MapCreator->pCurrentMap=this;
	// draw pixel by pixel
	for (int32_t iY=0; iY<Hgt; iY++)
	{
		for (int32_t iX=0; iX<Wdt; iX++)
		{
			// default to sky
			BYTE dummyPix;
			*pToBuf=0;
			if (pToBufBkg) *pToBufBkg=0;
			// render pixel value
			C4MCOverlay *pRenderedOverlay = nullptr;
			RenderPix(iX, iY, *pToBuf, pToBufBkg ? *pToBufBkg : dummyPix, MCT_NONE, false, true, &pRenderedOverlay);
			// add draw-callback for rendered overlay
			if (pRenderedOverlay)
				if (pRenderedOverlay->pDrawFunc)
					pRenderedOverlay->pDrawFunc->EnablePixel(iX, iY);
			// next pixel
			pToBuf++;
			if (pToBufBkg) pToBufBkg++;
		}
		// next line
		pToBuf+=iPitch-Wdt;
		if (pToBufBkg) pToBufBkg+=iPitch-Wdt;
	}
	// reset render target
	if (MapCreator) MapCreator->pCurrentMap=nullptr;
	// success
	return true;
}

void C4MCMap::SetSize(int32_t iWdt, int32_t iHgt)
{
	// store new size
	Wdt=iWdt; Hgt=iHgt;
	// update relative values
	MapCreator->ReEvaluate();
}


// map creator

C4MapCreatorS2::C4MapCreatorS2(C4SLandscape *pLandscape, C4TextureMap *pTexMap, C4MaterialMap *pMatMap, int iPlayerCount) : C4MCNode(nullptr)
{
	// me r b creator
	MapCreator=this;
	// store members
	Landscape=pLandscape; TexMap=pTexMap; MatMap=pMatMap;
	PlayerCount=iPlayerCount;
	// set engine field for default stuff
	DefaultMap.MapCreator=this;
	DefaultOverlay.MapCreator=this;
	DefaultPoint.MapCreator=this;
	// default to landscape settings
	Default();
}

C4MapCreatorS2::~C4MapCreatorS2()
{
	// clear fields
	Clear();
}

void C4MapCreatorS2::Default()
{
	// default templates
	DefaultMap.Default();
	DefaultOverlay.Default();
	DefaultPoint.Default();
	pCurrentMap=nullptr;
}

void C4MapCreatorS2::Clear()
{
	// clear nodes
	C4MCNode::Clear();
	// clear callbacks
	CallbackArrays.Clear();
	// defaults templates
	Default();
}

bool C4MapCreatorS2::ReadFile(const char *szFilename, C4Group *pGrp)
{
	// create parser and read file
	try
	{
		C4MCParser(this).ParseFile(szFilename, pGrp);
	}
	catch (C4MCParserErr err)
	{
		err.show();
		return false;
	}
	// success
	return true;
}

bool C4MapCreatorS2::ReadScript(const char *szScript)
{
	// create parser and read
	try
	{
		C4MCParser(this).Parse(szScript);
	}
	catch (C4MCParserErr err)
	{
		err.show();
		return false;
	}
	// success
	return true;
}

C4MCMap *C4MapCreatorS2::GetMap(const char *szMapName)
{
	C4MCMap *pMap=nullptr; C4MCNode *pNode;
	// get map
	if (szMapName && *szMapName)
	{
		// by name...
		if ((pNode = GetNodeByName(szMapName)))
			if (pNode->Type() == MCN_Map)
				pMap = (C4MCMap *) pNode;
	}
	else
	{
		// or simply last map entry
		for (pNode = ChildL; pNode; pNode=pNode->Prev)
			if (pNode->Type() == MCN_Map)
			{
				pMap = (C4MCMap *) pNode;
				break;
			}
	}
	return pMap;
}

bool C4MapCreatorS2::Render(const char *szMapName, CSurface8*& sfcMap, CSurface8*& sfcMapBkg)
{
	assert(sfcMap == nullptr);
	assert(sfcMapBkg == nullptr);

	// get map
	C4MCMap *pMap=GetMap(szMapName);
	if (!pMap) return false;

	// get size
	int32_t sfcWdt, sfcHgt;
	sfcWdt=pMap->Wdt; sfcHgt=pMap->Hgt;
	if (!sfcWdt || !sfcHgt) return false;

	// create surfaces
	sfcMap = new CSurface8(sfcWdt, sfcHgt);
	sfcMapBkg = new CSurface8(sfcWdt, sfcHgt);
	assert(sfcMap->Pitch == sfcMapBkg->Pitch);

	// render map to surface
	pMap->RenderTo(sfcMap->Bits, sfcMapBkg->Bits, sfcMap->Pitch);

	// success
	return true;
}

static inline void DWordAlign(int &val)
{
	if (val%4) { val>>=2; val<<=2; val+=4; }
}

BYTE *C4MapCreatorS2::RenderBuf(const char *szMapName, int32_t &sfcWdt, int32_t &sfcHgt)
{
	// get map
	C4MCMap *pMap=GetMap(szMapName);
	if (!pMap) return nullptr;

	// get size
	sfcWdt=pMap->Wdt; sfcHgt=pMap->Hgt;
	if (!sfcWdt || !sfcHgt) return nullptr;
	int dwSfcWdt = sfcWdt;
	DWordAlign(dwSfcWdt);
	sfcWdt = dwSfcWdt;

	// create buffer
	BYTE *buf=new BYTE[sfcWdt*sfcHgt];

	// render and return it
	pMap->RenderTo(buf, nullptr, sfcWdt);
	return buf;
}

C4MCParserErr::C4MCParserErr(C4MCParser *pParser, const char *szMsg)
{
	// create error message
	sprintf(Msg, "%s: %s (%d)", pParser->Filename, szMsg, pParser->BPos ? SGetLine(pParser->BPos, pParser->CPos) : 0);
}

C4MCParserErr::C4MCParserErr(C4MCParser *pParser, const char *szMsg, const char *szPar)
{
	char Buf[C4MaxMessage];
	// create error message
	sprintf(Buf, szMsg, szPar);
	sprintf(Msg, "%s: %s (%d)", pParser->Filename, Buf, pParser->BPos ? SGetLine(pParser->BPos, pParser->CPos) : 0);
}

void C4MCParserErr::show()
{
	// log error
	Log(Msg);
}


// parser

C4MCParser::C4MCParser(C4MapCreatorS2 *pMapCreator)
{
	// store map creator
	MapCreator=pMapCreator;
	// reset some fields
	Code=nullptr; BPos = nullptr; CPos=nullptr; *Filename=0;
}

C4MCParser::~C4MCParser()
{
	// clean up
	Clear();
}

void C4MCParser::Clear()
{
	// clear code if present
	delete [] Code; Code=nullptr; BPos = nullptr; CPos=nullptr;
	// reset filename
	*Filename=0;
}

bool C4MCParser::AdvanceSpaces()
{
	char C, C2 = (char) 0;
	// defaultly, not in comment
	int32_t InComment = 0; // 0/1/2 = no comment/line comment/multi line comment
	// don't go past end
	while ((C = *CPos))
	{
		// loop until out of comment and non-whitespace is found
		switch (InComment)
		{
		case 0:
			if (C == '/')
			{
				CPos++;
				switch (*CPos)
				{
				case '/': InComment = 1; break;
				case '*': InComment = 2; break;
				default: CPos--; return true;
				}
			}
			else if ((BYTE) C > 32) return true;
			break;
		case 1:
			if (((BYTE) C == 13) || ((BYTE) C == 10)) InComment = 0;
			break;
		case 2:
			if ((C == '/') && (C2 == '*')) InComment = 0;
			break;
		}
		// next char; store prev
		CPos++; C2 = C;
	}
	// end of code reached; return false
	return false;
}

bool C4MCParser::GetNextToken()
{
	// move to start of token
	if (!AdvanceSpaces()) { CurrToken=MCT_EOF; return false; }
	// store offset
	const char *CPos0 = CPos;
	int32_t Len = 0;
	// token get state
	enum TokenGetState
	{
		TGS_None,       // just started
		TGS_Ident,      // getting identifier
		TGS_Int,        // getting integer
		TGS_Dir         // getting directive
	};
	TokenGetState State = TGS_None;

	// loop until finished
	while (true)
	{
		// get char
		char C = *CPos;

		switch (State)
		{
		case TGS_None:
			// get token type by first char
			// +/- are operators
			if ((((C >= '0') && (C <= '9')) || (C == '+') || (C == '-')))
				State = TGS_Int;                              // integer by +, -, 0-9
			else if (C == '#')  State = TGS_Dir;                              // directive by "#"
			else if (C == ';') {CPos++; CurrToken=MCT_SCOLON;   return true; }  // ";"
			else if (C == '=') {CPos++; CurrToken=MCT_EQ;       return true; }  // "="
			else if (C == '{') {CPos++; CurrToken=MCT_BLOPEN;   return true; }  // "{"
			else if (C == '}') {CPos++; CurrToken=MCT_BLCLOSE;  return true; }  // "}"
			else if (C == '&') {CPos++; CurrToken=MCT_AND;      return true; }  // "&"
			else if (C == '|') {CPos++; CurrToken=MCT_OR;       return true; }  // "|"
			else if (C == '^') {CPos++; CurrToken=MCT_XOR;      return true; }  // "^"
			else if (C >= '@')  State = TGS_Ident;                            // identifier by all non-special chars
			else
			{
				// unrecognized char
				CPos++;
				throw C4MCParserErr(this, "unexpected character found");
			}
			break;

		case TGS_Ident: // ident and directive: parse until non ident-char is found
		case TGS_Dir:
			if (((C < '0') || (C > '9')) && ((C < 'a') || (C > 'z')) && ((C < 'A') || (C > 'Z')) && (C != '_'))
			{
				// return ident/directive
				Len = std::min<int32_t>(Len, C4MaxName);
				SCopy(CPos0, CurrTokenIdtf, Len);
				if (State==TGS_Ident) CurrToken=MCT_IDTF; else CurrToken=MCT_DIR;
				return true;
			}
			break;

		case TGS_Int: // integer: parse until non-number is found
			if ((C < '0') || (C > '9'))
			{
				// return integer
				Len = std::min<int32_t>(Len, C4MaxName);
				CurrToken=MCT_INT;
				// check for "-"
				if (Len == 1 && *CPos0 == '-')
				{
					CurrToken = MCT_RANGE;
					return true;
				}
				else if ('%' == C) { CPos++; CurrToken=MCT_PERCENT; } // "%"
				else if ('p' == C)
				{
					// p or px
					++CPos;
					if ('x' == *CPos) ++CPos;
					CurrToken=MCT_PX;
				}
				SCopy(CPos0, CurrTokenIdtf, Len);
				// it's not, so return the int32_t
				sscanf(CurrTokenIdtf, "%d", &CurrTokenVal);
				return true;
			}
			break;

		}
		// next char
		CPos++; Len++;
	}

}

static void PrintNodeTree(C4MCNode *pNode, int depth)
{
	for (int i = 0; i < depth; ++i)
		printf("  ");
	switch (pNode->Type())
	{
	case MCN_Node: printf("Node %s\n", pNode->Name); break;
	case MCN_Overlay: printf("Overlay %s\n", pNode->Name); break;
	case MCN_Point: printf("Point %s\n", pNode->Name); break;
	case MCN_Map: printf("Map %s\n", pNode->Name); break;
	}
	for (C4MCNode * pChild = pNode->Child0; pChild; pChild = pChild->Next)
		PrintNodeTree(pChild, depth + 1);
}

void C4MCParser::ParseTo(C4MCNode *pToNode)
{
	C4MCNode *pNewNode=nullptr;  // new node
	bool Done=false;          // finished?
	C4MCNodeType LastOperand = C4MCNodeType(-1); // last first operand of operator
	char FieldName[C4MaxName];// buffer for current field to access
	C4MCNode *pCpyNode;       // node to copy from
	// current state
	enum ParseState
	{
		PS_NONE,      // just started
		PS_KEYWD1,    // got block-opening keyword (map, overlay etc.)
		PS_KEYWD1N,   // got name for block
		PS_AFTERNODE, // node has been parsed; expect ; or operator
		PS_GOTOP,     // got operator
		PS_GOTIDTF,   // got identifier, expect '=', ';' or '{'; identifier remains in CurrTokenIdtf
		PS_GOTOPIDTF, // got identifier after operator; accept ';' or '{' only
		PS_SETFIELD   // accept field value; field is stored in FieldName
	};
	ParseState State = PS_NONE;
	// parse until end of file (or block)
	while (GetNextToken())
	{
		switch (State)
		{
		case PS_NONE:
		case PS_GOTOP:
			switch (CurrToken)
			{
			case MCT_DIR:
				// top level needed
				if (!pToNode->GlobalScope())
					throw C4MCParserErr(this, C4MCErr_NoDirGlobal);
				// no directives so far
				throw C4MCParserErr(this, C4MCErr_UnknownDir, CurrTokenIdtf);
				break;
			case MCT_IDTF:
				// identifier: check keywords
				if (SEqual(CurrTokenIdtf, C4MC_Overlay))
				{
					// overlay: create overlay node, using default template
					pNewNode = new C4MCOverlay(this, pToNode, MapCreator->DefaultOverlay, false);
					State=PS_KEYWD1;
				}
				else if (SEqual(CurrTokenIdtf, C4MC_Point) && !pToNode->GetNodeByName(CurrTokenIdtf))
				{
					// only in overlays
					if (!pToNode->Overlay())
						throw C4MCParserErr(this, C4MCErr_PointOnlyOvl);
					// create point node, using default template
					pNewNode = new C4MCPoint(this, pToNode, MapCreator->DefaultPoint, false);
					State=PS_KEYWD1;
				}
				else if (SEqual(CurrTokenIdtf, C4MC_Map))
				{
					// map: check top level
					if (!pToNode->GlobalScope())
						throw C4MCParserErr(this, C4MCErr_MapNoGlobal);
					// create map node, using default template
					pNewNode = new C4MCMap(this, pToNode, MapCreator->DefaultMap, false);
					State=PS_KEYWD1;
				}
				else
				{
					// so this is either a field-set or a defined node
					// '=', ';' or '{' may follow, none of these will clear the CurrTokenIdtf
					// so safely assume it preserved and just update the state
					if (State==PS_GOTOP) State=PS_GOTOPIDTF; else State=PS_GOTIDTF;
				}
				// operator: check type
				if (State == PS_GOTOP && pNewNode)
					if (LastOperand != pNewNode->Type())
						throw C4MCParserErr(this, C4MCErr_OpTypeErr);
				break;
			case MCT_BLCLOSE:
			case MCT_EOF:
				// block done
				Done=true;
				break;
			default:
				// we don't like that
				throw C4MCParserErr(this, C4MCErr_IdtfExp);
				break;
			}
			break;
		case PS_KEYWD1:
			if (CurrToken==MCT_IDTF)
			{
				// name the current node
				SCopy(CurrTokenIdtf, pNewNode->Name, C4MaxName);
				State=PS_KEYWD1N;
				break;
			}
			else if (pToNode->GlobalScope())
			{
				// disallow unnamed nodes in global scope
				throw C4MCParserErr(this, C4MCErr_UnnamedNoGlbl);
			}
			// in local scope, allow unnamed; so continue
		case PS_KEYWD1N:
			// do expect a block opening
			if (CurrToken!=MCT_BLOPEN)
				throw C4MCParserErr(this, C4MCErr_BlOpenExp);
			// parse new node
			ParseTo(pNewNode);
			// check file end
			if (CurrToken==MCT_EOF)
				throw C4MCParserErr(this, C4MCErr_EOF);
			// reset state
			State=PS_AFTERNODE;
			break;
		case PS_GOTIDTF:
		case PS_GOTOPIDTF:
			switch (CurrToken)
			{
			case MCT_EQ:
				// so it's a field set
				// not after operators
				if (State==PS_GOTOPIDTF)
					throw C4MCParserErr(this, C4MCErr_Obj2Exp);
				// store field name
				SCopy(CurrTokenIdtf, FieldName, C4MaxName);
				// update state to accept value
				State=PS_SETFIELD;
				break;
			case MCT_BLOPEN:
			case MCT_SCOLON:
			case MCT_AND: case MCT_OR: case MCT_XOR:
				// so it's a node copy
				// local scope only
				if (pToNode->GlobalScope())
					throw C4MCParserErr(this, C4MCErr_ReinstNoGlobal, CurrTokenIdtf);
				// get the node
				pCpyNode=pToNode->GetNodeByName(CurrTokenIdtf);
				if (!pCpyNode)
					throw C4MCParserErr(this, C4MCErr_UnknownObj, CurrTokenIdtf);
				// create the copy
				switch (pCpyNode->Type())
				{
				case MCN_Overlay:
					// create overlay
					pNewNode=new C4MCOverlay(this, pToNode, *((C4MCOverlay *) pCpyNode), false);
					break;
				case MCN_Map:
					// maps not allowed
					if (pCpyNode->Type() == MCN_Map)
						throw C4MCParserErr(this, C4MCErr_MapNoGlobal, CurrTokenIdtf);
					break;
				default:
					// huh?
					throw C4MCParserErr(this, C4MCErr_ReinstUnknown, CurrTokenIdtf);
					break;
				}
				// check type for operators
				if (State==PS_GOTOPIDTF)
					if (LastOperand != pNewNode->Type())
						throw C4MCParserErr(this, C4MCErr_OpTypeErr);
				// further overloads?
				if (CurrToken==MCT_BLOPEN)
				{
					// parse new node
					ParseTo(pNewNode);
					// get next token, as we'll simply fall through to PS_AFTERNODE
					GetNextToken();
					// check file end
					if (CurrToken==MCT_EOF)
						throw C4MCParserErr(this, C4MCErr_EOF);
				}
				// reset state
				State=PS_AFTERNODE;
				break;

			default:
				throw C4MCParserErr(this, C4MCErr_EqSColonBlOpenExp);
				break;
			}
			// fall through to next case, if it was a named node reinstanciation
			if (State != PS_AFTERNODE) break;
		case PS_AFTERNODE:
			// expect operator or semicolon
			switch (CurrToken)
			{
			case MCT_SCOLON:
				// reset state
				State=PS_NONE;
				break;
			case MCT_AND:
			case MCT_OR:
			case MCT_XOR:
				// operator: not in global scope
				if (pToNode->GlobalScope())
					throw C4MCParserErr(this, C4MCErr_OpsNoGlobal);
				// set operator
				if (!pNewNode->SetOp(CurrToken))
					throw C4MCParserErr(this, "';' expected");
				LastOperand=pNewNode->Type();
				// update state
				State=PS_GOTOP;
				break;
			default:
				throw C4MCParserErr(this, C4MCErr_SColonOrOpExp);
				break;
			}
			// node done
			// evaluate node and children, if this is top-level
			// we mustn't evaluate everything immediately, because parents must be evaluated first!
			if (pToNode->GlobalScope()) pNewNode->ReEvaluate();
			pNewNode=nullptr;
			break;
		case PS_SETFIELD:
			ParseValue (pToNode, FieldName);
			/*// set field: accept integer constants and identifiers
			switch (CurrToken)
			  {
			  case MCT_IDTF:
			    // reset value field
			    CurrTokenVal=0;
			  case MCT_INT:
			    break;
			  default:
			    throw C4MCParserErr(this, C4MCErr_FieldConstExp, CurrTokenIdtf);
			    break;
			  }
			// set field
			if (!pToNode->SetField(this, FieldName, CurrTokenIdtf, CurrTokenVal, CurrToken))
			  // field not found
			  throw C4MCParserErr(this, C4MCErr_Field404, FieldName);
			// now, the one and only thing to get is a semicolon
			if (!GetNextToken())
			  throw C4MCParserErr(this, C4MCErr_EOF);
			if (CurrToken != MCT_SCOLON)
			  throw C4MCParserErr(this, C4MCErr_SColonExp);*/
			// reset state
			State=PS_NONE;
			break;
		}
		// don't get another token!
		if (Done) break;
	}
	// end of file expected?
	if (State != PS_NONE)
	{
		if (State == PS_GOTOP)
			throw C4MCParserErr(this, C4MCErr_Obj2Exp);
		else
			throw C4MCParserErr(this, C4MCErr_EOF);
	}
}

void C4MCParser::ParseValue(C4MCNode *pToNode, const char *szFieldName)
{
	int32_t Value;
	C4MCTokenType Type;
	switch (CurrToken)
	{
	case MCT_IDTF:
	{
		// set field
		if (!pToNode->SetField(this, szFieldName, CurrTokenIdtf, 0, CurrToken))
			// field not found
			throw C4MCParserErr(this, C4MCErr_Field404, szFieldName);
		if (!GetNextToken())
			throw C4MCParserErr(this, C4MCErr_EOF);
		break;
	}
	case MCT_INT:
	case MCT_PX:
	case MCT_PERCENT:
	{
		Value = CurrTokenVal;
		Type = CurrToken;
		if (!GetNextToken())
			throw C4MCParserErr(this, C4MCErr_EOF);
		// range
		if (MCT_RANGE == CurrToken)
		{
			// Get the second value
			if (!GetNextToken())
				throw C4MCParserErr(this, C4MCErr_EOF);
			if (MCT_INT == CurrToken || MCT_PX == CurrToken || MCT_PERCENT == CurrToken)
			{
				Value += Random (CurrTokenVal - Value);
			}
			else
				throw C4MCParserErr(this, C4MCErr_FieldConstExp, CurrTokenIdtf);
			Type = CurrToken;
			if (!GetNextToken())
				throw C4MCParserErr(this, C4MCErr_EOF);
		}
		if (!pToNode->SetField(this, szFieldName, CurrTokenIdtf, Value, Type))
			// field not found
			throw C4MCParserErr(this, C4MCErr_Field404, szFieldName);
		break;
	}
	default:
	{
		throw C4MCParserErr(this, C4MCErr_FieldConstExp, CurrTokenIdtf);
	}
	}

	// now, the one and only thing to get is a semicolon
	if (CurrToken != MCT_SCOLON)
		throw C4MCParserErr(this, C4MCErr_SColonExp);


	/*
	// set field: accept integer constants and identifiers
	switch (CurrToken)
	  {
	  case MCT_IDTF:
	    // reset value field
	    CurrTokenVal=0;
	    // set field
	    if (!pToNode->SetField(this, szFieldName, CurrTokenIdtf, CurrTokenVal, CurrToken))
	      // field not found
	      throw C4MCParserErr(this, C4MCErr_Field404, szFieldName);
	    break;
	  case MCT_INT:
	    Value1 = CurrTokenVal;
	    while (GetNextToken ())
	      {
	      switch (CurrToken)
	        {
	        case MCT_SCOLON:
	          // set field
	          if (!pToNode->SetField(this, szFieldName, CurrTokenIdtf, Value1, MCT_INT))
	            // field not found
	            throw C4MCParserErr(this, C4MCErr_Field404, szFieldName);
	          return;
	          break;
	        case MCT_RANGE:
	          break;
	        case MCT_INT:
	          Value2 = CurrTokenVal;
	          Value1 += Random (Value2 - Value1);
	          break;
	        default:
	          throw C4MCParserErr(this, C4MCErr_SColonExp);
	          break;
	        }
	      }
	    break;
	  default:
	    throw C4MCParserErr(this, C4MCErr_FieldConstExp, CurrTokenIdtf);
	    break;
	  }
	// now, the one and only thing to get is a semicolon
	if (!GetNextToken())
	  throw C4MCParserErr(this, C4MCErr_EOF);
	if (CurrToken != MCT_SCOLON)
	  throw C4MCParserErr(this, C4MCErr_SColonExp);*/
}

void C4MCParser::ParseFile(const char *szFilename, C4Group *pGrp)
{
	size_t iSize; // file size

	// clear any old data
	Clear();
	// store filename
	SCopy(szFilename, Filename, C4MaxName);
	// check group
	if (!pGrp) throw C4MCParserErr(this, C4MCErr_NoGroup);
	// get file
	if (!pGrp->AccessEntry(szFilename, &iSize))
		// 404
		throw C4MCParserErr(this, C4MCErr_404);
	// file is empty?
	if (!iSize) return;
	// alloc mem
	Code = new char[iSize+1];
	// read file
	pGrp->Read((void *) Code, iSize);
	Code[iSize]=0;
	// parse it
	BPos=Code;
	CPos=Code;
	ParseTo(MapCreator);
	if (false) PrintNodeTree(MapCreator, 0);
	// free code
	// on errors, this will be done be destructor
	Clear();
}

void C4MCParser::Parse(const char *szScript)
{
	// clear any old data
	Clear();
	// parse it
	BPos=szScript;
	CPos=szScript;
	ParseTo(MapCreator);
	if (false) PrintNodeTree(MapCreator, 0);
	// free code
	// on errors, this will be done be destructor
	Clear();

}

void C4MCParser::ParseMemFile(const char *szScript, const char *szFilename)
{
	// clear any old data
	Clear();
	// store filename
	SCopy(szFilename, Filename, C4MaxName);
	// parse it
	BPos=szScript;
	CPos=szScript;
	ParseTo(MapCreator);
	// on errors, this will be done be destructor
	Clear();
}


// algorithms ---------------------

// helper func
bool PreparePeek(C4MCOverlay **ppOvrl, int32_t &iX, int32_t &iY, C4MCOverlay **ppTopOvrl)
{
	// zoom out
	iX/=(*ppOvrl)->ZoomX; iY/=(*ppOvrl)->ZoomY;
	// get owning overlay
	C4MCOverlay *pOvrl2=(*ppOvrl)->OwnerOverlay();
	if (!pOvrl2) return false;
	// get uppermost overlay
	C4MCOverlay *pNextOvrl;
	for (*ppTopOvrl=pOvrl2; (pNextOvrl=(*ppTopOvrl)->OwnerOverlay()); *ppTopOvrl=pNextOvrl) {}
	// get first of operator-chain
	pOvrl2=pOvrl2->FirstOfChain();
	// set new overlay
	*ppOvrl=pOvrl2;
	// success
	return true;
}

#define a  pOvrl->Alpha
#define b  pOvrl->Beta
#define s  pOvrl->Seed
#define z  C4MC_ZoomRes
#define z2 (C4MC_ZoomRes*C4MC_ZoomRes)

bool AlgoSolid(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// solid always solid :)
	return true;
}

bool AlgoRandom(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// totally random
	return !((((s ^ (iX<<2) ^ (iY<<5))^((s>>16)+1+iX+(iY<<2)))/17)%(a.Evaluate(C4MC_SizeRes)+2));
}

bool AlgoChecker(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// checkers with size of 10
	return !(((iX/(z*10))%2)^((iY/(z*10))%2));
}

bool AlgoBozo(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// do some bozo stuff - keep it regular here, since it may be modified by turbulence
	int32_t iXC=(iX/10+s+(iY/80))%(z*2)-z;
	int32_t iYC=(iY/10+s+(iX/80))%(z*2)-z;
	int32_t id=Abs(iXC*iYC); // ((iSeed^iX^iY)%z)
	return id > z2*(a.Evaluate(C4MC_SizeRes)+10)/50;
}

bool AlgoSin(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// a sine curve where bottom is filled
	return iY > fixtoi((Sin(itofix(iX/z*10))+1)*z*10);
}

bool AlgoBoxes(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// For percents instead of Pixels
	int32_t pxb = b.Evaluate(pOvrl->Wdt);
	int32_t pxa = a.Evaluate(pOvrl->Wdt);
	// return whether inside box
	return Abs(iX+(s%4738))%(pxb*z+1)<pxa*z+1 && Abs(iY+(s/4738))%(pxb*z+1)<pxa*z+1;
}

bool AlgoRndChecker(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// randomly set squares with size of 10
	return AlgoRandom(pOvrl, iX/(z*10), iY/(z*10));
}

bool AlgoLines(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// For percents instead of Pixels
	int32_t pxb = b.Evaluate(pOvrl->Wdt);
	int32_t pxa = a.Evaluate(pOvrl->Wdt);
	// return whether inside line
	return Abs(iX+(s%4738))%(pxb*z+1)<pxa*z+1;
}

bool AlgoBorder(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	C4MCOverlay *pTopOvrl;
	// get params before, since pOvrl will be changed by PreparePeek
	int32_t la=a.Evaluate(pOvrl->Wdt); int32_t lb=b.Evaluate(pOvrl->Hgt);
	// prepare a pixel peek from owner
	if (!PreparePeek(&pOvrl, iX, iY, &pTopOvrl)) return false;
	// query a/b pixels in x/y-directions
	for (int32_t x=iX-la; x<=iX+la; x++) if (pTopOvrl->InBounds(x, iY)) if (!pOvrl->PeekPix(x, iY)) return true;
	for (int32_t y=iY-lb; y<=iY+lb; y++) if (pTopOvrl->InBounds(iX, y)) if (!pOvrl->PeekPix(iX, y)) return true;
	// nothing found
	return false;
}

bool AlgoMandel(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// how many iterations?
	uint32_t iMandelIter = a.Evaluate(C4MC_SizeRes) != 0 ? a.Evaluate(C4MC_SizeRes) : 1000;
	if (iMandelIter < 10) iMandelIter = 10;
	// calc c & ci values
	double c =  ((double) iX / z / pOvrl->Wdt - .5 * ((double) pOvrl->ZoomX / z)) * 4;
	double ci = ((double) iY / z / pOvrl->Hgt - .5 * ((double) pOvrl->ZoomY / z)) * 4;
	// create _z & _zi
	double _z = c, _zi = ci;
	double xz;
	uint32_t i;
	for (i=0; i<iMandelIter; i++)
	{
		xz = _z * _z - _zi * _zi;
		_zi = 2 * _z * _zi + ci;
		_z = xz + c;
		if (_z * _z + _zi * _zi > 4) break;
	}
	return !(i<iMandelIter);
}

bool AlgoGradient(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	return (std::abs((iX^(iY*3)) * 2531011L) % 214013L) % z > iX / pOvrl->Wdt;
}

bool AlgoScript(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	C4AulParSet Pars(iX, iY, pOvrl->Alpha.Evaluate(C4MC_SizeRes), pOvrl->Beta.Evaluate(C4MC_SizeRes));
	return ::GameScript.Call(FormatString("ScriptAlgo%s", pOvrl->Name).getData(), &Pars).getBool();
}

bool AlgoRndAll(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// return by seed and params; ignore pos
	return s%100<a.Evaluate(C4MC_SizeRes);
}

bool AlgoPolygon(C4MCOverlay *pOvrl, int32_t iX, int32_t iY)
{
	// Wo do not support empty polygons.
	if (!pOvrl -> ChildL) return false;
	int32_t uX = 0; // last point before current point
	int32_t uY = 0; // with uY != iY
	int32_t cX, cY; // current point
	int32_t lX = 0; // x of really last point before current point
	int32_t count = 0;
	bool ignore = false;
	int32_t zX; //Where edge intersects with line
	C4MCNode *pChild, *pStartChild;
	//get a point with uY!=iY, or anyone
	for (pChild = pOvrl -> ChildL; pChild->Prev; pChild = pChild->Prev)
		if (pChild->Type() == MCN_Point)
		{
			uX = ((C4MCPoint*) pChild) -> X * 100;
			lX = uX;
			uY = ((C4MCPoint*) pChild) -> Y * 100;
			if (iY != uY) break;
		}
	pStartChild = pChild -> Next;
	if (!pStartChild) pStartChild = pOvrl->Child0;
	if (!pStartChild) return false;
	for (pChild = pStartChild; ; pChild=pChild -> Next)
	{
		if (!pChild) pChild = pOvrl->Child0;
		if (pChild->Type() == MCN_Point)
		{
			cX = ((C4MCPoint*) pChild) -> X * 100;
			cY = ((C4MCPoint*) pChild) -> Y * 100;
			//If looking at line
			if (ignore)
			{
				//if C is on line
				if (cY == iY)
				{
					//if I is on edge
					if (((lX < iX) == (iX < cX)) || (cX == iX)) return true;
				}
				else
				{
					//if edge intersects line
					if ((uY < iY) == (iY < cY) && (lX >= iX)) count++;
					ignore = false;
					uX = cX;
					uY = cY;
				}
			}
			//if looking at ray
			else
			{
				//If point C lays on ray
				if (cY == iY)
				{
					//are I and C the same points?
					if (cX == iX) return true;
					//skip this point for now
					ignore = true;
				}
				else
				{
					//if edge intersects line
					if ((uY < iY) == (iY <= cY))
					{
						//and edge intersects ray, because both points are right of iX
						if (iX < std::min (uX, cX))
						{
							count++;
						}
						//or one is right of I
						else if (iX <= std::max (uX, cX))
						{
							//and edge intersects with ray
							if (iX < (zX = ((cX - uX) * (iY - uY) / (cY - uY)) + uX)) count++;
							//if I lays on CU
							if (zX == iX) return true;
						}
					}
					uX = cX;
					uY = cY;
				}
			}
			lX = cX;
		}
		if (pChild -> Next == pStartChild) break;
		if (!pChild -> Next) if (pStartChild == pOvrl->Child0) break;
	}
	//if edge has changed side of ray uneven times
	if ((count & 1) > 0) return true; else return false;
}

#undef a
#undef b
#undef s
#undef z
#undef z2

C4MCAlgorithm C4MCAlgoMap[] =
{
	{ "solid",      &AlgoSolid },
	{ "random",     &AlgoRandom },
	{ "checker",    &AlgoChecker },
	{ "bozo",       &AlgoBozo },
	{ "sin",        &AlgoSin },
	{ "boxes",      &AlgoBoxes },
	{ "rndchecker", &AlgoRndChecker },
	{ "lines",      &AlgoLines },
	{ "border",     &AlgoBorder },
	{ "mandel",     &AlgoMandel },
	{ "gradient",   &AlgoGradient },
	{ "script",     &AlgoScript },
	{ "rndall",     &AlgoRndAll },
	{ "poly",       &AlgoPolygon },
	{ "",     nullptr }
};

#define offsC4MCOvrl(x) reinterpret_cast<C4MCOverlayOffsetType>(&C4MCOverlay::x)

namespace {
	C4MCNodeAttr C4MCOvrlMap[] =
	{
		{ "x",           C4MCV_Percent,     offsC4MCOvrl(RX)            },
		{ "y",           C4MCV_Percent,     offsC4MCOvrl(RY)            },
		{ "wdt",         C4MCV_Percent,     offsC4MCOvrl(RWdt)          },
		{ "hgt",         C4MCV_Percent,     offsC4MCOvrl(RHgt)          },
		{ "ox",          C4MCV_Percent,     offsC4MCOvrl(ROffX)         },
		{ "oy",          C4MCV_Percent,     offsC4MCOvrl(ROffY)         },
		{ "mat",         C4MCV_Material,    offsC4MCOvrl(Material)      },
		{ "tex",         C4MCV_Texture,     offsC4MCOvrl(Texture)       },
		{ "algo",        C4MCV_Algorithm,   offsC4MCOvrl(Algorithm)     },
		{ "sub",         C4MCV_Boolean,     offsC4MCOvrl(Sub)           },
		{ "zoomX",       C4MCV_Zoom,        offsC4MCOvrl(ZoomX)         },
		{ "zoomY",       C4MCV_Zoom,        offsC4MCOvrl(ZoomY)         },
		{ "a",           C4MCV_Pixels,      offsC4MCOvrl(Alpha)         },
		{ "b",           C4MCV_Pixels,      offsC4MCOvrl(Beta)          },
		{ "turbulence",  C4MCV_Integer,     offsC4MCOvrl(Turbulence)    },
		{ "lambda",      C4MCV_Integer,     offsC4MCOvrl(Lambda)        },
		{ "rotate",      C4MCV_Integer,     offsC4MCOvrl(Rotate)        },
		{ "seed",        C4MCV_Integer,     offsC4MCOvrl(FixedSeed)     },
		{ "invert",      C4MCV_Boolean,     offsC4MCOvrl(Invert)        },
		{ "loosebounds", C4MCV_Boolean,     offsC4MCOvrl(LooseBounds)   },
		{ "grp",         C4MCV_Boolean,     offsC4MCOvrl(Group)         },
		{ "mask",        C4MCV_Boolean,     offsC4MCOvrl(Mask)          },
		{ "evalFn",      C4MCV_ScriptFunc,  offsC4MCOvrl(pEvaluateFunc) },
		{ "drawFn",      C4MCV_ScriptFunc,  offsC4MCOvrl(pDrawFunc)     },
		{ "", C4MCV_None, nullptr }
	};
}
