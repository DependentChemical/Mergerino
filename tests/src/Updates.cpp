// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Updates.hpp"

#include "common/Version.hpp"
#include "Test.hpp"

#include <semver/semver.hpp>

using namespace chatterino;

TEST(Updates, MustBeDowngrade)
{
    EXPECT_TRUE(Updates::isDowngradeOf("1.0.0", "2.4.5"))
        << "1.0.0 must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.0.0", "2.4.5"))
        << "2.0.0 must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.0", "2.4.5"))
        << "2.4.0 must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.4-beta", "2.4.5"))
        << "2.4.4-beta must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta", "2.4.5"))
        << "2.4.5-beta must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta.1", "2.4.5-beta.2"))
        << "2.4.5-beta.1 must be a downgrade of 2.4.5-beta.2";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta", "2.4.5-beta.2"))
        << "2.4.5-beta must be a downgrade of 2.4.5-beta.2";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta.2", "2.4.6-beta.1"))
        << "2.4.5-beta.2 must be a downgrade of 2.4.6-beta.1";
    EXPECT_TRUE(Updates::isDowngradeOf("2.5.4", "2.5.5-beta.1"))
        << "2.5.4 must be a downgrade of 2.5.5-beta.1";
}

TEST(Updates, MustNotBeDowngrade)
{
    EXPECT_FALSE(Updates::isDowngradeOf("2.4.5", "2.4.5"))
        << "2.4.5 must not be a downgrade of 2.4.5";
    EXPECT_FALSE(Updates::isDowngradeOf("2.4.5", "2.4.5-beta"))
        << "2.4.5 must not be a downgrade of 2.4.5-beta";
}

TEST(Updates, ValidateCurrentVersion)
{
    EXPECT_NO_THROW([[maybe_unused]] auto v = semver::from_string(
                        Version::instance().version().toStdString()))
        << "Current version must be valid semver";
    EXPECT_EQ(Version::instance().version(), CHATTERINO_VERSION);
}

TEST(Updates, DetectUpdateByVersionOrCommit)
{
    const QString commitA(40, u'a');
    const QString commitB(40, u'b');

    EXPECT_TRUE(
        Updates::isUpdateAvailable("1.3.4", "1.3.3", commitA, commitA));
    EXPECT_TRUE(
        Updates::isUpdateAvailable("1.3.3", "1.3.3", commitB, commitA));

    EXPECT_FALSE(
        Updates::isUpdateAvailable("1.3.3", "1.3.3", commitA, commitA));
    EXPECT_FALSE(Updates::isUpdateAvailable("1.3.3", "1.3.3", {}, commitA));
    EXPECT_FALSE(
        Updates::isUpdateAvailable("1.3.2", "1.3.3", commitB, commitA));
}

TEST(Updates, DetectReleaseOnlyWhenNewerThanBuild)
{
    const auto releaseTime =
        QDateTime::fromString(QStringLiteral("2026-11-11T07:14:27Z"),
                               Qt::ISODate);
    const auto olderBuildTime =
        QDateTime::fromString(QStringLiteral("2026-11-11T06:00:00Z"),
                               Qt::ISODate);
    const auto newerBuildTime =
        QDateTime::fromString(QStringLiteral("2026-11-11T08:00:00Z"),
                               Qt::ISODate);

    EXPECT_TRUE(Updates::isReleaseNewerThanBuild(releaseTime, olderBuildTime))
        << "A release published after the build must be newer";
    EXPECT_FALSE(Updates::isReleaseNewerThanBuild(releaseTime, newerBuildTime))
        << "A release published before the build must not be newer";
    EXPECT_TRUE(Updates::isReleaseNewerThanBuild({}, releaseTime))
        << "An unknown build time must be treated as newer";
    EXPECT_TRUE(Updates::isReleaseNewerThanBuild(releaseTime, {}))
        << "An unknown release time must be treated as newer";
}
