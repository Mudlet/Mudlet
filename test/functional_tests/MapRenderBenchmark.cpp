/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * Report-only performance baseline for the 2D mapper's paint path, the
 * companion to PipelineBenchmark for the text pipeline.
 *
 * Absolute numbers are meaningless across machines, so nothing is asserted on
 * timing: the gate is a before/after comparison of two builds of this binary on
 * the SAME machine with test/compare-perf-baseline.py. One
 * `METRIC <name> <value>` line is printed per measurement.
 *
 * The map is supplied rather than generated, because the interesting workload
 * is a real pathological one and it is far too big to commit:
 *
 *   MUDLET_BENCH_MAP=/path/to/map.dat QT_QPA_PLATFORM=offscreen ./MapRenderBenchmark
 *
 * Without that variable the whole benchmark skips. The reference map is
 * Ssaliss' 2.2-million-room Aetherspace map from Lusternia, the one the mapper
 * has been stress-tested against since 2015:
 * https://bugs.launchpad.net/mudlet/+bug/1500927 (attachment 4578177, 7z).
 *
 * MUDLET_BENCH_AREA=<id> pins the area to draw; by default the benchmark picks
 * the area with the most rooms, and within it the Z level with the most rooms,
 * which is deterministic for a given map file.
 *
 * Every scenario draws the same area and Z level at a different zoom, since
 * zoom - not room count - is what decides how many rooms land on screen:
 *   close  the mapper's own default zoom, a handful of rooms
 *   near   a twentieth of the level's span across the widget
 *   mid    a quarter of the level's span across the widget
 *   fit    the whole Z level in the viewport, the worst case
 * and each frame pans by one map unit, so no frame is a repeat of the last.
 *
 * MUDLET_BENCH_FRAME_HASH=1 swaps the timed passes for a correctness pass: it
 * renders a fixed number of frames per scenario and prints one
 * `FRAMEHASH <scenario> <frame> <digest>` line each. Two builds that print the
 * same digests drew the same pixels, which is how a change that claims to only
 * cull work off-screen proves it did not also cull work on-screen. The frame
 * count is fixed rather than derived from a warm-up frame's duration, since a
 * timing-dependent count would not line up between two builds.
 */

#include <QCryptographicHash>
#include <QFileInfo>
#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <limits>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TAreaGridIndex.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgMapper.h"
#include "mudlet.h"

// Whether this binary is AddressSanitizer-instrumented. Emitted as an invariant
// so the compare script refuses an ASan-vs-release comparison (their absolute
// numbers are incomparable).
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define BENCH_BUILD_ASAN 1
#endif
#endif
#if !defined(BENCH_BUILD_ASAN) && defined(__SANITIZE_ADDRESS__)
#define BENCH_BUILD_ASAN 1
#endif
#ifndef BENCH_BUILD_ASAN
#define BENCH_BUILD_ASAN 0
#endif

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

class MapRenderBenchmark : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Map-Render-Benchmark-Host");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;
    QString mMapPath;

    // A typical mapper size when docked to the side of a main window.
    static constexpr int kWidgetWidth = 1000;
    static constexpr int kWidgetHeight = 700;
    // Report the FASTEST pass rather than the mean: the least-disturbed pass
    // isolates intrinsic speed from transient CPU contention, which is what a
    // before/after gate wants.
    static constexpr int kPasses = 5;
    // Frames per pass are chosen per scenario from a warm-up frame, because a
    // zoomed-out frame costs many times what a zoomed-in one does and a count
    // that suits one wastes minutes on the other. Every timing is reported per
    // frame, so the count does not have to match between two runs.
    static constexpr int kMaxFramesPerPass = 8;
    static constexpr double kTargetPassMs = 600.0;
    // Frames hashed per scenario when MUDLET_BENCH_FRAME_HASH is set.
    static constexpr int kHashFrames = 4;

    struct Scenario
    {
        const char* name;
        // Fraction of the Z level's larger span that the viewport covers. 1.0
        // fits the whole level; 0 means "leave the area's own default zoom".
        double spanFraction;
    };

    static constexpr Scenario kScenarios[] = {
            {"close", 0.0},
            {"near", 0.05},
            {"mid", 0.25},
            {"fit", 1.0},
    };

    static void emitMetric(const char* name, double value) { std::printf("METRIC %s %.3f\n", name, value); }

    static void emitMetric(const char* name, qint64 value) { std::printf("METRIC %s %lld\n", name, value); }

    static void emitMetric(const QString& name, double value) { emitMetric(name.toUtf8().constData(), value); }

    static void emitMetric(const QString& name, qint64 value) { emitMetric(name.toUtf8().constData(), value); }

    // A frame that is one flat colour means nothing was drawn, and timings from
    // it would describe an empty widget rather than the map.
    static bool frameHasContent(const QImage& frame)
    {
        if (frame.isNull()) {
            return false;
        }
        const QRgb first = frame.pixel(0, 0);
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                if (frame.pixel(x, y) != first) {
                    return true;
                }
            }
        }
        return false;
    }

    // A digest of the frame's pixels, in a fixed format so that two builds are
    // not told apart by the platform's preferred pixmap depth.
    static QByteArray frameDigest(const QImage& frame)
    {
        const QImage canonical = frame.convertToFormat(QImage::Format_ARGB32);
        QCryptographicHash hash(QCryptographicHash::Sha1);
        for (int y = 0; y < canonical.height(); ++y) {
            hash.addData(QByteArrayView(reinterpret_cast<const char*>(canonical.constScanLine(y)), canonical.width() * 4));
        }
        return hash.result().toHex();
    }

    static qint64 readPeakRssKb()
    {
#if defined(Q_OS_LINUX)
        QFile status(qsl("/proc/self/status"));
        if (!status.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return 0;
        }
        const QString text = QString::fromLatin1(status.readAll());
        for (const QString& line : text.split(QChar::LineFeed)) {
            if (line.startsWith(qsl("VmHWM:"))) {
                return QStringView{line}.mid(6).trimmed().split(QChar::Space).constFirst().toLongLong();
            }
        }
#endif
        return 0;
    }

private slots:
    void initTestCase()
    {
        mMapPath = qEnvironmentVariable("MUDLET_BENCH_MAP");
        if (mMapPath.isEmpty()) {
            QSKIP("MUDLET_BENCH_MAP is not set - point it at a saved Mudlet map file to run this benchmark");
        }
        if (!QFileInfo::exists(mMapPath)) {
            QFAIL(qPrintable(qsl("MUDLET_BENCH_MAP points at \"%1\", which does not exist").arg(mMapPath)));
        }
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        // QApplication's construction adopts the environment locale, which on
        // some machines makes printf("%f") emit comma decimals the compare
        // script cannot parse.
        std::setlocale(LC_NUMERIC, "C");
        initializeQRCResources();
        emitMetric("build_asan", static_cast<qint64>(BENCH_BUILD_ASAN));
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Ephemeral port (0) so parallel worktree runs never collide.
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
    }

    // Everything runs in one slot: loading the map is minutes of work on the
    // maps worth benchmarking, and QtTest would tear the profile down and make
    // every later slot pay it again.
    void benchMapRender()
    {
        const bool hashFrames = qEnvironmentVariableIntValue("MUDLET_BENCH_FRAME_HASH") != 0;
        // Somewhere to write the hashed frames to, for when two digests differ
        // and the question becomes which pixels did.
        const QString frameDir = hashFrames ? qEnvironmentVariable("MUDLET_BENCH_FRAME_DIR") : QString();
        mudlet::self()->mSkipDefaultPackageInstall = true;
        Host* host = TestProfile::create(mHostname, mLocalhost, QString::number(mPort));
        QVERIFY(host);
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the stub");

        // The mapper has to exist before the map is restored into it, exactly
        // as TMainConsole::loadMap() arranges it.
        host->showHideOrCreateMapper(false);
        QVERIFY(host->mpMap);
        QVERIFY(host->mpMap->mpMapper);
        TMap* pMap = host->mpMap.data();

        QElapsedTimer loadTimer;
        loadTimer.start();
        QVERIFY2(pMap->restore(mMapPath), qPrintable(qsl("could not restore the map at \"%1\"").arg(mMapPath)));
        const double restoreSeconds = loadTimer.nsecsElapsed() / 1.0e9;
        loadTimer.restart();
        pMap->audit();
        const double auditSeconds = loadTimer.nsecsElapsed() / 1.0e9;

        T2DMap* p2dMap = host->mpMap->mpMapper->mp2dMap;
        QVERIFY(p2dMap);
        p2dMap->init();

        emitMetric("map_format_version", static_cast<qint64>(pMap->mVersion));
        emitMetric("map_restore_seconds", restoreSeconds);
        emitMetric("map_audit_seconds", auditSeconds);
        emitMetric("map_rooms", static_cast<qint64>(pMap->mpRoomDB->size()));
        emitMetric("map_areas", static_cast<qint64>(pMap->mpRoomDB->getAreaMap().size()));

        const int areaId = chooseArea(pMap);
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        QVERIFY2(pArea, "the chosen area is not in the map");
        const int zLevel = chooseZLevel(pArea);
        const int roomsOnLevel = pArea->getRoomsForZ(zLevel).size();
        QVERIFY2(roomsOnLevel > 0, "the chosen Z level holds no rooms");

        // Workload invariants: two builds that disagree on any of these did not
        // draw the same thing, so their timings are not comparable.
        emitMetric("bench_area_id", static_cast<qint64>(areaId));
        emitMetric("bench_area_rooms", static_cast<qint64>(pArea->getAreaRooms().size()));
        emitMetric("bench_z_level", static_cast<qint64>(zLevel));
        emitMetric("bench_rooms_on_z_level", static_cast<qint64>(roomsOnLevel));
        emitMetric("bench_grid_mode", static_cast<qint64>(pArea->gridMode ? 1 : 0));
        if (hashFrames) {
            emitMetric("bench_z_colocated_rooms", coLocatedRoomCount(pMap, pArea, zLevel));
        }
        emitMetric("bench_widget_width", static_cast<qint64>(kWidgetWidth));
        emitMetric("bench_widget_height", static_cast<qint64>(kWidgetHeight));
        // So that compare-perf-baseline.py can say that a run has no timings
        // rather than that the gate names a metric nobody emits.
        emitMetric("bench_frame_hash_mode", static_cast<qint64>(hashFrames ? 1 : 0));

        // The mapper is inside a dock widget whose layout would otherwise clip
        // it; sizing the widget itself is what decides the drawn rect, since
        // T2DMap::paintEvent() works from width()/height().
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        // Park the view on the chosen area. Three things have to agree or
        // paintEvent() re-centres on the player and silently draws a different,
        // usually tiny, area instead - which reads as a suspiciously fast frame
        // rather than as a failure:
        //  - the player has to BE in this area, since paintEvent() resets
        //    mShiftMode whenever mRoomID and the player's room disagree,
        //  - mRoomID has to name that same room, and
        //  - mNewMove has to be clear, as it overrides mShiftMode outright.
        const int benchRoomId = centreRoomOfLevel(pMap, pArea, zLevel);
        QVERIFY2(benchRoomId > 0, "the chosen Z level yielded no room to stand the player in");
        pMap->mRoomIdHash[pMap->mProfileName] = benchRoomId;
        pMap->mNewMove = false;
        p2dMap->mRoomID = benchRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterZ = zLevel;

        const auto [spanX, spanY] = zLevelSpan(pArea, zLevel);
        // Centre on the room the player is standing in, in the same terms
        // paintEvent() does it - mMapCenterY is a *negated* room y. Deriving
        // the centre from the area's own span bounds instead is a trap: those
        // are held negated, so the obvious arithmetic parks the viewport on
        // the empty coordinates opposite the rooms and every frame draws
        // nothing at full speed.
        TRoom* pBenchRoom = pMap->mpRoomDB->getRoom(benchRoomId);
        QVERIFY2(pBenchRoom, "the room chosen to stand the player in is not in the room DB");
        p2dMap->mMapCenterX = pBenchRoom->x();
        p2dMap->mMapCenterY = pBenchRoom->y() * -1;

        emitMetric("bench_z_span_x", static_cast<qint64>(spanX));
        emitMetric("bench_z_span_y", static_cast<qint64>(spanY));

        QPixmap target(kWidgetWidth, kWidgetHeight);
        const qreal defaultZoom = pArea->get2DMapZoom();

        for (const Scenario& scenario : kScenarios) {
            if (scenario.spanFraction > 0.0) {
                // xyzoom is the number of map units across the SHORTER widget
                // dimension, so scale by the shorter side to make the requested
                // fraction of the level fit in both directions.
                const double units = std::max(1.0, std::max(spanX, spanY) * scenario.spanFraction);
                const double shorterSideRatio = (kWidgetWidth > kWidgetHeight) ? (static_cast<double>(kWidgetHeight) / kWidgetWidth) : 1.0;
                pArea->set2DMapZoom(std::max(1.0, units * shorterSideRatio));
            } else {
                pArea->set2DMapZoom(defaultZoom);
            }

            const double centerX = p2dMap->mMapCenterX;
            const double centerY = p2dMap->mMapCenterY;

            // First frame outside the timed passes: it pays for the symbol and
            // label pixmap caches this zoom invalidated, and proves the map
            // really reaches the pixmap.
            target.fill(Qt::magenta);
            QElapsedTimer warmUpTimer;
            warmUpTimer.start();
            p2dMap->render(&target);
            const double warmUpMs = warmUpTimer.nsecsElapsed() / 1.0e6;
            QVERIFY2(frameHasContent(target.toImage()),
                     qPrintable(qsl("scenario \"%1\" rendered a single flat colour - nothing was drawn, so its timings would describe an empty widget").arg(QString::fromUtf8(scenario.name))));
            // The area really on screen, checked after a frame has run rather
            // than before: paintEvent() is what settles it, and a scenario that
            // drew somewhere else is not the workload these numbers claim.
            QCOMPARE(p2dMap->getAreaId(), areaId);

            const QString prefix = qsl("render_%1").arg(QString::fromUtf8(scenario.name));
            if (hashFrames) {
                for (int frame = 0; frame < kHashFrames; ++frame) {
                    // A quarter of a room per frame, and on both axes, so the
                    // four frames slide the room lattice through a whole cell:
                    // a cull that is a room out at an edge shows up in one of
                    // them even though this map repeats every room.
                    p2dMap->mMapCenterX = centerX + frame / static_cast<double>(kHashFrames);
                    p2dMap->mMapCenterY = centerY + frame / static_cast<double>(kHashFrames);
                    // Rendering into a pixmap the last frame already wrote to
                    // would let anything the paint path leaves untouched carry
                    // over, so the digest would depend on the frame before it.
                    target.fill(Qt::magenta);
                    // The widget only, without its children: one of them holds a
                    // blinking text cursor, which would make the digest a
                    // function of the wall clock rather than of the paint path.
                    p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
                    const QImage frameImage = target.toImage();
                    std::printf("FRAMEHASH %s %d %s\n", scenario.name, frame, frameDigest(frameImage).constData());
                    if (!frameDir.isEmpty()) {
                        frameImage.save(qsl("%1/%2-%3.png").arg(frameDir, QString::fromUtf8(scenario.name), QString::number(frame)));
                    }
                }
            } else {
                const int framesPerPass = qBound(1, static_cast<int>(kTargetPassMs / std::max(0.001, warmUpMs)), kMaxFramesPerPass);
                double best = std::numeric_limits<double>::max();
                for (int pass = 0; pass < kPasses; ++pass) {
                    QElapsedTimer timer;
                    timer.start();
                    for (int frame = 0; frame < framesPerPass; ++frame) {
                        // Pan by a map unit per frame so no frame is a repeat of
                        // the one before it, the way a walking player's is not.
                        p2dMap->mMapCenterX = centerX + frame;
                        p2dMap->render(&target);
                    }
                    best = std::min(best, timer.nsecsElapsed() / 1.0e9);
                }
                emitMetric(qsl("%1_ms").arg(prefix), (best / framesPerPass) * 1000.0);
                emitMetric(qsl("%1_fps").arg(prefix), framesPerPass / best);
                emitMetric(qsl("%1_frames_per_pass").arg(prefix), static_cast<qint64>(framesPerPass));
            }
            p2dMap->mMapCenterX = centerX;
            p2dMap->mMapCenterY = centerY;

            emitMetric(qsl("%1_zoom").arg(prefix), pArea->get2DMapZoom());
            // xspan: how many map units fit across the widget, so also the
            // number of rooms across a fully populated level.
            emitMetric(qsl("%1_units_across").arg(prefix), p2dMap->mRoomWidth > 0.0f ? kWidgetWidth / static_cast<double>(p2dMap->mRoomWidth) : 0.0);
            // How many rooms the frame could possibly have drawn. The whole
            // point of the exercise is the gap between this and the room count
            // the paint path actually walks, so it is worth a metric of its own.
            // It moves with where the map happened to be panned when the last
            // frame of a pass finished, so it is reported rather than gated -
            // see the note on it in compare-perf-baseline.py.
            const int roomsVisible = visibleRoomCount(p2dMap, pArea, zLevel);
            emitMetric(qsl("%1_rooms_visible").arg(prefix), static_cast<qint64>(roomsVisible));
            QVERIFY2(roomsVisible > 0,
                     qPrintable(qsl("scenario \"%1\" has no room of the area anywhere in its viewport, so it timed the cost of drawing empty space").arg(QString::fromUtf8(scenario.name))));
        }

        pArea->set2DMapZoom(defaultZoom);
        if (const qint64 peakRssKb = readPeakRssKb(); peakRssKb > 0) {
            emitMetric("peak_rss_kb", peakRssKb);
        }
    }

private:
    // The area with the most rooms, so the benchmark lands on the part of the
    // map that is actually hard to draw. Ties break on the lower area id, so
    // the choice is the same on every run and every build.
    int chooseArea(TMap* pMap) const
    {
        if (const int pinned = qEnvironmentVariableIntValue("MUDLET_BENCH_AREA"); pinned != 0) {
            return pinned;
        }
        int bestId = 0;
        int bestCount = -1;
        const QMap<int, TArea*>& areas = pMap->mpRoomDB->getAreaMap();
        for (auto it = areas.constBegin(); it != areas.constEnd(); ++it) {
            if (!it.value()) {
                continue;
            }
            const int count = it.value()->getAreaRooms().size();
            if (count > bestCount) {
                bestCount = count;
                bestId = it.key();
            }
        }
        return bestId;
    }

    // Likewise the busiest Z level of that area.
    static int chooseZLevel(TArea* pArea)
    {
        int bestZ = 0;
        int bestCount = -1;
        for (int z = pArea->min_z; z <= pArea->max_z; ++z) {
            const int count = pArea->getRoomsForZ(z).size();
            if (count > bestCount) {
                bestCount = count;
                bestZ = z;
            }
        }
        return bestZ;
    }

    // How many rooms on the level share their (x, y) with another room. They are
    // the one thing that can legitimately move pixels when the paint path's
    // iteration order changes: paintEvent() gives whichever of a co-located
    // group it reaches first the un-collided styling and the rest the collided
    // one, and which that is has never been specified.
    static qint64 coLocatedRoomCount(TMap* pMap, TArea* pArea, int zLevel)
    {
        const QSet<int>& roomsOnLevel = pArea->getRoomsForZ(zLevel);
        QHash<qint64, int> roomsPerCell;
        roomsPerCell.reserve(roomsOnLevel.size());
        for (const int roomId : roomsOnLevel) {
            TRoom* pRoom = pMap->mpRoomDB->getRoom(roomId);
            if (!pRoom) {
                continue;
            }
            ++roomsPerCell[(static_cast<qint64>(pRoom->x()) << 32) | static_cast<quint32>(pRoom->y())];
        }
        qint64 coLocated = 0;
        for (const int roomsHere : roomsPerCell) {
            if (roomsHere > 1) {
                coLocated += roomsHere;
            }
        }
        return coLocated;
    }

    // The room nearest the middle of the level, so the player stands in the
    // thick of it rather than out at a corner.
    static int centreRoomOfLevel(TMap* pMap, TArea* pArea, int zLevel)
    {
        const double midX = (pArea->xminForZ.value(zLevel, pArea->min_x) + pArea->xmaxForZ.value(zLevel, pArea->max_x)) / 2.0;
        const double midY = (pArea->yminForZ.value(zLevel, pArea->min_y) + pArea->ymaxForZ.value(zLevel, pArea->max_y)) / 2.0;
        int bestId = 0;
        double bestDistance = std::numeric_limits<double>::max();
        for (const int roomId : pArea->getRoomsForZ(zLevel)) {
            TRoom* pRoom = pMap->mpRoomDB->getRoom(roomId);
            if (!pRoom) {
                continue;
            }
            const double dx = pRoom->x() - midX;
            const double dy = pRoom->y() - midY;
            const double distance = (dx * dx) + (dy * dy);
            // Ties break on the lower room id so the choice is the same on
            // every run, which QSet iteration order alone would not give.
            if (distance < bestDistance || (qFuzzyCompare(1.0 + distance, 1.0 + bestDistance) && roomId < bestId)) {
                bestDistance = distance;
                bestId = roomId;
            }
        }
        return bestId;
    }

    // Rooms whose grid cell lies in the viewport, asked of the paint path's own
    // bounds rather than a copy of them: a copy would go on reporting the old
    // number after a change to the real ones, which is the number this is here
    // to catch moving.
    static int visibleRoomCount(T2DMap* p2dMap, TArea* pArea, int zLevel)
    {
        if (p2dMap->mRoomWidth <= 0.0f || p2dMap->mRoomHeight <= 0.0f) {
            return 0;
        }
        const QRect bounds = T2DMap::viewportRoomBounds(p2dMap->mRX, p2dMap->mRY, p2dMap->mRoomWidth, p2dMap->mRoomHeight, kWidgetWidth, kWidgetHeight);
        return pArea->getGridIndex().roomsInViewport(zLevel, bounds.left(), bounds.right(), bounds.top(), bounds.bottom()).size();
    }

    static std::pair<double, double> zLevelSpan(TArea* pArea, int zLevel)
    {
        const double spanX = pArea->xmaxForZ.value(zLevel, pArea->max_x) - pArea->xminForZ.value(zLevel, pArea->min_x);
        const double spanY = pArea->ymaxForZ.value(zLevel, pArea->max_y) - pArea->yminForZ.value(zLevel, pArea->min_y);
        return {std::max(1.0, spanX), std::max(1.0, spanY)};
    }

    void deleteProfileDirectory()
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

static void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "MapRenderBenchmark.moc"
QTEST_MAIN(MapRenderBenchmark)
