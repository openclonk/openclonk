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
#include "editor/C4ConsoleQtState.h"
#include "editor/C4Console.h"
#include "object/C4Object.h"
#include "object/C4DefList.h"
#include "object/C4Def.h"
#include "script/C4Effect.h"

/* Property path for property setting synchronization */

C4PropertyPath::C4PropertyPath(const C4PropertyPath &parent, int32_t elem_index)
{
	path.Format("%s[%d]", parent.GetPath(), (int)elem_index);
	path_type = PPT_Index;
}

C4PropertyPath::C4PropertyPath(const C4PropertyPath &parent, const char *child_property, C4PropertyPath::PathType path_type)
	: path_type(path_type)
{
	if (path_type == PPT_Property)
		path.Format("%s.%s", parent.GetPath(), child_property);
	else if (path_type == PPT_SetFunction)
		path.Format("%s->%s", parent.GetPath(), child_property);
	else
	{
		assert(false);
	}
}

void C4PropertyPath::SetProperty(const char *set_string) const
{
	// Compose script to update property
	StdStrBuf script;
	if (path_type != PPT_SetFunction)
		script.Format("%s=%s", path.getData(), set_string);
	else
		script.Format("%s(%s)", path.getData(), set_string);
	// Execute synced scripted
	// TODO: Use silent editor control later; for now it's good to have the output shown
	::Console.In(script.getData());
}

void C4PropertyPath::SetProperty(const C4Value &to_val) const
{
	SetProperty(to_val.GetDataString(9999999).getData());
}


/* Property editing */

C4PropertyDelegate::C4PropertyDelegate(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: QObject(), factory(factory)
{
	// Resolve getter+setter callback names
	if (props)
	{
		set_function = props->GetPropertyStr(P_Set);
		async_get_function = props->GetPropertyStr(P_AsyncGet);
	}
}

void C4PropertyDelegate::UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const
{
	editor->setGeometry(option.rect);
}

bool C4PropertyDelegate::GetPropertyValue(C4PropList *props, C4String *key, C4Value *out_val) const
{
	if (async_get_function)
	{
		*out_val = props->Call(async_get_function.Get());
		return true;
	}
	else
	{
		return props->GetPropertyByS(key, out_val);
	}
}

QString C4PropertyDelegate::GetDisplayString(const C4Value &v, C4Object *obj) const
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

C4PropertyDelegateInt::C4PropertyDelegateInt(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props), min(std::numeric_limits<int32_t>::min()), max(std::numeric_limits<int32_t>::max()), step(1)
{
	// TODO min/max/step
	if (props)
	{
		min = props->GetPropertyInt(P_Min, min);
		max = props->GetPropertyInt(P_Max, max);
		step = props->GetPropertyInt(P_Step, step);
	}
}

void C4PropertyDelegateInt::SetEditorData(QWidget *editor, const C4Value &val) const
{
	QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
	spinBox->setValue(val.getInt());
}

void C4PropertyDelegateInt::SetModelData(QObject *editor, const C4PropertyPath &property_path) const
{
	QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
	spinBox->interpretText();
	property_path.SetProperty(C4VInt(spinBox->value()));
}

QWidget *C4PropertyDelegateInt::CreateEditor(const C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const
{
	QSpinBox *editor = new QSpinBox(parent);
	editor->setMinimum(min);
	editor->setMaximum(max);
	editor->setSingleStep(step);
	connect(editor, &QSpinBox::editingFinished, this, [editor, this]() {
		emit EditingDoneSignal(editor);
	});
	return editor;
}

C4PropertyDelegateLabelAndButtonWidget::C4PropertyDelegateLabelAndButtonWidget(QWidget *parent)
	: QWidget(parent), layout(nullptr), label(nullptr), button(nullptr)
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMargin(0);
	layout->setSpacing(0);
	label = new QLabel(this);
	layout->addWidget(label);
	button = new QPushButton(QString(LoadResStr("IDS_CNS_MORE")), this);
	layout->addWidget(button);
}

C4PropertyDelegateColor::C4PropertyDelegateColor(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props)
{
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

void C4PropertyDelegateColor::SetEditorData(QWidget *aeditor, const C4Value &val) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	uint32_t background_color = static_cast<uint32_t>(val.getInt()) & 0xffffff;
	uint32_t foreground_color = GetTextColorForBackground(background_color);
	QPalette palette = editor->label->palette();
	palette.setColor(editor->label->backgroundRole(), QColor(QRgb(background_color)));
	palette.setColor(editor->label->foregroundRole(), QColor(QRgb(foreground_color)));
	editor->label->setPalette(palette);
	editor->label->setAutoFillBackground(true);
	editor->label->setText(GetDisplayString(val, NULL));
	editor->last_value = val;
}

void C4PropertyDelegateColor::SetModelData(QObject *aeditor, const C4PropertyPath &property_path) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	property_path.SetProperty(editor->last_value);
}

QWidget *C4PropertyDelegateColor::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const
{
	Editor *editor;
	std::unique_ptr<Editor> peditor((editor = new Editor(parent)));
	connect(editor->button, &QPushButton::pressed, this, [editor, this]() {
		QColor clr = QColorDialog::getColor(QColor(editor->last_value.getInt()), editor, QString(), QColorDialog::ShowAlphaChannel);
		editor->last_value.SetInt(clr.rgba());
		this->SetEditorData(editor, editor->last_value); // force update on display
		emit EditingDoneSignal(editor);
	});
	return peditor.release();
}

QString C4PropertyDelegateColor::GetDisplayString(const C4Value &v, C4Object *obj) const
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

C4PropertyDelegateEnum::C4PropertyDelegateEnum(const C4PropertyDelegateFactory *factory, C4PropList *props, const C4ValueArray *poptions)
	: C4PropertyDelegate(factory, props)
{
	// Build enum options from C4Value definitions in script
	if (!poptions && props) poptions = props->GetPropertyArray(P_Options);
	if (poptions)
	{
		options.reserve(poptions->GetSize());
		for (int32_t i = 0; i < poptions->GetSize(); ++i)
		{
			const C4Value &v = poptions->GetItem(i);
			C4PropList *props = v.getPropList();
			if (!props) continue;
			Option option;
			option.name = props->GetPropertyStr(P_Name);
			if (!option.name.Get()) option.name = ::Strings.RegString("???");
			option.value_key = props->GetPropertyStr(P_ValueKey);
			props->GetProperty(P_Value, &option.value);
			option.type = C4V_Type(props->GetPropertyInt(P_Type, C4V_Any));
			option.option_key = props->GetPropertyStr(P_OptionKey);
			// Derive storage type from given elements in delegate definition
			if (option.type != C4V_Any)
				option.storage_type = Option::StorageByType;
			else if (option.option_key.Get())
				option.storage_type = Option::StorageByKey;
			else
				option.storage_type = Option::StorageByValue;
			// Child delegate for value (resolved at runtime because there may be circular references)
			props->GetProperty(P_Delegate, &option.adelegate_val);
			options.push_back(option);
		}
	}
}

void C4PropertyDelegateEnum::ReserveOptions(int32_t num)
{
	options.reserve(num);
}

void C4PropertyDelegateEnum::AddTypeOption(C4String *name, C4V_Type type, const C4Value &val, C4PropertyDelegate *adelegate)
{
	Option option;
	option.name = name;
	option.type = type;
	option.value = val;
	option.storage_type = Option::StorageByType;
	option.adelegate = adelegate;
	options.push_back(option);
}

void C4PropertyDelegateEnum::AddConstOption(C4String *name, const C4Value &val)
{
	Option option;
	option.name = name;
	option.value = val;
	option.storage_type = Option::StorageByValue;
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
			if (props)
			{
				C4Value propval;
				props->GetPropertyByS(option.option_key.Get(), &propval);
				match = (val == propval);
			}
			break;
		}
		default: break;
		}
		if (match) break;
		++iopt;
	}
	// If no option matches, just pick first
	return match ? iopt : -1;
}

void C4PropertyDelegateEnum::UpdateEditorParameter(C4PropertyDelegateEnum::Editor *editor) const
{
	// Recreate parameter settings editor associated with the currently selected option of an enum
	if (editor->parameter_widget)
	{
		editor->parameter_widget->deleteLater();
		editor->parameter_widget = NULL;
	}
	int32_t idx = editor->option_box->currentIndex();
	if (idx < 0 || idx >= options.size()) return;
	const Option &option = options[idx];
	// Lazy-resolve parameter delegate
	EnsureOptionDelegateResolved(option);
	// Create editor if needed
	if (option.adelegate)
	{
		// Determine value to be shown in editor
		C4Value parameter_val = editor->last_val;
		if (option.value_key.Get())
		{
			C4PropList *props = editor->last_val.getPropList();
			if (props) props->GetPropertyByS(option.value_key.Get(), &parameter_val);
		}
		// Show it
		editor->parameter_widget = option.adelegate->CreateEditor(factory, editor, QStyleOptionViewItem());
		if (editor->parameter_widget)
		{
			editor->layout->addWidget(editor->parameter_widget);
			option.adelegate->SetEditorData(editor->parameter_widget, parameter_val);
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
	}
}

void C4PropertyDelegateEnum::SetEditorData(QWidget *aeditor, const C4Value &val) const
{
	Editor *editor = static_cast<Editor*>(aeditor);
	editor->last_val = val;
	editor->updating = true;
	// Update option selection
	int32_t index = std::max<int32_t>(GetOptionByValue(val), 0);
	editor->option_box->setCurrentIndex(index);
	// Update parameter
	UpdateEditorParameter(editor);
	editor->updating = false;
}

void C4PropertyDelegateEnum::SetModelData(QObject *aeditor, const C4PropertyPath &property_path) const
{
	// Fetch value from editor
	Editor *editor = static_cast<Editor*>(aeditor);
	int32_t idx = editor->option_box->currentIndex();
	if (idx < 0 || idx >= options.size()) return;
	const Option &option = options[idx];
	// Store directly in value or in a proplist field?
	C4PropertyPath use_path;
	if (option.value_key.Get())
		use_path = C4PropertyPath(property_path, option.value_key->GetCStr());
	else
		use_path = property_path;
	// Value from a parameter or directly from the enum?
	if (option.adelegate)
	{
		// Value from a parameter.
		// Using a setter function?
		if (option.adelegate->GetSetFunction())
			use_path = C4PropertyPath(use_path, option.adelegate->GetSetFunction(), C4PropertyPath::PPT_SetFunction);
		option.adelegate->SetModelData(editor->parameter_widget, use_path);
	}
	else
	{
		// No parameter. Use value.
		use_path.SetProperty(option.value);
	}
}

QWidget *C4PropertyDelegateEnum::CreateEditor(const C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const
{
	Editor *editor = new Editor(parent);
	editor->layout = new QHBoxLayout(editor);
	editor->layout->setContentsMargins(0, 0, 0, 0);
	editor->layout->setMargin(0);
	editor->layout->setSpacing(0);
	editor->updating = true;
	editor->option_box = new QComboBox(editor);
	editor->layout->addWidget(editor->option_box);
	for (auto &option : options) editor->option_box->addItem(option.name->GetCStr());
	void (QComboBox::*currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
	connect(editor->option_box, currentIndexChanged, editor, [editor, this](int newval) {
		if (!editor->updating) this->UpdateOptionIndex(editor, newval); });
	editor->updating = false;
	return editor;
}

void C4PropertyDelegateEnum::UpdateOptionIndex(C4PropertyDelegateEnum::Editor *editor, int newval) const
{
	UpdateEditorParameter(editor);
	emit EditorValueChangedSignal(editor);
}

void C4PropertyDelegateEnum::EnsureOptionDelegateResolved(const Option &option) const
{
	// Lazy-resolve parameter delegate
	if (!option.adelegate && option.adelegate_val.GetType() != C4V_Nil)
		option.adelegate = factory->GetDelegateByValue(option.adelegate_val);
}

QString C4PropertyDelegateEnum::GetDisplayString(const C4Value &v, class C4Object *obj) const
{
	// Display string from value
	int32_t idx = GetOptionByValue(v);
	if (idx < 0)
	{
		// Value not found: Default display
		return C4PropertyDelegate::GetDisplayString(v, obj);
	}
	else
	{
		// Value found: Display option string plus parameter
		const Option &option = options[idx];
		QString result = option.name->GetCStr();
		// Lazy-resolve parameter delegate
		EnsureOptionDelegateResolved(option);
		if (option.adelegate)
		{
			C4Value param_val = v;
			if (option.value_key.Get())
			{
				C4PropList *vp = v.getPropList();
				if (vp) vp->GetPropertyByS(option.value_key, &param_val);
			}
			result += " ";
			result += option.adelegate->GetDisplayString(param_val, obj);
		}
		return result;
	}
}

const C4PropertyDelegateShape *C4PropertyDelegateEnum::GetShapeDelegate(const C4Value &val) const
{
	// Does this delegate own a shape? Forward decision into selected option.
	int32_t option_idx = GetOptionByValue(val);
	if (option_idx < 0) return nullptr;
	const Option &option = options[option_idx];
	EnsureOptionDelegateResolved(option);
	if (!option.adelegate) return nullptr;
	C4Value param_val = val;
	if (option.value_key.Get())
	{
		C4PropList *vp = val.getPropList();
		if (vp) vp->GetPropertyByS(option.value_key, &param_val);
	}
	return option.adelegate->GetShapeDelegate(param_val);
}

C4PropertyDelegateDef::C4PropertyDelegateDef(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props)
{
	// Collect sorted definitions
	std::vector<C4Def *> defs = ::Definitions.GetAllDefs(props ? props->GetPropertyStr(P_Filter) : NULL);
	std::sort(defs.begin(), defs.end(), [](C4Def *a, C4Def *b) -> bool {
		return strcmp(a->GetName(), b->GetName()) < 0;
	});
	// Add them
	ReserveOptions(defs.size() + 1);
	AddConstOption(::Strings.RegString("nil"), C4VNull); // nil is always an option
	for (C4Def *def : defs)
	{
		C4RefCntPointer<C4String> option_name = ::Strings.RegString(FormatString("%s (%s)", def->id.ToString(), def->GetName()));
		AddConstOption(option_name, C4Value(def));
	}
}

C4PropertyDelegateBool::C4PropertyDelegateBool(const C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateEnum(factory, props)
{
	// Add boolean options
	ReserveOptions(2);
	AddConstOption(::Strings.RegString(LoadResStr("IDS_CNS_FALSE")), C4VBool(false));
	AddConstOption(::Strings.RegString(LoadResStr("IDS_CNS_TRUE")), C4VBool(true));
}

bool C4PropertyDelegateBool::GetPropertyValue(C4PropList *props, C4String *key, C4Value *out_val) const
{
	// Force value to bool
	props->GetPropertyByS(key, out_val);
	if (out_val->GetType() != C4V_Bool) *out_val = C4VBool(!!*out_val);
	return true;
}

C4PropertyDelegateHasEffect::C4PropertyDelegateHasEffect(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegateBool(factory, props)
{
	if (props) effect = props->GetPropertyStr(P_Effect);
}

bool C4PropertyDelegateHasEffect::GetPropertyValue(C4PropList *props, C4String *key, C4Value *out_val) const
{
	const C4Object *obj = props->GetObject();
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

void C4PropertyDelegateC4ValueInput::SetEditorData(QWidget *aeditor, const C4Value &val) const
{
	Editor *editor = static_cast<Editor *>(aeditor);
	editor->edit->setText(val.GetDataString().getData());
}

void C4PropertyDelegateC4ValueInput::SetModelData(QObject *aeditor, const C4PropertyPath &property_path) const
{
	// Only set model data when pressing Enter explicitely; not just when leaving 
	Editor *editor = static_cast<Editor *>(aeditor);
	if (editor->commit_pending)
	{
		property_path.SetProperty(editor->edit->text().toUtf8());
		editor->commit_pending = false;
	}
}

QWidget *C4PropertyDelegateC4ValueInput::CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const
{
	// Editor is just an edit box plus a "..." button for array/proplist types
	Editor *editor = new Editor(parent);
	editor->layout = new QHBoxLayout(editor);
	editor->layout->setContentsMargins(0, 0, 0, 0);
	editor->layout->setMargin(0);
	editor->layout->setSpacing(0);
	editor->edit = new QLineEdit(editor);
	editor->layout->addWidget(editor->edit);
	editor->extended_button = new QPushButton("...", editor); // TODO imnplement extended button
	editor->layout->addWidget(editor->extended_button);
	editor->extended_button->hide();
	editor->edit->setFocus();
	// EditingDone only on Return; not just when leaving edit field
	connect(editor->edit, &QLineEdit::returnPressed, editor, [this, editor]() {
		editor->commit_pending = true;
		emit EditingDoneSignal(editor);
	});
	return editor;
}


/* Areas shown in viewport */

C4PropertyDelegateShape::C4PropertyDelegateShape(const class C4PropertyDelegateFactory *factory, C4PropList *props)
	: C4PropertyDelegate(factory, props), clr(0xffff0000), can_move_center(false)
{
	if (props)
	{
		shape_type = props->GetPropertyStr(P_Type);
		clr = props->GetPropertyInt(P_Color) | 0xff000000;
		can_move_center = props->GetPropertyBool(P_CanMoveCenter);
	}
}

void C4PropertyDelegateShape::SetModelData(QObject *editor, const C4PropertyPath &property_path) const
{
	C4ConsoleQtShape *shape = static_cast<C4ConsoleQtShape *>(editor);
	property_path.SetProperty(shape->GetValue());
}

void C4PropertyDelegateShape::Paint(QPainter *painter, const QStyleOptionViewItem &option, const C4Value &val) const
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
	if (shape_type && shape_type->GetData() == "circle")
	{
		painter->drawEllipse(inner_rect);
		if (can_move_center) painter->drawPoint(inner_rect.center());
	}
	else
	{
		painter->drawRect(inner_rect);
	}
	painter->restore();
}


/* Delegate factory: Create delegates based on the C4Value type */

C4PropertyDelegate *C4PropertyDelegateFactory::CreateDelegateByString(const C4String *str, C4PropList *props) const
{
	// safety
	if (!str) return NULL;
	// create default base types
	if (str->GetData() == "int") return new C4PropertyDelegateInt(this, props);
	if (str->GetData() == "color") return new C4PropertyDelegateColor(this, props);
	if (str->GetData() == "def") return new C4PropertyDelegateDef(this, props);
	if (str->GetData() == "enum") return new C4PropertyDelegateEnum(this, props);
	if (str->GetData() == "bool") return new C4PropertyDelegateBool(this, props);
	if (str->GetData() == "has_effect") return new C4PropertyDelegateHasEffect(this, props);
	if (str->GetData() == "c4valueenum") return new C4PropertyDelegateC4ValueEnum(this, props);
	if (str->GetData() == "rect" || str->GetData() == "circle") return new C4PropertyDelegateShape(this, props);
	if (str->GetData() == "any") return new C4PropertyDelegateC4ValueInput(this, props);
	// unknown type
	return NULL;
}

C4PropertyDelegate *C4PropertyDelegateFactory::CreateDelegateByValue(const C4Value &val) const
{
	switch (val.GetType())
	{
	case C4V_Nil:
		return new C4PropertyDelegateC4ValueInput(this, NULL);
	case C4V_Array:
		return new C4PropertyDelegateEnum(this, NULL, val.getArray());
	case C4V_PropList:
	{
		C4PropList *props = val._getPropList();
		if (!props) break;
		return CreateDelegateByString(props->GetPropertyStr(P_Type), props);
	}
	case C4V_String:
		return CreateDelegateByString(val._getStr(), NULL);
	default:
		// Invalid delegte: No editor.
		break;
	}
	return NULL;
}

C4PropertyDelegate *C4PropertyDelegateFactory::GetDelegateByValue(const C4Value &val) const
{
	auto iter = delegates.find(val);
	if (iter != delegates.end()) return iter->second.get();
	C4PropertyDelegate *new_delegate = CreateDelegateByValue(val);
	delegates.insert(std::make_pair(val, std::unique_ptr<C4PropertyDelegate>(new_delegate)));
	return new_delegate;
}

C4PropertyDelegate *C4PropertyDelegateFactory::GetDelegateByIndex(const QModelIndex &index) const
{
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	if (!prop) return NULL;
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
	if (!d) return;
	// Fetch property only first time - ignore further updates to simplify editing
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	if (!prop || !prop->about_to_edit) return;
	prop->about_to_edit = false;
	C4Value val;
	C4PropList *props = prop->parent_proplist.getPropList();
	if (props)
	{
		d->GetPropertyValue(props, prop->key, &val);
		d->SetEditorData(editor, val);
	}
}

void C4PropertyDelegateFactory::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	// Fetch property value from editor and set it into proplist
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!d) return;
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	SetPropertyData(d, editor, prop);
}

void C4PropertyDelegateFactory::SetPropertyData(const C4PropertyDelegate *d, QObject *editor, C4ConsoleQtPropListModel::Property *editor_prop) const
{
	// Safety: Ensure target properties still exist
	C4PropList *target_props = editor_prop->parent_proplist.getPropList();
	if (!target_props) return;
	// Compose set command
	C4PropertyPath path(editor_prop->parent_proplist.GetDataString().getData());
	C4PropertyPath subpath;
	if (d->GetSetFunction())
		subpath = C4PropertyPath(path, d->GetSetFunction(), C4PropertyPath::PPT_SetFunction);
	else
		subpath = C4PropertyPath(path, editor_prop->key->GetCStr());
	// Set according to delegate
	d->SetModelData(editor, subpath);
}

QWidget *C4PropertyDelegateFactory::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!d) return NULL;
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	prop->about_to_edit = true;
	QWidget *editor = d->CreateEditor(this, parent, option);
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
	return editor;
}

void C4PropertyDelegateFactory::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!d) return;
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
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (d && prop && d->HasCustomPaint())
	{
		C4Value val;
		C4PropList *props = prop->parent_proplist.getPropList();
		if (props)
		{
			d->GetPropertyValue(props, prop->key, &val);
			d->Paint(painter, option, val);
			return;
		}
	}
	// Otherwise use default paint implementation
	QStyledItemDelegate::paint(painter, option, index);
}


/* Proplist table view */

C4ConsoleQtPropListModel::C4ConsoleQtPropListModel(C4PropertyDelegateFactory *delegate_factory)
	: delegate_factory(delegate_factory)
{
	header_font.setBold(true);
}

C4ConsoleQtPropListModel::~C4ConsoleQtPropListModel()
{
}

bool C4ConsoleQtPropListModel::AddPropertyGroup(C4PropList *add_proplist, int32_t group_index, QString name, C4PropList *target_proplist)
{
	const char *editor_prop_prefix = "EditorProp_";
	auto new_properties = add_proplist->GetSortedLocalProperties(editor_prop_prefix, target_proplist);
	if (!new_properties.size()) return false;
	if (property_groups.size() == group_index) property_groups.resize(group_index + 1);
	PropertyGroup &properties = property_groups[group_index];
	C4PropListStatic *proplist_static = add_proplist->IsStatic();
	properties.name = name;
	properties.props.resize(new_properties.size());
	for (int32_t i = 0; i < new_properties.size(); ++i)
	{
		Property *prop = &properties.props[i];
		prop->parent_proplist.SetPropList(target_proplist);
		prop->key = NULL;
		prop->display_name = NULL;
		prop->delegate_info.Set0(); // default C4Value delegate
		prop->group_idx = group_index;
		C4Value published_prop_val;
		add_proplist->GetPropertyByS(new_properties[i], &published_prop_val);
		C4PropList *published_prop = published_prop_val.getPropList();
		if (published_prop)
		{
			prop->key = published_prop->GetPropertyStr(P_Key);
			prop->display_name = published_prop->GetPropertyStr(P_Name);
			prop->delegate_info.SetPropList(published_prop);
		}
		if (!prop->key) properties.props[i].key = ::Strings.RegString(new_properties[i]->GetCStr() + strlen(editor_prop_prefix));
		if (!prop->display_name) properties.props[i].display_name = ::Strings.RegString(new_properties[i]->GetCStr() + strlen(editor_prop_prefix));
		prop->delegate = delegate_factory->GetDelegateByValue(prop->delegate_info);
		C4Value v;
		prop->delegate->GetPropertyValue(target_proplist, prop->key, &v);
		// Connect editable shape to property
		const C4PropertyDelegateShape *new_shape_delegate = prop->delegate->GetShapeDelegate(v);
		if (new_shape_delegate != prop->shape_delegate)
		{
			prop->shape_delegate = new_shape_delegate;
			if (new_shape_delegate)
			{
				C4ConsoleQtShape *shape = ::Console.EditCursor.GetShapes()->CreateShape(target_proplist->GetObject(), published_prop, v);
				C4PropertyDelegateFactory *factory = this->delegate_factory;
				connect(shape, &C4ConsoleQtShape::ShapeDragged, new_shape_delegate, [factory, new_shape_delegate, shape, prop]() {
					factory->SetPropertyData(new_shape_delegate, shape, prop);
				});
				prop->shape.Set(shape);
			}
		}
	}
	return true;
}

void C4ConsoleQtPropListModel::SetPropList(class C4PropList *new_proplist)
{
	// Update properties
	proplist.SetPropList(new_proplist);
	// Determine number of property groups
	int32_t num_groups = 0;
	if (new_proplist)
	{
		// Objects only: Published properties
		C4Object *obj = new_proplist->GetObject();
		if (obj)
		{
			// Properties from effects (no inheritance supported)
			for (C4Effect *fx = obj->pEffects; fx; fx = fx->pNext)
			{
				QString name = fx->GetName();
				if (AddPropertyGroup(fx, num_groups, name, fx))
					++num_groups;
			}
			// Properties from object
			for (C4PropList *check_proplist = new_proplist; check_proplist; check_proplist = check_proplist->GetPrototype())
			{
				QString name;
				C4PropListStatic *proplist_static = check_proplist->IsStatic();
				if (proplist_static)
					name = QString(proplist_static->GetDataString().getData());
				else
					name = check_proplist->GetName();
				if (AddPropertyGroup(check_proplist, num_groups, name, new_proplist))
					++num_groups;
			}
			// properties from global list
			C4Def *editor_base = C4Id2Def(C4ID::EditorBase);
			if (editor_base)
				if (AddPropertyGroup(editor_base, num_groups, LoadResStr("IDS_CNS_OBJECT"), new_proplist))
					++num_groups;
		}
		// Always: Internal properties
		auto new_properties = new_proplist->GetSortedLocalProperties();
		if (property_groups.size() == num_groups) property_groups.resize(num_groups + 1);
		PropertyGroup &internal_properties = property_groups[num_groups];
		internal_properties.name = LoadResStr("IDS_CNS_INTERNAL");
		internal_properties.props.resize(new_properties.size());
		for (int32_t i = 0; i < new_properties.size(); ++i)
		{
			internal_properties.props[i].parent_proplist = proplist;
			internal_properties.props[i].key = new_properties[i];
			internal_properties.props[i].display_name = new_properties[i];
			internal_properties.props[i].delegate_info.Set0(); // default C4Value delegate
			internal_properties.props[i].delegate = NULL; // init when needed
			internal_properties.props[i].group_idx = num_groups;
			internal_properties.props[i].shape.Clear();
			internal_properties.props[i].shape_delegate = nullptr;
		}
		++num_groups;
	}
	property_groups.resize(num_groups);
	QModelIndex topLeft = index(0, 0, QModelIndex());
	QModelIndex bottomRight = index(rowCount() - 1, columnCount() - 1, QModelIndex());
	emit dataChanged(topLeft, bottomRight);
	emit layoutChanged();
}

int C4ConsoleQtPropListModel::rowCount(const QModelIndex & parent) const
{
	// Nothing loaded?
	if (!proplist.getPropList()) return 0;
	// Top level: Property groups
	if (!parent.isValid()) return property_groups.size();
	// Mid level: Descend into property lists
	QModelIndex grandparent = parent.parent();
	if (!grandparent.isValid())
	{
		if (parent.row() >= 0 && parent.row() < property_groups.size())
			return property_groups[parent.row()].props.size();
	}
	return 0;
}

int C4ConsoleQtPropListModel::columnCount(const QModelIndex & parent) const
{
	return 2; // Name + Data
}

QVariant C4ConsoleQtPropListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// Table headers
	if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal)
	{
		if (section == 0) return QVariant(LoadResStr("IDS_CTL_NAME"));
		if (section == 1) return QVariant(LoadResStr("IDS_CNS_VALUE"));
	}
	return QVariant();
}

QVariant C4ConsoleQtPropListModel::data(const QModelIndex & index, int role) const
{
	// Anything loaded?
	C4PropList *props = proplist.getPropList();
	if (!props) return QVariant();
	// Headers
	QModelIndex parent = index.parent();
	if (!parent.isValid())
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
	Property *prop = static_cast<Property *>(index.internalPointer());
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
			prop->delegate->GetPropertyValue(props, prop->key, &v);
			return QVariant(prop->delegate->GetDisplayString(v, props->GetObject()));
		}
		}
	}
	else if (role == Qt::BackgroundColorRole && index.column()==1)
	{
		C4Value v;
		prop->delegate->GetPropertyValue(props, prop->key, &v);
		QColor bgclr = prop->delegate->GetDisplayBackgroundColor(v, props->GetObject());
		if (bgclr.isValid()) return bgclr;
	}
	else if (role == Qt::TextColorRole && index.column() == 1)
	{
		C4Value v;
		prop->delegate->GetPropertyValue(props, prop->key, &v);
		QColor txtclr = prop->delegate->GetDisplayTextColor(v, props->GetObject());
		if (txtclr.isValid()) return txtclr;
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
		return createIndex(row, column, nullptr);
	}
	// Property?
	QModelIndex grandparent = parent.parent();
	if (!grandparent.isValid())
	{
		const PropertyGroup *property_group = NULL;
		if (parent.row() >= 0 && parent.row() < property_groups.size())
		{
			property_group = &property_groups[parent.row()];
			if (row < 0 || row >= property_group->props.size()) return QModelIndex();
			const Property * prop = &(property_group->props[row]);
			return createIndex(row, column, const_cast<Property *>(prop));
		}
	}
	return QModelIndex();
}

QModelIndex C4ConsoleQtPropListModel::parent(const QModelIndex &index) const
{
	Property *prop = static_cast<Property *>(index.internalPointer());
	if (!prop) return QModelIndex();
	// Find list to use
	return createIndex(prop->group_idx, 0, nullptr);
}

Qt::ItemFlags C4ConsoleQtPropListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.isValid() && index.column() == 1 && index.internalPointer())
	{
		Property *prop = static_cast<Property *>(index.internalPointer());
		C4PropList *parent_proplist = prop->parent_proplist.getPropList();
		// Only object properties are editable at the moment
		if (parent_proplist && !parent_proplist->IsFrozen() && (parent_proplist->GetObject()==parent_proplist))
			flags |= Qt::ItemIsEditable;
		else
			flags &= ~Qt::ItemIsEnabled;
	}
	return flags;
}
