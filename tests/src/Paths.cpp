// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Paths.hpp"

#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

using namespace chatterino;

namespace {

/// Paths are compared independently of the separator used by the platform.
QString normalize(const QString &path)
{
    return QDir::fromNativeSeparators(QDir::cleanPath(path));
}

QString fileIn(const QString &directory, const QString &name)
{
    return QDir(directory).filePath(name);
}

void writeFile(const QString &path, const QString &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    const auto data = content.toUtf8();

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(data), static_cast<qint64>(data.size()));
}

}  // namespace

TEST(Paths, DataIsStoredNextToTheExecutableByDefault)
{
    const auto applicationDir = QStringLiteral("C:/Mergerino");
    const auto root = Paths::computeRootDirectory(applicationDir, QString());
    const auto expected = QStringLiteral("C:/Mergerino/settings");

    EXPECT_EQ(Paths::portableDataFolderName(), QStringLiteral("settings"));
    EXPECT_EQ(normalize(root), normalize(expected));
}

TEST(Paths, ConfiguredDataRootIsUsedAsIs)
{
    const auto applicationDir = QStringLiteral("C:/Mergerino");
    const auto configured =
        QDir::current().absoluteFilePath(QStringLiteral("MergerinoData"));

    const auto root = Paths::computeRootDirectory(applicationDir, configured);

    EXPECT_EQ(normalize(root), normalize(configured));
}

TEST(Paths, RelativeDataRootIsResolvedAgainstTheExecutable)
{
    const auto applicationDir = QStringLiteral("C:/Mergerino");
    const auto root = Paths::computeRootDirectory(
        applicationDir, QStringLiteral("MergerinoData"));
    const auto expected = QDir(applicationDir).absoluteFilePath(
        QStringLiteral("MergerinoData"));

    EXPECT_EQ(normalize(root), normalize(expected));
}

TEST(Paths, LegacyDataFoldersAreKnown)
{
    const auto folders = Paths::legacyDataFolderNames();

    EXPECT_TRUE(folders.contains(QStringLiteral("Settings")));
    EXPECT_TRUE(folders.contains(QStringLiteral("Cache")));
    EXPECT_TRUE(folders.contains(QStringLiteral("Logs")));
    EXPECT_TRUE(folders.contains(QStringLiteral("Misc")));
}

TEST(Paths, LegacyDataIsCopiedIntoThePortableFolder)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());

    const auto legacyRoot = directory.filePath(QStringLiteral("legacy"));
    const auto target = directory.filePath(QStringLiteral("portable/settings"));

    writeFile(fileIn(legacyRoot, QStringLiteral("Settings/settings.json")),
              QStringLiteral("{}"));
    writeFile(fileIn(legacyRoot, QStringLiteral("Cache/emote.png")),
              QStringLiteral("emote"));
    writeFile(fileIn(legacyRoot, QStringLiteral("Logs/chat.log")),
              QStringLiteral("log"));

    EXPECT_TRUE(Paths::migrateLegacyData(target, {legacyRoot}));

    const auto settingsFile = fileIn(target, QStringLiteral("settings.json"));
    const auto cachedEmote = fileIn(target, QStringLiteral("Cache/emote.png"));
    const auto logFile = fileIn(target, QStringLiteral("Logs/chat.log"));

    // Settings are stored directly inside the data folder
    EXPECT_TRUE(QFileInfo::exists(settingsFile));
    // Other folders keep their name
    EXPECT_TRUE(QFileInfo::exists(cachedEmote));
    EXPECT_TRUE(QFileInfo::exists(logFile));
}

TEST(Paths, LegacyDataDoesNotOverwriteExistingFiles)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());

    const auto legacyRoot = directory.filePath(QStringLiteral("legacy"));
    const auto target = directory.filePath(QStringLiteral("portable/settings"));

    writeFile(fileIn(legacyRoot, QStringLiteral("Settings/settings.json")),
              QStringLiteral("legacy"));
    writeFile(fileIn(target, QStringLiteral("settings.json")),
              QStringLiteral("portable"));

    EXPECT_TRUE(Paths::migrateLegacyData(target, {legacyRoot}));

    QFile file(fileIn(target, QStringLiteral("settings.json")));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), QByteArray("portable"));
}

TEST(Paths, DeletedLegacyFoldersAreRemovedInsteadOfCopied)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());

    const auto legacyRoot = directory.filePath(QStringLiteral("legacy"));
    const auto target = directory.filePath(QStringLiteral("portable/settings"));

    writeFile(fileIn(legacyRoot, QStringLiteral("Settings/settings.json")),
              QStringLiteral("{}"));
    writeFile(fileIn(legacyRoot, QStringLiteral("Cache/emote.png")),
              QStringLiteral("emote"));

    EXPECT_TRUE(Paths::migrateLegacyData(target, {legacyRoot},
                                         {QStringLiteral("Cache")}));

    const auto settingsFile = fileIn(target, QStringLiteral("settings.json"));
    const auto legacyEmote =
        fileIn(legacyRoot, QStringLiteral("Cache/emote.png"));
    const auto cachedEmote = fileIn(target, QStringLiteral("Cache/emote.png"));

    EXPECT_TRUE(QFileInfo::exists(settingsFile));
    // The cache is deleted instead of being copied
    EXPECT_FALSE(QFileInfo::exists(legacyEmote));
    EXPECT_FALSE(QFileInfo::exists(cachedEmote));
}

TEST(Paths, NothingIsCopiedWithoutLegacyData)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());

    const auto legacyRoot = directory.filePath(QStringLiteral("legacy"));
    const auto target = directory.filePath(QStringLiteral("portable/settings"));

    EXPECT_FALSE(Paths::migrateLegacyData(target, {legacyRoot}));
    EXPECT_FALSE(QFileInfo::exists(target));
}
