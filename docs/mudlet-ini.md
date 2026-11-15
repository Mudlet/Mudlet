# Mudlet.ini

`Mudlet.ini` holds Mudlet's application-wide settings: everything that is not per profile (that lives in the profile's
XML and `profile.ini`) and not per map (in the map file). This page is for developers - what the file is, how code
reaches it, and every key in it. Players are better served by the wiki's
[Command line options](https://wiki.mudlet.org/w/Command_line_options) page, which explains where the config directory
is.

## Where it is

`<config root>/Mudlet.ini`, in INI format. Keys without a `/` are in the `[General]` section.

The config root is chosen once per run by `resolveConfigRoot()` (`src/MudletApp.cpp`):

1. **`portable.txt`** beside the executable (or, failing that, in the default config directory) wins. Its first line
   names the root, relative to the executable's directory; an empty marker beside the executable means `./portable`.
   Under an AppImage the executable's directory is that of `$APPIMAGE`. An unusable root falls back to step 2 with a
   warning.
2. Otherwise **`~/.config/mudlet`** on every platform (`QDir::homePath()`), or **`$XDG_CONFIG_HOME/mudlet`** when that
   variable holds an absolute path - unless the old directory makes the stronger claim (it has profiles, else a
   `Mudlet.ini`, else it merely exists), in which case Mudlet stays there and logs how to migrate.

The GUI settles the root in `MudletApp::settleConfigRoot()` (from `mudlet::setupConfig()`), a windowless pool in the
`HostManager` constructor, and `mudlet-headless --config-dir <folder>` through `MudletApp::chooseConfigRoot()`.
`--profile` does not move it.

## Reading and writing it

```cpp
if (QSettings* settings = MudletApp::getQSettings()) {
    const bool on = settings->value(qsl("someKey"), true).toBool();
}
```

- **`MudletApp::getQSettings()` returns `nullptr` until the root is settled**, then one shared `QSettings` for the rest
  of the run (rebuilt only if the root changes). Code that can run before that - early startup, a test without a root -
  must handle the null, typically by using the default. A few readers that must run before `QApplication` exists
  (`highDpiScaleFactorRoundingPolicy` in `main.cpp`, the command line's translations in `MudletApp.cpp`) open the
  same file with a `QSettings` of their own.
- **A corrupt or unreadable file** is reported once as an ERROR; every read then returns the caller's default and
  writes are lost.
- **Most main-window keys are written only by `mudlet::writeSettings()`**, which runs when the main window closes, a
  profile is saved or closed, Preferences closes, Lua `saveProfile()` succeeds, and the debug console's filter
  changes. Everything else writes where it changes.
- **"ini only" keys** below have no control: only a hand edit sets them. Some are still written back with their
  current value (`copyAsImageTimeout`, `minLengthForSpellCheck`, `autosaveIntervalMinutes`, the theme download
  keys), so they appear in the file and a hand edit survives; the rest are never written and appear only once
  someone adds them.
- **The crash reporter's policy is not here** - see [Other stores](#other-settings-stores).

### Adding a key

- Read it with an explicit default, so a missing key and an older file behave as before.
- Say when it takes effect. Many keys are read once at startup or cached in a `static`, so a changed value needs a
  restart; if Preferences changes it live, push the new value to whatever cached it.
- An environment variable that overrides the key is the established way for tests and benchmarks to set it per run
  (see [Runtime tuning](platform-builds.md#runtime-tuning-the-trigger-match-pool)).
- Add a row here.

## Keys

"When" is when Mudlet reads the key from the file, so when a hand edit takes effect. A change made through a control
in "Set from" (Preferences, a toolbar, a menu, Lua) normally applies at once and is written to the file for the next
read. Preferences labels are quoted as they appear in English.

### Application and startup

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `interfaceLanguage` | locale code | first system language with a complete translation, else `en_US` | Preferences "Interface language:" | startup | UI translation |
| `firstLaunchDate` | ISO-8601 UTC | none | written once, on the first run with no profiles and an empty file | read once per run | Older than a few months marks an experienced player: no tours or first-run hints |
| `enableMultiViewMode` | bool | false | "MultiView" toolbar button | startup | Show several profiles side by side |
| `storePasswordsSecurely` | bool | true | Preferences "Store character login passwords in:" | startup (migrates the passwords) | OS keychain (true) or plain text in the profile (false) |
| `enableMuteAPI` | bool | false | toolbar mute menu; Lua `setConfig("muteMediaAPI")` | startup | Mute sounds started by scripts |
| `enableMuteGame` | bool | false | toolbar mute menu; Lua `setConfig("muteMediaGame")` | startup | Mute sounds the game triggers (MSP, GMCP) |
| `copyAsImageTimeout` | int, seconds | 3 | ini only | startup | Time limit for "copy as image" |
| `minLengthForSpellCheck` | int, characters | 3 | ini only | startup | Shortest word that is spell-checked |
| `scrollbackTutorialsShown` | int 0-3 | 0 | counter | startup | Times the split-screen hint was shown |
| `characterModeWarningsShown` | int 0-3 | 0 | counter | startup | Times the character-at-a-time warning was shown |
| `compactInputLineTutorialsShown` | int 0-3 | 0 | counter | startup | Times the compact input line hint was shown |
| `uiTourShown` | bool | false | set when the tour runs | first profile opened | The UI tour was offered |
| `whatsNew/<feature>/dismissed` | bool | false | "Got it" on the feature callout (`redesignedSettings` also by opening Preferences) | each time it would show | Callout dismissed; `<feature>` is `redesignedSettings` or `profileTabShortcuts` |
| `whatsNew/<feature>/shownCount` | int | 0 | counter; stops after 5 | each time | Times the callout appeared |
| `telnetHandlerEnabled` | bool | false | Preferences "Mudlet handles telnet:// and telnets:// links"; startup prompt | startup | Register Mudlet for `telnet://` links |
| `telnetHandlerAsked` | bool | absent | startup prompt | startup | The prompt was answered (only its presence is checked) |
| `telnetHandlerDontAsk` | bool | false | "Don't ask again" in the startup prompt | startup | Do not prompt again |
| `highDpiScaleFactorRoundingPolicy` | `round`, `ceil`, `floor`, `roundpreferfloor` or `passthrough` | Mudlet writes `PassThrough` when absent | ini only; `QT_SCALE_FACTOR_ROUNDING_POLICY` overrides | startup, before `QApplication`; not on macOS | Qt's high-DPI scale rounding |
| `mcpServerEnabled` | bool | false | Preferences "Let an AI assistant run Lua in Mudlet" | startup | Start the MCP server (MCP builds only) |
| `mcpServerPort` | int 1-65535 | 11235 | Preferences "Port:" | startup | MCP server port |
| `debugConsole/categories` | `TDebug::Categories` flags | all | Central Debug Console "Show" menu | startup | Categories the debug console shows |

### Appearance and main window

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `appearance` | int: 0 system, 1 light, 2 dark | 0 | Preferences "Appearance" | startup | Light or dark theme |
| `showIconsInMenus` | `Qt::CheckState` | 1 (follow the system) | Preferences "Show icons on menus" | startup | Icons in menus |
| `menuBarAltKeyNavigation` | int: 0 with a screen reader, 1 always, 2 never | 0 | Preferences "Pressing Alt on its own moves focus to the menu bar:" | startup | Alt alone focuses the menu bar |
| `colouredIcons` | bool | false | Preferences "Colored icons" | startup | Colored toolbar icons |
| `mainiconsize` | int, × 8 px | 3 (2 full-screen) | Preferences "Icon size toolbars:" | startup | Toolbar icon size |
| `tefoldericonsize` | int | 3 | Preferences "Icon size in tree views:" | startup | Editor tree icon size |
| `menuBarVisibility` | int: 0 never, 1 until a profile loads, 3 always | 3 | Preferences "Show menu bar:" | startup | Menu bar visibility |
| `toolBarVisibility` | as above | 0; forced to 1 when both bars would be hidden | Preferences "Show main toolbar:"; toolbar context menu | startup | Toolbar visibility |
| `showTabConnectionIndicators` | bool | false | Preferences "Show connection status on tabs"; tab context menu; Lua `setConfig("showTabConnectionIndicators")` | startup | Connection dot on profile tabs |
| `pos`, `size` | `QPoint`, `QSize` | (0,0), 750×550 | remembered | startup | Main window position and size |
| `mainWindowGeometry` | `saveGeometry()` | none | remembered | startup, when maximized or full screen | Main window geometry |
| `maximized`, `fullScreen` | bool | false | remembered | startup | Main window state |
| `showOnlyMonospacedFonts` | bool | false | Preferences "Show only monospaced fonts" | Preferences opens | Font picker filter |

### Editor

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `editorTextOptions` | `QTextOption::Flags` | 0 | Preferences "Show Spaces/Tabs", "Show Line/Paragraphs" | startup | Whitespace markers in the Lua editor |
| `autosaveIntervalMinutes` | int, minutes; 0 or below is off | 2 | ini only | each profile load | Autosave interval of the editor and the map |
| `editorSidebarLabelsShown` | bool | true | editor sidebar chevron | editor opens | Sidebar names or icons only |
| `script_editor_pos`, `script_editor_size` | `QPoint`, `QSize` | on the profile's screen, default size | remembered | editor opens | Editor window |
| `Editor/banner_permanently_hidden/profiles/<profile>/<view>/intro` | bool | false | "hide permanently" on the editor's intro banner | each view switch | Intro banner hidden for one profile and view; `<view>` is the lower-case view name, e.g. `cmtriggerview` |
| `colorSublimeThemesURL` | URL | the Colorsublime themes archive on GitHub | ini only (default written on first use) | Preferences loads themes | Where editor themes are downloaded from |
| `themesUpdatePeriod` | int, ms | 86400000 (a day) | ini only (default written on first use) | as above | How often themes are downloaded again |

### Mapper

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `invertMapZoom` | bool | false | Preferences "Invert zoom direction" | startup | Reverse the wheel's zoom direction |
| `drawUpperLowerLevels` | bool | true | Preferences "Draw rooms on upper and lower levels"; mapper menu; Lua `setConfig("showUpperLowerLevels")` | startup | Show rooms of the levels above and below |
| `reportMapIssuesToConsole` | bool | false | Preferences "report map issues on screen" | startup | Full map audit report on screen |
| `mapCenterSmallAreas` | bool | false | ini only | each profile load | Center an area that fits in the view while following the player |
| `directionalExitLines` | bool | true | ini only | a 2D map is made | An exit line to a room that is not straight ahead leaves in its exit's direction and bends; `false` draws it straight |
| `fgColorDialogMapLabel`, `bgColorDialogMapLabel`, `outlineColorDialogMapLabel` | `QColor` | yellow, translucent blue, yellow | map label dialog | dialog opens | Last colors used for a map label |
| `lastExportAreaImageDirectory` | path | Desktop | "Export area to image" | each export | Last folder for area images |

### Network

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `autoLoginUsernameDelay` | int, ms, 0-60000 | 2000 | ini only | each connection | Wait before auto-login sends the character name |
| `autoLoginPasswordDelay` | int, ms, 0-60000 | 1000 | ini only | each connection | Wait before auto-login sends the password |

### Updater

Builds with the updater, except macOS, whose updater (Sparkle) keeps its own preferences. The `DBLSQD/` prefix is kept
from the updater Mudlet used before.

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `DBLSQD/autoDownload` | bool | true (written when absent) | Preferences "Disable automatic updates" (inverted); update dialog | each update check | Download updates automatically |
| `DBLSQD/skipRelease` | version | empty | update dialog "Skip this version" | each update check | Version the player skipped |
| `DBLSQD/updateFilePath`, `DBLSQD/updateFileVersion` | path, version | empty | written after a download, removed once installed | startup, each check | An installer waiting to run |

### IRC and speech

| Key | Type | Default | Set from | When | Meaning |
| --- | --- | --- | --- | --- | --- |
| `ircMessageBufferLimit` | int, lines | 5000 | IRC command `/MSGLIMIT <n>` | IRC window opens | Lines kept per IRC buffer |
| `SpeechRecognition/selectedModel` | model folder name | empty (best installed model) | ini only | each model lookup | Vosk model to use |

### Remembered dialog state

| Key | Type | Meaning |
| --- | --- | --- |
| `lastFileDialogLocation` | path | One "last folder" shared by Mudlet's file dialogs (replay, map, log folder, packages, modules, profile icon, label image) |
| `profilePreferencesGeometry` | `saveGeometry()` | Preferences window |
| `settingsSidebarLabelsShown` | bool, default true | Preferences sidebar names or icons only |
| `settingsRedesignBannerSeen` | bool | "Settings redesigned" banner dismissed |
| `connectionDialogActiveTab` | int | Last tab of the connection dialog |
| `deletedDefaultMuds` | string list | Predefined games the player removed from the list |
| `composerGeometry` | `saveGeometry()` | Composer window |
| `packageAuthor` | string | Pre-fills the package exporter's author |
| `packageExporterSplitterState`, `packageManagerSplitterState` | `saveState()` | Splitter positions |
| `DetachedWindow/<profile>/geometry`, `DetachedWindow/<profile>/windowState` | `saveGeometry()`, `saveState()` | A detached profile window, by its unsanitized profile name |

### Runtime tuning

Documented with their environment variables in
[platform-builds.md](platform-builds.md#runtime-tuning-the-trigger-match-pool): `triggerMatchThreads`,
`triggerMatchThreshold`, `triggerMatchFloodLines`, `triggerMatchMissesPerMatch`, `triggerMatchSpinMicroseconds`
(all in Preferences), `triggerMatchSampleEvery` and `ingestThread` (ini only).

### Legacy keys

Read only to migrate them, or written for older versions of Mudlet sharing the file.

| Key | What happens to it |
| --- | --- |
| `darkTheme` | Read into `appearance`, then removed |
| `showOnlyMyProfiles` | Read into `connectionDialogActiveTab`, then removed |
| `showMenuBar`, `showToolbar` | Written, never read, for older versions |
| `profiles/<profile>/password`, `profiles/<profile>/login` | Plain-text logins from before profiles had their own password file; read only when that file is empty, `login` then removed |
| `commandSearchSplitterState` | Seeds the per-profile value in `profile.ini` |
| `m*EditorSplitterState`, `mVarEditorSplitterState`, `showAllTriggerControls` | Removed whenever the editor saves its settings |
| `Editor/banner_permanently_hidden/<view>[/intro]` | Global banner keys, removed without migrating |

## Other settings stores

Not `Mudlet.ini`, though easily mistaken for it:

- **Crash reporting** - `autoSendCrashReports` is in `QSettings("Mudlet", "CrashReporter")`, the platform's native
  store (the registry on Windows), shared by `src/crash_reporter/main.cpp` and Preferences. It ignores `portable.txt`
  and the config root.
- **`<profile>/profile.ini`** - per-profile settings such as the command line names (`Host.cpp`).
- **Windows registry** - the `.mpackage` and `telnet://` associations (`main.cpp`).
- **QtKeychain on Android** keeps passwords in a `QSettings` of its own (`CredentialManager.cpp`).
