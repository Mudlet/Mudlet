# Vosk library install from the Lua STT setup dialog

Date: 2026-07-30
Branch: `feature/speech-to-text`
Related: PR [#8960](https://github.com/Mudlet/Mudlet/pull/8960)

## Problem

The speech-to-text setup dialog can download language **models** but not the
**Vosk library** those models need. On a machine without `libvosk.dylib` /
`libvosk.so` / `libvosk.dll`, the feature is unusable and the dialog offers no
route forward: it reports "Speech-to-text is not available on this system" and
stops there. Nothing in the repository — no docs, no README, no in-app text —
tells a user what to install or where to put it.

This is a regression introduced by the branch itself. Commit `eea38e68c`
("refactor: replace C++ STT setup dialog with Lua UI") deleted
`dlgSpeechRecognitionSetup.cpp` (1131 lines), which had a working
"Download Library" button, a "Manual Install..." link, per-platform URLs and
install paths, a safe copy-to-temp-then-rename replace sequence, and
Windows-specific handling that extracted the MinGW runtime DLLs `libvosk.dll`
depends on. Commit `b40176393`, eleven minutes later, ported **model**
downloading to Lua. Library downloading was never ported.

No reviewer asked for that removal. Every inline comment on #8960 is from
Copilot and concerns other things; the only STTUI.lua comment is about model
checksum verification. The removal appears to be collateral from a
scope-reduction pass responding to maintainer feedback on 2026-03-05
("if there's a way that something can be done without modifying the core code
then that should be pursued as it lowers our maintenance burden").

## Approach

Restore the capability in Lua, adding to core C++ only what Lua provably cannot
do. This continues the direction `eea38e68c` set and answers the maintenance
burden concern directly: roughly 65 lines of new C++ against the ~1000 removed.

### C++ additions

| Addition | Location | Why it cannot be Lua |
|---|---|---|
| `vosk-lib/` added to both search-path lists | `VoskRecognizer.cpp` (~L121, ~L223) | Search paths are defined there |
| `hashFile(path, algorithm)` | `TLuaInterpreter.cpp` | No hashing is exposed to Lua today |
| `stt.reloadLibrary()` | `TLuaInterpreterSpeechToText.cpp` | Wraps existing `VoskRecognizer::resetLibraryLoadState()` |
| `stt.getPlatformKey()` | `TLuaInterpreterSpeechToText.cpp` | Compile-time platform/arch knowledge |

`stt.getPlatformKey()` earns its place: `getOS()` cannot identify arm64. Its
processor branch handles `Q_PROCESSOR_ARM_V5/V6/V7` but not `ARM_V8`, so on
Apple Silicon and aarch64 Linux it returns no processor string at all. Linux
needs `x86_64` vs `aarch64` to select the right archive. The helper returns one
of `macos`, `windows-x64`, `windows-x86`, `linux-x86_64`, `linux-aarch64`.

`hashFile()` is deliberately general rather than STT-specific so the pending
model-checksum review item can later be closed in pure Lua. It streams the file
through `QCryptographicHash` and returns a lowercase hex digest, or `nil` plus a
message.

`stt.reloadLibrary()` refuses while the recognizer is listening or initialized,
returning `false` plus a message rather than unloading a library whose resolved
function pointers are still live.

### Install location

Install to `<mudlet data>/vosk-lib/`, alongside the existing `vosk-models/`.

The deleted C++ code installed to `<app>/Contents/Frameworks/` on macOS,
`/usr/local/lib/` on Linux, and the application directory on Windows. Those are
poor targets: `/usr/local/lib` needs root, which is the likely reason a tester
on Fedora reported installing Vosk by hand even while the Download Library
button existed; and writing into a signed `.app` bundle breaks its code
signature so the application will not launch. A user data directory needs no
elevation, breaks no signature, and behaves identically on all three platforms.

### Pinned versions and verification

The URL and expected SHA-256 for each platform key live in a table in
`STTUI.lua`, so pinned versions can be updated without recompiling. Download is
refused unless the URL is HTTPS. After download the archive is hashed with
`hashFile()` and compared against the pin; on mismatch the archive is deleted
and nothing is installed.

Vosk publishes fixed release artifacts, so the hashes are stable. Note that
macOS builds stopped at v0.3.42 while Linux and Windows reached v0.3.45.

### Flow

1. `stt.getPlatformKey()` selects the entry; unknown key falls through to
   manual-install guidance.
2. `downloadFile()` fetches the archive to a temp path.
3. `hashFile()` verifies it against the pin; abort on mismatch.
4. `unzipAsync()` extracts to a temp directory.
5. The library (and on Windows its bundled runtime DLLs) is moved into
   `<mudlet data>/vosk-lib/`.
6. `stt.reloadLibrary()` re-runs detection.
7. The dialog re-renders, now showing the library as present.

A "Manual Install" link to `https://alphacephei.com/vosk/install` remains
visible throughout, and is the sole option when the platform key is unknown.

## Error handling

Every failure path sets a visible dialog status: unknown platform, non-HTTPS
URL, download error, hash mismatch, extraction failure, unwritable target
directory, and reload refusal.

The existing model-download path contains a silent-failure class this design
must not reproduce. `STTUI.lua` calls `unzipAsync(zipPath, extractPath)` and
discards the return value. `unzipAsync` has two early-return paths — invalid
`QTemporaryDir` and `mkpath` failure — that return `nil` plus a message via
`warnArgumentValue` without creating the future, so neither `sysUnzipDone` nor
`sysUnzipError` is ever raised. The dialog then waits forever with no error
shown, because `warnArgumentValue` only prints when `smDebugMode` is on. The
library flow checks the return value, and a watchdog on each async step reports
"nothing happened" rather than leaving a spinner.

## Testing

Extends `src/mudlet-lua/tests/STT_spec.lua`, using the stubbing pattern already
established there:

- platform key selects the correct URL
- hash mismatch aborts and installs nothing
- hash match proceeds to install
- unknown platform degrades to manual-install guidance
- `unzipAsync` returning `nil` surfaces an error instead of hanging

`hashFile()` gets C++ coverage in `test/` against a known SHA-256 vector.

## Risks

**macOS hardened runtime may make this moot on release builds.** Mudlet's macOS
releases are notarized, which requires hardened runtime, which by default
enables library validation — refusing to `dlopen` a third-party library not
signed by the same team. If that is in effect, a downloaded `libvosk.dylib` will
not load regardless of where it is installed, and the remedy is the
`com.apple.security.cs.disable-library-validation` entitlement. This affects
manual installation identically, so it is a pre-existing property of the feature
rather than something introduced here, but it should be tested against a signed
build before investing further in the macOS path. No entitlements file exists in
the repository; signing configuration lives elsewhere.

**The feature's inclusion is contested.** Two maintainers argued on #8960 for
pausing new features to reduce the bug backlog, and one suggested this belongs
outside core entirely. The small C++ footprint is a deliberate response and
should be stated explicitly in the PR.

## Out of scope

- Retrofitting pinned checksums onto model downloads (closes a standing Copilot
  review item; possible in pure Lua once `hashFile()` exists)
- Fixing the silent-failure class in the existing model-download path
- Any change to how the recognizer itself works
