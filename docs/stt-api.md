# The `stt.*` Speech-to-Text Bridge API

This document is the contract for Mudlet's speech-to-text bridge: the `stt.*`
Lua table, the `sysSTT*` events, and the semantics a client must honour to
call itself an implementation. It is written to be implementable by clients
other than desktop Mudlet — a client with a Lua runtime and any speech engine
(native, WebAssembly, or platform API) can provide this surface, and packages
written against it run unchanged.

Design contract, before the tables:

- **Inert until called.** Nothing listens, downloads, or runs until a script
  calls `stt.*`. The bridge ships no UI; buttons, routing and policy belong to
  packages consuming the events.
- **One recognizer per client.** There is one microphone and one decoder,
  shared across profiles. Events are raised on the **active profile**;
  routing text anywhere else is consumer policy, not bridge behaviour.
- **The recognizer's state is the single truth.** There is no parallel
  "active" flag; `stt.listening()` and `stt.getInfo().state` read the
  engine.
- **Honest capabilities.** `getInfo().capabilities` reports what the loaded
  backend genuinely does. Consumers MUST adapt to the flags rather than
  assume; implementations MUST NOT claim capabilities they do not deliver —
  in particular `onDevice = true` is a privacy statement that audio never
  leaves the machine.

## Functions — core recognition (every implementation)

| Function | Returns | Behaviour |
| --- | --- | --- |
| `stt.init([modelPath])` | `true` \| `nil, error` | Load a model and reach `ready`. A `modelPath` chooses its engine by the directory's own layout — a Vosk model and a sherpa-onnx model are never confused, even on a machine with both engines installed. A layout matching neither falls back to the install-preference order, but only on the first `stt.init()` of a session: once an engine exists, a path that names no recognisable layout keeps the engine that is already built rather than re-resolving to whichever is preferred. The engine is built lazily, on the first call that needs one, and is reused across calls that keep naming the same engine — including across `stt.close()`, which releases its native resources but does not discard the engine object. A `stt.init()` whose model belongs to a *different* engine than the one currently built replaces it: the old engine releases its resources and is torn down, and a fresh engine is built for the new model — engines can be switched within a session, with no restart needed. With no argument, uses the default installed model; if no model-based engine is installed but a model-less one is (the built-in macOS backend today), that one is used instead, with nothing to install. The three ways it can have nothing to load are distinguished, because they send the reader to different places: no engine library (naming where it was looked for), no model installed (naming the directory one belongs in), and a path that does not exist. When the configured Vosk model is missing and another is loaded in its place, the substitution is reported through `sysSTTError` rather than made quietly — the call still succeeds; the notice is Vosk's alone, because the `selectedModel` setting it describes is resolved only against Vosk's models directory. Naming a model whose engine is not installed is refused, naming that engine and where its library was looked for, and the engine already loaded is left as it was — a Vosk model on a sherpa-only machine never reaches the sherpa decoder to be reported as a broken sherpa model. The built-in macOS backend loads no models at all, so giving it a `modelPath` is refused rather than answered `true` for a model it never read. |
| `stt.start()` | `true` \| `nil, error` | Begin listening. `true` means the request was accepted, not always that audio is already flowing: a client that must ask permission first reports `starting`, and the outcome arrives as `sysSTTStateChanged`. A request refused outright — no model, a phrase still processing, a microphone that will not open, permission already denied — returns `nil` and an error, with the detail in `sysSTTError`. Starting while already listening, or while `starting`, succeeds without asking twice. |
| `stt.stop()` | `true` \| `nil, error` | Stop listening and **finalise**: remaining audio is decoded and reported via `sysSTTResult`, and the state reaches `ready`. Whether it has got there by the time the call returns depends on the engine, the way `start()`'s `starting` does: Vosk and sherpa-onnx decode the remainder inside the call, so the state is already `ready`; the built-in macOS backend returns while still `processing` and settles when its recognition task reports back (or after its finalise timeout), with `sysSTTStateChanged` announcing it. A package must not assume the first shape — `stt.stop()` immediately followed by `stt.start()`, and two `stt.toggle()` calls in quick succession, are **refused on macOS**, because starting from `processing` is refused. Wait for `sysSTTStateChanged` to report `ready`. Stopping when nothing is listening succeeds; stopping in `error` returns `nil` and a message, since "stopped" and "was never running because it failed" are different answers. |
| `stt.toggle()` | `true`=now listening, `false`=stopped \| `nil, error` | Convenience start/stop. When it starts, `true` carries the same caveat as `stt.start()`: the request was accepted, and a client that must ask permission first reports `starting` with the outcome arriving as `sysSTTStateChanged`. Both answers come from the same check. |
| `stt.close()` | `true` | Release the model and native resources; state returns to `uninitialized`. The engine itself is not discarded — a later `stt.init()` naming the same engine's model reuses it, while one naming a *different* engine's model tears this one down and builds that one instead, per `stt.init()` above. Safe when nothing is initialized. |
| `stt.available()` | boolean | Some engine — Vosk, sherpa-onnx, or the built-in macOS backend — is present and loadable. False is the normal state on a machine with nothing installed and no built-in backend for this platform. |
| `stt.initialized()` | boolean | A model is loaded (`state` is neither `uninitialized` nor `error`). |
| `stt.listening()` | boolean | The engine is capturing now. Reads the engine, always in step with `getInfo().listening`. |
| `stt.setSilenceTimeout(msec)` | `true` \| `nil, error` | After `msec` of continuous silence, listening ends exactly as `stt.stop()` would — finalised, never discarded. `0` (the default) keeps listening open-ended. Holds across listening sessions, not across restarts - neither this nor `setSensitivity` is saved. |
| `stt.setSensitivity(mode)` | `true` \| `nil, error` | How quickly an utterance is judged finished: `"short"` for commands, `"default"` for balanced use, `"long"` for dictation. Engines map this onto their own end-of-speech detection, so the effect is comparable rather than identical between them; an engine that must rebuild to apply it may pause briefly when a model is already loaded. Not every engine can, and `capabilities.sensitivityTuning` says which before you ask: the built-in macOS backend has no end-of-speech tuning at all, and an older libvosk without the endpointer symbol cannot be reached. Both report `sensitivityTuning = false` and return `nil` with a message naming the engine — a refusal to plan around, not a fault to retry, so it is returned rather than raised as `sysSTTError`. An engine that reports `sensitivityTuning = true` can still fail a single call: sherpa-onnx rebuilds the model to re-bake its endpoint rules, and a rebuild that fails returns `nil` with a message pointing at the `sysSTTError` the backend already raised. That one is worth retrying; the capability answer is not. |
| `stt.setVocabulary(words)` | boolean \| `nil, error` | Tell the engine which words to expect, so it favours them when a sound could be several things — a game's command verbs, its exits, the names of what is in front of you. Takes an array of words or short phrases; keep it to a shortlist rather than a dictionary, since applying one can make the engine rebuild and pause briefly. `true` means the engine took them. `false` is not an error: it means this backend cannot use vocabulary at all (see capabilities), so correct the results yourself instead. A backend that *can* and failed this time also returns `false`, reporting the fault through `sysSTTError`, so a caller branching only on the boolean still degrades gracefully while the failure stays visible. `nil, error` means there was no engine to offer the words to. What "took them" actually means differs enough between backends that it is spelled out per engine below the `capabilities` table. |
| `stt.getInfo()` | table | Everything the engine can say about itself at this moment: what is loaded, what it is capable of, and what it is doing right now. The keys are listed below. Always a table, even with no engine installed: every key below is present except `version` and `language`, which need a recognizer to exist, and every capability reads `false` - so a caller can read it without guarding. |

## Functions — model and library management (platform-tier)

These manage on-disk engine artifacts and are inherently platform-specific.
A client whose engine ships differently (for example a browser client using
WebAssembly builds or a platform speech API) MAY implement them as honest
stubs: `getPlatformKey` returning `nil`, `reloadLibrary`/`unloadLibrary`
returning `false` with a message, `listModels` returning `{}`.

| Function | Returns | Behaviour |
| --- | --- | --- |
| `stt.getModelPath()` | string | Directory models are installed into, for whichever model-based engine (Vosk or sherpa-onnx) is actually loaded — falling back to the install-preference order, then Vosk's own directory, before anything is loaded. The built-in macOS backend never answers for this: it has no models directory of its own. |
| `stt.getLibraryPath()` | string | User-writable directory the engine library is installed into, resolved the same way as `stt.getModelPath()`. |
| `stt.listModels()` | table | Array of `{name, path}` for installed models, across **every** model-based engine at once — not only whichever is currently loaded. Deliberately works without the engine library, so downloaded models stay visible. |
| `stt.getPlatformKey()` | string \| `nil` | Platform/architecture key for selecting an engine build (`"macos"`, `"windows-x64"`, `"windows-x86"`, `"linux-x86_64"`, `"linux-aarch64"`); `nil` when no published build exists. |
| `stt.reloadLibrary()` | boolean \| `false, error` | Re-run engine detection after an install, for **both** dynamically-loaded engines: it resets each one's "already looked" latch, releases the mapped module and probes again, so a newly installed engine is found without restarting Mudlet. It does *not* leave anything unmapped — re-detection means loading — so this is not the call for freeing a file you want to overwrite. Refuses while the recognizer is in use or holds live native resources, and answers whether speech is available at all afterwards, not whether Vosk in particular is. |
| `stt.unloadLibrary()` | `true` \| `false, error` | Unload the engine so its file can be deleted (Windows cannot delete a mapped module). Same refusal rules, plus one of its own: with any engine other than Vosk loaded it refuses rather than reporting an unload it cannot perform. **Vosk only** — see below. |

**Known limitation: `unloadLibrary` acts on Vosk alone.** Desktop Mudlet's
dynamically-loaded backends are Vosk and sherpa-onnx. `reloadLibrary` covers
both, so a sherpa-onnx install is found without restarting — but it re-probes
as it finishes, which maps the library straight back in. Leaving a module
unmapped is `unloadLibrary`'s job, and that one is still Vosk-only: with any
other engine loaded it refuses rather than reporting an unload it cannot
perform. So replacing an installed sherpa-onnx library on Windows, where the
loader will not let a mapped file be overwritten, still requires quitting
Mudlet. Until `unloadLibrary` is made engine-aware there is no way around
that.
The built-in macOS backend loads no library at all, so neither call has
anything to act on for it — but neither is a no-op either. Both refuse while
the recognizer is listening, is initialized, or holds live native resources —
any one of the three is enough. A loaded Apple recognizer meets at least the
second and third even when it is sitting idle in `ready`, so they return
`false` with "cannot unload the speech recognition library while it is in use,
close speech recognition first" — `stt.reloadLibrary()` says "cannot reload"
in the same sentence, since the two calls report their own names — a refusal
driven by a backend the Vosk loader knows nothing about. A loaded sherpa-onnx
recognizer blocks a Vosk unload in exactly the same way. Call `stt.close()`
first if the intent really is to unload Vosk — and note that closing is not
enough to make `stt.unloadLibrary()` succeed while a non-Vosk engine is the one
loaded: it then refuses with a message naming that engine, rather than
answering `true` for a library it has no way to release.

`stt.available()` / `getInfo().available`, `stt.getModelPath()`,
`stt.getLibraryPath()`, and `getInfo().searchPaths` answer for whichever
model-based engine is actually loaded once one is, and for the
install-preference order otherwise — `stt.available()` also counts a
model-less backend, since `stt.init()` can reach one with nothing installed.
`stt.listModels()` is the one exception: it unions every model-based
engine's installed models rather than picking one, since a downloaded model
should stay visible whether or not its engine happens to be the one loaded
right now.

## `stt.getInfo()`

| Key | Type | Meaning |
| --- | --- | --- |
| `backend` | string | Name of whichever engine is actually loaded (on desktop Mudlet: `"Vosk"`, `"sherpa-onnx"`, or `"Apple Speech"`), not a fixed value. |
| `available` | boolean | Engine present and loadable. |
| `initialized` | boolean | Model loaded. |
| `listening` | boolean | Capturing now. |
| `state` | string | `"uninitialized"`, `"ready"`, `"starting"`, `"listening"`, `"processing"`, `"error"`. Distinguishes `error` from `uninitialized`, which `initialized` alone cannot. `"starting"` means listening was asked for and something outside the client — permission, typically — has still to answer; a consumer shows "waiting" rather than "listening". |
| `modelPath` | string | The model actually loaded (empty when none) — not the install directory. |
| `silenceTimeout` | integer | Current timeout in ms; `0` while disabled. |
| `audioLevel` | number | Level last received from the microphone, `0.0`–`1.0`; `0` while not listening. Sampled during speech, it distinguishes a phrase the engine misheard from one it barely received — failures that look identical in the text and need opposite remedies. |
| `sensitivity` | string | `"short"`, `"default"` or `"long"`; how quickly an utterance is judged finished. |
| `capabilities` | table | See below. **May change when a model is loaded**, or when the engine library is unloaded or reloaded — on some backends biasing is a property of the model rather than of the engine. Re-read after `stt.init()` rather than caching at startup, or follow `sysSTTCapabilitiesChanged`. |
| `version`, `language` | string | Present once a recognizer instance exists. |
| `searchPaths` | table | Where a loadable engine library is looked for (platform-tier; may be empty). This is an install location, not a statement about the running engine: it names whichever model-based engine is loaded, and when none is - because the built-in macOS backend is active, or nothing is initialised yet - it names the one that would be preferred if you installed it. So it stays a real, checkable directory to install into even while `backend` names an engine that needs no library at all. |

### `capabilities`

| Key | Meaning when `true` |
| --- | --- |
| `biasing` | `setVocabulary` biases recognition toward the supplied words. |
| `grammar` | `setVocabulary` can constrain recognition to the supplied words. |
| `words` | `sysSTTWords` fires with per-word detail alongside each final. |
| `sensitivityTuning` | `setSensitivity` can tune end-of-speech detection. `false` means this engine never can, which is a different matter from a call that failed once — see that row. Named apart from the top-level `sensitivity` above, which is the mode currently in force: one says which setting, this says whether the setting can be changed at all. |
| `onDevice` | Audio is processed on this machine and never leaves it. An implementation backed by a remote service MUST report `false`. |

Desktop Mudlet's Vosk backend reports `{biasing = false, grammar = false,
words, sensitivityTuning, onDevice = true}`. `biasing` and `grammar` are always
`false` there. The other two each follow a symbol resolving in the installed
libvosk — `words` the word-level one, `sensitivityTuning` the endpointer — so an
older or partial library reports `false` rather than promising a `sysSTTWords`
that never arrives, or a sensitivity the engine cannot be told about. Unloading
or reloading the library changes both answers, announced through
`sysSTTCapabilitiesChanged`. Its sherpa-onnx backend reports `{biasing,
grammar = false, words = false, sensitivityTuning = true, onDevice = true}`,
where `sensitivityTuning` is unconditional — the endpoint rules are its own, so a
`setSensitivity` that answers `false` there means the model rebuild it needs
failed, not that tuning is beyond it — and `biasing` is `true`
only once a model whose directory carries a `bpe.vocab` file has loaded —
swap in a model without one and it drops back to `false`, announced through
`sysSTTCapabilitiesChanged` the way any other capability change is. The
built-in macOS backend reports `{biasing = true, grammar = false, words =
true, sensitivityTuning = false, onDevice = true}` unconditionally: none of them
depends on a model, because this backend loads none, and it decides its own
end-of-speech behaviour with nothing exposed to tune.

### `setVocabulary` in practice

`capabilities.biasing`/`capabilities.grammar` say whether a backend can use
vocabulary at all; what actually happens when a package calls
`stt.setVocabulary()` differs enough between desktop Mudlet's three backends
that a package should not assume one behaves like another:

- **Vosk**: `Unsupported`, always. Vosk has no biasing or grammar-constraint
  wiring, so the call returns `false` unconditionally and a package must
  correct results client-side.
- **sherpa-onnx**: real biasing, but only for a model whose directory carries
  a `bpe.vocab` file next to its sub-word units — a model without one reports
  `capabilities.biasing = false` and behaves exactly like Vosk. Applying words
  to a biasable model rebuilds the decoder from the retained vocabulary, so it
  cannot happen while listening: offering words mid-utterance is retained but
  returns `false` — reported through `sysSTTError` as a failure this attempt
  made, not as "this backend can't", since it can — and takes effect only at
  the model's next load, whether that is a later `setVocabulary()` call made
  once back in `ready`, or the next `stt.init()`. Words are matched to the
  model's own sub-word units case-wise before they are offered — upper for a
  model whose tokens are upper, lower for one whose tokens are lower — because
  a word in the wrong case tokenises into pieces the model never scores and the
  bias then silently does nothing. A word sherpa-onnx's hotword
  format cannot represent is left out of the biasing rather than passed
  through: an entry whose tokens open with `:` or `#`, which that format reads
  as a score, and an entry with nothing in it. Those are named in a
  `sysSTTError` when the vocabulary is set, and the rest of the list still
  applies — the parser has no escape for them and reacts to one by ending the
  process, so there is nothing safe to pass through.
- **The built-in macOS backend**: real biasing through `contextualStrings`,
  attached to each recognition request rather than baked into a model. There
  is nothing to rebuild, so the call always returns `true`. But a request
  already being decoded keeps the words it was built with, so new vocabulary
  takes effect on the **next** request — the next utterance while still
  listening, or the next `stt.start()` — never the utterance in progress.

## Events

All events are raised on the **active profile**, and every handler receives
**two string arguments**: the event name, then the payload below. String
arguments only — the one type every client event system carries.

| Event | Argument | When |
| --- | --- | --- |
| `sysSTTPartialResult` | text so far | During recognition; may revise as more audio arrives. Never final. |
| `sysSTTResult` | final text | An utterance completed — by endpointing, `stt.stop()`, or the silence timeout. The consumer's cue to act on the text. |
| `sysSTTWords` | JSON string | Alongside each `sysSTTResult`, on backends whose `words` capability is true. Describes **the text as emitted**: an implementation that drops a word from the result must drop it here too, or the two events describe different phrases. Schema below. |
| `sysSTTStateChanged` | state name | Any transition between the six states. |
| `sysSTTError` | message | Anything the user should know went wrong: refusals to start, capture faults, model failures, and a configured model quietly replaced by another. The state moves to `error` for faults, but refusal messages can arrive without a state change. Most refusals carry the same text the call returned as its second value; a refused `stt.start()` is the exception, since the engine's own reason is what the event carries while the call returns only a pointer to it. **Raised with no engine installed too** — a consumer driving the bridge from events alone must be able to tell "no engine" from "nothing said yet". |
| `sysSTTCapabilitiesChanged` | JSON string | The `capabilities` table changed: a model loaded, a model was released — including by `stt.close()` — or the engine library was unloaded or reloaded underneath it. Same keys as `getInfo().capabilities`. Which of those actually fire it differs by backend, because different backends hang different capabilities off different things: sherpa-onnx's `biasing` follows the loaded model, so releasing one changes it, while Vosk's `words` follows a library symbol and releasing a model changes nothing. |

### `sysSTTWords` schema

A JSON array, one object per word of the accompanying final result:

```json
[{"word": "quick", "conf": 1.0, "start": 0.75, "end": 1.02}, ...]
```

`conf` is 0–1; `start`/`end` are seconds, but which clock they are measured on
is the engine's own and a consumer must not assume one: Vosk resets its decoder
only when a session is cancelled, so its timings accumulate across the whole
listening session, while the built-in macOS backend builds a fresh recognition
request per utterance, so its timings restart with each phrase. Desktop
Mudlet's sherpa-onnx backend never raises this event at all — its `words`
capability is `false` — so it has no timings of either shape to describe. Treat them as relative within a
single `sysSTTWords` payload rather than as a session timeline. The
timings are load-bearing: a word whose span covers pooled silence rather than
speech is how decoder hallucinations are told apart from spoken words, so an
implementation that cannot supply real timings must not claim the `words`
capability.

## Semantics implementations must preserve

1. **Stop finalises; only errors and engine artifacts discard.** `stt.stop()`
   and the silence timeout both deliver the pending utterance via
   `sysSTTResult`. No path silently drops recognised speech except a fault,
   which reports via `sysSTTError` — and except what the engine produced from
   silence rather than from a person. Desktop Mudlet's Vosk backend discards a
   lone filler word the decoder itself scored below 0.8 confidence, or that it
   returned no confidence for at all, and a leading word whose timings show it
   spanned a pause rather than being spoken. Neither reports, because neither
   was said. A lone filler word the decoder is confident about is delivered,
   however the utterance finished - "i" is a command, not an artifact.
2. **Refusals the engine caused speak.** A call the engine could not satisfy
   says why through `sysSTTError` as well as in its return value. This holds
   when there is no engine at all: an implementation with nothing installed
   still raises the event, or a consumer written against events alone cannot
   tell a missing engine from a quiet microphone. A refusal caused by the
   script's own arguments is returned but not announced, since one package's
   mistake is not news for every other package on the profile - `stt.init()`
   given a path that does not exist, or given any path at all to a backend
   that loads none, are both of that kind, and a package offering a saved
   setting on every profile load would otherwise report one on every start. So is a limit
   the engine can never lift, which is a capability answer of rule 3's kind
   rather than a fault: `setSensitivity` on a backend whose
   `capabilities.sensitivityTuning` is false. Announced, it would fire on every
   single start for a package that reapplies its saved settings whenever
   speech begins. A call that fails on an engine which *can* do the thing is
   an ordinary refusal and speaks, so the two must be told apart before the
   attempt rather than guessed from its result.
3. **`setVocabulary`'s boolean is a capability answer**, not a success flag.
   Packages branch on it: `true` → engine handles vocabulary; `false` → apply
   client-side correction.
4. **Permission prompts are the implementation's problem.** The first
   `start()` may trigger an OS microphone consent flow; a denial reports as
   `sysSTTError` with the state moving to `error`, never as a hang.
5. **No recognition telemetry.** Nothing recognised, partial or final, is
   sent anywhere by the bridge. What packages do with the text is their
   declared business, but the bridge itself is local-only.
