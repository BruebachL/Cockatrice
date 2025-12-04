#include "deck_analytics_widget.h"

#include "add_analytics_panel_dialog.h"
#include "analyzer_modules/mana_base/mana_base_add_dialog.h"
#include "analyzer_modules/mana_curve/mana_curve_add_dialog.h"
#include "analyzer_modules/mana_devotion/mana_devotion_add_dialog.h"
#include "deck_analytics_widget_base.h"
#include "deck_analytics_widget_factory.h"
#include "deck_list_statistics_analyzer.h"
#include "mana_base_widget.h"
#include "mana_curve_widget.h"
#include "mana_devotion_widget.h"

#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

DeckAnalyticsWidget::DeckAnalyticsWidget(QWidget *parent, DeckListModel *model) : QWidget(parent)
{
    AnalyticsWidgetFactory::instance().registerType(
        "manaBase", [](QWidget *p, DeckListStatisticsAnalyzer *an) { return new ManaBaseWidget(p, an); });
    AnalyticsWidgetFactory::instance().registerType(
        "manaCurve", [](QWidget *p, DeckListStatisticsAnalyzer *an) { return new ManaCurveWidget(p, an); });
    AnalyticsWidgetFactory::instance().registerType(
        "manaDevotion", [](QWidget *p, DeckListStatisticsAnalyzer *an) { return new ManaDevotionWidget(p, an); });

    QVBoxLayout *layout = new QVBoxLayout(this);

    // controls
    QHBoxLayout *ctl = new QHBoxLayout();
    auto addBtn = new QPushButton(tr("Add Panel"), this);
    auto removeBtn = new QPushButton(tr("Remove Selected"), this);
    auto saveBtn = new QPushButton(tr("Save Layout"), this);
    auto loadBtn = new QPushButton(tr("Load Layout"), this);
    ctl->addWidget(addBtn);
    ctl->addWidget(removeBtn);
    ctl->addWidget(saveBtn);
    ctl->addWidget(loadBtn);
    layout->addLayout(ctl);

    connect(addBtn, &QPushButton::clicked, this, &DeckAnalyticsWidget::onAddPanel);
    connect(removeBtn, &QPushButton::clicked, this, &DeckAnalyticsWidget::onRemoveSelected);
    connect(saveBtn, &QPushButton::clicked, this, &DeckAnalyticsWidget::saveLayout);
    connect(loadBtn, &QPushButton::clicked, this, &DeckAnalyticsWidget::loadLayout);

    listWidget = new QListWidget(this);
    listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    listWidget->setDefaultDropAction(Qt::MoveAction);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(listWidget);

    statsAnalyzer = new DeckListStatisticsAnalyzer(this, model);
    statsAnalyzer->analyze();

    loadLayout();
}

void DeckAnalyticsWidget::updateDisplays()
{
    statsAnalyzer->analyze();
}

void DeckAnalyticsWidget::onAddPanel()
{
    AddAnalyticsPanelDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString sel = dlg.selectedType();
    if (sel.isEmpty())
        return;

    // create empty widget first
    AnalyticsWidgetBase *w = AnalyticsWidgetFactory::instance().create(sel, this, statsAnalyzer);
    if (!w)
        return;

    // run widget's own config dialog
    if (!w->applyConfigFromDialog()) {
        w->deleteLater();
        return;
    }

    addPanelInstance(w, w->saveConfig());
}

void DeckAnalyticsWidget::addPanelInstance(AnalyticsWidgetBase *panel, const QJsonObject &cfg)
{
    QListWidgetItem *itm = new QListWidgetItem(listWidget);
    // Ask widget for a reasonable size and enforce a minimum
    QSize sz = panel->sizeHint();
    if (sz.height() < 300)
        sz.setHeight(300); // good default minimum

    itm->setSizeHint(sz);
    listWidget->addItem(itm);
    listWidget->setItemWidget(itm, panel);
    panel->loadConfig(cfg);
    panel->updateDisplay();
}

void DeckAnalyticsWidget::onRemoveSelected()
{
    auto it = listWidget->currentRow();
    if (it < 0)
        return;
    QListWidgetItem *itm = listWidget->takeItem(it);
    if (!itm)
        return;
    QWidget *w = listWidget->itemWidget(itm);
    if (w) {
        w->deleteLater();
    }
    delete itm;
}

void DeckAnalyticsWidget::saveLayout()
{
    QJsonArray arr;
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *itm = listWidget->item(i);
        QWidget *w = listWidget->itemWidget(itm);
        auto *panel = qobject_cast<AnalyticsWidgetBase *>(w);
        if (!panel)
            continue;
        QJsonObject entry;
        entry["type"] = panel->widgetType();
        entry["config"] = panel->saveConfig();
        arr.append(entry);
    }
    QJsonDocument doc(arr);
    QSettings s;
    s.setValue("deckAnalytics/layout", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void DeckAnalyticsWidget::loadLayout()
{
    // when manually triggered, clear + reload
    if (!loadLayoutInternal()) {
        // user explicitly asked to load, but no layout existed
        // optional: show a small message
        addDefaultPanels();
    }
}

void DeckAnalyticsWidget::addDefaultPanels()
{
    // Example default layout. You can change these.
    struct DefaultPanel
    {
        QString type;
        QJsonObject defaultConfig;
    };

    QVector<DefaultPanel> defaults = {{"manaCurve", {}}, {"manaBase", {}}, {"manaDevotion", {}}};

    for (const auto &d : defaults) {
        AnalyticsWidgetBase *w = AnalyticsWidgetFactory::instance().create(d.type, this, statsAnalyzer);
        if (!w)
            continue;

        w->loadConfig(d.defaultConfig);
        addPanelInstance(w, d.defaultConfig);
    }
}

bool DeckAnalyticsWidget::loadLayoutInternal()
{
    QSettings s;
    QString layoutData = s.value("deckAnalytics/layout").toString();
    if (layoutData.isEmpty())
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(layoutData.toUtf8());
    if (!doc.isArray())
        return false;

    QJsonArray arr = doc.array();
    if (arr.isEmpty())
        return false;

    // clear existing (should be empty during ctor)
    while (listWidget->count()) {
        auto itm = listWidget->takeItem(0);
        QWidget *w = listWidget->itemWidget(itm);
        if (w)
            w->deleteLater();
        delete itm;
    }

    for (auto v : arr) {
        if (!v.isObject())
            continue;

        QJsonObject o = v.toObject();
        QString type = o["type"].toString();
        QJsonObject cfg = o["config"].toObject();

        AnalyticsWidgetBase *w = AnalyticsWidgetFactory::instance().create(type, this, statsAnalyzer);
        if (!w)
            continue;

        w->loadConfig(cfg);
        addPanelInstance(w, cfg);
    }

    return true;
}
