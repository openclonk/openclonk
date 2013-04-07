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
#include <C4GraphicsSystem.h>
#include <C4GraphicsResource.h>
#include <C4Game.h>
#include <C4Control.h>
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
	if (nextAction)
		delete nextAction;
}

void C4MenuWindowAction::ClearPointers(C4Object *pObj)
{
	C4Object *targetObj = target ? target->GetObject() : 0;

	if (targetObj == pObj)
	{
		// not only forget object, but completely invalidate action
		action = 0;
		target = 0;
	}
	if (nextAction)
		nextAction->ClearPointers(pObj);
}
bool C4MenuWindowAction::Init(C4ValueArray *array, int32_t index)
{
	if (array->GetSize() == 0) // safety
		return false;

	// an array of actions?
	if (array->GetItem(0).getArray())
	{
		//LogF("..initing multi-action event (index %d)", index);
		// add action to action chain?
		if (index+1 < array->GetSize())
		{
			nextAction = new C4MenuWindowAction();
			nextAction->Init(array, index + 1);
		}
		// continue with one sub array
		array = array->GetItem(index).getArray();
		if (!array) return false;
	}
	// retrieve type of action
	int newAction = array->GetItem(0).getInt();
	action = 0; // still invalid!

	switch (newAction)
	{
	case C4MenuWindowActionID::Call:
		if (array->GetSize() < 3) return false;
		target = array->GetItem(1).getPropList();
		text = array->GetItem(2).getStr();
		if (!target || !text) return false;
		if (array->GetSize() >= 4)
			value = C4Value(array->GetItem(3));
		text->IncRef();

		// important! needed to identify actions later!
		id = ::MenuWindowRoot.GenerateActionID();

		break;

	case C4MenuWindowActionID::SetTag:
		if (array->GetSize() < 4) return false;
		target = array->GetItem(1).getObj(); // getObj on purpose. Need to validate that.
		subwindowID = array->GetItem(2).getInt();
		text = array->GetItem(3).getStr();
		if (!text) return false;
		text->IncRef();
		break;

	default:
		return false;
	}

	action = newAction;
	return true;
}

void C4MenuWindowAction::Execute(C4MenuWindow *parent, int32_t player, unsigned int tag, int32_t actionType)
{
	assert(parent && "C4MenuWindow::Execute must always be called with parent");
	//LogF("Excuting action (nextAction: %x, subwID: %d, target: %x, text: %s, type: %d)", nextAction, subwindowID, target, text->GetCStr(), actionType);
	// invalid ID? can be set by removal of target object
	if (action)
	{
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
				break;
			// the action needs to be synchronized! Assemble command and put it into control queue!
			Game.Input.Add(CID_MenuCommand, new C4ControlMenuCommand(id, player, main->GetID(), parent->GetID(), parent->target, tag, actionType));
			break;
		}

		case C4MenuWindowActionID::SetTag:
		{
			C4MenuWindow *window = main;
			if (subwindowID == 0)
				window = parent;
			else if (subwindowID > 0)
			{
				C4Object *targetObj = dynamic_cast<C4Object*> (target);
				window = main->GetSubWindow(subwindowID, targetObj);
			}
			if (window)
				window->SetTag(text);
			break;
		}

		default:
			assert(false && "C4MenuWindowAction without valid or invalidated ID");
			break;
		}
	} // action

	if (nextAction)
	{
		nextAction->Execute(parent, player, tag, actionType);
	}
}

bool C4MenuWindowAction::ExecuteCommand(int32_t actionID, C4MenuWindow *parent, int32_t player)
{
	// target has already been checked for validity
	if (id == actionID && action)
	{
		assert(action == C4MenuWindowActionID::Call && "C4ControlMenuCommand for invalid action!");

		// get menu main window
		C4MenuWindow *main = parent;
		C4MenuWindow *from = main;
		while (from->parent)
		{
			main = from;
			from = from->parent;
		}
		//LogF("command synced.. target: %x, targetObj: %x, func: %s", target, target->GetObject(), text->GetCStr());
		C4AulParSet Pars(C4VInt(player), C4VInt(main->GetID()), C4VInt(parent->GetID()), C4VObj(parent->target), value);
		target->Call(text->GetCStr(), &Pars);
		return true;
	}
	if (nextAction)
		return nextAction->ExecuteCommand(actionID, parent, player);
	return false;
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

	float offX = 25.0f;
	float offY = 0.0f;
	float x = cgo.X + parentRight - offX;
	float yOrigin = cgo.Y + parentTop + offY;
	float hgt = (parentBottom - parentTop) - offY;

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
	bar.fctEnd.Draw(cgo.Surface, x, yOrigin + yOffset /*- bar.fctEnd.Hgt*/);
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

C4MenuWindowAction *C4MenuWindowProperty::GetActionForTag(unsigned int hash)
{
	if (!taggedProperties.count(hash)) return 0;
	return taggedProperties[hash].action;
}

void C4MenuWindowProperty::CleanUp(Prop &prop)
{
	switch (type)
	{
	case frameDecoration:
		if (prop.deco) delete prop.deco;
		break;
	case onClickAction:
	case onMouseInAction:
	case onMouseOutAction:
	case onCloseAction:
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
	currentTag = hash;

	switch (type)
	{
	case C4MenuWindowPropertyName::left:
	case C4MenuWindowPropertyName::right:
	case C4MenuWindowPropertyName::top:
	case C4MenuWindowPropertyName::bottom:
	case C4MenuWindowPropertyName::backgroundColor:
	case C4MenuWindowPropertyName::style:
	case C4MenuWindowPropertyName::priority:
		current->d = value.getInt();
		break;

	case C4MenuWindowPropertyName::relLeft:
	case C4MenuWindowPropertyName::relRight:
	case C4MenuWindowPropertyName::relTop:
	case C4MenuWindowPropertyName::relBottom:
		current->f = float(value.getInt()) / 1000.0f;
		break;
	case C4MenuWindowPropertyName::symbolObject:
	{
		C4PropList *symbol = value.getPropList();
		if (symbol)
			current->obj = symbol->GetObject();
		else current->def = 0;
		break;
	}
	case C4MenuWindowPropertyName::symbolDef:
	{
		C4PropList *symbol = value.getPropList();
		if (symbol)
			current->def = symbol->GetDef();
		else current->def = 0;
		break;
	}
	case C4MenuWindowPropertyName::frameDecoration:
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
	case C4MenuWindowPropertyName::text:
	{
		C4String *string = value.getStr();
		StdCopyStrBuf *buf = new StdCopyStrBuf();
		if (string)
			buf->Copy(string->GetCStr());
		else buf->Copy("");
		current->strBuf = buf;
		break;
	}
	case C4MenuWindowPropertyName::onClickAction:
	case C4MenuWindowPropertyName::onMouseInAction:
	case C4MenuWindowPropertyName::onMouseOutAction:
	case C4MenuWindowPropertyName::onCloseAction:
	{
		C4ValueArray *array = value.getArray();
		if (array)
		{
			assert (!current->action && "Prop() contains action prior to assignment");
			current->action = new C4MenuWindowAction();
			current->action->Init(array);
		}
		break;
	}

	default:
		assert(false && "C4MenuWindowAction should never have undefined type");
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
		case C4MenuWindowPropertyName::onMouseInAction:
		case C4MenuWindowPropertyName::onMouseOutAction:
		case C4MenuWindowPropertyName::onCloseAction:
			if (iter->second.action)
				iter->second.action->ClearPointers(pObj);
		break;
		default:
			return;
		}
	}
}

bool C4MenuWindowProperty::SwitchTag(C4String *tag)
{
	unsigned int hash = tag->Hash;
	if (!taggedProperties.count(hash)) return false; // tag not available
	if (current == &taggedProperties[hash]) return false; // tag already set?
	current = &taggedProperties[hash];
	currentTag = hash;
	return true;
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
	// relative offsets are standard, only need to set exact offset
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
	props[C4MenuWindowPropertyName::onMouseInAction].SetNull(hash);
	props[C4MenuWindowPropertyName::onMouseOutAction].SetNull(hash);
	props[C4MenuWindowPropertyName::onCloseAction].SetNull(hash);
	props[C4MenuWindowPropertyName::style].SetNull(hash);
	props[C4MenuWindowPropertyName::priority].SetNull(hash);

	parent = 0;
	wasRemoved = false;
	visible = true;
	hasMouseFocus = false;
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

bool C4MenuWindow::CreateFromPropList(C4PropList *proplist, bool resetStdTag, bool isUpdate)
{
	assert(parent && "MenuWindow created from proplist without parent (fails for ID tag)");

	bool layoutUpdateRequired = false; // needed for position changes etc
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
		{
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relLeft, C4MenuWindowPropertyName::left, standardHash);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Y] == key)
		{
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relTop, C4MenuWindowPropertyName::top, standardHash);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Wdt] == key)
		{
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relRight, C4MenuWindowPropertyName::right, standardHash);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Hgt] == key)
		{
			SetArrayTupleProperty(property, C4MenuWindowPropertyName::relBottom, C4MenuWindowPropertyName::bottom, standardHash);
			layoutUpdateRequired = true;
		}
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
		{
			props[C4MenuWindowPropertyName::text].Set(property, standardHash);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Prototype] == key)
			; // do nothing
		else if(&Strings.P[P_Mode] == key) // note that "Mode" is abused here for saving whether we have mouse focus
			hasMouseFocus = property.getBool();
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
		}
		else if(&Strings.P[P_OnClick] == key)
			props[C4MenuWindowPropertyName::onClickAction].Set(property, standardHash);
		else if(&Strings.P[P_OnMouseIn] == key)
			props[C4MenuWindowPropertyName::onMouseInAction].Set(property, standardHash);
		else if(&Strings.P[P_OnMouseOut] == key)
			props[C4MenuWindowPropertyName::onMouseOutAction].Set(property, standardHash);
		else if(&Strings.P[P_OnClose] == key)
			props[C4MenuWindowPropertyName::onCloseAction].Set(property, standardHash);
		else if(&Strings.P[P_Style] == key)
		{
			props[C4MenuWindowPropertyName::style].Set(property, standardHash);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Priority] == key)
		{
			props[C4MenuWindowPropertyName::priority].Set(property, standardHash);
			layoutUpdateRequired = true;
		}
		else
		{
			// possibly sub-window?
			C4PropList *subwindow = property.getPropList();
			if (subwindow)
			{
				C4MenuWindow *child = new C4MenuWindow();
				AddChild(child);

				if (!child->CreateFromPropList(subwindow, isUpdate == true, false))
					RemoveChild(child, false);
				else
					layoutUpdateRequired = true;
			}
		}
	}

	if (layoutUpdateRequired)
		parent->lastDrawPosition.dirty = 2;

	if (resetStdTag)
		SetTag(&Strings.P[P_Std]);

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
	props[C4MenuWindowPropertyName::onMouseInAction].ClearPointers(pObj);
	props[C4MenuWindowPropertyName::onMouseOutAction].ClearPointers(pObj);
	props[C4MenuWindowPropertyName::onCloseAction].ClearPointers(pObj);

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
		//LogF("adding main window: %d [I am %d, root: %d]", child->GetID(), id, int(this == &::MenuWindowRoot));
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
		//LogF("child-map-size: %d, remov %d [I am %d]", childrenIDMap.size(), child->GetID(), id);
		return;
	}
}

void C4MenuWindow::ChildGotID(C4MenuWindow *child)
{
	assert(parent && "ChildGotID called on window root, should not propagate over main windows!");
	if (!isMainWindow)
		return parent->ChildGotID(child);
	childrenIDMap.insert(std::make_pair(child->GetID(), child));
	//LogF("child+map+size: %d, added %d [I am %d]", childrenIDMap.size(), child->GetID(), id);
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

void C4MenuWindow::RemoveChild(C4MenuWindow *child, bool close, bool all)
{
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		if (child && (*iter != child)) continue;
		// don't delete twice
		(*iter)->wasRemoved = true;
		// close properly (calls etc.?)
		if (close)
			(*iter)->Close();

		// if the child had an quick-access entry, remove it
		if ((*iter)->GetID() != 0)
			ChildWithIDRemoved(*iter);

		delete *iter;
		if (!all)
		{
			children.erase(iter);
			return;
		}
	}
	if (all)
		children.clear();
}

void C4MenuWindow::ClearChildren(bool close)
{
	RemoveChild(0, close, true);
}

void C4MenuWindow::Close()
{
	// first, close all children and dispose of them properly
	ClearChildren(true);

	// make call to target object if applicable
	C4MenuWindowAction *action = props[C4MenuWindowPropertyName::onCloseAction].GetAction();
	if (action)
	{
		action->Execute(this, NO_OWNER, props[C4MenuWindowPropertyName::onCloseAction].GetCurrentTag(), C4MenuWindowPropertyName::onCloseAction);
	}
	//LogF("C4MenuWindow::Close, parent: %d [I am %d]", parent ? parent->GetID() : 0, id);

	if (!wasRemoved && parent)
		parent->RemoveChild(this);
}

void C4MenuWindow::EnableScrollBar(bool enable, float childrenHeight)
{
	const int32_t &style = props[C4MenuWindowPropertyName::style].GetInt();
	if (style & C4MenuWindowStyleFlag::FitChildren)
	{
		float height = lastDrawPosition.bottom - lastDrawPosition.top;
		props[C4MenuWindowPropertyName::bottom].current->d += (childrenHeight - height);
		return;
	}

	if (!enable && scrollBar)
	{
		delete scrollBar; scrollBar = 0;
	}
	else if (enable && !scrollBar)
	{
		scrollBar = new C4MenuWindowScrollBar();
		scrollBar->parent = this;
	}
}

void C4MenuWindow::UpdateLayout()
{
	//LogF("Updating Layout %p, root: %d, main: %d, style: %d", this, int(this == &::MenuWindowRoot), int(isMainWindow), props[C4MenuWindowPropertyName::style].GetInt());
	const int32_t &style = props[C4MenuWindowPropertyName::style].GetInt();

	// update scroll bar according to children
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
	if (first) // no children?
	{
		lastDrawPosition.topMostChild = 0.0f;
		lastDrawPosition.bottomMostChild = 0.0f;
	}

	// check whether we need a scroll-bar and then add it
	float height = lastDrawPosition.bottom - lastDrawPosition.top;
	float childHgt = lastDrawPosition.bottomMostChild - lastDrawPosition.topMostChild;

	EnableScrollBar(childHgt > height, childHgt);

	// special layout selected?
	if (style & C4MenuWindowStyleFlag::GridLayout)
		UpdateLayoutGrid();
	else if (style & C4MenuWindowStyleFlag::VerticalLayout)
		UpdateLayoutVertical();
}

void C4MenuWindow::UpdateLayoutGrid()
{
	const int32_t width = lastDrawPosition.right - lastDrawPosition.left;
	const int32_t height = lastDrawPosition.bottom - lastDrawPosition.top;

	unsigned int stdHash = Strings.P[P_Std].Hash;

	std::list<C4MenuWindow*> sorted;
	sorted.insert(sorted.begin(), children.begin(), children.end());
	sorted.sort(C4MenuWindow::CompareMenuWindowsByPriority);

	int32_t borderX(0), borderY(0);
	int32_t currentX = borderX;
	int32_t currentY = borderY;
	int32_t maxChildHeight = 0;

	for(std::list<C4MenuWindow*>::iterator iter = sorted.begin(); iter != sorted.end(); ++iter)
	{
		C4MenuWindow *child = *iter;
		const int32_t childWdt = child->lastDrawPosition.right - child->lastDrawPosition.left;
		const int32_t childHgt = child->lastDrawPosition.bottom - child->lastDrawPosition.top;
		// remember the heighest child
		if (!maxChildHeight || (childHgt > maxChildHeight))
			maxChildHeight = childHgt;

		child->props[C4MenuWindowPropertyName::left].SetInt(stdHash, currentX);
		child->props[C4MenuWindowPropertyName::right].SetInt(stdHash, currentX + childWdt);
		child->props[C4MenuWindowPropertyName::top].SetInt(stdHash, currentY);
		child->props[C4MenuWindowPropertyName::bottom].SetInt(stdHash, currentY + childHgt);

		currentX += childWdt + borderX;
		if (currentX + childWdt >= width)
		{
			currentX = borderX;
			currentY += maxChildHeight + borderY;
		}
	}

	lastDrawPosition.topMostChild = 0;
	lastDrawPosition.bottomMostChild = currentY;

	// do we need a scroll bar?
	EnableScrollBar(currentY >= height, currentY);
}

void C4MenuWindow::UpdateLayoutVertical()
{
	const int32_t height = lastDrawPosition.bottom - lastDrawPosition.top;

	unsigned int stdHash = Strings.P[P_Std].Hash;

	std::list<C4MenuWindow*> sorted;
	sorted.insert(sorted.begin(), children.begin(), children.end());
	sorted.sort(C4MenuWindow::CompareMenuWindowsByPriority);

	int32_t borderY(0);
	int32_t currentY = borderY;

	for(std::list<C4MenuWindow*>::iterator iter = sorted.begin(); iter != sorted.end(); ++iter)
	{
		C4MenuWindow *child = *iter;
		const int32_t childHgt = child->lastDrawPosition.bottom - child->lastDrawPosition.top;

		child->props[C4MenuWindowPropertyName::top].SetInt(stdHash, currentY);
		child->props[C4MenuWindowPropertyName::bottom].SetInt(stdHash, currentY + childHgt);

		currentY += childHgt + borderY;
	}

	lastDrawPosition.topMostChild = 0;
	lastDrawPosition.bottomMostChild = currentY;

	// do we need a scroll bar?
	EnableScrollBar(currentY >= height, currentY);
}

bool C4MenuWindow::DrawChildren(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom, int32_t withMultipleFlag)
{
	// note that withMultipleFlag only plays a roll for the root-menu
	bool oneDrawn = false; // was at least one child drawn?
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		C4MenuWindow *child = *iter;
		if (lastDrawPosition.dirty == 1)
			child->lastDrawPosition.dirty = 2;
		if (withMultipleFlag != -1)
		{
			const int32_t &style = child->props[C4MenuWindowPropertyName::style].GetInt();
			if ((withMultipleFlag == 0) &&  (style & C4MenuWindowStyleFlag::Multiple)) continue;
			if ((withMultipleFlag == 1) && !(style & C4MenuWindowStyleFlag::Multiple)) continue;
		}

		if (child->Draw(cgo, player, parentLeft, parentTop, parentRight, parentBottom))
			oneDrawn = true;
		// draw only one window when drawing non-Multiple windows
		if (oneDrawn && (withMultipleFlag == 0)) return true;
	}
	return oneDrawn;
}

bool C4MenuWindow::Draw(C4TargetFacet &cgo, int32_t player)
{
	if (!IsVisible()) return false;
	// assume I am the root and use the whole viewport for drawing - minus some standard border
	const int32_t &left = props[C4MenuWindowPropertyName::left].GetInt();
	const int32_t &right = props[C4MenuWindowPropertyName::right].GetInt();
	const int32_t &top = props[C4MenuWindowPropertyName::top].GetInt();
	const int32_t &bottom = props[C4MenuWindowPropertyName::bottom].GetInt();

	float leftDrawX = cgo.X + left;
	float topDrawY = cgo.Y + top;
	float rightDrawX = cgo.X + cgo.Wdt * cgo.Zoom + right;
	float bottomDrawY = cgo.Y + cgo.Hgt * cgo.Zoom + bottom;

	float wdt = rightDrawX - leftDrawX;
	float hgt = bottomDrawY - topDrawY;

	// abuse properties, we are root, we are allowed to do that
	// rounding against small errors
	if ((int(wdt) != int(lastDrawPosition.right)) || (int(hgt) != int(lastDrawPosition.bottom)))
	{
		//LogF("Updating root lastDrawPosition %f|%f // %f|%f", cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt);
		lastDrawPosition.right = wdt;
		lastDrawPosition.bottom = hgt;
		lastDrawPosition.dirty = 1;
	}
	else
		lastDrawPosition.dirty = 0;

	// step one: draw all non-multiple windows
	DrawChildren(cgo, player, leftDrawX - standardHorizontalBorder, topDrawY  - standardVerticalBorder, rightDrawX + standardHorizontalBorder, bottomDrawY + standardVerticalBorder, 1);
	// TODO: adjust rectangle for main menu if multiple windows exist
	// step two: draw one "main" menu
	DrawChildren(cgo, player, leftDrawX, topDrawY, rightDrawX, bottomDrawY, 0);
	return true;
}

bool C4MenuWindow::Draw(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom)
{
	// message hidden?
	if (!IsVisible()) return false;

	// player can see menu?
	if (target)
		if (!target->IsVisible(player, false))
			return false;
	// fetch style
	const int32_t &style = props[C4MenuWindowPropertyName::style].GetInt();
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

	// always update drawing position, needed for mouse input etc.
	lastDrawPosition.left = leftDrawX;
	lastDrawPosition.right = rightDrawX;
	lastDrawPosition.top = topDrawY;
	lastDrawPosition.bottom = bottomDrawY;

	// do we need to update children positions etc.?
	bool updateLayout = lastDrawPosition.dirty == 1;
	if (lastDrawPosition.dirty > 0)
	{
		--lastDrawPosition.dirty;

		if (updateLayout)
			UpdateLayout();
	}
	// check whether we are scrolling
	float childHgt = lastDrawPosition.bottomMostChild - lastDrawPosition.topMostChild;

	if (scrollBar)
		childOffsetY = -1.0f * (scrollBar->offset * (childHgt - height));

	// if ANY PARENT has scroll bar, then adjust clipper
	float clipX1(0.0f), clipX2(0.0f), clipY1(0.0f), clipY2(0.0f);
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
		int alignment = ALeft;
		int32_t textHgt = ::GraphicsResource.FontRegular.BreakMessage(strBuf->getData(), int32_t(width), &sText, true);
		float textYOffset = 0.0f, textXOffset = 0.0f;
		if (style & C4MenuWindowStyleFlag::TextVCenter)
			textYOffset = height/2.0f - float(textHgt)/2.0f;
		else if (style & C4MenuWindowStyleFlag::TextBottom)
			textYOffset += height - float(textHgt);
		if (style & C4MenuWindowStyleFlag::TextHCenter)
		{
			int wdt, hgt;
			::GraphicsResource.FontRegular.GetTextExtent(sText.getData(), wdt, hgt, true);
			textXOffset = width/2.0f - float(wdt)/2.0f;
		}
		else if (style & C4MenuWindowStyleFlag::TextRight)
		{
			alignment = ARight;
			int wdt, hgt;
			::GraphicsResource.FontRegular.GetTextExtent(sText.getData(), wdt, hgt, true);
			textXOffset = width - float(wdt);
		}
		pDraw->TextOut(sText.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface, cgo.X + leftDrawX + textXOffset, cgo.Y + topDrawY + textYOffset, 0xffffffff, ALeft);
		// enable auto scroll
		float textBottom = lastDrawPosition.top + float(textHgt);
		if (textBottom > lastDrawPosition.bottom)
			lastDrawPosition.bottom = textBottom;
	}

	if (clipping)
	{
		pDraw->RestorePrimaryClipper();
	}

	if (GraphicsSystem.ShowMenuInfo) // print helpful debug info
	{
		pDraw->DrawFrameDw(cgo.Surface, leftDrawX, topDrawY, rightDrawX,bottomDrawY, clipping ? C4RGB(255, 255, 0) :C4RGB(0, 255, 0));
		StdStrBuf buf = FormatString("%s(%d)", target ? target->GetName() : "", id);
		pDraw->TextOut(buf.getData(), ::GraphicsResource.FontCaption, 1.0, cgo.Surface, cgo.X + leftDrawX, cgo.Y + topDrawY, 0xffff00ff, ALeft);
	}

	if (scrollBar)
	{
		scrollBar->Draw(cgo, player, leftDrawX, topDrawY, rightDrawX, bottomDrawY);
	}

	DrawChildren(cgo, player, leftDrawX, topDrawY + childOffsetY, rightDrawX, bottomDrawY + childOffsetY);
	return true;
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
		if (props[i].SwitchTag(tag))
		{
			// only if tag could have changed position etc.
			if (i <= C4MenuWindowPropertyName::relBottom || i == C4MenuWindowPropertyName::text || i == C4MenuWindowPropertyName::style || i == C4MenuWindowPropertyName::priority)
			if (parent)
			{
				parent->lastDrawPosition.dirty = 2;
				//LogF("Change tag %d to %s", i, tag->GetCStr());
			}

		}

	// .. and children
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
		(*iter)->SetTag(tag);
}

void C4MenuWindow::OnMouseIn(int32_t player)
{
	assert(!hasMouseFocus && "custom menu window properly loaded incorrectly!");
	hasMouseFocus = true;

	// no need to notify children, this is done in MouseInput

	// execute action
	int32_t actionType = C4MenuWindowPropertyName::onMouseInAction;
	C4MenuWindowAction *action = props[actionType].GetAction();
	if (!action) return;
	action->Execute(this, player, props[actionType].GetCurrentTag(), actionType);
}

void C4MenuWindow::OnMouseOut(int32_t player)
{
	assert(hasMouseFocus && "custom menu window properly loaded incorrectly!");
	hasMouseFocus = false;

	// needs to notify children
	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		C4MenuWindow *child = *iter;
		if (child->hasMouseFocus)
			child->OnMouseOut(player);
	}

	// execute action
	int32_t actionType = C4MenuWindowPropertyName::onMouseOutAction;
	C4MenuWindowAction *action = props[actionType].GetAction();
	if (!action) return;
	action->Execute(this, player, props[actionType].GetCurrentTag(), actionType);
}

bool C4MenuWindow::MouseInput(int32_t player, int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam)
{
	if (!visible) return false;

	if (target)
		if (!target->IsVisible(player, false))
			return false;

	// we have mouse focus! Is this new?
	if (!hasMouseFocus)
		OnMouseIn(player);

	/*if (parent && mouse.IsLDown())
		LogF("%f|%f - %d|%d", lastDrawPosition.left, lastDrawPosition.top, mouseX, mouseY);
	else LogF("called on root %d|%d", mouseX, mouseY);*/

	// children actually have a higher priority
	bool overChild = false; // remember for later, catch all actions that are in theory over children, even if not reaction (if main window)
	// use reverse iterator since children with higher Priority appear later in the list
	for (std::list<C4MenuWindow*>::reverse_iterator iter = children.rbegin(); iter != children.rend(); ++iter)
	{
		C4MenuWindow *child = *iter;
		int32_t childLeft = static_cast<int32_t>(child->lastDrawPosition.left);
		int32_t childRight = static_cast<int32_t>(child->lastDrawPosition.right);
		int32_t childTop = static_cast<int32_t>(child->lastDrawPosition.top);
		int32_t childBottom = static_cast<int32_t>(child->lastDrawPosition.bottom);
		//LogF("%d|%d in %d|%d // %d|%d", mouseX, mouseY, childLeft, childTop, childRight, childBottom);
		bool inArea = true;
		if ((mouseX < childLeft) || (mouseX > childRight)) inArea = false;
		else if ((mouseY < childTop) || (mouseY > childBottom)) inArea = false;

		if (!inArea) // notify child if it had mouse focus before
		{
			if (child->hasMouseFocus)
				child->OnMouseOut(player);
			continue;
		}

		overChild = true;
		if (child->MouseInput(player, button, mouseX, mouseY, dwKeyParam))
			return true;
	}

	if (button == C4MC_Button_LeftDown)
	{
		C4MenuWindowAction *action = props[C4MenuWindowPropertyName::onClickAction].GetAction();
		if (action)
		{
			action->Execute(this, player, props[C4MenuWindowPropertyName::onClickAction].GetCurrentTag(), C4MenuWindowPropertyName::onClickAction);
			return true;
		}
	}

	// for scroll-enabled windows, scroll contents with wheel
	if (scrollBar && (button == C4MC_Button_Wheel))
	{
		short delta = (short)(dwKeyParam >> 16);
		float fac = (lastDrawPosition.bottomMostChild - lastDrawPosition.topMostChild);
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

bool C4MenuWindow::ExecuteCommand(int32_t actionID, int32_t player, int32_t subwindowID, int32_t actionType, C4Object *target, unsigned int tag)
{
	if (isMainWindow && subwindowID) // we are a main window! try a shortcut through the ID?
	{
		//LogF("passing command... %d, %d, %d, %d, %d [I am %d, MW]", actionID, player, subwindowID, actionType, tag, id);
		// the reasoning for that shortcut is that I assume that usually windows with actions will also have an ID assigned
		// this obviously doesn't have to be the case, but I believe it's worth the try
		std::pair<std::multimap<int32_t, C4MenuWindow*>::iterator, std::multimap<int32_t, C4MenuWindow*>::iterator> range;
		range = childrenIDMap.equal_range(subwindowID);

		for (std::multimap<int32_t, C4MenuWindow*>::iterator iter = range.first; iter != range.second; ++iter)
		{
			if (iter->second->ExecuteCommand(actionID, player, subwindowID, actionType, target, tag))
				return true;
		}
		// it is not possible that another window would match the criteria. Abort later after self-check
	}

	// are we elligible?
	if ((id == subwindowID) && (this->target == target))
	{
		C4MenuWindowAction *action = props[actionType].GetActionForTag(tag);
		if (action)
		{
			if (action->ExecuteCommand(actionID, this, player))
				return true;
		}
		// note that we should not simply return false here
		// there is no guarantee that only one window with that target&ID existss
	}

	// not caught, forward to children!
	// abort if main window, though. See above
	if (isMainWindow && subwindowID) return false;

	for (std::list<C4MenuWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		C4MenuWindow *child = *iter;
		if (child->ExecuteCommand(actionID, player, subwindowID, actionType, target, tag))
			return true;
	}
	return false;
}
