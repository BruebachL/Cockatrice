#ifndef TAG_SETTINGS_H
#define TAG_SETTINGS_H

#include "settings_manager.h"

class TagsSettings : public SettingsManager
{
    Q_OBJECT
    friend class SettingsCache;

    explicit TagsSettings(const QString &settingPath, QObject *parent = nullptr);
    TagsSettings(const TagsSettings & /*other*/);

public:
    QStringList getDefaultTags();

    void setDefaultTags(const QStringList &tags);

signals:
    void defaultTagsChanged();
};

#endif // TAG_SETTINGS_H
