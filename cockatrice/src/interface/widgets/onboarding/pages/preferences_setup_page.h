#ifndef PREFERENCES_SETUP_PAGE_H
#define PREFERENCES_SETUP_PAGE_H

#include "../first_run_wizard_page.h"

class QCheckBox;
class QGroupBox;

/**
 * A curated subset of settings scattered across AppearanceSettingsPage and
 * general SettingsCache toggles. Reads/writes the exact same SettingsCache
 * keys so nothing set here can drift from what Settings shows afterwards.
 *
 * NOTE ON SETTER NAMES: getScaleCards()/setCardScaling(),
 * getShowShortcuts()/setShowShortcuts(), and getRoundCardCorners()/
 * setRoundCardCorners() are confirmed from AppearanceSettingsPage.
 * getCheckUpdatesOnStartup() and getShowTipsOnStartup() are confirmed
 * (used in MainWindow::startupConfigCheck). The setters for those two,
 * plus all of notificationsEnabled/soundEnabled/picDownload/
 * doubleClickToPlay, are inferred from the get/set-by-member-name
 * convention those examples establish -- flag any that don't match.
 */
class PreferencesSetupPage : public FirstRunWizardPage
{
    Q_OBJECT

public:
    explicit PreferencesSetupPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isSkippable() const override;
    QString stepTitle() const override;
    QString stepSubtitle() const override;
    void retranslateUi() override;

private:
    QGroupBox *appearanceGroup;
    QCheckBox *styleUserListCheckBox;
    QCheckBox *cardScalingCheckBox;
    QCheckBox *roundCardCornersCheckBox;

    QGroupBox *notificationsGroup;
    QCheckBox *notificationsEnabledCheckBox;
    QCheckBox *soundEnabledCheckBox;

    QGroupBox *gameplayGroup;
    QCheckBox *doubleClickToPlayCheckBox;

    QGroupBox *menuGroup;
    QCheckBox *showShortcutsCheckBox;

    QGroupBox *dataGroup;
    QCheckBox *picDownloadCheckBox;
    QCheckBox *checkUpdatesOnStartupCheckBox;
    QCheckBox *showTipsOnStartupCheckBox;
};

#endif // PREFERENCES_SETUP_PAGE_H