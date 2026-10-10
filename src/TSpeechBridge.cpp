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

#include "TSpeechBridge.h"

#include "Host.h"
#include "HostManager.h"
#include "SherpaRecognizer.h"
#include "SpeechRecognizer.h"
#include "TAppFrontend.h"
#include "TEvent.h"
#include "VoskRecognizer.h"

#if defined(Q_OS_MACOS)
#include "AppleSpeechRecognizer.h"
#endif

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;

TSpeechBridge::TSpeechBridge(QObject* parent)
: QObject(parent)
{
    if (smpSelf) {
        qWarning() << "TSpeechBridge::TSpeechBridge() WARNING - a TSpeechBridge already exists, so instance() keeps pointing at that one.";
        return;
    }
    smpSelf = this;
}

TSpeechBridge::~TSpeechBridge()
{
    if (smpSelf == this) {
        smpSelf = nullptr;
    }
}

SpeechRecognizer* TSpeechBridge::speechRecognizer() const
{
    return mpSpeechRecognizer;
}

void TSpeechBridge::raiseSpeechEvent(const QString& name, const QString& value)
{
    // The owner outranks the active profile: with the microphone held, every
    // result, state change and fault belongs to the session that is running,
    // whatever the player has since tabbed to. Only with nobody listening does
    // "the profile in front" become the right answer - that is where a refusal
    // from stt.init() goes. Capability changes are not here at all: they
    // describe the engine rather than a session, so announceSpeechCapabilities-
    // IfChanged() raises them on every profile.
    //
    // With one exception, and it is worth exactly one sentence. An engine that
    // ends a session says so in two steps - the state first, so that a handler
    // is never told the microphone is still open, and then what became of the
    // phrase that was in flight - and the release rides on the first of them.
    // The second step is the one that says the words are lost, and it belongs
    // to the profile that spoke them rather than to whoever is in front now. So
    // the state handler leaves that profile behind for the next event and this
    // spends it, once: a release with no session ending behind it - the
    // ordinary stop - leaves nothing here, and routing goes straight back to
    // the profile in front.
    Host* pHost = mpMicrophoneOwner.data();
    if (!pHost && mpMicrophoneOwnerEnding) {
        pHost = mpMicrophoneOwnerEnding.data();
        mpMicrophoneOwnerEnding = nullptr;
    }
    if (!pHost) {
        pHost = TAppFrontend::instance()->getActiveHost();
    }
    raiseSpeechEventOn(pHost, name, value);
}

void TSpeechBridge::raiseSpeechEventOn(Host* pHost, const QString& name, const QString& value)
{
    if (!pHost) {
        // A fault landing as the last profile closes has nowhere to be raised,
        // and dropping it silently leaves no trace of it anywhere. Only the
        // error path is logged: the result and state events are ordinary
        // traffic, and warning on every one of those would bury this.
        if (name == qsl("sysSTTError")) {
            qWarning().noquote() << "speech recognition error with no active profile to report it to:" << value;
        }
        return;
    }
    const bool error = (name == qsl("sysSTTError"));
    if (error && mSpeechErrorsBeingDelivered > 0) {
        return;
    }
    TEvent event{};
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(value);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    if (error) {
        ++mSpeechErrorsBeingDelivered;
    }
    pHost->raiseEvent(event);
    if (error) {
        --mSpeechErrorsBeingDelivered;
    }
}

Host* TSpeechBridge::microphoneOwner() const
{
    return mpMicrophoneOwner;
}

bool TSpeechBridge::claimMicrophoneFor(Host* pHost)
{
    if (!pHost) {
        return false;
    }
    if (mpMicrophoneOwner == pHost) {
        return true;
    }

    // A phrase still being decoded is owed to the profile that spoke it, and
    // the claim is what routes it there - so the microphone cannot change hands
    // until that has landed. Taking it here would orphan the phrase: the owner
    // would move, the result would arrive for a profile that never said it, and
    // the one that did would be left with a session that simply stopped.
    //
    // Refused rather than waited for, because a decode can outlive the call. It
    // is the same answer docs/stt-api.md already gives a stop-then-start on a
    // backend that finalises asynchronously - try again in a moment - and the
    // caller passes that on rather than a session of somebody else's being
    // destroyed for a start that was going to be refused anyway.
    if (mpSpeechRecognizer && mpSpeechRecognizer->state() == SpeechRecognizer::State::Processing) {
        return false;
    }


    // Told before the microphone moves, and through the old owner by name
    // rather than through raiseSpeechEvent(): a moment later the owner is the
    // profile that asked for it, and the notice would arrive at the game that is
    // about to start listening instead of the one that just stopped.
    Host* pLosing = mpMicrophoneOwner;
    if (pLosing) {
        raiseSpeechEventOn(pLosing, qsl("sysSTTHandover"), pHost->getName());
        // Ended rather than merely renamed. One decoder means the audio the old
        // profile was collecting cannot be kept while the new one records over
        // it, and leaving it running would route the rest of a half-spoken
        // phrase to a game that never asked for it. The stop happens while the
        // old owner still holds the claim, so the state changes it raises are
        // its news too - and the release that triggers is why the assignment
        // below comes last.
        if (mpSpeechRecognizer && (mpSpeechRecognizer->listening() || mpSpeechRecognizer->starting())) {
            mpSpeechRecognizer->stopListening();
        }

        // Asked again, because the state to test is the one the stop left behind
        // rather than the one before it. A backend that finalises the last phrase
        // asynchronously - AppleSpeechRecognizer does - returns from the stop
        // while still Processing, so the guard above saw only Listening and had
        // nothing to catch. Moving the owner now would hand that phrase to the
        // profile taking the microphone instead of the one that spoke it.
        //
        // The caller gets the same "try again in a moment" it gets above. The
        // losing profile has already been told of the handover, and that stands:
        // its session really has ended, and it keeps the microphone only until
        // its phrase lands, when the session's end releases it. The retry then
        // finds nobody holding it and announces nothing, so the handover is told
        // once. Announcing it after the stop instead would put it behind the
        // state change, and docs/stt-api.md tells a script the state change is
        // what follows sysSTTHandover.
        if (mpSpeechRecognizer && mpSpeechRecognizer->state() == SpeechRecognizer::State::Processing) {
            return false;
        }
    }

    mpMicrophoneOwner = pHost;
    emit microphoneOwnerChanged();
    return true;
}

void TSpeechBridge::releaseMicrophone()
{
    if (!mpMicrophoneOwner) {
        return;
    }
    mpMicrophoneOwner = nullptr;
    emit microphoneOwnerChanged();
}

// Which Backend enum value corresponds to a live recognizer's concrete type.
// Not kept as a member: the object's own type already says which backend
// built it, so a second, parallel note of the same fact could only drift from
// it. Returns Auto for a null recognizer or a type this does not recognise,
// which initSpeechRecognition() below treats as "nothing to compare against".
static SpeechRecognizerFactory::Backend currentSpeechBackend(SpeechRecognizer* pRecognizer)
{
    if (qobject_cast<SherpaRecognizer*>(pRecognizer)) {
        return SpeechRecognizerFactory::Backend::Sherpa;
    }
    if (qobject_cast<VoskRecognizer*>(pRecognizer)) {
        return SpeechRecognizerFactory::Backend::Vosk;
    }
#if defined(Q_OS_MACOS)
    if (qobject_cast<AppleSpeechRecognizer*>(pRecognizer)) {
        return SpeechRecognizerFactory::Backend::Platform;
    }
#endif
    return SpeechRecognizerFactory::Backend::Auto;
}

void TSpeechBridge::initSpeechRecognition(SpeechRecognizerFactory::Backend backend)
{
    if (mpSpeechRecognizer) {
        // Auto and "the backend already built" both keep what is there:
        // stt.start() and the other Lua setters pass Auto or an on-demand
        // choice on every call, and rebuilding on every one of those would
        // tear down a working recognizer under a caller who never asked to
        // switch engines.
        if (backend == SpeechRecognizerFactory::Backend::Auto || backend == currentSpeechBackend(mpSpeechRecognizer)) {
            return;
        }
    }

    // Build the replacement before touching what is already working. The
    // backend is derived from the model directory's layout, so a mistyped path
    // on a machine with only one engine installed resolves to the other one and
    // create() answers nullptr - and tearing down first would have cost the
    // caller their loaded model to answer a call that could not be honoured.
    SpeechRecognizer* pReplacement = SpeechRecognizerFactory::create(backend, this);
    if (!pReplacement) {
        return;
    }

    // Everything about the replacement is established - wired up, then
    // published - before the old engine is touched. Retiring it first raised
    // sysSTTStateChanged from its releaseResources() while mpSpeechRecognizer
    // still pointed at it, so a Lua handler calling stt.init() from that event
    // built and initialised an engine the outer frame then threw away: three
    // consecutive statements acting on three different objects. A re-entrant
    // caller now finds the new engine already in place and fully connected.
    // Bridge glue only: recognizer signals surface as Lua events on the profile
    // holding the microphone (see raiseSpeechEvent()). Text routing, UI state
    // and policy all belong to the packages consuming these events.
    connect(pReplacement, &SpeechRecognizer::partialResult, this, [this](const QString& text) {
        raiseSpeechEvent(qsl("sysSTTPartialResult"), text);
    });
    connect(pReplacement, &SpeechRecognizer::finalResult, this, [this](const QString& text) {
        // Counted while it is delivered, so that a handler closing the engine on
        // the strength of this very phrase is not told the phrase was lost -
        // see sttClose(). Counted rather than flagged: a handler is free to
        // finalise another one from inside this one.
        ++mSpeechResultsBeingDelivered;
        raiseSpeechEvent(qsl("sysSTTResult"), text);
        --mSpeechResultsBeingDelivered;
    });
    connect(pReplacement, &SpeechRecognizer::errorOccurred, this, [this](const QString& message) {
        raiseSpeechEvent(qsl("sysSTTError"), message);
    });
    // Word-level detail travels as one JSON string argument: table arguments
    // need per-Host Lua registry bookkeeping this glue should not own, and a
    // string is the one type every client's event system carries
    connect(pReplacement, &SpeechRecognizer::wordsResult, this, [this](const QVariantList& words) {
        QJsonArray array;
        for (const QVariant& word : words) {
            array.append(QJsonObject::fromVariantMap(word.toMap()));
        }
        raiseSpeechEvent(qsl("sysSTTWords"), QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
    });
    // Documented as re-readable rather than cached, so the change has to reach a
    // consumer that did read it once. The recognizer noticing its own view move
    // is a trigger, not the decision: whether Lua saw a change is decided
    // against what Lua was last told, which is what the engine cannot know.
    connect(pReplacement, &SpeechRecognizer::capabilitiesChanged, this, [this](SpeechRecognizer::Capabilities) {
        announceSpeechCapabilitiesIfChanged();
    });
    connect(pReplacement, &SpeechRecognizer::stateChanged, this, [this](SpeechRecognizer::State newState) {
        QString stateName;
        switch (newState) {
        case SpeechRecognizer::State::Ready:
            stateName = qsl("ready");
            break;
        case SpeechRecognizer::State::Starting:
            stateName = qsl("starting");
            break;
        case SpeechRecognizer::State::Listening:
            stateName = qsl("listening");
            break;
        case SpeechRecognizer::State::Processing:
            stateName = qsl("processing");
            break;
        case SpeechRecognizer::State::Error:
            stateName = qsl("error");
            break;
        case SpeechRecognizer::State::Uninitialized:
            stateName = qsl("uninitialized");
            break;
        }
        raiseSpeechEvent(qsl("sysSTTStateChanged"), stateName);
        // Released only once the event above has gone to the profile that owned
        // the session, so the state change that ends a session is still the old
        // owner's news. Processing keeps the claim: the phrase is still being
        // decoded and its result is owed to the same profile.
        //
        // Against the state the engine is in now rather than the one the event
        // described: a handler of that event can have started a session of its
        // own - "ready" is exactly where a package waits to start listening -
        // and it took the microphone as it did. Releasing on the older state
        // would leave that session running with nobody holding it, which
        // stt.listening() answers for by saying no.
        const SpeechRecognizer::State settledState = mpSpeechRecognizer ? mpSpeechRecognizer->state() : newState;
        switch (settledState) {
        case SpeechRecognizer::State::Ready:
        case SpeechRecognizer::State::Error:
        case SpeechRecognizer::State::Uninitialized:
            // Left for whatever the engine says next, and for that alone - see
            // raiseSpeechEvent(). An engine that ends a session mid-phrase
            // reports the loss immediately after this state change, and the
            // profile that was speaking is the one that needs to hear it.
            mpMicrophoneOwnerEnding = mpMicrophoneOwner;
            QTimer::singleShot(0ms, this, [this]() {
                mpMicrophoneOwnerEnding = nullptr;
            });
            releaseMicrophone();
            break;
        case SpeechRecognizer::State::Starting:
        case SpeechRecognizer::State::Listening:
        case SpeechRecognizer::State::Processing:
            break;
        }
    });

    // Latched, then swapped, then retired: the old engine is only released
    // once mpSpeechRecognizer already names its replacement. It is parented to
    // this and has live signal connections - releaseResources() drops its
    // native resources, disconnect() detaches the connections made above for
    // it, and deleteLater() - rather than delete - defers the destruction,
    // since a handler reached through one of those connections may still be on
    // the stack (the same reentrancy hazard SherpaRecognizer::slot_pcmReady()
    // guards against).
    SpeechRecognizer* pRetiring = mpSpeechRecognizer;
    mpSpeechRecognizer = pReplacement;
    if (pRetiring) {
        // Disconnected first: releaseResources() sets the state and, on a
        // backend whose capabilities follow the model, announces the change -
        // and mpSpeechRecognizer already names the replacement by now, so a
        // handler reached from either event would read the new engine while
        // being told about the dead one. The retirement is meant to be silent.
        // Said before releaseResources() below, which is what resets the state
        // this reads - not before the disconnect, which has no bearing on it:
        // raiseSpeechEvent() goes to the microphone's owner rather than over
        // the retiring engine's connections. An engine swap reached from
        // stt.init() while a phrase is in flight takes that phrase with it,
        // and rule 1 requires a drop the script did not ask for to report.
        if (pRetiring->listening() || pRetiring->state() == SpeechRecognizer::State::Processing) {
            raiseSpeechEvent(qsl("sysSTTError"), qsl("changing the speech engine stopped the listening session that was under way - anything said during it is lost"));
        }
        pRetiring->disconnect();
        pRetiring->releaseResources();
        pRetiring->deleteLater();
    }

    // Last, once mpSpeechRecognizer names the engine Lua will read and the old
    // one is released and detached - it is awaiting deleteLater() rather than
    // already destroyed, which is what lets stt.init() compare against it. A recognizer existing at all changes what getInfo() answers,
    // and so does replacing one engine with another that can do different
    // things - neither of which any recognizer is in a position to announce for
    // itself. Reached whenever an engine is created or swapped - any stt call
    // that finds none built, or asks for a different one - so a package
    // following the event rather than re-reading no longer believes an engine's
    // first answer for ever (#10760).
    announceSpeechCapabilitiesIfChanged();
}

// The capabilities payload as stt.getInfo() would report them: with no
// recognizer every one is false, which is what sttGetInfo() pushes and so what
// a consumer reads before anything has created one.
static QString speechCapabilitiesPayload(const SpeechRecognizer* pRecognizer)
{
    const SpeechRecognizer::Capabilities current = pRecognizer ? pRecognizer->capabilities() : SpeechRecognizer::Capabilities{};
    QJsonObject capabilities;
    capabilities.insert(qsl("biasing"), current.biasing);
    capabilities.insert(qsl("grammar"), current.grammar);
    capabilities.insert(qsl("words"), current.wordResults);
    // Carried like the rest: docs/stt-api.md promises this event the same keys
    // as getInfo().capabilities, and a package rebuilding from the event would
    // otherwise read a missing key as "this engine never can" - on Vosk,
    // exactly the flag that moves when the library is unloaded or reloaded.
    capabilities.insert(qsl("sensitivityTuning"), current.sensitivityTuning);
    capabilities.insert(qsl("onDevice"), current.onDevice);
    return QString::fromUtf8(QJsonDocument(capabilities).toJson(QJsonDocument::Compact));
}

void TSpeechBridge::announceSpeechCapabilitiesIfChanged()
{
    if (mAnnouncedSpeechCapabilities.isEmpty()) {
        // What Lua has been reading from getInfo() all along, so that a
        // recognizer coming into existence registers as the change it is
        mAnnouncedSpeechCapabilities = speechCapabilitiesPayload(nullptr);
    }

    const QString current = speechCapabilitiesPayload(mpSpeechRecognizer);
    if (current == mAnnouncedSpeechCapabilities) {
        return;
    }
    // Every profile, not just the microphone's owner - the one event here that
    // is broadcast. Results, state and faults belong to the session that is
    // running, so they go to whoever holds the microphone. Capabilities are not
    // a property of a session at all: they describe the engine, and every
    // profile reads the same ones back from stt.getInfo(). Sending this to the
    // owner alone would change what the other profiles read while telling only
    // one of them, which is the same fault this function exists to fix.
    //
    // Over a copy of the list, because each raise runs Lua and a handler may
    // open or close a profile while this is walking it.
    auto* pHostManager = HostManager::self();
    const QList<QSharedPointer<Host>> profiles = pHostManager ? pHostManager->hostList() : QList<QSharedPointer<Host>>();
    // Nothing to deliver to means nothing is announced and nothing is recorded:
    // the baseline must only ever name what was actually delivered. Recording an
    // announcement that went nowhere would leave every later comparison finding
    // the baseline already equal, and the change would never be made good.
    if (profiles.isEmpty()) {
        return;
    }
    // Recorded before the first raise, not after the last: a handler reached
    // from one of these is free to call back in here, and an unrecorded
    // baseline would let it announce the same move again.
    mAnnouncedSpeechCapabilities = current;
    for (const auto& pHost : profiles) {
        // Each raise runs Lua, and reacting to a capability change by calling
        // stt.reloadLibrary() or stt.init() is the documented thing to do - so a
        // handler can land back in here, announce a newer payload to every
        // profile, and return. Carrying on would then deliver this older one to
        // the profiles the outer loop has not reached, leaving them holding a
        // value nothing will correct: the baseline already names the newer one.
        // The same shape as MudletMedia::setMuted()'s guard over its list.
        if (mAnnouncedSpeechCapabilities != current) {
            break;
        }
        if (pHost) {
            raiseSpeechEventOn(pHost.data(), qsl("sysSTTCapabilitiesChanged"), current);
        }
    }
}

void TSpeechBridge::profileClosing(Host* pHost)
{
    // A profile that closes while holding the microphone takes its session with
    // it. Left running, the owner pointer would clear with the Host and every
    // further result would fall through to whichever profile is now in front -
    // a game that never asked to listen, receiving the tail of someone else's
    // phrase. allProfilesClosed() is the same rule with nobody left to hand
    // back to.
    if (mpMicrophoneOwner == pHost) {
        // Processing counts: a phrase still decoding for the profile that is
        // going away has nowhere to be delivered, and the release below would
        // otherwise let it fall through to whichever profile is now in front.
        if (mpSpeechRecognizer && (mpSpeechRecognizer->listening() || mpSpeechRecognizer->starting() || mpSpeechRecognizer->state() == SpeechRecognizer::State::Processing)) {
            mpSpeechRecognizer->cancel();
        }
        releaseMicrophone();
    }
}

void TSpeechBridge::allProfilesClosed()
{
    // One recognizer is shared across profiles and outlives any one of them,
    // but with none left there is no profile to raise sysSTT* on and nobody to
    // stop it: a session the closing profile started would otherwise hold the
    // microphone open, recording light and all, for the rest of the run.
    if (mpSpeechRecognizer) {
        mpSpeechRecognizer->cancel();
        mpSpeechRecognizer->releaseResources();
    }
}
