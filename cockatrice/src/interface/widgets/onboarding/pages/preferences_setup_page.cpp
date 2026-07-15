#include "preferences_setup_page.h"

#include "../../client/settings/cache_settings.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
QGroupBox *makeGroup(QWidget *parent, QVBoxLayout *outerLayout)
{
    auto *group = new QGroupBox(parent);
    new QVBoxLayout(group);
    outerLayout->addWidget(group);
    return group;
}
} // namespace

PreferencesSetupPage::PreferencesSetupPage(QWidget *parent) : FirstRunWizardPage(parent)
{
    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);

    appearanceGroup = makeGroup(content, contentLayout);
    styleUserListCheckBox = new QCheckBox(appearanceGroup);
    cardScalingCheckBox = new QCheckBox(appearanceGroup);
    roundCardCornersCheckBox = new QCheckBox(appearanceGroup);
    appearanceGroup->layout()->addWidget(styleUserListCheckBox);
    appearanceGroup->layout()->addWidget(cardScalingCheckBox);
    appearanceGroup->layout()->addWidget(roundCardCornersCheckBox);

    notificationsGroup = makeGroup(content, contentLayout);
    notificationsEnabledCheckBox = new QCheckBox(notificationsGroup);
    soundEnabledCheckBox = new QCheckBox(notificationsGroup);
    notificationsGroup->layout()->addWidget(notificationsEnabledCheckBox);
    notificationsGroup->layout()->addWidget(soundEnabledCheckBox);

    gameplayGroup = makeGroup(content, contentLayout);
    doubleClickToPlayCheckBox = new QCheckBox(gameplayGroup);
    gameplayGroup->layout()->addWidget(doubleClickToPlayCheckBox);

    menuGroup = makeGroup(content, contentLayout);
    showShortcutsCheckBox = new QCheckBox(menuGroup);
    menuGroup->layout()->addWidget(showShortcutsCheckBox);

    dataGroup = makeGroup(content, contentLayout);
    picDownloadCheckBox = new QCheckBox(dataGroup);
    checkUpdatesOnStartupCheckBox = new QCheckBox(dataGroup);
    showTipsOnStartupCheckBox = new QCheckBox(dataGroup);
    dataGroup->layout()->addWidget(picDownloadCheckBox);
    dataGroup->layout()->addWidget(checkUpdatesOnStartupCheckBox);
    dataGroup->layout()->addWidget(showTipsOnStartupCheckBox);

    contentLayout->addStretch();

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(content);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(scrollArea);

    SettingsCache &settings = SettingsCache::instance();

    connect(styleUserListCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setStyleUserList);
    connect(cardScalingCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setCardScaling);
    connect(roundCardCornersCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setRoundCardCorners);

    connect(notificationsEnabledCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setNotificationsEnabled);
    connect(soundEnabledCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setSoundEnabled);

    connect(doubleClickToPlayCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setDoubleClickToPlay);

    /*connect(showShortcutsCheckBox, &QCheckBox::checkStateChanged, this,
            [](bool checked) { SettingsCache::instance().setShowShortcuts(checked); });*/

    connect(picDownloadCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setPicDownload);
    connect(checkUpdatesOnStartupCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setCheckUpdatesOnStartup);
    connect(showTipsOnStartupCheckBox, &QCheckBox::checkStateChanged, &settings, &SettingsCache::setShowTipsOnStartup);

    retranslateUi();
}

void PreferencesSetupPage::initializePage()
{
    SettingsCache &settings = SettingsCache::instance();

    styleUserListCheckBox->setChecked(settings.getStyleUserList());
    cardScalingCheckBox->setChecked(settings.getScaleCards());
    roundCardCornersCheckBox->setChecked(settings.getRoundCardCorners());

    notificationsEnabledCheckBox->setChecked(settings.getNotificationsEnabled());
    soundEnabledCheckBox->setChecked(settings.getSoundEnabled());

    doubleClickToPlayCheckBox->setChecked(settings.getDoubleClickToPlay());

    showShortcutsCheckBox->setChecked(settings.getShowShortcuts());

    picDownloadCheckBox->setChecked(settings.getPicDownload());
    checkUpdatesOnStartupCheckBox->setChecked(settings.getCheckUpdatesOnStartup());
    showTipsOnStartupCheckBox->setChecked(settings.getShowTipsOnStartup());
}

bool PreferencesSetupPage::isSkippable() const
{
    return true;
}

QString PreferencesSetupPage::stepTitle() const
{
    return tr("A Few Preferences");
}

QString PreferencesSetupPage::stepSubtitle() const
{
    return tr("Defaults are fine — tweak these now or from Settings anytime.");
}

void PreferencesSetupPage::retranslateUi()
{
    appearanceGroup->setTitle(tr("Appearance"));
    styleUserListCheckBox->setText(tr("Use the styled user list (avatars, role colours)"));
    cardScalingCheckBox->setText(tr("Scale cards to fit the window"));
    roundCardCornersCheckBox->setText(tr("Round card corners"));

    notificationsGroup->setTitle(tr("Notifications && Sound"));
    notificationsEnabledCheckBox->setText(tr("Show desktop notifications"));
    soundEnabledCheckBox->setText(tr("Play sound effects"));

    gameplayGroup->setTitle(tr("Gameplay"));
    doubleClickToPlayCheckBox->setText(tr("Double-click a card to play it"));

    menuGroup->setTitle(tr("Menus"));
    showShortcutsCheckBox->setText(tr("Show keyboard shortcuts in menus"));

    dataGroup->setTitle(tr("Updates && Data"));
    picDownloadCheckBox->setText(tr("Automatically download card images"));
    picDownloadCheckBox->setToolTip(tr("Turn this off if you're on a limited connection — "
                                       "card art just won't load until you turn it back on."));
    checkUpdatesOnStartupCheckBox->setText(tr("Check for client updates on startup"));
    showTipsOnStartupCheckBox->setText(tr("Show tip of the day on startup"));
}