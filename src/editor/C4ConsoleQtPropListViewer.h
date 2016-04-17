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

/* Proplist table view */

#ifndef INC_C4ConsoleQtPropListViewer
#define INC_C4ConsoleQtPropListViewer
#ifdef WITH_QT_EDITOR

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for glew.h
#include "editor/C4ConsoleQt.h"
#include "script/C4Value.h"

// Path to a property, like e.g. Object(123).foo.bar[456].baz
// Used to allow proper synchronization of property setting
class C4PropertyPath
{
	// TODO: For now just storing the path. May want to keep the path info later to allow validation/updating of values
	StdCopyStrBuf path;

public:
	enum PathType
	{
		PPT_Root = 0,
		PPT_Property = 1,
		PPT_Index = 2,
		PPT_SetFunction = 3,
	} path_type;
public:
	C4PropertyPath() {}
	C4PropertyPath(const char *path) : path(path), path_type(PPT_Root) {}
	C4PropertyPath(const C4PropertyPath &parent, int32_t elem_index);
	C4PropertyPath(const C4PropertyPath &parent, const char *child_property, PathType path_type = PPT_Property);
	const char *GetPath() const { return path.getData(); }

	void SetProperty(const char *set_string) const;
	void SetProperty(const C4Value &to_val) const;
};

class C4PropertyDelegate : public QObject
{
	Q_OBJECT

protected:
	const class C4PropertyDelegateFactory *factory;
	C4RefCntPointer<C4String> set_function, async_get_function;

public:
	C4PropertyDelegate(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	virtual ~C4PropertyDelegate() { }

	virtual void SetEditorData(QWidget *editor, const C4Value &val) const = 0;
	virtual void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const = 0;
	virtual QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const = 0;
	virtual void UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const;
	virtual bool GetPropertyValue(C4PropList *props, C4String *key, C4Value *out_val) const;
	virtual QString GetDisplayString(const C4Value &val, class C4Object *obj) const;
	virtual QColor GetDisplayTextColor(const C4Value &val, class C4Object *obj) const;
	virtual QColor GetDisplayBackgroundColor(const C4Value &val, class C4Object *obj) const;

	const char *GetSetFunction() const { return set_function.Get() ? set_function->GetCStr() : NULL; } // get name of setter function for this property

signals:
	void EditorValueChangedSignal(QWidget *editor) const;
	void EditingDoneSignal(QWidget *editor) const;
};

class C4PropertyDelegateInt : public C4PropertyDelegate
{
private:
	int32_t min, max, step;
public:
	C4PropertyDelegateInt(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	void SetEditorData(QWidget *editor, const C4Value &val) const override;
	void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const override;
};

// Editor: Text displaying value plus a button that opens an extended editor
class C4PropertyDelegateLabelAndButtonWidget : public QWidget
{
	Q_OBJECT

public:
	QHBoxLayout *layout;
	QLabel *label;
	QPushButton *button;
	C4Value last_value;

	C4PropertyDelegateLabelAndButtonWidget(QWidget *parent);
};

class C4PropertyDelegateColor : public C4PropertyDelegate
{
public:
	typedef C4PropertyDelegateLabelAndButtonWidget Editor;

	C4PropertyDelegateColor(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	void SetEditorData(QWidget *editor, const C4Value &val) const override;
	void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const override;
	QString GetDisplayString(const C4Value &v, C4Object *obj) const override;
	QColor GetDisplayTextColor(const C4Value &val, class C4Object *obj) const override;
	QColor GetDisplayBackgroundColor(const C4Value &val, class C4Object *obj) const override;
};

// Widget holder class
class C4PropertyDelegateEnumEditor : public QWidget
{
	Q_OBJECT

public:
	C4Value last_val;
	QComboBox *option_box;
	QHBoxLayout *layout;
	QWidget *parameter_widget;
	bool updating;

	C4PropertyDelegateEnumEditor(QWidget *parent)
		: QWidget(parent), option_box(NULL), layout(NULL), parameter_widget(NULL), updating(false) { }
};

class C4PropertyDelegateEnum : public C4PropertyDelegate
{
	Q_OBJECT

public:
	typedef C4PropertyDelegateEnumEditor Editor; // qmake doesn't like nested classes

	class Option
	{
	public:
		C4RefCntPointer<C4String> name; // Display name in Editor enum dropdown box
		C4RefCntPointer<C4String> option_key;
		C4RefCntPointer<C4String> value_key;
		C4V_Type type; // Assume this option is set when value is of given type
		C4Value value; // Value to set if this entry is selected
		mutable C4PropertyDelegate *adelegate; // Delegate to display if this entry is selected (pointer owned by C4PropertyDelegateFactory)
		C4Value adelegate_val; // Value to resolve adelegate from
		// How the currently selected option is identified from the value
		enum StorageType {
			StorageNone=0, // Invalid option
			StorageByType=1, // Use type to identify this enum
			StorageByValue=2, // This option sets a constant value
			StorageByKey=3, // Assume value is a proplist; identify option by field option_key
		} storage_type;

		Option() : type(C4V_Any), adelegate(NULL), storage_type(StorageNone) {}
	};
private:
	std::vector<Option> options;

protected:
	void ReserveOptions(int32_t num);
public:
	C4PropertyDelegateEnum(const class C4PropertyDelegateFactory *factory, C4PropList *props, const C4ValueArray *poptions=NULL);

	void AddTypeOption(C4String *name, C4V_Type type, const C4Value &val, C4PropertyDelegate *adelegate=NULL);
	void AddConstOption(C4String *name, const C4Value &val);

	void SetEditorData(QWidget *editor, const C4Value &val) const override;
	void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const override;
	QString GetDisplayString(const C4Value &val, class C4Object *obj) const override;

private:
	int32_t GetOptionByValue(const C4Value &val) const;
	void UpdateEditorParameter(C4PropertyDelegateEnum::Editor *editor) const;

public slots:
	void UpdateOptionIndex(Editor *editor, int idx) const;
};

// Select a definition
class C4PropertyDelegateDef : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateDef(const C4PropertyDelegateFactory *factory, C4PropList *props);
};

// true or false
class C4PropertyDelegateBool : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateBool(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	bool GetPropertyValue(C4PropList *props, C4String *key, C4Value *out_val) const override;
};

// true or false depending on whether effect is present
class C4PropertyDelegateHasEffect : public C4PropertyDelegateBool
{
private:
	C4RefCntPointer<C4String> effect;
public:
	C4PropertyDelegateHasEffect(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	bool GetPropertyValue(C4PropList *props, C4String *key, C4Value *out_val) const override;
};

// C4Value setting using an enum
class C4PropertyDelegateC4ValueEnum : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateC4ValueEnum(const C4PropertyDelegateFactory *factory, C4PropList *props);
};

class C4PropertyDelegateC4ValueInputEditor : public QWidget
{
	Q_OBJECT

public:
	QHBoxLayout *layout;
	QLineEdit *edit;
	QPushButton *extended_button;
	bool commit_pending;

	C4PropertyDelegateC4ValueInputEditor(QWidget *parent)
		: QWidget(parent), layout(NULL), edit(NULL), extended_button(NULL), commit_pending(false){ }
};

// C4Value setting using an input box
class C4PropertyDelegateC4ValueInput : public C4PropertyDelegate
{
public:
	typedef C4PropertyDelegateC4ValueInputEditor Editor;

	C4PropertyDelegateC4ValueInput(const C4PropertyDelegateFactory *factory, C4PropList *props) : C4PropertyDelegate(factory, props) { }

	void SetEditorData(QWidget *editor, const C4Value &val) const override;
	void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const override;
};

class C4PropertyDelegateFactory : public QStyledItemDelegate
{
	Q_OBJECT

	mutable std::map<C4Value, std::unique_ptr<C4PropertyDelegate> > delegates;

	C4PropertyDelegate *CreateDelegateByString(const C4String *str, C4PropList *props=NULL) const;
	C4PropertyDelegate *CreateDelegateByValue(const C4Value &val) const;
	C4PropertyDelegate *GetDelegateByIndex(const QModelIndex &index) const;
public:
	C4PropertyDelegateFactory() { }
	~C4PropertyDelegateFactory() { }

	C4PropertyDelegate *GetDelegateByValue(const C4Value &val) const;

	void ClearDelegates();

private:
	void EditorValueChanged(QWidget *editor);
	void EditingDone(QWidget *editor);

protected:
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

// Prop list view implemented as a model view
class C4ConsoleQtPropListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	struct Property
	{
		C4PropertyPath property_path;
		C4Value parent_proplist;
		C4RefCntPointer<C4String> display_name;
		C4RefCntPointer<C4String> key;
		C4Value delegate_info;
		C4PropertyDelegate *delegate;
		bool about_to_edit;
		int32_t group_idx;

		Property() : delegate(NULL), about_to_edit(false), group_idx(-1) {}
	};
	struct PropertyGroup
	{
		QString name;
		std::vector<Property> props;
	};
private:
	C4Value proplist;
	std::vector<PropertyGroup> property_groups;
	std::vector< Property > internal_properties;  // proplist-properties
	QFont header_font;
	C4PropertyDelegateFactory *delegate_factory;
public:
	C4ConsoleQtPropListModel(C4PropertyDelegateFactory *delegate_factory);
	~C4ConsoleQtPropListModel();

	bool AddPropertyGroup(C4PropList *add_proplist, int32_t group_index, QString name, C4PropList *ignore_overridden);
	void SetPropList(class C4PropList *new_proplist);
	class C4PropList *GetPropList() const { return proplist.getPropList(); }

public:
	virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtPropListViewer
