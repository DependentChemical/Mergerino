// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Paths.hpp"

#include "common/Modes.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <cassert>

using namespace std::literals;

namespace chatterino {

namespace {

/// Copies the contents of `source` into `destination`.
///
/// Files that already exist in the destination are never overwritten.
void copyDirectoryContents(const QString &source, const QString &destination)
{
    QDir sourceDirectory(source);
    if (!sourceDirectory.exists())
    {
        return;
    }

    if (!QDir().mkpath(destination))
    {
        return;
    }

    const auto entries = sourceDirectory.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const auto &entry : entries)
    {
        if (entry.isSymLink())
        {
            // Symlinks are not followed, they could point at their own
            // parent directory.
            continue;
        }

        const auto target = combinePath(destination, entry.fileName());

        if (entry.isDir())
        {
            copyDirectoryContents(entry.absoluteFilePath(), target);
            continue;
        }

        if (QFileInfo::exists(target))
        {
            continue;
        }

        QFile::copy(entry.absoluteFilePath(), target);
    }
}

/// Returns whether both paths point to the same directory.
bool isSameDirectory(const QString &a, const QString &b)
{
    const auto canonicalA = QFileInfo(a).canonicalFilePath();
    const auto canonicalB = QFileInfo(b).canonicalFilePath();

    const auto pathA = canonicalA.isEmpty() ? QDir::cleanPath(a) : canonicalA;
    const auto pathB = canonicalB.isEmpty() ? QDir::cleanPath(b) : canonicalB;

#ifdef Q_OS_WIN
    return pathA.compare(pathB, Qt::CaseInsensitive) == 0;
#else
    return pathA == pathB;
#endif
}

}  // namespace

Paths::Paths()
{
    this->initAppFilePathHash();

    this->initCheckPortable();
    this->initRootDirectory();
    this->runLegacyDataMigration();
    this->initSubDirectories();
}

bool Paths::createFolder(const QString &folderPath)
{
    return QDir().mkpath(folderPath);
}

bool Paths::isPortable() const
{
    return Modes::instance().isPortable;
}

QString Paths::cacheDirectory() const
{
    static const auto pathSetting = [] {
        QStringSetting cachePathSetting("/cache/path");

        cachePathSetting.connect([](const auto &newPath) {
            if (!newPath.isEmpty())
            {
                QDir().mkpath(newPath);
            }
        });

        return cachePathSetting;
    }();

    auto path = pathSetting.getValue();

    if (path.isEmpty())
    {
        return this->cacheDirectory_;
    }

    return path;
}

QString Paths::cacheFilePath(const QString &fileName) const
{
    return combinePath(this->cacheDirectory(), fileName);
}

void Paths::initAppFilePathHash()
{
    this->applicationFilePathHash =
        QCryptographicHash::hash(
            QCoreApplication::applicationFilePath().toUtf8(),
            QCryptographicHash::Sha224)
            .toBase64()
            .mid(0, 32)
            .replace("+", "-")
            .replace("/", "x");
}

void Paths::initCheckPortable()
{
    this->portable_ = QFileInfo::exists(
        combinePath(QCoreApplication::applicationDirPath(), "portable"));
}

void Paths::initRootDirectory()
{
    assert(this->portable_.has_value());

    // Mergerino is portable by default: every file the application writes
    // lives in the "settings" folder next to the executable. A different
    // data root can be configured with the `data-root=` option of the
    // `modes` file.

    this->rootAppDataDirectory = Paths::computeRootDirectory(
        QCoreApplication::applicationDirPath(), Modes::instance().dataRoot);
}

QString Paths::portableDataFolderName()
{
    return QStringLiteral("settings");
}

QString Paths::computeRootDirectory(const QString &applicationDirPath,
                                    const QString &configuredDataRoot)
{
    if (!configuredDataRoot.isEmpty())
    {
        auto path = QDir::fromNativeSeparators(configuredDataRoot);
        if (QDir::isRelativePath(path))
        {
            path = QDir(applicationDirPath).absoluteFilePath(path);
        }
        return QDir::cleanPath(path);
    }

    // portable
    return combinePath(applicationDirPath, Paths::portableDataFolderName());
}

QStringList Paths::legacyDataFolderNames()
{
    return {
        QStringLiteral("Settings"),
        QStringLiteral("Cache"),
        QStringLiteral("Logs"),
        QStringLiteral("Misc"),
        QStringLiteral("ProfileAvatars"),
        QStringLiteral("Plugins"),
        QStringLiteral("Themes"),
        QStringLiteral("Crashes"),
        QStringLiteral("Dictionaries"),
        QStringLiteral("IPC"),
    };
}

QString Paths::legacyAppDataDirectory()
{
    auto path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty())
    {
        return {};
    }

// Normalize the Windows app data folder casing for the branded app name.
#ifdef Q_OS_WIN
    path.replace(QCoreApplication::applicationName(),
                 QStringLiteral("Mergerino"), Qt::CaseInsensitive);
#endif
    return QDir::cleanPath(path);
}

bool Paths::migrateLegacyData(const QString &target,
                               const QStringList &legacyRoots,
                               const QStringList &deletedFolders)
{
    if (target.isEmpty())
    {
        return false;
    }

    bool migrated = false;
    for (const auto &legacyRoot : legacyRoots)
    {
        if (isSameDirectory(legacyRoot, target))
        {
            continue;
        }

        for (const auto &name : Paths::legacyDataFolderNames())
        {
            const auto source = combinePath(legacyRoot, name);
            if (!QFileInfo(source).isDir())
            {
                continue;
            }

            if (deletedFolders.contains(name))
            {
                // Disposable files are deleted instead of being copied, so
                // that no stale files are left behind. The application uses
                // the cache inside the portable data folder instead.
                QDir(source).removeRecursively();
                migrated = true;
                continue;
            }

            // Settings used to be stored in their own folder. They now live
            // directly inside the data folder.
            const auto destination = name == QStringLiteral("Settings")
                                         ? target
                                         : combinePath(target, name);
            if (isSameDirectory(source, destination))
            {
                continue;
            }

            copyDirectoryContents(source, destination);
            migrated = true;
        }
    }

    return migrated;
}

void Paths::runLegacyDataMigration()
{
    const auto &target = this->rootAppDataDirectory;
    if (target.isEmpty())
    {
        return;
    }

    // The migration only has to run once per data folder.
    const auto marker = combinePath(target, ".legacy-data-migrated");
    if (QFileInfo::exists(marker))
    {
        return;
    }

    // Older versions either stored their data next to the executable
    // (portable mode) or in the app data directory.
    QStringList legacyRoots{QCoreApplication::applicationDirPath()};
    const auto appDataDirectory = Paths::legacyAppDataDirectory();
    if (!appDataDirectory.isEmpty())
    {
        legacyRoots << appDataDirectory;
    }

    // The cache of the previous installation is deleted instead of being
    // copied. The application uses the cache inside the portable data folder.
    if (!Paths::migrateLegacyData(target, legacyRoots,
                                   {QStringLiteral("Cache")}))
    {
        return;
    }

    QFile markerFile(marker);
    if (markerFile.open(QIODevice::WriteOnly))
    {
        markerFile.close();
    }
}

void Paths::initSubDirectories()
{
    // required the app data directory to be set first
    assert(!this->rootAppDataDirectory.isEmpty());

    // create settings subdirectories and validate that they are created
    // properly
    auto makePath = [&](const QString &name) -> QString {
        auto path = combinePath(this->rootAppDataDirectory, name);

        if (!QDir().mkpath(path))
        {
            throw std::runtime_error("Could not create directory \""s +
                                     path.toStdString() + "\"");
        }

        return path;
    };

    makePath("");
    // Settings are stored directly inside the data folder.
    this->settingsDirectory = this->rootAppDataDirectory;
    this->cacheDirectory_ = makePath("Cache");
    this->messageLogDirectory = makePath("Logs");
    this->miscDirectory = makePath("Misc");
    this->twitchProfileAvatars =
        makePath(combinePath("ProfileAvatars", "twitch"));
    this->pluginsDirectory = makePath("Plugins");
    this->themesDirectory = makePath("Themes");
    this->crashdumpDirectory = makePath("Crashes");
    this->dictionariesDirectory = makePath("Dictionaries");
#ifdef Q_OS_WIN
    this->ipcDirectory = makePath("IPC");
#else
    // NOTE: We do *NOT* use IPC on non-Windows platforms.
    // If we start, we should re-consider this directory.
    this->ipcDirectory = "/tmp";
#endif
}

}  // namespace chatterino
