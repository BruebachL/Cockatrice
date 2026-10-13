#include "gtest/gtest.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <libcockatrice/utility/user_data_backup.h>

namespace
{

TEST(UserDataBackup, PathInsideDirectory)
{
    QTemporaryDir installDir;
    ASSERT_TRUE(installDir.isValid());
    QDir install(installDir.path());
    ASSERT_TRUE(install.mkpath("data/decks"));
    QString dataDir = install.absoluteFilePath("data");
    QString decksDir = install.absoluteFilePath("data/decks");

    EXPECT_TRUE(UserDataBackup::isPathInsideDirectory(decksDir, installDir.path()));
    EXPECT_TRUE(UserDataBackup::isPathInsideDirectory(dataDir, installDir.path()));
    EXPECT_TRUE(UserDataBackup::isPathInsideDirectory(installDir.path(), installDir.path()));
}

TEST(UserDataBackup, PathOutsideDirectory)
{
    QTemporaryDir installDir;
    QTemporaryDir otherDir;
    ASSERT_TRUE(installDir.isValid());
    ASSERT_TRUE(otherDir.isValid());

    EXPECT_FALSE(UserDataBackup::isPathInsideDirectory(otherDir.path(), installDir.path()));
    EXPECT_FALSE(UserDataBackup::isPathInsideDirectory("", installDir.path()));
    EXPECT_FALSE(UserDataBackup::isPathInsideDirectory(otherDir.path(), ""));
}

TEST(UserDataBackup, SiblingPrefixIsNotInside)
{
    QTemporaryDir baseDir;
    ASSERT_TRUE(baseDir.isValid());
    QDir base(baseDir.path());
    ASSERT_TRUE(base.mkpath("Cockatrice"));
    ASSERT_TRUE(base.mkpath("CockatriceOther"));
    QString install = base.absoluteFilePath("Cockatrice");
    QString sibling = base.absoluteFilePath("CockatriceOther/decks");

    EXPECT_FALSE(UserDataBackup::isPathInsideDirectory(sibling, install))
        << "a sibling whose name shares a prefix must not count as inside";
}

TEST(UserDataBackup, FilterKeepsOnlyAtRiskPaths)
{
    QTemporaryDir installDir;
    QTemporaryDir safeDir;
    ASSERT_TRUE(installDir.isValid());
    ASSERT_TRUE(safeDir.isValid());
    QString atRisk = QDir(installDir.path()).absoluteFilePath("data/decks");

    QStringList filtered = UserDataBackup::filterPathsInsideDirectory({atRisk, safeDir.path()}, installDir.path());
    ASSERT_EQ(filtered.size(), 1);
    EXPECT_EQ(filtered.front(), atRisk);
}

TEST(UserDataBackup, BackupAtRiskDataCopiesDecks)
{
    QTemporaryDir installDir;
    QTemporaryDir backupRoot;
    ASSERT_TRUE(installDir.isValid());
    ASSERT_TRUE(backupRoot.isValid());
    QDir install(installDir.path());
    ASSERT_TRUE(install.mkpath("data/decks"));
    QString decksDir = install.absoluteFilePath("data/decks");

    QFile deck(decksDir + "/mydeck.cod");
    ASSERT_TRUE(deck.open(QIODevice::WriteOnly | QIODevice::Text));
    deck.write("<cockatrice_deck version=\"1\"/>\n");
    deck.close();

    QString backupLocation;
    ASSERT_TRUE(UserDataBackup::backupAtRiskData(QDir(installDir.path()).absoluteFilePath("data"), backupRoot.path(),
                                                 &backupLocation));
    EXPECT_TRUE(QFile::exists(backupLocation + "/decks/mydeck.cod"));
}

TEST(UserDataBackup, CopyMissingDirectoryFails)
{
    QTemporaryDir backupRoot;
    ASSERT_TRUE(backupRoot.isValid());
    EXPECT_FALSE(UserDataBackup::copyDirectoryRecursively("/nonexistent-cockatrice-path-xyz", backupRoot.path()));
}

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
