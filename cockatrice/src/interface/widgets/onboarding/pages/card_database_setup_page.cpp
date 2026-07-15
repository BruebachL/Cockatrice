#include "card_database_setup_page.h"

#include "../../client/settings/cache_settings.h"

#include <libcockatrice/card/database/card_database_manager.h>

#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

CardDatabaseSetupPage::CardDatabaseSetupPage(QWidget *parent) : FirstRunWizardPage(parent)
{
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    statusLabel->setAlignment(Qt::AlignCenter);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0); // indeterminate -- the subprocess doesn't report byte-level progress to us
    progressBar->setTextVisible(false);
    progressBar->setFixedWidth(280);

    retryButton = new QPushButton(this);
    manualButton = new QPushButton(this);

    connect(retryButton, &QPushButton::clicked, this, [this] {
        setState(State::Running);
        emit updateRequested();
    });
    connect(manualButton, &QPushButton::clicked, this, &CardDatabaseSetupPage::manualSetupRequested);

    // ── Advanced: custom download source ───────────────────────────────
    advancedToggleButton = new QPushButton(this);
    advancedToggleButton->setCheckable(true);
    advancedToggleButton->setChecked(false);
    advancedToggleButton->setFlat(true);
    advancedToggleButton->setStyleSheet("QPushButton { text-align: left; padding: 5px 12px; font-weight: bold; }"
                                        "QPushButton:checked { }");

    advancedPanel = new QWidget(this);
    advancedPanel->setVisible(false);

    urlLineEdit = new QLineEdit(advancedPanel);
    urlHintLabel = new QLabel(advancedPanel);
    urlHintLabel->setWordWrap(true);

    restoreDefaultUrlButton = new QPushButton(advancedPanel);
    applyAndRetryButton = new QPushButton(advancedPanel);

    connect(advancedToggleButton, &QPushButton::toggled, this, &CardDatabaseSetupPage::onToggleAdvanced);
    connect(restoreDefaultUrlButton, &QPushButton::clicked, this, &CardDatabaseSetupPage::onRestoreDefaultUrl);
    connect(applyAndRetryButton, &QPushButton::clicked, this, &CardDatabaseSetupPage::onApplyCustomUrl);

    auto *advancedButtonRow = new QHBoxLayout;
    advancedButtonRow->addWidget(restoreDefaultUrlButton);
    advancedButtonRow->addStretch();
    advancedButtonRow->addWidget(applyAndRetryButton);

    auto *advancedLayout = new QVBoxLayout(advancedPanel);
    advancedLayout->setContentsMargins(12, 4, 12, 4);
    advancedLayout->addWidget(urlLineEdit);
    advancedLayout->addWidget(urlHintLabel);
    advancedLayout->addLayout(advancedButtonRow);

    auto *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(statusLabel);
    layout->addSpacing(12);
    layout->addWidget(progressBar, 0, Qt::AlignHCenter);
    layout->addSpacing(12);
    layout->addWidget(retryButton, 0, Qt::AlignHCenter);
    layout->addWidget(manualButton, 0, Qt::AlignHCenter);
    layout->addSpacing(16);
    layout->addWidget(advancedToggleButton);
    layout->addWidget(advancedPanel);
    layout->addStretch();

    retranslateUi();
}

bool CardDatabaseSetupPage::alreadyHaveDatabase() const
{
    return CardDatabaseManager::getInstance()->getCardList().count() > 0;
}

QString CardDatabaseSetupPage::oracleSettingsFilePath() const
{
    // Same file OracleWizard reads/writes ("oracle.ini" in the shared settings
    // path) and the same key LoadSetsPage falls back from -- so a URL set
    // here is picked up identically by the background updater and by the
    // full interactive Oracle wizard if the user opens that later.
    return SettingsCache::instance().getSettingsPath() + "oracle.ini";
}

QString CardDatabaseSetupPage::readCustomUrl() const
{
    QSettings oracleSettings(oracleSettingsFilePath(), QSettings::IniFormat);
    return oracleSettings.value("allsetsurl").toString();
}

void CardDatabaseSetupPage::writeCustomUrl(const QString &url)
{
    QSettings oracleSettings(oracleSettingsFilePath(), QSettings::IniFormat);
    if (url.isEmpty()) {
        // Matches LoadSetsPage::actDownloadFinishedSetsFile's own behaviour:
        // absence of the key, not an empty value, is what means "use default".
        oracleSettings.remove("allsetsurl");
    } else {
        oracleSettings.setValue("allsetsurl", url);
    }
}

void CardDatabaseSetupPage::initializePage()
{
    urlLineEdit->setText(readCustomUrl());

    if (state != State::NotStarted) {
        return; // don't restart if the user navigated back and forward
    }

    if (alreadyHaveDatabase()) {
        setState(State::Succeeded);
        return;
    }

    setState(State::Running);
    emit updateRequested();
}

void CardDatabaseSetupPage::onUpdateFinished(bool success)
{
    setState(success ? State::Succeeded : State::Failed);
}

void CardDatabaseSetupPage::onToggleAdvanced(bool open)
{
    advancedToggleButton->setText(open ? tr("▼  Advanced: custom download source")
                                       : tr("▶  Advanced: custom download source"));
    advancedPanel->setVisible(open);
}

void CardDatabaseSetupPage::onApplyCustomUrl()
{
    const QString text = urlLineEdit->text().trimmed();

    if (!text.isEmpty()) {
        const QUrl url = QUrl::fromUserInput(text);
        if (!url.isValid()) {
            QMessageBox::warning(this, tr("Invalid URL"),
                                 tr("That doesn't look like a valid URL. Double-check it and try again, "
                                    "or clear the field to use the default source."));
            return;
        }
    }

    writeCustomUrl(text);
    setState(State::Running);
    emit updateRequested();
}

void CardDatabaseSetupPage::onRestoreDefaultUrl()
{
    urlLineEdit->clear();
    writeCustomUrl(QString());
}

void CardDatabaseSetupPage::setState(State newState)
{
    state = newState;

    progressBar->setVisible(state == State::Running);
    retryButton->setVisible(state == State::Failed);
    manualButton->setVisible(state == State::Failed);
    applyAndRetryButton->setEnabled(state != State::Running);

    switch (state) {
        case State::NotStarted:
            break;
        case State::Running:
            statusLabel->setText(tr("Downloading the latest card database…"));
            break;
        case State::Succeeded:
            statusLabel->setText(tr("Card database ready ✓"));
            break;
        case State::Failed:
            statusLabel->setText(
                tr("Couldn't download the card database automatically. Check your connection and retry, "
                   "set it up manually, or skip this for now — you can do it later from the Card Database menu."));
            break;
    }

    emit completeChanged();
}

bool CardDatabaseSetupPage::isComplete() const
{
    // Never block progress on this: worst case the user plays without card
    // data and fixes it later from the menu, exactly as they could today.
    return state != State::Running;
}

bool CardDatabaseSetupPage::isSkippable() const
{
    return state != State::Succeeded;
}

QString CardDatabaseSetupPage::stepTitle() const
{
    return tr("Card Database");
}

QString CardDatabaseSetupPage::stepSubtitle() const
{
    return tr("Cockatrice needs card data to know what you're playing with.");
}

void CardDatabaseSetupPage::retranslateUi()
{
    retryButton->setText(tr("Retry"));
    manualButton->setText(tr("Set up manually…"));

    onToggleAdvanced(advancedToggleButton->isChecked());
    urlLineEdit->setPlaceholderText(tr("Leave blank to use the default source"));
    urlHintLabel->setText(tr("Only change this if you know you need a mirror or a custom card data source."));
    restoreDefaultUrlButton->setText(tr("Restore default"));
    applyAndRetryButton->setText(tr("Apply && retry"));

    setState(state); // refresh status text in the new language
}