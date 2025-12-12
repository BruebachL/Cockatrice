#ifndef COCKATRICE_LAYOUT_INSPECTOR_H
#define COCKATRICE_LAYOUT_INSPECTOR_H
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QWidget>

class LayoutOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit LayoutOverlay(QWidget *parent = nullptr);
    void setTarget(QWidget *t);
    void setEnabledOverlay(bool en);
    void setShowDimensions(bool show);
    void setShowMargins(bool show);
    void setHighlightSelected(QWidget *selected);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QWidget *m_target;
    QWidget *m_selected;
    bool m_showDimensions;
    bool m_showMargins;
};

class LayoutInspector : public QWidget
{
    Q_OBJECT
public:
    explicit LayoutInspector(QWidget *target = nullptr, QWidget *parent = nullptr);
    void attachTo(QWidget *target);
    QWidget *target() const
    {
        return m_target;
    }

private slots:
    void onTreeSelection();
    void applyLayoutChanges();
    void applyWidgetChanges();
    void onRefresh();
    void onOverlayToggle(int st);
    void onShowDimensionsToggle(int st);
    void onShowMarginsToggle(int st);
    void onStretchChanged(int idx);
    void onMoveGridItem();
    void onGridSelect(QTreeWidgetItem *it, int);
    void onSetAlignment();
    void onJumpToWidget();
    void onHighlightWidget();
    void onAnalyzeLayout();
    void onExportLayoutInfo();
    void onCompareMinimumSizes();

private:
    void buildUi();
    void populatePolicyBox(QComboBox *box);
    void populateTree();
    void addChildrenToItem(QWidget *w, QTreeWidgetItem *parent);
    void refreshAll();
    void selectWidget(QWidget *w);
    void updatePropertyPanel();
    void updateLayoutPanel();
    void updateInfoPanel();
    void populateGridEditor();
    int findIndexByData(QComboBox *cb, int d);
    int findMaxGridRow(QGridLayout *g);
    int findMaxGridCol(QGridLayout *g);
    void removeWidgetFromLayout(QLayout *L, QWidget *w);
    void repaintTarget();
    void showAttachDialog();
    void populateLayoutTree(QTreeWidgetItem *parentItem, QLayout *layout);
    void populateWidgetTree(QTreeWidget *tree, QWidget *widget, QTreeWidgetItem *parent);
    QString analyzeWidget(QWidget *w);
    QString analyzeLayout(QLayout *l);
    QStringList detectLayoutIssues();
    void updateIssuesPanel();

private:
    QWidget *m_target = nullptr;
    QWidget *m_selected = nullptr;
    QLayout *m_layout = nullptr;

    QTreeWidget *m_tree;
    QStackedWidget *m_stack;

    // Info panel
    QLabel *m_infoWidget;
    QLabel *m_infoSize;
    QLabel *m_infoSizeHint;
    QLabel *m_infoMinSize;
    QLabel *m_infoMaxSize;
    QLabel *m_infoGeometry;
    QLabel *m_infoVisible;
    QLabel *m_infoEnabled;
    QTextEdit *m_issuesText;

    // Widget panel
    QComboBox *m_hPolicy;
    QComboBox *m_vPolicy;
    QSpinBox *m_minWSpin;
    QSpinBox *m_maxWSpin;
    QSpinBox *m_minHSpin;
    QSpinBox *m_maxHSpin;
    QCheckBox *m_fixedCheck;
    QSpinBox *m_fixedWSpin;
    QSpinBox *m_fixedHSpin;

    // Layout panel
    QSpinBox *m_spacingSpin;
    QSpinBox *m_marginSpin;
    QSpinBox *m_marginLeft;
    QSpinBox *m_marginTop;
    QSpinBox *m_marginRight;
    QSpinBox *m_marginBottom;
    QWidget *m_stretchArea;
    QVBoxLayout *m_stretchLayout;
    QWidget *m_gridEditor;
    QTreeWidget *m_gridItems;
    QWidget *m_gridItemWidget = nullptr;

    // Alignment
    QCheckBox *m_alignLeft;
    QCheckBox *m_alignHCenter;
    QCheckBox *m_alignRight;
    QCheckBox *m_alignTop;
    QCheckBox *m_alignVCenter;
    QCheckBox *m_alignBottom;

    // Overlay controls
    QCheckBox *m_overlayToggle;
    QCheckBox *m_showDimensions;
    QCheckBox *m_showMargins;
    LayoutOverlay *m_overlay;
};

#endif // COCKATRICE_LAYOUT_INSPECTOR_H