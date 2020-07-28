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
#include "editor/C4ConsoleGUI.h" // for OpenGL
#include "editor/C4ConsoleQt.h"
#include "editor/C4ConsoleQtShapes.h"
#include "editor/C4PropertyPath.h"
#include "script/C4Value.h"

class C4ConsoleQtPropListModel;
struct C4ConsoleQtPropListModelProperty;

class C4PropertyDelegate : public QObject
{
	Q_OBJECT

protected:
	const class C4PropertyDelegateFactory *factory;
	C4Value creation_props;
	C4RefCntPointer<C4String> set_function, async_get_function, name;
	C4PropertyPath::PathType set_function_type;
	C4RefCntPointer<C4String> update_callback;

public:
	C4PropertyDelegate(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	~C4PropertyDelegate() override = default;

	virtual void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const {};
	virtual void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const {};
	virtual QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const = 0;
	virtual void UpdateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option) const;
	virtual bool GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const;
	virtual bool GetPropertyValueBase(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const;
	virtual QString GetDisplayString(const C4Value &val, class C4Object *obj, bool short_names) const;
	virtual QColor GetDisplayTextColor(const C4Value &val, class C4Object *obj) const;
	virtual QColor GetDisplayBackgroundColor(const C4Value &val, class C4Object *obj) const;
	const char *GetSetFunction() const { return set_function.Get() ? set_function->GetCStr() : nullptr; } // get name of setter function for this property
	virtual const class C4PropertyDelegateShape *GetShapeDelegate(C4Value &val, C4PropertyPath *shape_path) const { return nullptr;  }
	virtual const class C4PropertyDelegateShape *GetDirectShapeDelegate() const { return nullptr; }
	const char *GetUpdateCallback() const { return update_callback ? update_callback->GetCStr() : nullptr; }
	virtual bool HasCustomPaint() const { return false; }
	virtual bool Paint(QPainter *painter, const QStyleOptionViewItem &option, const C4Value &val) const { return false; }
	virtual C4PropertyPath GetPathForProperty(struct C4ConsoleQtPropListModelProperty *editor_prop) const;
	C4PropertyPath GetPathForProperty(const C4PropertyPath &parent_path, const char *default_subpath) const;
	C4String *GetNameStr() const { return name.Get(); }
	const C4Value &GetCreationProps() const { return creation_props; }
	virtual bool IsPasteValid(const C4Value &val) const = 0;

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

	void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const override;
	void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegateStringEditor : public QWidget
{
	Q_OBJECT
private:
	QLineEdit *edit;
	QPushButton *localization_button;
	bool text_edited, commit_pending;
	C4Value value;
	char lang_code[3];
	C4Value base_proplist;
	std::unique_ptr<class C4ConsoleQtLocalizeStringDlg> localization_dialogue;

	void OpenLocalizationDialogue();
	void CloseLocalizationDialogue();
	void StoreEditedText();
public:
	C4PropertyDelegateStringEditor(QWidget *parent, bool has_localization_button);
	void SetValue(const C4Value &val);
	C4Value GetValue();
	bool IsCommitPending() const { return commit_pending; }
	void SetCommitPending(bool to_val) { commit_pending = to_val; }
signals:
	void EditingDoneSignal() const;
};

class C4PropertyDelegateString : public C4PropertyDelegate
{
private:
	bool translatable;
public:
	typedef C4PropertyDelegateStringEditor Editor;

	C4PropertyDelegateString(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const override;
	void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	QString GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const override;
	bool IsPasteValid(const C4Value &val) const override;
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
	C4PropertyPath property_path;
	bool button_pending;

	C4PropertyDelegateLabelAndButtonWidget(QWidget *parent);
};

class C4PropertyDelegateDescendPath : public C4PropertyDelegate
{
protected:
	C4Value info_proplist;
	bool edit_on_selection;
	C4RefCntPointer<C4String> descend_path;
public:
	typedef C4PropertyDelegateLabelAndButtonWidget Editor;

	C4PropertyDelegateDescendPath(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	void SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
};

class C4PropertyDelegateArray : public C4PropertyDelegateDescendPath
{
private:
	int32_t max_array_display;
	mutable C4PropertyDelegate *element_delegate; // lazy eval

	void ResolveElementDelegate() const;
public:
	C4PropertyDelegateArray(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	QString GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const override;
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegatePropList : public C4PropertyDelegateDescendPath
{
private:
	C4RefCntPointer<C4String> display_string;
public:
	C4PropertyDelegatePropList(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	QString GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const override;
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegateEffectEditor : public QWidget
{
	Q_OBJECT

public:
	QHBoxLayout *layout;
	QPushButton *remove_button, *edit_button;
	C4PropertyPath property_path;

	C4PropertyDelegateEffectEditor(QWidget *parent);
};

class C4PropertyDelegateEffect : public C4PropertyDelegate
{
public:
	typedef C4PropertyDelegateEffectEditor Editor;

	C4PropertyDelegateEffect(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	void SetEditorData(QWidget *aeditor, const C4Value &val, const C4PropertyPath &property_path) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	QString GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const override;
	bool IsPasteValid(const C4Value &val) const override { return false; }
	bool GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const override;
	C4PropertyPath GetPathForProperty(C4ConsoleQtPropListModelProperty *editor_prop) const override;
};

class C4PropertyDelegateColor : public C4PropertyDelegate
{
private:
	uint32_t alpha_mask;

public:
	typedef C4PropertyDelegateLabelAndButtonWidget Editor;

	C4PropertyDelegateColor(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const override;
	void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	QString GetDisplayString(const C4Value &v, C4Object *obj, bool short_names) const override;
	QColor GetDisplayTextColor(const C4Value &val, class C4Object *obj) const override;
	QColor GetDisplayBackgroundColor(const C4Value &val, class C4Object *obj) const override;
	bool IsPasteValid(const C4Value &val) const override;

private:
	void OpenColorDialogue(Editor *editor) const;
};

// Display delegate for deep combo box. Handles the help tooltip showing.
class C4StyledItemDelegateWithButton : public QStyledItemDelegate
{
	Q_OBJECT

public:
	enum ButtonType
	{
		BT_Help,
		BT_PlaySound,
	} button_type;

	C4StyledItemDelegateWithButton(ButtonType bt) : button_type(bt) { }

public:
	bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
protected:
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

// A combo box that can select from a model with nested elements
// On click, descend into child elements
class C4DeepQComboBox : public QComboBox
{
	Q_OBJECT

	bool editable, manual_text_edited;
	QString last_edited_text;
	bool is_next_close_blocked;
	int last_popup_height;
	std::unique_ptr<C4StyledItemDelegateWithButton> item_delegate;
	QSize default_icon_size;

public:
	enum
	{
		OptionIndexRole = Qt::UserRole + 1,
		ObjectHighlightRole = Qt::UserRole + 2,
		ValueStringRole = Qt::UserRole + 3,
		PriorityNameSortRole = Qt::UserRole + 4,
	};

	C4DeepQComboBox(QWidget *parent, C4StyledItemDelegateWithButton::ButtonType button_type, bool editable);

	void showPopup() override;
	void hidePopup() override;

	void setCurrentModelIndex(QModelIndex new_index);
	int32_t GetCurrentSelectionIndex();
	void BlockNextCloseEvent() { is_next_close_blocked = true; }; // after item selection on a "play" button, the combo dropdown should stay open

public slots:
	void doShowPopup() { showPopup(); }

signals:
	void NewItemSelected(int32_t new_item);
	void TextChanged(const QString &new_text);

protected:
	// event filter for view: Catch mouse clicks to descend into children
	bool eventFilter(QObject *obj, QEvent *event) override;
};

// Widget holder class
class C4PropertyDelegateEnumEditor : public QWidget
{
	Q_OBJECT

public:
	enum { INDEX_Custom_Value = -1 };
	C4Value last_val;
	C4Value last_parameter_val; // Resolved parameter of last_val - assigned for shape parameters only
	int32_t last_selection_index; // Index of selection in model or INDEX_Custom_Value for custom value that does not resolve to an existing entry in editable enum
	C4PropertyPath last_get_path;
	C4DeepQComboBox *option_box;
	QHBoxLayout *layout;
	QWidget *parameter_widget;
	bool updating, option_changed, dropdown_pending;
	const C4PropertyDelegate *paint_parameter_delegate; // Delegate to draw over the parameter_widget if it's an empty transparent QWidget (for shape delegates)

	C4PropertyDelegateEnumEditor(QWidget *parent)
		: QWidget(parent), last_selection_index(-1), option_box(nullptr), layout(nullptr), parameter_widget(nullptr),
		updating(false), option_changed(false), dropdown_pending(false), paint_parameter_delegate(nullptr) { }

	void paintEvent(QPaintEvent *) override;
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
		C4RefCntPointer<C4String> short_name; // Shortened name displayed as sub-delegate
		C4RefCntPointer<C4String> help; // Tooltip text
		C4RefCntPointer<C4String> group; // Grouping in enum dropdown box; nested groups separated by '/'
		C4RefCntPointer<C4String> option_key;
		C4RefCntPointer<C4String> value_key;
		C4RefCntPointer<C4String> sound_name; // Assigned for options that have a play button
		C4V_Type type{C4V_Any}; // Assume this option is set when value is of given type
		C4Value props; // Stored pointer to proplist defining this option
		C4Value value; // Value to set if this entry is selected
		bool force_serialization{false}; // If serialization should be forced on value
		C4Value value_function; // Function to be called to set value
		mutable C4PropertyDelegate *adelegate{nullptr}; // Delegate to display if this entry is selected (pointer owned by C4PropertyDelegateFactory)
		C4Value adelegate_val; // Value to resolve adelegate from
		// How the currently selected option is identified from the value
		enum StorageType {
			StorageNone=0, // Invalid option
			StorageByType=1, // Use type to identify this enum
			StorageByValue=2, // This option sets a constant value
			StorageByKey=3, // Assume value is a proplist; identify option by field option_key
		} storage_type{StorageNone};
		int32_t priority{0}; // Custom sort order

		Option() = default;
	};

protected:
	virtual C4StyledItemDelegateWithButton::ButtonType GetOptionComboBoxButtonType() const { return C4StyledItemDelegateWithButton::BT_Help; }

private:
	std::vector<Option> options;
	Option default_option;
	bool allow_editing;
	bool sorted;

protected:
	C4RefCntPointer<C4String> empty_name; // Override for name of empty option

protected:
	void ClearOptions();
	void ReserveOptions(int32_t num);
	QStandardItemModel *CreateOptionModel() const;
public:
	C4PropertyDelegateEnum(const class C4PropertyDelegateFactory *factory, C4PropList *props, const C4ValueArray *poptions=nullptr);

	void AddTypeOption(C4String *name, C4V_Type type, const C4Value &val, C4PropertyDelegate *adelegate=nullptr);
	void AddConstOption(C4String *name, const C4Value &val, C4String *group=nullptr, C4String *sound_name=nullptr);

	void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const override;
	void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	QString GetDisplayString(const C4Value &val, class C4Object *obj, bool short_names) const override;
	const class C4PropertyDelegateShape *GetShapeDelegate(C4Value &val, C4PropertyPath *shape_path) const override; // Forward to parameter of selected option
	bool HasCustomPaint() const override { return true; }
	bool Paint(QPainter *painter, const QStyleOptionViewItem &option, const C4Value &val) const override;
	bool IsPasteValid(const C4Value &val) const override;

private:
	QModelIndex GetModelIndexByID(QStandardItemModel *model, QStandardItem *parent_item, int32_t id, const QModelIndex &parent) const;
	int32_t GetOptionByValue(const C4Value &val) const;
	void UpdateEditorParameter(C4PropertyDelegateEnum::Editor *editor, bool by_selection) const;
	void EnsureOptionDelegateResolved(const Option &option) const;
	void SetOptionValue(const C4PropertyPath &use_path, const C4PropertyDelegateEnum::Option &option, const C4Value &option_value) const;
	void UpdateOptionIndex(Editor *editor, int idx, const QString *custom_text) const;
};

// Select a definition
class C4PropertyDelegateDef : public C4PropertyDelegateEnum
{
private:
	C4RefCntPointer<C4String> filter_property;
public:
	C4PropertyDelegateDef(const C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;

private:
	void AddDefinitions(class C4ConsoleQtDefinitionListModel *def_list_model, QModelIndex parent, C4String *group);
};

// Select an object
class C4PropertyDelegateObject : public C4PropertyDelegateEnum
{
private:
	C4RefCntPointer<C4String> filter;
	size_t max_nearby_objects; // maximum number of objects shown in "nearby" list

	C4RefCntPointer<C4String> GetObjectEntryString(C4Object *obj) const;
	void UpdateObjectList();
public:
	C4PropertyDelegateObject(const C4PropertyDelegateFactory *factory, C4PropList *props);

	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	QString GetDisplayString(const C4Value &v, class C4Object *obj, bool short_names) const override;
	bool IsPasteValid(const C4Value &val) const override;
};

// Select a sound
class C4PropertyDelegateSound : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateSound(const C4PropertyDelegateFactory *factory, C4PropList *props);
	QString GetDisplayString(const C4Value &v, class C4Object *obj, bool short_names) const override;
	bool IsPasteValid(const C4Value &val) const override;
protected:
	C4StyledItemDelegateWithButton::ButtonType GetOptionComboBoxButtonType() const override { return C4StyledItemDelegateWithButton::BT_PlaySound; }
};

// true or false
class C4PropertyDelegateBool : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateBool(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	bool GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const override;
	bool IsPasteValid(const C4Value &val) const override;
};

// true or false depending on whether effect is present
class C4PropertyDelegateHasEffect : public C4PropertyDelegateBool
{
private:
	C4RefCntPointer<C4String> effect;
public:
	C4PropertyDelegateHasEffect(const class C4PropertyDelegateFactory *factory, C4PropList *props);

	bool GetPropertyValue(const C4Value &container, C4String *key, int32_t index, C4Value *out_val) const override;
};

// C4Value setting using an enum
class C4PropertyDelegateC4ValueEnum : public C4PropertyDelegateEnum
{
public:
	C4PropertyDelegateC4ValueEnum(const C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override { return true; }
};

class C4PropertyDelegateC4ValueInputEditor : public QWidget // TODO: Merge with C4PropertyDelegateLabelAndButtonWidget
{
	Q_OBJECT

public:
	QHBoxLayout *layout;
	QLineEdit *edit;
	QPushButton *extended_button;
	bool commit_pending;
	C4PropertyPath property_path;

	C4PropertyDelegateC4ValueInputEditor(QWidget *parent);
};

// C4Value setting using an input box
class C4PropertyDelegateC4ValueInput : public C4PropertyDelegate
{
public:
	typedef C4PropertyDelegateC4ValueInputEditor Editor;

	C4PropertyDelegateC4ValueInput(const C4PropertyDelegateFactory *factory, C4PropList *props) : C4PropertyDelegate(factory, props) { }

	void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const override;
	void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override;
	bool IsPasteValid(const C4Value &val) const override { return true; }
};

// areas shown in viewport
class C4PropertyDelegateShape : public C4PropertyDelegate
{
	uint32_t clr;

	virtual void DoPaint(QPainter *painter, const QRect &inner_rect) const = 0;
public:
	C4PropertyDelegateShape(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	void SetEditorData(QWidget *editor, const C4Value &val, const C4PropertyPath &property_path) const override { } // TODO maybe implement update?
	void SetModelData(QObject *editor, const C4PropertyPath &property_path, class C4ConsoleQtShape *prop_shape) const override;
	QWidget *CreateEditor(const class C4PropertyDelegateFactory *parent_delegate, QWidget *parent, const QStyleOptionViewItem &option, bool by_selection, bool is_child) const override { return nullptr; }
	const C4PropertyDelegateShape *GetShapeDelegate(C4Value &val, C4PropertyPath *shape_path) const override { return this; }
	const C4PropertyDelegateShape *GetDirectShapeDelegate() const override { return this; }
	bool HasCustomPaint() const override { return true; }
	bool Paint(QPainter *painter, const QStyleOptionViewItem &option, const C4Value &val) const override;
	QString GetDisplayString(const C4Value &v, class C4Object *obj, bool short_names) const override { return QString(); }

	virtual void ConnectSignals(C4ConsoleQtShape *shape, const C4PropertyPath &property_path) const;
};

class C4PropertyDelegateRect : public C4PropertyDelegateShape
{
	C4RefCntPointer<C4String> storage;

	void DoPaint(QPainter *painter, const QRect &inner_rect) const override;
public:
	C4PropertyDelegateRect(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegateCircle : public C4PropertyDelegateShape
{
	bool can_move_center;

	void DoPaint(QPainter *painter, const QRect &inner_rect) const override;
public:
	C4PropertyDelegateCircle(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegatePoint : public C4PropertyDelegateShape
{
	bool horizontal_fix{ false };
	bool vertical_fix{ false };

	void DoPaint(QPainter *painter, const QRect &inner_rect) const override;
public:
	C4PropertyDelegatePoint(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegateGraph : public C4PropertyDelegateShape
{
	bool horizontal_fix = false;
	bool vertical_fix = false;
	bool structure_fix = false;

	void DoPaint(QPainter *painter, const QRect &inner_rect) const override;
protected:
	bool IsVertexPasteValid(const C4Value &val) const;
	bool IsEdgePasteValid(const C4Value &val) const;
public:
	C4PropertyDelegateGraph(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;

	void ConnectSignals(C4ConsoleQtShape *shape, const C4PropertyPath &property_path) const override;
};

class C4PropertyDelegatePolyline : public C4PropertyDelegateGraph
{
	void DoPaint(QPainter *painter, const QRect &inner_rect) const override;
public:
	C4PropertyDelegatePolyline(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegatePolygon : public C4PropertyDelegateGraph
{
	void DoPaint(QPainter *painter, const QRect &inner_rect) const override;
public:
	C4PropertyDelegatePolygon(const class C4PropertyDelegateFactory *factory, C4PropList *props);
	bool IsPasteValid(const C4Value &val) const override;
};

class C4PropertyDelegateFactory : public QStyledItemDelegate
{
	Q_OBJECT

	mutable std::map<C4PropList *, std::unique_ptr<C4PropertyDelegate> > delegates;
	mutable C4PropertyDelegateEffect effect_delegate;
	mutable QWidget *current_editor{nullptr};
	mutable C4PropertyDelegate *current_editor_delegate;
	mutable C4Value last_edited_value;
	class C4ConsoleQtPropListModel *property_model{nullptr};
	class C4ConsoleQtDefinitionListModel *def_list_model;

	C4PropertyDelegate *CreateDelegateByPropList(C4PropList *props) const;
	C4PropertyDelegate *GetDelegateByIndex(const QModelIndex &index) const;
public:
	C4PropertyDelegateFactory();
	~C4PropertyDelegateFactory() override = default;

	C4PropertyDelegate *GetDelegateByValue(const C4Value &val) const;
	C4PropertyDelegateEffect *GetEffectDelegate() const { return &effect_delegate; }

	void ClearDelegates();
	void SetPropertyData(const C4PropertyDelegate *d, QObject *editor, C4ConsoleQtPropListModelProperty *editor_prop) const;
	void SetPropertyModel(class C4ConsoleQtPropListModel *new_property_model) { property_model = new_property_model; }
	void SetDefinitionListModel(class C4ConsoleQtDefinitionListModel *new_def_list_model) { def_list_model = new_def_list_model; }
	class C4ConsoleQtDefinitionListModel *GetDefinitionListModel() const { return def_list_model; }
	class C4ConsoleQtPropListModel *GetPropertyModel() const { return property_model; }
	void OnPropListChanged();
	bool CheckCurrentEditor(C4PropertyDelegate *d, QWidget *editor) const;

private:
	void EditorValueChanged(QWidget *editor);
	void EditingDone(QWidget *editor);
	void CopyToClipboard(const QModelIndex &index);
	bool PasteFromClipboard(const QModelIndex &index, bool check_only);

protected:
	// Model callbacks forwarded to actual delegates
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void destroyEditor(QWidget *editor, const QModelIndex &index) const override;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
};

// Delegate for the name column of the property window
// For now, just use the default + help button
class C4PropertyNameDelegate : public C4StyledItemDelegateWithButton
{
	Q_OBJECT

	class C4ConsoleQtPropListModel *property_model{nullptr};

public:
	C4PropertyNameDelegate() : C4StyledItemDelegateWithButton(C4StyledItemDelegateWithButton::BT_Help) { }

	void SetPropertyModel(class C4ConsoleQtPropListModel *new_property_model) { property_model = new_property_model; }
};

// One property in the prop list model view
struct C4ConsoleQtPropListModelProperty
{
	C4PropertyPath property_path;
	C4Value parent_value;
	C4RefCntPointer<C4String> display_name;
	C4RefCntPointer<C4String> help_text;
	C4RefCntPointer<C4String> key;
	C4Value delegate_info;
	C4PropertyDelegate *delegate{nullptr};
	bool about_to_edit{false};
	int32_t priority{0};

	// Parent group index
	int32_t group_idx{-1};

	// Each property may be connected to one shape shown in the viewport for editing
	C4ConsoleQtShapeHolder *shape;
	const C4PropertyDelegate *shape_delegate{nullptr};
	C4PropertyPath shape_property_path;

	C4ConsoleQtPropListModelProperty() = default;
};

// Prop list view implemented as a model view
class C4ConsoleQtPropListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	typedef C4ConsoleQtPropListModelProperty Property;
	struct PropertyGroup
	{
		QString name;
		std::vector<Property> props;
	};
	struct TargetStackEntry // elements of the path for setting child properties
	{
		C4PropertyPath path;
		// TODO: Would be nice to store only path without values and info_proplist. However, info_proplist is hard to resolve when traversing up
		// So just keep the value for now and hope that proplists do not change during selection
		C4Value value, info_proplist;

		TargetStackEntry(const C4PropertyPath &path, const C4Value &value, const C4Value &info_proplist)
			: path(path), value(value), info_proplist(info_proplist) {}
	};
	struct EditedPath // Information about how to find currently edited element (to restore after model update)
	{
		C4PropertyPath target_path;
		int32_t major_index, minor_index;
	};
private:
	C4Value target_value; // Target value for which properties are listed (either proplist or array)
	C4Value base_proplist; // Parent-most value, i.e. object or effect selected and framed in editor
	C4Value info_proplist; // Proplist from which available properties are derived. May differ from target_proplist in child proplists.
	C4PropertyPath target_path; // script path to target proplist to set values
	std::list<TargetStackEntry> target_path_stack; // stack of target paths descended into by setting child properties
	std::vector<PropertyGroup> property_groups;
	QFont header_font, important_property_font;
	C4PropertyDelegateFactory *delegate_factory;
	QItemSelectionModel *selection_model;
	bool layout_valid; // set to false when property numbers change
	std::map<std::string, C4ConsoleQtShapeHolder> shapes; // shapes currently shown in editor. Indexed by get path
public:
	C4ConsoleQtPropListModel(C4PropertyDelegateFactory *delegate_factory);
	~C4ConsoleQtPropListModel() override;

	void SetSelectionModel(QItemSelectionModel *m) { selection_model = m; }
	QItemSelectionModel *GetSelectionModel() const { return selection_model; }

	bool AddPropertyGroup(C4PropList *add_proplist, int32_t group_index, QString name, C4PropList *target_proplist, const C4PropertyPath &group_target_path, C4Object *base_object, C4String *default_selection, int32_t *default_selection_index);
	bool AddEffectGroup(int32_t group_index, C4Object *base_object);
	void SetBasePropList(C4PropList *new_proplist); // Clear stack and select new proplist
	void DescendPath(const C4Value &new_value, C4PropList *new_info_proplist, const C4PropertyPath &new_path); // Add proplist to stack
	void AscendPath(); // go back one element in target path stack
	void UpdateValue(bool select_default);
	void DoOnUpdateCall(const C4PropertyPath &updated_path, const C4PropertyDelegate *delegate);

private:
	int32_t UpdateValuePropList(C4PropList *target_proplist, int32_t *default_selection_group, int32_t *default_selection_index);
	int32_t UpdateValueArray(C4ValueArray *target_array, int32_t *default_selection_group, int32_t *default_selection_index);

signals:
	void ProplistChanged(int32_t major_sel, int32_t minor_sel) const;

private slots:
	void UpdateSelection(int32_t major_sel, int32_t minor_sel) const;

public:
	class C4PropList *GetTargetPropList() const { return target_value.getPropList(); }
	class C4ValueArray *GetTargetArray() const { return target_value.getArray(); }
	class C4PropList *GetBasePropList() const { return base_proplist.getPropList(); }
	int32_t GetTargetPathStackSize() const { return target_path_stack.size(); }
	const char *GetTargetPathText() const { return target_path.GetGetPath(); }
	QString GetTargetPathHelp() const;
	const char *GetTargetPathName() const;
	bool IsArray() const { return !!target_value.getArray(); }
	void AddArrayElement();
	void RemoveArrayElement();
	bool IsTargetReadonly() const;
	C4ConsoleQtPropListModel::Property *GetPropByIndex(const QModelIndex &index) const;
	class C4ConsoleQtShape *GetShapeByPropertyPath(const char *property_path);

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	Qt::DropActions supportedDropActions() const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
	QStringList mimeTypes() const override;
	QMimeData *mimeData(const QModelIndexList &indexes) const override;
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtPropListViewer
