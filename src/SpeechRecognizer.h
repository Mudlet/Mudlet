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

#ifndef MUDLET_SPEECHRECOGNIZER_H
#define MUDLET_SPEECHRECOGNIZER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

// Abstract base class for speech recognition backends.
// This abstraction allows swapping between different speech recognition
// engines (Vosk, Whisper, platform APIs) without changing UI or Lua code.

class SpeechRecognizer : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Uninitialized, // No model loaded
        Ready,         // Model loaded, not listening
        // Asked to listen, not yet listening: something outside this process
        // has to answer first, and the answer is not synchronous. Permission
        // to use a microphone is the usual reason - macOS asks the player the
        // first time, and a browser asks on every session - which is why this
        // is a state of the contract rather than a quirk of one platform. A
        // consumer shows "waiting", and a second request while here is
        // refused rather than asked twice.
        Starting,
        Listening,  // Actively capturing and processing audio
        Processing, // Processing final audio after stop
        Error       // An error occurred
    };
    Q_ENUM(State)

    explicit SpeechRecognizer(QObject* parent = nullptr)
    : QObject(parent)
    {
    }
    ~SpeechRecognizer() override = default;

    SpeechRecognizer(const SpeechRecognizer&) = delete;
    SpeechRecognizer& operator=(const SpeechRecognizer&) = delete;

    // === Core Operations ===

    // Initialize the recognizer with a model. Returns true on success.
    // @param modelPath: Path to the speech recognition model directory
    virtual bool initialize(const QString& modelPath) = 0;

    // Start listening for speech input from the microphone.
    // Emits partialResult() as speech is recognized in real-time.
    // Emits finalResult() when an utterance is complete.
    // Not virtual: which states may start, and what a refusal says, are the
    // same for every engine, and each backend had grown its own copy - one of
    // which had drifted into moving to Error on a refusal. A backend
    // implements doStartListening() and inherits the rules.
    void startListening()
    {
        if (state() != State::Ready) {
            // Every refusal but "already listening" and "already starting"
            // reports why: this returns void, so silence reads to the caller
            // as a successful start. The state is left alone - a refusal is
            // not a fault, and docs/stt-api.md says a refusal may arrive
            // without a state change.
            //
            // The first two are unreachable from Lua as things stand:
            // sttStart() and sttToggle() both refuse on !initialized() first,
            // which is exactly Uninitialized-or-Error, and answer in their own
            // words. Kept for a direct C++ caller, and because a bridge that
            // stopped pre-gating would need them - but nothing translated here
            // reaches a player through those two paths today.
            if (state() == State::Uninitialized) {
                //: Shown when speech recognition is asked to listen before a language model is loaded
                emit errorOccurred(tr("Recognizer not initialized. Call initialize() first."));
            } else if (state() == State::Error) {
                //: Shown when speech recognition is asked to listen while it is in an error state
                emit errorOccurred(tr("Speech recognition is in an error state - reload the model before listening again."));
            } else if (state() == State::Processing) {
                //: Shown when speech recognition is asked to listen while still transcribing the previous phrase
                emit errorOccurred(tr("Speech recognition is still processing the previous phrase."));
            }
            return;
        }
        doStartListening();
        // Deliberately not asserted here, though the rule below is real. A
        // backend reaches Listening through setState(), which raises
        // sysSTTStateChanged into Lua synchronously, and a handler that stops
        // on that event - "stop after the first phrase", the ordinary
        // push-to-talk shape - runs doStopListening() and lands back on Ready
        // before this frame resumes. Ready on return is therefore a legitimate
        // outcome as well as the symptom of a backend that did nothing, and
        // nothing here can separate the two.
    }

    // Finalises the pending utterance, emitting finalResult() with whatever
    // is pending. Only Listening has one to finalise.
    void stopListening()
    {
        // Starting is a stop too, for cancel()'s reason below: there is no
        // audio to finalise yet, but leaving the request pending means
        // answering the permission dialog later opens the microphone after
        // the caller asked to stop. Handled here rather than in each caller -
        // sttToggle() read Starting as "not listening", took the start branch,
        // and told the player it was now listening while the microphone opened
        // behind them.
        if (state() == State::Starting) {
            cancel();
            return;
        }
        if (state() != State::Listening) {
            return;
        }
        doStopListening();
    }

    // Abandons what is in flight without finalising it; no finalResult() is
    // emitted. Starting counts: a request still waiting on a permission
    // dialog has no audio to abandon, but leaving it pending means answering
    // the dialog later opens the microphone after the caller asked to stop.
    void cancel()
    {
        if (state() == State::Starting) {
            setState(State::Ready);
            return;
        }
        if (state() != State::Listening && state() != State::Processing) {
            return;
        }
        doCancel();
    }

    // Stop listening automatically after this many milliseconds of continuous
    // silence, finalising the utterance as stopListening() would. 0 (the
    // default) keeps listening open-ended. Backends without the capability
    // ignore it and keep reporting 0.
    virtual void setSilenceTimeout(int msec) { Q_UNUSED(msec) }
    virtual int silenceTimeout() const { return 0; }

    // === Capabilities ===
    // What this backend can actually do, so consumers adapt instead of
    // guessing. Answered as a group rather than one virtual at a time because
    // they change together and, on some backends, change at all: whether
    // recognition can be biased is a property of the loaded model rather than
    // of the engine, so a backend re-reads them when a model loads and says so
    // through capabilitiesChanged().
    struct Capabilities
    {
        // Recognition can be biased toward a supplied vocabulary
        bool biasing = false;
        // Recognition can be constrained to a supplied grammar
        bool grammar = false;
        // Results carry per-word confidence and timing (wordsResult signal)
        bool wordResults = false;
        // End-of-speech detection can be tuned by setSensitivity(). Named apart
        // from getInfo()'s top-level "sensitivity", which is the mode string
        // currently in force - one says which setting, the other whether the
        // setting can be changed at all. False is
        // not "this attempt failed" but "this engine never can" - the built-in
        // macOS backend decides its own endpointing, and a libvosk without the
        // endpointer symbol cannot be reached. Without this, a caller could
        // not tell those apart from an engine that can tune and failed once,
        // and would stop asking about something merely transient.
        bool sensitivityTuning = false;
        // Audio is processed locally. False is the honest default: a backend
        // that has not said otherwise has not promised recordings stay on this
        // machine, and inheriting that promise by omission is the one way this
        // particular flag must never be wrong.
        bool onDevice = false;

        bool operator==(const Capabilities&) const = default;
    };

    virtual Capabilities capabilities() const { return {}; }

    // Named readers over the same answers, for a caller that wants one of
    // them. Not virtual: a backend overrides capabilities() alone, so the
    // group and the individual answers cannot come to disagree.
    bool supportsBiasing() const { return capabilities().biasing; }
    bool supportsGrammar() const { return capabilities().grammar; }
    bool supportsWordResults() const { return capabilities().wordResults; }
    bool supportsSensitivityTuning() const { return capabilities().sensitivityTuning; }
    bool onDevice() const { return capabilities().onDevice; }

    // What became of a vocabulary offered to the engine. Three outcomes, not
    // two: a backend that cannot use vocabulary at all is a different matter
    // from one that can and failed this time, and collapsing them tells a
    // consumer to fall back to correcting results itself when what actually
    // happened was a fault worth reporting.
    enum class VocabularyResult {
        Applied,     // In effect; the engine is biasing toward these words
        Unsupported, // This backend cannot use vocabulary - correct client-side
        Failed       // It can, and this attempt did not work
    };
    // A backend answering Failed reports why through errorOccurred() itself,
    // on every path that can reach it. The caller sees one enum value for
    // several different problems and cannot describe any of them, so anything
    // it emitted instead would be vague enough to be worth less than nothing
    // beside the specific message a backend that already spoke had emitted.

    Q_ENUM(VocabularyResult)

    // Supply vocabulary for biasing or grammar constraint.
    //
    // Not virtual: the capability decides whether this backend can take words
    // at all, so the answer and the capability cannot disagree - a backend
    // advertising biasing and answering Unsupported for ever, or refusing
    // words it claims to accept, are both unrepresentable now. A backend that
    // can take them overrides applyVocabulary() and never sees Unsupported.
    VocabularyResult setVocabulary(const QStringList& words)
    {
        // Held whatever the answer below is. Which models can be biased is
        // only known once one is loaded, so words offered while an unbiasable
        // model is loaded would otherwise be lost, and a later switch to a
        // model that can bias would compile in nothing.
        const bool changed = (mVocabulary != words);
        mVocabulary = words;

        const Capabilities can = capabilities();
        if (!can.biasing && !can.grammar) {
            mVocabularyApplied = false;
            return VocabularyResult::Unsupported;
        }
        if (!changed && mVocabularyApplied) {
            // Already in effect and the last attempt succeeded; nothing to rebuild
            return VocabularyResult::Applied;
        }
        const VocabularyResult result = applyVocabulary(words);
        // applyVocabulary() can reach Lua - sherpa reloads the model to bias
        // it, and setState() dispatches handlers inline - so a handler calling
        // stt.setVocabulary() re-enters and completes a whole offer of its own
        // before this line runs. Committing the outer verdict then would leave
        // the flag describing one word list and mVocabulary another, and the
        // next identical offer short-circuits to Applied against a model that
        // never received it: the exact desync the flag exists to prevent.
        if (mVocabulary != words) {
            return result;
        }
        // applyVocabulary()'s documented contract, checked rather than only
        // written down: the capability test above has already ruled the third
        // case out, so a backend answering Unsupported here would be claiming
        // support and refusing the words in the same breath.
        Q_ASSERT(result != VocabularyResult::Unsupported);
        mVocabularyApplied = (result == VocabularyResult::Applied);
        return result;
    }

    // What was last offered, applied or not. A backend reads this when a model
    // loads, to bias toward words that arrived while it could not.
    const QStringList& vocabulary() const { return mVocabulary; }

    // Level of the audio last received from the microphone, 0.0 to 1.0, or 0
    // while not listening. Reported so a consumer can tell a phrase the engine
    // misheard from one the microphone barely received - two failures that
    // look identical in the results and need opposite remedies.
    virtual float audioLevel() const { return 0.0f; }

    // === State Queries ===

    // The recognizer's state, and the only truth about it: there is no
    // parallel "active" flag anywhere. Held here rather than by each backend
    // so the rule below - a change is announced exactly once, and only when it
    // is a change - holds for every implementation instead of being
    // re-established correctly by each one.
    State state() const { return mState; }

    // Convenience methods
    bool listening() const { return state() == State::Listening; }
    bool ready() const { return state() == State::Ready; }
    bool initialized() const { return state() != State::Uninitialized && state() != State::Error; }
    // Waiting on the permission that decides a start, and nothing else - a
    // recognizer that is already listening answers false. Callers wanting "a
    // start is outstanding or has landed" have to ask for both, the way
    // speechStartAccepted() does.
    bool starting() const { return state() == State::Starting; }

    // Whether the backend currently holds live native resources (e.g. handles
    // into a dynamically-loaded library) that must be released before that
    // library can be safely unloaded. Defaults to false for backends with no
    // such resources.
    virtual bool hasLiveNativeResources() const { return false; }

    // Release any native resources held by the backend and return to an
    // uninitialized state. Default is a no-op for backends holding nothing.
    virtual void releaseResources() {}

    // Path of the model the backend is currently working from. Empty when the
    // backend has never been given one, or has no concept of a model on disk.
    virtual QString modelPath() const { return QString(); }

    // === Language/Model Support ===

    virtual QString currentLanguage() const = 0;

    // Set the recognition language. Returns true on success.
    // May require model reload depending on backend.
    virtual bool setLanguage(const QString& languageCode) = 0;

    // === Backend Information ===

    // Get human-readable name of the backend (e.g., "Vosk", "Whisper")
    virtual QString backendName() const = 0;

    virtual QString backendVersion() const = 0;

    // === Recognition Settings ===

    // Sensitivity mode controls how quickly the recognizer detects end of speech.
    // Short: Fast detection, good for commands (may cut off longer phrases)
    // Default: Balanced for typical use
    // Long: Slower detection, good for dictation (waits longer before ending)
    enum class Sensitivity {
        Short,   // Fast end-of-speech detection (commands)
        Default, // Balanced (typical use)
        Long     // Slow end-of-speech detection (dictation)
    };
    Q_ENUM(Sensitivity)

    // Set how quickly end-of-speech is detected
    // False when this engine cannot tune end-of-speech detection at all, so a
    // caller is told rather than being handed a readback that agrees with the
    // request and disagrees with the engine
    virtual bool setSensitivity(Sensitivity sensitivity) = 0;
    virtual Sensitivity sensitivity() const = 0;

protected:
    // Take the words, for a backend whose capabilities say it can. Answers
    // Applied or Failed only - setVocabulary() has already ruled out the third
    // case, so a backend never has to reason about it.
    //
    // mVocabularyApplied is kept in sync by setVocabulary() alone; this method
    // never touches it. A backend that reapplies the retained vocabulary()
    // itself - typically when a model loads, per vocabulary()'s comment - has
    // two ways to record that, and which one is right depends on whether the
    // reapply actually did anything: a backend that must rebuild to bias a
    // newly loaded model calls the inherited setVocabulary(vocabulary()), so
    // the flag is set as a side effect of that real work; a backend that
    // already baked vocabulary() in as part of loading - nothing left to do -
    // calls noteVocabularyApplied() instead, since routing that through
    // setVocabulary() would pay for a second, redundant reload just to flip a
    // bookkeeping flag. A backend whose model swap instead invalidates a bias
    // already in effect, without immediately reapplying it, must call
    // clearAppliedVocabulary() itself - otherwise the flag still says the old
    // bias holds, and the next identical offer is wrongly short-circuited into
    // Applied against a model that was never given the words.
    //
    // Worth knowing which of these is actually exercised. Only sherpa reaches
    // either hook, and it takes noteVocabularyApplied(), with its reasons at
    // the call site. Vosk supports no vocabulary at all, and the built-in
    // macOS backend rebuilds the bias into each request, so it answers Applied
    // without either hook - a third route none of this describes. The
    // setVocabulary(vocabulary()) one has no user at all, so it is reasoning
    // rather than something the tests hold to.
    virtual VocabularyResult applyVocabulary(const QStringList& words)
    {
        Q_UNUSED(words)
        // Reached only by a backend that claims biasing or grammar in
        // capabilities() and then does not implement this - and the contract
        // above says every Failed explains itself. Without this the caller
        // gets false and total silence, which stt.setVocabulary documents to
        // the player as "this backend cannot use vocabulary": a wiring mistake
        // wearing the words of a deliberate capability answer.
        //: Shown when a speech engine claims it can use a vocabulary but has no way to apply one, which is a fault in the engine rather than anything the player did
        emit errorOccurred(tr("This speech engine claims it can use a vocabulary but does not implement one."));
        return VocabularyResult::Failed;
    }

    // Corrects mVocabularyApplied when a backend changes what is applied
    // outside setVocabulary() - see applyVocabulary()'s comment. Kept minimal
    // on purpose: it only clears the flag. Reapplication needs this call
    // before calling setVocabulary(vocabulary()), not that call alone: the
    // words have not changed, so setVocabulary() alone would short-circuit on
    // whatever mVocabularyApplied last said - true or not - and the model just
    // loaded would never be asked at all.
    void clearAppliedVocabulary() { mVocabularyApplied = false; }

    // The other half of the pair above: for a backend that applied
    // vocabulary() as an unavoidable side effect of work it was doing anyway
    // (typically loading a model), and has nothing left to do beyond recording
    // that it happened. See applyVocabulary()'s comment for when this is the
    // right call instead of setVocabulary(vocabulary()).
    void noteVocabularyApplied() { mVocabularyApplied = true; }

    // The engine's half of the three above, reached only once the state rules
    // above have allowed entry: a backend never re-checks the state to decide
    // whether one of these may begin.
    //
    // That covers entry only. A doStartListening() that must wait on something
    // outside this process - permission, a device becoming ready - returns
    // while still Starting and finishes the work later, in its own callback or
    // slot. By the time that continuation runs, cancel() may already have
    // moved the state back to Ready without telling it: cancel() called during
    // Starting is handled entirely by the base above and never reaches
    // doCancel(). A continuation therefore has no other way to learn the
    // request was withdrawn, and must check state() == Starting itself before
    // opening a microphone or otherwise acting - releasing there anything it
    // had already acquired while Starting, since no doCancel() call is coming
    // to release it.
    //
    // doStartListening() must also leave the recognizer in a terminal state,
    // itself or through its continuation, before the request is done: Listening
    // once audio is flowing, Starting while a continuation is still pending, or
    // Error on failure. Error is the only one the Lua layer reads as a refused
    // start - see speechStartAccepted() - so a backend that fails must reach
    // it rather than simply returning. Ready on return is not a failure: a
    // handler stopping on the sysSTTStateChanged this raises lands back there
    // inside the same frame, which is the ordinary push-to-talk shape, and is
    // why the check above this one is deliberately not asserted.
    virtual void doStartListening() = 0;
    virtual void doStopListening() = 0;
    virtual void doCancel() = 0;

    // Move to a new state, announcing it only when it differs from the
    // current one. Backends transition through this rather than assigning, so
    // no consumer sees a stateChanged that changed nothing, or misses one
    // that did.
    void setState(State newState)
    {
        if (mState == newState) {
            return;
        }
        mState = newState;
        emit stateChanged(newState);
    }

signals:
    // Emitted during recognition with partial (non-final) text.
    // This text may change as more audio is processed.
    void partialResult(const QString& text);

    // Emitted when an utterance is complete with the final transcription.
    void finalResult(const QString& text);

    // Emitted alongside each final result on backends whose
    // supportsWordResults() is true. Each word is a QVariantMap with keys
    // "word", "start", "end", "conf".
    void wordsResult(const QVariantList& words);

    void stateChanged(SpeechRecognizer::State newState);

    // Emitted when what the backend can do changes, which is when a model is
    // loaded, replaced, or released - releasing counts, because a capability
    // hung off the loaded model goes away with it, and a backend whose
    // capabilities hang off the library alone will simply find nothing changed
    // and stay quiet. Without it a consumer that read the capabilities
    // once - the obvious thing to do - would go on believing them after they
    // stopped being true.
    void capabilitiesChanged(SpeechRecognizer::Capabilities newCapabilities);

    void errorOccurred(const QString& errorMessage);

private:
    State mState = State::Uninitialized;

    // Retained by setVocabulary() for every backend, so none has to remember
    // to keep words it could not use yet
    QStringList mVocabulary;

    // Whether the words in mVocabulary are what the backend is currently
    // biased toward. A repeat offer short-circuits to Applied only when this
    // is true - a backend that last answered Failed must be given another
    // chance, not agreement it never earned.
    bool mVocabularyApplied = false;
};

#endif // MUDLET_SPEECHRECOGNIZER_H
