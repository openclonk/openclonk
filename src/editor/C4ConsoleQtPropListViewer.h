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

#include <C4Include.h> // needed for automoc
#include <C4ConsoleGUI.h> // for glew.h
#include <C4ConsoleQt.h>
#include <C4Value.h>

// Path to a property, like e.g. Object(123).foo.bar[456].baz
// Used to allow proper synchronization of property setting
class C4PropertyPath
{
	// TODO: For now just storing the path. May want to keep the path info later to allow validation/updating of values
	StdCopyStrBuf path;

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

public:
	C4PropertyDelegate(const class C4PropertyDelegateFactory *factory)
		: factory(factory) { }
	virtual ~C4PropertyDelegate() { }

	virtual void SetEditorData(QWidget *editor, const C4Value &val) const = 0;
	virtual void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const = 0;
	virtual QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const = 0;
	virtual void UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const;

signals:
	void EditorValueChangedSignal(QWidget *editor) const;
	void EditingDoneSignal(QWidget *editor) const;
};

class C4PropertyDelegateInt : public C4PropertyDelegate
{
public:
	C4PropertyDelegateInt(const class C4PropertyDelegateFactory *factory, const C4PropList *props=NULL);

	void SetEditorData(QWidget *editor, const C4Value &val) const override;
	void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const override;
};

// Widget holder class
class C4PropertyDelegateEnumEditor : public QWidget
{
	Q_OBJECT

public:
	const class C4PropertyDelegateEnum *parent_delegate;
	C4Value last_val;
	QComboBox *option_box;
	QHBoxLayout *layout;
	QWidget *parameter_widget;
	bool updating;

	C4PropertyDelegateEnumEditor(QWidget *parent, const class C4PropertyDelegateEnum *parent_delegate)
		: QWidget(parent), parent_delegate(parent_delegate), option_box(NULL), layout(NULL), parameter_widget(NULL), updating(false) { }
public slots :
	void UpdateOptionIndex(int idx);
};

class C4PropertyDelegateEnum : public C4PropertyDelegate
{
	Q_OBJECT

public:
	typedef C4PropertyDelegateEnumEditor Editor; // qmake doesn't like nested classes

	struct Option
	{
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
public:
	C4PropertyDelegateEnum(const class C4PropertyDelegateFactory *factory, int reserve_count = 0);
	C4PropertyDelegateEnum(const class C4PropertyDelegateFactory *factory, const C4ValueArray &props);

	void AddTypeOption(C4String *name, C4V_Type type, const C4Value &val, C4PropertyDelegate *adelegate=NULL);

	void SetEditorData(QWidget *editor, const C4Value &val) const override;
	void SetModelData(QWidget *editor, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option) const override;
	void UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const override;

private:
	int32_t GetOptionByValue(const C4Value &val) const;
	void UpdateEditorParameter(C4PropertyDelegateEnum::Editor *editor) const;

public slots:
	void UpdateOptionIndex(Editor *editor, int idx) const;
};

class C4PropertyDelegateC4Value : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateC4Value(const C4PropertyDelegateFactory *factory);
};

class C4PropertyDelegateFactory : public QStyledItemDelegate
{
	Q_OBJECT

	mutable std::map<C4Value, std::unique_ptr<C4PropertyDelegate> > delegates;

	C4PropertyDelegate *CreateDelegateByString(const C4String *str, const C4PropList *props=NULL) const;
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

};

// Prop list view implemented as a model view
class C4ConsoleQtPropListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	struct Property
	{
		C4PropertyPath property_path;
		C4Value *parent_proplist;
		C4RefCntPointer<C4String> name;
		C4Value delegate_info;
		C4PropertyDelegate *delegate;
		bool about_to_edit;

		Property() : parent_proplist(NULL), delegate(NULL), about_to_edit(false) {}
	};
private:
	C4Value proplist;
	std::vector< Property > properties;
public:
	C4ConsoleQtPropListModel();
	~C4ConsoleQtPropListModel();

	void SetPropList(class C4PropList *new_proplist);

protected:
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
