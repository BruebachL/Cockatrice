#ifndef CARD_DATABASE_SETUP_PAGE_H
#define CARD_DATABASE_SETUP_PAGE_H

#include "../first_run_wizard_page.h"

class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QWidget;

/**
 * First-run card database step.
 *
 * Deliberately thin: the download/parse/save pipeline lives in the
 * separate "oracle" tool and is already driven from MainWindow via a
 * background QProcess (MainWindow::createCardUpdateProcess). This page
 * asks the wizard's owner to kick that off via updateRequested() and
 * reflects its progress -- it never talks to the network or card database
 * directly, so there's no second copy of that pipeline in the client.
 *
 * The optional "Advanced" panel lets the user override the download source
 * before that background process runs. It writes directly to oracle.ini's
 * "allsetsurl" key -- the exact same file and key LoadSetsPage reads --
 * rather than inventing a parallel settings path, so a URL set here is
 * respected identically whether the background updater or the full
 * interactive Oracle wizard ends up reading it.
 */
class CardDatabaseSetupPage : public FirstRunWizardPage
{
    Q_OBJECT

public:
    explicit CardDatabaseSetupPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    bool isSkippable() const override;
    QString stepTitle() const override;
    QString stepSubtitle() const override;
    void retranslateUi() override;

    //! Called by FirstRunWizard once the background updater process exits.
    void onUpdateFinished(bool success);

signals:
    void updateRequested();
    void manualSetupRequested();

private:
    enum class State
    {
        NotStarted,
        Running,
        Succeeded,
        Failed,
    };

    void setState(State newState);
    bool alreadyHaveDatabase() const;

    QString oracleSettingsFilePath() const;
    QString readCustomUrl() const;
    void writeCustomUrl(const QString &url);

    void onToggleAdvanced(bool open);
    void onApplyCustomUrl();
    void onRestoreDefaultUrl();

    QLabel *statusLabel;
    QProgressBar *progressBar;
    QPushButton *retryButton;
    QPushButton *manualButton;

    QPushButton *advancedToggleButton;
    QWidget *advancedPanel;
    QLineEdit *urlLineEdit;
    QLabel *urlHintLabel;
    QPushButton *restoreDefaultUrlButton;
    QPushButton *applyAndRetryButton;

    State state = State::NotStarted;
};

#endif // CARD_DATABASE_SETUP_PAGE_H