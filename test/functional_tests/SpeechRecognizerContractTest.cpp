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
 * is reachable from any test here. That is why modelPath() and currentLanguage()
 * read the live model handle rather than a remembered string - the answer is
 * then true from paths a test cannot reach, instead of resting on where an
 * assignment happens to sit.
 *
 * Run with: ctest -R SpeechRecognizerContractTest -V
 */

#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "SpeechRecognizer.h"
#include "VoskRecognizer.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <optional>

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
};

#include "SpeechRecognizerContractTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpeechRecognizerContractTest)
