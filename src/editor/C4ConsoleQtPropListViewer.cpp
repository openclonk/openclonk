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
#include "editor/C4Console.h"
#include "object/C4Object.h"
#include "object/C4DefList.h"
#include "object/C4Def.h"

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

C4PropertyDelegate::C4PropertyDelegate(const C4PropertyDelegateFactory *factory, const C4PropList *props)
	: QObject(), factory(factory)
{
	// Resolve setter callback name
	if (props) set_function = props->GetPropertyStr(P_Set);
}

void C4PropertyDelegate::UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const
{
	editor->setGeometry(option.rect);
}

C4PropertyDelegateInt::C4PropertyDelegateInt(const C4PropertyDelegateFactory *factory, const C4PropList *props)
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

void C4PropertyDelegateInt::SetModelData(QWidget *editor, const C4PropertyPath &property_path) const
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

C4PropertyDelegateEnum::C4PropertyDelegateEnum(const C4PropertyDelegateFactory *factory, const C4PropList *props, const C4ValueArray *poptions)
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
			const C4PropList *props = v.getPropList();
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
	for (auto &option : options)
	{
		bool match = false;
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
	return iopt % options.size();
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
	if (!option.adelegate && option.adelegate_val.GetType() != C4V_Nil)
		option.adelegate = factory->GetDelegateByValue(option.adelegate_val);
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
	editor->option_box->setCurrentIndex(GetOptionByValue(val));
	// Update parameter
	UpdateEditorParameter(editor);
	editor->updating = false;
}

void C4PropertyDelegateEnum::SetModelData(QWidget *aeditor, const C4PropertyPath &property_path) const
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
	for (auto &option : options) editor->option_box->addItem(option.name->GetData().getData());
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

C4PropertyDelegateDef::C4PropertyDelegateDef(const C4PropertyDelegateFactory *factory, const C4PropList *props)
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

C4PropertyDelegateC4ValueEnum::C4PropertyDelegateC4ValueEnum(const C4PropertyDelegateFactory *factory, const C4PropList *props)
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

void C4PropertyDelegateC4ValueInput::SetModelData(QWidget *aeditor, const C4PropertyPath &property_path) const
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




/* Delegate factory: Create delegates based on the C4Value type */

C4PropertyDelegate *C4PropertyDelegateFactory::CreateDelegateByString(const C4String *str, const C4PropList *props) const
{
	// safety
	if (!str) return NULL;
	// create default base types
	if (str->GetData() == "int") return new C4PropertyDelegateInt(this, props);
	if (str->GetData() == "def") return new C4PropertyDelegateDef(this, props);
	if (str->GetData() == "c4valueenum") return new C4PropertyDelegateC4ValueEnum(this, props);
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
	C4PropList *props = prop->parent_proplist->getPropList();
	if (props)
	{
		props->GetPropertyByS(prop->name.Get(), &val);
		d->SetEditorData(editor, val);
	}
}

void C4PropertyDelegateFactory::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	// Fetch property value from editor and set it into proplist
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!d) return;
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	C4PropList *props = prop->parent_proplist->getPropList();
	if (props)
	{
		// Compose set command
		C4PropertyPath path(prop->parent_proplist->GetDataString().getData());
		C4PropertyPath subpath;
		if (d->GetSetFunction())
			subpath = C4PropertyPath(path, d->GetSetFunction(), C4PropertyPath::PPT_SetFunction);
		else
			subpath = C4PropertyPath(path, prop->name->GetCStr());
		d->SetModelData(editor, subpath);
	}
}

QWidget *C4PropertyDelegateFactory::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	C4PropertyDelegate *d = GetDelegateByIndex(index);
	if (!d) return NULL;
	C4ConsoleQtPropListModel::Property *prop = static_cast<C4ConsoleQtPropListModel::Property *>(index.internalPointer());
	prop->about_to_edit = true;
	QWidget *editor = d->CreateEditor(this, parent, option);
	// Connect value change signals
	// For some reason, commitData needs a non-const pointer
	connect(d, &C4PropertyDelegate::EditorValueChangedSignal, editor, [editor, this](QWidget *signal_editor) {
		if (signal_editor == editor) const_cast<C4PropertyDelegateFactory *>(this)->EditorValueChanged(editor);
	});
	connect(d, &C4PropertyDelegate::EditingDoneSignal, editor, [editor, this](QWidget *signal_editor) {
		if (signal_editor == editor) const_cast<C4PropertyDelegateFactory *>(this)->EditingDone(editor);
	});
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


/* Proplist table view */

C4ConsoleQtPropListModel::C4ConsoleQtPropListModel()
{
	header_font.setBold(true);
}

C4ConsoleQtPropListModel::~C4ConsoleQtPropListModel()
{
}

void C4ConsoleQtPropListModel::SetPropList(class C4PropList *new_proplist)
{
	// Update properties
	proplist.SetPropList(new_proplist);
	if (new_proplist)
	{
		// Always: Internal properties
		auto new_properties = new_proplist->GetSortedLocalProperties();
		internal_properties.resize(new_properties.size());
		for (int32_t i = 0; i < new_properties.size(); ++i)
		{
			internal_properties[i].parent_proplist = &proplist;
			internal_properties[i].name = new_properties[i];
			internal_properties[i].delegate_info.Set0(); // default C4Value delegate
			internal_properties[i].delegate = NULL; // init when needed
			internal_properties[i].is_internal = true;
		}
		// Objects only: Published properties
		if (new_proplist->GetObject())
		{
			const char *editor_prop_prefix = "EditorProp_";
			auto new_properties = new_proplist->GetSortedProperties(editor_prop_prefix);
			published_properties.resize(new_properties.size());
			for (int32_t i = 0; i < new_properties.size(); ++i)
			{
				published_properties[i].parent_proplist = &proplist;
				published_properties[i].name = NULL;
				published_properties[i].delegate_info.Set0(); // default C4Value delegate
				published_properties[i].delegate = NULL; // init when needed
				published_properties[i].is_internal = false;
				C4Value published_prop_val;
				new_proplist->GetPropertyByS(new_properties[i], &published_prop_val);
				C4PropList *published_prop = published_prop_val.getPropList();
				if (published_prop)
				{
					published_properties[i].name = published_prop->GetPropertyStr(P_Name);
					published_properties[i].delegate_info.SetPropList(published_prop);
				}
				if (!published_properties[i].name) published_properties[i].name = ::Strings.RegString(new_properties[i]->GetCStr() + strlen(editor_prop_prefix));
			}
		}
		else
		{
			published_properties.clear();
		}
	}
	else
	{
		internal_properties.clear();
		published_properties.clear();
	}
	QModelIndex topLeft = index(0, 0, QModelIndex());
	QModelIndex bottomRight = index(rowCount() - 1, columnCount() - 1, QModelIndex());
	emit dataChanged(topLeft, bottomRight);
	emit layoutChanged();
}

int C4ConsoleQtPropListModel::rowCount(const QModelIndex & parent) const
{
	// Nothing loaded?
	if (!proplist.getPropList()) return 0;
	// Top level: Published and internal properties
	if (!parent.isValid()) return 2;
	// Mid level: Descend into property lists
	QModelIndex grandparent = parent.parent();
	if (!grandparent.isValid())
	{
		if (parent.row() == 0) return published_properties.size();
		if (parent.row() == 1) return internal_properties.size();
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
				if (index.row() == 0) return QVariant(LoadResStr("IDS_CNS_PROPERTIES"));
				if (index.row() == 1) return QVariant(LoadResStr("IDS_CNS_INTERNAL"));
			}
			else if (role == Qt::FontRole)
			{
				return header_font;
			}
		}
		return QVariant();
	}
	// Query latest data from prop list
	if (role == Qt::DisplayRole)
	{
		Property *prop = static_cast<Property *>(index.internalPointer());
		if (!prop) return QVariant();
		switch (index.column())
		{
		case 0: // First col: Property Name
			return QVariant(prop->name->GetCStr());
		case 1: // Second col: Property value
		{
			C4Value v;
			props->GetPropertyByS(prop->name, &v); // Just display nil if property is not set
			return QVariant(v.GetDataString().getData());
		}
		}
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
		if (row == 0 || row == 1) return createIndex(row, column, NULL);
		return QModelIndex();
	}
	// Property?
	QModelIndex grandparent = parent.parent();
	if (!grandparent.isValid())
	{
		const std::vector< Property > *property_list = NULL;
		if (parent.row() == 0) property_list = &published_properties;
		if (parent.row() == 1) property_list = &internal_properties;
		if (property_list)
		{
			if (row < 0 || row >= property_list->size()) return QModelIndex();
			const Property * prop = &(*property_list)[row];
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
	return createIndex(prop->is_internal ? 1 : 0, 0, NULL);
}

Qt::ItemFlags C4ConsoleQtPropListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.isValid() && index.column() == 1 && index.internalPointer())
	{
		Property *prop = static_cast<Property *>(index.internalPointer());
		C4PropList *parent_proplist = prop->parent_proplist->getPropList();
		// Only object properties are editable at the moment
		if (parent_proplist && !parent_proplist->IsFrozen() && (parent_proplist->GetObject()==parent_proplist))
			flags |= Qt::ItemIsEditable;
		else
			flags &= ~Qt::ItemIsEnabled;
	}
	return flags;
}
