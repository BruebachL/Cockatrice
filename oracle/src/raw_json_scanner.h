#ifndef RAW_JSON_SCANNER_H
#define RAW_JSON_SCANNER_H

#include <QByteArray>
#include <QList>
#include <QString>

namespace RawJson
{

struct SetRange
{
    /** @brief Byte offset of the set's object within the scanned buffer. */
    qsizetype start = -1;
    /** @brief Byte length of the set's object, including the surrounding braces. */
    qsizetype length = 0;
    /** @brief Number of entries in the set's "cards" array. */
    int cardCount = 0;
    QString code;
    QString name;
    QString type;
    QString releaseDate;
};

struct ScanError
{
    bool isError() const
    {
        return !message.isEmpty();
    }
    QString message;
};

/**
 * @brief Scans a full MTGJSON document without materializing the JSON tree.
 *
 * Splits the top-level "data" object into per-set byte ranges and reads each
 * set's metadata directly from the raw bytes. The oracle importer can then
 * parse one set at a time during import, keeping peak memory far below a single
 * QJsonDocument::fromJson() over the whole file.
 *
 * The whole document is structurally validated while scanning (strings,
 * escapes, braces, and a trailing-content check), so malformed input is
 * rejected just like QJsonDocument::fromJson would.
 */
ScanError scanSetRanges(const QByteArray &json, QList<SetRange> &ranges);

} // namespace RawJson

#endif // RAW_JSON_SCANNER_H