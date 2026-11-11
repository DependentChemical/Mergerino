// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QStringList>

#include <optional>

namespace chatterino {

class Paths
{
public:
    Paths();

    // Root directory for the configuration files. Mergerino is portable by
    // default, so this is the "settings" folder next to the executable.
    QString rootAppDataDirectory;

    // Directory for settings files. Same as <appDataDirectory>
    QString settingsDirectory;

    // Directory for message log files. Same as <appDataDirectory>/Logs
    QString messageLogDirectory;

    // Directory for miscellaneous files. Same as <appDataDirectory>/Misc
    QString miscDirectory;

    // Directory for crashdumps. Same as <appDataDirectory>/Crashes
    QString crashdumpDirectory;

    // Hash of QCoreApplication::applicationFilePath()
    QString applicationFilePathHash;

    // Profile avatars for Twitch <appDataDirectory>/ProfileAvatars/twitch
    QString twitchProfileAvatars;

    // Plugin files live here. <appDataDirectory>/Plugins
    QString pluginsDirectory;

    // Custom themes live here. <appDataDirectory>/Themes
    QString themesDirectory;

    // Spell checking dictionaries <appDataDirectory>/Dictionaries
    QString dictionariesDirectory;

    // Directory for shared memory files.
    // <appDataDirectory>/IPC   on Windows
    // /tmp                     elsewhere
    QString ipcDirectory;

    bool createFolder(const QString &folderPath);
    [[deprecated("use Modes::instance().portable instead")]] bool isPortable()
        const;

    /// The name of the folder that holds all data of a portable installation.
    ///
    /// This folder is created next to the executable.
    static QString portableDataFolderName();

    /// Computes the directory that holds all application data.
    ///
    /// By default all data is stored in the "settings" folder next to the
    /// executable, which makes Mergerino fully portable. A different data
    /// root can be configured with the `data-root=` option of the `modes`
    /// file. Relative data roots are resolved against `applicationDirPath`.
    static QString computeRootDirectory(const QString &applicationDirPath,
                                        const QString &configuredDataRoot);

    /// The names of the folders used by older versions to store their data.
    ///
    /// These are used to migrate existing data into the portable layout.
    static QStringList legacyDataFolderNames();

    /// The data directory used by older, non-portable versions.
    ///
    /// On Windows this is `%APPDATA%\Mergerino`.
    static QString legacyAppDataDirectory();

    /// Migrates the data of previous installations into the portable data
    /// folder.
    ///
    /// `legacyRoots` are the directories that older versions used to store
    /// their data in. Files that already exist in `target` are never
    /// overwritten. Folders listed in `deletedFolders` are deleted instead of
    /// being copied, because they only contain disposable files. Returns
    /// whether anything was migrated.
    static bool migrateLegacyData(const QString &target,
                                   const QStringList &legacyRoots,
                                   const QStringList &deletedFolders = {});

    QString cacheDirectory() const;

    /// Returns the full file path for a file in the cache directory
    ///
    /// e.g. cacheFilePath("foo") will return <cacheDirectory>/foo
    QString cacheFilePath(const QString &fileName) const;

private:
    void initAppFilePathHash();
    void initCheckPortable();
    void initRootDirectory();
    void initSubDirectories();

    /// Migrates data of previous installations into the portable data folder.
    void runLegacyDataMigration();

    std::optional<bool> portable_;

    // Directory for cache files. Same as <appDataDirectory>/Misc
    QString cacheDirectory_;
};

}  // namespace chatterino
