/**
 * @file user_data_backup.h
 * @ingroup Utility
 * @brief Helpers to detect and back up user data stored inside the install directory.
 *
 * Background: Windows uninstallers shipped in 3.1.0/3.1.1 ran a blanket
 * "RMDir /r $INSTDIR". For portable installs that deletes $INSTDIR\\data
 * (decks, settings, pictures) along with the payload, and for normal installs
 * it deletes anything the user pointed into $INSTDIR (e.g. a custom deck
 * folder). The fixed uninstaller preserves foreign files, but the destructive
 * one is already on disk on affected machines, so the client keeps a safety
 * copy of at-risk data outside the install directory and warns the user.
 */

#ifndef USER_DATA_BACKUP_H
#define USER_DATA_BACKUP_H

#include <QString>
#include <QStringList>

namespace UserDataBackup
{

/**
 * @brief Returns true when @p path lives inside @p dir (or equals it).
 *
 * Both sides are canonicalized (symlinks, `.`, `..`, separators resolved)
 * before comparison. On Windows the comparison is case-insensitive.
 */
bool isPathInsideDirectory(const QString &path, const QString &dir);

/**
 * @brief Filters @p paths down to the ones inside @p dir.
 */
QStringList filterPathsInsideDirectory(const QStringList &paths, const QString &dir);

/**
 * @brief Recursively copies @p sourceDir into @p destDir, creating it as needed.
 * @returns true when every file was copied (or there was nothing to copy).
 */
bool copyDirectoryRecursively(const QString &sourceDir, const QString &destDir);

/**
 * @brief Copies deck files (*.cod, *.dec, *.mwdeck) found directly in @p sourceDir to @p destDir.
 * @returns number of files copied, or -1 on error.
 */
int backupDeckFiles(const QString &sourceDir, const QString &destDir);

/**
 * @brief Backs up an at-risk data directory (e.g. portable $INSTDIR/data) to a timestamped
 * folder under @p backupRoot.
 * @param sourceDir directory holding user data (must exist).
 * @param backupRoot directory OUTSIDE the install tree that will hold timestamped backups.
 * @param backupLocationOut receives the created backup directory on success.
 * @returns true on success.
 */
bool backupAtRiskData(const QString &sourceDir, const QString &backupRoot, QString *backupLocationOut = nullptr);

/**
 * @brief Default backup root guaranteed to live outside the install directory.
 */
QString defaultBackupRoot();

} // namespace UserDataBackup

#endif // USER_DATA_BACKUP_H
