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
 * Regression guard for #9659: an interrupting ttsSpeak() dropping the utterance
 * it was asked to speak.
 *
 * Every speech engine Qt wraps stops the running utterance inside say() and
 * reports Ready for it - speech-dispatcher, SAPI, WinRT and AVFoundation all
 * do. Mudlet raises its TTS events off those state changes and drains
 * ttsQueue() on any Ready, so that one was read as "the engine is idle": the
 * queued line was spoken straight over the utterance the script had just asked
 * for, and that utterance was never heard at all.
 *
 * Qt's mock engine, which the Lua specs in Media_spec.lua use, never reports
 * that Ready - it stays in Speaking with no state change at all, which is the
 * other half of the same issue and is covered there. So the guard itself needs
 * that Ready delivered the way a real engine delivers it, which is what this
 * test does: it drives the Lua API for everything else and hands
 * TLuaInterpreter::ttsStateChanged() the state change the engine's
 * QTextToSpeech::stateChanged signal would have carried.
 *
 * Run with: ctest -R TtsInterruptingSpeakTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "mudlet.h"

#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForTtsInterruptingSpeakTest();

class TtsInterruptingSpeakTest : public QObject
{
    Q_OBJECT

private:
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("TtsInterruptingSpeak-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

#ifdef QT_TEXTTOSPEECH_LIB
    // Runs a snippet in the profile's Lua state, failing the test with the
    // script's own error message when it does not run cleanly. Lua assert()s in
    // the snippet are how the queue and the current line are read back.
    bool runLua(const QString& script) { return mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }
#endif

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForTtsInterruptingSpeakTest();

        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        // Picked up by TLuaInterpreter::ttsBuild(), which then asks for Qt's
        // deterministic mock engine instead of whatever the host machine would
        // otherwise speak out loud.
        qputenv("MUDLET_TEST_MODE", "1");

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(mudlet::self()->getHostManager().addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);
        // A bare Host blocks script compilation until the full profile boot
        // would clear this, and these tests need their snippets to compile:
        mpHost->mBlockScriptCompile = false;
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        qunsetenv("MUDLET_TEST_MODE");
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theReadyFromAnInterruptedUtteranceDoesNotDrainTheQueue()
    {
#ifndef QT_TEXTTOSPEECH_LIB
        QSKIP("Mudlet was built without text-to-speech support");
#else
        QVERIFY2(runLua(qsl("ttsClearQueue() ttsSkip()")), "could not reset the TTS state");
        if (!runLua(qsl("assert(#ttsGetVoices() > 0)"))) {
            QSKIP("Qt's mock speech engine is unavailable here, so nothing can be made to speak");
        }

        QVERIFY(runLua(qsl("ttsSpeak('the utterance already being spoken')")));
        QVERIFY(runLua(qsl("ttsQueue('the queued line')")));
        QVERIFY(runLua(qsl("ttsSpeak('the utterance the script asked for')")));

        // What every real engine reports next: the utterance say() stopped has
        // ended. It is not the engine falling idle, and the queue must survive
        // it - the requested utterance is what should be being spoken.
        TLuaInterpreter::ttsStateChanged(QTextToSpeech::State::Ready);

        QVERIFY2(runLua(qsl("assert(#ttsGetQueue() == 1, 'the queued line was spoken over the utterance ttsSpeak() asked for, queue holds '..#ttsGetQueue())")),
                 "the queue was drained by the Ready that reported the interrupted utterance ending");
        QVERIFY2(runLua(qsl("assert(ttsGetCurrentLine() == 'the utterance the script asked for', 'the current line became: '..tostring(ttsGetCurrentLine()))")),
                 "the drained line replaced the utterance the script asked for");

        QVERIFY(runLua(qsl("ttsClearQueue() ttsSkip()")));
#endif
    }

    // The other side of the same guard: an explicit stop really does leave the
    // engine idle, so its Ready has to keep draining the queue as it always has.
    void test_theReadyFromAnExplicitSkipStillDrainsTheQueue()
    {
#ifndef QT_TEXTTOSPEECH_LIB
        QSKIP("Mudlet was built without text-to-speech support");
#else
        QVERIFY2(runLua(qsl("ttsClearQueue() ttsSkip()")), "could not reset the TTS state");
        if (!runLua(qsl("assert(#ttsGetVoices() > 0)"))) {
            QSKIP("Qt's mock speech engine is unavailable here, so nothing can be made to speak");
        }

        QVERIFY(runLua(qsl("ttsSpeak('the utterance being spoken over')")));
        QVERIFY(runLua(qsl("ttsSpeak('the utterance the script asked for')")));
        QVERIFY(runLua(qsl("ttsQueue('the queued line')")));
        QVERIFY(runLua(qsl("ttsSkip()")));

        QVERIFY2(runLua(qsl("assert(#ttsGetQueue() == 0, 'the queue still holds '..#ttsGetQueue())")), "an explicit skip left the queue undrained");
        QVERIFY2(runLua(qsl("assert(ttsGetCurrentLine() == 'the queued line', 'the current line is: '..tostring(ttsGetCurrentLine()))")), "the skip did not start speaking the queued line");

        QVERIFY(runLua(qsl("ttsClearQueue() ttsSkip()")));
#endif
    }
};

void initializeQRCResourcesForTtsInterruptingSpeakTest()
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

#include "TtsInterruptingSpeakTest.moc"
QTEST_MAIN(TtsInterruptingSpeakTest)
