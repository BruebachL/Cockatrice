#include "theme_inspector_widget.h"

#include "selector_dialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSet>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

// ===========================================================
// Constructor
// ===========================================================
ThemeInspectorWidget::ThemeInspectorWidget(const QString &liveCssPath, QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Theme Inspector - Visual Stylesheet Editor");
    setMinimumSize(1400, 800);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Add toolbar first
    mainLayout->addWidget(createToolbar());

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setObjectName("mainSplitter");

    // ---------------- Widget Tree ----------------
    auto *treeGroup = new QGroupBox("Application Widget Tree");
    auto *treeLayout = new QVBoxLayout(treeGroup);
    treeLayout->setContentsMargins(4, 8, 4, 4);

    widgetTree = new QTreeWidget;
    widgetTree->setHeaderLabels({"Widget / Object Name", "Type"});
    widgetTree->setColumnWidth(0, 350);
    widgetTree->setUniformRowHeights(true);
    widgetTree->setSelectionMode(QAbstractItemView::SingleSelection);
    widgetTree->setAlternatingRowColors(true);
    treeLayout->addWidget(widgetTree);

    mainSplitter->addWidget(treeGroup);

    auto *bottom = new QSplitter(Qt::Horizontal);
    bottom->setObjectName("bottomSplitter");

    // ---------------- Left panel ----------------
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    auto *infoGroup = new QGroupBox("Widget Details");
    auto *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setContentsMargins(4, 8, 4, 4);

    widgetInfo = new QPlainTextEdit;
    widgetInfo->setReadOnly(true);
    widgetInfo->setMinimumHeight(120);
    widgetInfo->setMaximumHeight(180);
    widgetInfo->setPlaceholderText("Select a widget from the tree to see its properties...");
    infoLayout->addWidget(widgetInfo);
    leftLayout->addWidget(infoGroup);

    auto *suggestGroup = new QGroupBox("How to Target This Widget");
    auto *suggestLayout = new QVBoxLayout(suggestGroup);
    suggestLayout->setContentsMargins(4, 8, 4, 4);

    selectorSuggestionList = new QListWidget;
    selectorSuggestionList->setToolTip("Double-click any selector to create a new styling rule");
    suggestLayout->addWidget(selectorSuggestionList);
    leftLayout->addWidget(suggestGroup);

    bottom->addWidget(left);

    // ---------------- Right panel ----------------
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    auto *tabs = new QTabWidget;

    // Tab 1: Affecting Rules
    auto *rulesTab = new QWidget;
    auto *rulesTabLayout = new QVBoxLayout(rulesTab);
    rulesTabLayout->setContentsMargins(4, 4, 4, 4);

    ruleTree = new QTreeWidget;
    ruleTree->setHeaderLabels({"Selector", "Line"});
    ruleTree->setColumnWidth(0, 300);
    ruleTree->setSelectionMode(QAbstractItemView::SingleSelection);
    ruleTree->setToolTip("Rules affecting the selected widget (click to edit)");
    ruleTree->setAlternatingRowColors(true);
    rulesTabLayout->addWidget(ruleTree);

    tabs->addTab(rulesTab, "Affecting Rules");

    // Tab 2: All Rules
    auto *allRulesTab = new QWidget;
    auto *allRulesTabLayout = new QVBoxLayout(allRulesTab);
    allRulesTabLayout->setContentsMargins(4, 4, 4, 4);

    allRulesTree = new QTreeWidget;
    allRulesTree->setHeaderLabels({"Selector", "Line", "Widgets"});
    allRulesTree->setColumnWidth(0, 300);
    allRulesTree->setSortingEnabled(true);
    allRulesTree->setSelectionMode(QAbstractItemView::SingleSelection);
    allRulesTree->setToolTip("All CSS rules (click to edit)");
    allRulesTree->setAlternatingRowColors(true);
    allRulesTabLayout->addWidget(allRulesTree);

    tabs->addTab(allRulesTab, "All Rules");

    // Tab 3: Object Names Reference
    auto *objectNamesTab = new QWidget;
    auto *objectNamesLayout = new QVBoxLayout(objectNamesTab);
    objectNamesLayout->setContentsMargins(4, 4, 4, 4);

    objectNamesTree = new QTreeWidget;
    objectNamesTree->setHeaderLabels({"Object Name", "Widget Type", "Path"});
    objectNamesTree->setColumnWidth(0, 200);
    objectNamesTree->setColumnWidth(1, 150);
    objectNamesTree->setSelectionMode(QAbstractItemView::SingleSelection);
    objectNamesTree->setAlternatingRowColors(true);
    objectNamesTree->setSortingEnabled(true);
    objectNamesTree->setToolTip("Named widgets (use #objectName in CSS). Double-click to locate.");
    objectNamesLayout->addWidget(objectNamesTree);

    connect(objectNamesTree, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem *item) {
        if (!item)
            return;
        QWidget *w = qobject_cast<QWidget *>(item->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w) {
            // Find and select in main tree
            QTreeWidgetItemIterator it(widgetTree);
            while (*it) {
                QWidget *tw = qobject_cast<QWidget *>((*it)->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
                if (tw == w) {
                    widgetTree->setCurrentItem(*it);
                    widgetTree->scrollToItem(*it);
                    updateForSelection();
                    return;
                }
                ++it;
            }
        }
    });

    tabs->addTab(objectNamesTab, "Object Names");

    // Tab 4: Icons Reference
    auto *iconsTab = new QWidget;
    auto *iconsLayout = new QVBoxLayout(iconsTab);
    iconsLayout->setContentsMargins(4, 4, 4, 4);

    iconsTree = new QTreeWidget;
    iconsTree->setHeaderLabels({"Icon Preview", "Widget", "Object Name"});
    iconsTree->setColumnWidth(0, 150);
    iconsTree->setColumnWidth(1, 150);
    iconsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    iconsTree->setAlternatingRowColors(true);
    iconsTree->setIconSize(QSize(24, 24));
    iconsTree->setToolTip("Icons in your app (use icon: url(...) in CSS)");
    iconsLayout->addWidget(iconsTree);

    tabs->addTab(iconsTab, "Icons");

    rightLayout->addWidget(tabs);

    // Rule editor section
    auto *editorGroup = new QGroupBox("CSS Editor");
    auto *editorGroupLayout = new QVBoxLayout(editorGroup);
    editorGroupLayout->setContentsMargins(4, 8, 4, 4);

    ruleEditor = new QPlainTextEdit;
    ruleEditor->setPlaceholderText("Select a rule to edit its CSS properties...\n\nExample:\n    background-color: "
                                   "#2b2b2b;\n    color: white;\n    border: 1px solid #555;");
    ruleEditor->setMinimumHeight(150);
    editorGroupLayout->addWidget(ruleEditor);

    // ---------------- Property suggestions ----------------
    auto *propLabel = new QLabel("Quick Insert Properties:");
    propLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    editorGroupLayout->addWidget(propLabel);

    propertySuggestionList = new QListWidget;
    propertySuggestionList->setMaximumHeight(100);
    propertySuggestionList->setToolTip("Click to insert a CSS property template");
    propertySuggestionList->setSelectionMode(QAbstractItemView::SingleSelection);
    editorGroupLayout->addWidget(propertySuggestionList);

    connect(propertySuggestionList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        if (!item || ruleEditor->toPlainText().isEmpty())
            return;

        QString prop = item->text().split(" ").first();

        // Insert at cursor position
        QTextCursor cursor = ruleEditor->textCursor();

        // Add proper formatting with helpful placeholder
        QString insertion = QString("    %1: /* value */;\n").arg(prop);
        cursor.insertText(insertion);

        // Select the placeholder
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 13);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 9);
        ruleEditor->setTextCursor(cursor);
        ruleEditor->setFocus();
    });

    auto *buttonLayout = new QHBoxLayout;

    auto *apply = new QPushButton("Save & Apply");
    apply->setToolTip("Save changes to stylesheet and reload the theme");
    connect(apply, &QPushButton::clicked, this, &ThemeInspectorWidget::applyRuleEdit);
    buttonLayout->addWidget(apply);

    auto *revert = new QPushButton("Revert");
    revert->setToolTip("Reload stylesheet from disk, discarding unsaved changes");
    connect(revert, &QPushButton::clicked, this, &ThemeInspectorWidget::reloadStylesheet);
    buttonLayout->addWidget(revert);

    buttonLayout->addStretch();
    editorGroupLayout->addLayout(buttonLayout);

    rightLayout->addWidget(editorGroup);

    bottom->addWidget(right);

    // Set splitter ratios
    bottom->setStretchFactor(0, 1);
    bottom->setStretchFactor(1, 2);

    mainSplitter->addWidget(bottom);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);

    // ---------------- Main layout ----------------
    mainLayout->addWidget(mainSplitter);
    setLayout(mainLayout);

    // ---------------- Connections ----------------
    connect(widgetTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::updateForSelection);
    connect(ruleTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBody);
    connect(allRulesTree, &QTreeWidget::itemSelectionChanged, this, &ThemeInspectorWidget::showRuleBodyFromAllRules);

    // Double-click on selector suggestion creates a rule
    connect(selectorSuggestionList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (!item)
            return;

        auto items = widgetTree->selectedItems();
        if (items.isEmpty())
            return;

        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w)
            return;

        QString fullText = item->text();
        QString selector = fullText.split("\n").first();

        addRuleForWidget(w, selector, "    /* Add your styles here */\n    background-color: ;\n    color: ;");
    });

    rebuildTree();
    setStylesheetPath(liveCssPath);
}

// ===========================================================
// Toolbar
// ===========================================================
QWidget *ThemeInspectorWidget::createToolbar()
{
    auto *bar = new QWidget;
    bar->setObjectName("toolbar");
    bar->setStyleSheet("#toolbar { background-color: #f0f0f0; border-bottom: 2px solid #ccc; } "
                       "QToolButton { background-color: white; border: 1px solid #ccc; border-radius: 3px; padding: "
                       "6px 12px; color: black; } "
                       "QToolButton:hover { background-color: #e8e8e8; } "
                       "QToolButton:checked { background-color: #d0d0ff; border-color: #8888ff; } "
                       "QLabel { color: black; }");
    auto *l = new QHBoxLayout(bar);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(12);

    hideInspectorCheckbox = new QToolButton;
    hideInspectorCheckbox->setText("Hide Inspector from Tree");
    hideInspectorCheckbox->setCheckable(true);
    hideInspectorCheckbox->setChecked(true);
    hideInspectorCheckbox->setToolTip("When checked, the Theme Inspector itself won't appear in the widget tree");
    connect(hideInspectorCheckbox, &QToolButton::toggled, this, &ThemeInspectorWidget::rebuildTree);
    l->addWidget(hideInspectorCheckbox);

    l->addWidget(createSeparator());

    auto *refresh = new QToolButton;
    refresh->setText("↻ Refresh");
    refresh->setToolTip("Refresh the widget tree to see any new widgets");
    connect(refresh, &QToolButton::clicked, this, &ThemeInspectorWidget::rebuildTree);
    l->addWidget(refresh);

    auto *expand = new QToolButton;
    expand->setText("Expand All");
    expand->setToolTip("Expand all items in the widget tree");
    connect(expand, &QToolButton::clicked, this, &ThemeInspectorWidget::expandAllItems);
    l->addWidget(expand);

    auto *collapse = new QToolButton;
    collapse->setText("Collapse All");
    collapse->setToolTip("Collapse all items in the widget tree");
    connect(collapse, &QToolButton::clicked, this, &ThemeInspectorWidget::collapseAllItems);
    l->addWidget(collapse);

    l->addWidget(createSeparator());

    auto *addWidgetRule = new QToolButton;
    addWidgetRule->setText("+ New Rule for Widget");
    addWidgetRule->setToolTip("Create a new CSS rule for the selected widget");
    connect(addWidgetRule, &QToolButton::clicked, [this]() {
        auto items = widgetTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Widget Selected",
                                     "Please select a widget from the tree first.\n\n"
                                     "Tip: Click on any widget in the tree to select it.");
            return;
        }

        QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (!w) {
            QMessageBox::warning(this, "Invalid Selection", "The selected item is not a valid widget.");
            return;
        }

        QtSelectors::SelectorGroups g = QtSelectors::possibleSelectorsGrouped(w);
        SelectorDialog dlg(g.types, g.objects, g.pseudos, this);
        dlg.setWindowTitle("Choose Selector for Widget Rule");

        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForWidget(w, sel, "    /* Widget-specific styles */\n    background-color: ;\n    color: ;");
        }
    });
    l->addWidget(addWidgetRule);

    auto *addGlobalRule = new QToolButton;
    addGlobalRule->setText("+ New Global Rule");
    addGlobalRule->setToolTip("Create a new CSS rule that applies globally");
    connect(addGlobalRule, &QToolButton::clicked, [this]() {
        updateGlobalTypes();
        SelectorDialog dlg(globalTypes, globalObjects, globalPseudos, this);
        dlg.setWindowTitle("Choose Selector for Global Rule");

        if (dlg.exec() == QDialog::Accepted) {
            const QString sel = dlg.selectedSelector();
            if (!sel.isEmpty())
                addRuleForAll(sel, "    /* Global styles */\n    background-color: ;\n    color: ;");
        }
    });
    l->addWidget(addGlobalRule);

    auto *delRule = new QToolButton;
    delRule->setText("Delete Rule");
    delRule->setToolTip("Delete the selected rule from the stylesheet");
    connect(delRule, &QToolButton::clicked, this, &ThemeInspectorWidget::deleteSelectedRule);
    l->addWidget(delRule);

    l->addWidget(createSeparator());

    auto *editSelector = new QToolButton;
    editSelector->setText("Edit Selector");
    editSelector->setToolTip("Change the selector for the selected rule");
    connect(editSelector, &QToolButton::clicked, this, &ThemeInspectorWidget::editSelectedRuleSelector);
    l->addWidget(editSelector);

    auto *moveUp = new QToolButton;
    moveUp->setText("↑ Move Up");
    moveUp->setToolTip("Move rule earlier (lower priority in Qt CSS)");
    connect(moveUp, &QToolButton::clicked, [this]() { moveSelectedRule(-1); });
    l->addWidget(moveUp);

    auto *moveDown = new QToolButton;
    moveDown->setText("↓ Move Down");
    moveDown->setToolTip("Move rule later (HIGHER priority - will override earlier rules)");
    connect(moveDown, &QToolButton::clicked, [this]() { moveSelectedRule(1); });
    l->addWidget(moveDown);

    l->addStretch();

    // Status label
    statusLabel = new QLabel("Ready");
    statusLabel->setStyleSheet("color: #666; padding: 4px;");
    l->addWidget(statusLabel);

    return bar;
}

QFrame *ThemeInspectorWidget::createSeparator()
{
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setStyleSheet("margin: 0px 4px;");
    return sep;
}

// ===========================================================
// Global selector collection
// ===========================================================
void ThemeInspectorWidget::updateGlobalTypes()
{
    globalTypes.clear();
    globalObjects.clear();
    globalPseudos = {":hover", ":disabled", ":focus", ":enabled", ":checked", ":unchecked"};

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets())
        collectAllWidgetsRecursive(w, widgets);

    QSet<QString> types;
    QSet<QString> objects;

    for (QWidget *w : widgets) {
        types.insert(w->metaObject()->className());
        if (!w->objectName().isEmpty())
            objects.insert("#" + w->objectName());
    }

    globalTypes = QStringList(types.begin(), types.end());
    globalObjects = QStringList(objects.begin(), objects.end());

    std::sort(globalTypes.begin(), globalTypes.end());
    std::sort(globalObjects.begin(), globalObjects.end());
}

// ===========================================================
// Tree expand / collapse
// ===========================================================
void ThemeInspectorWidget::expandAllItems()
{
    widgetTree->expandAll();
    updateStatus("Expanded all widget tree items");
}

void ThemeInspectorWidget::collapseAllItems()
{
    widgetTree->collapseAll();
    updateStatus("Collapsed all widget tree items");
}

void ThemeInspectorWidget::expandItemRecursive(QTreeWidgetItem *item)
{
    if (!item)
        return;
    widgetTree->expandItem(item);
    for (int i = 0; i < item->childCount(); ++i)
        expandItemRecursive(item->child(i));
}

void ThemeInspectorWidget::collapseItemRecursive(QTreeWidgetItem *item)
{
    if (!item)
        return;
    widgetTree->collapseItem(item);
    for (int i = 0; i < item->childCount(); ++i)
        collapseItemRecursive(item->child(i));
}

// ===========================================================
// Widget tree
// ===========================================================
void ThemeInspectorWidget::rebuildTree()
{
    widgetTree->clear();

    int totalWidgets = 0;
    QList<QWidget *> topLevelWindows;

    // Separate main windows from other top-level widgets
    for (QWidget *top : QApplication::topLevelWidgets()) {
        if (hideInspectorCheckbox && hideInspectorCheckbox->isChecked()) {
            // Skip the inspector and its children
            if (top == this || isChildOf(top, this))
                continue;
        }

        if (qobject_cast<QMainWindow *>(top) || top->isWindow()) {
            topLevelWindows.append(top);
        }
    }

    // Sort: QMainWindow instances first
    std::sort(topLevelWindows.begin(), topLevelWindows.end(), [](QWidget *a, QWidget *b) {
        bool aIsMain = qobject_cast<QMainWindow *>(a) != nullptr;
        bool bIsMain = qobject_cast<QMainWindow *>(b) != nullptr;
        if (aIsMain != bIsMain)
            return aIsMain; // Main windows first
        return false;
    });

    for (QWidget *top : topLevelWindows) {
        QString objectName = top->objectName().isEmpty() ? "(unnamed)" : top->objectName();
        QString type = top->metaObject()->className();

        auto *root = new QTreeWidgetItem(widgetTree);
        root->setText(0, objectName);
        root->setText(1, type);
        root->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(top)));

        // Make main windows bold and blue
        if (qobject_cast<QMainWindow *>(top)) {
            QFont font = root->font(0);
            font.setBold(true);
            root->setFont(0, font);
            root->setFont(1, font);
            root->setForeground(0, QBrush(QColor(50, 100, 200)));
            root->setForeground(1, QBrush(QColor(50, 100, 200)));
        }

        totalWidgets += addChildrenToItem(top, root) + 1;
    }

    widgetTree->expandToDepth(1);
    rebuildAllRulesTree();
    rebuildObjectNamesTree();
    rebuildIconsTree();

    updateStatus(QString("Loaded %1 widgets from application").arg(totalWidgets));
}

bool ThemeInspectorWidget::isChildOf(QWidget *widget, QWidget *potentialParent) const
{
    QWidget *parent = widget;
    while (parent) {
        if (parent == potentialParent)
            return true;
        parent = parent->parentWidget();
    }
    return false;
}

int ThemeInspectorWidget::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    int count = 0;
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c)) {
            QString objectName = cw->objectName().isEmpty() ? "(unnamed)" : cw->objectName();
            QString type = cw->metaObject()->className();

            auto *it = new QTreeWidgetItem(parent);
            it->setText(0, objectName);
            it->setText(1, type);
            it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(cw)));

            // Highlight named widgets
            if (!cw->objectName().isEmpty()) {
                QFont font = it->font(0);
                font.setBold(true);
                it->setFont(0, font);
                it->setForeground(0, QBrush(QColor(100, 150, 255)));
            }

            count += addChildrenToItem(cw, it) + 1;
        }
    }
    return count;
}

// ===========================================================
// Selection & Summary
// ===========================================================
void ThemeInspectorWidget::updateForSelection()
{
    widgetInfo->clear();
    selectorSuggestionList->clear();
    ruleTree->clear();
    ruleEditor->clear();
    propertySuggestionList->clear();

    auto items = widgetTree->selectedItems();
    if (items.isEmpty())
        return;

    QWidget *w = qobject_cast<QWidget *>(items.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
    if (!w)
        return;

    widgetInfo->setPlainText(widgetSummary(w));

    auto suggestions = selectorSuggestions(w);
    for (const auto &s : suggestions) {
        QString displayText = QString("%1\n  ➜ %2").arg(s.selector, s.explanation);
        auto *item = new QListWidgetItem(displayText);
        item->setToolTip("Double-click to create a new CSS rule with this selector");
        selectorSuggestionList->addItem(item);
    }

    // Populate property suggestions based on widget type
    const QMetaObject *mo = w->metaObject();
    QSet<QString> suggestedProps;

    while (mo) {
        QString typeName = mo->className();
        for (auto it = QtSelectors::qtPropertyMap.constBegin(); it != QtSelectors::qtPropertyMap.constEnd(); ++it) {
            if (it.value().contains(typeName))
                suggestedProps.insert(it.key());
        }
        mo = mo->superClass();
    }

    // Add essential properties for all widgets
    suggestedProps.insert("color");
    suggestedProps.insert("background-color");
    suggestedProps.insert("border");
    suggestedProps.insert("border-radius");
    suggestedProps.insert("padding");
    suggestedProps.insert("margin");
    suggestedProps.insert("font-size");
    suggestedProps.insert("font-weight");

    QStringList sortedProps = QStringList(suggestedProps.begin(), suggestedProps.end());
    std::sort(sortedProps.begin(), sortedProps.end());

    for (const QString &prop : sortedProps) {
        QString hint;
        for (const auto &p : QtSelectors::QT_CSS_PROPERTIES) {
            if (p.first == prop) {
                hint = " - " + p.second.split(":").last().trimmed();
                break;
            }
        }
        propertySuggestionList->addItem(prop + hint);
    }

    updateRuleMatches(w);
}

QString ThemeInspectorWidget::widgetSummary(QWidget *w) const
{
    if (!w)
        return QString();

    QStringList info;
    info << QString("Type: %1").arg(w->metaObject()->className());
    info << QString("Object Name: %1").arg(w->objectName().isEmpty() ? "(none)" : w->objectName());

    if (!w->objectName().isEmpty()) {
        info << "";
        info << "⚠ QT CSS RULE ORDER MATTERS:";
        info << "Unlike web CSS, Qt stylesheets use LAST RULE WINS";
        info << "regardless of specificity! Use ↑↓ buttons to reorder.";
        info << "";
        info << "Best selector for this widget:";
        info << QString("  %1#%2").arg(w->metaObject()->className(), w->objectName());
    }

    info << "";
    info << QString("Path: %1").arg(widgetPath(w));
    info
        << QString("Visible: %1  |  Enabled: %2").arg(w->isVisible() ? "Yes" : "No").arg(w->isEnabled() ? "Yes" : "No");
    info << QString("Size: %1×%2px  |  Position: (%3, %4)").arg(w->width()).arg(w->height()).arg(w->x()).arg(w->y());

    return info.join("\n");
}

QString ThemeInspectorWidget::widgetPath(QWidget *w) const
{
    QStringList parts;
    while (w) {
        QString part = w->objectName().isEmpty() ? w->metaObject()->className() : w->objectName();
        parts.prepend(part);
        w = w->parentWidget();
    }
    return parts.join(" → ");
}

QVector<QtSelectors::SelectorSuggestion> ThemeInspectorWidget::selectorSuggestions(QWidget *w) const
{
    QVector<QtSelectors::SelectorSuggestion> out;
    if (!w)
        return out;

    QString type = w->metaObject()->className();

    // Object name selectors (highest specificity - these WILL override QWidget rules)
    if (!w->objectName().isEmpty()) {
        out.push_back(
            {type + "#" + w->objectName(), QString("Targets this specific %1 (highest specificity)").arg(type), 100});
        out.push_back({"#" + w->objectName(), "Targets by ID only (high specificity)", 95});
    }

    // Type selector
    out.push_back({type, QString("All %1 widgets (overrides parent classes)").arg(type), 80});

    // Only include valid pseudos
    const auto validPseudos = QtSelectors::WIDGET_VALID_PSEUDOS.value(type);
    if (!validPseudos.isEmpty()) {
        for (const auto &p : QtSelectors::PSEUDO_STATES) {
            if (validPseudos.contains(p.first))
                out.push_back({type + p.first, QString("%1 when %2").arg(type, p.second.toLower()), 60});
        }
    }

    // Only include valid subcontrols
    const auto validSubs = QtSelectors::WIDGET_VALID_SUBCONTROLS.value(type);
    if (!validSubs.isEmpty()) {
        for (const auto &s : QtSelectors::SUBCONTROLS) {
            if (validSubs.contains(s.first))
                out.push_back({type + s.first, QString("%1's %2").arg(type, s.second.toLower()), 40});
        }
    }

    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) { return a.confidence > b.confidence; });

    return out;
}

// ===========================================================
// Stylesheet
// ===========================================================
void ThemeInspectorWidget::setStylesheetPath(const QString &path)
{
    watcher.removePaths(watcher.files());
    stylesheetPath = path;

    if (!path.isEmpty()) {
        watcher.addPath(path);
        connect(&watcher, &QFileSystemWatcher::fileChanged, this, &ThemeInspectorWidget::reloadStylesheet,
                Qt::UniqueConnection);
        reloadStylesheet();
    }
}

void ThemeInspectorWidget::reloadStylesheet()
{
    if (stylesheetPath.isEmpty())
        return;

    QFile f(stylesheetPath);
    if (!f.open(QIODevice::ReadOnly)) {
        updateStatus(QString("Error: Could not open stylesheet file"), true);
        return;
    }

    stylesheetText = QString::fromUtf8(f.readAll());
    parseStylesheet();
    rebuildAllRulesTree();

    qApp->setStyleSheet({});
    qApp->setStyleSheet(stylesheetText);

    updateStatus(QString("✓ Loaded %1 CSS rules from stylesheet").arg(rules.size()));

    // Re-add the file to watcher if it was removed
    if (!watcher.files().contains(stylesheetPath))
        watcher.addPath(stylesheetPath);
}

void ThemeInspectorWidget::parseStylesheet()
{
    rules.clear();

    QString text = stylesheetText;
    // Remove C-style comments
    text.remove(QRegularExpression(R"(/\*[\s\S]*?\*/)"));

    QRegularExpression re(R"(([^\{]+)\{([^\}]*)\})");
    int line = 1;

    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        QString body = m.captured(2).trimmed();

        // Split multiple selectors
        for (QString sel : m.captured(1).split(',', Qt::SkipEmptyParts)) {
            rules.push_back({sel.trimmed(), body, line});
        }

        line += m.captured(0).count('\n');
    }
}

// ===========================================================
// Rule logic
// ===========================================================
void ThemeInspectorWidget::updateRuleMatches(QWidget *w)
{
    ruleTree->clear();
    if (!w)
        return;

    int matchCount = 0;
    QVector<int> matchingIndices;

    for (int i = 0; i < rules.size(); ++i) {
        if (selectorMatches(w, rules[i].selector)) {
            matchingIndices.push_back(i);
            matchCount++;
        }
    }

    // Show rules in order of application (last one wins in Qt!)
    for (int idx : matchingIndices) {
        auto *it = new QTreeWidgetItem(ruleTree);
        it->setText(0, rules[idx].selector);
        it->setText(1, QString::number(rules[idx].line));
        it->setData(0, Qt::UserRole, idx);
        it->setToolTip(0, "Click to edit this rule's properties\n\n" + rules[idx].body);

        // Highlight the last matching rule (the one that actually applies)
        if (idx == matchingIndices.last()) {
            QFont font = it->font(0);
            font.setBold(true);
            it->setFont(0, font);
            it->setFont(1, font);
            it->setForeground(0, QBrush(QColor(50, 150, 50)));
            it->setToolTip(0, "✓ THIS RULE WINS (appears last)\n\nClick to edit:\n" + rules[idx].body);
        }
    }

    ruleTree->resizeColumnToContents(0);

    if (matchCount == 0) {
        auto *hint = new QTreeWidgetItem(ruleTree);
        hint->setText(0, "No rules currently affect this widget");
        hint->setForeground(0, QBrush(Qt::gray));
        hint->setFlags(hint->flags() & ~Qt::ItemIsSelectable);
    } else if (matchCount > 1) {
        auto *hint = new QTreeWidgetItem(ruleTree);
        hint->setText(0, QString("⚠ %1 rules match - LAST ONE WINS in Qt CSS!").arg(matchCount));
        hint->setForeground(0, QBrush(QColor(200, 100, 0)));
        hint->setFlags(hint->flags() & ~Qt::ItemIsSelectable);
        ruleTree->insertTopLevelItem(0, hint);
    }
}

// ===========================================================
// Enhanced selector matching
// ===========================================================
bool ThemeInspectorWidget::widgetHasPseudo(QWidget *w, const QString &pseudo) const
{
    if (pseudo.isEmpty())
        return true;

    if (pseudo == "hover")
        return w->underMouse();
    if (pseudo == "disabled")
        return !w->isEnabled();
    if (pseudo == "enabled")
        return w->isEnabled();
    if (pseudo == "focus")
        return w->hasFocus();
    if (pseudo == "checked") {
        if (auto cb = qobject_cast<QCheckBox *>(w))
            return cb->isChecked();
        if (auto rb = qobject_cast<QRadioButton *>(w))
            return rb->isChecked();
        if (auto btn = qobject_cast<QPushButton *>(w))
            return btn->isCheckable() && btn->isChecked();
    }
    if (pseudo == "unchecked") {
        if (auto cb = qobject_cast<QCheckBox *>(w))
            return !cb->isChecked();
        if (auto rb = qobject_cast<QRadioButton *>(w))
            return !rb->isChecked();
        if (auto btn = qobject_cast<QPushButton *>(w))
            return btn->isCheckable() && !btn->isChecked();
    }
    if (pseudo == "visible")
        return w->isVisible();
    if (pseudo == "hidden")
        return !w->isVisible();

    // For other pseudos, assume they match (conservative approach)
    return true;
}

bool ThemeInspectorWidget::widgetHasSubcontrol(QWidget *w, const QString &sub) const
{
    if (sub.isEmpty())
        return true;

    const QString type = w->metaObject()->className();
    if (!QtSelectors::WIDGET_VALID_SUBCONTROLS.contains(type))
        return false;
    return QtSelectors::WIDGET_VALID_SUBCONTROLS[type].contains("::" + sub);
}

bool ThemeInspectorWidget::selectorMatches(QWidget *w, const QString &s) const
{
    if (!w || s.isEmpty())
        return false;

    // Simple regex for basic selector matching
    QRegularExpression re(R"(^(?<type>[\w]+)?(#(?<obj>[\w-]+))?(?::(?<pseudo>[\w-]+))?(?:::(?<sub>[\w-]+))?$)");
    QRegularExpressionMatch m = re.match(s.trimmed());
    if (!m.hasMatch())
        return false;

    QString selType = m.captured("type");
    QString selObj = m.captured("obj");
    QString selPseudo = m.captured("pseudo");
    QString selSub = m.captured("sub");

    // Match type (check inheritance chain)
    if (!selType.isEmpty()) {
        const QMetaObject *mo = w->metaObject();
        bool found = false;
        while (mo) {
            if (selType == mo->className()) {
                found = true;
                break;
            }
            mo = mo->superClass();
        }
        if (!found)
            return false;
    }

    // Match object name
    if (!selObj.isEmpty() && w->objectName() != selObj)
        return false;

    // Match pseudo-state
    if (!widgetHasPseudo(w, selPseudo))
        return false;

    // Match subcontrol
    if (!widgetHasSubcontrol(w, selSub))
        return false;

    return true;
}

// ===========================================================
// Rule editing
// ===========================================================
void ThemeInspectorWidget::showRuleBody()
{
    auto items = ruleTree->selectedItems();
    if (items.isEmpty()) {
        ruleEditor->clear();
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < rules.size())
        ruleEditor->setPlainText(rules[idx].body);
}

void ThemeInspectorWidget::showRuleBodyFromAllRules()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        ruleEditor->clear();
        return;
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < rules.size()) {
        ruleEditor->setPlainText(rules[idx].body);
        highlightMatchingWidget(rules[idx].selector);
    }
}

void ThemeInspectorWidget::applyRuleEdit()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        // Try ruleTree instead
        items = ruleTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Rule Selected",
                                     "Please select a CSS rule to edit.\n\n"
                                     "Tip: Click on any rule in the 'Active Rules' or 'All Rules' tab.");
            return;
        }
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    rules[idx].body = ruleEditor->toPlainText();

    // Reconstruct stylesheet
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Save Error",
                              QString("Could not write to stylesheet file:\n%1\n\n"
                                      "Check file permissions.")
                                  .arg(stylesheetPath));
        return;
    }

    f.write(out.toUtf8());
    f.close();

    updateStatus("✓ Stylesheet saved and applied successfully");

    // Reload will happen automatically via file watcher
}

// ===========================================================
// All rules
// ===========================================================
void ThemeInspectorWidget::rebuildAllRulesTree()
{
    allRulesTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets())
        collectAllWidgetsRecursive(w, widgets);

    for (int i = 0; i < rules.size(); ++i) {
        int count = 0;
        for (QWidget *w : widgets) {
            if (selectorMatches(w, rules[i].selector))
                ++count;
        }

        auto *it = new QTreeWidgetItem(allRulesTree);
        it->setText(0, rules[i].selector);
        it->setText(1, QString::number(rules[i].line));
        it->setText(2, QString::number(count));
        it->setData(0, Qt::UserRole, i);
        it->setToolTip(0, "Click to edit this rule\n\n" + rules[i].body);

        // Color-code by match count
        if (count == 0) {
            it->setForeground(2, QBrush(Qt::gray));
            it->setToolTip(2, "This rule doesn't currently match any widgets");
        } else if (count > 10) {
            it->setForeground(2, QBrush(QColor(50, 150, 50)));
            it->setToolTip(2, QString("This rule affects %1 widgets").arg(count));
        } else {
            it->setToolTip(2, QString("This rule affects %1 widget(s)").arg(count));
        }
    }

    allRulesTree->resizeColumnToContents(0);
    allRulesTree->resizeColumnToContents(1);
}

// ===========================================================
// Object Names Tree
// ===========================================================
void ThemeInspectorWidget::rebuildObjectNamesTree()
{
    objectNamesTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (hideInspectorCheckbox && hideInspectorCheckbox->isChecked() && (w == this || isChildOf(w, this)))
            continue;
        collectAllWidgetsRecursive(w, widgets);
    }

    // Group by object name
    QMap<QString, QList<QWidget *>> grouped;
    for (QWidget *w : widgets) {
        if (!w->objectName().isEmpty()) {
            grouped[w->objectName()].append(w);
        }
    }

    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        const QString &objName = it.key();
        const QList<QWidget *> &widgetList = it.value();

        if (widgetList.size() == 1) {
            // Single widget with this name
            QWidget *w = widgetList.first();
            auto *item = new QTreeWidgetItem(objectNamesTree);
            item->setText(0, objName);
            item->setText(1, w->metaObject()->className());
            item->setText(2, widgetPath(w));
            item->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(w)));
            item->setToolTip(0, "Double-click to locate in widget tree");
            item->setToolTip(1, QString("Use #%1 in CSS to target this widget").arg(objName));
        } else {
            // Multiple widgets with same name - create parent group
            auto *parent = new QTreeWidgetItem(objectNamesTree);
            parent->setText(0, QString("%1 (%2 widgets)").arg(objName).arg(widgetList.size()));
            parent->setText(1, "");
            parent->setText(2, "");
            QFont font = parent->font(0);
            font.setBold(true);
            parent->setFont(0, font);
            parent->setForeground(0, QBrush(QColor(200, 100, 0)));
            parent->setToolTip(0, QString("⚠ Multiple widgets share #%1 - CSS will affect all of them!").arg(objName));

            for (QWidget *w : widgetList) {
                auto *child = new QTreeWidgetItem(parent);
                child->setText(0, "");
                child->setText(1, w->metaObject()->className());
                child->setText(2, widgetPath(w));
                child->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(w)));
                child->setToolTip(0, "Double-click to locate in widget tree");
            }
        }
    }

    objectNamesTree->expandAll();
    objectNamesTree->resizeColumnToContents(0);
    objectNamesTree->resizeColumnToContents(1);
}

// ===========================================================
// Icons Tree
// ===========================================================
void ThemeInspectorWidget::rebuildIconsTree()
{
    iconsTree->clear();

    QList<QWidget *> widgets;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (hideInspectorCheckbox && hideInspectorCheckbox->isChecked() && (w == this || isChildOf(w, this)))
            continue;
        collectAllWidgetsRecursive(w, widgets);
    }

    QSet<QString> seenIcons;

    for (QWidget *w : widgets) {
        QIcon icon;
        QPixmap pixmap;
        QString iconSource;
        QString resourcePath;

        // Check different widget types for icons
        if (auto *btn = qobject_cast<QPushButton *>(w)) {
            icon = btn->icon();
            iconSource = "QPushButton";
        } else if (auto *tb = qobject_cast<QToolButton *>(w)) {
            icon = tb->icon();
            iconSource = "QToolButton";
        } else if (auto *cb = qobject_cast<QCheckBox *>(w)) {
            icon = cb->icon();
            iconSource = "QCheckBox";
        } else if (auto *rb = qobject_cast<QRadioButton *>(w)) {
            icon = rb->icon();
            iconSource = "QRadioButton";
        } else if (auto *label = qobject_cast<QLabel *>(w)) {
            pixmap = label->pixmap(Qt::ReturnByValue);
            if (!pixmap.isNull()) {
                iconSource = "QLabel (pixmap)";
            }
        }

        if (!icon.isNull() || !pixmap.isNull()) {
            QString iconKey = QString("%1_%2_%3").arg(iconSource, w->objectName(), QString::number((qintptr)w));

            if (!seenIcons.contains(iconKey)) {
                seenIcons.insert(iconKey);

                auto *it = new QTreeWidgetItem(iconsTree);

                if (!icon.isNull()) {
                    it->setIcon(0, icon);
                    it->setText(0, "(icon)");

                    // Try to extract resource path
                    QList<QSize> sizes = icon.availableSizes();
                    if (!sizes.isEmpty()) {
                        it->setToolTip(0, QString("Icon with sizes: %1").arg(sizes.first().width()));
                    }
                } else if (!pixmap.isNull()) {
                    it->setIcon(0, QIcon(pixmap));
                    it->setText(0, QString("(pixmap %1×%2)").arg(pixmap.width()).arg(pixmap.height()));
                }

                it->setText(1, iconSource);
                it->setText(2, w->objectName().isEmpty() ? "(unnamed)" : w->objectName());
                it->setData(0, Qt::UserRole, QVariant::fromValue(QPointer<QObject>(w)));

                QString cssExample = w->objectName().isEmpty()
                                         ? QString("%1 { icon: url(:/path/to/icon.png); }").arg(iconSource)
                                         : QString("#%1 { icon: url(:/path/to/icon.png); }").arg(w->objectName());

                it->setToolTip(0, "CSS Example:\n" + cssExample);
            }
        }
    }

    iconsTree->resizeColumnToContents(0);
    iconsTree->resizeColumnToContents(1);
}

void ThemeInspectorWidget::collectAllWidgetsRecursive(QWidget *w, QList<QWidget *> &out) const
{
    if (!w)
        return;
    out.append(w);
    for (QObject *c : w->children()) {
        if (auto *cw = qobject_cast<QWidget *>(c))
            collectAllWidgetsRecursive(cw, out);
    }
}

// ===========================================================
// Highlight
// ===========================================================
void ThemeInspectorWidget::highlightMatchingWidget(const QString &selector)
{
    if (selector.trimmed().isEmpty() || selector.trimmed() == "QWidget")
        return;

    widgetTree->clearSelection();

    // Try to find and select a matching widget
    QTreeWidgetItemIterator it(widgetTree);
    while (*it) {
        QWidget *w = qobject_cast<QWidget *>((*it)->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w && selectorMatches(w, selector)) {
            (*it)->setSelected(true);
            widgetTree->scrollToItem(*it);
            updateForSelection();
            return;
        }
        ++it;
    }
}

// ===========================================================
// Rule creation / deletion
// ===========================================================
void ThemeInspectorWidget::moveSelectedRule(int direction)
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        items = ruleTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Rule Selected", "Please select a CSS rule to move.");
            return;
        }
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    int newIdx = idx + direction;
    if (newIdx < 0 || newIdx >= rules.size()) {
        updateStatus("Cannot move rule further in that direction", true);
        return;
    }

    // Swap rules
    Rule temp = rules[idx];
    rules[idx] = rules[newIdx];
    rules[newIdx] = temp;

    // Write to file
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(out.toUtf8());
        f.close();
        updateStatus(QString("✓ Moved rule: %1").arg(temp.selector));
    }

    rebuildAllRulesTree();

    // Refresh the affecting rules for currently selected widget
    auto widgetItems = widgetTree->selectedItems();
    if (!widgetItems.isEmpty()) {
        QWidget *w =
            qobject_cast<QWidget *>(widgetItems.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w) {
            updateRuleMatches(w);
        }
    }

    // Reselect the moved rule
    for (int i = 0; i < allRulesTree->topLevelItemCount(); ++i) {
        auto *item = allRulesTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toInt() == newIdx) {
            allRulesTree->setCurrentItem(item);
            break;
        }
    }
}

void ThemeInspectorWidget::editSelectedRuleSelector()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        items = ruleTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Rule Selected", "Please select a CSS rule to edit its selector.");
            return;
        }
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    QString oldSelector = rules[idx].selector;
    bool ok;
    QString newSelector = QInputDialog::getText(
        this, "Edit Selector", QString("Enter new selector for this rule:\n\nCurrent: %1").arg(oldSelector),
        QLineEdit::Normal, oldSelector, &ok);

    if (!ok || newSelector.trimmed().isEmpty() || newSelector == oldSelector)
        return;

    newSelector = newSelector.trimmed();

    // Check if new selector already exists
    for (int i = 0; i < rules.size(); ++i) {
        if (i != idx && rules[i].selector == newSelector) {
            QMessageBox::warning(this, "Selector Exists",
                                 QString("A rule with selector '%1' already exists.").arg(newSelector));
            return;
        }
    }

    rules[idx].selector = newSelector;

    // Write to file
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(out.toUtf8());
        f.close();
        updateStatus(QString("✓ Updated selector: %1 → %2").arg(oldSelector, newSelector));
    }

    rebuildAllRulesTree();

    // Refresh the affecting rules for currently selected widget
    auto widgetItems = widgetTree->selectedItems();
    if (!widgetItems.isEmpty()) {
        QWidget *w =
            qobject_cast<QWidget *>(widgetItems.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w) {
            updateRuleMatches(w);
        }
    }
}

void ThemeInspectorWidget::addRuleForWidget(QWidget *, const QString &selector, const QString &body)
{
    if (selector.isEmpty())
        return;

    // Check if rule already exists
    for (const auto &r : rules) {
        if (r.selector == selector) {
            QMessageBox::information(this, "Rule Already Exists",
                                     QString("A CSS rule for selector '%1' already exists.\n\n"
                                             "You can edit it in the 'All Rules' tab.")
                                         .arg(selector));
            return;
        }
    }

    rules.push_back({selector, body, rules.isEmpty() ? 1 : rules.back().line + 1});

    // Write to file immediately
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(out.toUtf8());
        f.close();
        updateStatus(QString("✓ Created new rule: %1").arg(selector));
    } else {
        QMessageBox::critical(this, "Save Error", "Could not save the new rule to the stylesheet file.");
        return;
    }

    rebuildAllRulesTree();

    // Refresh the affecting rules for currently selected widget
    auto widgetItems = widgetTree->selectedItems();
    if (!widgetItems.isEmpty()) {
        QWidget *w =
            qobject_cast<QWidget *>(widgetItems.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w) {
            updateRuleMatches(w);
        }
    }
}

void ThemeInspectorWidget::addRuleForAll(const QString &selector, const QString &body)
{
    addRuleForWidget(nullptr, selector, body);
}

void ThemeInspectorWidget::deleteSelectedRule()
{
    auto items = allRulesTree->selectedItems();
    if (items.isEmpty()) {
        // Try ruleTree instead
        items = ruleTree->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::information(this, "No Rule Selected", "Please select a CSS rule to delete.");
            return;
        }
    }

    int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= rules.size())
        return;

    QString selector = rules[idx].selector;

    auto reply = QMessageBox::question(this, "Confirm Deletion",
                                       QString("Are you sure you want to delete this CSS rule?\n\n"
                                               "Selector: %1\n\n"
                                               "This action cannot be undone.")
                                           .arg(selector),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    rules.removeAt(idx);

    // Write to file
    QString out;
    for (const auto &r : rules)
        out += QString("%1 {\n%2\n}\n\n").arg(r.selector, r.body);

    QFile f(stylesheetPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(out.toUtf8());
        f.close();
        updateStatus(QString("✓ Deleted rule: %1").arg(selector));
    }

    rebuildAllRulesTree();

    // Refresh the affecting rules for currently selected widget
    auto widgetItems = widgetTree->selectedItems();
    if (!widgetItems.isEmpty()) {
        QWidget *w =
            qobject_cast<QWidget *>(widgetItems.first()->data(0, Qt::UserRole).value<QPointer<QObject>>().data());
        if (w) {
            updateRuleMatches(w);
        }
    }

    ruleEditor->clear();
}

// ===========================================================
// Status updates
// ===========================================================
void ThemeInspectorWidget::updateStatus(const QString &message, bool isError)
{
    if (statusLabel) {
        statusLabel->setText(message);
        statusLabel->setStyleSheet(isError ? "color: #ff4444; padding: 4px; font-weight: bold;"
                                           : "color: #44ff44; padding: 4px;");

        // Reset to gray after 4 seconds
        QTimer::singleShot(4000, this, [this]() {
            if (statusLabel) {
                statusLabel->setText("Ready");
                statusLabel->setStyleSheet("color: gray; padding: 4px;");
            }
        });
    }
}