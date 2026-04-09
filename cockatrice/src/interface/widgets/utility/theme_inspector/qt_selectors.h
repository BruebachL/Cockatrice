#ifndef COCKATRICE_QT_SELECTORS_H
#define COCKATRICE_QT_SELECTORS_H

#include <QHash>
#include <QMap>
#include <QStringList>
#include <QWidget>

namespace QtSelectors
{
struct SelectorGroups
{
    QStringList types;
    QStringList objects;
    QStringList pseudos;
    QStringList subControls;
};

struct SelectorSuggestion
{
    QString selector;
    QString explanation;
    int confidence;
};

// Full Qt pseudo-states (simplified friendly descriptions)
inline const QVector<QPair<QString, QString>> PSEUDO_STATES = {
    {":active", "Widget is in the active window"},
    {":adjoins-item", "Branch of a QTreeView is adjacent to an item"},
    {":alternate", "Alternate row in QAbstractItemView with alternating colors"},
    {":bottom", "Item is at the bottom (e.g. QTabBar)"},
    {":checked", "Item is checked (e.g. QCheckBox, QRadioButton)"},
    {":closable", "Item can be closed (e.g. QDockWidget)"},
    {":closed", "Item is in closed state"},
    {":default", "Default item (e.g. default QPushButton)"},
    {":disabled", "Widget is disabled"},
    {":editable", "QComboBox is editable"},
    {":edit-focus", "Item has edit focus (Qt Extended only)"},
    {":enabled", "Item is enabled"},
    {":exclusive", "Item is part of exclusive group"},
    {":first", "Item is first in a list"},
    {":flat", "Item is flat (e.g. QPushButton)"},
    {":floatable", "Item can be floated (e.g. QDockWidget)"},
    {":focus", "Item has input focus"},
    {":has-children", "Item has children (e.g. QTreeView)"},
    {":has-siblings", "Item has siblings (e.g. QTreeView)"},
    {":horizontal", "Item has horizontal orientation"},
    {":hover", "Mouse is over this item"},
    {":indeterminate", "Item has indeterminate state (partially checked)"},
    {":last", "Item is last in a list"},
    {":left", "Item is positioned at left"},
    {":maximized", "Item is maximized (e.g. QMdiSubWindow)"},
    {":middle", "Item is in the middle (not first or last)"},
    {":minimized", "Item is minimized (e.g. QMdiSubWindow)"},
    {":movable", "Item can be moved (e.g. QDockWidget)"},
    {":no-frame", "Item has no frame (frameless QLineEdit/QSpinBox)"},
    {":non-exclusive", "Item is part of a non-exclusive group"},
    {":off", "Toggle item is off"},
    {":on", "Toggle item is on"},
    {":only-one", "Item is the only one (lone tab)"},
    {":open", "Item is in open state (expanded)"},
    {":next-selected", "Next item is selected"},
    {":pressed", "Item is being pressed"},
    {":previous-selected", "Previous item is selected"},
    {":read-only", "Item is read-only or non-editable"},
    {":right", "Item is positioned at right"},
    {":selected", "Item is selected"},
    {":top", "Item is positioned at top"},
    {":unchecked", "Item is unchecked"},
    {":vertical", "Item has vertical orientation"},
    {":window", "Widget is a top-level window"}};

// Subcontrols
inline const QVector<QPair<QString, QString>> SUBCONTROLS = {
    {"::add-line", "Button to add a line (QScrollBar)"},
    {"::add-page", "Region between handle and add-line (QScrollBar)"},
    {"::branch", "Branch indicator (QTreeView)"},
    {"::chunk", "Progress chunk (QProgressBar)"},
    {"::close-button", "Close button (QDockWidget, QTabBar)"},
    {"::corner", "Corner between scrollbars (QAbstractScrollArea)"},
    {"::down-arrow", "Down arrow (QComboBox, QHeaderView, QScrollBar, QSpinBox)"},
    {"::down-button", "Down button (QScrollBar, QSpinBox)"},
    {"::drop-down", "Drop-down button (QComboBox)"},
    {"::float-button", "Float button (QDockWidget)"},
    {"::groove", "Groove of QSlider"},
    {"::indicator", "Indicator (QCheckBox, QRadioButton, QMenu item, QGroupBox)"},
    {"::handle", "Handle (QScrollBar, QSplitter, QSlider)"},
    {"::icon", "Icon (QAbstractItemView, QMenu)"},
    {"::item", "Item (QAbstractItemView, QMenuBar, QMenu, QStatusBar)"},
    {"::left-arrow", "Left arrow (QScrollBar)"},
    {"::left-corner", "Left corner (QTabWidget)"},
    {"::menu-arrow", "Arrow of a QToolButton with menu"},
    {"::menu-button", "Menu button of a QToolButton"},
    {"::menu-indicator", "Menu indicator (QPushButton)"},
    {"::right-arrow", "Right arrow (QMenu, QScrollBar)"},
    {"::pane", "Pane/frame (QTabWidget)"},
    {"::right-corner", "Right corner (QTabWidget)"},
    {"::scroller", "Scroller (QMenu, QTabBar)"},
    {"::section", "Section (QHeaderView)"},
    {"::separator", "Separator (QMenu, QMainWindow)"},
    {"::sub-line", "Subtract line button (QScrollBar)"},
    {"::sub-page", "Region between handle and sub-line (QScrollBar)"},
    {"::tab", "Tab (QTabBar, QToolBox)"},
    {"::tab-bar", "Tab bar (QTabWidget)"},
    {"::tear", "Tear indicator (QTabBar)"},
    {"::tearoff", "Tear-off indicator (QMenu)"},
    {"::text", "Text (QAbstractItemView)"},
    {"::title", "Title (QGroupBox, QDockWidget)"},
    {"::up-arrow", "Up arrow (QHeaderView, QScrollBar, QSpinBox)"},
    {"::up-button", "Up button (QSpinBox)"}};

inline const QHash<QString, QStringList> WIDGET_VALID_PSEUDOS = {
    {"QLabel", {":hover", ":disabled", ":enabled"}},
    {"QCheckBox", {":checked", ":unchecked", ":disabled", ":enabled", ":hover", ":focus"}},
    {"QRadioButton", {":checked", ":unchecked", ":disabled", ":enabled", ":hover", ":focus"}},
    {"QPushButton",
     {":default", ":flat", ":checked", ":open", ":closed", ":hover", ":disabled", ":enabled", ":focus", ":pressed"}},
    {"QScrollBar", {":horizontal", ":vertical", ":hover", ":disabled", ":enabled", ":focus"}},
    {"QSlider", {":horizontal", ":vertical", ":hover", ":disabled", ":enabled", ":focus"}},
    {"QTabBar",
     {":first", ":last", ":middle", ":only-one", ":previous-selected", ":next-selected", ":selected", ":top", ":bottom",
      ":left", ":right", ":hover", ":disabled", ":enabled", ":focus"}},
    {"QDockWidget", {":closable", ":floatable", ":movable", ":vertical", ":hover", ":disabled", ":enabled", ":focus"}},
    {"QTreeView", {":open", ":closed", ":has-children", ":has-siblings", ":hover", ":disabled", ":enabled", ":focus"}},
    {"QAbstractScrollArea", {":hover", ":disabled", ":enabled", ":focus"}},
    {"QLineEdit", {":hover", ":disabled", ":enabled", ":focus", ":read-only"}},
    {"QComboBox", {":editable", ":hover", ":disabled", ":enabled", ":focus", ":open", ":closed"}},
    {"QHeaderView",
     {":middle", ":first", ":last", ":only-one", ":next-selected", ":previous-selected", ":selected", ":checked"}},
    {"QToolBar", {":top", ":left", ":right", ":bottom", ":first", ":last", ":middle", ":only-one"}},
    {"QGroupBox", {":checked", ":unchecked", ":disabled", ":enabled", ":hover", ":focus"}},
    {"QMenu", {":selected", ":default", ":exclusive", ":non-exclusive", ":hover", ":disabled", ":enabled", ":focus"}},
    {"QMenuBar", {":hover", ":disabled", ":enabled", ":focus"}},
    {"QTabWidget", {":top", ":bottom", ":left", ":right"}},
    {"QSpinBox", {":hover", ":disabled", ":enabled", ":focus"}},
    {"QDoubleSpinBox", {":hover", ":disabled", ":enabled", ":focus"}},
    {"QToolButton", {":hover", ":disabled", ":enabled", ":focus", ":pressed"}},
    {"QToolBox", {":only-one", ":first", ":last", ":middle", ":previous-selected", ":next-selected", ":selected"}}};

inline const QHash<QString, QStringList> WIDGET_VALID_SUBCONTROLS = {
    {"QCheckBox", {"::indicator"}},
    {"QRadioButton", {"::indicator"}},
    {"QPushButton", {"::menu-indicator"}},
    {"QToolButton",
     {"::menu-indicator", "::menu-button", "::menu-arrow", "::up-arrow", "::down-arrow", "::left-arrow",
      "::right-arrow"}},
    {"QScrollBar",
     {"::handle", "::add-line", "::sub-line", "::add-page", "::sub-page", "::up-arrow", "::down-arrow", "::left-arrow",
      "::right-arrow"}},
    {"QSlider", {"::groove", "::handle"}},
    {"QTabBar", {"::tab", "::close-button", "::tear", "::scroller"}},
    {"QTabWidget", {"::pane", "::tab-bar", "::left-corner", "::right-corner"}},
    {"QDockWidget", {"::title", "::close-button", "::float-button"}},
    {"QGroupBox", {"::title", "::indicator"}},
    {"QHeaderView", {"::section", "::up-arrow", "::down-arrow"}},
    {"QMenu", {"::item", "::indicator", "::separator", "::scroller", "::tearoff"}},
    {"QMenuBar", {"::item"}},
    {"QSpinBox", {"::up-button", "::down-button", "::up-arrow", "::down-arrow"}},
    {"QDoubleSpinBox", {"::up-button", "::down-button", "::up-arrow", "::down-arrow"}},
    {"QToolBar", {"::separator", "::handle"}},
    {"QTableView", {"::item", "::section"}},
    {"QListView", {"::item"}},
    {"QTreeView", {"::branch", "::item"}},
    {"QAbstractScrollArea", {"::corner"}},
    {"QProgressBar", {"::chunk"}},
    {"QComboBox", {"::drop-down", "::down-arrow"}}};

inline const QVector<QPair<QString, QString>> QT_CSS_PROPERTIES = {
    {"alternate-background-color", "Brush: Alternate background for QAbstractItemView"},
    {"background", "Shorthand: background-color, image, repeat, position"},
    {"background-color", "Brush: Background color"},
    {"background-image", "Url: Background image"},
    {"background-repeat", "Repeat: Image repeat"},
    {"background-position", "Alignment: Background image alignment"},
    {"background-attachment", "Attachment: Fixed or scroll background"},
    {"background-clip", "Origin: Rectangle where background is drawn"},
    {"background-origin", "Origin: Rectangle for background positioning"},
    {"border", "Shorthand: border-width, border-style, border-color"},
    {"border-top", "Shorthand: top border"},
    {"border-right", "Shorthand: right border"},
    {"border-bottom", "Shorthand: bottom border"},
    {"border-left", "Shorthand: left border"},
    {"border-color", "Box Colors: color of all border edges"},
    {"border-top-color", "Brush: Top edge color"},
    {"border-right-color", "Brush: Right edge color"},
    {"border-bottom-color", "Brush: Bottom edge color"},
    {"border-left-color", "Brush: Left edge color"},
    {"border-image", "Border Image: Image to fill border"},
    {"border-radius", "Radius: Corner radius"},
    {"border-top-left-radius", "Radius: Top-left corner radius"},
    {"border-top-right-radius", "Radius: Top-right corner radius"},
    {"border-bottom-right-radius", "Radius: Bottom-right corner radius"},
    {"border-bottom-left-radius", "Radius: Bottom-left corner radius"},
    {"border-style", "Border Style: Style of all edges"},
    {"border-top-style", "Border Style: Top edge style"},
    {"border-right-style", "Border Style: Right edge style"},
    {"border-bottom-style", "Border Style: Bottom edge style"},
    {"border-left-style", "Border Style: Left edge style"},
    {"border-width", "Box Lengths: Width of all edges"},
    {"border-top-width", "Length: Width of top edge"},
    {"border-right-width", "Length: Width of right edge"},
    {"border-bottom-width", "Length: Width of bottom edge"},
    {"border-left-width", "Length: Width of left edge"},
    {"bottom", "Length: Move subcontrol from bottom"},
    {"button-layout", "Number: Layout of buttons in QDialogButtonBox"},
    {"color", "Brush: Text color"},
    {"dialogbuttonbox-buttons-have-icons", "Boolean: Show icons in QDialogButtonBox"},
    {"font", "Font: Shorthand for font-family, size, style, weight"},
    {"font-family", "String: Font family"},
    {"font-size", "Font Size: Font size in px or pt"},
    {"font-style", "Font Style: italic, normal, etc."},
    {"font-weight", "Font Weight: bold, normal, etc."},
    {"gridline-color", "Color: QTableView grid line color"},
    {"height", "Length: Height of a subcontrol or widget"},
    {"icon", "Url+: Icon for QPushButton"},
    {"icon-size", "Length: Width and height of icon"},
    {"image", "Url+: Image for subcontrol contents"},
    {"image-position", "Alignment: Image alignment in subcontrol"},
    {"left", "Length: Offset from left"},
    {"lineedit-password-character", "Number: Password character unicode"},
    {"lineedit-password-mask-delay", "Number: Delay for password masking"},
    {"margin", "Box Lengths: Widget margins"},
    {"margin-top", "Length: Top margin"},
    {"margin-right", "Length: Right margin"},
    {"margin-bottom", "Length: Bottom margin"},
    {"margin-left", "Length: Left margin"},
    {"max-height", "Length: Maximum height"},
    {"max-width", "Length: Maximum width"},
    {"messagebox-text-interaction-flags", "Number: Text interaction in QMessageBox"},
    {"min-height", "Length: Minimum height"},
    {"min-width", "Length: Minimum width"},
    {"opacity", "Number: Opacity 0-255"},
    {"outline", "Outline around border"},
    {"outline-color", "Outline color"},
    {"outline-offset", "Offset of outline"},
    {"outline-style", "Outline pattern"},
    {"outline-radius", "Rounded corners of outline"},
    {"outline-bottom-left-radius", "Outline bottom-left radius"},
    {"outline-bottom-right-radius", "Outline bottom-right radius"},
    {"outline-top-left-radius", "Outline top-left radius"},
    {"outline-top-right-radius", "Outline top-right radius"},
    {"padding", "Box Lengths: Padding inside widget"},
    {"padding-top", "Top padding"},
    {"padding-right", "Right padding"},
    {"padding-bottom", "Bottom padding"},
    {"padding-left", "Left padding"},
    {"paint-alternating-row-colors-for-empty-area", "Boolean: Paint alternating rows for empty area"},
    {"position", "relative|absolute: Subcontrol positioning"},
    {"right", "Length: Offset from right"},
    {"selection-background-color", "Brush: Background of selected text/items"},
    {"selection-color", "Brush: Foreground of selected text/items"},
    {"show-decoration-selected", "Boolean: Full row selection in QListView"},
    {"spacing", "Length: Internal spacing"},
    {"subcontrol-origin", "Origin rectangle of subcontrol"},
    {"subcontrol-position", "Alignment: Position within subcontrol-origin"},
    {"titlebar-show-tooltips-on-buttons", "Boolean: Show tooltips on titlebar buttons"},
    {"widget-animation-duration", "Number: Animation duration in ms"},
    {"text-align", "Alignment: Text/icon alignment (QPushButton/QProgressBar)"},
    {"text-decoration", "none|underline|overline|line-through: Text effects"},
    {"top", "Length: Offset from top"},
    {"width", "Length: Width of subcontrol or widget"}};

static const QMap<QString, QStringList> qtPropertyMap{
    {"height", {"QWidget"}},
    {"width", {"QWidget"}},
    {"min-height",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSizeGrip", "QSpinBox", "QSplitter", "QStatusBar",
      "QTextEdit", "QToolButton"}},
    {"min-width",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSizeGrip", "QSpinBox", "QSplitter", "QStatusBar",
      "QTextEdit", "QToolButton"}},
    {"max-height",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSizeGrip", "QSpinBox", "QSplitter", "QStatusBar",
      "QTextEdit", "QToolTip"}},
    {"max-width",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSizeGrip", "QSpinBox", "QSplitter", "QStatusBar",
      "QTextEdit", "QToolTip"}},

    // Background properties
    {"background",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QDialog", "QFrame", "QGroupBox", "QLabel",
      "QLineEdit", "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip",
      "QWidget"}},
    {"background-color",
     {"QWidget", "QLabel", "QLineEdit", "QFrame", "QPushButton", "QTextEdit", "QCheckBox", "QRadioButton"}},
    {"background-image", {"QWidget"}},
    {"background-repeat", {"QWidget"}},
    {"background-position", {"QWidget"}},
    {"background-attachment", {"QAbstractScrollArea"}},
    {"background-clip",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QDialog", "QFrame", "QGroupBox", "QLabel",
      "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip", "QWidget"}},
    {"background-origin",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QDialog", "QFrame", "QGroupBox", "QLabel",
      "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip", "QWidget"}},

    // Borders
    {"border",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip", "QWidget"}},
    {"border-color",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip", "QWidget"}},
    {"border-radius",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip"}},
    {"border-style",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip"}},
    {"border-width",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip"}},
    {"border-image",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip"}},

    // Text & Font
    {"color", {"QWidget"}},
    {"font", {"QWidget"}},
    {"font-family", {"QWidget"}},
    {"font-size", {"QWidget"}},
    {"font-style", {"QWidget"}},
    {"font-weight", {"QWidget"}},

    // Padding & Margin
    {"padding",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip"}},
    {"margin",
     {"QAbstractItemView", "QAbstractSpinBox", "QCheckBox", "QComboBox", "QFrame", "QGroupBox", "QLabel", "QLineEdit",
      "QMenu", "QMenuBar", "QPushButton", "QRadioButton", "QSplitter", "QTextEdit", "QToolTip"}},

    // Selection
    {"selection-background-color", {"QWidget"}},
    {"selection-color", {"QWidget"}},

    // Special Qt-only
    {"gridline-color", {"QTableView"}},
    {"lineedit-password-character", {"QLineEdit"}},
    {"lineedit-password-mask-delay", {"QLineEdit"}},
    {"icon", {"QPushButton"}},
    {"icon-size",
     {"QCheckBox", "QListView", "QPushButton", "QRadioButton", "QTabBar", "QToolBar", "QToolBox", "QTreeView"}},
    {"image", {"QWidget"}},
    {"image-position", {"QWidget"}},
    {"spacing", {"QCheckBox", "QGroupBox", "QMenuBar", "QRadioButton"}},
    {"subcontrol-origin", {"QWidget"}},
    {"subcontrol-position", {"QWidget"}},
    {"text-align", {"QPushButton", "QProgressBar"}},
    {"text-decoration", {"QWidget"}},
    {"widget-animation-duration", {"QWidget"}},
    {"opacity", {"QToolTip"}},
    {"outline", {"QWidget"}},
    {"outline-color", {"QWidget"}},
    {"outline-offset", {"QWidget"}},
    {"outline-style", {"QWidget"}},
    {"outline-radius", {"QWidget"}},
    {"outline-top-left-radius", {"QWidget"}},
    {"outline-top-right-radius", {"QWidget"}},
    {"outline-bottom-left-radius", {"QWidget"}},
    {"outline-bottom-right-radius", {"QWidget"}}};

// Returns basic grouped selectors for a widget
inline SelectorGroups possibleSelectorsGrouped(QWidget *w)
{
    SelectorGroups g;
    if (!w)
        return g;

    const QString type = w->metaObject()->className();
    g.types << type;

    if (!w->objectName().isEmpty()) {
        g.objects << "#" + w->objectName();
        g.objects << type + "#" + w->objectName();
    }

    // Only valid pseudos for this widget
    g.pseudos = QtSelectors::WIDGET_VALID_PSEUDOS.value(type);

    // Only valid subcontrols
    g.subControls = QtSelectors::WIDGET_VALID_SUBCONTROLS.value(type);

    return g;
}

} // namespace QtSelectors

#endif // COCKATRICE_QT_SELECTORS_H
