# AI Assistant instructions for Mudlet

## Project overview

Mudlet is a cross-platform MUD client built with Qt6 and C++20, providing scripting capabilities in Lua 5.1. The project emphasizes "powerful simplicity" - clean interface with deep customization options.

## Skills

Task-specific instructions live in `.agents/skills/<name>/SKILL.md`, in the [Agent Skills](https://agentskills.io) format. Claude Code, GitHub Copilot, Cursor and OpenAI Codex all read this directory (Claude Code via the `.claude/skills` symlink). Read the relevant one before starting that kind of task rather than working from memory:

- `build-mudlet` - building, rebuilding or running Mudlet on any platform
- `fuzz-apis` - fuzzing the parsers and scripting API under sanitizer builds
- `improve-performance` - benchmarking, measurement discipline and finding where time goes
- `improve-test-coverage` - measuring C++ line coverage and growing the test suite
- `open-pr` - publishing a branch and opening a pull request upstream

## Coding standards
All files should end with a newline character at the end of the file.

### C++ Conventions
In general: write modern C++20 code, but avoid C++ exceptions, templates, and concepts as those have performance/complexity considerations, avoiding which has made Mudlet the success it is today.

Use range-based for loops instead of iterator-based or index-based loops where appropriate.

See `docs/CONTRIBUTING.md` for the coding standards as well as the information below:

```cpp
// Class names: PascalCase with 'T' prefix for main classes
class TConsole : public QWidget

// Member variables: camelCase with 'm' prefix
// avoid starting with the `is` prefix
QString mProfileName;

// avoid starting function names with the `is` prefix as well
bool ready();

// Qt signals/slots: camelCase
signals:
    void profileChanged(const QString& name);
```

### QFlags and enums

When working with Qt `Q_DECLARE_FLAGS` types, always use the proper enum/flags type - never pass them as raw `int`:

```cpp
// Good - uses the typed enum
void setServerOrigin(Host*, const Host::DiscordOptionFlag);
QMap<Host*, Host::DiscordOptionFlags> mServerOriginFlags;

// Bad - loses type safety
void setServerOrigin(Host*, int flag);
QMap<Host*, int> mServerOriginFlags;
```

### String handling

```cpp
// Use qsl() macro for string literals (defined as QStringLiteral)
QString objectName = qsl("timer(Host:%1)(TTimerId:%2)").arg(hostName, timerName);

// Prefer QString for UI, tr() for user-visible strings
QString displayText = tr("Connection failed: %1").arg(errorMessage);

// Always add contextual comments for translators using //:
//: Toast notification shown when user dismisses an editor tip banner
QString toastMessage = tr("Banner hidden. <a href='undo'>Undo</a>");
```

```xml
// In .ui files, set "notr" attribute to true for string literals which require no translation
<widget>
  <property name="text">
    <string notr="true">-</string>
  </property>
</widget>
```

### Memory management

- Use Qt's parent-child system for automatic cleanup for Qt classes
- Otherwise, use C++ smart pointers for non-Qt classes

### Binary file formats

Every `QDataStream` that reads or writes a Mudlet file must call `setVersion(QDataStream::Qt_5_12)` before any data crosses it, on both halves of a reader/writer pair - `QFont`'s binary representation changed at Qt 5.13, so that pinned version *is* the on-disk format and changing it breaks file compatibility.

### Include management

Minimize `#include` directives to reduce build times:

**In header files (.h):**
- Only include what's needed for declarations in that header
- Use forward declarations (`class Foo;`) when only pointers or references are used
- Never include headers "just in case" - each include in a header propagates to all files that include it

**In source files (.cpp):**
- Only include headers actually used in that file
- When adding new code, verify you need each include you add
- Don't copy includes from similar files without checking if they're needed

## Key architecture points
Mudlet is single-threaded - all profiles, triggers, and the Lua engine run on the main thread. There are two exceptions: networking, which Qt handles in the background, and `TriggerMatchPool` (`src/TriggerMatchPool.h`), which during a flood of incoming text asks a few helper threads which triggers *cannot* match a line before the main thread walks them. Everything reachable from `TTrigger::prescanMayFire()` must stay a pure function of the trigger and the line - no counters, no logging, no Lua, nothing that writes - because it runs on those threads; the main thread still runs every trigger that fires, in order. Its tuning knobs (`triggerMatchThreads` and friends in `Mudlet.ini`, or the `MUDLET_MATCH_*` environment variables that override them for one run) are documented in `docs/platform-builds.md`.

`Host` (`src/Host.h`) is the per-profile session object: it owns the Lua interpreter, the trigger units and the console. The name reads like "server", but it is the profile.

`src/lua-function-list.json` is autogenerated, do not update it.

## Comments

Comments drift from the code they describe, and a stale one misleads worse than none; every comment also costs the reader's time and an agent's context window. Write one only when the code cannot say it, and keep it to a line or two.

Write a comment for a *why* the code can't carry: a Qt/OS/compiler quirk, a workaround, an ordering or thread-safety constraint, a file-format constraint, or why the obvious alternative is wrong.

Don't write comments that:

- restate what the code or a name already says (`// Save the profile` above `saveProfile()`)
- narrate the change: "previously…", "now…", "no longer…", "fixes #1234 where…", "moved from X". That history belongs in the commit message and PR description
- label code with banners or section dividers
- document a function whose signature already says everything

```cpp
// Bad: restates the code, and narrates a change nobody reading this file needs
// Previously we cleared the cache here, but that crashed when the profile closed,
// so now we check the pointer first and only clear it if it is still valid
if (mpCache) {
    mpCache->clear();
}

// Good: the one fact the code can't show
// Posted, not sent: the console may be mid-paint when this runs
QCoreApplication::postEvent(mpConsole, event);
```

Translator comments (`//:`), tool directives (`NOLINT`, `clang-format off`) and Lua `---` LDoc blocks are not clutter: never remove them to trim comments, but do update them when the string, line or API they describe changes.

## Tests

Two harnesses: Lua specs in `src/mudlet-lua/tests/*_spec.lua` (busted, run in the self-test profile), and C++ functional tests in `test/functional_tests/` (ctest).

Prefer a spec. A functional test statically links `mudlet_core`, so it costs ~250MB and a link step in every build tree unless it joins a grouped per-subsystem binary, where it costs a compile instead; either way each ctest case runs in its own process. Some subsystems have such a group today - the `*_GROUP_TEST_SOURCES` lists in `test/functional_tests/CMakeLists.txt`, which document how to join one. A spec is ~30KB and needs no rebuild at all because `mudlet-lua` loads from disk. Specs are also shared with Mudlet Web, so writing one grows that platform's coverage for free, which a functional test never does. Write a functional test when a spec genuinely cannot reach the behaviour: private C++ state, a path with no Lua entry point, or something that happens before Lua exists. Sanitiser coverage is not one of those reasons, as the spec run exercises the same instrumented binary.

Both harnesses fail silently rather than red when the setup is wrong, so confirm a new test fails without the fix before trusting it.

## Demo videos

To demonstrate a bug fix or UI change with a screen recording, follow the before & after video workflow in `docs/demo-videos.md` (Linux/Xvfb; records headlessly, then trims and labels the result for attaching to a PR).

## Build system notes

- **Before running any `cmake`, `ninja` or `make` command, read the `build-mudlet` skill in `.agents/skills/build-mudlet/SKILL.md`.** It carries the per-platform invocation and the parallelism pitfalls; an unbounded build command can saturate the machine's memory.
- **Build system**: CMake (handles platform-specific configurations). See https://wiki.mudlet.org/w/Compiling_Mudlet for instructions.
- Check code quality with clang-tidy using `.clang-tidy` configuration file
- Allow up to 10mins for a build - it can take a while
- Platform specifics and compile-time debugging defines: see `docs/platform-builds.md`

### Code formatting

After editing any C++ files (`.cpp`, `.h`), run clang-format before committing:

```bash
clang-format -i path/to/edited/file.cpp path/to/edited/file.h
```

On macOS, use the Homebrew-installed LLVM version to ensure compatibility:
```bash
$(brew --prefix llvm)/bin/clang-format -i path/to/edited/file.cpp path/to/edited/file.h
```

The project uses the `.clang-format` configuration in the repo root. This ensures consistent code style across the codebase.

### Static analysis

For complete setup instructions on how to run static analysis during a build, see: https://wiki.mudlet.org/w/Compiling_Mudlet#Static_analysis

### Git

Do not force-push to remote branches.

#### Commit trailers for AI-assisted work

When you (an AI assistant) help produce a commit, the commit message MUST include:

- `Assisted-by: AGENT_NAME:MODEL_VERSION` - identifies the AI tool and model. Use the actual model ID you are running as (e.g. `Assisted-by: Claude:claude-4.6-opus`).
- `Signed-off-by: Full Name <email>` - the human submitter's DCO sign-off. Do NOT fabricate or auto-add this on the human's behalf, and do NOT add a `Signed-off-by` for the AI itself.

Before the human signs off, they must have built and manually tested the change to confirm it works. Mudlet developers should not be the first to test AI-generated code. Ask the human to confirm they've tested the PR, and to provide the name and email to use for sign-off; only then add the `Signed-off-by` trailer.

Both trailers go at the end of the commit message. Apply this to every AI-assisted commit, not just the first. See the "AI Coding Assistants" section in `docs/CONTRIBUTING.md` for the full policy.
