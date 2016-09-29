/*
 *   Copyright (C) 2016 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Self
#include "sortedactivitiesmodel.h"

// Qt
#include <QColor>
#include <QObject>
#include <QTimer>

// KDE
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDirWatch>
#include <KLocalizedString>
#include <KActivities/Consumer>

#define KWINDOWSYSTEM_NO_DEPRECATED

#include <KWindowSystem>
#include <QX11Info>

#define PLASMACONFIG "plasma-org.kde.plasma.desktop-appletsrc"

namespace {

    class BackgroundCache: public QObject {
    public:
        BackgroundCache()
            : initialized(false)
            , plasmaConfig(KSharedConfig::openConfig(PLASMACONFIG))
        {
            using namespace std::placeholders;

            const auto configFile = QStandardPaths::writableLocation(
                                        QStandardPaths::GenericConfigLocation) +
                                    QLatin1Char('/') + PLASMACONFIG;

            KDirWatch::self()->addFile(configFile);

            QObject::connect(KDirWatch::self(), &KDirWatch::dirty,
                             this, &BackgroundCache::settingsFileChanged,
                             Qt::QueuedConnection);
            QObject::connect(KDirWatch::self(), &KDirWatch::created,
                             this, &BackgroundCache::settingsFileChanged,
                             Qt::QueuedConnection);

        }

        void settingsFileChanged(const QString &file)
        {
            if (!file.endsWith(PLASMACONFIG)) {
                return;
            }

            if (initialized) {
                plasmaConfig->reparseConfiguration();
                reload();
            }
        }

        void subscribe(SortedActivitiesModel *model)
        {
            if (!initialized) {
                reload();
            }

            models << model;
        }

        void unsubscribe(SortedActivitiesModel *model)
        {
            models.removeAll(model);

            if (models.isEmpty()) {
                initialized = false;
                forActivity.clear();
            }
        }

        QString backgroundFromConfig(const KConfigGroup &config) const
        {
            auto wallpaperPlugin = config.readEntry("wallpaperplugin");
            auto wallpaperConfig = config.group("Wallpaper").group(wallpaperPlugin).group("General");

            if (wallpaperConfig.hasKey("Image")) {
                // Trying for the wallpaper
                auto wallpaper = wallpaperConfig.readEntry("Image", QString());
                if (!wallpaper.isEmpty()) {
                    return wallpaper;
                }
            }
            if (wallpaperConfig.hasKey("Color")) {
                auto backgroundColor = wallpaperConfig.readEntry("Color", QColor(0, 0, 0));
                return backgroundColor.name();
            }

            return QString();
        }

        void reload()
        {
            auto newForActivity = forActivity;
            QHash<QString, int> lastScreenForActivity;

            // contains activities for which the wallpaper
            // has updated
            QStringList changedActivities;

            // Contains activities not covered by any containment
            QStringList ghostActivities = forActivity.keys();

            // Traversing through all containments in search for
            // containments that define activities in plasma
            for (const auto& containmentId: plasmaConfigContainments().groupList()) {
                const auto containment = plasmaConfigContainments().group(containmentId);
                const auto lastScreen  = containment.readEntry("lastScreen", 0);
                const auto activity    = containment.readEntry("activityId", QString());

                // Ignore the containment if the activity is not defined
                if (activity.isEmpty()) continue;

                // If we have already found the same activity from another
                // containment, we are using the new one only if
                // the previous one was a color and not a proper wallpaper,
                // or if the screen ID is closer to zero
                const bool processed = !ghostActivities.contains(activity) &&
                                        newForActivity.contains(activity) &&
                                        (lastScreenForActivity[activity] <= lastScreen);

                // qDebug() << "GREPME Searching containment " << containmentId
                //          << "for the wallpaper of the " << activity << " activity - "
                //          << "currently, we think that the wallpaper is " << processed << (processed ? newForActivity[activity] : QString())
                //          << "last screen is" << lastScreen
                //          ;

                if (processed &&
                    newForActivity[activity][0] != '#') continue;

                // Marking the current activity as processed
                ghostActivities.removeAll(activity);

                const auto background = backgroundFromConfig(containment);

                // qDebug() << "        GREPME Found wallpaper: " << background;

                if (background.isEmpty()) continue;

                // If we got this far and we already had a new wallpaper for
                // this activity, it means we now have a better one
                bool foundBetterWallpaper = changedActivities.contains(activity);

                if (foundBetterWallpaper || newForActivity[activity] != background) {
                    if (!foundBetterWallpaper) {
                        changedActivities << activity;
                    }

                    // qDebug() << "        GREPME Setting: " << activity << " = " << background << "," << lastScreen;
                    newForActivity[activity] = background;
                    lastScreenForActivity[activity] = lastScreen;
                }
            }

            initialized = true;

            // Removing the activities from the list if we haven't found them
            // while traversing through the containments
            for (const auto& activity: ghostActivities) {
                newForActivity.remove(activity);
            }

            // If we have detected the changes, lets notify everyone
            if (!changedActivities.isEmpty()) {
                forActivity = newForActivity;

                for (auto model: models) {
                    model->onBackgroundsUpdated(changedActivities);
                }
            }
        }

        KConfigGroup plasmaConfigContainments() {
            return plasmaConfig->group("Containments");
        }

        QHash<QString, QString> forActivity;
        QList<SortedActivitiesModel*> models;

        bool initialized;
        KSharedConfig::Ptr plasmaConfig;
    };

    static BackgroundCache &backgrounds()
    {
        // If you convert this to a shared pointer,
        // fix the connections to KDirWatcher
        static BackgroundCache cache;
        return cache;
    }

}

SortedActivitiesModel::SortedActivitiesModel(QVector<KActivities::Info::State> states, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_activitiesModel(new KActivities::ActivitiesModel(states, this))
    , m_activities(new KActivities::Consumer(this))
{
    setSourceModel(m_activitiesModel);

    setDynamicSortFilter(true);
    setSortRole(LastTimeUsed);
    sort(0, Qt::DescendingOrder);

    backgrounds().subscribe(this);

    const QList<WId> windows = KWindowSystem::stackingOrder();

    for (const auto& window: windows) {
        KWindowInfo info(window, NET::WMVisibleName, NET::WM2Activities);
        const QStringList activities = info.activities();

        if (activities.isEmpty() || activities.contains("00000000-0000-0000-0000-000000000000")) continue;

        for (const auto& activity: activities) {
            m_activitiesWindows[activity] << window;
        }
    }

    connect(KWindowSystem::self(), &KWindowSystem::windowAdded,
            this,                  &SortedActivitiesModel::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved,
            this,                  &SortedActivitiesModel::onWindowRemoved);
    connect(KWindowSystem::self(), SIGNAL(windowChanged(WId, NET::Properties, NET::Properties2)),
            this,                  SLOT(onWindowChanged(WId, NET::Properties, NET::Properties2)));
}

SortedActivitiesModel::~SortedActivitiesModel()
{
    backgrounds().unsubscribe(this);
}

bool SortedActivitiesModel::inhibitUpdates() const
{
    return m_inhibitUpdates;
}

void SortedActivitiesModel::setInhibitUpdates(bool inhibitUpdates)
{
    if (m_inhibitUpdates != inhibitUpdates) {
        m_inhibitUpdates = inhibitUpdates;
        emit inhibitUpdatesChanged(m_inhibitUpdates);

        setDynamicSortFilter(!inhibitUpdates);
    }
}

uint SortedActivitiesModel::lastUsedTime(const QString &activity) const
{
    if (m_activities->currentActivity() == activity) {
        return ~(uint)0;

    } else {
        KConfig config("kactivitymanagerd-switcher", KConfig::SimpleConfig);
        KConfigGroup times(&config, "LastUsed");

        return times.readEntry(activity, (uint)0);
    }
}

bool SortedActivitiesModel::lessThan(const QModelIndex &sourceLeft,
                                     const QModelIndex &sourceRight) const
{
    const auto activityLeft =
          sourceModel()->data(sourceLeft, KActivities::ActivitiesModel::ActivityId);
    const auto activityRight =
          sourceModel()->data(sourceRight, KActivities::ActivitiesModel::ActivityId);

    const auto timeLeft  = lastUsedTime(activityLeft.toString());
    const auto timeRight = lastUsedTime(activityRight.toString());

    return (timeLeft < timeRight) ||
           (timeLeft == timeRight && activityLeft < activityRight);
}

QHash<int, QByteArray> SortedActivitiesModel::roleNames() const
{
    if (!sourceModel()) return QHash<int, QByteArray>();

    auto roleNames = sourceModel()->roleNames();

    roleNames[LastTimeUsed]       = "lastTimeUsed";
    roleNames[LastTimeUsedString] = "lastTimeUsedString";
    roleNames[WindowCount]        = "windowCount";
    roleNames[HasWindows]         = "hasWindows";

    return roleNames;
}

QVariant SortedActivitiesModel::data(const QModelIndex &index, int role) const
{
    if (role == KActivities::ActivitiesModel::ActivityBackground) {
        const auto activity = activityIdForIndex(index);

        return backgrounds().forActivity[activity];

    } else if (role == LastTimeUsed || role == LastTimeUsedString) {
        const auto activity = activityIdForIndex(index);

        const auto time = lastUsedTime(activity);

        if (role == LastTimeUsed) {
            return QVariant(time);

        } else {
            const auto now = QDateTime::currentDateTime().toTime_t();

            if (time == 0) return i18n("Used some time ago");

            auto diff = now - time;

            // We do not need to be precise
            diff /= 60;
            const auto minutes = diff % 60; diff /= 60;
            const auto hours   = diff % 24; diff /= 24;
            const auto days    = diff % 30; diff /= 30;
            const auto months  = diff % 12; diff /= 12;
            const auto years   = diff;

            return (years > 0)   ? i18n("Used more than a year ago")
                 : (months > 0)  ? i18ncp("amount in months",  "Used a month ago",  "Used %1 months ago", months)
                 : (days > 0)    ? i18ncp("amount in days",    "Used a day ago",    "Used %1 days ago",   days)
                 : (hours > 0)   ? i18ncp("amount in hours",   "Used an hour ago",  "Used %1 hours ago",  hours)
                 : (minutes > 0) ? i18ncp("amount in minutes", "Used a minute ago", "Used %1 minutes ago",  minutes)
                 :                 i18n("Used a moment ago");

        }

    } else if (role == HasWindows || role == WindowCount) {
        const auto activity = activityIdForIndex(index);

        if (role == HasWindows) {
            return (m_activitiesWindows[activity].size() > 0);
        } else {
            return m_activitiesWindows[activity].size();
        }


    } else {
        return QSortFilterProxyModel::data(index, role);
    }
}

QString SortedActivitiesModel::activityIdForIndex(const QModelIndex &index) const
{
    return data(index, KActivities::ActivitiesModel::ActivityId).toString();
}

QString SortedActivitiesModel::activityIdForRow(int row) const
{
    return activityIdForIndex(index(row, 0));
}

int SortedActivitiesModel::rowForActivityId(const QString &activity) const
{
    int position = -1;

    for (int row = 0; row < rowCount(); ++row) {
        if (activity == activityIdForRow(row)) {
            position = row;
        }
    }

    return position;
}

QString SortedActivitiesModel::relativeActivity(int relative) const
{
    const auto currentActivity = m_activities->currentActivity();

    if (!sourceModel()) return QString();

    const auto currentRowCount = sourceModel()->rowCount();

    //x % 0 is undefined in c++
    if (currentRowCount == 0) {
        return QString();
    }

    int currentActivityRow = 0;

    for (; currentActivityRow < currentRowCount; currentActivityRow++) {
        if (activityIdForRow(currentActivityRow) == currentActivity) break;
    }

    currentActivityRow = currentActivityRow + relative;

    //wrap to within bounds for both positive and negative currentActivityRows
    currentActivityRow = (currentRowCount + (currentActivityRow % currentRowCount)) % currentRowCount;

    return activityIdForRow(currentActivityRow);
}

void SortedActivitiesModel::onCurrentActivityChanged(const QString &currentActivity)
{
    if (m_previousActivity == currentActivity) return;

    const int previousActivityRow = rowForActivityId(m_previousActivity);
    emit rowChanged(previousActivityRow, { LastTimeUsed, LastTimeUsedString });

    m_previousActivity = currentActivity;

    const int currentActivityRow = rowForActivityId(m_previousActivity);
    emit rowChanged(currentActivityRow, { LastTimeUsed, LastTimeUsedString });
}

void SortedActivitiesModel::onBackgroundsUpdated(const QStringList &activities)
{
    for (const auto &activity: activities) {
        const int row = rowForActivityId(activity);
        emit rowChanged(row, { KActivities::ActivitiesModel::ActivityBackground });
    }
}

void SortedActivitiesModel::onWindowAdded(WId window)
{
    KWindowInfo info(window, 0, NET::WM2Activities);
    const QStringList activities = info.activities();

    if (activities.isEmpty() || activities.contains("00000000-0000-0000-0000-000000000000")) return;

    for (const auto& activity: activities) {
        if (!m_activitiesWindows[activity].contains(window)) {
            m_activitiesWindows[activity] << window;

            rowChanged(rowForActivityId(activity),
                m_activitiesWindows.size() == 1
                    ? QVector<int>{WindowCount, HasWindows}
                    : QVector<int>{WindowCount});
        }
    }
}

void SortedActivitiesModel::onWindowRemoved(WId window)
{
    for (const auto& activity: m_activitiesWindows.keys()) {
        if (m_activitiesWindows[activity].contains(window)) {
            m_activitiesWindows[activity].removeAll(window);

            rowChanged(rowForActivityId(activity),
                m_activitiesWindows.size() == 0
                    ? QVector<int>{WindowCount, HasWindows}
                    : QVector<int>{WindowCount});
        }
    }
}

void SortedActivitiesModel::onWindowChanged(WId window, NET::Properties properties, NET::Properties2 properties2)
{
    Q_UNUSED(properties);

    if (properties2 & NET::WM2Activities) {
        onWindowRemoved(window);
        onWindowAdded(window);
    }
}

void SortedActivitiesModel::rowChanged(int row, const QVector<int> &roles)
{
    if (row == -1) return;
    emit dataChanged(index(row, 0), index(row, 0), roles);
}
