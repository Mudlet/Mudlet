/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QAudioDevice>
#include <QDataStream>
#include <QElapsedTimer>
#include <QMediaDevices>
#include <QDeadlineTimer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MediaBackendTestHelper.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAudioMixer.h"
#include "TMainConsole.h"
#include "MudletMedia.h"
#include "TMedia.h"
#include "TMediaData.h"
#include "TMediaVoice.h"
#include "TelnetServerStub.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// QSKIP and SKIP_OR_FAIL_WITHOUT return from the test case they are in, so this has to be a macro.
// A machine with no audio output cannot have one opened, which is no fault of the backend.
#define REQUIRE_MIXING()                                                                                                                                                                               \
    do {                                                                                                                                                                                               \
        if (QMediaDevices::audioOutputs().isEmpty()) {                                                                                                                                                 \
            QSKIP("This machine has no audio output for the shared output to open.");                                                                                                                  \
        }                                                                                                                                                                                              \
        SKIP_OR_FAIL_WITHOUT(backendFeedsTheMix() ? QString() : noMixingReason());                                                                                                                     \
    } while (false)

/*
 * Sounds and music play through one shared output, TAudioMixer. An output opened and closed for each
 * sound makes macOS claim Bluetooth headphones for every one, which with AirPods breaks up other apps'
 * audio and can leave Mudlet silent. These pin that a burst opens the output once, that it closes once
 * nothing plays, that a stop silences a sound at once, and that mute reaches a mixed sound.
 */
class TAudioMixerTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "Test-Audio-Mixer";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mHostname);
        if (!TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()))) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY(mpHost && mpHost->mpMedia);
        TAudioMixer::self()->setIdleCloseDelay(10s);
    }

    void cleanup()
    {
        if (mpHost && mpHost->mpMedia) {
            mpHost->mpMedia->stopAllMediaPlayers();
        }
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        delete mudlet::self();
        TestProfile::removeProfileDirectory(mHostname);
    }

    // First, so nothing has checked the backend yet. GMCP alone does not start the check, as Mudlet offers
    // Client.Media to every game; a game that sends some has it checked before its first sound.
    void test_aGameThatSendsMediaHasTheBackendCheckedBeforeItsFirstSound()
    {
        if (!qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty()) {
            QSKIP("A backend is named, so there is nothing to check before the first sound.");
        }
        QVERIFY2(!TMediaVoice::probeStarted(), "Something checked the backend before the game said anything.");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpServer->clientConnected();
                         },
                         QDeadlineTimer(8s)),
                 "The profile never connected to the game.");
        mpServer->sendRaw(QByteArray("\xff\xfb\xc9", 3)); // IAC WILL GMCP
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpServer->receivedSoFar().contains("Client.Media");
                         },
                         QDeadlineTimer(8s)),
                 "Mudlet never offered Client.Media.");
        const bool vitalsArrived = gameSays(qsl("Char.Vitals {\"hp\": \"100\"}"));
        QVERIFY2(vitalsArrived, "The game's vitals never arrived.");
        QVERIFY(!TMediaVoice::probeStarted());

        const bool mediaArrived = gameSays(qsl("Client.Media.Default {\"url\": \"https://example.com/media/\"}"));
        QVERIFY2(mediaArrived, "The game's Client.Media.Default never arrived.");
        QVERIFY(TMediaVoice::probeStarted());
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     []() {
                                         return TMediaVoice::probePassed();
                                     },
                                     QDeadlineTimer(6s))
                                     ? QString()
                                     : noMixingReason());

        TMediaData first = clipData(writeClip(qsl("first.wav"), 3000), TMediaData::MediaTypeSound);
        media()->playMedia(first);
        QCOMPARE(media()->playersMixed(), 1);
    }

    // Video keeps an output of its own, timed against its picture; the rest are mixed
    void test_soundsAndMusicAreMixed()
    {
        REQUIRE_MIXING();
        TMediaData sound = clipData(writeClip(qsl("sound.wav"), 3000), TMediaData::MediaTypeSound);
        media()->playMedia(sound);
        TMediaData music = clipData(writeClip(qsl("music.wav"), 3000), TMediaData::MediaTypeMusic);
        media()->playMedia(music);
        QVERIFY2(waitForPlaying(2), "The sound and the music never both started.");
        QCOMPARE(media()->playersMixed(), 2);
    }

    // The whole point of sharing: however many sounds come and go, the headphones are claimed once
    void test_aBurstOfSoundsOpensTheOutputOnce()
    {
        REQUIRE_MIXING();
        const QString fileName = writeClip(qsl("hit.wav"), 150);
        const int openedBefore = TAudioMixer::self()->timesOpened();

        for (int hit = 0; hit < 20; ++hit) {
            TMediaData data = clipData(fileName, TMediaData::MediaTypeSound);
            data.setMediaKey(qsl("hit%1").arg(hit % 6));
            media()->playMedia(data);
            QTest::qWait(100ms);
        }
        QTest::qWait(500ms);

        SKIP_OR_FAIL_WITHOUT(TAudioMixer::self()->timesOpened() > openedBefore || TAudioMixer::self()->outputOpen() ? QString() : noAudioReason());
        QVERIFY2(TAudioMixer::self()->timesOpened() - openedBefore <= 1,
                 qPrintable(qsl("A burst of 20 sounds opened the shared output %1 times.").arg(TAudioMixer::self()->timesOpened() - openedBefore)));
        QVERIFY(TAudioMixer::self()->outputOpen());
    }

    // Held open while nothing plays, Mudlet would keep the headphones and count as playing audio
    void test_theOutputClosesOnceNothingPlays()
    {
        REQUIRE_MIXING();
        TAudioMixer::self()->setIdleCloseDelay(400ms);
        TMediaData data = clipData(writeClip(qsl("short.wav"), 300), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     []() {
                                         return TAudioMixer::self()->outputOpen();
                                     },
                                     QDeadlineTimer(5s))
                                     ? QString()
                                     : noAudioReason());
        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return !TAudioMixer::self()->outputOpen();
                         },
                         QDeadlineTimer(5s)),
                 "The shared output stayed open with nothing playing.");
    }

    // The mix holds a head start of each sound; a stop must not let that play on
    void test_aStopDropsWhatASoundHasQueued()
    {
        REQUIRE_MIXING();
        const QString fileName = writeClip(qsl("stopped.wav"), 5000);
        TMediaData data = clipData(fileName, TMediaData::MediaTypeSound);
        media()->playMedia(data);
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     []() {
                                         return TAudioMixer::self()->queuedSamples() > 0;
                                     },
                                     QDeadlineTimer(5s))
                                     ? QString()
                                     : noAudioReason());

        TMediaData stop = clipData(fileName, TMediaData::MediaTypeSound);
        media()->stopMedia(stop);
        QCOMPARE(TAudioMixer::self()->queuedSamples(), 0);
    }

    void test_muteReachesAMixedSound()
    {
        REQUIRE_MIXING();
        TMediaData data = clipData(writeClip(qsl("muted.wav"), 3000), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        QVERIFY2(waitForPlaying(1), "The sound never started.");
        QVERIFY(media()->playersMixed() == 1);

        media()->muteMedia(TMediaData::MediaProtocolAPI);
        QVERIFY2(mixedVoice() && mixedVoice()->muted(), "Muting game and script media did not reach a mixed sound.");
        media()->unmuteMedia(TMediaData::MediaProtocolAPI);
        QVERIFY(!mixedVoice()->muted());
    }

    // A pooled player is handed back to TMedia's handlers for every track; its voice must stay connected too
    void test_aReusedPlayerStillStopsAtOnce()
    {
        REQUIRE_MIXING();
        const QString fileName = writeClip(qsl("reused.wav"), 3000);
        TMediaData data = clipData(fileName, TMediaData::MediaTypeSound);
        data.setMediaKey(qsl("reused"));
        TMediaData stop = clipData(fileName, TMediaData::MediaTypeSound);

        media()->playMedia(data);
        SKIP_OR_FAIL_WITHOUT(waitForQueuedAudio() ? QString() : noAudioReason());
        media()->stopMedia(stop);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return media()->playersHoldingSource() == 0;
                         },
                         QDeadlineTimer(5s)),
                 "The stopped sound never let go of its source.");

        media()->playMedia(data);
        QCOMPARE(media()->mediaPlayerCount(), 1);
        QVERIFY2(waitForQueuedAudio(), "A reused player's sound never reached the shared output.");
        media()->stopMedia(stop);
        QCOMPARE(TAudioMixer::self()->queuedSamples(), 0);
    }

    // A new player's voice hears the end of each pass after TMedia has already started the next one.
    // The output is kept shut, so what was queued stays to be seen and no audio device is needed.
    void test_aLoopingSoundStillStopsAtOnceOnALaterPass()
    {
        SKIP_OR_FAIL_WITHOUT(backendFeedsTheMix() ? QString() : noMixingReason());
        TAudioMixer::self()->setOutputSuspended(true);
        auto resume = qScopeGuard([]() {
            TAudioMixer::self()->setOutputSuspended(false);
        });
        const QString fileName = writeClip(qsl("looping.wav"), 300);
        TMediaData data = clipData(fileName, TMediaData::MediaTypeSound);
        data.setMediaLoops(TMediaData::MediaLoopsRepeat);
        media()->playMedia(data);
        QVERIFY2(waitForQueuedAudio(), "The looping sound never reached the shared output.");
        QTest::qWait(800ms);
        QVERIFY2(TAudioMixer::self()->queuedSamples() > 0, "The looping sound stopped reaching the shared output after its first pass.");

        TMediaData stop = clipData(fileName, TMediaData::MediaTypeSound);
        media()->stopMedia(stop);
        QCOMPARE(TAudioMixer::self()->queuedSamples(), 0);
    }

    void test_aSoundMovedOffTheMixAsItStartsStillEndsOnce_data()
    {
        QTest::addColumn<int>("passes");
        QTest::newRow("once") << 1;
        QTest::newRow("looped") << 2;
    }

    // A sound the backend hands no audio for is moved to an output of its own. Moved at any moment
    // around its start, before or after its first audio, it must still play every pass and be
    // reported ending once. Signalled by hand, as no backend can be made to stall on cue.
    void test_aSoundMovedOffTheMixAsItStartsStillEndsOnce()
    {
        QFETCH(const int, passes);
        SKIP_OR_FAIL_WITHOUT(backendFeedsTheMix() ? QString() : noMixingReason());
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("registerAnonymousEventHandler('sysMediaStarted', function() started = started + 1 end)\n"
                                                                         "registerAnonymousEventHandler('sysMediaFinished', function() finished = finished + 1 end)")));
        const QString fileName = writeClip(qsl("stalls.wav"), 300);

        // The move that matters lands after the decoder starts and before the first audio reaches the
        // voice, and when that is varies from run to run
        for (const int delayMs : {0, 5, 10, 15, 20, 30, 45}) {
            QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("started, finished = 0, 0")));
            TMediaData data = clipData(fileName, TMediaData::MediaTypeSound);
            data.setMediaLoops(passes);
            media()->playMedia(data);
            QTest::qWait(delayMs);
            const TMediaVoice* voice = mixedVoice();
            QVERIFY2(voice, "The sound was not played through the shared output.");
            emit const_cast<TMediaVoice*>(voice)->signal_unsupported();
            QCOMPARE(media()->playersMixed(), 0);

            const QString when = qsl("moved off the shared output %1 ms after it was played").arg(delayMs);
            QVERIFY2(QTest::qWaitFor(
                             [&]() {
                                 return media()->playersInPlayingState() == 0 && luaHolds(qsl("finished >= %1").arg(passes));
                             },
                             QDeadlineTimer(10s)),
                     qPrintable(qsl("A sound %1 never ended.").arg(when)));
            QTest::qWait(200ms);
            QVERIFY2(luaHolds(qsl("finished == %1").arg(passes)), qPrintable(qsl("A sound %1 was reported ending more than once a pass.").arg(when)));
            QVERIFY2(luaHolds(qsl("started == %1").arg(passes)), qPrintable(qsl("A sound %1 was reported starting again.").arg(when)));
        }
    }

    // The mute switch applies to a sound from its start, not only when it is next flipped
    void test_aSoundStartedWhileMutedStartsMuted()
    {
        REQUIRE_MIXING();
        MudletMedia::self()->setApiMuted(true);
        auto restore = qScopeGuard([]() {
            MudletMedia::self()->setApiMuted(false);
        });
        TMediaData data = clipData(writeClip(qsl("startsmuted.wav"), 3000), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        QVERIFY2(waitForPlaying(1), "The sound never started.");
        QVERIFY2(mixedVoice() && mixedVoice()->muted(), "A sound started while script media was muted was heard.");
    }

    // A pause keeps what a sound has queued, so it carries on from the same place when resumed
    void test_aPausedSoundKeepsItsAudio()
    {
        REQUIRE_MIXING();
        TMediaData data = clipData(writeClip(qsl("paused.wav"), 5000), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        SKIP_OR_FAIL_WITHOUT(waitForQueuedAudio() ? QString() : noAudioReason());

        TMediaData pause = clipData(qsl("paused.wav"), TMediaData::MediaTypeSound);
        media()->pauseMedia(pause);
        QTest::qWait(100ms);
        const qint64 held = TAudioMixer::self()->queuedSamples();
        QVERIFY2(held > 0, "A paused sound lost what it had queued.");
        QTest::qWait(300ms);
        QCOMPARE(TAudioMixer::self()->queuedSamples(), held);
    }

    // Paused, a sound must not hold the headphones; resumed, it needs the output back
    void test_aPausedSoundLetsTheOutputCloseAndResumingReopensIt()
    {
        REQUIRE_MIXING();
        TAudioMixer::self()->setIdleCloseDelay(400ms);
        TMediaData data = clipData(writeClip(qsl("pausedidle.wav"), 5000), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        SKIP_OR_FAIL_WITHOUT(waitForQueuedAudio() ? QString() : noAudioReason());

        TMediaData pause = clipData(qsl("pausedidle.wav"), TMediaData::MediaTypeSound);
        media()->pauseMedia(pause);
        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return !TAudioMixer::self()->outputOpen();
                         },
                         QDeadlineTimer(5s)),
                 "A paused sound kept the shared output open.");

        media()->playMedia(data);
        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return TAudioMixer::self()->outputOpen();
                         },
                         QDeadlineTimer(5s)),
                 "A resumed sound never got the shared output back.");
    }

    // Mudlet's main thread runs scripts and triggers, and can be busy for a while; the sound must not wait on it
    void test_aBusyMainThreadDoesNotCutTheSoundOut()
    {
        REQUIRE_MIXING();
        TMediaData data = clipData(writeClip(qsl("steady.wav"), 5000), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        SKIP_OR_FAIL_WITHOUT(waitForQueuedAudio() ? QString() : noAudioReason());
        QTest::qWait(300ms);

        const int ranDryBefore = TAudioMixer::self()->timesRanDry();
        QElapsedTimer busy;
        busy.start();
        while (busy.elapsed() < 400) {
            // Blocking on purpose, as a long script would: no events are processed
        }
        QTest::qWait(300ms);
        QCOMPARE(TAudioMixer::self()->timesRanDry(), ranDryBefore);
    }

    // Once the check of a backend nobody named has answered, every sound is mixed
    void test_aCheckedBackendMixesTheFirstSound()
    {
        if (!qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty()) {
            QSKIP("A backend is named, so there is nothing to check before the first sound.");
        }
        TMediaVoice::probeBackend();
        QVERIFY(TMediaVoice::probeStarted());
        SKIP_OR_FAIL_WITHOUT(QTest::qWaitFor(
                                     []() {
                                         return TMediaVoice::probePassed();
                                     },
                                     QDeadlineTimer(6s))
                                     ? QString()
                                     : noMixingReason());

        TMediaData first = clipData(writeClip(qsl("first.wav"), 3000), TMediaData::MediaTypeSound);
        media()->playMedia(first);
        QCOMPARE(media()->playersMixed(), 1);
    }

    // A backend that is known not to feed a voice keeps every player on an output of its own
    void test_aBackendThatCannotFeedTheMixKeepsOutputsOfItsOwn()
    {
        if (backendFeedsTheMix()) {
            QSKIP("This Qt Multimedia backend feeds the shared output, so there is no fallback to see.");
        }
        TMediaData data = clipData(writeClip(qsl("fallback.wav"), 3000), TMediaData::MediaTypeSound);
        media()->playMedia(data);
        QCOMPARE(media()->playersMixed(), 0);
    }

private:
    TMedia* media() const { return mpHost->mpMedia.data(); }

    // Only one sound plays where this is asked
    const TMediaVoice* mixedVoice() const { return media()->firstMixedVoice(); }

    QString noMixingReason() const
    {
        return qsl("This Qt Multimedia backend (\"%1\") does not feed the shared output, so every player keeps an output of its own. Needs Qt's FFmpeg backend.")
                .arg(QString::fromLocal8Bit(qgetenv("QT_MEDIA_BACKEND")));
    }

    // Asking starts the one-time probe of a backend nobody named, which takes a moment to answer
    static bool backendFeedsTheMix()
    {
        if (!qEnvironmentVariable("QT_MEDIA_BACKEND").isEmpty()) {
            return !TMediaVoice::unsupported();
        }
        return QTest::qWaitFor(
                []() {
                    return !TMediaVoice::unsupported();
                },
                QDeadlineTimer(6s));
    }

    bool waitForQueuedAudio() const
    {
        return QTest::qWaitFor(
                []() {
                    return TAudioMixer::self()->queuedSamples() > 0;
                },
                QDeadlineTimer(5s));
    }

    QString noAudioReason() const { return qsl("No audio reached the shared output, so it never opened: this runner may have no audio device."); }

    TMediaData clipData(const QString& fileName, const int type) const
    {
        TMediaData data;
        data.setMediaProtocol(TMediaData::MediaProtocolAPI);
        data.setMediaType(type);
        data.setMediaInput(TMediaData::MediaInputFile);
        data.setMediaFileName(fileName);
        data.setMediaVolume(1); // quiet enough not to disturb a desktop run
        return data;
    }

    // compileAndExecuteScript() reports a raised error as false, so a failed assert() comes back as one
    bool luaHolds(const QString& condition) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(%1)").arg(condition)); }

    bool waitForPlaying(const int players) const
    {
        return QTest::qWaitFor(
                [&]() {
                    return media()->playersInPlayingState() >= players;
                },
                QDeadlineTimer(10s));
    }

    // Sends a GMCP message and a line after it, and waits for the line: the message was handled first
    bool gameSays(const QString& gmcp)
    {
        static int lines = 0;
        const QString line = qsl("line %1 from the game").arg(++lines);
        mpServer->sendRaw(QByteArray("\xff\xfa\xc9", 3) + gmcp.toUtf8() + QByteArray("\xff\xf0", 2) + line.toUtf8() + "\r\n");
        return QTest::qWaitFor(
                [&]() {
                    TBuffer& buffer = mpHost->mainConsoleView()->buffer;
                    for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
                        if (buffer.line(i).contains(line)) {
                            return true;
                        }
                    }
                    return false;
                },
                QDeadlineTimer(8s));
    }

    QString writeClip(const QString& fileName, const int durationMs) const
    {
        const QString mediaPath = MudletApp::getMudletPath(enums::profileMediaPath, mHostname);
        if (!QDir().mkpath(mediaPath)) {
            QTest::qFail("Could not create the profile media directory.", __FILE__, __LINE__);
            return {};
        }
        QFile file(qsl("%1/%2").arg(mediaPath, fileName));
        if (!file.open(QIODevice::WriteOnly)) {
            QTest::qFail("Could not write the test media file.", __FILE__, __LINE__);
            return {};
        }
        file.write(wavBytes(durationMs));
        return fileName;
    }

    // Silent 16-bit mono PCM: what is asserted is where the audio goes, not what it sounds like
    static QByteArray wavBytes(const int durationMs)
    {
        constexpr int sampleRate = 8000;
        constexpr int bytesPerSample = 2;
        const int dataBytes = sampleRate * bytesPerSample * durationMs / 1000;

        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << static_cast<quint32>(36 + dataBytes);
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << static_cast<quint32>(16);
        out << static_cast<quint16>(1);
        out << static_cast<quint16>(1);
        out << static_cast<quint32>(sampleRate);
        out << static_cast<quint32>(sampleRate * bytesPerSample);
        out << static_cast<quint16>(bytesPerSample);
        out << static_cast<quint16>(8 * bytesPerSample);
        out.writeRawData("data", 4);
        out << static_cast<quint32>(dataBytes);
        wav.append(QByteArray(dataBytes, '\0'));
        return wav;
    }
};

#include "TAudioMixerTest.moc"
MUDLET_GROUPED_TEST_MAIN(TAudioMixerTest)
