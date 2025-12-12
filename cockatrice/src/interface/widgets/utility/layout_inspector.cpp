#include "layout_inspector.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

// ============================================================================
// LayoutOverlay - Enhanced with more visual feedback
// ============================================================================

LayoutOverlay::LayoutOverlay(QWidget *parent)
    : QWidget(parent), m_target(nullptr), m_selected(nullptr), m_showDimensions(true), m_showMargins(false)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_AlwaysStackOnTop);
}

void LayoutOverlay::setTarget(QWidget *t)
{
    m_target = t;
    update();
}

void LayoutOverlay::setEnabledOverlay(bool en)
{
    setVisible(en);
}

void LayoutOverlay::setShowDimensions(bool show)
{
    m_showDimensions = show;
    update();
}

void LayoutOverlay::setShowMargins(bool show)
{
    m_showMargins = show;
    update();
}

void LayoutOverlay::setHighlightSelected(QWidget *selected)
{
    m_selected = selected;
    update();
}

void LayoutOverlay::paintEvent(QPaintEvent *)
{
    if (!m_target || !isVisible() || !parentWidget())
        return;

    QWidget *top = parentWidget();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QList<QWidget *> list = m_target->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively);
    list.prepend(m_target);

    for (QWidget *w : list) {
        if (!w->isVisible())
            continue;

        QRect r = w->rect();
        QPoint g = w->mapToGlobal(r.topLeft());
        QPoint mapped = top->mapFromGlobal(g);
        QRect finalRect(mapped, r.size());

        if (!rect().intersects(finalRect))
            continue;

        // Color based on properties
        bool isSelected = (w == m_selected);
        bool hasLayout = (w->layout() != nullptr);

        QColor fillCol;
        QColor borderCol;

        if (isSelected) {
            fillCol = QColor(255, 200, 0, 120);
            borderCol = QColor(255, 150, 0, 255);
        } else if (hasLayout) {
            fillCol = QColor(0, 150, 200, 60);
            borderCol = QColor(0, 100, 150, 180);
        } else {
            fillCol = QColor(200, 100, 0, 40);
            borderCol = QColor(150, 70, 0, 150);
        }

        p.fillRect(finalRect, fillCol);
        p.setPen(QPen(borderCol, isSelected ? 2 : 1));
        p.drawRect(finalRect);

        // Draw dimensions
        if (m_showDimensions) {
            QString dimText = QString("%1×%2").arg(finalRect.width()).arg(finalRect.height());
            QFont f = p.font();
            f.setPointSize(9);
            f.setBold(isSelected);
            p.setFont(f);
            p.setPen(Qt::black);
            p.drawText(finalRect.topLeft() + QPoint(4, 14), dimText);

            QString className = w->metaObject()->className();
            if (!w->objectName().isEmpty())
                className = w->objectName();
            p.drawText(finalRect.topLeft() + QPoint(4, 28), className);
        }

        // Draw margins if showing layout
        if (m_showMargins && w->layout()) {
            QLayout *layout = w->layout();
            int left, top, right, bottom;
            layout->getContentsMargins(&left, &top, &right, &bottom);

            p.setPen(QPen(QColor(255, 0, 0, 180), 1, Qt::DashLine));

            QRect marginRect = finalRect.adjusted(left, top, -right, -bottom);
            p.drawRect(marginRect);

            // Draw margin values
            p.setPen(Qt::red);
            if (top > 5)
                p.drawText(finalRect.center().x() - 10, finalRect.top() + top - 2, QString::number(top));
            if (left > 5)
                p.drawText(finalRect.left() + 2, finalRect.center().y(), QString::number(left));
        }
    }
}

// ============================================================================
// LayoutInspector - Enhanced implementation
// ============================================================================

LayoutInspector::LayoutInspector(QWidget *target, QWidget *parent)
    : QWidget(parent), m_target(nullptr), m_layout(nullptr)
{
    setWindowTitle("Qt Layout & Sizing Inspector - Enhanced");
    setMinimumSize(900, 600);
    setWindowFlags(Qt::Window | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    buildUi();
    if (target)
        attachTo(target);
}

void LayoutInspector::attachTo(QWidget *target)
{
    if (!target)
        return;

    m_target = target;
    m_layout = target->layout();

    QWidget *top = m_target->window();

    // Overlay MUST be parented BEFORE geometry is applied
    m_overlay->setParent(top);
    m_overlay->setGeometry(top->rect());
    m_overlay->raise();
    m_overlay->setTarget(target);
    m_overlay->setVisible(m_overlayToggle->isChecked());
    m_overlay->update();

    populateTree();
    refreshAll();
    updateIssuesPanel();
}

void LayoutInspector::buildUi()
{
    auto *main = new QHBoxLayout(this);

    // Create splitter for resizable panels
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    main->addWidget(splitter);

    // LEFT: Tree panel
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    QLabel *treeLabel = new QLabel("<b>Widget Hierarchy</b>");
    leftLayout->addWidget(treeLabel);

    m_tree = new QTreeWidget;
    m_tree->setHeaderHidden(true);
    connect(m_tree, &QTreeWidget::itemClicked, this, &LayoutInspector::onTreeSelection);
    leftLayout->addWidget(m_tree);

    splitter->addWidget(leftPanel);

    // MIDDLE: Tabbed property panels
    QTabWidget *tabs = new QTabWidget;

    // Tab 1: Info Panel
    QWidget *infoPanel = new QWidget;
    QVBoxLayout *infoLayout = new QVBoxLayout(infoPanel);

    QGroupBox *sizeGroup = new QGroupBox("Size Information");
    QFormLayout *sizeForm = new QFormLayout(sizeGroup);

    m_infoWidget = new QLabel("-");
    m_infoSize = new QLabel("-");
    m_infoSizeHint = new QLabel("-");
    m_infoMinSize = new QLabel("-");
    m_infoMaxSize = new QLabel("-");
    m_infoGeometry = new QLabel("-");
    m_infoVisible = new QLabel("-");
    m_infoEnabled = new QLabel("-");

    sizeForm->addRow("Widget:", m_infoWidget);
    sizeForm->addRow("Current Size:", m_infoSize);
    sizeForm->addRow("Size Hint:", m_infoSizeHint);
    sizeForm->addRow("Minimum Size:", m_infoMinSize);
    sizeForm->addRow("Maximum Size:", m_infoMaxSize);
    sizeForm->addRow("Geometry:", m_infoGeometry);
    sizeForm->addRow("Visible:", m_infoVisible);
    sizeForm->addRow("Enabled:", m_infoEnabled);

    infoLayout->addWidget(sizeGroup);

    QGroupBox *issuesGroup = new QGroupBox("Layout Issues & Warnings");
    QVBoxLayout *issuesLayout = new QVBoxLayout(issuesGroup);
    m_issuesText = new QTextEdit;
    m_issuesText->setReadOnly(true);
    m_issuesText->setMaximumHeight(150);
    issuesLayout->addWidget(m_issuesText);

    QPushButton *analyzeBtn = new QPushButton("Analyze Layout");
    connect(analyzeBtn, &QPushButton::clicked, this, &LayoutInspector::onAnalyzeLayout);
    issuesLayout->addWidget(analyzeBtn);

    infoLayout->addWidget(issuesGroup);
    infoLayout->addStretch();

    tabs->addTab(infoPanel, "Info");

    // Tab 2: Widget Properties Panel
    QWidget *widgetPanel = new QWidget;
    QScrollArea *widgetScroll = new QScrollArea;
    widgetScroll->setWidgetResizable(true);
    widgetScroll->setWidget(widgetPanel);

    QFormLayout *wform = new QFormLayout(widgetPanel);

    m_hPolicy = new QComboBox;
    populatePolicyBox(m_hPolicy);
    m_vPolicy = new QComboBox;
    populatePolicyBox(m_vPolicy);
    wform->addRow("H Policy:", m_hPolicy);
    wform->addRow("V Policy:", m_vPolicy);

    m_minWSpin = new QSpinBox;
    m_minWSpin->setRange(0, 10000);
    m_maxWSpin = new QSpinBox;
    m_maxWSpin->setRange(0, 10000);
    m_maxWSpin->setValue(16777215); // QWIDGETSIZE_MAX
    m_minHSpin = new QSpinBox;
    m_minHSpin->setRange(0, 10000);
    m_maxHSpin = new QSpinBox;
    m_maxHSpin->setRange(0, 10000);
    m_maxHSpin->setValue(16777215);

    wform->addRow("Min Width:", m_minWSpin);
    wform->addRow("Max Width:", m_maxWSpin);
    wform->addRow("Min Height:", m_minHSpin);
    wform->addRow("Max Height:", m_maxHSpin);

    m_fixedCheck = new QCheckBox("Use Fixed Size");
    m_fixedWSpin = new QSpinBox;
    m_fixedWSpin->setRange(0, 10000);
    m_fixedHSpin = new QSpinBox;
    m_fixedHSpin->setRange(0, 10000);
    wform->addRow(m_fixedCheck);
    wform->addRow("Fixed Width:", m_fixedWSpin);
    wform->addRow("Fixed Height:", m_fixedHSpin);

    QPushButton *applyWidgetBtn = new QPushButton("Apply Widget Changes");
    connect(applyWidgetBtn, &QPushButton::clicked, this, &LayoutInspector::applyWidgetChanges);
    wform->addRow(applyWidgetBtn);

    QPushButton *resetWidgetBtn = new QPushButton("Reset to Defaults");
    connect(resetWidgetBtn, &QPushButton::clicked, this, [this]() {
        if (m_selected) {
            m_selected->setMinimumSize(0, 0);
            m_selected->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            m_selected->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            repaintTarget();
            updatePropertyPanel();
        }
    });
    wform->addRow(resetWidgetBtn);

    tabs->addTab(widgetScroll, "Widget");

    // Tab 3: Layout Panel
    QWidget *layoutPanel = new QWidget;
    QScrollArea *layoutScroll = new QScrollArea;
    layoutScroll->setWidgetResizable(true);
    layoutScroll->setWidget(layoutPanel);

    QVBoxLayout *lbox = new QVBoxLayout(layoutPanel);

    QGroupBox *spacingGroup = new QGroupBox("Spacing & Margins");
    QFormLayout *lform = new QFormLayout(spacingGroup);

    m_spacingSpin = new QSpinBox;
    m_spacingSpin->setRange(0, 200);
    lform->addRow("Spacing:", m_spacingSpin);

    m_marginSpin = new QSpinBox;
    m_marginSpin->setRange(0, 200);
    lform->addRow("Uniform Margin:", m_marginSpin);

    // Individual margins
    m_marginLeft = new QSpinBox;
    m_marginLeft->setRange(0, 200);
    m_marginTop = new QSpinBox;
    m_marginTop->setRange(0, 200);
    m_marginRight = new QSpinBox;
    m_marginRight->setRange(0, 200);
    m_marginBottom = new QSpinBox;
    m_marginBottom->setRange(0, 200);

    lform->addRow("Left Margin:", m_marginLeft);
    lform->addRow("Top Margin:", m_marginTop);
    lform->addRow("Right Margin:", m_marginRight);
    lform->addRow("Bottom Margin:", m_marginBottom);

    lbox->addWidget(spacingGroup);

    QPushButton *applyLayoutBtn = new QPushButton("Apply Layout Changes");
    connect(applyLayoutBtn, &QPushButton::clicked, this, &LayoutInspector::applyLayoutChanges);
    lbox->addWidget(applyLayoutBtn);

    m_stretchArea = new QWidget;
    m_stretchLayout = new QVBoxLayout(m_stretchArea);
    lbox->addWidget(new QLabel("<b>Stretch / Row-Column Editor:</b>"));
    lbox->addWidget(m_stretchArea);

    lbox->addStretch();

    tabs->addTab(layoutScroll, "Layout");

    // Tab 4: Grid Editor (if applicable)
    QWidget *gridPanel = new QWidget;
    QVBoxLayout *gridLayout = new QVBoxLayout(gridPanel);

    m_gridItems = new QTreeWidget;
    m_gridItems->setHeaderLabel("Grid Items");
    connect(m_gridItems, &QTreeWidget::itemClicked, this, &LayoutInspector::onGridSelect);
    gridLayout->addWidget(m_gridItems);

    QPushButton *moveBtn = new QPushButton("Move Selected Item...");
    connect(moveBtn, &QPushButton::clicked, this, &LayoutInspector::onMoveGridItem);
    gridLayout->addWidget(moveBtn);

    QGroupBox *alignGroup = new QGroupBox("Alignment");
    QVBoxLayout *alignLayout = new QVBoxLayout(alignGroup);

    m_alignLeft = new QCheckBox("Left");
    m_alignHCenter = new QCheckBox("H-Center");
    m_alignRight = new QCheckBox("Right");
    m_alignTop = new QCheckBox("Top");
    m_alignVCenter = new QCheckBox("V-Center");
    m_alignBottom = new QCheckBox("Bottom");

    alignLayout->addWidget(m_alignLeft);
    alignLayout->addWidget(m_alignHCenter);
    alignLayout->addWidget(m_alignRight);
    alignLayout->addWidget(m_alignTop);
    alignLayout->addWidget(m_alignVCenter);
    alignLayout->addWidget(m_alignBottom);

    QPushButton *setAlignBtn = new QPushButton("Set Alignment");
    connect(setAlignBtn, &QPushButton::clicked, this, &LayoutInspector::onSetAlignment);
    alignLayout->addWidget(setAlignBtn);

    gridLayout->addWidget(alignGroup);

    tabs->addTab(gridPanel, "Grid");

    splitter->addWidget(tabs);

    // RIGHT: Controls
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rlay = new QVBoxLayout(rightPanel);

    QLabel *controlLabel = new QLabel("<b>Controls</b>");
    rlay->addWidget(controlLabel);

    QPushButton *refresh = new QPushButton("Refresh All");
    connect(refresh, &QPushButton::clicked, this, &LayoutInspector::onRefresh);
    rlay->addWidget(refresh);

    m_overlayToggle = new QCheckBox("Show Overlay");
    m_overlayToggle->setChecked(true);
    connect(m_overlayToggle, &QCheckBox::checkStateChanged, this, &LayoutInspector::onOverlayToggle);
    rlay->addWidget(m_overlayToggle);

    m_showDimensions = new QCheckBox("Show Dimensions");
    m_showDimensions->setChecked(true);
    connect(m_showDimensions, &QCheckBox::checkStateChanged, this, &LayoutInspector::onShowDimensionsToggle);
    rlay->addWidget(m_showDimensions);

    m_showMargins = new QCheckBox("Show Margins");
    connect(m_showMargins, &QCheckBox::checkStateChanged, this, &LayoutInspector::onShowMarginsToggle);
    rlay->addWidget(m_showMargins);

    QPushButton *jumpBtn = new QPushButton("Jump to Widget");
    connect(jumpBtn, &QPushButton::clicked, this, &LayoutInspector::onJumpToWidget);
    rlay->addWidget(jumpBtn);

    QPushButton *highlightBtn = new QPushButton("Highlight Widget");
    connect(highlightBtn, &QPushButton::clicked, this, &LayoutInspector::onHighlightWidget);
    rlay->addWidget(highlightBtn);

    QPushButton *autoBtn = new QPushButton("Auto Adjust Sizes");
    connect(autoBtn, &QPushButton::clicked, this, [this]() {
        if (m_target) {
            m_target->adjustSize();
            repaintTarget();
            QTimer::singleShot(100, this, &LayoutInspector::updateInfoPanel);
        }
    });
    rlay->addWidget(autoBtn);

    QPushButton *compareBtn = new QPushButton("Compare Sizes");
    connect(compareBtn, &QPushButton::clicked, this, &LayoutInspector::onCompareMinimumSizes);
    rlay->addWidget(compareBtn);

    QPushButton *exportBtn = new QPushButton("Export Info...");
    connect(exportBtn, &QPushButton::clicked, this, &LayoutInspector::onExportLayoutInfo);
    rlay->addWidget(exportBtn);

    QPushButton *choose = new QPushButton("Attach to...");
    connect(choose, &QPushButton::clicked, this, &LayoutInspector::showAttachDialog);
    rlay->addWidget(choose);

    rlay->addStretch(1);
    splitter->addWidget(rightPanel);

    // Set splitter sizes
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 4);
    splitter->setStretchFactor(2, 1);

    m_overlay = new LayoutOverlay(nullptr);
    m_overlay->hide();
}

// ============================================================================
// Slots Implementation
// ============================================================================

void LayoutInspector::onJumpToWidget()
{
    if (!m_selected)
        return;

    QWidget *w = m_selected;
    QWidget *p = w->parentWidget();

    while (p) {
        if (QScrollArea *sa = qobject_cast<QScrollArea *>(p)) {
            sa->ensureWidgetVisible(w);
            return;
        }
        p = p->parentWidget();
    }

    QRect r = w->rect();
    QPoint g = w->mapToGlobal(r.topLeft());
    QWidget *win = w->window();

    if (win) {
        QRect winRect = win->geometry();
        QPoint centerOnWidget = g - QPoint(winRect.width() / 2, winRect.height() / 2);
        win->move(centerOnWidget);
    }
}

void LayoutInspector::onHighlightWidget()
{
    if (!m_selected || !m_overlay)
        return;

    m_overlay->setHighlightSelected(m_selected);
    m_overlay->setVisible(true);
    m_overlayToggle->setChecked(true);

    // Flash effect
    for (int i = 0; i < 3; ++i) {
        QTimer::singleShot(i * 200, this, [this, i]() { m_overlay->setVisible(i % 2 == 0); });
    }
    QTimer::singleShot(600, this, [this]() { m_overlay->setVisible(true); });
}

void LayoutInspector::onTreeSelection()
{
    QTreeWidgetItem *it = m_tree->currentItem();
    if (!it)
        return;

    QVariant v = it->data(0, Qt::UserRole);
    QWidget *w = qvariant_cast<QWidget *>(v);

    if (!w)
        return;

    selectWidget(w);
    m_overlay->setHighlightSelected(w);
}

void LayoutInspector::applyWidgetChanges()
{
    if (!m_selected)
        return;

    m_selected->setMinimumSize(m_minWSpin->value(), m_minHSpin->value());
    m_selected->setMaximumSize(m_maxWSpin->value(), m_maxHSpin->value());

    QSizePolicy sp(static_cast<QSizePolicy::Policy>(m_hPolicy->currentData().toInt()),
                   static_cast<QSizePolicy::Policy>(m_vPolicy->currentData().toInt()));
    m_selected->setSizePolicy(sp);

    if (m_fixedCheck->isChecked()) {
        m_selected->setFixedSize(m_fixedWSpin->value(), m_fixedHSpin->value());
    }

    repaintTarget();
    QTimer::singleShot(50, this, &LayoutInspector::updatePropertyPanel);
    QTimer::singleShot(50, this, &LayoutInspector::updateInfoPanel);
}

void LayoutInspector::applyLayoutChanges()
{
    if (!m_layout)
        return;

    m_layout->setSpacing(m_spacingSpin->value());

    m_layout->setContentsMargins(m_marginLeft->value(), m_marginTop->value(), m_marginRight->value(),
                                 m_marginBottom->value());

    repaintTarget();
}

void LayoutInspector::onRefresh()
{
    populateTree();
    refreshAll();
    updateIssuesPanel();
}

void LayoutInspector::onOverlayToggle(int st)
{
    bool en = (st == Qt::Checked);
    if (m_overlay) {
        m_overlay->setVisible(en);
        if (en) {
            m_overlay->raise();
            m_overlay->update();
        }
    }
}

void LayoutInspector::onShowDimensionsToggle(int st)
{
    if (m_overlay) {
        m_overlay->setShowDimensions(st == Qt::Checked);
    }
}

void LayoutInspector::onShowMarginsToggle(int st)
{
    if (m_overlay) {
        m_overlay->setShowMargins(st == Qt::Checked);
    }
}

void LayoutInspector::onStretchChanged(int)
{
    QSpinBox *spin = qobject_cast<QSpinBox *>(sender());
    if (!spin || !m_layout)
        return;

    int pos = spin->property("posIndex").toInt();
    int val = spin->value();

    if (QGridLayout *g = qobject_cast<QGridLayout *>(m_layout)) {
        bool isRow = spin->property("isRow").toBool();
        if (isRow)
            g->setRowStretch(pos, val);
        else
            g->setColumnStretch(pos, val);
    } else if (QBoxLayout *b = qobject_cast<QBoxLayout *>(m_layout)) {
        b->setStretch(pos, val);
    }

    repaintTarget();
}

void LayoutInspector::onMoveGridItem()
{
    if (!m_gridItemWidget || !m_layout)
        return;

    bool ok = false;
    int row = QInputDialog::getInt(this, "Move Item", "New row:", 0, 0, 100, 1, &ok);
    if (!ok)
        return;

    int col = QInputDialog::getInt(this, "Move Item", "New col:", 0, 0, 100, 1, &ok);
    if (!ok)
        return;

    int rowSpan = QInputDialog::getInt(this, "Row Span", "rowSpan:", 1, 1, 100, 1, &ok);
    if (!ok)
        return;

    int colSpan = QInputDialog::getInt(this, "Column Span", "colSpan:", 1, 1, 100, 1, &ok);
    if (!ok)
        return;

    if (QGridLayout *g = qobject_cast<QGridLayout *>(m_layout)) {
        removeWidgetFromLayout(g, m_gridItemWidget);
        g->addWidget(m_gridItemWidget, row, col, rowSpan, colSpan);
        repaintTarget();
        populateGridEditor();
    }
}

void LayoutInspector::onGridSelect(QTreeWidgetItem *it, int)
{
    if (!it)
        return;
    QVariant v = it->data(0, Qt::UserRole);
    m_gridItemWidget = qvariant_cast<QWidget *>(v);
}

void LayoutInspector::onSetAlignment()
{
    if (!m_gridItemWidget || !m_layout)
        return;

    Qt::Alignment a = {};

    if (m_alignLeft->isChecked())
        a |= Qt::AlignLeft;
    if (m_alignHCenter->isChecked())
        a |= Qt::AlignHCenter;
    if (m_alignRight->isChecked())
        a |= Qt::AlignRight;
    if (m_alignTop->isChecked())
        a |= Qt::AlignTop;
    if (m_alignVCenter->isChecked())
        a |= Qt::AlignVCenter;
    if (m_alignBottom->isChecked())
        a |= Qt::AlignBottom;

    if (QGridLayout *g = qobject_cast<QGridLayout *>(m_layout))
        g->setAlignment(m_gridItemWidget, a);
    else if (QBoxLayout *b = qobject_cast<QBoxLayout *>(m_layout))
        b->setAlignment(m_gridItemWidget, a);

    repaintTarget();
}

void LayoutInspector::onAnalyzeLayout()
{
    updateIssuesPanel();
}

void LayoutInspector::onExportLayoutInfo()
{
    if (!m_target)
        return;

    QString filename =
        QFileDialog::getSaveFileName(this, "Export Layout Info", "", "Text Files (*.txt);;All Files (*)");

    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "Layout Inspector Export\n";
    out << "=======================\n\n";
    out << "Target: " << m_target->metaObject()->className() << "\n\n";

    if (m_selected) {
        out << analyzeWidget(m_selected) << "\n\n";
    }

    if (m_layout) {
        out << analyzeLayout(m_layout) << "\n\n";
    }

    QStringList issues = detectLayoutIssues();
    if (!issues.isEmpty()) {
        out << "Issues Found:\n";
        for (const QString &issue : issues) {
            out << "  - " << issue << "\n";
        }
    }

    file.close();
    QMessageBox::information(this, "Export Complete", "Layout info exported to:\n" + filename);
}

void LayoutInspector::onCompareMinimumSizes()
{
    if (!m_selected)
        return;

    QString report;
    report += QString("Widget: %1\n").arg(m_selected->metaObject()->className());
    report += QString("Current Size: %1×%2\n").arg(m_selected->width()).arg(m_selected->height());
    report += QString("Size Hint: %1×%2\n").arg(m_selected->sizeHint().width()).arg(m_selected->sizeHint().height());
    report +=
        QString("Minimum Size: %1×%2\n").arg(m_selected->minimumSize().width()).arg(m_selected->minimumSize().height());
    report += QString("Minimum Size Hint: %1×%2\n")
                  .arg(m_selected->minimumSizeHint().width())
                  .arg(m_selected->minimumSizeHint().height());
    report +=
        QString("Maximum Size: %1×%2\n").arg(m_selected->maximumSize().width()).arg(m_selected->maximumSize().height());

    // QSizePolicy sp = m_selected->sizePolicy();
    report += QString("\nH Policy: %1\n").arg(m_hPolicy->currentText());
    report += QString("V Policy: %1\n").arg(m_vPolicy->currentText());

    QMessageBox::information(this, "Size Comparison", report);
}

// ============================================================================
// Helper Functions
// ============================================================================

void LayoutInspector::populateTree()
{
    if (!m_tree || !m_target)
        return;

    m_tree->clear();

    QString rootName = m_target->objectName().isEmpty() ? m_target->metaObject()->className() : m_target->objectName();

    QTreeWidgetItem *root = new QTreeWidgetItem(m_tree, QStringList(rootName));
    root->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)m_target));

    addChildrenToItem(m_target, root);
    m_tree->expandAll();
}

void LayoutInspector::addChildrenToItem(QWidget *w, QTreeWidgetItem *parent)
{
    if (!w || !parent)
        return;

    for (QObject *c : w->children()) {
        QWidget *cw = qobject_cast<QWidget *>(c);
        if (!cw)
            continue;

        QString name = cw->objectName().isEmpty()
                           ? cw->metaObject()->className()
                           : QString("%1 (%2)").arg(cw->objectName()).arg(cw->metaObject()->className());

        QTreeWidgetItem *it = new QTreeWidgetItem(parent, QStringList(name));
        it->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)cw));

        addChildrenToItem(cw, it);
    }
}

void LayoutInspector::populatePolicyBox(QComboBox *box)
{
    if (!box)
        return;

    box->clear();

    struct PolicyEntry
    {
        QString name;
        QSizePolicy::Policy policy;
    };

    PolicyEntry entries[] = {{"Fixed", QSizePolicy::Fixed},         {"Minimum", QSizePolicy::Minimum},
                             {"Maximum", QSizePolicy::Maximum},     {"Preferred", QSizePolicy::Preferred},
                             {"Expanding", QSizePolicy::Expanding}, {"MinimumExpanding", QSizePolicy::MinimumExpanding},
                             {"Ignored", QSizePolicy::Ignored}};

    for (const auto &e : entries) {
        box->addItem(e.name, QVariant::fromValue(int(e.policy)));
    }
}

void LayoutInspector::updateLayoutPanel()
{
    if (!m_target)
        return;

    QLayout *layout = (m_selected ? m_selected->layout() : m_target->layout());

    if (!layout) {
        m_stretchArea->hide();
        return;
    }

    m_stretchArea->show();

    // Clear previous controls
    QLayoutItem *item;
    while ((item = m_stretchLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    // Set spacing & margins
    m_spacingSpin->setValue(layout->spacing());

    int left, top, right, bottom;
    layout->getContentsMargins(&left, &top, &right, &bottom);
    m_marginSpin->setValue(left);
    m_marginLeft->setValue(left);
    m_marginTop->setValue(top);
    m_marginRight->setValue(right);
    m_marginBottom->setValue(bottom);

    // For box layouts
    if (QBoxLayout *box = qobject_cast<QBoxLayout *>(layout)) {
        QWidget *stretchWidget = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(stretchWidget);
        h->addWidget(new QLabel("Item stretches:"));

        for (int i = 0; i < box->count(); ++i) {
            QSpinBox *s = new QSpinBox;
            s->setRange(0, 100);
            s->setValue(box->stretch(i));
            s->setProperty("posIndex", i);
            connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &LayoutInspector::onStretchChanged);
            h->addWidget(new QLabel(QString::number(i)));
            h->addWidget(s);
        }

        m_stretchLayout->addWidget(stretchWidget);
    }

    // For grid layouts
    if (QGridLayout *grid = qobject_cast<QGridLayout *>(layout)) {
        int maxRow = findMaxGridRow(grid);
        int maxCol = findMaxGridCol(grid);

        // Row stretches
        QWidget *rowWidget = new QWidget;
        QHBoxLayout *rlay = new QHBoxLayout(rowWidget);
        rlay->addWidget(new QLabel("Row stretches:"));

        for (int r = 0; r <= maxRow; ++r) {
            QSpinBox *s = new QSpinBox;
            s->setRange(0, 100);
            s->setValue(grid->rowStretch(r));
            s->setProperty("posIndex", r);
            s->setProperty("isRow", true);
            connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &LayoutInspector::onStretchChanged);
            rlay->addWidget(new QLabel(QString::number(r)));
            rlay->addWidget(s);
        }

        m_stretchLayout->addWidget(rowWidget);

        // Column stretches
        QWidget *colWidget = new QWidget;
        QHBoxLayout *clay = new QHBoxLayout(colWidget);
        clay->addWidget(new QLabel("Column stretches:"));

        for (int c = 0; c <= maxCol; ++c) {
            QSpinBox *s = new QSpinBox;
            s->setRange(0, 100);
            s->setValue(grid->columnStretch(c));
            s->setProperty("posIndex", c);
            s->setProperty("isRow", false);
            connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &LayoutInspector::onStretchChanged);
            clay->addWidget(new QLabel(QString::number(c)));
            clay->addWidget(s);
        }

        m_stretchLayout->addWidget(colWidget);

        populateGridEditor();
    }
}

void LayoutInspector::updatePropertyPanel()
{
    if (!m_selected)
        return;

    QSizePolicy sp = m_selected->sizePolicy();
    m_hPolicy->setCurrentIndex(findIndexByData(m_hPolicy, int(sp.horizontalPolicy())));
    m_vPolicy->setCurrentIndex(findIndexByData(m_vPolicy, int(sp.verticalPolicy())));

    m_minWSpin->setValue(m_selected->minimumWidth());
    m_maxWSpin->setValue(m_selected->maximumWidth());
    m_minHSpin->setValue(m_selected->minimumHeight());
    m_maxHSpin->setValue(m_selected->maximumHeight());

    bool isFixed = (m_selected->minimumSize() == m_selected->maximumSize() && m_selected->minimumWidth() > 0 &&
                    m_selected->minimumHeight() > 0);
    m_fixedCheck->setChecked(isFixed);
    m_fixedWSpin->setValue(m_selected->width());
    m_fixedHSpin->setValue(m_selected->height());
}

void LayoutInspector::updateInfoPanel()
{
    if (!m_selected)
        return;

    QString className = m_selected->metaObject()->className();
    if (!m_selected->objectName().isEmpty())
        className = m_selected->objectName() + " (" + className + ")";
    m_infoWidget->setText(className);

    m_infoSize->setText(QString("%1×%2").arg(m_selected->width()).arg(m_selected->height()));

    QSize hint = m_selected->sizeHint();
    m_infoSizeHint->setText(QString("%1×%2").arg(hint.width()).arg(hint.height()));

    QSize minSize = m_selected->minimumSize();
    m_infoMinSize->setText(QString("%1×%2").arg(minSize.width()).arg(minSize.height()));

    QSize maxSize = m_selected->maximumSize();
    QString maxStr = (maxSize.width() >= QWIDGETSIZE_MAX || maxSize.height() >= QWIDGETSIZE_MAX)
                         ? "Unlimited"
                         : QString("%1×%2").arg(maxSize.width()).arg(maxSize.height());
    m_infoMaxSize->setText(maxStr);

    QRect geo = m_selected->geometry();
    m_infoGeometry->setText(QString("(%1, %2) %3×%4").arg(geo.x()).arg(geo.y()).arg(geo.width()).arg(geo.height()));

    m_infoVisible->setText(m_selected->isVisible() ? "Yes" : "No");
    m_infoEnabled->setText(m_selected->isEnabled() ? "Yes" : "No");
}

void LayoutInspector::updateIssuesPanel()
{
    QStringList issues = detectLayoutIssues();

    if (issues.isEmpty()) {
        m_issuesText->setHtml("<span style='color: green;'><b>✓ No issues detected</b></span>");
    } else {
        QString html = "<b>Potential Issues:</b><ul>";
        for (const QString &issue : issues) {
            html += "<li>" + issue + "</li>";
        }
        html += "</ul>";
        m_issuesText->setHtml(html);
    }
}

void LayoutInspector::populateGridEditor()
{
    if (!m_layout)
        return;

    QGridLayout *g = qobject_cast<QGridLayout *>(m_layout);
    if (!g)
        return;

    m_gridItems->clear();

    int maxR = findMaxGridRow(g);
    int maxC = findMaxGridCol(g);

    for (int r = 0; r <= maxR; ++r) {
        for (int c = 0; c <= maxC; ++c) {
            QLayoutItem *li = g->itemAtPosition(r, c);
            if (!li)
                continue;

            QWidget *w = li->widget();
            if (!w)
                continue;

            QString name = w->objectName().isEmpty() ? w->metaObject()->className() : w->objectName();

            QTreeWidgetItem *it =
                new QTreeWidgetItem(m_gridItems, QStringList(QString("%1 @ row %2, col %3").arg(name).arg(r).arg(c)));
            it->setData(0, Qt::UserRole, QVariant::fromValue((QObject *)w));
        }
    }

    m_gridItems->expandAll();
}

void LayoutInspector::selectWidget(QWidget *w)
{
    m_selected = w;
    updatePropertyPanel();
    updateInfoPanel();

    m_layout = w->layout();
    updateLayoutPanel();
}

void LayoutInspector::refreshAll()
{
    if (m_selected) {
        updatePropertyPanel();
        updateInfoPanel();
    }

    updateLayoutPanel();
    repaintTarget();
}

void LayoutInspector::removeWidgetFromLayout(QLayout *L, QWidget *w)
{
    if (!L || !w)
        return;

    for (int i = 0; i < L->count(); ++i) {
        QLayoutItem *it = L->itemAt(i);
        if (!it)
            continue;

        if (it->widget() == w) {
            L->takeAt(i);
            return;
        }

        if (QLayout *child = it->layout())
            removeWidgetFromLayout(child, w);
    }
}

int LayoutInspector::findIndexByData(QComboBox *cb, int d)
{
    for (int i = 0; i < cb->count(); ++i)
        if (cb->itemData(i).toInt() == d)
            return i;
    return 0;
}

int LayoutInspector::findMaxGridRow(QGridLayout *g)
{
    int maxR = -1;
    for (int i = 0; i < g->count(); ++i) {
        int r, c, rs, cs;
        g->getItemPosition(i, &r, &c, &rs, &cs);
        maxR = std::max(maxR, r + rs - 1);
    }
    return std::max(0, maxR);
}

int LayoutInspector::findMaxGridCol(QGridLayout *g)
{
    int maxC = -1;
    for (int i = 0; i < g->count(); ++i) {
        int r, c, rs, cs;
        g->getItemPosition(i, &r, &c, &rs, &cs);
        maxC = std::max(maxC, c + cs - 1);
    }
    return std::max(0, maxC);
}

void LayoutInspector::repaintTarget()
{
    if (!m_target)
        return;

    QWidget *top = m_target->window();

    if (m_overlay->parentWidget() != top) {
        m_overlay->setParent(top);
        m_overlay->setGeometry(top->rect());
    }

    m_target->updateGeometry();
    m_target->update();
    top->update();

    m_overlay->raise();
    m_overlay->update();
}

QString LayoutInspector::analyzeWidget(QWidget *w)
{
    if (!w)
        return "";

    QString report;
    report += QString("Widget: %1\n").arg(w->metaObject()->className());
    report += QString("Object Name: %1\n").arg(w->objectName());
    report += QString("Size: %1×%2\n").arg(w->width()).arg(w->height());
    report += QString("Size Hint: %1×%2\n").arg(w->sizeHint().width()).arg(w->sizeHint().height());
    report += QString("Minimum: %1×%2\n").arg(w->minimumSize().width()).arg(w->minimumSize().height());
    report += QString("Maximum: %1×%2\n").arg(w->maximumSize().width()).arg(w->maximumSize().height());

    QSizePolicy sp = w->sizePolicy();
    report += QString("H Policy: %1, V Policy: %2\n").arg(sp.horizontalPolicy()).arg(sp.verticalPolicy());

    return report;
}

QString LayoutInspector::analyzeLayout(QLayout *l)
{
    if (!l)
        return "";

    QString report;
    report += QString("Layout: %1\n").arg(l->metaObject()->className());
    report += QString("Spacing: %1\n").arg(l->spacing());

    int left, top, right, bottom;
    l->getContentsMargins(&left, &top, &right, &bottom);
    report += QString("Margins: L=%1, T=%2, R=%3, B=%4\n").arg(left).arg(top).arg(right).arg(bottom);
    report += QString("Item Count: %1\n").arg(l->count());

    return report;
}

QStringList LayoutInspector::detectLayoutIssues()
{
    QStringList issues;

    if (!m_target)
        return issues;

    // Check selected widget
    if (m_selected) {
        // Check for conflicting size constraints
        if (m_selected->minimumWidth() > m_selected->maximumWidth() ||
            m_selected->minimumHeight() > m_selected->maximumHeight()) {
            issues << QString("%1: Minimum size exceeds maximum size!").arg(m_selected->metaObject()->className());
        }

        // Check if size hint is ignored
        QSize hint = m_selected->sizeHint();
        QSize minSize = m_selected->minimumSize();
        if (hint.width() < minSize.width() || hint.height() < minSize.height()) {
            issues << QString("%1: Size hint is smaller than minimum size").arg(m_selected->metaObject()->className());
        }

        // Check for very large widgets
        if (m_selected->width() > 5000 || m_selected->height() > 5000) {
            issues << QString("%1: Widget is unusually large (%2×%3)")
                          .arg(m_selected->metaObject()->className())
                          .arg(m_selected->width())
                          .arg(m_selected->height());
        }

        // Check for invisible but space-taking widgets
        if (!m_selected->isVisible() && m_selected->minimumSize().width() > 0) {
            issues << QString("%1: Widget is hidden but has minimum size constraints")
                          .arg(m_selected->metaObject()->className());
        }
    }

    // Check layout
    if (m_layout) {
        // Check for negative spacing
        if (m_layout->spacing() < 0) {
            issues << "Layout has negative spacing";
        }

        // Check for excessive margins
        int left, top, right, bottom;
        m_layout->getContentsMargins(&left, &top, &right, &bottom);
        if (left > 100 || top > 100 || right > 100 || bottom > 100) {
            issues << QString("Layout has very large margins (L=%1, T=%2, R=%3, B=%4)")
                          .arg(left)
                          .arg(top)
                          .arg(right)
                          .arg(bottom);
        }

        // Check for empty layouts
        if (m_layout->count() == 0) {
            issues << "Layout is empty (no items)";
        }
    }

    return issues;
}

void LayoutInspector::showAttachDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Select Target Widget");
    dlg.resize(600, 700);

    QVBoxLayout *v = new QVBoxLayout(&dlg);

    QLineEdit *search = new QLineEdit;
    search->setPlaceholderText("Search widgets by name or class...");
    v->addWidget(search);

    QTreeWidget *tree = new QTreeWidget;
    tree->setHeaderLabel("Widget / Layout Hierarchy");
    v->addWidget(tree, 1);

    QHBoxLayout *h = new QHBoxLayout;
    QPushButton *ok = new QPushButton("Attach");
    QPushButton *cancel = new QPushButton("Cancel");
    h->addStretch(1);
    h->addWidget(ok);
    h->addWidget(cancel);
    v->addLayout(h);

    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    tree->clear();

    const QList<QWidget *> tops = QApplication::topLevelWidgets();

    QWidget *mainWindow = nullptr;
    QList<QWidget *> userWindows;
    QList<QWidget *> internalWindows;

    for (QWidget *w : tops) {
        QString cls = w->metaObject()->className();

        if (!mainWindow && qobject_cast<QMainWindow *>(w)) {
            mainWindow = w;
            continue;
        }

        bool isInternal = cls.contains("Private", Qt::CaseInsensitive) ||
                          cls.contains("Container", Qt::CaseInsensitive) ||
                          cls.contains("Popup", Qt::CaseInsensitive) || cls.contains("QComboBoxPrivate") ||
                          cls.contains("QMenu") || cls.contains("QToolTip") || cls.contains("QWhatsThis") ||
                          cls.contains("QCompleter") || (w->windowFlags() & Qt::Popup);

        if (isInternal) {
            internalWindows.append(w);
        } else {
            userWindows.append(w);
        }
    }

    if (mainWindow) {
        populateWidgetTree(tree, mainWindow, nullptr);
    }

    for (QWidget *w : userWindows) {
        populateWidgetTree(tree, w, nullptr);
    }

    if (!internalWindows.isEmpty()) {
        QTreeWidgetItem *internalRoot = new QTreeWidgetItem(tree, QStringList{"<Internal Qt Widgets>"});
        internalRoot->setFlags(internalRoot->flags() & ~Qt::ItemIsSelectable);

        for (QWidget *w : internalWindows) {
            populateWidgetTree(tree, w, internalRoot);
        }
    }

    tree->expandToDepth(2);

    connect(search, &QLineEdit::textChanged, tree, [tree](const QString &t) {
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            bool match = (*it)->text(0).contains(t, Qt::CaseInsensitive);
            (*it)->setHidden(!match && !t.isEmpty());
            ++it;
        }
    });

    connect(ok, &QPushButton::clicked, this, [&]() {
        QTreeWidgetItem *it = tree->currentItem();
        if (!it)
            return;

        QObject *obj = it->data(0, Qt::UserRole).value<QObject *>();
        QWidget *w = qobject_cast<QWidget *>(obj);

        if (!w) {
            QMessageBox::warning(&dlg, "Invalid Selection", "Selected item is not a QWidget.");
            return;
        }

        attachTo(w);
        dlg.accept();
    });

    dlg.exec();
}

void LayoutInspector::populateLayoutTree(QTreeWidgetItem *parentItem, QLayout *layout)
{
    if (!layout)
        return;

    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem *li = layout->itemAt(i);

        if (li->spacerItem()) {
            auto *it = new QTreeWidgetItem(parentItem, QStringList{"QSpacerItem"});
            it->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<QObject *>(nullptr)));
            continue;
        }

        if (QLayout *childLayout = li->layout()) {
            auto *it = new QTreeWidgetItem(
                parentItem, QStringList{QString("Layout: %1").arg(childLayout->metaObject()->className())});
            it->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<QObject *>(childLayout)));
            populateLayoutTree(it, childLayout);
            continue;
        }

        if (QWidget *w = li->widget()) {
            QString name = w->objectName().isEmpty()
                               ? QString("%1").arg(w->metaObject()->className())
                               : QString("%1 (%2)").arg(w->objectName()).arg(w->metaObject()->className());

            auto *it = new QTreeWidgetItem(parentItem, QStringList{name});
            it->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<QObject *>(w)));
        }
    }
}

void LayoutInspector::populateWidgetTree(QTreeWidget *tree, QWidget *widget, QTreeWidgetItem *parent)
{
    if (!widget)
        return;

    QString label = widget->objectName().isEmpty()
                        ? QString("%1").arg(widget->metaObject()->className())
                        : QString("%1 (%2)").arg(widget->objectName()).arg(widget->metaObject()->className());

    QTreeWidgetItem *item =
        parent ? new QTreeWidgetItem(parent, QStringList(label)) : new QTreeWidgetItem(tree, QStringList(label));

    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<QObject *>(widget)));

    if (QLayout *L = widget->layout()) {
        QString lname = QString("Layout: %1").arg(L->metaObject()->className());
        QTreeWidgetItem *layoutItem = new QTreeWidgetItem(item, QStringList(lname));
        layoutItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<QObject *>(L)));
        populateLayoutTree(layoutItem, L);
    }

    for (QObject *obj : widget->children()) {
        if (QWidget *w = qobject_cast<QWidget *>(obj)) {
            populateWidgetTree(tree, w, item);
        }
    }
}