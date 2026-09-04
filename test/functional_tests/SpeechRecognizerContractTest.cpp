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

/*
 * The library-shape half of the stt.* contract, which STT_spec.lua cannot
 * reach.
 *
 * The spec runs against the Lua bindings, and on a machine with no engine
 * installed those refuse before they touch a recognizer - so an invariant about
 * what the backend claims when a symbol is missing is invisible from there. It
 * showed: three of the fixes this branch claims could be reverted with the whole
 * spec still green.
 *
 * These run in the configuration CI actually has - no libvosk present, every
 * function pointer null - which is the same shape as the incomplete library that
 * found the defects in the first place. Verified by reverting each fix and
 * watching the case go red, which is also the limit worth stating: with no
 * library, initialize() refuses at its first guard, so nothing below that guard
 * is reachable. That is why modelPath() and currentLanguage() read the live
 * model handle rather than a remembered string - the answer is then true from
 * paths CI cannot reach, instead of resting on where an assignment happens to
 * sit. aReleasedModelIsNoLongerNamed() is the one case that does go below the
 * guard, and it skips wherever no library loads: run it against a real libvosk,
 * or against a stand-in exporting the ten symbols loadVoskLibrary() resolves,
 * to see dropping that gate go red.
 *
 * Run with: ctest -R SpeechRecognizerContractTest -V
 */

#include <QtTest/QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "SherpaRecognizer.h"
#include "SpeechRecognizer.h"
#include "SpeechRecognizerFactory.h"
#include "TLuaInterpreter.h"
#include "VoskRecognizer.h"
#include "mudlet.h"

#if defined(Q_OS_MACOS)
#include "AppleSpeechRecognizer.h"
#endif

#include "GroupedTest.h"

#include <memory>
#include <optional>

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

namespace {

// Minimal stand-in for a backend that can bias. No in-tree backend can yet -
// VoskRecognizer hardcodes biasing/grammar to false - so setVocabulary()'s
// short-circuit around a failed applyVocabulary() has nothing else to run
// against. Only the pure virtuals need real bodies; capabilities() and
// applyVocabulary() are the two calls the test actually cares about.
class BiasingStubRecognizer : public SpeechRecognizer
{
public:
    Capabilities capabilities() const override
    {
        Capabilities can;
        can.biasing = mBiasingSupported;
        return can;
    }

    bool initialize(const QString&) override { return true; }
    QString currentLanguage() const override { return QString(); }
    bool setLanguage(const QString&) override { return true; }
    QString backendName() const override { return qsl("BiasingStub"); }
    QString backendVersion() const override { return qsl("1.0"); }
    bool setSensitivity(Sensitivity) override { return true; }
    Sensitivity sensitivity() const override { return Sensitivity::Default; }

    // Test-controlled outcome and call count, so the fix's short-circuit can
    // be proven both ways: it must not skip a re-apply while the backend is
    // still failing, and it must skip one once the backend has succeeded.
    VocabularyResult mNextApplyResult = VocabularyResult::Failed;
    int mApplyCallCount = 0;

    // Flipped off to prove a backend with neither biasing nor grammar is
    // never asked to apply anything at all, not merely told Unsupported.
    bool mBiasingSupported = true;

protected:
    void doStartListening() override {}
    void doStopListening() override {}
    void doCancel() override {}

    VocabularyResult applyVocabulary(const QStringList& words) override
    {
        Q_UNUSED(words)
        ++mApplyCallCount;
        return mNextApplyResult;
    }
};

// A backend whose start does not complete inside the call, the way one
// waiting on a permission dialog does not. Nothing in tree can be held in
// Starting from a test - Vosk reaches it only behind a real microphone
// permission prompt - so the base class's rules for that state need a backend
// whose start parks there on demand.
class PendingStartStubRecognizer : public SpeechRecognizer
{
public:
    // Ready is what startListening() requires, and there is no model to load
    // to get there
    bool initialize(const QString&) override
    {
        setState(State::Ready);
        return true;
    }
    QString currentLanguage() const override { return QString(); }
    bool setLanguage(const QString&) override { return true; }
    QString backendName() const override { return qsl("PendingStartStub"); }
    QString backendVersion() const override { return qsl("1.0"); }
    bool setSensitivity(Sensitivity) override { return true; }
    Sensitivity sensitivity() const override { return Sensitivity::Default; }

    int mStopCallCount = 0;
    int mCancelCallCount = 0;

protected:
    void doStartListening() override { setState(State::Starting); }
    void doStopListening() override { ++mStopCallCount; }
    void doCancel() override { ++mCancelCallCount; }
};

} // namespace

class SpeechRecognizerContractTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    // One word of Vosk's "result" array
    static QJsonObject word(const QString& text, const double start, const double end)
    {
        QJsonObject entry;
        entry.insert(qsl("word"), text);
        entry.insert(qsl("conf"), 1.0);
        entry.insert(qsl("start"), start);
        entry.insert(qsl("end"), end);
        return entry;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        // VoskRecognizer's path helpers go through mudlet::getMudletPath(),
        // which dereferences mudlet::self()
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        // The latch is process-global, so a case that sets it must not leave it
        // standing for the next one. So is the load state, and on a machine
        // that has libvosk installed that is the one which carries: every case
        // here shares a process, an earlier one calling initialize() leaves
        // sLibraryLoaded standing, and libraryAvailable() then answers from
        // that cache without re-running the probe the next case is about. The
        // order is stt.reloadLibrary()'s - release first, then lift the latch,
        // since a refused release would leave the flags it reads still set.
        VoskRecognizer::resetLibraryLoadState();
        VoskRecognizer::unloadLibraryByRequest(false);
    }

    // A capability is a promise that an event will arrive. Claimed without the
    // symbol behind it, a package that waits for sysSTTWords before acting on
    // sysSTTResult waits for ever.
    void wordResultsIsNotClaimedWithoutTheLibrary()
    {
        if (VoskRecognizer::libraryAvailable()) {
            QSKIP("libvosk is installed here, so the symbol this case is about may legitimately have resolved");
        }

        VoskRecognizer recognizer;
        QVERIFY2(!recognizer.capabilities().wordResults, "words was claimed with no library to supply timings");
        QVERIFY2(recognizer.capabilities().onDevice, "Vosk decodes locally whether or not it is loaded");
    }

    // Remembering a sensitivity the engine was never told about makes the
    // readback agree with the caller and disagree with the engine - pauses that
    // never happen, with every readback insisting they are configured. On a
    // libvosk without the endpointer symbol, calling through it is a null
    // function pointer rather than a wrong answer.
    void sensitivityIsRefusedRatherThanRememberedWithoutTheSymbol()
    {
        if (VoskRecognizer::libraryAvailable()) {
            QSKIP("libvosk is installed here, so the endpointer symbol may legitimately have resolved");
        }

        VoskRecognizer recognizer;
        QVERIFY2(!recognizer.setSensitivity(SpeechRecognizer::Sensitivity::Short), "a sensitivity was accepted with no engine to apply it to");
        QCOMPARE(recognizer.sensitivity(), SpeechRecognizer::Sensitivity::Default);
        QVERIFY2(!recognizer.setEndpointerMode(VoskRecognizer::EndpointerMode::Long), "an endpointer mode was accepted with no symbol to set it through");
        QCOMPARE(recognizer.endpointerMode(), VoskRecognizer::EndpointerMode::Default);
    }

    // getInfo().modelPath is documented as "the model actually loaded (empty
    // when none)", and a package reads it to decide whether setup has already
    // happened. Naming a model that failed to load, or one that has just been
    // freed, makes it skip the init it needs.
    //
    // Where the assignment sits inside initialize() is not what this holds -
    // with no library installed initialize() refuses at its first guard, so the
    // later failure paths are not reachable from here at all, and a case
    // written against the assignment order would pass whatever that order was.
    // modelPath() reads the live model handle instead, which is what makes the
    // answer true from every path rather than from the ones a test can reach.
    void modelPathNamesOnlyAModelThatLoaded()
    {
        VoskRecognizer recognizer;
        QVERIFY(recognizer.modelPath().isEmpty());

        QVERIFY(!recognizer.initialize(qsl("/definitely/not/a/model/path/for/testing")));
        QVERIFY2(recognizer.modelPath().isEmpty(), "a path that does not exist was reported as the loaded model");

        // A directory that exists but is not a model: past the existence check,
        // refused by the engine - or by there being no engine
        QVERIFY(!recognizer.initialize(mConfigDir.path()));
        QVERIFY2(recognizer.modelPath().isEmpty(), "a directory the engine refused was reported as the loaded model");
        QVERIFY2(recognizer.currentLanguage().isEmpty(), "a language was reported for a model that never loaded");
        QVERIFY2(!recognizer.hasLiveNativeResources(), "a failed load left native handles behind");

        recognizer.releaseResources();
        QVERIFY(recognizer.modelPath().isEmpty());
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
    }

    // stt.unloadLibrary() exists so the library's file can be replaced, which
    // Windows refuses while the module is mapped. A stt.init() that maps it
    // back in behind the caller locks the file again with nothing to explain
    // the permission error that follows.
    void initDoesNotLiftTheUnloadLatch()
    {
        VoskRecognizer::unloadLibraryByRequest(true);
        QVERIFY2(!VoskRecognizer::libraryAvailable(), "the latch did not keep the library out");

        VoskRecognizer recognizer;
        QSignalSpy errors(&recognizer, &SpeechRecognizer::errorOccurred);
        QVERIFY(errors.isValid());

        QVERIFY(!recognizer.initialize(mConfigDir.path()));
        QVERIFY2(!VoskRecognizer::libraryAvailable(), "stt.init() mapped the library back in behind the unload");

        // The refusal has to name the latch rather than report the library as
        // missing: on a machine where it is installed those are opposite
        // remedies, and with none installed the message is the only thing that
        // tells the two guards apart at all.
        QCOMPARE(errors.count(), 1);
        const QString message = errors.first().first().toString();
        QVERIFY2(message.contains(qsl("reloadLibrary")), qPrintable(qsl("the refusal did not name what lifts the latch: %1").arg(message)));
    }

    // Documented as re-readable rather than cacheable, which needs the change
    // to be announced at all - and announced once, not on every read.
    void capabilityChangesAreAnnouncedOnce()
    {
        VoskRecognizer recognizer;
        QSignalSpy spy(&recognizer, &SpeechRecognizer::capabilitiesChanged);
        QVERIFY(spy.isValid());

        recognizer.announceCapabilitiesIfChanged();
        QCOMPARE(spy.count(), 1);
        recognizer.announceCapabilitiesIfChanged();
        QCOMPARE(spy.count(), 1);
    }

    // docs/stt-api.md: "refusal messages can arrive without a state change".
    // A start refused for want of a model is a refusal, not a fault - the
    // recognizer is exactly as usable afterwards as before, so moving it to
    // Error told a package driving its controls from state to offer a reload
    // of a model that was never loaded. The rule lives in the base class so
    // every backend refuses the same way, and this pins it there.
    void aRefusedStartLeavesTheStateAlone()
    {
        VoskRecognizer recognizer;
        QSignalSpy errors(&recognizer, &SpeechRecognizer::errorOccurred);
        QSignalSpy states(&recognizer, &SpeechRecognizer::stateChanged);
        QVERIFY(errors.isValid());
        QVERIFY(states.isValid());

        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
        recognizer.startListening();
        QCOMPARE(errors.count(), 1);
        QVERIFY2(states.isEmpty(), "a refusal is not a fault, so it must not announce a state change");
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);

        // Nothing is listening, so there is nothing to stop or abandon, and
        // neither call may say anything about it
        recognizer.stopListening();
        recognizer.cancel();
        QCOMPARE(errors.count(), 1);
        QVERIFY(states.isEmpty());
    }

    // A start that has not completed is still a start, and stopping one has to
    // withdraw it: leaving the request pending means answering the permission
    // dialog later opens the microphone after the caller asked to stop. The
    // base class routes Starting through cancel() for exactly that, and
    // nothing pinned it - the word did not appear in this file at all, so the
    // branch could be deleted with every case still green.
    void stoppingAPendingStartWithdrawsIt()
    {
        PendingStartStubRecognizer recognizer;
        QSignalSpy states(&recognizer, &SpeechRecognizer::stateChanged);
        QVERIFY(states.isValid());

        QVERIFY(recognizer.initialize(QString()));
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Ready);

        recognizer.startListening();
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Starting);

        recognizer.stopListening();
        QVERIFY2(recognizer.state() == SpeechRecognizer::State::Ready, "a stop while still Starting left the request pending, so answering the permission dialog would open the microphone anyway");

        // Withdrawn by the base itself, which is why the header tells a
        // backend's own continuation to re-check state() rather than wait for
        // a doCancel() that never comes
        QCOMPARE(recognizer.mCancelCallCount, 0);
        // Nothing was captured, so there is nothing to finalise either
        QCOMPARE(recognizer.mStopCallCount, 0);

        // Announced, not merely arrived at: Ready, Starting, Ready
        QCOMPARE(states.count(), 3);
        QCOMPARE(states.at(2).at(0).value<SpeechRecognizer::State>(), SpeechRecognizer::State::Ready);
    }

    // modelPath() and currentLanguage() are gated on the live model handle
    // rather than reading remembered strings, so a released model cannot still
    // be named - releaseResources() clears mModelPath but nothing clears the
    // language, and the gate is the whole of what keeps that honest.
    //
    // Needs a library and a model it accepts, which CI has neither of: with no
    // libvosk, initialize() refuses at its first guard and no handle is ever
    // taken, so there is nothing for a release to have to hide. Where they are
    // installed - a developer machine, or a run with a stand-in library on the
    // search path - this is the case that catches the gate being dropped.
    void aReleasedModelIsNoLongerNamed()
    {
        // A model directory of this case's own, rather than whatever happens
        // to be installed: the library is what decides whether this can run,
        // and depending on a downloaded model as well would narrow that to
        // almost nowhere. Taken away again at the end, since
        // listModelsSpansEveryModelBasedEngine() counts what is on disk.
        const QString modelPath = qsl("%1/vosk-model-contract-test").arg(VoskRecognizer::modelsDirectoryPath());
        QVERIFY(QDir().mkpath(qsl("%1/am").arg(modelPath)));
        auto removeModelDir = qScopeGuard([modelPath]() {
            QDir(modelPath).removeRecursively();
        });

        VoskRecognizer recognizer;
        if (!recognizer.initialize(modelPath)) {
            QSKIP("no Vosk library that loads a model here, so no live model handle can be taken to release");
        }

        QVERIFY2(!recognizer.modelPath().isEmpty(), "a loaded model must be named");
        QVERIFY2(!recognizer.currentLanguage().isEmpty(), "a loaded model must report the language it was read as");
        QVERIFY(recognizer.hasLiveNativeResources());

        recognizer.releaseResources();
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
        QVERIFY2(!recognizer.hasLiveNativeResources(), "releaseResources() left native handles behind");
        QVERIFY2(recognizer.modelPath().isEmpty(), "a model path survived the model it describes");
        QVERIFY2(recognizer.currentLanguage().isEmpty(), "a language survived the model it was read from");
    }

    // Which models can be biased is only known once one is loaded, so words a
    // package offers while an unbiasable model is loaded - or before any is -
    // would be lost, and a later switch to a model that can bias would compile
    // in nothing. The base class keeps them for every backend, so none has to
    // remember to. Vosk cannot bias at all, which is what makes it the right
    // backend to prove the words survive an Unsupported answer.
    void wordsOfferedToABackendThatCannotUseThemAreStillKept()
    {
        VoskRecognizer recognizer;
        QVERIFY2(!recognizer.supportsBiasing() && !recognizer.supportsGrammar(), "this case needs a backend that cannot take vocabulary");

        const QStringList words{qsl("kill"), qsl("look"), qsl("inventory")};
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Unsupported);
        QCOMPARE(recognizer.vocabulary(), words);

        // Offered again unchanged: still Unsupported here, and still held
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Unsupported);
        QCOMPARE(recognizer.vocabulary(), words);
    }

    // setVocabulary()'s short-circuit for an unchanged offer must answer
    // Applied only when the last actual attempt succeeded - not merely
    // because the words match what was offered before. A backend that keeps
    // answering Failed for the same words must be asked again, not agreed
    // with, and once it does succeed a further unchanged offer must not
    // trigger a redundant re-apply.
    void aFailedApplyIsNotShortCircuitedIntoApplied()
    {
        BiasingStubRecognizer recognizer;
        QVERIFY2(recognizer.supportsBiasing(), "this case needs a backend that can bias");

        const QStringList words{qsl("kill"), qsl("look"), qsl("inventory")};

        recognizer.mNextApplyResult = SpeechRecognizer::VocabularyResult::Failed;
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Failed);
        QCOMPARE(recognizer.mApplyCallCount, 1);

        // Same words offered again while still failing: must not be agreed
        // with as if they were in effect
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Failed);
        QCOMPARE(recognizer.mApplyCallCount, 2);

        // The backend recovers; the same words now succeed
        recognizer.mNextApplyResult = SpeechRecognizer::VocabularyResult::Applied;
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Applied);
        QCOMPARE(recognizer.mApplyCallCount, 3);

        // Offered again unchanged: short-circuits now, so no redundant re-apply
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Applied);
        QCOMPARE(recognizer.mApplyCallCount, 3);

        // A different word list must still re-apply even though the last
        // attempt succeeded - the short-circuit is for an unchanged offer,
        // not for a backend that is currently in a good mood
        const QStringList otherWords{qsl("cast"), qsl("quaff")};
        QCOMPARE(recognizer.setVocabulary(otherWords), SpeechRecognizer::VocabularyResult::Applied);
        QCOMPARE(recognizer.mApplyCallCount, 4);
    }

    // setVocabulary() must not call through to a backend that has said it can
    // do nothing with vocabulary - Unsupported has to mean the words were
    // never handed over, not merely that nothing came back from doing so.
    void unsupportedVocabularyNeverReachesTheBackend()
    {
        BiasingStubRecognizer recognizer;
        recognizer.mBiasingSupported = false;
        QVERIFY2(!recognizer.supportsBiasing() && !recognizer.supportsGrammar(), "this case needs a backend that cannot take vocabulary");

        const QStringList words{qsl("kill"), qsl("look"), qsl("inventory")};
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Unsupported);
        QCOMPARE(recognizer.mApplyCallCount, 0);
    }

    // Reported as the place to install a model into, so it has to be a place
    // that exists. A made-up default named a directory the user never created,
    // and the "install a model" message was unreachable behind it.
    //
    // Only asserts anything on a machine with a Vosk model installed:
    // VoskRecognizer::defaultModelPath() has no made-up fallback any more - that
    // absence is the fix - so on a clean CI machine it is empty and this returns
    // without checking anything. Kept because it is the one place the regression
    // would be caught, and it is caught on any developer machine set up to use
    // the feature at all.
    void theDefaultModelPathIsAModelOrNothing()
    {
        const QString defaultPath = VoskRecognizer::defaultModelPath();
        if (defaultPath.isEmpty()) {
            return;
        }
        QVERIFY2(QDir(defaultPath).exists(), qPrintable(qsl("the default model path names a directory that does not exist: %1").arg(defaultPath)));
    }

    // The timings are what tell a decoder artifact from a spoken word: the
    // decoder assigns the silence before an utterance to its first word, so a
    // phantom "the" carries the whole pause. Without them there is no evidence,
    // and guessing costs the player the first word of every phrase.
    void aLeadingWordIsOnlyPhantomWhenTheTimingsSaySo()
    {
        QVERIFY2(!VoskRecognizer::leadingWordIsPhantom(QJsonArray{}), "with no word detail at all there is nothing to judge by");

        QJsonObject untimed;
        untimed.insert(qsl("word"), qsl("the"));
        QVERIFY2(!VoskRecognizer::leadingWordIsPhantom(QJsonArray{untimed}), "a word with no start/end must not be struck on a guess");

        QVERIFY2(!VoskRecognizer::leadingWordIsPhantom(QJsonArray{word(qsl("the"), 0.10, 0.34), word(qsl("dragon"), 0.34, 0.81)}), "a briefly spoken leading word was struck as a phantom");
        QVERIFY2(VoskRecognizer::leadingWordIsPhantom(QJsonArray{word(qsl("the"), 0.00, 2.50), word(qsl("dragon"), 2.50, 2.92)}), "a leading word spanning 2.5s of silence was kept as speech");
    }

    // sysSTTWords describes the text sysSTTResult carried. When a word is
    // struck from the text it has to go from the word list too, or the two
    // events describe different phrases.
    void theWordListDescribesTheTextAsEmitted()
    {
        const QJsonArray words{word(qsl("the"), 0.00, 2.50), word(qsl("dragon"), 2.50, 2.92), word(qsl("attacks"), 2.92, 3.40)};

        const QVariantList kept = VoskRecognizer::wordsFromResult(words, false);
        QCOMPARE(kept.size(), 3);
        QCOMPARE(kept.first().toMap().value(qsl("word")).toString(), qsl("the"));

        const QVariantList stripped = VoskRecognizer::wordsFromResult(words, true);
        QCOMPARE(stripped.size(), 2);
        QCOMPARE(stripped.first().toMap().value(qsl("word")).toString(), qsl("dragon"));
        QCOMPARE(stripped.first().toMap().value(qsl("start")).toDouble(), 2.50);
    }

    // docs/stt-api.md allows dropping only what was not spoken. A result the
    // decoder never produced, and one whose bytes do not parse, both used to
    // leave the transcription empty - which is the path "nothing was said"
    // takes - so a phrase the engine had already accepted was discarded with
    // no event and no log line, indistinguishable to the player from a
    // microphone that heard nothing.
    void aResultThatCannotBeReadIsReportedRatherThanDropped()
    {
        QJsonObject result;
        QString reason;

        QVERIFY2(!VoskRecognizer::parseEngineResult(nullptr, result, reason), "a result the decoder never produced was read as a transcription");
        QVERIFY2(!reason.isEmpty(), "nothing was said about the missing result");

        reason.clear();
        QVERIFY2(!VoskRecognizer::parseEngineResult(R"({"text":)", result, reason), "truncated JSON was read as a transcription");
        QVERIFY2(!reason.isEmpty(), "nothing was said about the unreadable result");

        reason.clear();
        QVERIFY2(!VoskRecognizer::parseEngineResult("[]", result, reason), "a JSON array was read as a result object");
        QVERIFY2(!reason.isEmpty(), "nothing was said about the unexpected shape");

        reason.clear();
        QVERIFY2(VoskRecognizer::parseEngineResult(R"({"text":"the dragon attacks"})", result, reason), qPrintable(reason));
        QCOMPARE(result.value(qsl("text")).toString(), qsl("the dragon attacks"));
    }

    // Every setting Mudlet keeps lives in the Mudlet.ini inside its config
    // directory. A default-constructed QSettings is a different store - native
    // format keyed by the organisation and application names - which ignores
    // the redirected config directory that portable mode is, and carries the
    // application name in its path, so a release build and a public test build
    // would not read each other's choice of model.
    void theSelectedModelIsReadFromMudletsOwnSettings()
    {
        const QString modelsDir = VoskRecognizer::modelsDirectoryPath();
        // Two installed models, so which one is named proves where the answer
        // came from: getBestAvailableModel() scores the English one higher, and
        // it is what a selection has to be able to override
        QVERIFY(QDir().mkpath(qsl("%1/vosk-model-small-en-us-0.15/am").arg(modelsDir)));
        QVERIFY(QDir().mkpath(qsl("%1/vosk-model-small-fr-0.22/am").arg(modelsDir)));

        auto* pSettings = mudlet::getQSettings();
        QVERIFY(pSettings);
        pSettings->beginGroup(qsl("SpeechRecognition"));
        pSettings->remove(qsl("selectedModel"));
        pSettings->endGroup();

        QCOMPARE(QDir(VoskRecognizer::getSelectedModelPath()).dirName(), qsl("vosk-model-small-en-us-0.15"));
        QVERIFY(VoskRecognizer::missingSelectedModel().isEmpty());

        pSettings->beginGroup(qsl("SpeechRecognition"));
        pSettings->setValue(qsl("selectedModel"), qsl("vosk-model-small-fr-0.22"));
        pSettings->endGroup();
        QVERIFY2(QDir(VoskRecognizer::getSelectedModelPath()).dirName() == qsl("vosk-model-small-fr-0.22"), "the chosen model was not read back from the settings Mudlet writes");

        // The substitution notice stt.init() raises hangs off this, so a read
        // from the wrong store leaves it unreachable however the key was set
        pSettings->beginGroup(qsl("SpeechRecognition"));
        pSettings->setValue(qsl("selectedModel"), qsl("vosk-model-that-was-deleted"));
        pSettings->endGroup();
        QCOMPARE(VoskRecognizer::missingSelectedModel(), qsl("vosk-model-that-was-deleted"));

        pSettings->beginGroup(qsl("SpeechRecognition"));
        pSettings->remove(qsl("selectedModel"));
        pSettings->endGroup();
        QVERIFY(QDir(qsl("%1/vosk-model-small-en-us-0.15").arg(modelsDir)).removeRecursively());
        QVERIFY(QDir(qsl("%1/vosk-model-small-fr-0.22").arg(modelsDir)).removeRecursively());
    }
    // The filter that decides whether a lone filler word was really said. Its
    // audio-level term used to be read at the moment the decoder endpoints -
    // which is after the trailing pause that caused the endpoint - so it was
    // always true by then and discarded the word whatever the decoder's
    // confidence. Measured before the fix: a phrase peaking at 0.457 was down
    // to 0.013 against a 0.05 gate, and a conf-1.0 "i" was delivered when the
    // script called stop() but dropped when the decoder endpointed on its own.
    // "i" is how a MUD player asks for their inventory.
    void aConfidentLoneFillerWordSurvivesBothWaysOfFinishing()
    {
        const std::optional<double> certain{1.0};
        const std::optional<double> unsure{0.4};
        const std::optional<double> none{};

        QVERIFY2(!VoskRecognizer::loneFillerWordWasNotSpoken(qsl("i"), certain), "a confidently decoded inventory command was treated as a decoder artifact");
        QVERIFY2(!VoskRecognizer::loneFillerWordWasNotSpoken(qsl("you"), certain), "a confidently decoded filler word was treated as a decoder artifact");

        QVERIFY2(VoskRecognizer::loneFillerWordWasNotSpoken(qsl("i"), unsure), "a filler word the decoder itself doubted was reported as speech");
        QVERIFY2(VoskRecognizer::loneFillerWordWasNotSpoken(qsl("the"), none), "a filler word with no confidence to judge by was reported as speech");

        QVERIFY2(!VoskRecognizer::loneFillerWordWasNotSpoken(qsl("dragon"), none), "a word that is not a filler word was discarded");
        QVERIFY2(!VoskRecognizer::loneFillerWordWasNotSpoken(qsl("i attack"), none), "a phrase was discarded as though it were a lone filler word");
    }

    // The model directory chooses the engine, so stt.init(path) needs no
    // second argument and a package that installed one engine's models never
    // has to name it. A layout that matches nothing is Auto rather than a
    // guess: guessing wrong loads a decoder against the wrong graph and fails
    // deep inside the library instead of here.
    void theModelDirectoryChoosesTheEngine()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // A Vosk model is a directory holding an "am" subdirectory
        const QString voskDir = qsl("%1/vosk-model-small-en-us-0.15").arg(dir.path());
        QVERIFY(QDir().mkpath(qsl("%1/am").arg(voskDir)));
        QCOMPARE(SpeechRecognizerFactory::backendForModelDir(voskDir), SpeechRecognizerFactory::Backend::Vosk);

        // A sherpa streaming transducer model is three .onnx files plus tokens.txt
        const QString sherpaDir = qsl("%1/sherpa-onnx-streaming-zipformer-en").arg(dir.path());
        QVERIFY(QDir().mkpath(sherpaDir));
        for (const QString& name : {qsl("encoder.onnx"), qsl("decoder.onnx"), qsl("joiner.onnx"), qsl("tokens.txt")}) {
            QFile file(qsl("%1/%2").arg(sherpaDir, name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.close();
        }
        QCOMPARE(SpeechRecognizerFactory::backendForModelDir(sherpaDir), SpeechRecognizerFactory::Backend::Sherpa);

        const QString emptyDir = qsl("%1/nothing-in-here").arg(dir.path());
        QVERIFY(QDir().mkpath(emptyDir));
        QCOMPARE(SpeechRecognizerFactory::backendForModelDir(emptyDir), SpeechRecognizerFactory::Backend::Auto);
    }

    // Round-trips through the settings string, because a backend chosen by the
    // player is written to Mudlet.ini and read back on the next run: an
    // identifier that does not survive the trip silently reverts their choice.
    void aBackendIdentifierSurvivesTheRoundTrip()
    {
        for (const auto backend : {SpeechRecognizerFactory::Backend::Vosk, SpeechRecognizerFactory::Backend::Sherpa}) {
            const QString identifier = SpeechRecognizerFactory::backendIdentifier(backend);
            QVERIFY2(!identifier.isEmpty(), "a backend the player can choose needs a name to store");
            QCOMPARE(SpeechRecognizerFactory::backendFromIdentifier(identifier), backend);
        }
        QCOMPARE(SpeechRecognizerFactory::backendFromIdentifier(qsl("not-an-engine")), SpeechRecognizerFactory::Backend::Auto);
    }

    // sherpa reports biasing only when the loaded model can actually be
    // biased, which needs bpe.vocab - the text "piece score" file, NOT the
    // bpe.model binary that model packages ship. Claiming biasing without it
    // makes setVocabulary() answer Applied over a decoder that scores nothing.
    void sherpaClaimsNoBiasingBeforeAModelIsLoaded()
    {
        SherpaRecognizer recognizer;
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
        QVERIFY2(!recognizer.capabilities().biasing, "biasing is a property of the loaded model, and none is loaded");
        QCOMPARE(recognizer.setVocabulary({qsl("kill"), qsl("look")}), SpeechRecognizer::VocabularyResult::Unsupported);
        QVERIFY2(recognizer.vocabulary().size() == 2, "the words must be kept for a model that can bias later");
    }

    // capabilities().biasing and modelPath() must answer from the live handle,
    // not from mSupportsBiasing/mModelPath alone: both are set by loadModel()
    // before the recognizer handle exists, and mModelPath survives
    // releaseResources() untouched, so without the gate modelPath() would go on
    // naming a model that is no longer loaded. releaseResources() does clear
    // mSupportsBiasing and mBpeVocabPath - SherpaRecognizer.h says it keeps
    // those redundant with the gate on purpose rather than leaning on it
    // alone - so biasing is held to the same answer from two directions. This pins the gate on a recognizer that never
    // successfully loaded a model at all - the strongest case reachable
    // without the sherpa-onnx library actually installed, since only a real,
    // successful load ever sets mSupportsBiasing true or mModelPath non-empty
    // in the first place. The load-then-release case this cannot reach was
    // verified by hand against the real sherpa-onnx library and a model
    // carrying a real bpe.vocab.
    void sherpaCapabilitiesAndModelPathClearOnRelease()
    {
        SherpaRecognizer recognizer;
        QVERIFY(recognizer.modelPath().isEmpty());
        QVERIFY(recognizer.currentLanguage().isEmpty());
        QVERIFY2(!recognizer.capabilities().biasing, "biasing was claimed with no model ever loaded");

        // Fails at the very first guard (no library here) without ever
        // touching mModelPath/mSupportsBiasing - releaseResources() must still
        // leave a consistent, empty answer rather than assume it has anything to undo
        QVERIFY(!recognizer.initialize(qsl("/definitely/not/a/model/path/for/testing")));
        recognizer.releaseResources();

        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
        QVERIFY2(recognizer.modelPath().isEmpty(), "a model path survived releaseResources()");
        QVERIFY2(recognizer.currentLanguage().isEmpty(), "a language survived releaseResources()");
        QVERIFY2(!recognizer.capabilities().biasing, "biasing was still claimed after releaseResources()");
        QVERIFY2(!recognizer.hasLiveNativeResources(), "releaseResources() left native handles behind");
    }

    // Apple's recognizer needs no download and no model, so unlike Vosk and
    // sherpa it is available on any Mac as soon as the user has granted speech
    // permission. What it must never do is claim a capability it cannot honour:
    // recognition is forced on-device, so onDevice is true and audio never
    // reaches Apple; contextualStrings gives real biasing; and the per-segment
    // confidence and timestamps make wordResults honest.
    void theAppleBackendClaimsOnlyWhatTheFrameworkGives()
    {
#if defined(Q_OS_MACOS)
        AppleSpeechRecognizer recognizer;
        const auto can = recognizer.capabilities();
        QVERIFY2(can.onDevice, "recognition is forced on-device, so this must not under-report");
        QVERIFY2(can.biasing, "contextualStrings is what carries a package's vocabulary");
        QVERIFY2(can.wordResults, "SFTranscriptionSegment carries confidence and timings");
        QVERIFY2(!can.grammar, "there is no grammar-constraint API to back this claim");

        // No model on disk to name, ever - a package gating setup on a model
        // path must not be told to go and install one
        QCOMPARE(recognizer.modelPath(), QString());
        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
#else
        QCOMPARE(SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform), false);
#endif
    }

    // gap 1: backendForModelDir() used to be dead code - grep found no caller
    // outside a test - so stt.init(path) picked an engine by install
    // preference (Auto) rather than by what the directory actually is. With
    // nothing installed, create(Auto) and create(<a specific backend>) are
    // both null, so nullptr alone cannot tell "Auto picked nothing" from "the
    // right engine was asked for and wasn't there". create() names an
    // unavailable *specific* backend in a warning but says nothing for Auto's
    // own empty-list case, and that asymmetry is what makes the warning the
    // one externally observable proof that backendForModelDir()'s answer is
    // what actually reached create(), rather than Auto's install-preference
    // order (which would try sherpa first on a mixed-install machine - the
    // exact bug this fix closes).
    void modelDirectorySelectsTheEngineCreateAttempts()
    {
        if (VoskRecognizer::libraryAvailable()) {
            QSKIP("libvosk is installed here, so create(Vosk) would succeed rather than warn");
        }

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString voskDir = qsl("%1/vosk-model-small-en-us-0.15").arg(dir.path());
        QVERIFY(QDir().mkpath(qsl("%1/am").arg(voskDir)));

        const auto backend = SpeechRecognizerFactory::backendForModelDir(voskDir);
        QCOMPARE(backend, SpeechRecognizerFactory::Backend::Vosk);

        QTest::ignoreMessage(QtWarningMsg, "SpeechRecognizerFactory: Vosk backend requested but not available");
        QVERIFY2(!SpeechRecognizerFactory::create(backend, nullptr), "a backend requested by name must still refuse when its library is not installed");
    }

    // gap 1's other half: stt.init()'s new caller, mudlet::initSpeechRecognition(),
    // must actually forward the backend it is given rather than falling back
    // to its old hardcoded Auto. Same warning-message proof as above, this
    // time through the real call site.
    void initSpeechRecognitionForwardsAnExplicitBackend()
    {
        if (VoskRecognizer::libraryAvailable()) {
            QSKIP("libvosk is installed here, so create(Vosk) would succeed rather than warn");
        }
        QVERIFY2(!mudlet::self()->speechRecognizer(), "an earlier case in this file left a live recognizer behind");

        QTest::ignoreMessage(QtWarningMsg, "SpeechRecognizerFactory: Vosk backend requested but not available");
        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Vosk);
        QVERIFY2(!mudlet::self()->speechRecognizer(), "an unavailable backend must not have been silently swapped for Auto's own choice");
    }

    // gap 2: stt.init() with no argument must be able to reach the built-in
    // macOS backend, which needs neither a library nor a model on disk.
    // *When* to prefer it over a model-based engine is a decision inside
    // TLuaInterpreter::sttInit() with no entry point below Lua, so it is
    // proven where CLAUDE.md says a Lua-reachable behaviour belongs: in
    // STT_spec.lua's "names where a model belongs when none is installed, or
    // succeeds via a model-less backend" case, not duplicated here. What this
    // pins is the primitive that decision depends on - that the backend it
    // selects on macOS actually accepts stt.init() with no model path.
    void theModelLessBackendInitializesWithNoModelPath()
    {
#if defined(Q_OS_MACOS)
        if (!SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform)) {
            QSKIP("no system speech recognizer available for this locale on this machine");
        }

        std::unique_ptr<SpeechRecognizer> recognizer(SpeechRecognizerFactory::create(SpeechRecognizerFactory::Backend::Platform, nullptr));
        QVERIFY2(recognizer.get(), "the built-in macOS backend must be creatable directly, even though availableBackends() omits it");
        QVERIFY2(recognizer->initialize(QString()), "the model-less backend must accept stt.init() with no path at all");
        QCOMPARE(recognizer->state(), SpeechRecognizer::State::Ready);
#else
        // Shape-check on every other platform: there is no model-less backend
        // to reach yet, so there is nothing more this case can prove here.
        QCOMPARE(SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform), false);
#endif
    }

    // gap 3: stt.available() and getInfo().available used to answer for Vosk
    // alone, so a sherpa-only or Apple-only install was told nothing was
    // there even though speech recognition genuinely works. Called directly
    // against a bare lua_State - none of these reads touch a Host or a
    // profile, so building one would add weight without adding coverage.
    void availabilityIsNotVoskOnly()
    {
        lua_State* L = luaL_newstate();
        QVERIFY(L);
        auto closeState = qScopeGuard([L]() {
            lua_close(L);
        });

        // Read once, then asserted against a stated expectation rather than
        // against production's own expression: computing the expected value
        // the same way stt.available() computes it made this mirror the
        // binding instead of checking it, and a revert to "Vosk alone" stayed
        // green because both sides moved together.
        TLuaInterpreter::sttIsAvailable(L);
        const bool available = lua_toboolean(L, -1);
        lua_pop(L, 1);

        TLuaInterpreter::sttGetInfo(L);
        QVERIFY(lua_istable(L, -1));
        lua_getfield(L, -1, "available");
        QCOMPARE(static_cast<bool>(lua_toboolean(L, -1)), available);
        lua_pop(L, 1);

        lua_getfield(L, -1, "searchPaths");
        QVERIFY2(lua_istable(L, -1), "searchPaths must always be a table, engine installed or not");
        lua_pop(L, 2); // searchPaths, then the getInfo() table

        const bool voskInstalled = SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Vosk);
        const bool sherpaInstalled = SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Sherpa);
        const bool platformInstalled = SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform);

        if (!sherpaInstalled && !platformInstalled) {
            // Nothing here can tell a Vosk-only answer from a correct one:
            // with Vosk the only candidate, both say the same thing. Stated
            // rather than quietly passing, since a green that proves nothing
            // is what this case was rewritten to stop.
            QCOMPARE(available, voskInstalled);
            QSKIP("only Vosk could answer on this machine, so this cannot tell a Vosk-only availability answer from a correct one");
        }
        if (voskInstalled) {
            QCOMPARE(available, true);
            QSKIP("libvosk is installed here as well, so a Vosk-only answer would also be true");
        }

        // A non-Vosk engine is installed and Vosk is not: speech recognition
        // genuinely works on this machine, and the only way to answer false is
        // to have asked Vosk alone.
        QVERIFY2(available, "a sherpa-onnx or built-in backend is installed here, so availability must not be answered from Vosk alone");
    }

    // gap 3: with no model-based engine installed at all, stt.getModelPath()
    // and stt.getLibraryPath() must still name a real, checkable place - the
    // same default they always answered before sherpa existed. The branch
    // that answers with sherpa's own paths once a SherpaRecognizer is
    // actually loaded needs the real library to exercise and cannot be proven
    // here - said plainly rather than writing a case that would pass either way.
    void installPathsDefaultToVoskWithNothingLoaded()
    {
        if (VoskRecognizer::libraryAvailable() || SherpaRecognizer::sherpaAvailable()) {
            QSKIP("a model-based engine is installed here, so the default this case pins does not apply");
        }
        QVERIFY2(!mudlet::self()->speechRecognizer(), "a live recognizer would make these answer for it instead of the default");

        lua_State* L = luaL_newstate();
        QVERIFY(L);
        auto closeState = qScopeGuard([L]() {
            lua_close(L);
        });

        TLuaInterpreter::sttGetModelPath(L);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), VoskRecognizer::modelsDirectoryPath());
        lua_pop(L, 1);

        TLuaInterpreter::sttGetLibraryPath(L);
        QCOMPARE(QString::fromUtf8(lua_tostring(L, -1)), VoskRecognizer::userLibraryPath());
        lua_pop(L, 1);
    }

    // gap 3: stt.listModels() must show what is on disk for every model-based
    // engine, not only Vosk's directory - a downloaded sherpa model has to
    // stay visible even with nothing loaded. Unlike the two cases above, this
    // needs no library at all: getInstalledModels() only reads directory
    // layouts, so it is provable in full without installing anything.
    void listModelsSpansEveryModelBasedEngine()
    {
        const QString voskModelsDir = VoskRecognizer::modelsDirectoryPath();
        const QString sherpaModelsDir = SherpaRecognizer::modelsDirectoryPath();
        const QString voskModel = qsl("contract-test-vosk-model");
        const QString sherpaModel = qsl("contract-test-sherpa-model");

        QVERIFY(QDir().mkpath(qsl("%1/%2/am").arg(voskModelsDir, voskModel)));
        QVERIFY(QDir().mkpath(qsl("%1/%2").arg(sherpaModelsDir, sherpaModel)));
        for (const QString& name : {qsl("encoder.onnx"), qsl("decoder.onnx"), qsl("joiner.onnx"), qsl("tokens.txt")}) {
            QFile file(qsl("%1/%2/%3").arg(sherpaModelsDir, sherpaModel, name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.close();
        }
        auto cleanup = qScopeGuard([&]() {
            QDir(qsl("%1/%2").arg(voskModelsDir, voskModel)).removeRecursively();
            QDir(qsl("%1/%2").arg(sherpaModelsDir, sherpaModel)).removeRecursively();
        });

        lua_State* L = luaL_newstate();
        QVERIFY(L);
        auto closeState = qScopeGuard([L]() {
            lua_close(L);
        });

        TLuaInterpreter::sttListModels(L);
        QVERIFY(lua_istable(L, -1));
        const int tableIdx = lua_gettop(L);

        QStringList names;
        for (int i = 1;; ++i) {
            lua_rawgeti(L, tableIdx, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                break;
            }
            lua_getfield(L, -1, "name");
            names << QString::fromUtf8(lua_tostring(L, -1));
            lua_pop(L, 2); // name, then the {name,path} table - leaves the array slot behind
        }

        QVERIFY2(names.contains(voskModel), "a Vosk model on disk was not listed");
        QVERIFY2(names.contains(sherpaModel), "a sherpa model on disk was not listed - listModels must not be Vosk-only");
    }

    // Last in the file on purpose: alone among the cases here, these two
    // leave a recognizer built on mudlet::self() for the rest of the
    // process - keeping one is the behaviour they exist to prove - and
    // several cases above open by asserting the bridge is empty.
    // Live testing found that mudlet::initSpeechRecognition() returned
    // immediately whenever mpSpeechRecognizer was already built, so
    // stt.close() followed by stt.init() naming a different engine kept
    // feeding models to the first engine the session ever built. Fixing that
    // brought the opposite risk with it, so the two rules that must NOT
    // rebuild are pinned here.
    //
    // Runs on macOS only, and only where the system speech recognizer is
    // available for the machine's locale: the built-in macOS backend is the
    // one backend this file can build with no library and no model installed,
    // and without it there is nothing to hold on to. Everywhere else this
    // asserts that no such backend exists and skips.
    void initSpeechRecognitionKeepsTheBackendOnAutoAndOnTheSameBackend()
    {
#if defined(Q_OS_MACOS)
        if (!SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform)) {
            QSKIP("no system speech recognizer available for this locale on this machine");
        }
        QVERIFY2(!mudlet::self()->speechRecognizer(), "an earlier case in this file left a live recognizer behind");

        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Platform);
        const QPointer<SpeechRecognizer> firstRecognizer = mudlet::self()->speechRecognizer();
        QVERIFY2(firstRecognizer, "the built-in macOS backend must have been built");

        // Auto, and the backend already in place, must both leave it alone -
        // several call sites pass one of these on every setter call, and
        // rebuilding on either would tear down a working recognizer under a
        // caller who never asked to switch engines.
        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Auto);
        QCOMPARE(mudlet::self()->speechRecognizer(), firstRecognizer.data());
        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Platform);
        QCOMPARE(mudlet::self()->speechRecognizer(), firstRecognizer.data());
#else
        QCOMPARE(SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform), false);
        QSKIP("no backend on this platform can be built without an engine library, so this cannot be exercised here");
#endif
    }

    // sttInit() derives the backend from the model directory's own layout, so
    // naming a sherpa-onnx model on a Vosk-only machine resolves to a backend
    // that cannot be built. Tearing the old engine down before finding that
    // out cost the player a loaded, working model for a mistyped path - so the
    // replacement is built first, and a failure leaves what is there untouched.
    //
    // Runs on macOS only, needs the system speech recognizer available for the
    // machine's locale, and additionally skips wherever libvosk is installed,
    // since then the request below would succeed and there would be no failed
    // replacement to observe. On a developer machine with Vosk set up it never
    // executes; CI, which installs neither, is where it does its work.
    void initSpeechRecognitionKeepsAWorkingBackendWhenTheReplacementCannotBeBuilt()
    {
#if defined(Q_OS_MACOS)
        if (!SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform)) {
            QSKIP("no system speech recognizer available for this locale on this machine");
        }
        if (VoskRecognizer::libraryAvailable()) {
            QSKIP("libvosk is installed here, so requesting it below would succeed instead of failing the way this case needs");
        }

        // Reaches the case above's recognizer when that one ran, and builds one
        // otherwise, so this does not depend on the order the slots run in
        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Platform);
        const QPointer<SpeechRecognizer> workingRecognizer = mudlet::self()->speechRecognizer();
        QVERIFY2(workingRecognizer, "the built-in macOS backend must have been built");

        QTest::ignoreMessage(QtWarningMsg, "SpeechRecognizerFactory: Vosk backend requested but not available");
        mudlet::self()->initSpeechRecognition(SpeechRecognizerFactory::Backend::Vosk);

        QVERIFY2(mudlet::self()->speechRecognizer(), "a replacement that could not be built left the bridge with no engine at all");
        QCOMPARE(mudlet::self()->speechRecognizer(), workingRecognizer.data());

        // Not merely still pointed at: an engine retired the way the swap path
        // retires one is deleteLater()d, so the destruction only lands on an
        // event loop turn. Take one, then check the object is still there.
        QTest::qWait(1);
        QVERIFY2(!workingRecognizer.isNull(), "the working backend was torn down for a replacement that never arrived");
        QCOMPARE(mudlet::self()->speechRecognizer(), workingRecognizer.data());
#else
        QCOMPARE(SpeechRecognizerFactory::backendAvailable(SpeechRecognizerFactory::Backend::Platform), false);
        QSKIP("no backend on this platform can be built without an engine library, so this cannot be exercised here");
#endif
    }
};

#include "SpeechRecognizerContractTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpeechRecognizerContractTest)
