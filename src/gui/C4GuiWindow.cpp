/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013 David Dormagen
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

 /*
	A flexible ingame menu system that can be used to compose large GUIs out of multiple windows.
	
	Every window is basically a rectangle that can contain some make-up-information (symbol/text/...) and coordinates.
	Those coordinates can either be relative to the window's parent or in total pixels or a mixture of both.
	
	The entry point for all of the callbacks for mouse input, drawing, etc. is one normal window which always exists and happens
	to be the parent of ALL of the script-created menus. Callbacks are usually forwarded to the children.
	
	If you want to add new window properties (similar to backgroundColor, onClickAction etc.) you have to make sure that they are
	serialized correctly and cleaned up if necessary when a menu window is closed or the property is overwritten by a script call!
*/

#include <C4Include.h>
#include <C4GuiWindow.h>

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
#include <C4Viewport.h>

#include <cmath>

C4GuiWindowAction::~C4GuiWindowAction()
{
	if (text)
		text->DecRef();
	if (nextAction)
		delete nextAction;
}

const C4Value C4GuiWindowAction::ToC4Value(bool first)
{
	C4ValueArray *array = new C4ValueArray();

	switch (action)
	{
	case C4GuiWindowActionID::Call:
		array->SetSize(4);
		array->SetItem(0, C4Value(action));
		array->SetItem(1, C4Value(target));
		array->SetItem(2, C4Value(text));
		array->SetItem(3, value);
		break;

	case C4GuiWindowActionID::SetTag:
		array->SetSize(4);
		array->SetItem(0, C4Value(action));
		array->SetItem(1, C4Value(text));
		array->SetItem(2, C4Value(subwindowID));
		array->SetItem(3, C4Value(target));
		break;

	case 0: // can actually happen if the action is invalidated
		break;

	default:
		assert(false && "trying to save C4GuiWindowAction without valid action");
		break;
	}

	assert (array->GetSize() < 6);
	array->SetSize(6);
	array->SetItem(5, C4Value(id));

	if (!first || !nextAction) return C4Value(array);

	// this action is the first in a chain of actions
	// all following actions (and this one) have to be put into another array
	C4ValueArray *container = new C4ValueArray();
	int32_t size = 1;
	container->SetSize(size);
	container->SetItem(0, C4Value(array));

	C4GuiWindowAction *next = nextAction;
	while (next)
	{
		C4Value val = next->ToC4Value(false);
		++size;
		container->SetSize(size);
		container->SetItem(size - 1, val);
		next = next->nextAction;
	}
	return C4Value(container);
}

void C4GuiWindowAction::ClearPointers(C4Object *pObj)
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
bool C4GuiWindowAction::Init(C4ValueArray *array, int32_t index)
{
	if (array->GetSize() == 0) // safety
		return false;

	// an array of actions?
	if (array->GetItem(0).getArray())
	{
		// add action to action chain?
		if (index+1 < array->GetSize())
		{
			nextAction = new C4GuiWindowAction();
			nextAction->Init(array, index + 1);
		}
		// continue with one sub array
		array = array->GetItem(index).getArray();
		if (!array) return false;
	}
	// retrieve type of action
	int newAction = array->GetItem(0).getInt();
	action = 0; // still invalid!

	// when loading, the array has a size of 6 with the 5th element being the ID
	if (array->GetSize() == 6)
		id = array->GetItem(3).getInt();

	switch (newAction)
	{
	case C4GuiWindowActionID::Call:
		if (array->GetSize() < 3) return false;
		target = array->GetItem(1).getPropList();
		text = array->GetItem(2).getStr();
		if (!target || !text) return false;
		if (array->GetSize() >= 4)
			value = C4Value(array->GetItem(3));
		text->IncRef();

		// important! needed to identify actions later!
		if (!id)
			id = ::Game.GuiWindowRoot->GenerateActionID();

		break;

	case C4GuiWindowActionID::SetTag:
		if (array->GetSize() < 4) return false;
		text = array->GetItem(1).getStr();
		if (!text) return false;
		text->IncRef();
		subwindowID = array->GetItem(2).getInt();
		target = array->GetItem(3).getObj(); // getObj on purpose. Need to validate that.
		break;

	default:
		return false;
	}

	action = newAction;
	return true;
}

void C4GuiWindowAction::Execute(C4GuiWindow *parent, int32_t player, int32_t actionType)
{
	assert(parent && "C4GuiWindow::Execute must always be called with parent");
	//LogF("Excuting action (nextAction: %x, subwID: %d, target: %x, text: %s, type: %d)", nextAction, subwindowID, target, text->GetCStr(), actionType);

	// invalid ID? can be set by removal of target object
	if (action)
	{
		// get menu main window
		C4GuiWindow *main = parent;
		C4GuiWindow *from = main;
		while (!from->IsRoot())
		{
			main = from;
			from = static_cast<C4GuiWindow*>(from->GetParent());
		}

		switch (action)
		{
		case C4GuiWindowActionID::Call:
		{
			if (!target) // ohject removed in the meantime?
				break;
			// the action needs to be synchronized! Assemble command and put it into control queue!
			Game.Input.Add(CID_MenuCommand, new C4ControlMenuCommand(id, player, main->GetID(), parent->GetID(), parent->target, actionType));
			break;
		}

		case C4GuiWindowActionID::SetTag:
		{
			C4GuiWindow *window = main;
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
			assert(false && "C4GuiWindowAction without valid or invalidated ID");
			break;
		}
	} // action

	if (nextAction)
	{
		nextAction->Execute(parent, player, actionType);
	}
}

bool C4GuiWindowAction::ExecuteCommand(int32_t actionID, C4GuiWindow *parent, int32_t player)
{
	// target has already been checked for validity
	if (id == actionID && action)
	{
		assert(action == C4GuiWindowActionID::Call && "C4ControlMenuCommand for invalid action!");

		// get menu main window
		C4GuiWindow *main = parent;
		C4GuiWindow *from = main;
		while (!from->IsRoot())
		{
			main = from;
			from = static_cast<C4GuiWindow*>(from->GetParent());
		}
		//LogF("command synced.. target: %x, targetObj: %x, func: %s", target, target->GetObject(), text->GetCStr());
		C4AulParSet Pars(value, C4VInt(player), C4VInt(main->GetID()), C4VInt(parent->GetID()), C4VObj(parent->target));
		target->Call(text->GetCStr(), &Pars);
		return true;
	}
	if (nextAction)
		return nextAction->ExecuteCommand(actionID, parent, player);
	return false;
}

C4GuiWindowProperty::~C4GuiWindowProperty()
{
	// is cleaned up from destructor of C4GuiWindow
}

void C4GuiWindowProperty::SetInt(int32_t to, C4String *tag)
{
	if (!tag) tag = &Strings.P[P_Std];
	taggedProperties[tag] = Prop();
	current = &taggedProperties[tag];
	current->d = to;
}
void C4GuiWindowProperty::SetFloat(float to, C4String *tag)
{
	if (!tag) tag = &Strings.P[P_Std];
	taggedProperties[tag] = Prop();
	current = &taggedProperties[tag];
	current->f = to;
}
void C4GuiWindowProperty::SetNull(C4String *tag)
{
	if (!tag) tag = &Strings.P[P_Std];
	taggedProperties[tag] = Prop();
	current = &taggedProperties[tag];
	current->data = 0;
}

void C4GuiWindowProperty::CleanUp(Prop &prop)
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

void C4GuiWindowProperty::CleanUpAll()
{
	for (std::map<C4String*, Prop>::iterator iter = taggedProperties.begin(); iter != taggedProperties.end(); ++iter)
	{
		CleanUp(iter->second);
		if (iter->first != &Strings.P[P_Std])
			iter->first->DecRef();
	}
}

const C4Value C4GuiWindowProperty::ToC4Value()
{
	C4PropList *proplist = C4PropList::New();
	
	// go through all of the tagged properties and add a property to the proplist containing both the tag name
	// and the serialzed C4Value of the properties' value
	for(std::map<C4String*, Prop>::iterator iter = taggedProperties.begin(); iter != taggedProperties.end(); ++iter)
	{
		C4String *tagString = iter->first;
		const Prop &prop = iter->second;

		C4Value val;

		// get value to save
		switch (type)
		{
		case C4GuiWindowPropertyName::left:
		case C4GuiWindowPropertyName::right:
		case C4GuiWindowPropertyName::top:
		case C4GuiWindowPropertyName::bottom:
		case C4GuiWindowPropertyName::relLeft:
		case C4GuiWindowPropertyName::relRight:
		case C4GuiWindowPropertyName::relTop:
		case C4GuiWindowPropertyName::relBottom:
		case C4GuiWindowPropertyName::leftMargin:
		case C4GuiWindowPropertyName::rightMargin:
		case C4GuiWindowPropertyName::topMargin:
		case C4GuiWindowPropertyName::bottomMargin:
		case C4GuiWindowPropertyName::relLeftMargin:
		case C4GuiWindowPropertyName::relRightMargin:
		case C4GuiWindowPropertyName::relTopMargin:
		case C4GuiWindowPropertyName::relBottomMargin:
			assert (false && "Trying to get a single positional value from a GuiWindow for saving. Those should always be saved in pairs of two in a string.");
			break;

		case C4GuiWindowPropertyName::backgroundColor:
		case C4GuiWindowPropertyName::style:
		case C4GuiWindowPropertyName::priority:
		case C4GuiWindowPropertyName::player:
			val = C4Value(prop.d);
			break;

		case C4GuiWindowPropertyName::symbolObject:
			val = C4Value(prop.obj);
			break;

		case C4GuiWindowPropertyName::symbolDef:
			val = C4Value(prop.def);
			break;

		case C4GuiWindowPropertyName::frameDecoration:
			val = C4Value(prop.deco ? prop.deco->idSourceDef : C4ID::None);
			break;

		case C4GuiWindowPropertyName::text:
		case C4GuiWindowPropertyName::tooltip:
		{
			if (prop.strBuf)
			{
				// string existing?
				C4String *s = Strings.FindString(prop.strBuf->getData());
				if (!s) s = Strings.RegString(prop.strBuf->getData());
				val = C4Value(s);
			}
			break;
		}

		case C4GuiWindowPropertyName::onClickAction:
		case C4GuiWindowPropertyName::onMouseInAction:
		case C4GuiWindowPropertyName::onMouseOutAction:
		case C4GuiWindowPropertyName::onCloseAction:
			if (prop.action)
				val = prop.action->ToC4Value();
			break;

		default:
			assert(false && "C4GuiWindowAction should never have undefined type");
			break;
		} // switch

		proplist->SetPropertyByS(tagString, val);
	}

	return C4Value(proplist);
}

void C4GuiWindowProperty::Set(const C4Value &value, C4String *tag)
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
			Set(property, key);
		}
		return;
	}

	// special treatment for some that have to be deleted (due to owning string/frame deco/...)
	if (taggedProperties.count(tag))
		CleanUp(taggedProperties[tag]);
	else // new tag, retain the proplist if not standard
		if (tag != &Strings.P[P_Std])
			tag->IncRef();

	taggedProperties[tag] = Prop();
	// in order to make /current/ sane, always reset it - not relying on implementation details of std::map
	// if the user wants a special tag selected, he should do that (standard selection will still be "Std")
	current = &taggedProperties[tag];
	currentTag = tag;
	
	
	// now that a new property entry has been created and the old has been cleaned up, get the data from the C4Value
	switch (type)
	{
	case C4GuiWindowPropertyName::left:
	case C4GuiWindowPropertyName::right:
	case C4GuiWindowPropertyName::top:
	case C4GuiWindowPropertyName::bottom:
	case C4GuiWindowPropertyName::relLeft:
	case C4GuiWindowPropertyName::relRight:
	case C4GuiWindowPropertyName::relTop:
	case C4GuiWindowPropertyName::relBottom:
	case C4GuiWindowPropertyName::leftMargin:
	case C4GuiWindowPropertyName::rightMargin:
	case C4GuiWindowPropertyName::topMargin:
	case C4GuiWindowPropertyName::bottomMargin:
	case C4GuiWindowPropertyName::relLeftMargin:
	case C4GuiWindowPropertyName::relRightMargin:
	case C4GuiWindowPropertyName::relTopMargin:
	case C4GuiWindowPropertyName::relBottomMargin:
		assert (false && "Trying to set positional properties directly. Those should always come parsed from a string.");
		break;

	case C4GuiWindowPropertyName::backgroundColor:
	case C4GuiWindowPropertyName::style:
	case C4GuiWindowPropertyName::priority:
	case C4GuiWindowPropertyName::player:
		current->d = value.getInt();
		break;

	case C4GuiWindowPropertyName::symbolObject:
	{
		C4PropList *symbol = value.getPropList();
		if (symbol)
			current->obj = symbol->GetObject();
		else current->obj = 0;
		break;
	}
	case C4GuiWindowPropertyName::symbolDef:
	{
		C4PropList *symbol = value.getPropList();
		if (symbol)
			current->def = symbol->GetDef();
		else current->def = 0;
		break;
	}
	case C4GuiWindowPropertyName::frameDecoration:
	{
		C4Def *def = value.getDef();
		
		if (def)
		{
			current->deco = new C4GUI::FrameDecoration();
			if (!current->deco->SetByDef(def))
			{
				delete current->deco;
				current->deco = 0;
			}
		}
		break;
	}
	case C4GuiWindowPropertyName::text:
	case C4GuiWindowPropertyName::tooltip:
	{
		C4String *string = value.getStr();
		StdCopyStrBuf *buf = new StdCopyStrBuf();
		if (string)
			buf->Copy(string->GetCStr());
		else buf->Copy("");
		current->strBuf = buf;
		break;
	}
	case C4GuiWindowPropertyName::onClickAction:
	case C4GuiWindowPropertyName::onMouseInAction:
	case C4GuiWindowPropertyName::onMouseOutAction:
	case C4GuiWindowPropertyName::onCloseAction:
	{
		C4ValueArray *array = value.getArray();
		if (array)
		{
			assert (!current->action && "Prop() contains action prior to assignment");
			current->action = new C4GuiWindowAction();
			current->action->Init(array);
		}
		break;
	}

	default:
		assert(false && "C4GuiWindowAction should never have undefined type");
		break;
	} // switch
}

void C4GuiWindowProperty::ClearPointers(C4Object *pObj)
{
	// assume that we actually contain an object
	// go through all the tags and, in case the tag has anything to do with objects, check and clear it
	for (std::map<C4String*, Prop>::iterator iter = taggedProperties.begin(); iter != taggedProperties.end(); ++iter)
	{
		switch (type)
		{
		case C4GuiWindowPropertyName::symbolObject:
			if (iter->second.obj == pObj)
				iter->second.obj = 0;
		break;

		case C4GuiWindowPropertyName::onClickAction:
		case C4GuiWindowPropertyName::onMouseInAction:
		case C4GuiWindowPropertyName::onMouseOutAction:
		case C4GuiWindowPropertyName::onCloseAction:
			if (iter->second.action)
				iter->second.action->ClearPointers(pObj);
		break;
		default:
			return;
		}
	}
}

bool C4GuiWindowProperty::SwitchTag(C4String *tag)
{
	if (!taggedProperties.count(tag)) return false; // tag not available
	if (current == &taggedProperties[tag]) return false; // tag already set?
	current = &taggedProperties[tag];
	currentTag = tag;
	return true;
}

std::list<C4GuiWindowAction*> C4GuiWindowProperty::GetAllActions()
{
	std::list<C4GuiWindowAction*> allActions;
	for (std::map<C4String*, Prop>::iterator iter = taggedProperties.begin(); iter != taggedProperties.end(); ++iter)
	{
		Prop &p = iter->second;
		if (p.action)
			allActions.push_back(p.action);
	}
	return allActions;
}


C4GuiWindow::C4GuiWindow() : C4GUI::ScrollWindow(this)
{
	Init();
}

C4GuiWindow::C4GuiWindow(float stdBorderX, float stdBorderY) : C4GUI::ScrollWindow(this)
{
	Init();

	// set border values for std tag
	// relative offsets are standard, only need to set exact offset
	props[C4GuiWindowPropertyName::left].SetFloat(Pix2Em(stdBorderX));
	props[C4GuiWindowPropertyName::right].SetFloat(Pix2Em(-stdBorderX));
	props[C4GuiWindowPropertyName::top].SetFloat(Pix2Em(stdBorderY));
	props[C4GuiWindowPropertyName::bottom].SetFloat(Pix2Em(-stdBorderY));
}

void C4GuiWindow::Init()
{
	id = 0;
	name = nullptr;

	isMainWindow = false;
	mainWindowNeedsLayoutUpdate = false;

	// properties must know what they stand for
	for (int32_t i = 0; i < C4GuiWindowPropertyName::_lastProp; ++i)
		props[i].type = i;

	// standard values for all of the properties

	// exact offsets are standard 0
	props[C4GuiWindowPropertyName::left].SetNull();
	props[C4GuiWindowPropertyName::right].SetNull();
	props[C4GuiWindowPropertyName::top].SetNull();
	props[C4GuiWindowPropertyName::bottom].SetNull();
	// relative offsets are standard full screen 0,0 - 1,1
	props[C4GuiWindowPropertyName::relLeft].SetNull();
	props[C4GuiWindowPropertyName::relTop].SetNull();
	props[C4GuiWindowPropertyName::relBottom].SetFloat(1.0f);
	props[C4GuiWindowPropertyName::relRight].SetFloat(1.0f);
	// all margins are always standard 0
	props[C4GuiWindowPropertyName::leftMargin].SetNull();
	props[C4GuiWindowPropertyName::rightMargin].SetNull();
	props[C4GuiWindowPropertyName::topMargin].SetNull();
	props[C4GuiWindowPropertyName::bottomMargin].SetNull();
	props[C4GuiWindowPropertyName::relLeftMargin].SetNull();
	props[C4GuiWindowPropertyName::relTopMargin].SetNull();
	props[C4GuiWindowPropertyName::relBottomMargin].SetNull();
	props[C4GuiWindowPropertyName::relRightMargin].SetNull();
	// other properties are 0
	props[C4GuiWindowPropertyName::backgroundColor].SetNull();
	props[C4GuiWindowPropertyName::frameDecoration].SetNull();
	props[C4GuiWindowPropertyName::symbolObject].SetNull();
	props[C4GuiWindowPropertyName::symbolDef].SetNull();
	props[C4GuiWindowPropertyName::text].SetNull();
	props[C4GuiWindowPropertyName::tooltip].SetNull();
	props[C4GuiWindowPropertyName::onClickAction].SetNull();
	props[C4GuiWindowPropertyName::onMouseInAction].SetNull();
	props[C4GuiWindowPropertyName::onMouseOutAction].SetNull();
	props[C4GuiWindowPropertyName::onCloseAction].SetNull();
	props[C4GuiWindowPropertyName::style].SetNull();
	props[C4GuiWindowPropertyName::priority].SetNull();
	props[C4GuiWindowPropertyName::player].SetInt(-1);

	wasRemoved = false;
	closeActionWasExecuted = false;
	currentMouseState = MouseState::None;
	target = 0;
	pScrollBar->fAutoHide = true;
}

C4GuiWindow::~C4GuiWindow()
{
	ClearChildren(false);

	// delete certain properties that contain allocated elements or referenced strings
	for (int32_t i = 0; i < C4GuiWindowPropertyName::_lastProp; ++i)
		props[i].CleanUpAll();

	if (pScrollBar)
		delete pScrollBar;
}

// helper function
void C4GuiWindow::SetMarginProperties(const C4Value &property, C4String *tag)
{
	// the value might be a tagged proplist again
	if (property.GetType() == C4V_Type::C4V_PropList)
	{
		C4PropList *proplist = property.getPropList();
		for (C4PropList::Iterator iter = proplist->begin(); iter != proplist->end(); ++iter)
		{
			SetMarginProperties(iter->Value, iter->Key);
		}
		return;
	}

	// safety
	if (property.GetType() == C4V_Type::C4V_Array && property.getArray()->GetSize() == 0)
		return;

	// always set all four margins
	for (int i = 0; i < 4; ++i)
	{
		C4GuiWindowPropertyName relative, absolute;
		switch (i)
		{
		case 0:
			absolute = C4GuiWindowPropertyName::leftMargin;
			relative = C4GuiWindowPropertyName::relLeftMargin;
			break;
		case 1:
			absolute = C4GuiWindowPropertyName::topMargin;
			relative = C4GuiWindowPropertyName::relTopMargin;
			break;
		case 2:
			absolute = C4GuiWindowPropertyName::rightMargin;
			relative = C4GuiWindowPropertyName::relRightMargin;
			break;
		case 3:
			absolute = C4GuiWindowPropertyName::bottomMargin;
			relative = C4GuiWindowPropertyName::relBottomMargin;
			break;
		default:
			assert(false);
		}

		if (property.GetType() == C4V_Type::C4V_Array)
		{
			C4ValueArray *array = property.getArray();
			int realIndex = i % array->GetSize();
			SetPositionStringProperties(array->GetItem(realIndex), relative, absolute, tag);
		}
		else
			// normal string, hopefully
			SetPositionStringProperties(property, relative, absolute, tag);
	}
}

C4Value C4GuiWindow::MarginsToC4Value()
{
	C4ValueArray *array = new C4ValueArray();
	array->SetSize(4);

	array->SetItem(0, PositionToC4Value(C4GuiWindowPropertyName::relLeftMargin, C4GuiWindowPropertyName::leftMargin));
	array->SetItem(1, PositionToC4Value(C4GuiWindowPropertyName::relTopMargin, C4GuiWindowPropertyName::topMargin));
	array->SetItem(2, PositionToC4Value(C4GuiWindowPropertyName::relRightMargin, C4GuiWindowPropertyName::rightMargin));
	array->SetItem(3, PositionToC4Value(C4GuiWindowPropertyName::relBottomMargin, C4GuiWindowPropertyName::bottomMargin));

	return C4Value(array);
}

// helper function
void C4GuiWindow::SetPositionStringProperties(const C4Value &property, C4GuiWindowPropertyName relative, C4GuiWindowPropertyName absolute, C4String *tag)
{
	// the value might be a tagged proplist again
	if (property.GetType() == C4V_Type::C4V_PropList)
	{
		C4PropList *proplist = property.getPropList();
		for (C4PropList::Iterator iter = proplist->begin(); iter != proplist->end(); ++iter)
		{
			SetPositionStringProperties(iter->Value, relative, absolute, iter->Key);
		}
		return;
	}
	// safety
	if (property.GetType() != C4V_Type::C4V_String) return;
	StdStrBuf buf(property.getStr()->GetData());
	
	std::string trimmedString;
	size_t maxLength = buf.getLength();
	trimmedString.reserve(maxLength);
	// add all non-whitespace characters to the new string (strtod could abort the parsing otherwise)
	for (size_t i = 0; i < maxLength; ++i)
	{
		if (!isspace(buf[i]))
			trimmedString.push_back(buf[i]);
	}

	float relativeValue = 0.0;
	float absoluteValue = 0.0;

	const char *currentPosition = trimmedString.data();
	char *nextPosition;
	const char *lastPosition = trimmedString.data() + trimmedString.size();

	while (currentPosition < lastPosition)
	{
		// look for next float
		nextPosition = 0;
		float value = static_cast<float>(strtod(currentPosition, &nextPosition));

		// fail? exit right here (there must be some space left in the string for a unit, too)
		if (currentPosition == nextPosition || nextPosition == 0 || nextPosition >= lastPosition) break;

		if (*nextPosition == '%')
		{
			relativeValue += value;
			currentPosition = nextPosition + 1;
		}
		else if (*nextPosition == 'e' && *(nextPosition+1) == 'm')
		{
			absoluteValue += value;
			currentPosition = nextPosition + 2;
		}
		else // error, abort!
			break;
	}
	props[relative].SetFloat(relativeValue / 100.0f, tag);
	props[absolute].SetFloat(absoluteValue, tag);
}

// for saving
C4Value C4GuiWindow::PositionToC4Value(C4GuiWindowPropertyName relativeName, C4GuiWindowPropertyName absoluteName)
{
	// Go through all tags of the position attributes and save.
	// Note that the tags for both the relative and the absolute attribute are always the same.
	C4GuiWindowProperty &relative = props[relativeName];
	C4GuiWindowProperty &absolute = props[absoluteName];
	
	C4PropList *proplist = nullptr;
	const bool onlyStdTag = relative.taggedProperties.size() == 1;
	for (std::map<C4String*, C4GuiWindowProperty::Prop>::iterator iter = relative.taggedProperties.begin(); iter != relative.taggedProperties.end(); ++iter)
	{
		C4String *tag = iter->first;
		StdStrBuf buf;
		buf.Format("%f%%%+fem", iter->second.f, absolute.taggedProperties[tag].f);
		C4String *propString = Strings.RegString(buf);

		if (onlyStdTag)
			return C4Value(propString);
		else
		{
			if (proplist == nullptr)
				proplist = C4PropList::New();
			proplist->SetPropertyByS(tag, C4Value(propString));
		}
	}
	return C4Value(proplist);
}

const C4Value C4GuiWindow::ToC4Value()
{
	C4PropList *proplist = C4PropList::New();

	// it is necessary that this list contains all of the properties which can also be set somehow
	// if you add something, don't forget to also add the real serialization to the loop below
	int32_t toSave[] =
	{
		P_Left,
		P_Top,
		P_Right,
		P_Bottom,
		P_Margin,
		P_BackgroundColor,
		P_Decoration,
		P_Symbol,
		P_Target,
		P_Text,
		P_ID,
		P_OnClick,
		P_OnMouseIn,
		P_OnMouseOut,
		P_OnClose,
		P_Style,
		P_Mode,
		P_Priority,
		P_Player,
		P_Tooltip
	};

	const int32_t entryCount = sizeof(toSave) / sizeof(int32_t);

	for (size_t i = 0; i < entryCount; ++i)
	{
		int32_t prop = toSave[i];
		C4Value val;

		switch (prop)
		{
		case P_Left:
		case P_Top:
		case P_Right:
		case P_Bottom:
		{
#define PROPERTY_TUPLE(p, prop1, prop2) if (prop == p) { val = PositionToC4Value(prop1, prop2); }
			PROPERTY_TUPLE(P_Left, C4GuiWindowPropertyName::relLeft, C4GuiWindowPropertyName::left);
			PROPERTY_TUPLE(P_Top, C4GuiWindowPropertyName::relTop, C4GuiWindowPropertyName::top);
			PROPERTY_TUPLE(P_Right, C4GuiWindowPropertyName::relRight, C4GuiWindowPropertyName::right);
			PROPERTY_TUPLE(P_Bottom, C4GuiWindowPropertyName::relBottom, C4GuiWindowPropertyName::bottom);
#undef PROPERTY_TUPLE
			break;
		}
		case P_Margin: val = MarginsToC4Value(); break;
		case P_BackgroundColor: val = props[C4GuiWindowPropertyName::backgroundColor].ToC4Value(); break;
		case P_Decoration: val = props[C4GuiWindowPropertyName::frameDecoration].ToC4Value(); break;
		case P_Symbol:
			// either object or def
			val = props[C4GuiWindowPropertyName::symbolObject].ToC4Value();
			if (val == C4Value()) // is nil?
				val = props[C4GuiWindowPropertyName::symbolDef].ToC4Value();
			break;
		case P_Target: val = C4Value(target); break;
		case P_Text: val = props[C4GuiWindowPropertyName::text].ToC4Value(); break;
		case P_Tooltip: val = props[C4GuiWindowPropertyName::tooltip].ToC4Value(); break;
		case P_ID: val = C4Value(id); break;
		case P_OnClick: val = props[C4GuiWindowPropertyName::onClickAction].ToC4Value(); break;
		case P_OnMouseIn: val = props[C4GuiWindowPropertyName::onMouseInAction].ToC4Value(); break;
		case P_OnMouseOut: val = props[C4GuiWindowPropertyName::onMouseOutAction].ToC4Value(); break;
		case P_OnClose: val = props[C4GuiWindowPropertyName::onCloseAction].ToC4Value(); break;
		case P_Style: val = props[C4GuiWindowPropertyName::style].ToC4Value(); break;
		case P_Mode: val = C4Value(int32_t(currentMouseState)); break;
		case P_Priority: val = props[C4GuiWindowPropertyName::priority].ToC4Value(); break;
		case P_Player: val = props[C4GuiWindowPropertyName::player].ToC4Value(); break;

		default:
			assert(false);
			break;
		}

		proplist->SetProperty(C4PropertyName(prop), val);
	}

	// save children now, construct new names for them if necessary
	int32_t childIndex = 0;
	for (C4GUI::Element * element : *this)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(element);
		C4Value val = child->ToC4Value();
		C4String *childName = child->name;
		if (!childName)
		{
			StdStrBuf childNameBuf;
			childNameBuf.Format("_child_%03d", ++childIndex);
			childName = Strings.RegString(childNameBuf);
		}
		proplist->SetPropertyByS(childName, val);
	}

	return C4Value(proplist);
}

bool C4GuiWindow::CreateFromPropList(C4PropList *proplist, bool resetStdTag, bool isUpdate, bool isLoading)
{
	if (!proplist) return false;
	C4GuiWindow * parent = static_cast<C4GuiWindow*>(GetParent());
	assert((parent || isLoading) && "GuiWindow created from proplist without parent (fails for ID tag)");

	bool layoutUpdateRequired = false; // needed for position changes etc
	// get properties from proplist and check for those, that match an allowed property to set them
	C4ValueArray *properties = proplist->GetProperties();
	C4String *stdTag = &Strings.P[P_Std];
	for (int32_t i = 0; i < properties->GetSize(); ++i)
	{
		const C4Value &entry = properties->GetItem(i);
		C4String *key = entry.getStr();
		assert(key && "PropList returns non-string as key");

		C4Value property;
		proplist->GetPropertyByS(key, &property);

		C4Value value;

		if(&Strings.P[P_Left] == key)
		{
			SetPositionStringProperties(property, C4GuiWindowPropertyName::relLeft, C4GuiWindowPropertyName::left, stdTag);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Top] == key)
		{
			SetPositionStringProperties(property, C4GuiWindowPropertyName::relTop, C4GuiWindowPropertyName::top, stdTag);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Right] == key)
		{
			SetPositionStringProperties(property, C4GuiWindowPropertyName::relRight, C4GuiWindowPropertyName::right, stdTag);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Bottom] == key)
		{
			SetPositionStringProperties(property, C4GuiWindowPropertyName::relBottom, C4GuiWindowPropertyName::bottom, stdTag);
			layoutUpdateRequired = true;
		}
		else if (&Strings.P[P_Margin] == key)
		{
			SetMarginProperties(property, stdTag);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_BackgroundColor] == key)
			props[C4GuiWindowPropertyName::backgroundColor].Set(property, stdTag);
		else if(&Strings.P[P_Target] == key)
			target = property.getObj();
		else if(&Strings.P[P_Symbol] == key)
		{
			props[C4GuiWindowPropertyName::symbolDef].Set(property, stdTag);
			props[C4GuiWindowPropertyName::symbolObject].Set(property, stdTag);
		}
		else if(&Strings.P[P_Decoration] == key)
		{
			props[C4GuiWindowPropertyName::frameDecoration].Set(property, stdTag);
		}
		else if(&Strings.P[P_Text] == key)
		{
			props[C4GuiWindowPropertyName::text].Set(property, stdTag);
			layoutUpdateRequired = true;
		}
		else if (&Strings.P[P_Tooltip] == key)
		{
			props[C4GuiWindowPropertyName::tooltip].Set(property, stdTag);
		}
		else if(&Strings.P[P_Prototype] == key)
			; // do nothing
		else if(&Strings.P[P_Mode] == key) // note that "Mode" is abused here for saving whether we have mouse focus
			currentMouseState = property.getInt();
		else if(&Strings.P[P_ID] == key)
		{
			// setting IDs is only valid for subwindows or when loading savegames!
			if (parent && !isMainWindow)
			{
				if (id) // already have an ID? remove from parent
					parent->ChildWithIDRemoved(this);
				id = property.getInt();
				if (id != 0)
					parent->ChildGotID(this);
			}
			else
				if (!isLoading)
					id = property.getInt();
		}
		else if(&Strings.P[P_OnClick] == key)
			props[C4GuiWindowPropertyName::onClickAction].Set(property, stdTag);
		else if(&Strings.P[P_OnMouseIn] == key)
			props[C4GuiWindowPropertyName::onMouseInAction].Set(property, stdTag);
		else if(&Strings.P[P_OnMouseOut] == key)
			props[C4GuiWindowPropertyName::onMouseOutAction].Set(property, stdTag);
		else if(&Strings.P[P_OnClose] == key)
			props[C4GuiWindowPropertyName::onCloseAction].Set(property, stdTag);
		else if(&Strings.P[P_Style] == key)
		{
			props[C4GuiWindowPropertyName::style].Set(property, stdTag);
			layoutUpdateRequired = true;
		}
		else if(&Strings.P[P_Priority] == key)
		{
			props[C4GuiWindowPropertyName::priority].Set(property, stdTag);
			layoutUpdateRequired = true;
			// resort into parent's list
			if (parent)
				parent->ChildChangedPriority(this);
		}
		else if(&Strings.P[P_Player] == key)
			props[C4GuiWindowPropertyName::player].Set(property, stdTag);
		else
		{
			// possibly sub-window?
			C4PropList *subwindow = property.getPropList();
			if (subwindow)
			{
				// remember the name of the child; but ignore names starting with underscores
				C4String *childName = nullptr;
				if (key->GetCStr()[0] != '_')
					childName = key;

				// Do we already have a child with that name? That implies that we are updating here.
				C4GuiWindow *child = GetChildByName(childName);
				bool freshlyAdded = false;
				
				// first time referencing a child with that name? Create a new one!
				if (!child)
				{
					child = new C4GuiWindow();
					child->name = childName;
					AddChild(child);
					freshlyAdded = true;
				}

				if (!child->CreateFromPropList(subwindow, isUpdate == true, false, isLoading))
				{
					// Remove the child again if we just added it. However, ignore when just updating an existing child.
					if (freshlyAdded)
						RemoveChild(child, false);
				}
				else
					layoutUpdateRequired = true;
			}
		}
	}

	if (layoutUpdateRequired)
		RequestLayoutUpdate();

	if (resetStdTag)
		SetTag(stdTag);

	return true;
}

void C4GuiWindow::ClearPointers(C4Object *pObj)
{
	// not removing or clearing anything twice
	// if this flag is set, the object will not be used after this frame (callbacks?) anyway
	if (wasRemoved) return;

	if (target == pObj)
	{
		Close();
		return;
	}
	
	// all properties which have anything to do with objects need to be called from here!
	props[C4GuiWindowPropertyName::symbolObject].ClearPointers(pObj);
	props[C4GuiWindowPropertyName::onClickAction].ClearPointers(pObj);
	props[C4GuiWindowPropertyName::onMouseInAction].ClearPointers(pObj);
	props[C4GuiWindowPropertyName::onMouseOutAction].ClearPointers(pObj);
	props[C4GuiWindowPropertyName::onCloseAction].ClearPointers(pObj);

	for (auto iter = begin(); iter != end();)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(*iter);
		// increment the iterator before (possibly) deleting the child
		++iter;
		child->ClearPointers(pObj);
	}
}

C4GuiWindow *C4GuiWindow::AddChild(C4GuiWindow *child)
{
	if (IsRoot())
	{
		child->SetID(GenerateMenuID());
		child->isMainWindow = true;
		// update all windows asap
		mainWindowNeedsLayoutUpdate = true;
	}

	// child's priority is ususally 0 here, so just insert it in front of other windows with a priority below 0
	// when the child's priority updates, the update function will be called and the child will be sorted to the correct position
	ChildChangedPriority(child);

	return child;
}

void C4GuiWindow::ChildChangedPriority(C4GuiWindow *child)
{
	int prio = child->props[C4GuiWindowPropertyName::priority].GetInt();
	C4GUI::Element * insertBefore = nullptr;

	for (C4GUI::Element * element : *this)
	{
		C4GuiWindow * otherChild = static_cast<C4GuiWindow*>(element);
		if (otherChild->props[C4GuiWindowPropertyName::priority].GetInt() <= prio) continue;
		insertBefore = element;
		break;
	}
	// if the child is already at the correct position, do nothing
	assert(child != insertBefore);
	// resort
	// this method will take care of removing and re-adding the child
	InsertElement(child, insertBefore);
}

void C4GuiWindow::ChildWithIDRemoved(C4GuiWindow *child)
{
	if (IsRoot()) return;
	if (!isMainWindow)
		return static_cast<C4GuiWindow*>(GetParent())->ChildWithIDRemoved(child);
	std::pair<std::multimap<int32_t, C4GuiWindow*>::iterator, std::multimap<int32_t, C4GuiWindow*>::iterator> range;
	range = childrenIDMap.equal_range(child->GetID());

	for (std::multimap<int32_t, C4GuiWindow*>::iterator iter = range.first; iter != range.second; ++iter)
	{
		if (iter->second != child) continue;
		childrenIDMap.erase(iter);
		//LogF("child-map-size: %d, remov %d [I am %d]", childrenIDMap.size(), child->GetID(), id);
		return;
	}
}

void C4GuiWindow::ChildGotID(C4GuiWindow *child)
{
	assert(!IsRoot() && "ChildGotID called on window root, should not propagate over main windows!");
	if (!isMainWindow)
		return static_cast<C4GuiWindow*>(GetParent())->ChildGotID(child);
	childrenIDMap.insert(std::make_pair(child->GetID(), child));
	//LogF("child+map+size: %d, added %d [I am %d]", childrenIDMap.size(), child->GetID(), id);
}

C4GuiWindow *C4GuiWindow::GetChildByID(int32_t childID)
{
	for (Element * element : *this)
	{
		C4GuiWindow * child = static_cast<C4GuiWindow*>(element);
		if (child->id != childID) continue;
		return child;
	}
	return nullptr;
}

C4GuiWindow *C4GuiWindow::GetChildByName(C4String *childName)
{
	// invalid child names never match
	if (childName == nullptr) return nullptr;

	for (Element * element : *this)
	{
		C4GuiWindow * child = static_cast<C4GuiWindow*>(element);
		// every C4String is unique, so we can compare pointers here
		if (child->name != childName) continue;
		return child;
	}
	return nullptr;
}

C4GuiWindow *C4GuiWindow::GetSubWindow(int32_t childID, C4Object *childTarget)
{
	std::pair<std::multimap<int32_t, C4GuiWindow*>::iterator, std::multimap<int32_t, C4GuiWindow*>::iterator> range;
	range = childrenIDMap.equal_range(childID);

	for (std::multimap<int32_t, C4GuiWindow*>::iterator iter = range.first; iter != range.second; ++iter)
	{
		C4GuiWindow *subwindow = iter->second;
		if (subwindow->GetTarget() != childTarget) continue;
		return subwindow;
	}
	return 0;
}

void C4GuiWindow::RemoveChild(C4GuiWindow *child, bool close, bool all)
{
	// do a layout update asap
	if (!all && !IsRoot())
		RequestLayoutUpdate();

	if (child && close)
	{
		child->wasRemoved = true;
		child->Close();
		if (child->GetID() != 0)
			ChildWithIDRemoved(child);
		RemoveElement(static_cast<C4GUI::Element*>(child));
	}
	else if (close) // close all children
	{
		assert(all);
		for (Element * element : *this)
		{
			C4GuiWindow * child = static_cast<C4GuiWindow*>(element);
			child->wasRemoved = true;
			child->Close();
			if (child->GetID() != 0)
				ChildWithIDRemoved(child);
		}
	}

	if (all)
		C4GUI::ScrollWindow::ClearChildren();
}

void C4GuiWindow::ClearChildren(bool close)
{
	RemoveChild(0, close, true);
}

void C4GuiWindow::Close()
{
	// first, close all children and dispose of them properly
	ClearChildren(true);

	if (!closeActionWasExecuted)
	{
		closeActionWasExecuted = true;

		// make call to target object if applicable
		C4GuiWindowAction *action = props[C4GuiWindowPropertyName::onCloseAction].GetAction();
		// only calls are valid actions for OnClose
		if (action && action->action == C4GuiWindowActionID::Call)
		{
			// close is always syncronized (script call/object removal) and thus the action can be executed immediately
			// (otherwise the GUI&action would have been removed anyway..)
			action->ExecuteCommand(action->id, this, NO_OWNER);
		}
	}

	if (!wasRemoved)
	{
		assert(GetParent() && "Close()ing GUIWindow without parent");
		static_cast<C4GuiWindow*>(GetParent())->RemoveChild(this);
	}
}

void C4GuiWindow::EnableScrollBar(bool enable, float childrenHeight)
{
	const int32_t &style = props[C4GuiWindowPropertyName::style].GetInt();

	if (style & C4GuiWindowStyleFlag::FitChildren)
	{
		float height = float(rcBounds.Hgt)
				- Em2Pix(props[C4GuiWindowPropertyName::topMargin].GetFloat())
				- Em2Pix(props[C4GuiWindowPropertyName::bottomMargin].GetFloat());
		float adjustment = childrenHeight - height;
		props[C4GuiWindowPropertyName::bottom].current->f += Pix2Em(adjustment);
		assert(!std::isnan(props[C4GuiWindowPropertyName::bottom].current->f));
		// instantly pseudo-update the sizes in case of multiple refreshs before the next draw
		rcBounds.Hgt += adjustment;
		// parents that are somehow affected by their children will need to refresh their layout
		if (adjustment != 0.0)
			RequestLayoutUpdate();
		return;
	}

	if (style & C4GuiWindowStyleFlag::NoCrop) return;

	C4GUI::ScrollWindow::SetScrollBarEnabled(enable);
}


void C4GuiWindow::UpdateLayoutGrid()
{
	const int32_t &width = rcBounds.Wdt;
	const int32_t &height = rcBounds.Hgt;

	const int32_t borderX(0), borderY(0);
	int32_t currentX = borderX;
	int32_t currentY = borderY;
	int32_t lowestChildRelY = 0;
	int32_t maxChildHeight = 0;

	for (C4GUI::Element * element : *this)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(element);
		// calculate the space the child needs, correctly respecting the margins
		const float childWdtF = float(child->rcBounds.Wdt)
			+ Em2Pix(child->props[C4GuiWindowPropertyName::leftMargin].GetFloat()) + Em2Pix(child->props[C4GuiWindowPropertyName::rightMargin].GetFloat())
			+ float(width) * (child->props[C4GuiWindowPropertyName::relLeftMargin].GetFloat() + child->props[C4GuiWindowPropertyName::relRightMargin].GetFloat());
		const float childHgtF = float(child->rcBounds.Hgt)
			+ Em2Pix(child->props[C4GuiWindowPropertyName::topMargin].GetFloat()) + Em2Pix(child->props[C4GuiWindowPropertyName::bottomMargin].GetFloat())
			+ float(height) * (child->props[C4GuiWindowPropertyName::relTopMargin].GetFloat() + child->props[C4GuiWindowPropertyName::relBottomMargin].GetFloat());
		// do all the rounding after the calculations
		const int32_t childWdt = (int32_t)(childWdtF + 0.5f);
		const int32_t childHgt = (int32_t)(childHgtF + 0.5f);
		// remember the highest child to make sure rows don't overlap
		if (!maxChildHeight || (childHgt > maxChildHeight))
		{
			maxChildHeight = childHgt;
			lowestChildRelY = currentY + childHgt;
		}
		child->rcBounds.x = currentX;
		child->rcBounds.y = currentY;

		currentX += childWdt + borderX;
		if (currentX + childWdt >= width)
		{
			currentX = borderX;
			currentY += maxChildHeight + borderY;
			maxChildHeight = 0;
		}
	}

	// do we need a scroll bar?
	EnableScrollBar(currentY > height, lowestChildRelY);
}

void C4GuiWindow::UpdateLayoutVertical()
{
	const int32_t borderY(0);
	int32_t currentY = borderY;

	for (C4GUI::Element * element : *this)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(element);

		// Do the calculations in floats first to not lose accuracy.
		// Take the height of the child and then add the margins.
		const float childHgtF = float(child->rcBounds.Hgt)
			+ Em2Pix(child->props[C4GuiWindowPropertyName::topMargin].GetFloat()) + Em2Pix(child->props[C4GuiWindowPropertyName::bottomMargin].GetFloat())
			+ float(rcBounds.Hgt) * (child->props[C4GuiWindowPropertyName::relTopMargin].GetFloat() + child->props[C4GuiWindowPropertyName::relBottomMargin].GetFloat());
		const int32_t childHgt = (int32_t)(childHgtF + 0.5f);

		child->rcBounds.y = currentY;
		currentY += childHgt + borderY;
	}

	// do we need a scroll bar?
	EnableScrollBar(currentY > rcBounds.Hgt, currentY);
}

bool C4GuiWindow::DrawChildren(C4TargetFacet &cgo, int32_t player, int32_t withMultipleFlag, C4Rect *currentClippingRect)
{
	// remember old target rectangle and adjust
	float oldTargetX = cgo.TargetX;
	float oldTargetY = cgo.TargetY;
	C4Rect myClippingRect;
	if (IsRoot())
	{
		cgo.TargetX = 0;
		cgo.TargetY = 0;
		pDraw->StorePrimaryClipper();
		// default: full screen clipper
		myClippingRect = C4Rect(0, 0, cgo.Wdt * cgo.Zoom, cgo.Hgt * cgo.Zoom);
		currentClippingRect = &myClippingRect;
	}

	// if ANY PARENT has scroll bar, then adjust clipper
	int32_t clipX1(0), clipX2(0), clipY1(0), clipY2(0);
	bool clipping = GetClippingRect(clipX1, clipY1, clipX2, clipY2);
	
	const int32_t targetClipX1 = cgo.TargetX + clipX1;
	const int32_t targetClipY1 = cgo.TargetY + clipY1;
	const int32_t targetClipX2 = cgo.TargetX + clipX2;
	const int32_t targetClipY2 = cgo.TargetY + clipY2;

	if (clipping)
	{
		myClippingRect = C4Rect(targetClipX1, targetClipY1, targetClipX2, targetClipY2);
		currentClippingRect = &myClippingRect;
	}

	if (withMultipleFlag != 1)
	{
		cgo.TargetX += rcBounds.x;
		cgo.TargetY += rcBounds.y - iScrollY;
	}
	else
	{
		assert(IsRoot());
		assert(withMultipleFlag == 1);
	}


	// note that withMultipleFlag only plays a roll for the root-menu
	bool oneDrawn = false; // was at least one child drawn?
	//for (auto iter = rbegin(); iter != rend(); ++iter)
	for (auto iter = begin(); iter != end(); ++iter)
	{
		C4GUI::Element *element = *iter;
		C4GuiWindow *child = static_cast<C4GuiWindow*>(element);

		if (withMultipleFlag != -1)
		{
			const int32_t &style = child->props[C4GuiWindowPropertyName::style].GetInt();
			if ((withMultipleFlag == 0) && (style & C4GuiWindowStyleFlag::Multiple)) continue;
			if ((withMultipleFlag == 1) && !(style & C4GuiWindowStyleFlag::Multiple)) continue;
		}
		
		pDraw->SetPrimaryClipper(currentClippingRect->x, currentClippingRect->y, currentClippingRect->Wdt, currentClippingRect->Hgt);

		if (child->Draw(cgo, player, currentClippingRect))
			oneDrawn = true;
		// draw only one window when drawing non-Multiple windows
		if (oneDrawn && (withMultipleFlag == 0)) break;
	}

	// scrolling obviously does not affect the scroll bar
	cgo.TargetY += iScrollY;
	if (pScrollBar->IsVisible())
		pScrollBar->DrawElement(cgo);
	
	if (IsRoot())
	{
		pDraw->RestorePrimaryClipper();
	}

	// restore target rectangle
	cgo.TargetX = oldTargetX;
	cgo.TargetY = oldTargetY;
	return oneDrawn;
}

void C4GuiWindow::RequestLayoutUpdate()
{
	if (isMainWindow)
	{
		const int32_t &style = props[C4GuiWindowPropertyName::style].GetInt();
		
		if (!(style & C4GuiWindowStyleFlag::Multiple)) // are we a simple centered window?
			mainWindowNeedsLayoutUpdate = true;
		else // we are one of the multiple windows.. the root better do a full refresh
			static_cast<C4GuiWindow*>(GetParent())->mainWindowNeedsLayoutUpdate = true;
	}
	else static_cast<C4GuiWindow*>(GetParent())->RequestLayoutUpdate();
}

bool C4GuiWindow::UpdateChildLayout(C4TargetFacet &cgo, float parentWidth, float parentHeight)
{
	for (Element * element : *this)
	{
		C4GuiWindow *window = static_cast<C4GuiWindow*>(element);
		window->UpdateLayout(cgo, parentWidth, parentHeight);
	}
	return true;
}

bool C4GuiWindow::UpdateLayout(C4TargetFacet &cgo)
{
	assert(IsRoot()); // we are root

	// assume I am the root and use the whole viewport for drawing - minus some standard border
	const float &left = props[C4GuiWindowPropertyName::left].GetFloat();
	const float &right = props[C4GuiWindowPropertyName::right].GetFloat();
	const float &top = props[C4GuiWindowPropertyName::top].GetFloat();
	const float &bottom = props[C4GuiWindowPropertyName::bottom].GetFloat();

	float fullWidth = cgo.Wdt * cgo.Zoom;
	float fullHeight = cgo.Hgt * cgo.Zoom;

	float wdt = fullWidth - Em2Pix(left) + Em2Pix(right);
	float hgt = fullHeight - Em2Pix(top) + Em2Pix(bottom);

	const bool needUpdate = mainWindowNeedsLayoutUpdate || (rcBounds.Wdt != int32_t(wdt)) || (rcBounds.Hgt != int32_t(hgt));

	if (needUpdate)
	{
		mainWindowNeedsLayoutUpdate = false;

		// these are the coordinates for the centered non-multiple windows
		rcBounds.x = Em2Pix(left);
		rcBounds.y = Em2Pix(top);
		rcBounds.Wdt = wdt;
		rcBounds.Hgt = hgt;

		// first update all multiple windows (that can cover the whole screen)
		for (Element * element : *this)
		{
			C4GuiWindow *child = static_cast<C4GuiWindow*>(element);
			const int32_t &style = child->props[C4GuiWindowPropertyName::style].GetInt();
			if (!(style & C4GuiWindowStyleFlag::Multiple)) continue;
			child->UpdateLayout(cgo, fullWidth, fullHeight);
		}
		// then update all "main" windows in the center of the screen
		// todo: adjust the size of the main window based on the border-windows drawn before
		for (Element * element : *this)
		{
			C4GuiWindow *child = static_cast<C4GuiWindow*>(element);
			const int32_t &style = child->props[C4GuiWindowPropertyName::style].GetInt();
			if ((style & C4GuiWindowStyleFlag::Multiple)) continue;
			child->UpdateLayout(cgo, wdt, hgt);
		}

		pScrollBar->SetVisibility(false);
	}
	return true;
}

bool C4GuiWindow::UpdateLayout(C4TargetFacet &cgo, float parentWidth, float parentHeight)
{
	// fetch style
	const int32_t &style = props[C4GuiWindowPropertyName::style].GetInt();
	// fetch current position as shortcut for overview
	const float &left = props[C4GuiWindowPropertyName::left].GetFloat();
	const float &right = props[C4GuiWindowPropertyName::right].GetFloat();
	const float &top = props[C4GuiWindowPropertyName::top].GetFloat();
	const float &bottom = props[C4GuiWindowPropertyName::bottom].GetFloat();

	const float &relLeft = props[C4GuiWindowPropertyName::relLeft].GetFloat();
	const float &relRight = props[C4GuiWindowPropertyName::relRight].GetFloat();
	const float &relTop = props[C4GuiWindowPropertyName::relTop].GetFloat();
	const float &relBottom = props[C4GuiWindowPropertyName::relBottom].GetFloat();

	// same for margins
	const float &leftMargin = props[C4GuiWindowPropertyName::leftMargin].GetFloat();
	const float &rightMargin = props[C4GuiWindowPropertyName::rightMargin].GetFloat();
	const float &topMargin = props[C4GuiWindowPropertyName::topMargin].GetFloat();
	const float &bottomMargin = props[C4GuiWindowPropertyName::bottomMargin].GetFloat();

	const float &relLeftMargin = props[C4GuiWindowPropertyName::relLeftMargin].GetFloat();
	const float &relRightMargin = props[C4GuiWindowPropertyName::relRightMargin].GetFloat();
	const float &relTopMargin = props[C4GuiWindowPropertyName::relTopMargin].GetFloat();
	const float &relBottomMargin = props[C4GuiWindowPropertyName::relBottomMargin].GetFloat();

	// calculate drawing rectangle
	float leftDrawX = relLeft * parentWidth + Em2Pix(left) + (Em2Pix(leftMargin) + relLeftMargin * parentWidth);
	float rightDrawX = relRight * parentWidth + Em2Pix(right) - (Em2Pix(rightMargin) + relRightMargin * parentWidth);
	float topDrawY = relTop * parentHeight + Em2Pix(top) + (Em2Pix(topMargin) + relTopMargin * parentHeight);
	float bottomDrawY = relBottom * parentHeight + Em2Pix(bottom) - (Em2Pix(bottomMargin) + relBottomMargin * parentHeight);
	float width = rightDrawX - leftDrawX;
	float height = bottomDrawY - topDrawY;

	rcBounds.x = leftDrawX;
	rcBounds.y = topDrawY;
	rcBounds.Wdt = width;
	rcBounds.Hgt = height;

	// if this window contains text, we auto-fit to the text height
	StdCopyStrBuf *strBuf = props[C4GuiWindowPropertyName::text].GetStrBuf();
	if (strBuf)
	{
		StdStrBuf sText;
		int32_t textHgt = ::GraphicsResource.FontRegular.BreakMessage(strBuf->getData(), rcBounds.Wdt, &sText, true);
		// enable auto scroll
		if (textHgt > rcBounds.Hgt)
			rcBounds.Hgt = textHgt;
	}

	UpdateChildLayout(cgo, width, height);
	
	// update scroll bar
	// C4GUI::ScrollWindow::UpdateOwnPos();

	// special layout selected?
	if (style & C4GuiWindowStyleFlag::GridLayout)
		UpdateLayoutGrid();
	else if (style & C4GuiWindowStyleFlag::VerticalLayout)
		UpdateLayoutVertical();

	// check if we need a scroll-bar
	int32_t topMostChild = 0;
	int32_t bottomMostChild = rcBounds.Hgt;
	for (Element * element : *this)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(element);
		const int32_t &childTop = child->rcBounds.y;
		const int32_t childBottom = childTop + child->rcBounds.Hgt;
		if (childTop < topMostChild) topMostChild = childTop;
		if (childBottom > bottomMostChild) bottomMostChild = childBottom;
	}
	// subtract one against rounding errors
	iClientHeight = bottomMostChild - topMostChild - 1;
	C4GUI::ScrollWindow::Update();

	pScrollBar->rcBounds.Wdt = C4GUI_ScrollBarWdt;
	pScrollBar->rcBounds.x = rcBounds.Wdt - pScrollBar->rcBounds.Wdt;
	pScrollBar->rcBounds.y = 0;
	pScrollBar->rcBounds.Hgt = rcBounds.Hgt;
	pScrollBar->Update();

	return true;
}

bool C4GuiWindow::DrawAll(C4TargetFacet &cgo, int32_t player)
{
	assert(IsRoot()); // we are root
	if (!IsVisible()) return false;
	// this will check whether the viewport resized and we need an update
	UpdateLayout(cgo);
	// step one: draw all multiple-tagged windows
	DrawChildren(cgo, player, 1);
	// TODO: adjust rectangle for main menu if multiple windows exist
	// step two: draw one "main" menu
	DrawChildren(cgo, player, 0);
	return true;
}

bool C4GuiWindow::Draw(C4TargetFacet &cgo, int32_t player, C4Rect *currentClippingRect)
{
	assert(!IsRoot()); // not root, root needs to receive DrawAll

	// message hidden?
	const int32_t &myPlayer = props[C4GuiWindowPropertyName::player].GetInt();
	if (!IsVisible() || (myPlayer != -1 && player != myPlayer) || (target && !target->IsVisible(player, false)))
	{
		return false;
	}
	
	if (mainWindowNeedsLayoutUpdate)
	{
		assert(GetParent() && (static_cast<C4GuiWindow*>(GetParent())->IsRoot()));
		assert(isMainWindow);
		C4GuiWindow * parent = static_cast<C4GuiWindow*>(GetParent());
		UpdateLayout(cgo, parent->rcBounds.Wdt, parent->rcBounds.Hgt);
		mainWindowNeedsLayoutUpdate = false;
	}

	

	float childOffsetY = 0.0f; // for scrolling

	// check whether we are scrolling
	//float childHgt = lastDrawPosition.bottomMostChild - lastDrawPosition.topMostChild;

	//if (scrollBar)
	//	childOffsetY = -1.0f * (scrollBar->offset * (childHgt - rcBounds.Hgt));

	const int32_t outDrawX = cgo.TargetX + rcBounds.x;
	const int32_t outDrawY = cgo.TargetY + rcBounds.y;
	const int32_t outDrawWdt = rcBounds.Wdt;
	const int32_t outDrawHgt = rcBounds.Hgt;
	const int32_t outDrawRight = outDrawX + rcBounds.Wdt;
	const int32_t outDrawBottom = outDrawY + rcBounds.Hgt;
	// draw various properties
	C4Facet cgoOut(cgo.Surface, outDrawX, outDrawY, outDrawWdt, outDrawHgt);

	const int32_t &backgroundColor = props[C4GuiWindowPropertyName::backgroundColor].GetInt();
	if (backgroundColor)
		pDraw->DrawBoxDw(cgo.Surface, outDrawX, outDrawY, outDrawRight - 1.0f, outDrawBottom - 1.0f, backgroundColor);

	C4GUI::FrameDecoration *frameDecoration = props[C4GuiWindowPropertyName::frameDecoration].GetFrameDecoration();

	if (frameDecoration)
	{
		// the frame decoration will adjust for cgo.TargetX/Y itself
		C4Rect rect(
			outDrawX - frameDecoration->iBorderLeft - cgo.TargetX, 
			outDrawY - frameDecoration->iBorderTop - cgo.TargetY, 
			outDrawWdt + frameDecoration->iBorderRight + frameDecoration->iBorderLeft, 
			outDrawHgt + frameDecoration->iBorderBottom + frameDecoration->iBorderTop);
		frameDecoration->Draw(cgo, rect);
	}

	C4Object *symbolObject = props[C4GuiWindowPropertyName::symbolObject].GetObject();
	if (symbolObject)
	{
		symbolObject->DrawPicture(cgoOut, false, NULL);
	}
	else
	{
		C4Def *symbolDef = props[C4GuiWindowPropertyName::symbolDef].GetDef();
		if (symbolDef)
		{
			symbolDef->Draw(cgoOut);
		}
	}

	StdCopyStrBuf *strBuf = props[C4GuiWindowPropertyName::text].GetStrBuf();

	if (strBuf)
	{
		StdStrBuf sText;
		int alignment = ALeft;
		int32_t textHgt = ::GraphicsResource.FontRegular.BreakMessage(strBuf->getData(), outDrawWdt, &sText, true);
		float textYOffset = 0.0f, textXOffset = 0.0f;
		if (style & C4GuiWindowStyleFlag::TextVCenter)
			textYOffset = float(outDrawHgt)/2.0f - float(textHgt)/2.0f;
		else if (style & C4GuiWindowStyleFlag::TextBottom)
			textYOffset += float(outDrawHgt) - float(textHgt);
		if (style & C4GuiWindowStyleFlag::TextHCenter)
		{
			int wdt, hgt;
			::GraphicsResource.FontRegular.GetTextExtent(sText.getData(), wdt, hgt, true);
			textXOffset = float(outDrawWdt)/ 2.0f - float(wdt) / 2.0f;
		}
		else if (style & C4GuiWindowStyleFlag::TextRight)
		{
			alignment = ARight;
			int wdt, hgt;
			::GraphicsResource.FontRegular.GetTextExtent(sText.getData(), wdt, hgt, true);
			textXOffset = float(outDrawWdt) - float(wdt);
		}
		pDraw->TextOut(sText.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface, outDrawX + textXOffset, outDrawY + textYOffset, 0xffffffff, ALeft);
	}


	if (GraphicsSystem.ShowMenuInfo) // print helpful debug info
	{
		C4GuiWindow * parent = static_cast<C4GuiWindow*>(GetParent());

		DWORD frameColor = C4RGB(100, 150, 100);
		if (currentMouseState & MouseState::Focus) frameColor = C4RGB(0, 255, 0);

		pDraw->DrawFrameDw(cgo.Surface, outDrawX, outDrawY, outDrawRight, outDrawBottom, frameColor);
		if (target || id)
		{
			StdStrBuf buf = FormatString("%s(%d)", target ? target->GetName() : "", id);
			pDraw->TextOut(buf.getData(), ::GraphicsResource.FontCaption, 1.0, cgo.Surface, cgo.X + outDrawRight, cgo.Y + outDrawBottom - ::GraphicsResource.FontCaption.GetLineHeight(), 0xffff00ff, ARight);
		}
		//StdStrBuf buf2 = FormatString("(%d, %d, %d, %d)", rcBounds.x, rcBounds.y, rcBounds.Wdt, rcBounds.Hgt);
		//pDraw->TextOut(buf2.getData(), ::GraphicsResource.FontCaption, 1.0, cgo.Surface, cgo.X + outDrawX + rcBounds.Wdt / 2, cgo.Y + outDrawY + +rcBounds.Hgt / 2, 0xff00ffff, ACenter);
	}

	DrawChildren(cgo, player, -1, currentClippingRect);
	return true;
}

bool C4GuiWindow::GetClippingRect(int32_t &left, int32_t &top, int32_t &right, int32_t &bottom)
{
	const int32_t &style = props[C4GuiWindowPropertyName::style].GetInt();
	if (IsRoot() || isMainWindow || (style & C4GuiWindowStyleFlag::NoCrop))
		return false;

	if (pScrollBar->IsVisible())
	{
		left = rcBounds.x;
		top = rcBounds.y;
		right = rcBounds.Wdt + left;
		bottom = rcBounds.Hgt + top;
		return true;
	}

	/*const int32_t &style = props[C4GuiWindowPropertyName::style].GetInt();
	if (!isMainWindow && !(style & C4GuiWindowStyleFlag::NoCrop))
		return static_cast<C4GuiWindow*>(GetParent())->GetClippingRect(left, top, right, bottom);
		*/
	return false;
}

void C4GuiWindow::SetTag(C4String *tag)
{
	// set tag on all properties
	for (uint32_t i = 0; i < C4GuiWindowPropertyName::_lastProp; ++i)
		if (props[i].SwitchTag(tag))
		{
			// only if tag could have changed position etc.
			if (i <= C4GuiWindowPropertyName::relBottom || i == C4GuiWindowPropertyName::text || i == C4GuiWindowPropertyName::style || i == C4GuiWindowPropertyName::priority)
				RequestLayoutUpdate();
		}

	// .. and children
	for (C4GUI::Element * element : *this)
		(static_cast<C4GuiWindow*>(element))->SetTag(tag);
}

void C4GuiWindow::MouseEnter()
{
	const int32_t &player = ::MouseControl.GetPlayer();
	assert(player != NO_OWNER);
}

void C4GuiWindow::OnMouseIn(int32_t player, int32_t parentOffsetX, int32_t parentOffsetY)
{
	assert(!HasMouseFocus() && "custom menu window properly loaded incorrectly!");
	currentMouseState = MouseState::Focus;

	// no need to notify children, this is done in MouseInput

	// update tooltip info if applicable
	StdCopyStrBuf *strBuf = props[C4GuiWindowPropertyName::tooltip].GetStrBuf();
	if (strBuf)
	{
		C4Viewport * viewport = ::Viewports.GetViewport(player);
		if (viewport)
		{
			const float guiZoom = viewport->GetGUIZoom();
			const float x = float(parentOffsetX + rcBounds.x) / guiZoom;
			const float y = float(parentOffsetY + rcBounds.y) / guiZoom;
			const float wdt = float(rcBounds.Wdt) / guiZoom;
			const float hgt = float(rcBounds.Hgt) / guiZoom;
			::MouseControl.SetTooltipRectangle(C4Rect(x, y, wdt, hgt));
			::MouseControl.SetTooltipText(*strBuf);
		}
	}
	// execute action
	int32_t actionType = C4GuiWindowPropertyName::onMouseInAction;
	C4GuiWindowAction *action = props[actionType].GetAction();
	if (!action) return;
	action->Execute(this, player, actionType);
}

void C4GuiWindow::MouseLeave()
{
	const int32_t &player = ::MouseControl.GetPlayer();
	assert(player != NO_OWNER);

}
void C4GuiWindow::OnMouseOut(int32_t player)
{
	assert(HasMouseFocus() && "custom menu window probably loaded incorrectly!");
	currentMouseState = MouseState::None;

	// needs to notify children
	for (C4GUI::Element *iter : *this)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(iter);
		if (child->HasMouseFocus())
			child->OnMouseOut(player);
	}

	// execute action
	int32_t actionType = C4GuiWindowPropertyName::onMouseOutAction;
	C4GuiWindowAction *action = props[actionType].GetAction();
	if (!action) return;
	action->Execute(this, player, actionType);
}

bool C4GuiWindow::MouseInput(int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam)
{
	// only called on root
	assert(IsRoot());
	// Only allow one window to catch the mouse input.
	// Do not simply return, however, since other windows might need OnMouseOut().
	bool oneActionAlreadyExecuted = false;
	// non-multiple-windows have a higher priority
	// this is important since they are also drawn on top
	for (int withMultipleFlag = 0; withMultipleFlag <= 1; ++withMultipleFlag)
	{
		for (auto iter = rbegin(); iter != rend(); ++iter)
		{
			C4GuiWindow *child = static_cast<C4GuiWindow*>(*iter);

			const int32_t &style = child->props[C4GuiWindowPropertyName::style].GetInt();
			if ((withMultipleFlag == 0) && (style & C4GuiWindowStyleFlag::Multiple)) continue;
			if ((withMultipleFlag == 1) && !(style & C4GuiWindowStyleFlag::Multiple)) continue;
			
			// we are root, we have to adjust the position for the "main" windows that are centered
			int32_t adjustedMouseX = 0, adjustedMouseY = mouseY;
			int32_t offsetX = 0, offsetY = 0;
			if (withMultipleFlag == 0)
			{
				offsetX = -rcBounds.x;
				offsetY = -rcBounds.y;
			}

			adjustedMouseX = mouseX + offsetX;
			adjustedMouseY = mouseY + offsetY;

			int32_t childLeft = child->rcBounds.x;
			int32_t childRight = child->rcBounds.x + child->rcBounds.Wdt;
			int32_t childTop = child->rcBounds.y;
			int32_t childBottom = child->rcBounds.y + child->rcBounds.Hgt;
			//LogF("%d|%d in %d|%d // %d|%d", mouseX, mouseY, childLeft, childTop, childRight, childBottom);
			bool inArea = true;
			if ((adjustedMouseX < childLeft) || (adjustedMouseX > childRight)) inArea = false;
			else if ((adjustedMouseY < childTop) || (adjustedMouseY > childBottom)) inArea = false;

			if (!inArea) // notify child if it had mouse focus before
			{
				if (child->HasMouseFocus())
					child->OnMouseOut(player);
				continue;
			}
			// Don't break since some more OnMouseOut might be necessary
			if (oneActionAlreadyExecuted) continue;

			
			// keep the mouse coordinates relative to the child's bounds
			if (child->ProcessMouseInput(button, adjustedMouseX - childLeft, adjustedMouseY - childTop - iScrollY, dwKeyParam, childLeft - offsetX, childTop + iScrollY - offsetY))
			{
				oneActionAlreadyExecuted = true;
			}
		}
	}

	return oneActionAlreadyExecuted;
}

bool C4GuiWindow::ProcessMouseInput(int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam, int32_t parentOffsetX, int32_t parentOffsetY)
{
	const int32_t &player = ::MouseControl.GetPlayer();
	assert(player != NO_OWNER);

	// completely ignore mouse if the appropriate flag is set
	const int32_t &style = props[C4GuiWindowPropertyName::style].GetInt();
	if (style & C4GuiWindowStyleFlag::IgnoreMouse)
		return false;

	// if the window belongs to an invisible object, don't show
	// the "normal" visibility will be handed by the parent callback
	if (target)
		if (!target->IsVisible(player, false))
			return false;

	// we have mouse focus! Is this new?
	if (!HasMouseFocus())
		OnMouseIn(player, parentOffsetX, parentOffsetY);

	// do not simply break the loop since some OnMouseOut might go missing
	bool oneActionAlreadyExecuted = false;

	const int32_t scrollAdjustedMouseY = mouseY + iScrollY;

	// children actually have a higher priority
	bool overChild = false; // remember for later, catch all actions that are in theory over children, even if not reaction (if main window)
	// use reverse iterator since children with higher Priority appear later in the list
	for (auto iter = rbegin(); iter != rend(); ++iter)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(*iter);
		const int32_t childLeft = child->rcBounds.x;
		const int32_t childRight = child->rcBounds.x + child->rcBounds.Wdt;
		const int32_t childTop = child->rcBounds.y;
		const int32_t childBottom = child->rcBounds.y + child->rcBounds.Hgt;

		bool inArea = true;
		if ((mouseX <= childLeft) || (mouseX > childRight)) inArea = false;
		else if ((scrollAdjustedMouseY <= childTop) || (scrollAdjustedMouseY > childBottom)) inArea = false;

		if (!inArea) // notify child if it had mouse focus before
		{
			if (child->HasMouseFocus())
				child->OnMouseOut(player);
			continue;
		}

		if (oneActionAlreadyExecuted) continue;

		overChild = true;
		// keep coordinates relative to children
		if (child->ProcessMouseInput(button, mouseX - childLeft, scrollAdjustedMouseY - childTop, dwKeyParam, parentOffsetX + rcBounds.x, parentOffsetY + rcBounds.y - iScrollY))
		{
			oneActionAlreadyExecuted = true;
		}
	}

	if (oneActionAlreadyExecuted) return true;

	//C4GUI::Element::MouseInput(rMouse, button, mouseX, mouseY, dwKeyParam);

	// remember button-down events. The action will only be executed on button-up
	if (button == C4MC_Button_LeftDown)
		currentMouseState |= MouseState::MouseDown;
	// trigger!
	if (button == C4MC_Button_LeftUp && (currentMouseState & MouseState::MouseDown))
	{
		C4GuiWindowAction *action = props[C4GuiWindowPropertyName::onClickAction].GetAction();
		if (action)
		{
			action->Execute(this, player, C4GuiWindowPropertyName::onClickAction);
			return true;
		}
	}

	// for scroll-enabled windows, scroll contents with wheel
	if (pScrollBar->IsVisible() && (button == C4MC_Button_Wheel))
	{
		short delta = (short)(dwKeyParam >> 16);
		ScrollBy(-delta);
		//float fac = (lastDrawPosition.bottomMostChild - lastDrawPosition.topMostChild);
		//if (fac == 0.0f) fac = 1.0f;
		//scrollBar->ScrollBy(-float(delta) / fac);
		return true;
	}

	// forward to scroll-bar if in area
	if (pScrollBar->IsVisible())
	{
		if ((mouseX > pScrollBar->rcBounds.x && mouseX < pScrollBar->rcBounds.x + pScrollBar->rcBounds.Wdt)
			&& (mouseY > pScrollBar->rcBounds.y && mouseY < pScrollBar->rcBounds.y + pScrollBar->rcBounds.Hgt))
		{
			C4GUI::CMouse mouse(mouseX, mouseY);
			if (::MouseControl.IsLeftDown()) mouse.LDown = true;
			pScrollBar->MouseInput(mouse, button, mouseX - pScrollBar->rcBounds.x, mouseY - pScrollBar->rcBounds.y, dwKeyParam);
		}
	}


	// if the user still clicked on a menu - even if it didn't do anything, catch it
	// but do that only on the top-level to not stop traversing other branches
	if (isMainWindow)
		return overChild;
	return false;
}

bool C4GuiWindow::ExecuteCommand(int32_t actionID, int32_t player, int32_t subwindowID, int32_t actionType, C4Object *target)
{
	if (isMainWindow && subwindowID) // we are a main window! try a shortcut through the ID?
	{
		//LogF("passing command... %d, %d, %d, %d, %d [I am %d, MW]", actionID, player, subwindowID, actionType, tag, id);
		// the reasoning for that shortcut is that I assume that usually windows with actions will also have an ID assigned
		// this obviously doesn't have to be the case, but I believe it's worth the try
		std::pair<std::multimap<int32_t, C4GuiWindow*>::iterator, std::multimap<int32_t, C4GuiWindow*>::iterator> range;
		range = childrenIDMap.equal_range(subwindowID);

		for (std::multimap<int32_t, C4GuiWindow*>::iterator iter = range.first; iter != range.second; ++iter)
		{
			if (iter->second->ExecuteCommand(actionID, player, subwindowID, actionType, target))
				return true;
		}
		// it is not possible that another window would match the criteria. Abort later after self-check
	}

	// are we elligible?
	if ((id == subwindowID) && (this->target == target))
	{

		std::list<C4GuiWindowAction*> allActions = props[actionType].GetAllActions();
		for (std::list<C4GuiWindowAction*>::iterator iter = allActions.begin(); iter != allActions.end(); ++iter)
		{
			C4GuiWindowAction *action = *iter;
			assert(action && "C4GuiWindowProperty::GetAllActions returned list with null-pointer");

			if (action->ExecuteCommand(actionID, this, player))
				return true;
		}

		// note that we should not simply return false here
		// there is no guarantee that only one window with that target&ID exists
	}

	// not caught, forward to children!
	// abort if main window, though. See above
	if (isMainWindow && subwindowID) return false;

	//for (std::list<C4GuiWindow*>::iterator iter = children.begin(); iter != children.end(); ++iter)
	for (C4GUI::Element *element : *this)
	{
		C4GuiWindow *child = static_cast<C4GuiWindow*>(element);
		if (child->ExecuteCommand(actionID, player, subwindowID, actionType, target))
			return true;
	}
	return false;
}

bool C4GuiWindow::IsRoot()
{
	return this == Game.GuiWindowRoot;
}
