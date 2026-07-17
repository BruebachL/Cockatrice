/**
 * @file deck_filter_string.h
 * @ingroup DeckStorageWidgets
 */
//! \todo Document this file.

#ifndef DECK_FILTER_STRING_H
#define DECK_FILTER_STRING_H

#include "../interface/deck_loader/deck_loader.h"

#include <QLoggingCategory>
#include <QString>
#include <functional>

inline Q_LOGGING_CATEGORY(DeckFilterStringLog, "deck_filter_string");

/**
 * Data needed for deck filtering, decoupled from any widget.
 */
struct DeckFilterData
{
    QString filePath;
    DeckLoader *deckLoader;
};

/**
 * Extra info relevant to filtering that isn't present in the DeckFilterData
 */
struct ExtraDeckSearchInfo
{
    /**
     * The relative filepath starting from the deck folder
     */
    QString relativeFilePath;
};

typedef std::function<bool(const DeckFilterData &, const ExtraDeckSearchInfo &)> DeckFilter;

class DeckFilterString
{
public:
    DeckFilterString();
    explicit DeckFilterString(const QString &expr);
    bool check(const DeckFilterData &deck, const ExtraDeckSearchInfo &info) const
    {
        return filter(deck, info);
    }

    [[nodiscard]] bool valid() const
    {
        return _error.isEmpty();
    }

    QString error()
    {
        return _error;
    }

private:
    QString _error;
    DeckFilter filter;
};
#endif // DECK_FILTER_STRING_H
