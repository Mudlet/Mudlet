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
 * The rules that only a real sherpa-onnx engine can answer for: what a model
 * load leaves behind, when the capabilities it decides are announced, and what
 * becomes of a vocabulary the model cannot use. Each of those was reported
 * against a working install and none of them is reachable from the stand-in
 * engines the other speech tests drive - a stand-in agrees with whatever it
 * was written to agree with.
 *
 * Every case here needs a model, and no CI runner has one, so the whole class
 * skips without one. Point it at an installed model to run it:
 *
 *   MUDLET_TEST_SHERPA_MODEL=~/.config/mudlet/sherpa-models/sherpa-onnx-streaming-zipformer-en-2023-06-26 \
 *     ctest -R SherpaEngineTest -V
 *
 * A model carrying bpe.vocab is needed for the vocabulary case, which says so
 * and skips rather than passing quietly when given one without it.
 *
 * Nothing here opens a microphone: these are all properties of a loaded model.
 */

#include <QDir>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include "SherpaRecognizer.h"
#include "SpeechRecognizer.h"

#include "GroupedTest.h"

class SherpaEngineTest : public QObject
{
    Q_OBJECT

private:
    QString mModelPath;

    // The model named in the environment, or nothing when there is none to
    // run against
    static QString modelFromEnvironment()
    {
        QString path = qEnvironmentVariable("MUDLET_TEST_SHERPA_MODEL");
        if (path.startsWith(qsl("~/"))) {
            path = QDir::homePath() + path.mid(1);
        }
        return QDir(path).exists() ? path : QString();
    }

private slots:
    void initTestCase()
    {
        if (!SherpaRecognizer::sherpaAvailable()) {
            QSKIP("no sherpa-onnx library is installed here");
        }
        mModelPath = modelFromEnvironment();
        if (mModelPath.isEmpty()) {
            QSKIP("set MUDLET_TEST_SHERPA_MODEL to an installed sherpa model directory to run these");
        }
    }

    // What a model can do is decided by the model, so the announcement has to
    // arrive with the engine in the state that announcement describes. It used
    // to come while the load was still in progress, and a handler doing what
    // docs/stt-api.md asks for - re-read the capabilities, offer the words this
    // model can now bias toward - had its offer refused for an engine that was
    // not idle, though the model had in fact finished loading.
    void theCapabilitiesAreAnnouncedWithTheEngineReady()
    {
        SherpaRecognizer recognizer;

        SpeechRecognizer::State stateWhenAnnounced = SpeechRecognizer::State::Uninitialized;
        int announcements = 0;
        connect(&recognizer, &SpeechRecognizer::capabilitiesChanged, &recognizer, [&](SpeechRecognizer::Capabilities) {
            ++announcements;
            stateWhenAnnounced = recognizer.state();
        });

        QVERIFY2(recognizer.initialize(mModelPath), "the model did not load");

        if (announcements == 0) {
            QSKIP("this model changed no capability as it loaded, so nothing was announced to time");
        }
        QCOMPARE(stateWhenAnnounced, SpeechRecognizer::State::Ready);
    }

    // A model that has been released is not a model that is loaded, and the
    // path it was read from is remembered rather than cleared - so what makes
    // these answer for the engine's actual state is that they are read through
    // the live handle. A package probing modelPath() to decide whether setup
    // has already happened skips the load it needs otherwise.
    void aReleasedModelIsNoLongerNamed()
    {
        SherpaRecognizer recognizer;
        QVERIFY2(recognizer.initialize(mModelPath), "the model did not load");

        QVERIFY2(!recognizer.modelPath().isEmpty(), "a loaded model must be named");
        QVERIFY2(!recognizer.currentLanguage().isEmpty(), "a loaded model must report the language it was read as");
        QVERIFY(recognizer.hasLiveNativeResources());

        recognizer.releaseResources();

        QCOMPARE(recognizer.state(), SpeechRecognizer::State::Uninitialized);
        QVERIFY2(!recognizer.hasLiveNativeResources(), "releaseResources() left native handles behind");
        QVERIFY2(recognizer.modelPath().isEmpty(), "a model path survived the model it describes");
        QVERIFY2(recognizer.currentLanguage().isEmpty(), "a language survived the model it was read from");
        QVERIFY2(!recognizer.capabilities().biasing, "a released engine still claimed it could bias");
    }

    // Words this engine cannot use bias nothing, so the offer has not taken
    // effect and a later identical offer must be tried again rather than agreed
    // with. Recording it as applied - which any reload used to do - tells a
    // package its words are in force and stops it correcting results that
    // nothing is biasing.
    void aVocabularyTheModelRejectedIsNotRecordedAsApplied()
    {
        SherpaRecognizer recognizer;
        QVERIFY2(recognizer.initialize(mModelPath), "the model did not load");
        if (!recognizer.capabilities().biasing) {
            QSKIP("this model carries no bpe.vocab, so it cannot bias and vocabulary is refused outright");
        }

        // Every entry unusable: a leading ':' or '#' is what the hotwords file
        // format reserves, and an entry of nothing but those tokens leaves the
        // filter with nothing to pass on
        const QStringList rejected{qsl(":kill"), qsl("#look")};
        QCOMPARE(recognizer.setVocabulary(rejected), SpeechRecognizer::VocabularyResult::Failed);

        // The reload a package makes after installing a model, or that
        // setSensitivity() makes for it
        QVERIFY2(recognizer.initialize(mModelPath), "the model did not load a second time");

        QCOMPARE(recognizer.setVocabulary(rejected), SpeechRecognizer::VocabularyResult::Failed);
    }

    // The other half of that rule: words the engine can use are applied, and
    // stay applied across the reload the engine makes to bake them in - so a
    // package offering the same list again is not made to pay for a second
    // rebuild of a model that already holds them.
    void aVocabularyTheModelAcceptedStaysApplied()
    {
        SherpaRecognizer recognizer;
        QVERIFY2(recognizer.initialize(mModelPath), "the model did not load");
        if (!recognizer.capabilities().biasing) {
            QSKIP("this model carries no bpe.vocab, so it cannot bias and vocabulary is refused outright");
        }

        const QStringList words{qsl("kill"), qsl("look"), qsl("inventory")};
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Applied);
        QCOMPARE(recognizer.setVocabulary(words), SpeechRecognizer::VocabularyResult::Applied);
    }
};

#include "SherpaEngineTest.moc"
MUDLET_GROUPED_TEST_MAIN(SherpaEngineTest)
