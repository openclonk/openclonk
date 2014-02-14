/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Handles scripted map creation */

#include <C4Include.h>
#include <C4MapScript.h>
#include <C4Random.h>

C4MapScriptAlgo *FnParAlgo(C4PropList *algo_par);

bool C4MapScriptAlgo::GetXYProps(const C4PropList *props, C4PropertyName k, int32_t *out_xy, bool zero_defaults)
{
	// Evaluate property named "k" in proplist props to store two numbers in out_xy:
	// If props->k is a single integer, fill both numbers in out_xy with it
	// If props->k is an array, check that it contains two numbers and store them in out_xy
	if (!props->HasProperty(&Strings.P[k]))
	{
		if (zero_defaults) out_xy[0] = out_xy[1] = 0;
		return false;
	}
	C4Value val; C4ValueArray *arr;
	props->GetProperty(k, &val);
	if (arr = val.getArray())
	{
		if (arr->GetSize() != 2)
			throw new C4AulExecError(FormatString("C4MapScriptAlgo: Expected either integer or array with two integer elements in property \"%s\".", Strings.P[k].GetCStr()).getData());
		out_xy[0] = arr->GetItem(0).getInt();
		out_xy[1] = arr->GetItem(1).getInt();
	}
	else
	{
		out_xy[0] = out_xy[1] = val.getInt();
	}
	return true;
}

C4MapScriptAlgoLayer::C4MapScriptAlgoLayer(const C4PropList *props)
{
	// Get MAPALGO_Layer properties
	C4PropList *layer_pl = props->GetPropertyPropList(P_Layer);
	if (!layer_pl || !(layer = layer_pl->GetMapScriptLayer()))
		throw new C4AulExecError("C4MapScriptAlgoLayer: Expected layer in \"Layer\" property.");
}

uint8_t C4MapScriptAlgoLayer::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Layer at x,y: Just query pixel in layer. Pixels outside the layer range are zero.
	return layer->GetPix(x,y,0);
}

C4MapScriptAlgoRndChecker::C4MapScriptAlgoRndChecker(const C4PropList *props)
{
	// Get MAPALGO_RndChecker properties
	seed = props->GetPropertyInt(P_Seed);
	if (!seed) seed = Random(65536);
	set_percentage = BoundBy(props->GetPropertyInt(P_Ratio), 0, 100);
	if (!set_percentage) set_percentage = 50;
	checker_wdt = Abs(props->GetPropertyInt(P_Wdt));
	if (!checker_wdt) checker_wdt = 10;
	checker_hgt = Abs(props->GetPropertyInt(P_Hgt));
	if (!checker_hgt) checker_hgt = 10;
	C4Value is_fixed_offset_v;
	if (props->GetProperty(P_FixedOffset, &is_fixed_offset_v))
		is_fixed_offset = is_fixed_offset_v.getBool();
	else
		is_fixed_offset = false;
}

// Division and modulo operators that always round downwards
// Both assuming b>0
static int32_t divD(int32_t a, int32_t b) { return a/b-(a%b<0); }
static int32_t modD(int32_t a, int32_t b) { return (a>=0)?a%b:b-(-a)%b; }

// Creates a field of random numbers between 0 and scale-1. Returns the value of the field at position x,y
// Function should be deterministic for the same value of x,y, but should look somewhat random wrt neighbouring values of x,y
int32_t QuerySeededRandomField(int32_t seed, int32_t x, int32_t y, int32_t scale)
{
	return modD((((seed ^ (x*214013))*214013) ^ (y*214013)), scale);
}

uint8_t C4MapScriptAlgoRndChecker::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_RndChecker at x,y: Query a seeded random field scaled by checker_wdt,checker_hgt
	if (!is_fixed_offset) { x+=seed%checker_wdt; y+=((seed*214013)%checker_hgt); }
	x = divD(x, checker_wdt); y = divD(y, checker_hgt);
	return QuerySeededRandomField(seed, x,y, 100) < set_percentage;
}

C4MapScriptAlgoRect::C4MapScriptAlgoRect(const C4PropList *props)
{
	// Get MAPALGO_Rect properties
	rect = C4Rect(props->GetPropertyInt(P_X), props->GetPropertyInt(P_Y), props->GetPropertyInt(P_Wdt), props->GetPropertyInt(P_Hgt));
}

uint8_t C4MapScriptAlgoRect::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Rect at x,y: Return 1 for pixels contained in rect, 0 otherwise
	return rect.Contains(x, y);
}

C4MapScriptAlgoEllipsis::C4MapScriptAlgoEllipsis(const C4PropList *props)
{
	// Get MAPALGO_Ellipsis properties
	cx = props->GetPropertyInt(P_X);
	cy = props->GetPropertyInt(P_Y);
	wdt = Abs(props->GetPropertyInt(P_Wdt));
	hgt = Abs(props->GetPropertyInt(P_Hgt));
	if (!wdt) wdt = 10;
	if (!hgt) hgt = wdt;
}

uint8_t C4MapScriptAlgoEllipsis::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Ellipsis at x,y: Return 1 for pixels within ellipsis, 0 otherwise
	// warning: overflows for large values (wdt or hgt >=256)
	// but who would draw such large ellipsis anyway?
	uint64_t dx = Abs((cx-x)*hgt), dy = Abs((cy-y)*wdt);
	return dx*dx+dy*dy < uint64_t(wdt)*wdt*hgt*hgt;
}

C4MapScriptAlgoPolygon::C4MapScriptAlgoPolygon(const C4PropList *props)
{
	// Get MAPALGO_Polygon properties
	C4Value vptx, vpty;
	props->GetProperty(P_X, &vptx); props->GetProperty(P_Y, &vpty);
	C4ValueArray *ptx = vptx.getArray(), *pty = vpty.getArray();
	if (!ptx || !pty || ptx->GetSize() != pty->GetSize())
		throw new C4AulExecError("C4MapScriptAlgoPolygon: Expected two equally sized int arrays in properties \"X\" and \"Y\".");
	poly.resize(ptx->GetSize());
	for (int32_t i=0; i<ptx->GetSize(); ++i)
	{
		poly[i].x = ptx->GetItem(i).getInt();
		poly[i].y = pty->GetItem(i).getInt();
	}
	wdt = props->GetPropertyInt(P_Wdt);
	if (!wdt) wdt = 1;
	empty = !!props->GetPropertyInt(P_Empty);
	open = !!props->GetPropertyInt(P_Open);
	if (open && !empty) throw new C4AulExecError("C4MapScriptAlgoPolygon: Only empty polygons may be open.");
}

uint8_t C4MapScriptAlgoPolygon::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Polygon at x,y: Return 1 for pixels within the polygon or its borders, 0 otherwise
	int32_t crossings = 0;
	for (size_t i=0; i<poly.size(); ++i)
	{
		Pt pt1 = poly[i];
		Pt pt2 = poly[(i+1)%poly.size()];
		// check border line distance
		int32_t pdx = pt2.x-pt1.x, pdy = pt2.y-pt1.y, dx = x-pt1.x, dy = y-pt1.y;
		if (i!=poly.size()-1 || !open)
		{
			int64_t d = dx*pdy-dy*pdx;
			int32_t lsq = (pdx*pdx+pdy*pdy);
			if (d*d < wdt*wdt*lsq) // check distance perpendicular to line
			{
				if (Inside(dx*pdx+dy*pdy, 0, lsq)) // check if point lies within pt1 and pt2
					return true; // x/y lies on this line
			}
		}
		// check point distance
		if (dx*dx+dy*dy < wdt*wdt) return true; // x/y lies close to edge point
		// filling of polygon: point is contained if it crosses an off number of borders
		if (!empty && (pt1.y<=y) != (pt2.y<=y)) // crossing vertically?
		{
			// does line pt1-pt2 intersect line (x,y)-(inf,y)?
			crossings += (dx>dy*pdx/pdy);
		}
	}
	// x/y lies inside polygon
	return (crossings % 2)==1;
}

C4MapScriptAlgoLines::C4MapScriptAlgoLines(const C4PropList *props)
{
	// Get MAPALGO_Lines properties
	lx = props->GetPropertyInt(P_X);
	ly = props->GetPropertyInt(P_Y);
	if (!lx && !ly) throw new C4AulExecError("C4MapScriptAlgoLines: Invalid direction vector. Either \"X\" or \"Y\" must be nonzero!");
	ox = props->GetPropertyInt(P_OffX);
	oy = props->GetPropertyInt(P_OffY);
		// use sync-safe distance function to calculate line width
	int32_t l = Distance(0,0,lx,ly);
	// default distance: double line width, so lines and gaps have same width
	distance = props->GetPropertyInt(P_Distance);
	if (!distance) distance = l+l; // 1+1=2
	// cache for calculation
	ll = int64_t(lx)*lx+int64_t(ly)*ly;
	dl = int64_t(distance) * l;
}

uint8_t C4MapScriptAlgoLines::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Lines at x,y: Return 1 for pixels contained in lines, 0 for pixels between lines
	int64_t ax = int64_t(x)-ox;
	int64_t ay = int64_t(y)-oy;
	int64_t line_pos = (ax*lx + ay*ly) % dl;
	if (line_pos < 0) line_pos += dl;
	return line_pos < ll;
}

C4MapScriptAlgoModifier::C4MapScriptAlgoModifier(const C4PropList *props, int32_t min_ops, int32_t max_ops)
{
	// Evaluate "Op" property of all algos that take another algo or layer as an operand
	// Op may be a proplist or an array of proplists
	C4Value vops; int32_t n; C4ValueArray temp_ops;
	props->GetProperty(P_Op, &vops);
	C4ValueArray *ops = vops.getArray();
	if (!ops)
	{
		C4PropList *op = vops.getPropList();
		if (op)
		{
			temp_ops.SetItem(0, vops);
			ops = &temp_ops;
			n = 1;
		}
	}
	else
	{
		n = ops->GetSize();
	}
	if (!ops || n<min_ops || (max_ops && n>max_ops))
		throw new C4AulExecError(FormatString("C4MapScriptAlgo: Expected between %d and %d operands in property \"Op\".", (int)min_ops, (int)max_ops).getData());
	operands.resize(n);
	try
	{
		// can easily crash this by building a recursive prop list
		// unfortunately, protecting against that is not trivial
		for (int32_t i=0; i<n; ++i)
		{
			C4MapScriptAlgo *new_algo = FnParAlgo(ops->GetItem(i).getPropList());
			if (!new_algo) throw new C4AulExecError(FormatString("C4MapScriptAlgo: Operand %d in property \"Op\" not valid.", (int)min_ops, (int)max_ops).getData());
			operands[i] = new_algo;
		}
	}
	catch (...)
	{
		Clear();
		throw;
	}
}

void C4MapScriptAlgoModifier::Clear()
{
	// Child algos are owned by this algo, so delete them
	for (std::vector<C4MapScriptAlgo *>::iterator i=operands.begin(); i != operands.end(); ++i) delete *i;
	operands.clear();
}

uint8_t C4MapScriptAlgoAnd::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_And at x,y: 
	// Return 0 if any of the operands is 0. Otherwise, returns value of last operand.
	uint8_t val=0;
	for (std::vector<C4MapScriptAlgo *>::const_iterator i=operands.begin(); i != operands.end(); ++i)
		if (!(val=(**i)(x,y))) return false;
	return val;
}

uint8_t C4MapScriptAlgoOr::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Or at x,y: 
	// Return first nonzero operand
	uint8_t val;
	for (std::vector<C4MapScriptAlgo *>::const_iterator i=operands.begin(); i != operands.end(); ++i)
		if (val=(**i)(x,y)) return val;
	// If all operands are zero, return zero.
	return 0;
}

uint8_t C4MapScriptAlgoNot::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Not at x,y: 
	assert(operands.size()==1);
	// Return zero if operand is set and one otherwise
	return !(*operands[0])(x,y);
}

uint8_t C4MapScriptAlgoXor::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Xor at x,y: 
	assert(operands.size()==2);
	// If exactly one of the two operands is nonzero, return it. Otherwise, return zero.
	uint8_t v1=(*operands[0])(x,y), v2=(*operands[1])(x,y);
	return v1?v2 ?0: v1:v2;
}

C4MapScriptAlgoOffset::C4MapScriptAlgoOffset(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1)
{
	// Get MAPALGO_Offset properties
	ox = props->GetPropertyInt(P_OffX);
	oy = props->GetPropertyInt(P_OffY);
}

uint8_t C4MapScriptAlgoOffset::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Offset at x,y: 
	assert(operands.size()==1);
	// Return base layer shifted by ox,oy
	return (*operands[0])(x-ox,y-oy);
}

C4MapScriptAlgoScale::C4MapScriptAlgoScale(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1)
{
	// Get MAPALGO_Scale properties
	sx = props->GetPropertyInt(P_X);
	sy = props->GetPropertyInt(P_Y);
	if (!sx) sx=100;
	if (!sy) sy=100;
	cx = props->GetPropertyInt(P_OffX);
	cy = props->GetPropertyInt(P_OffY);
}

uint8_t C4MapScriptAlgoScale::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Scale at x,y: 
	assert(operands.size()==1);
	// Return base layer scaled by sx,sy percent from fixed point cx,cy
	return (*operands[0])((x-cx)*100/sx+cx,(y-cy)*100/sy+cy);
}

C4MapScriptAlgoRotate::C4MapScriptAlgoRotate(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1)
{
	// Get MAPALGO_Rotate properties
	int32_t r = props->GetPropertyInt(P_R);
	sr=fixtoi(Sin(itofix(r)), Precision);
	cr=fixtoi(Cos(itofix(r)), Precision);
	ox = props->GetPropertyInt(P_OffX);
	oy = props->GetPropertyInt(P_OffY);
}

uint8_t C4MapScriptAlgoRotate::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Rotate at x,y: 
	assert(operands.size()==1);
	// Return base layer rotated by angle r around point ox,oy
	x-=ox; y-=oy;
	return (*operands[0])(x*cr/Precision-y*sr/Precision+ox,x*sr/Precision+y*cr/Precision+oy);
}

C4MapScriptAlgoTurbulence::C4MapScriptAlgoTurbulence(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1)
{
	// Get MAPALGO_Turbulence properties
	seed = props->GetPropertyInt(P_Seed);
	if (!seed) seed = Random(65536);
	GetXYProps(props, P_Amplitude, amp, true);
	GetXYProps(props, P_Scale, scale, true);
	if (!scale[0]) scale[0] = 10; if (!scale[1]) scale[1] = 10;
	if (!amp[0] && !amp[1]) { amp[0] = amp[1] = 10; }
	iterations = props->GetPropertyInt(P_Iterations);
	if (!iterations) iterations = 2;
}

uint8_t C4MapScriptAlgoTurbulence::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Turbulence at x,y: 
	// move by a random offset iterations times
	assert(operands.size()==1);
	int32_t xy[] = {x, y};
	for (int32_t iter=0; iter<iterations; ++iter)
	{
		int32_t s[2], p[2];
		for (int dim=0; dim<2; ++dim)
		{
			s[dim] = divD(xy[dim], scale[dim]);
			p[dim] = modD(xy[dim], scale[dim]);
		}
		int32_t a[2][2];
		for (int dim=0; dim<2; ++dim)
		{
			int32_t aamp = amp[dim] / (iter+1);
			if (!aamp) continue;
			for (int dx=0; dx<2; ++dx) for (int dy=0; dy<2; ++dy) a[dx][dy] = QuerySeededRandomField(seed+dim, s[0]+dx, s[1]+dy, aamp) - aamp/2;
			int32_t a_interp = a[0][0]*(scale[0]-p[0])*(scale[1]-p[1])
							 + a[1][0]*(         p[0])*(scale[1]-p[1])
							 + a[0][1]*(scale[0]-p[0])*(         p[1])
							 + a[1][1]*(         p[0])*(         p[1]);
			xy[dim] += a_interp / (scale[0]*scale[1]);
		}
	}
	return (*operands[0])(xy[0],xy[1]);
}

void C4MapScriptAlgoBorder::ResolveBorderProps(int32_t *p)
{
	// Converts arrays in MAPALGO_Border properties to array of [inner border, outer border]
	// Input: Negative values mark outer borders; positive values mark inner borders
	int32_t inner=0, outer=0;
	for (int32_t i=0; i<2; ++i) if (p[i]>0) inner=p[i]; else if (p[i]<0) outer=-p[i];
	p[0] = inner; p[1] = outer;
}

C4MapScriptAlgoBorder::C4MapScriptAlgoBorder(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1)
{
	// Get MAPALGO_Border properties
	int32_t wdt[2] = {0,0};
	// Parameter Wdt fills all directions
	int32_t n_borders = 0;
	n_borders += GetXYProps(props, P_Wdt, wdt, false);
	for (int32_t i=0; i<2; ++i) left[i]=top[i]=right[i]=bottom[i]=wdt[i];
	// Individual direction parameters
	n_borders += GetXYProps(props, P_Left, left, false);
	n_borders += GetXYProps(props, P_Top, top, false);
	n_borders += GetXYProps(props, P_Right, right, false);
	n_borders += GetXYProps(props, P_Bottom, bottom, false);
	// Resolve negative/positive values to inner/outer borders
	ResolveBorderProps(left);
	ResolveBorderProps(top);
	ResolveBorderProps(right);
	ResolveBorderProps(bottom);
	// If nothing was specified, fill all directions with a default: Draw 1px of outer border
	if (!n_borders)
	{
		left[1] = top[1] = right[1] = bottom[1] = 1;
	}
}

uint8_t C4MapScriptAlgoBorder::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Border at x,y: Check if position is at a border of operand layer
	// For borders inside operand layer, return the operand material. For outside borders, just return 1. For non-borders, return 0.
	// Are we inside or outside?
	const C4MapScriptAlgo &l = *operands[0];
	uint8_t mat = l(x,y);
	bool inside = !!mat;
	if (!mat) mat=1; // return 1 for outside borders
	// Check four sideways directions
	const int32_t *ymove[] = { top, bottom }, *xmove [] ={ left, right };
	const int32_t d[] = { -1, +1 };
	for (int32_t dir=0; dir<2; ++dir)
	{
		int32_t hgt = ymove[inside!=!dir][!inside];
		for (int32_t dy=0; dy<hgt; ++dy)
			if (inside==!l(x,y+d[dir]*(dy+1)))
				return mat;
		int32_t wdt = xmove[inside!=!dir][!inside];
		for (int32_t dx=0; dx<wdt; ++dx)
			if (inside==!l(x+d[dir]*(dx+1),y))
				return mat;
	}
	// Not on border
	return 0;
}

C4MapScriptAlgoFilter::C4MapScriptAlgoFilter(const C4PropList *props) : C4MapScriptAlgoModifier(props,1,1)
{
	// Get MAPALGO_Filter properties
	C4Value spec;
	if (!props->GetProperty(P_Filter, &spec))
		throw new C4AulExecError("MapScriptAlgoFilter without Filter property.");
	filter.Init(spec);
}

uint8_t C4MapScriptAlgoFilter::operator () (int32_t x, int32_t y) const
{
	// Evaluate MAPALGO_Filter at x,y:
	// Return original color if it's marked to go through filter
	uint8_t col = (*operands[0])(x,y);
	return filter(col) ? col : 0;
}

C4MapScriptAlgo *FnParAlgo(C4PropList *algo_par)
{
	// Convert script function parameter to internal C4MapScriptAlgo class. Also resolve all parameters and nested child algos.
	if (!algo_par) return NULL;
	// if algo is a layer, take that directly
	C4MapScriptLayer *algo_layer = algo_par->GetMapScriptLayer();
	if (algo_layer) return new C4MapScriptAlgoLayer(algo_layer);
	// otherwise, determine by proplist parameter "algo"
	switch (algo_par->GetPropertyInt(P_Algo))
	{
	case MAPALGO_Layer:       return new C4MapScriptAlgoLayer(algo_par);
	case MAPALGO_RndChecker:  return new C4MapScriptAlgoRndChecker(algo_par);
	case MAPALGO_And:         return new C4MapScriptAlgoAnd(algo_par);
	case MAPALGO_Or:          return new C4MapScriptAlgoOr(algo_par);
	case MAPALGO_Xor:         return new C4MapScriptAlgoXor(algo_par);
	case MAPALGO_Not:         return new C4MapScriptAlgoNot(algo_par);
	case MAPALGO_Offset:      return new C4MapScriptAlgoOffset(algo_par);
	case MAPALGO_Scale:       return new C4MapScriptAlgoScale(algo_par);
	case MAPALGO_Rotate:      return new C4MapScriptAlgoRotate(algo_par);
	case MAPALGO_Rect:        return new C4MapScriptAlgoRect(algo_par);
	case MAPALGO_Ellipsis:    return new C4MapScriptAlgoEllipsis(algo_par);
	case MAPALGO_Polygon:     return new C4MapScriptAlgoPolygon(algo_par);
	case MAPALGO_Lines:       return new C4MapScriptAlgoLines(algo_par);
	case MAPALGO_Turbulence:  return new C4MapScriptAlgoTurbulence(algo_par);
	case MAPALGO_Border:      return new C4MapScriptAlgoBorder(algo_par);
	case MAPALGO_Filter:      return new C4MapScriptAlgoFilter(algo_par);
	default:
		throw new C4AulExecError(FormatString("got invalid algo: %d", algo_par->GetPropertyInt(P_Algo)).getData());
	}
	return NULL;
}
