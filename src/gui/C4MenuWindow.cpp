/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007-2008  Matthes Bender
 * Copyright (c) 2001-2008  Sven Eberhardt
 * Copyright (c) 2004-2011  Günther Brammer
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2012  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

 /* a flexisble ingame menu system that can be composed out of multiple windows */

#include <C4Include.h>
#include <C4MenuWindow.h>

#include <C4Application.h>
#include <C4DefList.h>
#include <C4GraphicsResource.h>
#include <C4Game.h>
#include <C4MouseControl.h>
#include <C4Object.h>
#include <C4Player.h>
#include <C4PlayerList.h>



const float standardVerticalBorder = 100.0f;
const float standardHorizontalBorder = 100.0f;

C4MenuWindow MenuWindowRoot = C4MenuWindow(standardVerticalBorder, standardHorizontalBorder);

C4MenuWindowAction::~C4MenuWindowAction()
{
	if (text)
		text->DecRef();
}

void C4MenuWindowAction::ClearPointers(C4Object *pObj)
{
	if (target == pObj)
	{
		// not only forget object, but completely invalidate action
		action = 0;
		target = 0;
	}
}
bool C4MenuWindowAction::Init(C4ValueArray *array)
{
	if (array->GetSize() == 0) // safety
		return false;

	action = array->GetItem(0).getInt();

	switch (action)
	{
	case C4MenuWindowActionID::Call:
		if (array->GetSize() < 3) return false;
		target = array->GetItem(1).getObj();
		text = array->GetItem(2).getStr();
		if (!target || !text) return false;
		text->IncRef();
		break;

	case C4MenuWindowActionID::SetTag:
		if (array->GetSize() < 4) return false;
		target = array->GetItem(1).getObj();
		subwindowID = array->GetItem(2).getInt();
		text = array->GetItem(3).getStr();
		if (!text) return false;
		text->IncRef();
		break;

	default:
		return false;
	}

	return true;
}

void C4MenuWindowAction::Execute(C4MenuWindow *parent)
{
	assert(parent && "C4MenuWindow::Execute must always be called with parent");

	// invalid ID? can be set by removal of target object
	if (!action) return;

	// get menu main window
	C4MenuWindow *main = parent;
	C4MenuWindow *from = main;
	while (from->parent)
	{
		main = from;
		from = from->parent;
	}

	switch (action)
	{
	case C4MenuWindowActionID::Call:
	{
		if (!target) // ohject removed in the meantime?
			return;

		C4AulParSet Pars(C4VInt(main->GetID()), C4VInt(parent->GetID()));
		target->Call(text->GetCStr(), &Pars);
		break;
	}

	case C4MenuWindowActionID::SetTag:
	{
		C4MenuWindow *window = main;
		if (subwindowID == 0)
			window = parent;
		else if (subwindowID > 0)
			window = main->GetSubWindow(subwindowID, target);
		if (window)
			window->SetTag(text);
		break;
	}

	default:
		assert(false && "C4MenuWindowAction without valid or invalidated ID");
		break;
	}
}

C4MenuWindowScrollBar::C4MenuWindowScrollBar() : offset(0.5f), decoration(0), parent(0)
{

}

C4MenuWindowScrollBar::~C4MenuWindowScrollBar()
{

}

void C4MenuWindowScrollBar::Draw(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom)
{
	C4GUI::ScrollBarFacets &facets = decoration ? *decoration : ::GraphicsResource.sfctScroll;
	C4GUI::DynBarFacet bar = facets.barScroll;

	float off = 25.0f;
	float x = cgo.X + parentRight - off;
	float yOrigin = cgo.Y + parentTop + off;
	float hgt = (parentBottom - parentTop) - off;

	bar.fctBegin.Draw(cgo.Surface, x, yOrigin);
	float yOffset = bar.fctBegin.Hgt;

	float segmentHeight = bar.fctMiddle.Hgt;
	int32_t steps = (hgt-5 - yOffset) / segmentHeight;

	for (int32_t i = 0; i < steps; ++i)
	{
		float currentHeight = segmentHeight;
		float toBottom = hgt-5 - yOffset;
		if (currentHeight > toBottom) currentHeight = toBottom;

		bar.fctMiddle.Hgt = currentHeight;
		bar.fctMiddle.Draw(cgo.Surface, x, yOrigin + yOffset);
		yOffset += segmentHeight;
	}
	bar.fctMiddle.Hgt = segmentHeight;
	bar.fctEnd.Draw(cgo.Surface, x, yOrigin + yOffset - bar.fctEnd.Hgt);
}

bool C4MenuWindowScrollBar::MouseInput(int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam)
{
	if (::MouseControl.IsLeftDown())
	{
		// calculate new position
		float wdt = parent->lastDrawPosition.bottom - parent->lastDrawPosition.top;
		if (wdt != 0.0f)
			offset = (mouseY - parent->lastDrawPosition.top) / wdt;
		return true;
	}
	return false;
}

C4MenuWindowProperty::~C4MenuWindowProperty()
{
	// is cleaned up from destructor of C4MenuWindow
}

void C4MenuWindowProperty::CleanUp(Prop &prop)
{
	switch (type)
	{
	case frameDecoration:
		if (prop.deco) delete prop.deco;
		break;
	case onClickAction:
		if (prop.action) delete prop.action;
		break;
	case text:
		if (prop.strBuf) delete prop.strBuf;
		break;
	default:
		break;
	}
}

void C4MenuWindowProperty::CleanUpAll()
{
	for (std::map<unsigned int, Prop>::iterator iter = taggedProperties.begin(); iter != taggedProperties.end(); ++iter)
		CleanUp(iter->second);
}

void C4MenuWindowProperty::Set(const C4Value &value, unsigned int hash)
{
	C4PropList *proplist = value.getPropList();
	bool isTaggedPropList = false;
	if (proplist)
		isTaggedPropList = !(proplist->GetDef() || proplist->GetObject());

	if (isTaggedPropList)
	{
		C4ValueArray *properties = proplist->GetProperties();

		for (int32_t i = 0; i < properties->GetSize(); ++i)
		{
			const C4Value &entry = properties->GetItem(i);
			C4String *key = entry.getStr();
			assert(key && "Proplist returns non-string as key");

			C4Value property;
			proplist->GetPropertyByS(key, &property);
			Set(property, key->Hash);
		}
		return;
	}

	// special treatment for some that have to be deleted (due to owning string/frame deco/...)
	if (taggedProperties.count(hash))
		CleanUp(taggedProperties[hash]);

	taggedProperties[hash] = Prop();
	// in order to make /current/ sane, always reset it - not relying about implementation of std::map
	// if the user wants a special tag selected, he should do that (standard selection will still be "Std")
	current = &taggedProperties[hash];

	switch (type)
	{
	case left:
	case right:
	case top:
	case bottom:
	case backgroundColor:
		current->d = value.getInt();
		break;

	case relLeft:
	case relRight:
	case relTop:
	case relBottom:
		current->f = float(value.getInt()) / 1000.0f;
		break;
	case symbolObject:
	{
		C4PropList *symbol = value.getPropList();
		if (symbol)
			current->obj = symbol->GetObject();
		else current->def = 0;
		break;
	}
	case symbolDef:
	{
		C4PropList *symbol = value.getPropList();
		if (symbol)
			current->def = symbol->GetDef();
		else current->def = 0;
		break;
	}
	case frameDecoration:
	{
		C4ID id = value.getC4ID();
		if (id != C4ID::None)
			if (C4Id2Def(id))
			{
				current->deco = new C4GUI::FrameDecoration();
				if (!current->deco->SetByDef(id))
				{
					delete current->deco;
					current->deco = 0;
				}
			}
		break;
	}
	case text:
	{
		C4String *string = value.getStr();
		StdCopyStrBuf *buf = new StdCopyStrBuf();
		if (string)
			buf->Copy(string->GetCStr());
		else buf->Copy("");
		current->strBuf = buf;
		break;
	}
	case onClickAction:
	{
		C4ValueArray *array = value.getArray();
		if (array)
		{
			current->action = new C4MenuWindowAction();
			if (!current->action->Init(array))
			{
				delete current->action;
				current->action = 0;
			}
		}
		break;
	}

	default:
		assert (false && "C4MenuWindowAction should never have undefined type");
		break;
	} // switch
}

void C4MenuWindowProperty::ClearPointers(C4Object *pObj)
{
	// assume that we actually contain an object
	for (std::map<unsigned int, Prop>::iterator iter = taggedProperties.begin(); iter != taggedProperties.end(); ++iter)
	{
		switch (type)
		{
		case C4MenuWindowPropertyName::symbolObject:
			if (iter->second.obj == pObj)
				iter->second.obj = 0;
		break;

		case C4MenuWindowPropertyName::onClickAction:
			if (iter->second.action)
				iter->second.action->ClearPointers(pObj);
		break;
		default:
			return;
		}
	}
}

void C4MenuWindowProperty::SwitchTag(C4String *tag)
{
	unsigned int hash = tag->Hash;
	if (taggedProperties.count(hash))
		current = &taggedProperties[hash];
}

C4MenuWindow::C4MenuWindow()
{
	Init();
}

C4MenuWindow::C4MenuWindow(float stdBorderX, float stdBorderY)
{
	Init();

	// set border values for "Std"
	unsigned int hash = Strings.P[P_Std].Hash;
	// exact offsets are standard 0
	props[C4MenuWindowPropertyName::left].SetInt(hash, int32_t(stdBorderX));
	props[C4MenuWindowPropertyName::right].SetInt(hash, int32_t(-stdBorderX));
	props[C4MenuWindowPropertyName::top].SetInt(hash, int32_t(stdBorderY));
	props[C4MenuWindowPropertyName::bottom].SetInt(hash, int32_t(-stdBorderY));
}

void C4MenuWindow::Init()
{
	id = 0;
	isMainWindow = false;

	// properties must know what they stand for
	for (int32_t i = 0; i < C4MenuWindowPropertyName::_lastProp; ++i)
		props[i].type = i;

	// standard values for all of the properties
	unsigned int hash = Strings.P[P_Std].Hash;
	// exact offsets are standard 0
	props[C4MenuWindowPropertyName::left].SetNull(hash);
	props[C4MenuWindowPropertyName::right].SetNull(hash);
	props[C4MenuWindowPropertyName::top].SetNull(hash);
	props[C4MenuWindowPropertyName::bottom].SetNull(hash);
	// relative offsets are standard full screen 0,0 - 1,1
	props[C4MenuWindowPropertyName::relLeft].SetNull(hash);
	props[C4MenuWindowPropertyName::relTop].SetNull(hash);
	props[C4MenuWindowPropertyName::relBottom].SetFloat(hash, 1.0f);
	props[C4MenuWindowPropertyName::relRight].SetFloat(hash, 1.0f);
	// other properties are 0
	props[C4MenuWindowPropertyName::backgroundColor].SetNull(hash);
	props[C4MenuWindowPropertyName::frameDecoration].SetNull(hash);
	props[C4MenuWindowPropertyName::symbolObject].SetNull(hash);
	props[C4MenuWindowPropertyName::symbolDef].SetNull(hash);
	props[C4MenuWindowPropertyName::text].SetNull(hash);
	props[C4MenuWindowPropertyName::onClickAction].SetNull(hash);

	parent = 0;
	visible = true;
	target = 0;
	scrollBar = 0;
}

C4MenuWindow::~C4MenuWindow()
{
	ClearChildren(false);

	// delete certain properties that contain allocated elements or referenced strings
	for (int32_t i = 0; i < C4MenuWindowPropertyName::_lastProp; ++i)
		props[i].CleanUpAll();

	if (scrollBar)
		delete scrollBar;
}

// helper function
void C4MenuWindow::SetArrayTupleProperty(const C4Value &property, C4MenuWindowPropertyName first, C4MenuWindowPropertyName second, unsigned int hash)
{
	C4ValueArray *array;
	if (array = property.getArray())
	{
		if (array->GetSize() > 0)
			props[first].Set(array->GetItem(0), hash);
		if (array->GetSize() > 1)
			props[second].Set(array->GetItem(1), hash);
	}
	else props[first].Set(property, hash);
}

bool C4MenuWindow::CreateFromPropList(C4PropList *proplist)
{
	assert(parent && "MenuWindow created from proplist without parent (fails for ID tag)");
	// get properties from proplist
	// make sure to delete the array later, we took ownership
	C4ValueArray *properties = proplist->GetProperties();
	unsigned int standardHash = Strings.P[P_Std].Hash;
	for (int32_t i = 0; i < properties->GetSize(); ++i)
	{
		const C4Value &entry = properties->GetItem(i);
		C4String *key = entry.getStr();
		assert(key && "PropList returns non-string as key");

		C4Value property;
		proplist->GetPropertyByS(key, &property);

		C4Value value;

		if(&Strings.P[P_X] == key)
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relLeft, C4MenuWindowPropertyName::left, standardHash);
		else if(&Strings.P[P_Y] == key)
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relTop, C4MenuWindowPropertyName::top, standardHash);
		else if(&Strings.P[P_Wdt] == key)
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relRight, C4MenuWindowPropertyName::right, standardHash);
		else if(&Strings.P[P_Hgt] == key)
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relBottom, C4MenuWindowPropertyName::bottom, standardHash);
		else if(&Strings.P[P_BackgroundColor] == key)
			props[C4MenuWindowPropertyName::backgroundColor].Set(property, standardHash);
		else if(&Strings.P[P_Target] == key)
			target = property.getObj();
		else if(&Strings.P[P_Symbol] == key)
		{
			props[C4MenuWindowPropertyName::symbolDef].Set(property, standardHash);
			props[C4MenuWindowPropertyName::symbolObject].Set(property, standardHash);
		}
		else if(&Strings.P[P_Decoration] == key)
		{
			props[C4MenuWindowPropertyName::frameDecoration].Set(property, standardHash);
		}
		else if(&Strings.P[P_Text] == key)
			props[C4MenuWindowPropertyName::text].Set(property, standardHash);
		else if(&Strings.P[P_ID] == key)
		{
			// setting IDs is only valid for subwindows!
			if (!isMainWindow)
			{
				if (id) // already have an ID? remove from parent
					parent->ChildWithIDRemoved(this);
				id = property.getInt();
				if (id != 0)
					parent->ChildGotID(this);
			}
			props[C4MenuWindowPropertyName::text].Set(property, standardHash);
		}
		else if(&Strings.P[P_OnClick] == key)
			props[C4MenuWindowPropertyName::onClickAction].Set(property, standardHash);
		else
		{
			// possibly sub-window?
			C4PropList *subwindow = property.getPropList();
			if (subwindow)
			{
				C4MenuWindow *child = new C4MenuWindow();
				AddChild(child);

				if (!child->CreateFromPropList(subwindow))
					RemoveChild(child, false);
			}
		}
	}

	return true;
}

void C4MenuWindow::ClearPointers(C4Object *pObj)
{
	if (target == pObj)
	{
		target = 0;
		Close();
		return;
	}
	props[C4MenuWindowPropertyName::symbolObject].ClearPointers(pObj);
	props[C4MenuWindowPropertyName::onClickAction].ClearPointers(pObj);

	// can't iterate directly over the children, since they might get deleted in the process
	std::vector<C4MenuWindow*> temp;
	temp.reserve(children.size());
	temp.assign(children.begin(), children.end());
	for (std::vector<C4MenuWindow*>::iterator iter = temp.begin(); iter != temp.end(); ++iter)
		(*iter)->ClearPointers(pObj);
}

C4MenuWindow *C4MenuWindow::AddChild(C4MenuWindow *child)
{
	child->parent = this;
	// are we the root menu? Is this a "main" submenu?
	if (!parent)
	{
		child->SetID(GenerateMenuID());
		child->isMainWindow = true;
	}
	children.push_back(child);
	return child;
}

void C4MenuWindow::ChildWithIDRemoved(C4MenuWindow *child)
{
	if (!parent) return;
	if (!isMainWindow)
		return parent->ChildWithIDRemoved(child);
	std::pair<std::multimap<int32_t, C4MenuWindow*>::iterator, std::multimap<int32_t, C4MenuWindow*>::iterator> range;
	range = childrenIDMap.equal_range(child->GetID());

	for (std::multimap<int32_t, C4MenuWindow*>::iterator iter = range.first; iter != range.second; ++iter)
	{
		if (iter->second != child) continue;
		childrenIDMap.erase(iter);
		return;
	}
}

void C4MenuWindow::ChildGotID(C4MenuWindow *child)
{
	assert (parent && "ChildGotID called on window root, should not propagate over main windows!");
	if (!isMainWindow)
		return parent->ChildGotID(child);
	childrenIDMap.insert(std::make_pair(child->GetID(), child));
}

C4MenuWindow *C4MenuWindow::GetChildByID(int32_t child)
{
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		if ((*iter)->id != child) continue;
		return *iter;
	}
	return 0;
}

C4MenuWindow *C4MenuWindow::GetSubWindow(int32_t childID, C4Object *childTarget)
{
	std::pair<std::multimap<int32_t, C4MenuWindow*>::iterator, std::multimap<int32_t, C4MenuWindow*>::iterator> range;
	range = childrenIDMap.equal_range(childID);

	for (std::multimap<int32_t, C4MenuWindow*>::iterator iter = range.first; iter != range.second; ++iter)
	{
		C4MenuWindow *subwindow = iter->second;
		if (subwindow->GetTarget() != childTarget) continue;
		return subwindow;
	}
	return 0;
}

void C4MenuWindow::RemoveChild(C4MenuWindow *child, bool close)
{
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		if (*iter != child) continue;
		// don't delete twice
		(*iter)->parent = 0;
		// close properly (calls etc.?)
		if (close)
			(*iter)->Close();

		// if the child had an quick-access entry, remove it
		if ((*iter)->GetID() != 0)
			ChildWithIDRemoved(*iter);

		delete *iter;
		children.erase(iter);
		return;
	}
}

void C4MenuWindow::ClearChildren(bool close)
{
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		(*iter)->parent = 0; // don't delete twice

		if (close)
			(*iter)->Close();
		delete *iter;
	}
	children.clear();
}

void C4MenuWindow::Close()
{
	// if we have a parent, make it forget us at end of call
	// when delete from the parent, "parent" is 0
	C4MenuWindow *_parent = parent;
	parent = 0;

	// first, close all children and dispose of them properly
	ClearChildren(true);

	// make call to target object if applicable
	if (target)
	{
		// todo
	}

	if (_parent)
		_parent->RemoveChild(this);
}

void C4MenuWindow::DrawChildren(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom)
{
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
		(*iter)->Draw(cgo, player, parentLeft, parentTop, parentRight, parentBottom);
}

void C4MenuWindow::Draw(C4TargetFacet &cgo, int32_t player)
{
	if (!IsVisible()) return;
	// assume I am the root and use the whole viewport for drawing - minus some standard border
	const int32_t &left = props[C4MenuWindowPropertyName::left].GetInt();
	const int32_t &right = props[C4MenuWindowPropertyName::right].GetInt();
	const int32_t &top = props[C4MenuWindowPropertyName::top].GetInt();
	const int32_t &bottom = props[C4MenuWindowPropertyName::bottom].GetInt();
	DrawChildren(cgo, player, cgo.X + left, cgo.Y + top, cgo.X + cgo.Wdt * cgo.Zoom + right, cgo.Y + cgo.Hgt * cgo.Zoom + bottom);
}

void C4MenuWindow::Draw(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom)
{
	// message hidden?
	if (!IsVisible()) return;

	// player can see message?
	if (target)
		if (!target->IsVisible(player, false))
			return;

	// fetch current position as shortcut for overview
	const int32_t &left = props[C4MenuWindowPropertyName::left].GetInt();
	const int32_t &right = props[C4MenuWindowPropertyName::right].GetInt();
	const int32_t &top = props[C4MenuWindowPropertyName::top].GetInt();
	const int32_t &bottom = props[C4MenuWindowPropertyName::bottom].GetInt();

	const float &relLeft = props[C4MenuWindowPropertyName::relLeft].GetFloat();
	const float &relRight = props[C4MenuWindowPropertyName::relRight].GetFloat();
	const float &relTop = props[C4MenuWindowPropertyName::relTop].GetFloat();
	const float &relBottom = props[C4MenuWindowPropertyName::relBottom].GetFloat();

	// calculate drawing rectangle
	float parentWidth = parentRight - parentLeft;
	float parentHeight = parentBottom - parentTop;
	float leftDrawX = parentLeft + relLeft * parentWidth + float(left);
	float rightDrawX = parentLeft + relRight * parentWidth + float(right);
	float topDrawY = parentTop + relTop * parentHeight + float(top);
	float bottomDrawY = parentTop + relBottom * parentHeight + float(bottom);
	float width = rightDrawX - leftDrawX;
	float height = bottomDrawY - topDrawY;
	float childOffsetY = 0.0f; // for scrolling

	// update last drawing position if changed, needed for input etc.
	if (leftDrawX != lastDrawPosition.left)
	{
		lastDrawPosition.left = leftDrawX;
		lastDrawPosition.right = rightDrawX;
		lastDrawPosition.top = topDrawY;
		lastDrawPosition.bottom = bottomDrawY;

		// host should update asap
		parent->lastDrawPosition.dirty = 1;
	}

	// do we need to update children positions etc.?
	if (lastDrawPosition.dirty)
	{
		lastDrawPosition.dirty = 0;
		bool first = true;
		for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
		{
			C4MenuWindow *child = *iter;
			// offset is against rounding weirdnesses
			if (first || (child->lastDrawPosition.top < lastDrawPosition.topMostChild))
				lastDrawPosition.topMostChild = child->lastDrawPosition.top + 1.0f;
			if (first || (child->lastDrawPosition.bottom > lastDrawPosition.bottomMostChild))
				lastDrawPosition.bottomMostChild = child->lastDrawPosition.bottom - 1.0f;
			first = false;
		}
	}
	// check whether we need a scroll-bar and then add it
	float childHgt = lastDrawPosition.bottomMostChild - lastDrawPosition.topMostChild;
	bool needScrollBar = (lastDrawPosition.dirty == 0) && (childHgt > height);

	if (!needScrollBar && scrollBar)
	{
		delete scrollBar; scrollBar = 0;
	}
	else if (needScrollBar && !scrollBar)
	{
		scrollBar = new C4MenuWindowScrollBar();
		scrollBar->parent = this;
	}

	if (scrollBar)
		childOffsetY = -0.5f * (scrollBar->offset * childHgt);

	// if ANY PARENT has scroll bar, then adjust clipper
	float clipX1, clipX2, clipY1, clipY2;
	bool clipping = parent->GetClippingRect(clipX1, clipY1, clipX2, clipY2);
	if (clipping)
	{
		pDraw->StorePrimaryClipper();
		pDraw->SetPrimaryClipper(int32_t(clipX1), int32_t(clipY1), int32_t(clipX2), int32_t(clipY2));
	}

	// draw various properties
	C4Facet cgoOut(cgo.Surface, leftDrawX, topDrawY, width, height);

	const int32_t &backgroundColor = props[C4MenuWindowPropertyName::backgroundColor].GetInt();
	if (backgroundColor)
		pDraw->DrawBoxDw(cgo.Surface, leftDrawX, topDrawY, rightDrawX - 1.0f, bottomDrawY - 1.0f, backgroundColor);

	C4GUI::FrameDecoration *frameDecoration = props[C4MenuWindowPropertyName::frameDecoration].GetFrameDecoration();

	if (frameDecoration)
	{
		C4Rect rect(leftDrawX - cgo.TargetX, topDrawY - cgo.TargetY, width, height);
		frameDecoration->Draw(cgo, rect);
	}

	C4Object *symbolObject = props[C4MenuWindowPropertyName::symbolObject].GetObject();
	if (symbolObject)
	{
		symbolObject->DrawPicture(cgoOut, false, NULL);
	}
	else
	{
		C4Def *symbolDef = props[C4MenuWindowPropertyName::symbolDef].GetDef();
		if (symbolDef)
		{
			symbolDef->Draw(cgoOut);
		}
	}

	StdCopyStrBuf *strBuf = props[C4MenuWindowPropertyName::text].GetStrBuf();

	if (strBuf)
	{
		StdStrBuf sText;
		int32_t textHgt = ::GraphicsResource.FontRegular.BreakMessage(strBuf->getData(), int32_t(width), &sText, true);
		pDraw->TextOut(sText.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface, cgo.X + leftDrawX, cgo.Y + topDrawY, 0xffffffff, ALeft);
		// enable auto scroll
		float textBottom = topDrawY + float(textHgt);
		if (textBottom > lastDrawPosition.bottom)
			lastDrawPosition.bottom = textBottom;
	}

	if (clipping)
		pDraw->RestorePrimaryClipper();

	if (scrollBar)
		scrollBar->Draw(cgo, player, leftDrawX, topDrawY, rightDrawX, bottomDrawY);

	DrawChildren(cgo, player, leftDrawX, topDrawY + childOffsetY, rightDrawX, bottomDrawY + childOffsetY);
}

bool C4MenuWindow::GetClippingRect(float &left, float &top, float &right, float &bottom)
{
	if (scrollBar)
	{
		left = lastDrawPosition.left;
		right = lastDrawPosition.right;
		top = lastDrawPosition.top;
		bottom = lastDrawPosition.bottom;
		return true;
	}
	if (parent)
		return parent->GetClippingRect(left, top, right, bottom);
	return false;
}

void C4MenuWindow::SetTag(C4String *tag)
{
	// set tag on all properties
	for (uint32_t i = 0; i < C4MenuWindowPropertyName::_lastProp; ++i)
		props[i].SwitchTag(tag);

	// .. and children
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
		(*iter)->SetTag(tag);
}

bool C4MenuWindow::MouseInput(int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam)
{
	if (!visible) return false;

	/*if (parent && mouse.IsLDown())
		LogF("%f|%f - %d|%d", lastDrawPosition.left, lastDrawPosition.top, mouseX, mouseY);
	else LogF("called on root %d|%d", mouseX, mouseY);*/

	// children actually have a higher priority
	bool overChild = false;
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		C4MenuWindow *child = *iter;
		int32_t childLeft = static_cast<int32_t>(child->lastDrawPosition.left);
		int32_t childRight = static_cast<int32_t>(child->lastDrawPosition.right);
		int32_t childTop = static_cast<int32_t>(child->lastDrawPosition.top);
		int32_t childBottom = static_cast<int32_t>(child->lastDrawPosition.bottom);
		//LogF("%d|%d in %d|%d // %d|%d", mouseX, mouseY, childLeft, childTop, childRight, childBottom);
		if ((mouseX < childLeft) || (mouseX > childRight)) continue;
		if ((mouseY < childTop) || (mouseY > childBottom)) continue;

		overChild = true;
		if (child->MouseInput(button, mouseX, mouseY, dwKeyParam))
			return true;
	}

	if (button == C4MC_Button_LeftDown)
	{
		C4MenuWindowAction *action = props[C4MenuWindowPropertyName::onClickAction].GetAction();
		if (action)
		{
			action->Execute(this);
			return true;
		}
	}

	// for scroll-enabled windows, scroll contents with wheel
	if (scrollBar && (button == C4MC_Button_Wheel))
	{
		short delta = (short)(dwKeyParam >> 16);
		float fac = (lastDrawPosition.right - lastDrawPosition.left) / 10.0f;
		if (fac == 0.0f) fac = 1.0f;
		scrollBar->ScrollBy(-float(delta) / fac);
		return true;
	}

	// forward to scroll-bar if in area
	if (scrollBar)
	{
		if (mouseX >= lastDrawPosition.right - 50.0f)
			return scrollBar->MouseInput(button, mouseX, mouseY, dwKeyParam);
	}

	// if the user still clicked on a menu - even if it didn't do anything, catch it
	// but do that only on the top-level to not stop traversin other branches
	if (isMainWindow)
		return overChild;
	return false;
}
