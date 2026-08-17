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
  "active" flag; `stt.isListening()` and `stt.getInfo().state` read the
  engine.
- **Honest capabilities.** `getInfo().capabilities` reports what the loaded
  backend genuinely does. Consumers MUST adapt to the flags rather than
  assume; implementations MUST NOT claim capabilities they do not deliver —
  in particular `onDevice = true` is a privacy statement that audio never
  leaves the machine.

## Functions — core recognition (every implementation)

| Function | Returns | Behaviour |
| --- | --- | --- |
| `stt.init([modelPath])` | `true` \| `nil, error` | Load a model and reach `ready`. With no argument, uses the default installed model; errors clearly when none exists. |
| `stt.start()` | `true` \| `nil, error` | Begin listening. Refusals from `error`/`processing`/`uninitialized` states raise `sysSTTError` with the reason; starting while already listening succeeds silently. |
| `stt.stop()` | `true` \| `nil, error` | Stop listening and **finalise**: remaining audio is decoded and reported via `sysSTTResult` before the state returns to `ready`. |
| `stt.toggle()` | `true`=now listening, `false`=stopped \| `nil, error` | Convenience start/stop. |
| `stt.close()` | `true` | Release the model and native resources; state returns to `uninitialized`. Safe when nothing is initialized. |
| `stt.isAvailable()` | boolean | The engine is present and loadable. False is the normal state on a machine with nothing installed. |
| `stt.isInitialized()` | boolean | A model is loaded (`state` is neither `uninitialized` nor `error`). |
| `stt.isListening()` | boolean | The engine is capturing now. Reads the engine, always in step with `getInfo().listening`. |
| `stt.setSilenceTimeout(msec)` | `true` \| `nil, error` | After `msec` of continuous silence, listening ends exactly as `stt.stop()` would — finalised, never discarded. `0` (the default) keeps listening open-ended. Persists across sessions. |
| `stt.setVocabulary(words)` | boolean | Supply an array of words/phrases for biasing or grammar constraint. Returns `true` **only when the engine applied it**; `false` is not an error — it means this backend cannot use vocabulary (see capabilities) and the caller should correct results client-side instead. |
| `stt.getInfo()` | table \| `nil` | Introspection snapshot; see below. `nil` when speech-to-text is unavailable. |

## Functions — model and library management (platform-tier)

These manage on-disk engine artifacts and are inherently platform-specific.
A client whose engine ships differently (for example a browser client using
WebAssembly builds or a platform speech API) MAY implement them as honest
stubs: `getPlatformKey` returning `nil`, `reloadLibrary`/`unloadLibrary`
returning `false` with a message, `listModels` returning `{}`.

| Function | Returns | Behaviour |
| --- | --- | --- |
| `stt.getModelPath()` | string | Directory models are installed into. |
| `stt.getLibraryPath()` | string | User-writable directory the engine library is installed into. |
| `stt.listModels()` | table | Array of `{name, path}` for installed models. Deliberately works without the engine library, so downloaded models stay visible. |
| `stt.getPlatformKey()` | string \| `nil` | Platform/architecture key for selecting an engine build (`"macos"`, `"windows-x64"`, `"linux-x86_64"`, `"linux-aarch64"`); `nil` when no published build exists. |
| `stt.reloadLibrary()` | boolean \| `false, error` | Re-run engine detection after an install. Refuses while the recognizer is in use or holds live native resources. |
| `stt.unloadLibrary()` | `true` \| `false, error` | Unload the engine so its file can be deleted (Windows cannot delete a mapped module). Same refusal rules. |

`hashFile(path, "sha256"|"sha1"|"md5")` is registered alongside the bridge as
a general-purpose global — packages use it to verify downloaded engine
archives — but it is not part of the `stt` namespace.

## `stt.getInfo()`

| Key | Type | Meaning |
| --- | --- | --- |
| `backend` | string | Engine name (`"Vosk"`). |
| `available` | boolean | Engine present and loadable. |
| `initialized` | boolean | Model loaded. |
| `listening` | boolean | Capturing now. |
| `state` | string | `"uninitialized"`, `"ready"`, `"listening"`, `"processing"`, `"error"`. Distinguishes `error` from `uninitialized`, which `initialized` alone cannot. |
| `modelPath` | string | The model actually loaded (empty when none) — not the install directory. |
| `silenceTimeout` | integer | Current timeout in ms; `0` while disabled. |
| `capabilities` | table | See below. |
| `version`, `language` | string | Present once a recognizer instance exists. |
| `searchPaths` | table | Where the engine library is looked for (platform-tier; may be empty). |

### `capabilities`

| Key | Meaning when `true` |
| --- | --- |
| `biasing` | `setVocabulary` biases recognition toward the supplied words. |
| `grammar` | `setVocabulary` can constrain recognition to the supplied words. |
| `words` | `sysSTTWords` fires with per-word detail alongside each final. |
| `onDevice` | Audio is processed on this machine and never leaves it. An implementation backed by a remote service MUST report `false`. |

Desktop Mudlet's Vosk backend reports `{biasing = false, grammar = false,
words = true, onDevice = true}`.

## Events

All events are raised on the **active profile**, with string arguments only —
the one argument type every client event system carries.

| Event | Argument | When |
| --- | --- | --- |
| `sysSTTPartialResult` | text so far | During recognition; may revise as more audio arrives. Never final. |
| `sysSTTResult` | final text | An utterance completed — by endpointing, `stt.stop()`, or the silence timeout. The consumer's cue to act on the text. |
| `sysSTTWords` | JSON string | Alongside each `sysSTTResult`, on backends whose `words` capability is true. Schema below. |
| `sysSTTStateChanged` | state name | Any transition between the five states. |
| `sysSTTError` | translated message | Anything the user should know went wrong: refusals to start, capture faults, model failures. The state moves to `error` for faults, but refusal messages can arrive without a state change. |

### `sysSTTWords` schema

A JSON array, one object per word of the accompanying final result:

```json
[{"word": "quick", "conf": 1.0, "start": 0.75, "end": 1.02}, ...]
```

`conf` is 0–1; `start`/`end` are seconds on the session's audio clock. The
timings are load-bearing: a word whose span covers pooled silence rather than
speech is how decoder hallucinations are told apart from spoken words, so an
implementation that cannot supply real timings must not claim the `words`
capability.

## Semantics implementations must preserve

1. **Stop finalises; only errors discard.** `stt.stop()` and the silence
   timeout both deliver the pending utterance via `sysSTTResult`. No path
   silently drops recognised speech except a fault, which reports via
   `sysSTTError`.
2. **Refusals speak.** A `start()` that cannot start says why through
   `sysSTTError`; silence after a call means it worked.
3. **`setVocabulary`'s boolean is a capability answer**, not a success flag.
   Packages branch on it: `true` → engine handles vocabulary; `false` → apply
   client-side correction.
4. **Permission prompts are the implementation's problem.** The first
   `start()` may trigger an OS microphone consent flow; a denial reports as
   `sysSTTError` with the state moving to `error`, never as a hang.
5. **No recognition telemetry.** Nothing recognised, partial or final, is
   sent anywhere by the bridge. What packages do with the text is their
   declared business, but the bridge itself is local-only.
