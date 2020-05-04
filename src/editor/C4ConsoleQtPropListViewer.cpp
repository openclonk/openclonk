/*
* OpenClonk, http://www.openclonk.org
*
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

#include "C4Include.h"
#include "script/C4Value.h"
#include "editor/C4ConsoleQtPropListViewer.h"
#include "editor/C4ConsoleQtDefinitionListViewer.h"
#include "editor/C4ConsoleQtState.h"
#include "editor/C4ConsoleQtLocalizeString.h"
#include "editor/C4Console.h"
#include "object/C4Object.h"
#include "object/C4GameObjects.h"
#include "object/C4DefList.h"
#include "object/C4Def.h"
#include "script/C4Effect.h"
#include "script/C4AulExec.h"
#include "platform/C4SoundInstance.h"


/* Property delegate base class */

C4PropertyDelegate::C4PropertyDelegate(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: QObject(), factory(factory), set_function_type(C4PropertyPath::PPT_SetFunction)
{
	// Resolve getter+setter callback names
	if (props)
	{
		creation_props = C4VPropList(props);
		name = props->GetPropertyStr(P_Name);
		set_function = props->GetPropertyStr(P_Set);
		if (props->GetPropertyBool(P_SetGlobal))
		{
			set_function_type = C4PropertyPath::PPT_GlobalSetFunction;
		}
		else if (props->GetPropertyBool(P_SetRoot))
		{
			set_function_type = C4PropertyPath::PPT_RootSetFunction;
		}
		else
		{
			set_function_type = C4PropertyPath::PPT_SetFunction;
		}
		async_get_function = props->GetPropertyStr(P_AsyncGet);
		update_callback = props->GetPropertyStr(P_OnUpdate);
	}
}

void C4PropertyDelegate::UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const
{
	editor->setGeometry(option.rect);
}

bool C4PropertyDelegate::GetPropertyValueBase(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const
{
	switch (container.GetType())
	{
	case C4V_PropList:
		return container._getPropList()->GetPropertyByS(key, out_val);
	case C4V_Array:
		*out_val = container._getArray()->GetItem(index);
		return true;
	default:
		return false;
	}
}

bool C4PropertyDelegate::GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const
{
	if (async_get_function)
	{
		C4PropList *props = container.getPropList();
		if (props)
		{
			*out_val = props->Call(async_get_function.Get());
			return true;
		}
		return false;
	}
	else
	{
		return GetPropertyValueBase(container, key, index, out_val);
	}
}

QString C4PropertyDelegate::GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const
{
	return QString(v.GetDataString().getData());
}

QColor C4PropertyDelegate::GetDisplayTextColor(const C4Value &val, class C4Object *obj) const
{
	return QColor(); // invalid = default
}

QColor C4PropertyDelegate::GetDisplayBackgroundColor(const C4Value &val, class C4Object *obj) const
{
	return QColor(); // invalid = default
}

C4PropertyPath C4PropertyDelegate::GetPathForProperty(C4ConsoleQtPropListModelProperty *editor_prop) const
{
	C4PropertyPath path;
	if (editor_prop->property_path.IsEmpty())
		path = C4PropertyPath(editor_prop->parent_value.getPropList());
	else
		path = editor_prop->property_path;
	return GetPathForProperty(path, editor_prop->key ? editor_prop->key->GetCStr() : nullptr);
}

C4PropertyPath C4PropertyDelegate::GetPathForProperty(const C4PropertyPath &parent_path, const char *default_subpath) const
{
	// Get path
	C4PropertyPath subpath;
	if (default_subpath && *default_subpath)
		subpath = C4PropertyPath(parent_path, default_subpath);
	else
		subpath = parent_path;
	// Set path
	if (GetSetFunction())
	{
		subpath.SetSetPath(parent_path, GetSetFunction(), set_function_type);
	}
	return subpath;
}


/* Integer delegate */

C4PropertyDelegateInt::C4PropertyDelegateInt(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props), min(std::numeric_limits<int32_t>::min()), max(std::numeric_limits<int32_t>::max()), step(1)
{
	if (props)
	{
		min = props->GetPropertyInt(P_Min, min);
		max = props->GetPropertyInt(P_Max, max);
		step = props->GetPropertyInt(P_Step, step);
	}
}

void C4PropertyDelegateInt::SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const
{
	QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
	spinBox->setValue(val.getInt());
}

void C4PropertyDelegateInt::SetModelData(QObject *editor, const C4PropertyPath &property_path, C4ConsoleQtShape *prop_shape) const
{
	QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
	spinBox->interpretText();
	property_path.SetProperty(C4VInt(spinBox->value()));
	factory->GetPropertyModel()->DoOnUpdateCall(property_path, this);
}

QWidget *C4PropertyDelegateInt::CreateEditor(const C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	QSpinBox *editor = new QSpinBox(parent);
	editor->setMinimum(min);
	editor->setMaximum(max);
	editor->setSingleStep(step);
	connect(editor, &QSpinBox::editingFinished, this, [editor, this]() {
		emit EditingDoneSignal(editor);
	});
	// Selection in child enum: Direct focus
	if (by_selection && is_child) editor->setFocus();
	return editor;
}

bool C4PropertyDelegateInt::IsPasteValid(const C4Value &val) const
{
	// Check int type and limits
	if (val.GetType() != C4V_Int) return false;
	int32_t ival = val._getInt();
	return (ival >= min && ival <= max);
}


/* String delegate */

C4PropertyDelegateStringEditor::C4PropertyDelegateStringEditor(QWidget *parent, bool has_localization_button)
	: QWidget(parent), edit(nullptr), localization_button(nullptr), commit_pending(false), text_edited(false)
{
	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMargin(0);
	layout->setSpacing(0);
	edit = new QLineEdit(this);
	layout->addWidget(edit);
	if (has_localization_button)
	{
		localization_button = new QPushButton(QString(LoadResStr("IDS_CNS_MORE")), this);
		layout->addWidget(localization_button);
		connect(localization_button, &QPushButton::pressed, this, [this]() {
			// Show dialogue
			OpenLocalizationDialogue();
		});
	}
	connect(edit, &QLineEdit::returnPressed, this, [this]() {
		text_edited = true;
		commit_pending = true;
		emit EditingDoneSignal();
	});
	connect(edit, &QLineEdit::textEdited, this, [this]() {
		text_edited = true;
		commit_pending = true;
	});
}

void C4PropertyDelegateStringEditor::OpenLocalizationDialogue()
{
	if (!localization_dialogue)
	{
		// Make sure we have an updated value
		StoreEditedText();
		// Make sure we're using a localized string
		if (value.GetType() != C4V_PropList)
		{
			C4PropList *value_proplist = ::Game.AllocateTranslatedString();
			if (value.GetType() == C4V_String)
			{
				C4String *lang = ::Strings.RegString(lang_code);
				value_proplist->SetPropertyByS(lang, value);
			}
			value = C4VPropList(value_proplist);
		}
		// Open dialogue on value
		localization_dialogue.reset(new C4ConsoleQtLocalizeStringDlg(::Console.GetState()->window.get(), value));
		connect(localization_dialogue.get(), &C4ConsoleQtLocalizeStringDlg::accepted, this, [this]() {
			// Usually, the proplist owned by localization_dialogue is the same as this->value
			// However, it may have changed if there was an update call that modified the value while the dialogue was open
			// In this case, take the value from the dialogue
			SetValue(C4VPropList(localization_dialogue->GetTranslations()));
			// Finish editing on the value
			CloseLocalizationDialogue();
			commit_pending = true;
			emit EditingDoneSignal();
		});
		connect(localization_dialogue.get(), &C4ConsoleQtLocalizeStringDlg::rejected, this, [this]() {
			CloseLocalizationDialogue();
		});
		localization_dialogue->show();
	}
}

void C4PropertyDelegateStringEditor::CloseLocalizationDialogue()
{
	if (localization_dialogue)
	{
		localization_dialogue->close();
		localization_dialogue.reset();
	}
}

void C4PropertyDelegateStringEditor::StoreEditedText()
{
	if (text_edited)
	{
		// TODO: Would be better to handle escaping in the C4Value-to-string code
		QString new_value = edit->text();
		new_value = new_value.replace(R"(\)", R"(\\)").replace(R"(")", R"(\")");
		C4Value text_value = C4VString(new_value.toUtf8());
		// If translatable, always store as translation proplist
		// This makes it easier to collect strings to be localized in the localization overview
		if (localization_button)
		{
			C4PropList *value_proplist = this->value.getPropList();
			if (!value_proplist)
			{
				value_proplist = ::Game.AllocateTranslatedString();
			}
			C4String *lang = ::Strings.RegString(lang_code);
			value_proplist->SetPropertyByS(lang, text_value);
		}
		else
		{
			this->value = text_value;
		}
		text_edited = false;
	}
}

void C4PropertyDelegateStringEditor::SetValue(const C4Value &val)
{
	// Set editor text to value
	// Resolve text string and default language for localized strings
	C4String *s;
	C4Value language;
	if (localization_button)
	{
		s = ::Game.GetTranslatedString(val, &language, true);
		C4String *language_string = language.getStr();
		SCopy(language_string ? language_string->GetCStr() : Config.General.LanguageEx, lang_code, 2);
		QFontMetrics fm(localization_button->font());
		localization_button->setFixedWidth(fm.width(lang_code) + 4);
		localization_button->setText(QString(lang_code));
	}
	else
	{
		s = val.getStr();
	}
	edit->setText(QString(s ? s->GetCStr() : ""));
	// Remember full value with all localizations
	if (val.GetType() == C4V_PropList)
	{
		if (val != this->value)
		{
			// Localization proplist: Create a copy (C4Value::Copy() would be nice)
			C4PropList *new_value_proplist = new C4PropListScript();
			this->value = C4VPropList(new_value_proplist);
			C4PropList *val_proplist = val.getPropList();
			for (C4String *lang : val_proplist->GetSortedLocalProperties())
			{
				C4Value lang_string;
				val_proplist->GetPropertyByS(lang, &lang_string);
				new_value_proplist->SetPropertyByS(lang, lang_string);
			}
		}
	}
	else
	{
		this->value = val;
	}
}

C4Value C4PropertyDelegateStringEditor::GetValue()
{
	// Flush edits from the text field into value
	StoreEditedText();
	// Return current value
	return this->value;
}

C4PropertyDelegateString::C4PropertyDelegateString(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props)
{
	if (props)
	{
		translatable = props->GetPropertyBool(P_Translatable);
	}
}

void C4PropertyDelegateString::SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor*>(aeditor);
	editor->SetValue(val);
}

void C4PropertyDelegateString::SetModelData(QObject *aeditor, const C4PropertyPath &property_path, C4ConsoleQtShape *prop_shape) const
{
	Editor *editor = static_cast<Editor*>(aeditor);
	// Only set model data when pressing Enter explicitely; not just when leaving 
	if (editor->IsCommitPending())
	{
		property_path.SetProperty(editor->GetValue());
		factory->GetPropertyModel()->DoOnUpdateCall(property_path, this);
		editor->SetCommitPending(false);
	}
}

QWidget *C4PropertyDelegateString::CreateEditor(const C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	Editor *editor = new Editor(parent, translatable);
	// EditingDone on return or when leaving edit field after a change has been made
	connect(editor, &Editor::EditingDoneSignal, editor, [this, editor]() {
		emit EditingDoneSignal(editor);
	});
	// Selection in child enum: Direct focus
	if (by_selection && is_child) editor->setFocus();
	return editor;
}

QString C4PropertyDelegateString::GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const
{
	// Raw string without ""
	C4String *s = translatable ? ::Game.GetTranslatedString(v, nullptr, true) : v.getStr();
	return QString(s ? s->GetCStr() : "");
}

bool C4PropertyDelegateString::IsPasteValid(const C4Value &val) const
{
	// Check string type or translatable proplist
	if (val.GetType() == C4V_String) return true;
	if (translatable)
	{
		C4PropList *val_p = val.getPropList();
		if (val_p)
		{
			return val_p->GetPropertyStr(P_Function) == &::Strings.P[P_Translate];
		}
	}
	return false;
}


/* Delegate editor: Text left and button right */

C4PropertyDelegateLabelAndButtonWidget::C4PropertyDelegateLabelAndButtonWidget(QWidget *parent)
	: QWidget(parent), layout(nullptr), label(nullptr), button(nullptr), button_pending(false)
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMargin(0);
	layout->setSpacing(0);
	label = new QLabel(this);
	QPalette palette = label->palette();
	palette.setColor(label->foregroundRole(), palette.color(QPalette::HighlightedText));
	palette.setColor(label->backgroundRole(), palette.color(QPalette::Highlight));
	label->setPalette(palette);
	layout->addWidget(label);
	button = new QPushButton(QString(LoadResStr("IDS_CNS_MORE")), this);
	layout->addWidget(button);
	// Make sure to draw over view in background
	setPalette(palette);
	setAutoFillBackground(true);
}


/* Descend path delegate base class for arrays and proplist */

C4PropertyDelegateDescendPath::C4PropertyDelegateDescendPath(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props), edit_on_selection(true)
{
	if (props)
	{
		info_proplist = C4VPropList(props); // Descend info is this definition
		edit_on_selection = props->GetPropertyBool(P_EditOnSelection, edit_on_selection);
		descend_path = props->GetPropertyStr(P_DescendPath);
	}
}
 
void C4PropertyDelegateDescendPath::SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	editor->label->setText(GetDisplayString(val, nullptr, false));
	editor->last_value = val;
	editor->property_path = property_path;
	if (editor->button_pending) emit editor->button->pressed();
}

QWidget *C4PropertyDelegateDescendPath::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	// Otherwise create display and button to descend path
	Editor *editor;
	std::unique_ptr<Editor> peditor((editor = new Editor(parent)));
	connect(editor->button, &QPushButton::pressed, this, [editor, this]() {
		// Value to descend into: Use last value on auto-press because it will not have been updated into the game yet
		// (and cannot be without going async in network mode)
		// On regular button press, re-resolve path to value
		C4Value val = editor->button_pending ? editor->last_value : editor->property_path.ResolveValue();
		bool is_proplist = !!val.getPropList(), is_array = !!val.getArray();
		if (is_proplist || is_array)
		{
			C4PropList *info_proplist = this->info_proplist.getPropList();
			// Allow descending into a sub-path
			C4PropertyPath descend_property_path(editor->property_path);
			if (is_proplist && descend_path)
			{
				// Descend value into sub-path
				val._getPropList()->GetPropertyByS(descend_path.Get(), &val);
				// Descend info_proplist into sub-path
				if (info_proplist)
				{
					C4PropList *info_editorprops = info_proplist->GetPropertyPropList(P_EditorProps);
					if (info_editorprops)
					{
						C4Value sub_info_proplist_val;
						info_editorprops->GetPropertyByS(descend_path.Get(), &sub_info_proplist_val);
						info_proplist = sub_info_proplist_val.getPropList();
					}
				}
				// Descend property path into sub-path
				descend_property_path = C4PropertyPath(descend_property_path, descend_path->GetCStr());
			}
			// No info proplist: Fall back to regular proplist viewing mode
			if (!info_proplist) info_proplist = val.getPropList();
			this->factory->GetPropertyModel()->DescendPath(val, info_proplist, descend_property_path);
			::Console.EditCursor.InvalidateSelection();
		}
	});
	if (by_selection && edit_on_selection) editor->button_pending = true;
	return peditor.release();
}


/* Array descend delegate */

C4PropertyDelegateArray::C4PropertyDelegateArray(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateDescendPath(factory, props), max_array_display(0), element_delegate(nullptr)
{
	if (props)
	{
		max_array_display = props->GetPropertyInt(P_Display);
	}
}

void C4PropertyDelegateArray::ResolveElementDelegate() const
{
	if (!element_delegate)
	{
		C4Value element_delegate_value;
		C4PropList *info_proplist = this->info_proplist.getPropList();
		if (info_proplist) info_proplist->GetProperty(P_Elements, &element_delegate_value);
		element_delegate = factory->GetDelegateByValue(element_delegate_value);
	}
}

QString C4PropertyDelegateArray::GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const
{
	C4ValueArray *arr = v.getArray();
	if (!arr) return QString(LoadResStr("IDS_CNS_INVALID"));
	int32_t n = v._getArray()->GetSize();
	ResolveElementDelegate();
	if (max_array_display && n)
	{
		QString result = "[";
		for (int32_t i = 0; i < std::min<int32_t>(n, max_array_display); ++i)
		{
			if (i) result += ",";
			result += element_delegate->GetDisplayString(v._getArray()->GetItem(i), obj, true);
		}
		if (n > max_array_display) result += ",...";
		result += "]";
		return result;
	}
	else if (n || !short_names)
	{
		// Default display (or display with 0 elements): Just show element number
		return QString(LoadResStr("IDS_CNS_ARRAYSHORT")).arg(n);
	}
	else
	{
		// Short display of empty array: Just leave it out.
		return QString("");
	}
}

bool C4PropertyDelegateArray::IsPasteValid(const C4Value &val) const
{
	// Check array type and all contents
	C4ValueArray *arr = val.getArray();
	if (!arr) return false;
	int32_t n = arr->GetSize();
	if (n)
	{
		ResolveElementDelegate();
		for (int32_t i = 0; i < arr->GetSize(); ++i)
		{
			C4Value item = arr->GetItem(i);
			if (!element_delegate->IsPasteValid(item)) return false;
		}
	}
	return true;
}


/* Proplist descend delegate */

C4PropertyDelegatePropList::C4PropertyDelegatePropList(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateDescendPath(factory, props)
{
	if (props)
	{
		display_string = props->GetPropertyStr(P_Display);
	}
}

QString C4PropertyDelegatePropList::GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const
{
	C4PropList *data = v.getPropList();
	if (!data) return QString(LoadResStr("IDS_CNS_INVALID"));
	if (!display_string) return QString("{...}");
	C4PropList *info_proplist = this->info_proplist.getPropList();
	C4PropList *info_editorprops = info_proplist ? info_proplist->GetPropertyPropList(P_EditorProps) : nullptr;
	// Replace all {{name}} by property values of name
	QString result = display_string->GetCStr();
	int32_t pos0, pos1;
	C4Value cv;
	while ((pos0 = result.indexOf("{{")) >= 0)
	{
		pos1 = result.indexOf("}}", pos0+2);
		if (pos1 < 0) break; // placeholder not closed
		// Get child value
		QString substring = result.mid(pos0+2, pos1-pos0-2);
		C4RefCntPointer<C4String> psubstring = ::Strings.RegString(substring.toUtf8());
		if (!data->GetPropertyByS(psubstring.Get(), &cv)) cv.Set0();
		// Try to display using child delegate
		QString display_value;
		if (info_editorprops)
		{
			C4Value child_delegate_val;
			if (info_editorprops->GetPropertyByS(psubstring.Get(), &child_delegate_val))
			{
				C4PropertyDelegate *child_delegate = factory->GetDelegateByValue(child_delegate_val);
				if (child_delegate)
				{
					display_value = child_delegate->GetDisplayString(cv, obj, true);
				}
			}
		}
		// If there is no child delegate, fall back to GetDataString()
		if (display_value.isEmpty()) display_value = cv.GetDataString().getData();
		// Put value into display string
		result.replace(pos0, pos1 - pos0 + 2, display_value);
	}
	return result;
}

bool C4PropertyDelegatePropList::IsPasteValid(const C4Value &val) const
{
	// Check proplist type
	C4PropList *pval = val.getPropList();
	if (!pval) return false;
	// Are there restrictions on allowed properties?
	C4PropList *info_proplist = this->info_proplist.getPropList();
	C4PropList *info_editorprops = info_proplist ? info_proplist->GetPropertyPropList(P_EditorProps) : nullptr;
	if (!info_editorprops) return true; // No restrictions: Allow everything
	// Otherwise all types properties must be valid for paste
	// (Extra properties are OK)
	std::vector< C4String * > properties = info_editorprops->GetUnsortedProperties(nullptr, nullptr);
	for (C4String *prop_name : properties)
	{
		if (prop_name == &::Strings.P[P_Prototype]) continue;
		C4Value child_delegate_val;
		if (!info_editorprops->GetPropertyByS(prop_name, &child_delegate_val)) continue;
		C4PropertyDelegate *child_delegate = factory->GetDelegateByValue(child_delegate_val);
		if (!child_delegate) continue;
		C4Value child_val;
		pval->GetPropertyByS(prop_name, &child_val);
		if (!child_delegate->IsPasteValid(child_val)) return false;
	}
	return true;
}


/* Effect delegate: Allows removal and descend into proplist */

C4PropertyDelegateEffectEditor::C4PropertyDelegateEffectEditor(QWidget *parent) : QWidget(parent), layout(nullptr), remove_button(nullptr), edit_button(nullptr)
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMargin(0);
	layout->setSpacing(0);
	remove_button = new QPushButton(QString(LoadResStr("IDS_CNS_REMOVE")), this);
	layout->addWidget(remove_button);
	edit_button = new QPushButton(QString(LoadResStr("IDS_CNS_MORE")), this);
	layout->addWidget(edit_button);
	// Make sure to draw over view in background
	setAutoFillBackground(true);
}

C4PropertyDelegateEffect::C4PropertyDelegateEffect(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props)
{
}

void C4PropertyDelegateEffect::SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	editor->property_path = property_path;
}

QWidget *C4PropertyDelegateEffect::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	Editor *editor;
	std::unique_ptr<Editor> peditor((editor = new Editor(parent)));
	// Remove effect button
	connect(editor->remove_button, &QPushButton::pressed, this, [editor, this]() {
		// Compose an effect remove call
		editor->property_path.DoCall("RemoveEffect(nil, nil, %s)");
		emit EditingDoneSignal(editor);
	});
	// Edit effect button
	connect(editor->edit_button, &QPushButton::pressed, this, [editor, this]() {
		// Descend into effect proplist (if the effect still exists)
		C4Value effect_val = editor->property_path.ResolveValue();
		C4PropList *effect_proplist = effect_val.getPropList();
		if (!effect_proplist)
		{
			// Effect lost
			emit EditingDoneSignal(editor);
		}
		else
		{
			// Effect OK. Edit it.
			this->factory->GetPropertyModel()->DescendPath(effect_val, effect_proplist, editor->property_path);
			::Console.EditCursor.InvalidateSelection();
		}
	});
	return peditor.release();
}

QString C4PropertyDelegateEffect::GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const
{
	C4PropList *effect_proplist = v.getPropList();
	C4Effect *effect = effect_proplist ? effect_proplist->GetEffect() : nullptr;
	if (effect)
	{
		if (effect->IsActive())
		{
			return QString("t=%1, interval=%2").arg(effect->iTime).arg(effect->iInterval);
		}
		else
		{
			return QString(LoadResStr("IDS_CNS_DEADEFFECT"));
		}
	}
	else
	{
		return QString("nil");
	}
}

bool C4PropertyDelegateEffect::GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const
{
	// Resolve effect by calling script function
	if (!key) return false;
	*out_val = AulExec.DirectExec(::ScriptEngine.GetPropList(), key->GetCStr(), "resolve effect", false, nullptr);
	return true;
}

C4PropertyPath C4PropertyDelegateEffect::GetPathForProperty(C4ConsoleQtPropListModelProperty *editor_prop) const
{
	// Property path is used directly for getting effect. No set function needed.
	return editor_prop->property_path;
}


/* Color delegate */

C4PropertyDelegateColor::C4PropertyDelegateColor(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props), alpha_mask(0u)
{
	if (props)
	{
		alpha_mask = props->GetPropertyInt(P_Alpha) << 24;
	}
}

uint32_t GetTextColorForBackground(uint32_t background_color)
{
	// White text on dark background; black text on bright background
	uint8_t r = (background_color >> 16) & 0xff;
	uint8_t g = (background_color >> 8) & 0xff;
	uint8_t b = (background_color >> 0) & 0xff;
	int32_t lgt = r * 30 + g * 59 + b * 11;
	return (lgt > 16000) ? 0 : 0xffffff;
}

void C4PropertyDelegateColor::SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	uint32_t background_color = static_cast<uint32_t>(val.getInt()) & 0xffffff;
	uint32_t foreground_color = GetTextColorForBackground(background_color);
	QPalette palette = editor->label->palette();
	palette.setColor(editor->label->backgroundRole(), QColor(QRgb(background_color)));
	palette.setColor(editor->label->foregroundRole(), QColor(QRgb(foreground_color)));
	editor->label->setPalette(palette);
	editor->label->setAutoFillBackground(true);
	editor->label->setText(GetDisplayString(val, nullptr, false));
	editor->last_value = val;
}

void C4PropertyDelegateColor::SetModelData(QObject *aeditor, const C4PropertyPath &property_path, C4ConsoleQtShape *prop_shape) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	property_path.SetProperty(editor->last_value);
	factory->GetPropertyModel()->DoOnUpdateCall(property_path, this);
}

QWidget *C4PropertyDelegateColor::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	Editor *editor;
	std::unique_ptr<Editor> peditor((editor = new Editor(parent)));
	connect(editor->button, &QPushButton::pressed, this, [editor, this]() {
		this->OpenColorDialogue(editor);
	});
	// Selection in child enum: Open dialogue immediately
	if (by_selection && is_child) OpenColorDialogue(editor);
	return peditor.release();
}

QString C4PropertyDelegateColor::GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const
{
	return QString("#%1").arg(uint32_t(v.getInt()), 8, 16, QChar('0'));
}

QColor C4PropertyDelegateColor::GetDisplayTextColor(const C4Value &val, class C4Object *obj) const
{
	uint32_t background_color = static_cast<uint32_t>(val.getInt()) & 0xffffff;
	uint32_t foreground_color = GetTextColorForBackground(background_color);
	return QColor(foreground_color);
}

QColor C4PropertyDelegateColor::GetDisplayBackgroundColor(const C4Value &val, class C4Object *obj) const
{
	return static_cast<uint32_t>(val.getInt()) & 0xffffff;
}

bool C4PropertyDelegateColor::IsPasteValid(const C4Value &val) const
{
	// Color is always int
	if (val.GetType() != C4V_Int) return false;
	return true;
}

void C4PropertyDelegateColor::OpenColorDialogue(C4PropertyDelegateLabelAndButtonWidget *editor) const
{
	// Show actual dialogue to change the color
	QColor clr = QColorDialog::getColor(QColor(editor->last_value.getInt() & (~alpha_mask)), editor, QString(), QColorDialog::ShowAlphaChannel);
	editor->last_value.SetInt(clr.rgba() | alpha_mask);
	this->SetEditorData(editor, editor->last_value, C4PropertyPath()); // force update on display
	emit EditingDoneSignal(editor);
}


/* Enum delegate combo box item delegate */

bool C4StyledItemDelegateWithButton::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	// Mouse move over a cell: Display tooltip if over help button
	QEvent::Type trigger_type = (button_type == BT_Help) ? QEvent::MouseMove : QEvent::MouseButtonPress;
	if (event->type() == trigger_type)
	{
		QVariant btn = model->data(index, Qt::DecorationRole);
		if (!btn.isNull())
		{
			QMouseEvent *mevent = static_cast<QMouseEvent *>(event);
			if (option.rect.contains(mevent->localPos().toPoint()))
			{
				if (mevent->localPos().x() >= option.rect.x() + option.rect.width() - option.rect.height())
				{
					switch (button_type)
					{
					case BT_Help:
						if (Config.Developer.ShowHelp)
						{
							QString tooltip_text = model->data(index, Qt::ToolTipRole).toString();
							QToolTip::showText(mevent->globalPos(), tooltip_text);
						}
						break;
					case BT_PlaySound:
						StartSoundEffect(model->data(index, Qt::ToolTipRole).toString().toUtf8());
						return true; // handled
					}
				}
			}
		}
	}
	return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void C4StyledItemDelegateWithButton::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// Paint icon on the right
	QStyleOptionViewItem override_option = option;
	override_option.decorationPosition = QStyleOptionViewItem::Right;
	QStyledItemDelegate::paint(painter, override_option, index);
}



/* Enum delegate combo box */

C4DeepQComboBox::C4DeepQComboBox(QWidget *parent, C4StyledItemDelegateWithButton::ButtonType button_type, bool editable)
	: QComboBox(parent), last_popup_height(0), is_next_close_blocked(false), editable(editable), manual_text_edited(false)
{
	item_delegate = std::make_unique<C4StyledItemDelegateWithButton>(button_type);
	QTreeView *view = new QTreeView(this);
	view->setFrameShape(QFrame::NoFrame);
	view->setSelectionBehavior(QTreeView::SelectRows);
	view->setAllColumnsShowFocus(true);
	view->header()->hide();
	view->setItemDelegate(item_delegate.get());
	setEditable(editable);
	// On expansion, enlarge view if necessery
	connect(view, &QTreeView::expanded, this, [this, view](const QModelIndex &index)
	{
		if (this->model() && view->parentWidget())
		{
			int child_row_count = this->model()->rowCount(index);
			if (child_row_count > 0)
			{
				// Get space to contain expanded leaf+1 item
				QModelIndex last_index = this->model()->index(child_row_count - 1, 0, index);
				int needed_height = view->visualRect(last_index).bottom() - view->visualRect(index).top() + view->height() - view->parentWidget()->height() + view->visualRect(last_index).height();
				int available_height = QApplication::desktop()->availableGeometry(view->mapToGlobal(QPoint(1, 1))).height(); // but do not expand past screen size
				int new_height = std::min(needed_height, available_height - 20);
				if (view->parentWidget()->height() < new_height) view->parentWidget()->resize(view->parentWidget()->width(), (this->last_popup_height=new_height));
			}
		}
	});
	// On selection, highlight object in editor
	view->setMouseTracking(true);
	connect(view, &QTreeView::entered, this, [this](const QModelIndex &index)
	{
		C4Object *obj = nullptr;
		int32_t obj_number = this->model()->data(index, ObjectHighlightRole).toInt();
		if (obj_number) obj = ::Objects.SafeObjectPointer(obj_number);
		::Console.EditCursor.SetHighlightedObject(obj);
	});
	// New item selection through combo box: Update model position
	connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
		[this](int index)
	{ 
		QModelIndex current = this->view()->currentIndex();
		QVariant selected_data = this->model()->data(current, OptionIndexRole);
		if (selected_data.type() == QVariant::Int)
		{
			// Reset manual text edit flag because the text is now provided by the view
			manual_text_edited = false;
			// Finish selection
			setCurrentModelIndex(current);
			emit NewItemSelected(selected_data.toInt());
		}
	});
	// New text typed in
	if (editable)
	{
		// text change event only sent after manual text change
		connect(lineEdit(), &QLineEdit::textEdited, [this](const QString &text)
		{
			manual_text_edited = true;
			last_edited_text = text;
		});
		// reflect in data after return press and when focus is lost
		connect(lineEdit(), &QLineEdit::returnPressed, [this]()
		{
			if (manual_text_edited)
			{
				emit TextChanged(last_edited_text);
				manual_text_edited = false;
			}
		});
		connect(lineEdit(), &QLineEdit::editingFinished, [this]()
		{
			if (manual_text_edited)
			{
				emit TextChanged(last_edited_text);
				manual_text_edited = false;
			}
		});
	}
	// Connect view to combobox
	setView(view);
	view->viewport()->installEventFilter(this);
	// No help icons in main box, unless dropped down
	default_icon_size = iconSize();
	setIconSize(QSize(0, 0));
}

void C4DeepQComboBox::showPopup()
{
	// New selection: Reset to root of model
	setRootModelIndex(QModelIndex());
	setIconSize(default_icon_size);
	QComboBox::showPopup();
	view()->setMinimumWidth(200); // prevent element list from becoming too small in nested dialogues
	if (last_popup_height && view()->parentWidget()) view()->parentWidget()->resize(view()->parentWidget()->width(), last_popup_height);
}

void C4DeepQComboBox::hidePopup()
{
	// Cleanup tree combobox
	::Console.EditCursor.SetHighlightedObject(nullptr);
	setIconSize(QSize(0, 0));
	QComboBox::hidePopup();
}

void C4DeepQComboBox::setCurrentModelIndex(QModelIndex new_index)
{
	setRootModelIndex(new_index.parent());
	setCurrentIndex(new_index.row());
	// Adjust text
	if (editable)
	{
		lineEdit()->setText(this->model()->data(new_index, ValueStringRole).toString());
		manual_text_edited = false;
	}
}

int32_t C4DeepQComboBox::GetCurrentSelectionIndex()
{
	QVariant selected_data = model()->data(model()->index(currentIndex(), 0, rootModelIndex()), OptionIndexRole);
	if (selected_data.type() == QVariant::Int)
	{
		// Valid selection
		return selected_data.toInt();
	}
	else
	{
		// Invalid selection
		return -1;
	}
}

// event filter for view: Catch mouse clicks to prevent closing from simple mouse clicks
bool C4DeepQComboBox::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == view()->viewport())
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			QPoint pos = static_cast<QMouseEvent *>(event)->pos();
			QModelIndex pressed_index = view()->indexAt(pos);
			QRect item_rect = view()->visualRect(pressed_index);
			// Check if a group was clicked
			bool item_clicked = item_rect.contains(pos);
			if (item_clicked)
			{
				QVariant selected_data = model()->data(pressed_index, OptionIndexRole);
				if (selected_data.type() != QVariant::Int)
				{
					// This is a group. Just expand that entry.
					QTreeView *tview = static_cast<QTreeView *>(view());
					if (!tview->isExpanded(pressed_index))
					{
						tview->setExpanded(pressed_index, true);
						int32_t child_row_count = model()->rowCount(pressed_index);
						tview->scrollTo(model()->index(child_row_count - 1, 0, pressed_index), QAbstractItemView::EnsureVisible);
						tview->scrollTo(pressed_index, QAbstractItemView::EnsureVisible);
					}
					is_next_close_blocked = true;
					return true;
				}
			}
			else
			{
				is_next_close_blocked = true;
				return false;
			}
			// Delegate handling: The forward to delegate screws up for me sometimes and just stops randomly
			// Prevent this by calling the event directly
			QStyleOptionViewItem option;
			option.rect = view()->visualRect(pressed_index);
			if (item_delegate->editorEvent(event, model(), option, pressed_index))
			{
				// If the down event is taken by a music play event, ignore the following button up
				is_next_close_blocked = true;
				return true;
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			if (is_next_close_blocked)
			{
				is_next_close_blocked = false;
				return true;
			}
		}
	}
	return QComboBox::eventFilter(obj, event);
}


/* Enumeration delegate editor */

void C4PropertyDelegateEnumEditor::paintEvent(QPaintEvent *ev)
{
	// Draw self
	QWidget::paintEvent(ev);
	// Draw shape widget
	if (paint_parameter_delegate && parameter_widget)
	{
		QPainter p(this);
		QStyleOptionViewItem view_item;
		view_item.rect.setTopLeft(parameter_widget->mapToParent(parameter_widget->rect().topLeft()));
		view_item.rect.setBottomRight(parameter_widget->mapToParent(parameter_widget->rect().bottomRight()));
		paint_parameter_delegate->Paint(&p, view_item, last_parameter_val);
		//p.fillRect(view_item.rect, QColor("red"));
	}
}


/* Enumeration (dropdown list) delegate */

C4PropertyDelegateEnum::C4PropertyDelegateEnum(const C4PropertyDelegateFactory *factory, C4PropList *props, const C4ValueArray *poptions)
	: C4PropertyDelegate(factory, props), allow_editing(false), sorted(false)
{
	// Build enum options from C4Value definitions in script
	if (!poptions && props) poptions = props->GetPropertyArray(P_Options);
	C4String *default_option_key, *default_value_key = nullptr;
	if (props)
	{
		default_option_key = props->GetPropertyStr(P_OptionKey);
		default_value_key = props->GetPropertyStr(P_ValueKey);
		allow_editing = props->GetPropertyBool(P_AllowEditing);
		empty_name = props->GetPropertyStr(P_EmptyName);
		sorted = props->GetPropertyBool(P_Sorted);
		default_option.option_key = default_option_key;
		default_option.value_key = default_value_key;
	}
	if (poptions)
	{
		options.reserve(poptions->GetSize());
		for (int32_t i = 0; i < poptions->GetSize(); ++i)
		{
			const C4Value &v = poptions->GetItem(i);
			C4PropList *props = v.getPropList();
			if (!props) continue;
			Option option;
			option.props.SetPropList(props);
			option.name = props->GetPropertyStr(P_Name);
			if (!option.name) option.name = ::Strings.RegString("???");
			option.help = props->GetPropertyStr(P_EditorHelp);
			option.group = props->GetPropertyStr(P_Group);
			option.value_key = props->GetPropertyStr(P_ValueKey);
			if (!option.value_key) option.value_key = default_value_key;
			props->GetProperty(P_Value, &option.value);
			if (option.value.GetType() == C4V_Nil && empty_name) option.name = empty_name.Get();
			option.short_name = props->GetPropertyStr(P_ShortName);
			if (!option.short_name) option.short_name = option.name.Get();
			props->GetProperty(P_DefaultValueFunction, &option.value_function);
			option.type = C4V_Type(props->GetPropertyInt(P_Type, C4V_Any));
			option.option_key = props->GetPropertyStr(P_OptionKey);
			if (!option.option_key) option.option_key = default_option_key;
			// Derive storage type from given elements in delegate definition
			if (option.type != C4V_Any)
				option.storage_type = Option::StorageByType;
			else if (option.option_key && option.value.GetType() != C4V_Nil)
				option.storage_type = Option::StorageByKey;
			else
				option.storage_type = Option::StorageByValue;
			// Child delegate for value (resolved at runtime because there may be circular references)
			props->GetProperty(P_Delegate, &option.adelegate_val);
			option.priority = props->GetPropertyInt(P_Priority);
			option.force_serialization = props->GetPropertyInt(P_ForceSerialization);
			options.push_back(option);
		}
	}
}

QStandardItemModel *C4PropertyDelegateEnum::CreateOptionModel() const
{
	// Create a QStandardItemModel tree from all options and their groups
	std::unique_ptr<QStandardItemModel> model(new QStandardItemModel());
	model->setColumnCount(1);
	int idx = 0;
	for (const Option &opt : options)
	{
		QStandardItem *new_item = model->invisibleRootItem(), *parent = nullptr;
		QStringList group_names;
		if (opt.group) group_names.append(QString(opt.group->GetCStr()).split(QString("/")));
		group_names.append(opt.name->GetCStr());
		for (const QString &group_name : group_names)
		{
			parent = new_item;
			int row_index = -1;
			for (int check_row_index = 0; check_row_index < new_item->rowCount(); ++check_row_index)
				if (new_item->child(check_row_index, 0)->text() == group_name)
				{
					row_index = check_row_index;
					new_item = new_item->child(check_row_index, 0);
					break;
				}
			if (row_index < 0)
			{
				QStandardItem *new_group = new QStandardItem(group_name);
				if (sorted)
				{
					// Groups always sorted by name. Could also sort by priority of highest priority element?
					new_group->setData("010000000"+group_name, C4DeepQComboBox::PriorityNameSortRole);
				}
				new_item->appendRow(new_group);
				new_item = new_group;
			}
		}
		// If this item is already set, add a duplicate entry
		if (new_item->data(C4DeepQComboBox::OptionIndexRole).isValid())
		{
			new_item = new QStandardItem(QString(opt.name->GetCStr()));
			parent->appendRow(new_item);
		}
		// Sort key
		if (sorted)
		{
			// Reverse priority and make positive, so we can sort by descending priority but ascending name
			new_item->setData(QString(FormatString("%09d%s", (int)(10000000-opt.priority), opt.name->GetCStr()).getData()), C4DeepQComboBox::PriorityNameSortRole);
		}
		new_item->setData(QVariant(idx), C4DeepQComboBox::OptionIndexRole);
		C4Object *item_obj_data = opt.value.getObj();
		if (item_obj_data) new_item->setData(QVariant(item_obj_data->Number), C4DeepQComboBox::ObjectHighlightRole);
		QString help = QString((opt.help ? opt.help : opt.name)->GetCStr());
		new_item->setData(help.replace('|', '\n'), Qt::ToolTipRole);
		if (opt.help) new_item->setData(QIcon(":/editor/res/Help.png"), Qt::DecorationRole);
		if (opt.sound_name) new_item->setData(QIcon(":/editor/res/Sound.png"), Qt::DecorationRole);
		if (allow_editing)
		{
			C4String *s = opt.value.getStr();
			new_item->setData(QString(s ? s->GetCStr() : ""), C4DeepQComboBox::ValueStringRole);
		}
		++idx;
	}
	// Sort model and all groups
	if (sorted)
	{
		model->setSortRole(C4DeepQComboBox::PriorityNameSortRole);
		model->sort(0, Qt::AscendingOrder);
	}
	return model.release();
}

void C4PropertyDelegateEnum::ClearOptions()
{
	options.clear();
}

void C4PropertyDelegateEnum::ReserveOptions(int32_t num)
{
	options.reserve(num);
}

void C4PropertyDelegateEnum::AddTypeOption(C4String *name, C4V_Type type, const C4Value &val, C4PropertyDelegate *adelegate)
{
	Option option;
	option.name = name;
	option.short_name = name;
	option.type = type;
	option.value = val;
	option.storage_type = Option::StorageByType;
	option.adelegate = adelegate;
	options.push_back(option);
}

void C4PropertyDelegateEnum::AddConstOption(C4String *name, const C4Value &val, C4String *group, C4String *sound_name)
{
	Option option;
	option.name = name;
	option.short_name = name;
	option.group = group;
	option.value = val;
	option.storage_type = Option::StorageByValue;
	if (sound_name)
	{
		option.sound_name = sound_name;
		option.help = sound_name;
	}
	options.push_back(option);
}

int32_t C4PropertyDelegateEnum::GetOptionByValue(const C4Value &val) const
{
	int32_t iopt = 0;
	bool match = false;
	for (auto &option : options)
	{
		switch (option.storage_type)
		{
		case Option::StorageByType:
			match = (val.GetTypeEx() == option.type);
			break;
		case Option::StorageByValue:
			match = (val == option.value);
			break;
		case Option::StorageByKey: // Compare value to value in property. Assume undefined as nil.
		{
			C4PropList *props = val.getPropList();
			C4PropList *def_props = option.value.getPropList();
			if (props && def_props)
			{
				C4Value propval, defval;
				props->GetPropertyByS(option.option_key.Get(), &propval);
				def_props->GetPropertyByS(option.option_key.Get(), &defval);
				match = (defval == propval);
			}
			break;
		}
		default: break;
		}
		if (match) break;
		++iopt;
	}
	// If no option matches, return sentinel value
	return match ? iopt : Editor::INDEX_Custom_Value;
}

void C4PropertyDelegateEnum::UpdateEditorParameter(C4PropertyDelegateEnum::Editor *editor, bool by_selection) const
{
	// Recreate parameter settings editor associated with the currently selected option of an enum
	if (editor->parameter_widget)
	{
		editor->parameter_widget->deleteLater();
		editor->parameter_widget = nullptr;
	}
	editor->paint_parameter_delegate = nullptr;
	int32_t idx = editor->last_selection_index;
	if (by_selection)
	{
		idx = editor->option_box->GetCurrentSelectionIndex();
	}
	// No parameter delegate if not a known option (custom text or invalid value)
	if (idx < 0 || idx >= options.size()) return;
	const Option &option = options[idx];
	// Lazy-resolve parameter delegate
	EnsureOptionDelegateResolved(option);
	// Create editor if needed
	if (option.adelegate)
	{
		// Determine value to be shown in editor
		C4Value parameter_val;
		if (!by_selection)
		{
			// Showing current selection: From last_val assigned in SetEditorData or by custom text
			parameter_val = editor->last_val;
		}
		else
		{
			// Selecting a new item: Set the default value
			parameter_val = option.value;
			// Although the default value is taken directly from SetEditorData, it needs to be set here to make child access into proplists and arrays possible
			// (note that actual setting is delayed by control queue and this may often the wrong value in some cases - the correct value will be shown on execution of the queue)
			SetOptionValue(editor->last_get_path, option, option.value);
		}
		// Resolve parameter value
		if (option.value_key)
		{
			C4Value child_val;
			C4PropList *props = parameter_val.getPropList();
			if (props) props->GetPropertyByS(option.value_key.Get(), &child_val);
			parameter_val = child_val;
		}
		// Show it
		editor->parameter_widget = option.adelegate->CreateEditor(factory, editor, QStyleOptionViewItem(), by_selection, true);
		if (editor->parameter_widget)
		{
			editor->layout->addWidget(editor->parameter_widget);
			C4PropertyPath delegate_value_path = editor->last_get_path;
			if (option.value_key) delegate_value_path = C4PropertyPath(delegate_value_path, option.value_key->GetCStr());
			option.adelegate->SetEditorData(editor->parameter_widget, parameter_val, delegate_value_path);
			// Forward editing signals
			connect(option.adelegate, &C4PropertyDelegate::EditorValueChangedSignal, editor->parameter_widget, [this, editor](QWidget *changed_editor)
			{
				if (changed_editor == editor->parameter_widget)
					if (!editor->updating)
						emit EditorValueChangedSignal(editor);
			});
			connect(option.adelegate, &C4PropertyDelegate::EditingDoneSignal, editor->parameter_widget, [this, editor](QWidget *changed_editor)
			{
				if (changed_editor == editor->parameter_widget) emit EditingDoneSignal(editor);
			});
		}
		else
		{
			// If the parameter widget is a shape display, show a dummy widget displaying the shape instead
			const C4PropertyDelegateShape *shape_delegate = option.adelegate->GetDirectShapeDelegate();
			if (shape_delegate)
			{
				// dummy widget that is not rendered. shape rendering is forwarded through own paint function
				editor->parameter_widget = new QWidget(editor);
				editor->layout->addWidget(editor->parameter_widget);
				editor->parameter_widget->setAttribute(Qt::WA_NoSystemBackground);
				editor->parameter_widget->setAttribute(Qt::WA_TranslucentBackground);
				editor->parameter_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
				editor->paint_parameter_delegate = shape_delegate;
				editor->last_parameter_val = parameter_val;
			}
		}
	}
}

QModelIndex C4PropertyDelegateEnum::GetModelIndexByID(QStandardItemModel *model, QStandardItem *parent_item, int32_t id, const QModelIndex &parent) const
{
	// Resolve data stored in model to model index in tree
	for (int row = 0; row < parent_item->rowCount(); ++row)
	{
		QStandardItem *child = parent_item->child(row, 0);
		QVariant v = child->data(C4DeepQComboBox::OptionIndexRole);
		if (v.type() == QVariant::Int && v.toInt() == id) return model->index(row, 0, parent);
		if (child->rowCount())
		{
			QModelIndex child_match = GetModelIndexByID(model, child, id, model->index(row, 0, parent));
			if (child_match.isValid()) return child_match;
		}
	}
	return QModelIndex();
}

void C4PropertyDelegateEnum::SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor*>(aeditor);
	editor->last_val = val;
	editor->last_get_path = property_path;
	editor->updating = true;
	// Update option selection
	int32_t index = GetOptionByValue(val);
	if (index == Editor::INDEX_Custom_Value && !allow_editing)
	{
		// Invalid value and no custom values allowed? Select first item.
		index = 0;
	}
	if (index == Editor::INDEX_Custom_Value)
	{
		// Custom value
		C4String *val_string = val.getStr();
		QString edit_string = val_string ? QString(val_string->GetCStr()) : QString(val.GetDataString().getData());
		editor->option_box->setEditText(edit_string);
	}
	else
	{
		// Regular enum entry
		QStandardItemModel *model = static_cast<QStandardItemModel *>(editor->option_box->model());
		editor->option_box->setCurrentModelIndex(GetModelIndexByID(model, model->invisibleRootItem(), index, QModelIndex()));
	}
	editor->last_selection_index = index;
	// Update parameter
	UpdateEditorParameter(editor, false);
	editor->updating = false;
	// Execute pending dropdowns from creation as child enums
	if (editor->dropdown_pending)
	{
		editor->dropdown_pending = false;
		QMetaObject::invokeMethod(editor->option_box, "doShowPopup", Qt::QueuedConnection);
		editor->option_box->showPopup();
	}
}

void C4PropertyDelegateEnum::SetModelData(QObject *aeditor, const C4PropertyPath &property_path, C4ConsoleQtShape *prop_shape) const
{
	// Fetch value from editor
	Editor *editor = static_cast<Editor*>(aeditor);
	/*QStandardItemModel *model = static_cast<QStandardItemModel *>(editor->option_box->model());
	QModelIndex selected_model_index = model->index(editor->option_box->currentIndex(), 0, editor->option_box->rootModelIndex());
	QVariant vidx = model->data(selected_model_index, C4DeepQComboBox::OptionIndexRole);
	if (vidx.type() != QVariant::Int) return;
	int32_t idx = vidx.toInt();
	if (idx < 0 || idx >= options.size()) return;*/
	int32_t idx = editor->last_selection_index;
	const Option *option;
	const C4Value *option_value;
	if (idx < 0)
	{
		option = &default_option;
		option_value = &editor->last_val;
	}
	else
	{
		option = &options[std::max<int32_t>(idx, 0)];
		option_value = &option->value;
	}
	// Store directly in value or in a proplist field?
	C4PropertyPath use_path;
	if (option->value_key.Get())
		use_path = C4PropertyPath(property_path, option->value_key->GetCStr());
	else
		use_path = property_path;
	// Value from a parameter or directly from the enum?
	if (option->adelegate)
	{
		// Default value on enum change (on main path; not use_path because the default value is always given as the whole proplist)
		if (editor->option_changed) SetOptionValue(property_path, *option, *option_value);
		// Value from a parameter.
		// Using a setter function?
		use_path = option->adelegate->GetPathForProperty(use_path, nullptr);
		option->adelegate->SetModelData(editor->parameter_widget, use_path, prop_shape);
	}
	else
	{
		// No parameter. Use value.
		if (editor->option_changed) SetOptionValue(property_path, *option, *option_value);
	}
	editor->option_changed = false;
}

void C4PropertyDelegateEnum::SetOptionValue(const C4PropertyPath &use_path, const C4PropertyDelegateEnum::Option &option, const C4Value &option_value) const
{
	// After an enum entry has been selected, set its value
	// Either directly by value or through a function
	// Get serialization base
	const C4PropList *ignore_base_props;
	if (option.force_serialization)
	{
		ignore_base_props = option_value.getPropList();
		if (ignore_base_props) ignore_base_props = (ignore_base_props->IsStatic() ? ignore_base_props->IsStatic()->GetParent() : nullptr);
	}
	else
	{
		ignore_base_props = option.props.getPropList();
	}
	const C4PropListStatic *ignore_base_props_static = ignore_base_props ? ignore_base_props->IsStatic() : nullptr;
	if (option.value_function.GetType() == C4V_Function)
	{
		use_path.SetProperty(FormatString("Call(%s, %s, %s)", option.value_function.GetDataString().getData(), use_path.GetRoot(), option_value.GetDataString(20, ignore_base_props_static).getData()).getData());
	}
	else
	{
		use_path.SetProperty(option_value, ignore_base_props_static);
	}
	factory->GetPropertyModel()->DoOnUpdateCall(use_path, this);
}

QWidget *C4PropertyDelegateEnum::CreateEditor(const C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	Editor *editor = new Editor(parent);
	editor->layout = new QHBoxLayout(editor);
	editor->layout->setContentsMargins(0, 0, 0, 0);
	editor->layout->setMargin(0);
	editor->layout->setSpacing(0);
	editor->updating = true;
	editor->option_box = new C4DeepQComboBox(editor, GetOptionComboBoxButtonType(), allow_editing);
	editor->layout->addWidget(editor->option_box);
	for (auto &option : options) editor->option_box->addItem(option.name->GetCStr());
	editor->option_box->setModel(CreateOptionModel());
	editor->option_box->model()->setParent(editor->option_box);
	// Signal for selecting a new entry from the dropdown menu
	connect(editor->option_box, &C4DeepQComboBox::NewItemSelected, editor, [editor, this](int32_t newval) {
		if (!editor->updating) this->UpdateOptionIndex(editor, newval, nullptr); });
	// Signal for write-in on enum delegates that allow editing
	if (allow_editing)
	{
		connect(editor->option_box, &C4DeepQComboBox::TextChanged, editor, [editor, this](const QString &new_text) {
			if (!editor->updating)
			{
				this->UpdateOptionIndex(editor, GetOptionByValue(C4VString(new_text.toUtf8())), &new_text);
			}
		});
	}

	editor->updating = false;
	// If created by a selection from a parent enum, show drop down immediately after value has been set
	editor->dropdown_pending = by_selection && is_child;
	return editor;
}

void C4PropertyDelegateEnum::UpdateOptionIndex(C4PropertyDelegateEnum::Editor *editor, int newval, const QString *custom_text) const
{
	bool has_changed = false;
	// Change by text entry?
	if (custom_text)
	{
		C4String *last_value_string = editor->last_val.getStr();
		if (!last_value_string || last_value_string->GetData() != custom_text->toUtf8())
		{
			editor->last_val = C4VString(custom_text->toUtf8());
			has_changed = true;
		}
	}
	// Update value and parameter delegate if selection changed
	if (newval != editor->last_selection_index)
	{
		editor->last_selection_index = newval;
		UpdateEditorParameter(editor, !custom_text);
		has_changed = true;
	}
	// Change either by text entry or by dropdown selection: Emit signal to parent
	if (has_changed)
	{
		editor->option_changed = true;
		emit EditorValueChangedSignal(editor);
	}
}

void C4PropertyDelegateEnum::EnsureOptionDelegateResolved(const Option &option) const
{
	// Lazy-resolve parameter delegate
	if (!option.adelegate && option.adelegate_val.GetType() != C4V_Nil)
		option.adelegate = factory->GetDelegateByValue(option.adelegate_val);
}

QString C4PropertyDelegateEnum::GetDisplayString(const C4Value &v, class C4Object *obj, bool short_names) const
{
	// Display string from value
	int32_t idx = GetOptionByValue(v);
	if (idx == Editor::INDEX_Custom_Value)
	{
		// Value not found: Default display of strings; full display of nonsense values for debugging purposes.
		C4String *custom_string = v.getStr();
		if (custom_string)
		{
			return QString(custom_string->GetCStr());
		}
		else
		{
			return C4PropertyDelegate::GetDisplayString(v, obj, short_names);
		}
	}
	else
	{
		// Value found: Display option string plus parameter
		const Option &option = options[idx];
		QString result = (short_names ? option.short_name : option.name)->GetCStr();
		// Lazy-resolve parameter delegate
		EnsureOptionDelegateResolved(option);
		if (option.adelegate)
		{
			C4Value param_val = v;
			if (option.value_key.Get())
			{
				param_val.Set0();
				C4PropList *vp = v.getPropList();
				if (vp) vp->GetPropertyByS(option.value_key, &param_val);
			}
			if (!result.isEmpty()) result += " ";
			result += option.adelegate->GetDisplayString(param_val, obj, short_names);
		}
		return result;
	}
}

const C4PropertyDelegateShape *C4PropertyDelegateEnum::GetShapeDelegate(C4Value &val, C4PropertyPath *shape_path) const
{
	// Does this delegate own a shape? Forward decision into selected option.
	int32_t option_idx = GetOptionByValue(val);
	if (option_idx == Editor::INDEX_Custom_Value) return nullptr;
	const Option &option = options[option_idx];
	EnsureOptionDelegateResolved(option);
	if (!option.adelegate) return nullptr;
	if (option.value_key.Get())
	{
		*shape_path = option.adelegate->GetPathForProperty(*shape_path, option.value_key->GetCStr());
		C4PropList *vp = val.getPropList();
		val.Set0();
		if (vp) vp->GetPropertyByS(option.value_key, &val);
	}
	return option.adelegate->GetShapeDelegate(val, shape_path);
}

bool C4PropertyDelegateEnum::Paint(QPainter *painter, const QStyleOptionViewItem &option, const C4Value &val) const
{
	// Custom painting: Forward to selected child delegate
	int32_t option_idx = GetOptionByValue(val);
	if (option_idx == Editor::INDEX_Custom_Value) return false;
	const Option &selected_option = options[option_idx];
	EnsureOptionDelegateResolved(selected_option);
	if (!selected_option.adelegate) return false;
	if (selected_option.adelegate->HasCustomPaint())
	{
		QStyleOptionViewItem parameter_option = QStyleOptionViewItem(option);
		parameter_option.rect.adjust(parameter_option.rect.width()/2, 0, 0, 0);
		C4Value parameter_val = val;
		if (selected_option.value_key.Get())
		{
			parameter_val.Set0();
			C4PropList *vp = val.getPropList();
			if (vp) vp->GetPropertyByS(selected_option.value_key, &parameter_val);
		}
		selected_option.adelegate->Paint(painter, parameter_option, parameter_val);
	}
	// Always return false to draw self using the standard method
	return false;
}

bool C4PropertyDelegateEnum::IsPasteValid(const C4Value &val) const
{
	// Strings always OK in editable enums
	if (val.GetType() == C4V_String && allow_editing) return true;
	// Must be a valid selection
	int32_t option_idx = GetOptionByValue(val);
	if (option_idx == Editor::INDEX_Custom_Value) return false;
	const Option &option = options[option_idx];
	// Check validity for parameter
	EnsureOptionDelegateResolved(option);
	if (!option.adelegate) return true; // No delegate? Then any value is OK.
	C4Value parameter_val;
	if (option.value_key.Get())
	{
		C4PropList *vp = val.getPropList();
		if (!vp) return false;
		vp->GetPropertyByS(option.value_key, &parameter_val); // if this fails, check parameter against nil
	}
	else
	{
		parameter_val = val;
	}
	return option.adelegate->IsPasteValid(parameter_val);
}


/* Definition delegate */

C4PropertyDelegateDef::C4PropertyDelegateDef(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props)
{
	// nil is always an option
	AddConstOption(empty_name ? empty_name.Get() : ::Strings.RegString("nil"), C4VNull);
	// Collect sorted definitions
	filter_property = props ? props->GetPropertyStr(P_Filter) : nullptr;
	if (filter_property)
	{
		// With filter just create a flat list
		std::vector<C4Def *> defs = ::Definitions.GetAllDefs(filter_property);
		std::sort(defs.begin(), defs.end(), [](C4Def *a, C4Def *b) -> bool {
			return strcmp(a->GetName(), b->GetName()) < 0;
		});
		// Add them
		for (C4Def *def : defs)
		{
			C4RefCntPointer<C4String> option_name = ::Strings.RegString(FormatString("%s (%s)", def->id.ToString(), def->GetName()));
			AddConstOption(option_name, C4Value(def), nullptr);
		}
	}
	else
	{
		// Without filter copy tree from definition list model
		C4ConsoleQtDefinitionListModel *def_list_model = factory->GetDefinitionListModel();
		// Recursively add all defs from model
		AddDefinitions(def_list_model, QModelIndex(), nullptr);
	}
}

void C4PropertyDelegateDef::AddDefinitions(C4ConsoleQtDefinitionListModel *def_list_model, QModelIndex parent, C4String *group)
{
	int32_t count = def_list_model->rowCount(parent);
	for (int32_t i = 0; i < count; ++i)
	{
		QModelIndex index = def_list_model->index(i, 0, parent);
		C4Def *def = def_list_model->GetDefByModelIndex(index);
		C4RefCntPointer<C4String> name = ::Strings.RegString(def_list_model->GetNameByModelIndex(index));
		if (def) AddConstOption(name.Get(), C4Value(def), group);
		if (def_list_model->rowCount(index))
		{
			AddDefinitions(def_list_model, index, group ? ::Strings.RegString(FormatString("%s/%s", group->GetCStr(), name->GetCStr()).getData()) : name.Get());
		}
	}
}

bool C4PropertyDelegateDef::IsPasteValid(const C4Value &val) const
{
	// Must be a definition or nil
	if (val.GetType() == C4V_Nil) return true;
	C4Def *def = val.getDef();
	if (!def) return false;
	// Check filter
	if (filter_property)
	{
		C4Value prop_val;
		if (!def->GetPropertyByS(filter_property, &prop_val)) return false;
		if (!prop_val) return false;
	}
	return true;
}


/* Object delegate */

C4PropertyDelegateObject::C4PropertyDelegateObject(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props), max_nearby_objects(20)
{
	// Settings
	if (props)
	{
		filter = props->GetPropertyStr(P_Filter);
	}
	// Actual object list is created/updated when the editor is created
}

C4RefCntPointer<C4String> C4PropertyDelegateObject::GetObjectEntryString(C4Object *obj) const
{
	// Compose object display string from containment(*), name, position (@x,y) and object number (#n)
	return ::Strings.RegString(FormatString("%s%s @%d,%d (#%d)", obj->Contained ? "*" : "", obj->GetName(), (int)obj->GetX(), (int)obj->GetY(), (int)obj->Number));
}

void C4PropertyDelegateObject::UpdateObjectList()
{
	// Re-create object list from current position
	ClearOptions();
	// Get matching objects first
	std::vector<C4Object *> objects;
	for (C4Object *obj : ::Objects) if (obj->Status)
	{
		C4Value filter_val;
		if (filter)
		{
			if (!obj->GetPropertyByS(filter, &filter_val)) continue;
			if (!filter_val) continue;
		}
		objects.push_back(obj);
	}
	// Get list sorted by distance from selected object
	std::vector<C4Object *> objects_by_distance;
	int32_t cx=0, cy=0;
	if (::Console.EditCursor.GetCurrentSelectionPosition(&cx, &cy))
	{
		objects_by_distance = objects;
		auto ObjDist = [cx, cy](C4Object *o) { return (o->GetX() - cx)*(o->GetX() - cx) + (o->GetY() - cy)*(o->GetY() - cy); };
		std::stable_sort(objects_by_distance.begin(), objects_by_distance.end(), [&ObjDist](C4Object *a, C4Object *b) { return ObjDist(a) < ObjDist(b); });
	}
	size_t num_nearby = objects_by_distance.size();
	bool has_all_objects_list = (num_nearby > max_nearby_objects);
	if (has_all_objects_list) num_nearby = max_nearby_objects;
	// Add actual objects
	ReserveOptions(1 + num_nearby + !!num_nearby + (has_all_objects_list ? objects.size() : 0));
	AddConstOption(::Strings.RegString("nil"), C4VNull); // nil is always an option
	if (num_nearby)
	{
		// TODO: "Select object" entry
		//AddCallbackOption(LoadResStr("IDS_CNS_SELECTOBJECT"));
		// Nearby list
		C4RefCntPointer<C4String> nearby_group;
		// If there are main objects, Create a subgroup. Otherwise, just put all elements into the main group.
		if (has_all_objects_list) nearby_group = ::Strings.RegString(LoadResStr("IDS_CNS_NEARBYOBJECTS"));
		for (int32_t i = 0; i < num_nearby; ++i)
		{
			C4Object *obj = objects_by_distance[i];
			AddConstOption(GetObjectEntryString(obj).Get(), C4VObj(obj), nearby_group.Get());
		}
		// All objects
		if (has_all_objects_list)
		{
			C4RefCntPointer<C4String> all_group = ::Strings.RegString(LoadResStr("IDS_CNS_ALLOBJECTS"));
			for (C4Object *obj : objects) AddConstOption(GetObjectEntryString(obj).Get(), C4VObj(obj), all_group.Get());
		}
	}
}

QWidget *C4PropertyDelegateObject::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	// Update object list for created editor
	// (This should be safe since the object delegate cannot contain nested delegates)
	const_cast<C4PropertyDelegateObject *>(this)->UpdateObjectList();
	return C4PropertyDelegateEnum::CreateEditor(parent_delegate, parent, option, by_selection, is_child);
}

QString C4PropertyDelegateObject::GetDisplayString(const C4Value &v, class C4Object *obj, bool short_names) const
{
	C4Object *vobj = v.getObj();
	if (vobj)
	{
		C4RefCntPointer<C4String> s = GetObjectEntryString(vobj);
		return QString(s->GetCStr());
	}
	else
	{
		return QString(v.GetDataString().getData());
	}
}

bool C4PropertyDelegateObject::IsPasteValid(const C4Value &val) const
{
	// Must be an object or nil
	if (val.GetType() == C4V_Nil) return true;
	C4Object *obj = val.getObj();
	if (!obj) return false;
	// Check filter
	if (filter)
	{
		C4Value prop_val;
		if (!obj->GetPropertyByS(filter, &prop_val)) return false;
		if (!prop_val) return false;
	}
	return true;
}


/* Sound delegate */

C4PropertyDelegateSound::C4PropertyDelegateSound(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props)
{
	// Add none-option
	AddConstOption(::Strings.RegString("nil"), C4VNull);
	// Add all sounds as options
	for (C4SoundEffect *fx = ::Application.SoundSystem.GetFirstSound(); fx; fx = fx->Next)
	{
		// Extract group name as path to sound, replacing "::" by "/" for enum groups
		StdStrBuf full_name_s(fx->GetFullName(), true);
		RemoveExtension(&full_name_s);
		const char *full_name = full_name_s.getData();
		const char *base_name = full_name, *pos;
		StdStrBuf group_string;
		while ((pos = SSearch(base_name, "::")))
		{
			if (group_string.getLength()) group_string.AppendChar('/');
			group_string.Append(base_name, pos - base_name - 2);
			base_name = pos;
		}
		C4RefCntPointer<C4String> group;
		if (group_string.getLength()) group = ::Strings.RegString(group_string);
		// Script name: Full name (without extension)
		C4RefCntPointer<C4String> sound_string = ::Strings.RegString(full_name_s);
		// Add the option
		AddConstOption(::Strings.RegString(base_name), C4VString(sound_string.Get()), group.Get(), sound_string.Get());
	}
}

QString C4PropertyDelegateSound::GetDisplayString(const C4Value &v, class C4Object *obj, bool short_names) const
{
	// Always show full sound name
	C4String *val_string = v.getStr();
	return val_string ? QString(val_string->GetCStr()) : QString(v.GetDataString().getData());
}

bool C4PropertyDelegateSound::IsPasteValid(const C4Value &val) const
{
	// Must be nil or a string
	if (val.GetType() == C4V_Nil) return true;
	if (val.GetType() != C4V_String) return false;
	return true;
}


/* Boolean delegate */

C4PropertyDelegateBool::C4PropertyDelegateBool(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props)
{
	// Add boolean options
	ReserveOptions(2);
	AddConstOption(::Strings.RegString(LoadResStr("IDS_CNS_FALSE")), C4VBool(false));
	AddConstOption(::Strings.RegString(LoadResStr("IDS_CNS_TRUE")), C4VBool(true));
}

bool C4PropertyDelegateBool::GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const
{
	// Force value to bool
	bool success = C4PropertyDelegateEnum::GetPropertyValue(container, key, index, out_val);
	if (out_val->GetType() != C4V_Bool) *out_val = C4VBool(!!*out_val);
	return success;
}

bool C4PropertyDelegateBool::IsPasteValid(const C4Value &val) const
{
	// Must be a boolean
	if (val.GetType() != C4V_Bool) return false;
	return true;
}


/* Has-effect delegate */

C4PropertyDelegateHasEffect::C4PropertyDelegateHasEffect(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateBool(factory, props)
{
	if (props) effect = props->GetPropertyStr(P_Effect);
}

bool C4PropertyDelegateHasEffect::GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const
{
	const C4Object *obj = container.getObj();
	if (obj && effect)
	{
		bool has_effect = false;
		for (C4Effect *fx = obj->pEffects; fx; fx = fx->pNext)
			if (!fx->IsDead())
				if (!strcmp(fx->GetName(), effect->GetCStr()))
				{
					has_effect = true;
					break;
				}
		*out_val = C4VBool(has_effect);
		return true;
	}
	return false;
}


/* C4Value via an enumeration delegate */

C4PropertyDelegateC4ValueEnum::C4PropertyDelegateC4ValueEnum(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props)
{
	// Add default C4Value selections
	ReserveOptions(10);
	AddTypeOption(::Strings.RegString("nil"), C4V_Nil, C4VNull);
	AddTypeOption(::Strings.RegString("bool"), C4V_Bool, C4VNull, factory->GetDelegateByValue(C4VString("bool")));
	AddTypeOption(::Strings.RegString("int"), C4V_Int, C4VNull, factory->GetDelegateByValue(C4VString("int")));
	AddTypeOption(::Strings.RegString("string"), C4V_String, C4VNull, factory->GetDelegateByValue(C4VString("string")));
	AddTypeOption(::Strings.RegString("array"), C4V_Array, C4VNull, factory->GetDelegateByValue(C4VString("array")));
	AddTypeOption(::Strings.RegString("function"), C4V_Function, C4VNull, factory->GetDelegateByValue(C4VString("function")));
	AddTypeOption(::Strings.RegString("object"), C4V_Object, C4VNull, factory->GetDelegateByValue(C4VString("object")));
	AddTypeOption(::Strings.RegString("def"), C4V_Def, C4VNull, factory->GetDelegateByValue(C4VString("def")));
	AddTypeOption(::Strings.RegString("effect"), C4V_Effect, C4VNull, factory->GetDelegateByValue(C4VString("effect")));
	AddTypeOption(::Strings.RegString("proplist"), C4V_PropList, C4VNull, factory->GetDelegateByValue(C4VString("proplist")));
}


/* C4Value via an edit field delegate */

C4PropertyDelegateC4ValueInputEditor::C4PropertyDelegateC4ValueInputEditor(QWidget *parent)
	: QWidget(parent), layout(nullptr), edit(nullptr), extended_button(nullptr), commit_pending(false)
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMargin(0);
	layout->setSpacing(0);
	edit = new QLineEdit(this);
	layout->addWidget(edit);
	extended_button = new QPushButton("...", this);
	extended_button->setMaximumWidth(extended_button->fontMetrics().boundingRect("...").width() + 6);
	layout->addWidget(extended_button);
	extended_button->hide();
	edit->setFocus();
	setLayout(layout);
}

void C4PropertyDelegateC4ValueInput::SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	editor->edit->setText(val.GetDataString().getData());
	if (val.GetType() == C4V_PropList || val.GetType() == C4V_Array)
	{
		editor->extended_button->show();
		editor->property_path = property_path;
	}
	else
	{
		editor->extended_button->hide();
	}
}

void C4PropertyDelegateC4ValueInput::SetModelData(QObject *aeditor, const C4PropertyPath &property_path, C4ConsoleQtShape *prop_shape) const
{
	// Only set model data when pressing Enter explicitely; not just when leaving 
	Editor *editor = static_cast<Editor *>(aeditor);
	if (editor->commit_pending)
	{
		property_path.SetProperty(editor->edit->text().toUtf8());
		factory->GetPropertyModel()->DoOnUpdateCall(property_path, this);
		editor->commit_pending = false;
	}
}

QWidget *C4PropertyDelegateC4ValueInput::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const
{
	// Editor is just an edit box plus a "..." button for array/proplist types
	Editor *editor = new Editor(parent);
	// EditingDone only on Return; not just when leaving edit field
	connect(editor->edit, &QLineEdit::returnPressed, editor, [this, editor]() {
		editor->commit_pending = true;
		emit EditingDoneSignal(editor);
	});
	connect(editor->extended_button, &QPushButton::pressed, editor, [this, editor]() {
		C4Value val = editor->property_path.ResolveValue();
		if (val.getPropList() || val.getArray())
		{
			this->factory->GetPropertyModel()->DescendPath(val, val.getPropList(), editor->property_path);
			::Console.EditCursor.InvalidateSelection();
		}
	});
	// Selection in child enum: Direct focus
	if (by_selection && is_child) editor->edit->setFocus();
	return editor;
}


/* Areas shown in viewport */

C4PropertyDelegateShape::C4PropertyDelegateShape(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props), clr(0xffff0000)
{
	if (props)
	{
		clr = props->GetPropertyInt(P_Color) | 0xff000000;
	}
}

void C4PropertyDelegateShape::SetModelData(QObject *editor, const C4PropertyPath &property_path, C4ConsoleQtShape *prop_shape) const
{
	// Only set shape data if triggered through shape movement signal; ignore update calls from e.g. parent enum editor
	if (!editor)
	{
		if (prop_shape && prop_shape->GetParentDelegate() == this)
		{
			property_path.SetProperty(prop_shape->GetValue());
			factory->GetPropertyModel()->DoOnUpdateCall(property_path, this);
		}
	}
}

bool C4PropertyDelegateShape::Paint(QPainter *painter, const QStyleOptionViewItem &option, const C4Value &val) const
{
	// Background color
	if (option.state & QStyle::State_Selected)
		painter->fillRect(option.rect, option.palette.highlight());
	else
		painter->fillRect(option.rect, option.palette.base());
	// Draw a frame in shape color
	painter->save();
	QColor frame_color = QColor(QRgb(clr & 0xffffff));
	int32_t width = Clamp<int32_t>(option.rect.height() / 8, 2, 6) &~1;
	QPen rect_pen(QBrush(frame_color), width, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	painter->setPen(rect_pen);
	QRect inner_rect = option.rect.adjusted(width / 2, width / 2, -width / 2, -width / 2);
	if (inner_rect.width() > inner_rect.height())
	{
		// Draw shape in right corner
		inner_rect.adjust(inner_rect.width() - inner_rect.height(), 0, 0, 0);
	}
	// Paint by shape type
	DoPaint(painter, inner_rect);
	// Done painting
	painter->restore();
	return true;
}

void C4PropertyDelegateShape::ConnectSignals(C4ConsoleQtShape *shape, const C4PropertyPath &property_path) const
{
	connect(shape, &C4ConsoleQtShape::ShapeDragged, this, [this, shape, property_path]() {
		this->SetModelData(nullptr, property_path, shape);
	});
}

/* Areas shown in viewport: Rectangle */

C4PropertyDelegateRect::C4PropertyDelegateRect(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateShape(factory, props)
{
	if (props)
	{
		storage = props->GetPropertyStr(P_Storage);
	}
}

void C4PropertyDelegateRect::DoPaint(QPainter *painter, const QRect &inner_rect) const
{
	painter->drawRect(inner_rect);
}

bool C4PropertyDelegateRect::IsPasteValid(const C4Value &val) const
{
	// Check storage as prop list
	if (storage)
	{
		// Proplist-stored rect must have defined properties
		C4PropertyName def_property_names[2][4] = { { P_x, P_y, P_wdt, P_hgt },{ P_X, P_Y, P_Wdt, P_Hgt } };
		C4PropertyName *property_names = nullptr;
		if (storage == &::Strings.P[P_proplist])
		{
			property_names = def_property_names[0];
		}
		else if (storage == &::Strings.P[P_Proplist])
		{
			property_names = def_property_names[1];
		}
		if (property_names)
		{
			C4PropList *val_proplist = val.getPropList();
			if (!val_proplist) return false;
			for (int32_t i = 0; i < 4; ++i)
			{
				C4Value propval;
				if (!val_proplist->GetProperty(property_names[i], &propval)) return false;
				if (propval.GetType() != C4V_Int) return false;
			}
			// extra properties are OK
		}
		return true;
	}
	// Check storage as array: Expect array with four elements. Width and height non-negative.
	C4ValueArray *val_arr = val.getArray();
	if (!val_arr || val_arr->GetSize() != 4) return false;
	for (int32_t i = 0; i < 4; ++i) if (val_arr->GetItem(i).GetType() != C4V_Int) return false;
	if (val_arr->GetItem(2)._getInt() < 0) return false;
	if (val_arr->GetItem(3)._getInt() < 0) return false;
	return true;
}


/* Areas shown in viewport: Circle */

C4PropertyDelegateCircle::C4PropertyDelegateCircle(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateShape(factory, props)
{
	if (props)
	{
		can_move_center = props->GetPropertyBool(P_CanMoveCenter);
	}
}

void C4PropertyDelegateCircle::DoPaint(QPainter *painter, const QRect &inner_rect) const
{
	painter->drawEllipse(inner_rect);
	if (can_move_center) painter->drawPoint(inner_rect.center());
}

bool C4PropertyDelegateCircle::IsPasteValid(const C4Value &val) const
{
	// Circle radius stored as single non-negative int
	if (!can_move_center) return (val.GetType() == C4V_Int) && (val.getInt() >= 0);
	// Circle+Center stored as array with three elements (radius, x, y)
	C4ValueArray *val_arr = val.getArray();
	if (!val_arr || val_arr->GetSize() != 3) return false;
	for (int32_t i = 0; i < 3; ++i) if (val_arr->GetItem(i).GetType() != C4V_Int) return false;
	if (val_arr->GetItem(0)._getInt() < 0) return false;
	return true;
}


/* Areas shown in viewport: Point */

C4PropertyDelegatePoint::C4PropertyDelegatePoint(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateShape(factory, props)
{
	if (props)
	{
		horizontal_fix = props->GetPropertyBool(P_HorizontalFix);
		vertical_fix = props->GetPropertyBool(P_VerticalFix);
	}
}

void C4PropertyDelegatePoint::DoPaint(QPainter *painter, const QRect &inner_rect) const
{
	QPoint ctr = inner_rect.center();
	int r = inner_rect.height() * 7 / 20;
	if (horizontal_fix && !vertical_fix)
	{
		painter->drawLine(ctr + QPoint(0, -r), ctr + QPoint(0, +r));
		painter->drawLine(ctr + QPoint(-r / 2, -r), ctr + QPoint(+r / 2, -r));
		painter->drawLine(ctr + QPoint(-r / 2, +r), ctr + QPoint(+r / 2, +r));
	}
	else if (vertical_fix && !horizontal_fix)
	{
		painter->drawLine(ctr + QPoint(-r, 0), ctr + QPoint(+r, 0));
		painter->drawLine(ctr + QPoint(-r, -r / 2), ctr + QPoint(-r, +r / 2));
		painter->drawLine(ctr + QPoint(+r, -r / 2), ctr + QPoint(+r, +r / 2));
	}
	else
	{
		if (!horizontal_fix)
		{
			painter->drawLine(ctr + QPoint(-r, -r), ctr + QPoint(+r, +r));
		}
		painter->drawLine(ctr + QPoint(+r, -r), ctr + QPoint(-r, +r));
		painter->drawEllipse(inner_rect);
	}
}

bool C4PropertyDelegatePoint::IsPasteValid(const C4Value &val) const
{
	// Point stored as array with two elements
	C4ValueArray *val_arr = val.getArray();
	if (!val_arr || val_arr->GetSize() != 2) return false;
	for (int32_t i = 0; i < 2; ++i) if (val_arr->GetItem(i).GetType() != C4V_Int) return false;
	return true;
}


/* Areas shown in viewport: Graph */

C4PropertyDelegateGraph::C4PropertyDelegateGraph(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateShape(factory, props)
{
	if (props)
	{
		horizontal_fix = props->GetPropertyBool(P_HorizontalFix);
		vertical_fix = props->GetPropertyBool(P_VerticalFix);
		structure_fix = props->GetPropertyBool(P_StructureFix);
	}
}

void C4PropertyDelegateGraph::DoPaint(QPainter *painter, const QRect &inner_rect) const
{
	// Draw symbol as a bunch of connected lines
	QPoint ctr = inner_rect.center();
	int r = inner_rect.height() * 7 / 20;
	painter->drawLine(ctr, ctr + QPoint(-r / 2, -r));
	painter->drawLine(ctr, ctr + QPoint(+r / 2, -r));
	painter->drawLine(ctr, ctr + QPoint(0, +r));
}

bool C4PropertyDelegateGraph::IsVertexPasteValid(const C4Value &val) const
{
	// Check that it's an array of at least one point
	const C4ValueArray *arr = val.getArray();
	if (!arr || !arr->GetSize()) return false;
	// Check validity of each point
	const int32_t n_props = 2;
	C4PropertyName property_names[n_props] = { P_X, P_Y };
	for (int32_t i_pt = 0; i_pt < arr->GetSize(); ++i_pt)
	{
		const C4Value &pt = arr->GetItem(i_pt);
		const C4PropList *ptp = pt.getPropList();
		if (!ptp) return false;
		for (auto & property_name : property_names)
		{
			C4Value ptprop;
			if (!ptp->GetProperty(property_name, &ptprop)) return false;
			if (ptprop.GetType() != C4V_Int) return false;
		}
	}
	return true;
}

bool C4PropertyDelegateGraph::IsEdgePasteValid(const C4Value &val) const
{
	// Check that it's an array
	// Empty is OK; it could be a graph with one vertex and no edges
	const C4ValueArray *arr = val.getArray();
	if (!arr || !arr->GetSize()) return false;
	// Check validity of each edge
	for (int32_t i_pt = 0; i_pt < arr->GetSize(); ++i_pt)
	{
		const C4Value pt = arr->GetItem(i_pt);
		const C4ValueArray *pta;
		const C4PropList *ptp = pt.getPropList();
		if (!ptp) return false;
		pta = ptp->GetPropertyArray(P_Vertices);
		if (!pta) return false;
		// Needs two vertices (may have more values which are ignored)
		if (pta->GetSize() < 2) return false;
	}
	return true;
}

bool C4PropertyDelegateGraph::IsPasteValid(const C4Value &val) const
{
	// Unfortunately, there is no good way to determine the correct value for fixed structure / position graph pastes
	// So just reject pastes for now
	// (TODO: Could store a default structure in a property and compare to that)
	if (horizontal_fix || vertical_fix || structure_fix) return false;
	// Check storage as prop list
	const int32_t n_props = 2;
	C4Value prop_vals[n_props]; // vertices & edges
	C4PropertyName property_names[n_props] = { P_Vertices, P_Edges };
	C4PropList *val_proplist = val.getPropList();
	if (!val_proplist) return false;
	for (int32_t i = 0; i < n_props; ++i)
	{
		val_proplist->GetProperty(property_names[i], &prop_vals[i]);
	}
	// extra properties are OK
	// Check validity of vertices and edges
	return IsVertexPasteValid(prop_vals[0]) && IsEdgePasteValid(prop_vals[1]);
}

void C4PropertyDelegateGraph::ConnectSignals(C4ConsoleQtShape *shape, const C4PropertyPath &property_path) const
{
	C4ConsoleQtGraph *shape_graph = static_cast<C4ConsoleQtGraph *>(shape);
	connect(shape_graph, &C4ConsoleQtGraph::GraphEdit, this, [this, shape, property_path](C4ControlEditGraph::Action action, int32_t index, int32_t x, int32_t y) {
		// Send graph editing via queue
		::Control.DoInput(CID_EditGraph, new C4ControlEditGraph(property_path.GetGetPath(), action, index, x, y), CDT_Decide);
		// Also send update callback to root object
		factory->GetPropertyModel()->DoOnUpdateCall(property_path, this);
	});
	connect(shape, &C4ConsoleQtShape::BorderSelectionChanged, this, []() {
		// Different part of the shape selected: Refresh info on next update
		::Console.EditCursor.InvalidateSelection();
	});
}



/* Areas shown in viewport: Polyline */

C4PropertyDelegatePolyline::C4PropertyDelegatePolyline(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateGraph(factory, props)
{
}

void C4PropertyDelegatePolyline::DoPaint(QPainter *painter, const QRect &inner_rect) const
{
	// Draw symbol as a sequence of connected lines
	QPoint ctr = inner_rect.center();
	int r = inner_rect.height() * 7 / 20;
	painter->drawLine(ctr + QPoint(-r, +r), ctr + QPoint(-r/3, -r));
	painter->drawLine(ctr + QPoint(-r / 3, -r), ctr + QPoint(+r / 3, +r));
	painter->drawLine(ctr + QPoint(+r / 3, +r), ctr + QPoint(+r, -r));
}

bool C4PropertyDelegatePolyline::IsPasteValid(const C4Value &val) const
{
	// Expect just a vertex array
	return IsVertexPasteValid(val);
}


/* Areas shown in viewport: Closed polyon */

C4PropertyDelegatePolygon::C4PropertyDelegatePolygon(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateGraph(factory, props)
{
}

void C4PropertyDelegatePolygon::DoPaint(QPainter *painter, const QRect &inner_rect) const
{
	// Draw symbol as a parallelogram
	QPoint ctr = inner_rect.center();
	int r = inner_rect.height() * 7 / 20;
	painter->drawLine(ctr + QPoint(-r * 3 / 2, +r), ctr + QPoint(-r, -r));
	painter->drawLine(ctr + QPoint(-r, -r), ctr + QPoint(+r * 3 / 2, -r));
	painter->drawLine(ctr + QPoint(+r * 3 / 2, -r), ctr + QPoint(+r, +r));
	painter->drawLine(ctr + QPoint(+r, +r), ctr + QPoint(-r * 3 / 2, +r));
}

bool C4PropertyDelegatePolygon::IsPasteValid(const C4Value &val) const
{
	// Expect just a vertex array
	return IsVertexPasteValid(val);
}


/* Delegate factory: Create delegates based on the C4Value type */

C4PropertyDelegateFactory::C4PropertyDelegateFactory() : effect_delegate(this, nullptr)
{

}

C4PropertyDelegate *C4PropertyDelegateFactory::CreateDelegateByPropList(C4PropList *props) const
{
	if (props)
	{
		const C4String *str = props->GetPropertyStr(P_Type);
		if (str)
		{
			// create default base types
			if (str->GetData() == "int") return new C4PropertyDelegateInt(this, props);
			if (str->GetData() == "string") return new C4PropertyDelegateString(this, props);
			if (str->GetData() == "array") return new C4PropertyDelegateArray(this, props);
			if (str->GetData() == "proplist") return new C4PropertyDelegatePropList(this, props);
			if (str->GetData() == "color") return new C4PropertyDelegateColor(this, props);
			if (str->GetData() == "def") return new C4PropertyDelegateDef(this, props);
			if (str->GetData() == "object") return new C4PropertyDelegateObject(this, props);
			if (str->GetData() == "enum") return new C4PropertyDelegateEnum(this, props);
			if (str->GetData() == "sound") return new C4PropertyDelegateSound(this, props);
			if (str->GetData() == "bool") return new C4PropertyDelegateBool(this, props);
			if (str->GetData() == "has_effect") return new C4PropertyDelegateHasEffect(this, props);
			if (str->GetData() == "c4valueenum") return new C4PropertyDelegateC4ValueEnum(this, props);
			if (str->GetData() == "rect") return new C4PropertyDelegateRect(this, props);
			if (str->GetData() == "circle") return new C4PropertyDelegateCircle(this, props);
			if (str->GetData() == "point") return new C4PropertyDelegatePoint(this, props);
			if (str->GetData() == "graph") return new C4PropertyDelegateGraph(this, props);
			if (str->GetData() == "polyline") return new C4PropertyDelegatePolyline(this, props);
			if (str->GetData() == "polygon") return new C4PropertyDelegatePolygon(this, props);
			if (str->GetData() == "any") return new C4PropertyDelegateC4ValueInput(this, props);
			// unknown type
			LogF("Invalid delegate type: %s.", str->GetCStr());
		}
	}
	// Default fallback
	return new C4PropertyDelegateC4ValueInput(this, props);
}

C4PropertyDelegate *C4PropertyDelegateFactory::GetDelegateByValue(const C4Value &val) const
{
	auto iter = delegates.find(val.getPropList());
	if (iter != delegates.end()) return iter->second.get();
	C4PropertyDelegate *new_delegate = CreateDelegateByPropList(val.getPropList());
	delegates.insert(std::make_pair(val.getPropList(), std::unique_ptr<C4PropertyDelegate>(new_delegate)));
	return new_delegate;
}

C4PropertyDelegate *C4PropertyDelegateFactory::GetDelegateByIndex(const QModelIndex &index) const
{
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	if (!prop) return nullptr;
	if (!prop->delegate) prop->delegate = GetDelegateByValue(prop->delegate_info);
	return prop->delegate;
}

void C4PropertyDelegateFactory::ClearDelegates()
{
	delegates.clear();
}

void C4PropertyDelegateFactory::EditorValueChanged(QWidget *editor)
{
	emit commitData(editor);
}

void C4PropertyDelegateFactory::EditingDone(QWidget *editor)
{
	emit commitData(editor);
	//emit closeEditor(editor); - done by qt somewhere else...
}

void C4PropertyDelegateFactory::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	// Put property value from proplist into editor
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!CheckCurrentEditor(d, editor)) return;
	// Fetch property only first time - ignore further updates to the same value to simplify editing
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	if (!prop) return;
	C4Value val;
	d->GetPropertyValue(prop->parent_value, prop->key, index.row(), &val);
	if (!prop->about_to_edit && val == last_edited_value) return;
	last_edited_value = val;
	prop->about_to_edit = false;
	d->SetEditorData(editor, val, d->GetPathForProperty(prop));
}

void C4PropertyDelegateFactory::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	// Fetch property value from editor and set it into proplist
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!CheckCurrentEditor(d, editor)) return;
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	SetPropertyData(d, editor, prop);
}

void C4PropertyDelegateFactory::SetPropertyData(const C4PropertyDelegate *d, QObject *editor, C4ConsoleQtPropListModel::Property *editor_prop) const
{
	// Set according to delegate
	const C4PropertyPath set_path = d->GetPathForProperty(editor_prop);
	d->SetModelData(editor, set_path, editor_prop->shape ? editor_prop->shape->Get() : nullptr);
}

QWidget *C4PropertyDelegateFactory::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!d) return nullptr;
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	prop->about_to_edit = true;
	QWidget *editor = d->CreateEditor(this, parent, option, true, false);
	// Connect value change signals (if editing is possible for this property)
	// For some reason, commitData needs a non-const pointer
	if (editor)
	{
		connect(d, &C4PropertyDelegate::EditorValueChangedSignal, editor, [editor, this](QWidget *signal_editor) {
			if (signal_editor == editor) const_cast<C4PropertyDelegateFactory *>(this)->EditorValueChanged(editor);
		});
		connect(d, &C4PropertyDelegate::EditingDoneSignal, editor, [editor, this](QWidget *signal_editor) {
			if (signal_editor == editor) const_cast<C4PropertyDelegateFactory *>(this)->EditingDone(editor);
		});
	}
	current_editor = editor;
	current_editor_delegate = d;
	return editor;
}

void C4PropertyDelegateFactory::destroyEditor(QWidget *editor, const QModelIndex &index) const
{
	if (editor == current_editor)
	{
		current_editor = nullptr;
		current_editor_delegate = nullptr;
		::Console.EditCursor.SetHighlightedObject(nullptr);
	}
	QStyledItemDelegate::destroyEditor(editor, index);
}

void C4PropertyDelegateFactory::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!CheckCurrentEditor(d, editor)) return;
	return d->UpdateEditorGeometry(editor, option);
}

QSize C4PropertyDelegateFactory::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	int height = QApplication::fontMetrics().height() + 4;
	return QSize(100, height);
}

void C4PropertyDelegateFactory::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// Delegate has custom painting?
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (d && prop && d->HasCustomPaint())
	{
		C4Value val;
		d->GetPropertyValue(prop->parent_value, prop->key, index.row(), &val);
		if (d->Paint(painter, option, val)) return;
	}
	// Otherwise use default paint implementation
	QStyledItemDelegate::paint(painter, option, index);
}

void C4PropertyDelegateFactory::OnPropListChanged()
{
	if (current_editor) emit closeEditor(current_editor);
}

bool C4PropertyDelegateFactory::CheckCurrentEditor(C4PropertyDelegate *d, QWidget *editor) const
{
	if (!d || (editor && editor != current_editor) || d != current_editor_delegate)
	{
		//const_cast<C4PropertyDelegateFactory *>(this)->emit closeEditor(current_editor);
		destroyEditor(current_editor, QModelIndex());
		return false;
	}
	return true;
}

static const QString property_mime_type("application/OpenClonkProperty");

void C4PropertyDelegateFactory::CopyToClipboard(const QModelIndex &index)
{
	// Re-resolve property. May have shifted while the menu was open
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!prop || !d) return;
	// Get data to copy
	C4Value val;
	d->GetPropertyValue(prop->parent_value, prop->key, index.row(), &val);
	StdStrBuf data_str(val.GetDataString(99999));
	// Copy it as an internal mime type and text
	// Presence of the internal type shows that this is a copied property so it can be safely evaluate without sync problems
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->clear();
	std::unique_ptr<QMimeData> data(new QMimeData());
	data->setData(property_mime_type, QByteArray(data_str.getData(), data_str.getSize()));
	data->setText(data_str.getData());
	clipboard->setMimeData(data.release());
}

bool C4PropertyDelegateFactory::PasteFromClipboard(const QModelIndex &index, bool check_only)
{
	// Re-resolve property. May have shifted while the menu was open
	C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!prop || !d) return false;
	// Check value to paste
	QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *data = clipboard->mimeData();
	if (!data) return false; // empty clipboard
	// Prefer copied property; fall back to text
	StdStrBuf str_data;
	if (data->hasFormat(property_mime_type))
	{
		QByteArray prop_data = data->data(property_mime_type);
		str_data.Copy(prop_data);
		// Check data type
		C4Value val = ::AulExec.DirectExec(&::ScriptEngine, str_data.getData(), "paste check", false, nullptr, false);
		if (!d->IsPasteValid(val)) return false;
	}
	else if (data->hasText())
	{
		// Text can always be pasted.
		// Cannot perform a type check here because a function may have been copied that affects sync.
		QString text = data->text();
		str_data.Copy(text.toUtf8());
	}
	else
	{
		// Unknown data type in clipboard. Cannot paste.
		return false;
	}
	if (check_only) return true;
	// Alright, paste!
	d->GetPathForProperty(prop).SetProperty(str_data.getData());
	return true;
}

bool C4PropertyDelegateFactory::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	// Custom context menu on item
	// I would like to use the regular context menu functions of Qt on the parent widget
	// but something is eating the right-click event before it triggers a context event.
	// So just hack it on right click.
	// Button check
	if (event->type() == QEvent::Type::MouseButtonPress)
	{
		QMouseEvent *mev = static_cast<QMouseEvent *>(event);
		if (mev->button() == Qt::MouseButton::RightButton)
		{
			// Item check
			C4ConsoleQtPropListModel::Property *prop = property_model->GetPropByIndex(index);
			C4PropertyDelegate *d = GetDelegateByIndex(index);
			if (d && prop)
			{
				// Context menu on a valid property: Show copy+paste menu
				QMenu *context = new QMenu(const_cast<QWidget *>(option.widget));
				QAction *copy_action = new QAction(LoadResStr("IDS_DLG_COPY"), context);
				QAction *paste_action = new QAction(LoadResStr("IDS_DLG_PASTE"), context);
				QModelIndex index_copy(index);
				connect(copy_action, &QAction::triggered, this, [this, index_copy]() {
					this->CopyToClipboard(index_copy);
				});
				connect(paste_action, &QAction::triggered, this, [this, index_copy]() {
					this->PasteFromClipboard(index_copy, false);
				});
				paste_action->setEnabled(PasteFromClipboard(index_copy, true)); // Paste grayed out if not valid
				context->addAction(copy_action);
				context->addAction(paste_action);
				context->popup(mev->globalPos());
				context->connect(context, &QMenu::aboutToHide, context, &QWidget::deleteLater);
				// It's easier to see which item is affected when it's selected
				QItemSelectionModel *sel_model = property_model->GetSelectionModel();
				QItemSelection new_sel;
				new_sel.select(model->index(index.row(), 0, index.parent()), index);
				sel_model->select(new_sel, QItemSelectionModel::SelectionFlag::SelectCurrent);
				return true;
			}
		}
	}
	return QStyledItemDelegate::editorEvent(event, model, option, index);
}


/* Proplist table view */

C4ConsoleQtPropListModel::C4ConsoleQtPropListModel(C4PropertyDelegateFactory *delegate_factory)
	: delegate_factory(delegate_factory), selection_model(nullptr)
{
	header_font.setBold(true);
	important_property_font.setBold(true);
	connect(this, &C4ConsoleQtPropListModel::ProplistChanged, this, &C4ConsoleQtPropListModel::UpdateSelection, Qt::QueuedConnection);
	layout_valid = false;
}

C4ConsoleQtPropListModel::~C4ConsoleQtPropListModel() = default;

bool C4ConsoleQtPropListModel::AddPropertyGroup(C4PropList *add_proplist, int32_t group_index, QString name, C4PropList *target_proplist, const C4PropertyPath &group_target_path, C4Object *base_object, C4String *default_selection, int32_t *default_selection_index)
{
	// Add all properties from this EditorProps group
	std::vector<C4String *> property_names = add_proplist->GetUnsortedProperties(nullptr);
	if (!property_names.size()) return false;
	// Prepare group array
	if (property_groups.size() == group_index)
	{
		layout_valid = false;
		property_groups.resize(group_index + 1);
	}
	PropertyGroup &properties = property_groups[group_index];
	// Resolve properties
	struct PropAndKey
	{
		C4PropList *prop;
		C4String *key;
		int32_t priority;
		C4String *name;

		PropAndKey(C4PropList *prop, C4String *key, int32_t priority, C4String *name)
			: prop(prop), key(key), priority(priority), name(name) {}
	};
	std::vector<PropAndKey> new_properties_resolved;
	new_properties_resolved.reserve(property_names.size());
	for (C4String *prop_name : property_names)
	{
		C4Value prop_val;
		add_proplist->GetPropertyByS(prop_name, &prop_val);
		C4PropList *prop = prop_val.getPropList();
		if (prop)
		{
			C4String *name = prop->GetPropertyStr(P_Name);
			if (!name) name = prop_name;
			int32_t priority = prop->GetPropertyInt(P_Priority);
			new_properties_resolved.emplace_back(PropAndKey({ prop, prop_name, priority, name }));
		}
	}
	// Sort by priority primarily and name secondarily
	std::sort(new_properties_resolved.begin(), new_properties_resolved.end(), [](const PropAndKey &a, const PropAndKey &b) -> bool {
		if (a.priority != b.priority) return a.priority > b.priority;
		return strcmp(a.name->GetCStr(), b.name->GetCStr()) < 0;
	});
	// Setup group
	properties.name = name;
	if (properties.props.size() != new_properties_resolved.size())
	{
		layout_valid = false;
		properties.props.resize(new_properties_resolved.size());
	}
	for (int32_t i = 0; i < new_properties_resolved.size(); ++i)
	{
		Property *prop = &properties.props[i];
		// Property access path
		prop->parent_value.SetPropList(target_proplist);
		prop->property_path = group_target_path;
		// ID for default selection memory
		const PropAndKey &prop_def = new_properties_resolved[i];
		if (default_selection == prop_def.key) *default_selection_index = i;
		// Property data
		prop->help_text = nullptr;
		prop->delegate_info.Set0(); // default C4Value delegate
		prop->group_idx = group_index;
		prop->about_to_edit = false;
		prop->key = prop_def.prop->GetPropertyStr(P_Key);
		if (!prop->key) properties.props[i].key = prop_def.key;
		prop->display_name = prop_def.name;
		if (!prop->display_name) prop->display_name = prop_def.key;
		prop->help_text = prop_def.prop->GetPropertyStr(P_EditorHelp);
		prop->priority = prop_def.priority;
		prop->delegate_info.SetPropList(prop_def.prop);
		prop->delegate = delegate_factory->GetDelegateByValue(prop->delegate_info);
		C4Value v;
		C4Value v_target_proplist = C4VPropList(target_proplist);
		prop->delegate->GetPropertyValue(v_target_proplist, prop->key, 0, &v);
		// Connect editable shape to property
		C4PropertyPath new_shape_property_path = prop->delegate->GetPathForProperty(prop);
		const C4PropertyDelegateShape *new_shape_delegate = prop->delegate->GetShapeDelegate(v, &new_shape_property_path);
		if (new_shape_delegate != prop->shape_delegate || !(prop->shape_property_path == new_shape_property_path))
		{
			prop->shape_delegate = new_shape_delegate;
			prop->shape_property_path = new_shape_property_path;
			if (new_shape_delegate)
			{
				// Re-use loaded shape if possible (e.g. if only the index has moved)
				std::string shape_index = std::string(prop->shape_property_path.GetGetPath());
				prop->shape = &shapes[shape_index];
				C4ConsoleQtShape *shape = prop->shape->Get();
				if (shape)
				{
					if (shape->GetProperties() != new_shape_delegate->GetCreationProps().getPropList())
					{
						// Shape at same path but with different properties? Then re-create
						shape = nullptr;
					}
				}
				if (!shape)
				{
					// New shape or shape type mismatch: Generate new shape at this path and put into the shape holder list
					shape = ::Console.EditCursor.GetShapes()->CreateShape(base_object ? base_object : target_proplist->GetObject(), new_shape_delegate->GetCreationProps().getPropList(), v, new_shape_delegate);
					new_shape_delegate->ConnectSignals(shape, prop->shape_property_path);
					prop->shape->Set(shape);
					prop->shape->SetLastValue(v);
				}
			}
			else
			{
				prop->shape = nullptr;
			}
		}
		if (prop->shape)
		{
			// Mark this shape to be kept aftre update is complete
			prop->shape->visit();
			// Update shape by value if it was changed externally
			if (!prop->shape->GetLastValue().IsIdenticalTo(v))
			{
				prop->shape->Get()->SetValue(v);
				prop->shape->SetLastValue(v);
			}
		}
	}
	return true;
}

bool C4ConsoleQtPropListModel::AddEffectGroup(int32_t group_index, C4Object *base_object)
{
	// Count non-dead effects
	C4Effect **effect_list = base_object ? &base_object->pEffects : &::ScriptEngine.pGlobalEffects;
	int32_t num_effects = 0;
	for (C4Effect *effect = *effect_list; effect; effect = effect->pNext)
	{
		num_effects += effect->IsActive();
	}
	// Return false to signal that no effect group has been added
	if (!num_effects) return false;
	// Prepare group array
	if (property_groups.size() == group_index)
	{
		layout_valid = false;
		property_groups.resize(group_index + 1);
	}
	PropertyGroup &properties = property_groups[group_index];
	if (properties.props.size() != num_effects)
	{
		layout_valid = false;
		properties.props.resize(num_effects);
	}
	properties.name = LoadResStr("IDS_CNS_EFFECTS");
	// Add all (non-dead) effects of given object (or global effects if base_object is nullptr)
	int32_t num_added = 0;
	for (C4Effect *effect = *effect_list; effect; effect = effect->pNext)
	{
		if (effect->IsActive())
		{
			Property *prop = &properties.props[num_added++];
			prop->parent_value.SetPropList(base_object ? (C4PropList *) base_object : &::ScriptEngine);
			prop->property_path = C4PropertyPath(effect, base_object);
			prop->help_text = nullptr;
			prop->delegate_info.Set0();
			prop->group_idx = group_index;
			prop->key = ::Strings.RegString(prop->property_path.GetGetPath());
			prop->display_name = effect->GetPropertyStr(P_Name);
			prop->priority = 0;
			prop->delegate = delegate_factory->GetEffectDelegate();
			prop->shape = nullptr;
			prop->shape_delegate = nullptr;
			prop->shape_property_path.Clear();
			prop->about_to_edit = false;
			prop->group_idx = group_index;
		}
	}
	// Return true to signal that effect group has been added
	return true;
}

void C4ConsoleQtPropListModel::SetBasePropList(C4PropList *new_proplist)
{
	// Clear stack and select new proplist
	// Update properties
	target_value.SetPropList(new_proplist);
	base_proplist.SetPropList(new_proplist);
	// objects derive their custom properties
	info_proplist.SetPropList(target_value.getObj());
	target_path = C4PropertyPath(new_proplist);
	target_path_stack.clear();
	UpdateValue(true);
	delegate_factory->OnPropListChanged();
}

void C4ConsoleQtPropListModel::DescendPath(const C4Value &new_value, C4PropList *new_info_proplist, const C4PropertyPath &new_path)
{
	// Add previous proplist to stack
	target_path_stack.emplace_back(target_path, target_value, info_proplist);
	// descend
	target_value = new_value;
	info_proplist.SetPropList(new_info_proplist);
	target_path = new_path;
	UpdateValue(true);
	delegate_factory->OnPropListChanged();
}

void C4ConsoleQtPropListModel::AscendPath()
{
	// Go up in target stack (if possible)
	for (;;)
	{
		if (!target_path_stack.size())
		{
			SetBasePropList(nullptr);
			return;
		}
		TargetStackEntry entry = target_path_stack.back();
		target_path_stack.pop_back();
		if (!entry.value || !entry.info_proplist) continue; // property was removed; go up further in stack
		// Safety: Make sure we're still on the same value
		C4Value target = entry.path.ResolveValue();
		if (!target.IsIdenticalTo(entry.value)) continue;
		// Set new value
		target_path = entry.path;
		target_value = entry.value;
		info_proplist = entry.info_proplist;
		UpdateValue(true);
		break;
	}
	// Any current editor needs to close
	delegate_factory->OnPropListChanged();
}

void C4ConsoleQtPropListModel::UpdateValue(bool select_default)
{
	emit layoutAboutToBeChanged();
	// Update target value from path
	target_value = target_path.ResolveValue();
	// Prepare shape list update
	C4ConsoleQtShapeHolder::begin_visit();
	// Safe-get from C4Values in case any prop lists or arrays got deleted
	int32_t num_groups, default_selection_group = -1, default_selection_index = -1;
	switch (target_value.GetType())
	{
	case C4V_PropList:
		num_groups = UpdateValuePropList(target_value._getPropList(), &default_selection_group, &default_selection_index);
		break;
	case C4V_Array:
		num_groups = UpdateValueArray(target_value._getArray(), &default_selection_group, &default_selection_index);
		break;
	default: // probably nil
		num_groups = 0;
		break;
	}
	// Remove any unreferenced shapes
	for (auto iter = shapes.begin(); iter != shapes.end(); )
	{
		if (!iter->second.was_visited())
		{
			iter = shapes.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	// Update model range
	if (num_groups != property_groups.size())
	{
		layout_valid = false;
		property_groups.resize(num_groups);
	}
	if (!layout_valid)
	{
		// We do not adjust persistent indices for now
		// Usually, if layout changed, it's because the target value changed and we don't want to select/expand random stuff in the new proplist
		layout_valid = true;
	}
	emit layoutChanged();
	QModelIndex topLeft = index(0, 0, QModelIndex());
	QModelIndex bottomRight = index(rowCount() - 1, columnCount() - 1, QModelIndex());
	emit dataChanged(topLeft, bottomRight);
	// Initial selection
	if (select_default) emit ProplistChanged(default_selection_group, default_selection_index);
}

void C4ConsoleQtPropListModel::UpdateSelection(int32_t major_sel, int32_t minor_sel) const
{
	if (selection_model)
	{
		// Select by indexed elements only
		selection_model->clearSelection();
		if (major_sel >= 0)
		{
			QModelIndex sel = index(major_sel, 0, QModelIndex());
			if (minor_sel >= 0) sel = index(minor_sel, 0, sel);
			selection_model->select(sel, QItemSelectionModel::SelectCurrent);
		}
		else
		{
			selection_model->select(QModelIndex(), QItemSelectionModel::SelectCurrent);
		}
	}
}

int32_t C4ConsoleQtPropListModel::UpdateValuePropList(C4PropList *target_proplist, int32_t *default_selection_group, int32_t *default_selection_index)
{
	assert(target_proplist);
	C4Object *base_obj = this->base_proplist.getObj(), *obj = nullptr;
	C4PropList *info_proplist = this->info_proplist.getPropList();
	int32_t num_groups = 0;
	// Selected shape properties
	C4ConsoleQtShape *selected_shape = ::Console.EditCursor.GetShapes()->GetSelectedShape();
	if (selected_shape)
	{
		// Find property information for this shape
		// Could also remember this pointer for every shape holder
		// - but that would have to be updated on any property group vector resize
		Property *prop = nullptr;
		for (PropertyGroup &grp : property_groups)
		{
			for (Property &check_prop : grp.props)
			{
				if (check_prop.shape && check_prop.shape->Get() == selected_shape)
				{
					prop = &check_prop;
					break;
				}
			}
			if (prop) break;
		}
		// Update selected shape item information
		if (prop && prop->delegate)
		{
			C4PropList *shape_item_editorprops, *shape_item_value;
			C4String *shape_item_name = nullptr;
			C4PropertyPath shape_item_target_path;
			C4Value v;
			C4Value v_target_proplist = C4VPropList(target_proplist);
			prop->delegate->GetPropertyValue(v_target_proplist, prop->key, 0, &v);
			C4PropertyPath shape_property_path = prop->delegate->GetPathForProperty(prop);
			prop->delegate->GetShapeDelegate(v, &shape_property_path); // to resolve v
			if (::Console.EditCursor.GetShapes()->GetSelectedShapeData(v, prop->shape_property_path, &shape_item_editorprops, &shape_item_value, &shape_item_name, &shape_item_target_path))
			{
				if (AddPropertyGroup(shape_item_editorprops, num_groups, QString(shape_item_name ? shape_item_name->GetCStr() :"???"), shape_item_value, shape_item_target_path, obj, nullptr, nullptr))
				{
					++num_groups;
				}
			}
		}
	}
	// Published properties
	if (info_proplist)
	{
		C4String *default_selection = info_proplist->GetPropertyStr(P_DefaultEditorProp);
		obj = info_proplist->GetObject();
		// Properties from effects (no inheritance supported)
		if (obj)
		{
			for (C4Effect *fx = obj->pEffects; fx; fx = fx->pNext)
			{
				if (!fx->IsActive()) continue; // skip dead effects
				QString name = fx->GetName();
				C4PropList *effect_editorprops = fx->GetPropertyPropList(P_EditorProps);
				if (effect_editorprops && AddPropertyGroup(effect_editorprops, num_groups, name, fx, C4PropertyPath(fx, obj), obj, nullptr, nullptr))
					++num_groups;
			}
		}
		// Properties from object (but not on definition)
		if (obj || !info_proplist->GetDef())
		{
			C4PropList *info_editorprops = info_proplist->GetPropertyPropList(P_EditorProps);
			if (info_editorprops)
			{
				QString name = info_proplist->GetName();
				if (AddPropertyGroup(info_editorprops, num_groups, name, target_proplist, target_path, base_obj, default_selection, default_selection_index))
					++num_groups;
				// Assign group for default selection
				if (*default_selection_index >= 0)
				{
					*default_selection_group = num_groups - 1;
					default_selection = nullptr; // don't find any other instances
				}
			}
		}
		// properties from global list for objects
		if (obj)
		{
			C4Def *editor_base = C4Id2Def(C4ID::EditorBase);
			C4PropList *info_editorprops = nullptr;
			
			if (editor_base && (info_editorprops = editor_base->GetPropertyPropList(P_EditorProps)))
			{
				if (AddPropertyGroup(info_editorprops, num_groups, LoadResStr("IDS_CNS_OBJECT"), target_proplist, target_path, base_obj, nullptr, nullptr))
					++num_groups;
			}
		}
	}
	// Always: Internal properties
	auto new_properties = target_proplist->GetSortedLocalProperties();
	if (property_groups.size() == num_groups) property_groups.resize(num_groups + 1);
	PropertyGroup &internal_properties = property_groups[num_groups];
	internal_properties.name = LoadResStr("IDS_CNS_INTERNAL");
	internal_properties.props.resize(new_properties.size());
	for (int32_t i = 0; i < new_properties.size(); ++i)
	{
		internal_properties.props[i].parent_value = this->target_value;
		internal_properties.props[i].property_path = target_path;
		internal_properties.props[i].key = new_properties[i];
		internal_properties.props[i].display_name = new_properties[i];
		internal_properties.props[i].help_text = nullptr;
		internal_properties.props[i].priority = 0;
		internal_properties.props[i].delegate_info.Set0(); // default C4Value delegate
		internal_properties.props[i].delegate = nullptr; // init when needed
		internal_properties.props[i].group_idx = num_groups;
		internal_properties.props[i].shape = nullptr;
		internal_properties.props[i].shape_property_path.Clear();
		internal_properties.props[i].shape_delegate = nullptr;
		internal_properties.props[i].about_to_edit = false;
	}
	++num_groups;
	// Effects
	// Add after internal because the gorup may be added/removed quickly
	if (obj)
	{
		// Object: Show object effects
		if (AddEffectGroup(num_groups, obj))
		{
			++num_groups;
		}
	}
	else if (target_proplist == &::ScriptEngine)
	{
		// Global object: Show global effects
		if (AddEffectGroup(num_groups, nullptr))
		{
			++num_groups;
		}
	}
	return num_groups;
}

int32_t C4ConsoleQtPropListModel::UpdateValueArray(C4ValueArray *target_array, int32_t *default_selection_group, int32_t *default_selection_index)
{
	if (property_groups.empty())
	{
		layout_valid = false;
		property_groups.resize(1);
	}
	C4PropList *info_proplist = this->info_proplist.getPropList();
	C4Value elements_delegate_value;
	if (info_proplist) info_proplist->GetProperty(P_Elements, &elements_delegate_value);
	property_groups[0].name = (info_proplist ? info_proplist->GetName() : LoadResStr("IDS_CNS_ARRAYEDIT"));
	PropertyGroup &properties = property_groups[0];
	if (properties.props.size() != target_array->GetSize())
	{
		layout_valid = false;
		properties.props.resize(target_array->GetSize());
	}
	C4PropertyDelegate *item_delegate = delegate_factory->GetDelegateByValue(elements_delegate_value);
	for (int32_t i = 0; i < properties.props.size(); ++i)
	{
		Property &prop = properties.props[i];
		prop.property_path = C4PropertyPath(target_path, i);
		prop.parent_value = target_value;
		prop.display_name = ::Strings.RegString(FormatString("%d", (int)i).getData());
		prop.help_text = nullptr;
		prop.key = nullptr;
		prop.priority = 0;
		prop.delegate_info = elements_delegate_value;
		prop.delegate = item_delegate;
		prop.about_to_edit = false;
		prop.group_idx = 0;
		prop.shape = nullptr; // array elements cannot have shapes
		prop.shape_property_path.Clear();
		prop.shape_delegate = nullptr;
	}
	return 1; // one group for the values
}

void C4ConsoleQtPropListModel::DoOnUpdateCall(const C4PropertyPath &updated_path, const C4PropertyDelegate *delegate)
{
	// If delegate has its own update clalback, perform that on the root
	const char *update_callback = delegate->GetUpdateCallback();
	if (update_callback)
	{
		::Console.EditCursor.EMControl(CID_Script, new C4ControlScript(FormatString("%s->%s(%s)", updated_path.GetRoot(), update_callback, updated_path.GetGetPath()).getData(), 0, false));
	}
	// Do general object property update control
	C4PropList *base_proplist = this->base_proplist.getPropList();
	C4Value q;
	if (base_proplist && base_proplist->GetProperty(P_EditorPropertyChanged, &q))
	{
		::Console.EditCursor.EMControl(CID_Script, new C4ControlScript(FormatString(R"(%s->%s("%s"))", updated_path.GetRoot(), ::Strings.P[P_EditorPropertyChanged].GetCStr(), updated_path.GetGetPath()).getData(), 0, false));
	}
}

C4ConsoleQtPropListModel::Property *C4ConsoleQtPropListModel::GetPropByIndex(const QModelIndex &index) const
{
	if (!index.isValid()) return nullptr;
	// Resolve group and row
	int32_t group_index = index.internalId(), row = index.row();
	// Prop list access: Properties are on 2nd level
	if (!group_index) return nullptr;
	--group_index;
	if (group_index >= property_groups.size()) return nullptr;
	if (row < 0 || row >= property_groups[group_index].props.size()) return nullptr;
	return const_cast<Property *>(&property_groups[group_index].props[row]);
}

int C4ConsoleQtPropListModel::rowCount(const QModelIndex & parent) const
{
	QModelIndex grandparent;
	// Top level: Property groups
	if (!parent.isValid())
	{
		return property_groups.size();
	}
	// Mid level: Descend into property lists
	grandparent = parent.parent();
	if (!grandparent.isValid())
	{
		if (parent.row() >= 0 && parent.row() < property_groups.size())
			return property_groups[parent.row()].props.size();
	}
	return 0; // no 3rd level depth
}

int C4ConsoleQtPropListModel::columnCount(const QModelIndex & parent) const
{
	return 2; // Name + Data (or Index + Data)
}

QVariant C4ConsoleQtPropListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// Table headers
	if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal)
	{
		if (section == 0)
			if (target_value.GetType() == C4V_Array)
				return QVariant(LoadResStr("IDS_CNS_INDEXSHORT"));
			else
				return QVariant(LoadResStr("IDS_CTL_NAME"));
		if (section == 1) return QVariant(LoadResStr("IDS_CNS_VALUE"));
	}
	return QVariant();
}

QVariant C4ConsoleQtPropListModel::data(const QModelIndex & index, int role) const
{
	// Headers
	int32_t group_index = index.internalId();
	if (!group_index)
	{
		if (!index.column())
		{
			if (role == Qt::DisplayRole)
			{
				if (index.row() >= 0 && index.row() < property_groups.size())
					return property_groups[index.row()].name;
			}
			else if (role == Qt::FontRole)
			{
				return header_font;
			}
		}
		return QVariant();
	}
	// Query latest data from prop list
	Property *prop = GetPropByIndex(index);
	if (!prop) return QVariant();
	if (!prop->delegate) prop->delegate = delegate_factory->GetDelegateByValue(prop->delegate_info);
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case 0: // First col: Property Name
			return QVariant(prop->display_name->GetCStr());
		case 1: // Second col: Property value
		{
			C4Value v;
			prop->delegate->GetPropertyValue(prop->parent_value, prop->key, index.row(), &v);
			return QVariant(prop->delegate->GetDisplayString(v, target_value.getObj(), true));
		}
		}
	}
	else if (role == Qt::BackgroundColorRole && index.column()==1)
	{
		C4Value v;
		prop->delegate->GetPropertyValue(prop->parent_value, prop->key, index.row(), &v);
		QColor bgclr = prop->delegate->GetDisplayBackgroundColor(v, target_value.getObj());
		if (bgclr.isValid()) return bgclr;
	}
	else if (role == Qt::TextColorRole && index.column() == 1)
	{
		C4Value v;
		prop->delegate->GetPropertyValue(prop->parent_value, prop->key, index.row(), &v);
		QColor txtclr = prop->delegate->GetDisplayTextColor(v, target_value.getObj());
		if (txtclr.isValid()) return txtclr;
	}
	else if (role == Qt::DecorationRole && index.column() == 0 && prop->help_text && Config.Developer.ShowHelp)
	{
		// Help icons in left column
		return QIcon(":/editor/res/Help.png");
	}
	else if (role == Qt::FontRole && index.column() == 0)
	{
		if (prop->priority >= 100) return important_property_font;
	}
	else if (role == Qt::ToolTipRole && index.column() == 0)
	{
		// Tooltip from property description. Default to display name in case it got truncated.
		if (prop->help_text)
			return QString(prop->help_text->GetCStr());
		else
			return QString(prop->display_name->GetCStr());
	}
	// Nothing to show
	return QVariant();
}

QModelIndex C4ConsoleQtPropListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (column < 0 || column > 1) return QModelIndex();
	// Top level index?
	if (!parent.isValid())
	{
		// Top level has headers only
		if (row < 0 || row >= property_groups.size()) return QModelIndex();
		return createIndex(row, column, (quintptr)0u);
	}
	if (parent.internalId()) return QModelIndex(); // No 3rd level depth
	// Validate range of property
	const PropertyGroup *property_group = nullptr;
	if (parent.row() >= 0 && parent.row() < property_groups.size())
	{
		property_group = &property_groups[parent.row()];
		if (row < 0 || row >= property_group->props.size()) return QModelIndex();
		return createIndex(row, column, (quintptr)parent.row()+1);
	}
	return QModelIndex();
}

QModelIndex C4ConsoleQtPropListModel::parent(const QModelIndex &index) const
{
	// Parent: Stored in internal ID
	auto parent_idx = index.internalId();
	if (parent_idx) return createIndex(parent_idx - 1, 0, (quintptr)0u);
	return QModelIndex();
}

Qt::ItemFlags C4ConsoleQtPropListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
	Property *prop = GetPropByIndex(index);
	if (index.isValid() && prop)
	{
		flags &= ~Qt::ItemIsDropEnabled; // only drop between the lines
		if (index.column() == 0)
		{
			// array elements can be re-arranged
			if (prop->parent_value.GetType() == C4V_Array) flags |= Qt::ItemIsDragEnabled;
		}
		else if (index.column() == 1)
		{
			// Disallow editing on readonly target (e.g. frozen proplist).
			// But always allow editing of effects.
			bool readonly = IsTargetReadonly() && prop->delegate != delegate_factory->GetEffectDelegate();
			if (!readonly)
				flags |= Qt::ItemIsEditable;
			else
				flags &= ~Qt::ItemIsEnabled;
		}
	}
	return flags;
}

Qt::DropActions C4ConsoleQtPropListModel::supportedDropActions() const
{
	return Qt::MoveAction;
}

bool C4ConsoleQtPropListModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	// Drag+Drop movement on array only
	if (action != Qt::MoveAction) return false;
	C4ValueArray *arr = target_value.getArray();
	if (!arr) return false;
	if (!data->hasFormat("application/vnd.text")) return false;
	if (row < 0) return false; // outside range: Could be above or below. Better don't drag at all.
	if (!parent.isValid()) return false; // in array only
	// Decode indices of rows to move
	QByteArray encodedData = data->data("application/vnd.text");
	StdStrBuf rearrange_call;
	rearrange_call.Format("MoveArrayItems(%%s, [%s], %d)", encodedData.constData(), row);
	target_path.DoCall(rearrange_call.getData());
	return true;
}

QStringList C4ConsoleQtPropListModel::mimeTypes() const
{
	QStringList types;
	types << "application/vnd.text";
	return types;
}

QMimeData *C4ConsoleQtPropListModel::mimeData(const QModelIndexList &indexes) const
{
	// Add all moved indexes
	QMimeData *mimeData = new QMimeData();
	QByteArray encodedData;
	int32_t count = 0;
	for (const QModelIndex &index : indexes)
	{
		if (index.isValid() && index.internalId())
		{
			if (count) encodedData.append(",");
			encodedData.append(QString::number(index.row()));
			++count;
		}
	}
	mimeData->setData("application/vnd.text", encodedData);
	return mimeData;
}

QString C4ConsoleQtPropListModel::GetTargetPathHelp() const
{
	// Help text in EditorInfo prop. Fall back to description.
	C4PropList *info_proplist = this->info_proplist.getPropList();
	if (!info_proplist) return QString();
	C4String *desc = info_proplist->GetPropertyStr(P_EditorHelp);
	if (!desc) desc = info_proplist->GetPropertyStr(P_Description);
	if (!desc) return QString();
	QString result = QString(desc->GetCStr());
	result = result.replace('|', '\n');
	return result;
}

const char *C4ConsoleQtPropListModel::GetTargetPathName() const
{
	// Name prop of current info.
	C4PropList *info_proplist = this->info_proplist.getPropList();
	if (!info_proplist) return nullptr;
	C4String *name = info_proplist->GetPropertyStr(P_Name);
	return name ? name->GetCStr() : nullptr;
}

void C4ConsoleQtPropListModel::AddArrayElement()
{
	C4Value new_val;
	C4PropList *info_proplist = this->info_proplist.getPropList();
	C4PropListStatic *info_proplist_static = nullptr;
	if (info_proplist)
	{
		info_proplist->GetProperty(P_DefaultValue, &new_val);
		info_proplist_static = info_proplist->IsStatic();
	}
	target_path.DoCall(FormatString("PushBack(%%s, %s)", new_val.GetDataString(10, info_proplist_static).getData()).getData());
}

void C4ConsoleQtPropListModel::RemoveArrayElement()
{
	// Compose script command to remove all selected array indices
	StdStrBuf script;
	for (QModelIndex idx : selection_model->selectedIndexes())
		if (idx.isValid() && idx.column() == 0)
			if (script.getLength())
				script.AppendFormat(",%d", idx.row());
			else
				script.AppendFormat("%d", idx.row());
	if (script.getLength()) target_path.DoCall(FormatString("RemoveArrayIndices(%%s, [%s])", script.getData()).getData());
}

bool C4ConsoleQtPropListModel::IsTargetReadonly() const
{
	if (target_path.IsEmpty()) return true;
	switch (target_value.GetType())
	{
	case C4V_Array:
		// Arrays are never frozen
		return false;
	case C4V_PropList:
	{
		C4PropList *parent_proplist = target_value._getPropList();
		if (parent_proplist->IsFrozen()) return true;
		return false;
	}
	default:
		return true;
	}
}

class C4ConsoleQtShape *C4ConsoleQtPropListModel::GetShapeByPropertyPath(const char *property_path)
{
	// Lookup in map
	auto entry = shapes.find(std::string(property_path));
	if (entry == shapes.end()) return nullptr;
	return entry->second.Get();
}
