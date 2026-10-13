#include "user_data_backup.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace UserDataBackup
{

static QString canonical(const QString &path)
{
    QString clean = QDir::cleanPath(path);
    QFileInfo info(clean);
    QString resolved = info.canonicalFilePath();
    if (resolved.isEmpty()) {
        // Path does not exist (yet) - fall back to absolute path so prefix
        // comparison still works for configured-but-missing folders.
        resolved = QDir(clean).absolutePath();
    }
#ifdef Q_OS_WIN
    return resolved.toLower();
#else
    return resolved;
#endif
}

bool isPathInsideDirectory(const QString &path, const QString &dir)
{
    if (path.isEmpty() || dir.isEmpty()) {
        return false;
    }
    QString canonPath = canonical(path);
    QString canonDir = canonical(dir);
    if (canonPath == canonDir) {
        return true;
    }
    if (!canonDir.endsWith('/')) {
        canonDir += '/';
    }
    return canonPath.startsWith(canonDir);
}

QStringList filterPathsInsideDirectory(const QStringList &paths, const QString &dir)
{
    QStringList result;
    for (const QString &path : paths) {
        if (isPathInsideDirectory(path, dir)) {
            result.append(path);
        }
    }
    return result;
}

bool copyDirectoryRecursively(const QString &sourceDir, const QString &destDir)
{
    QDir source(sourceDir);
    if (!source.exists()) {
        return false;
    }
    QDir dest(destDir);
    if (!dest.exists() && !QDir().mkpath(destDir)) {
        return false;
    }

    bool ok = true;
    QDirIterator it(sourceDir, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    const int prefixLength = QDir(sourceDir).absolutePath().length();
    while (it.hasNext()) {
        it.next();
        QString relative = QDir(it.path()).absolutePath().mid(prefixLength);
        // it.path() is the parent dir; rebuild relative path of the entry itself
        QString entryAbs = it.filePath();
        QString entryRel = QDir(sourceDir).relativeFilePath(entryAbs);
        QString target = QDir(destDir).absoluteFilePath(entryRel);
        Q_UNUSED(relative);
        if (it.fileInfo().isDir()) {
            if (!QDir().mkpath(target)) {
                ok = false;
            }
        } else {
            if (QFile::exists(target) && !QFile::remove(target)) {
                ok = false;
                continue;
            }
            if (!QFile::copy(entryAbs, target)) {
                ok = false;
            }
        }
    }
    return ok;
}

int backupDeckFiles(const QString &sourceDir, const QString &destDir)
{
    QDir source(sourceDir);
    if (!source.exists()) {
        return -1;
    }
    if (!QDir().mkpath(destDir)) {
        return -1;
    }
    static const QStringList deckFilters = {"*.cod", "*.dec", "*.mwdeck"};
    QStringList decks = source.entryList(deckFilters, QDir::Files);
    int copied = 0;
    for (const QString &deck : decks) {
        QString target = QDir(destDir).absoluteFilePath(deck);
        if (QFile::exists(target) && !QFile::remove(target)) {
            continue;
        }
        if (QFile::copy(source.absoluteFilePath(deck), target)) {
            ++copied;
        }
    }
    return copied;
}

bool backupAtRiskData(const QString &sourceDir, const QString &backupRoot, QString *backupLocationOut)
{
    QDir source(sourceDir);
    if (!source.exists()) {
        return false;
    }
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
    QString backupDir = QDir(backupRoot).absoluteFilePath("CockatriceDataBackup-" + timestamp);
    if (!QDir().mkpath(backupDir)) {
        return false;
    }
    if (!copyDirectoryRecursively(sourceDir, backupDir)) {
        return false;
    }
    if (backupLocationOut) {
        *backupLocationOut = backupDir;
    }
    return true;
}

QString defaultBackupRoot()
{
    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documents.isEmpty()) {
        return QDir(documents).absoluteFilePath("CockatriceBackups");
    }
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!appData.isEmpty()) {
        return QDir(appData).absoluteFilePath("../CockatriceBackups");
    }
    return QDir::temp().absoluteFilePath("CockatriceBackups");
}

} // namespace UserDataBackup
