---
name: before-after-videos
description: >-
  Record a before & after demo video of a Mudlet bug fix or UI change:
  headless recording on Linux with Xvfb/xdotool/ffmpeg, then trimming and
  labelling the result for attaching to a pull request or issue.
argument-hint: Optional issue/PR number and what to demonstrate
user-invocable: true
---

## When to use

Use this skill to produce visual evidence that a change works: a short video
showing the broken behaviour ("before") followed by the fixed behaviour
("after"). These videos are attached to pull requests and issues so reviewers
and reporters can see the fix without building the branch themselves.

## Platform support

This workflow is currently **Linux only** (Xvfb, xdotool, ImageMagick,
openbox). Additions documenting an equivalent macOS or Windows workflow are
very welcome - please extend this file.

## Prerequisites

- Packages: `xvfb`, `openbox`, `xdotool`, `imagemagick` (for `import`),
  `ffmpeg`.
- Two Mudlet binaries: a **before** build (the base branch, e.g. the
  merge-base with `development`) and an **after** build (your branch). Git
  worktrees are the easiest way to keep both build trees alive at once. When
  the change is toggled by a setting rather than code, one binary is enough.

## Workflow overview

1. **Plan the money shots first.** Decide exactly what on-screen evidence
   proves the bug and the fix, and script the fewest steps that produce it.
2. **Record** both sessions on a virtual display with one continuous ffmpeg
   capture (script below). Take a screenshot after every step; the script's
   `mark` helper prints each step's offset into the video - keep that output,
   it is your trim timeline.
3. **Verify the screenshots** before moving on. If a step missed (wrong
   coordinates, dialog in the way), fix and re-record - don't try to salvage
   a broken take in editing.
4. **Trim and label** (post-production section below).
5. **Verify the final video** by extracting frames, then attach it to the PR.

## Recording script

Adapt this reference script; the structure (everything in one script
invocation, fresh `HOME` per session, screenshot per step) matters more than
the specific steps.

```bash
#!/bin/bash
# Usage: ./record-demo.sh /path/to/before/mudlet /path/to/after/mudlet
set -u

BEFORE_BIN=$1
AFTER_BIN=$2
OUT=${OUT:-/tmp/mudlet-demo}
DPY=${DPY:-:78}
SIZE=1920x1080

rm -rf "$OUT" && mkdir -p "$OUT/shots"
export DISPLAY=$DPY

Xvfb "$DPY" -screen 0 "${SIZE}x24" &
XVFB_PID=$!
sleep 2
openbox &          # window decorations look better and title bars double as evidence
sleep 1
ffmpeg -y -loglevel error -f x11grab -video_size "$SIZE" -framerate 12 -i "$DPY" \
       -c:v libx264 -preset ultrafast -pix_fmt yuv420p "$OUT/raw.mkv" &
FFMPEG_PID=$!
T0=$(date +%s)      # video timestamps count from here
mark() { echo "[t=$(( $(date +%s) - T0 ))s] $1"; }   # trim timeline
sleep 1

session() { # $1 = mudlet binary, $2 = tag ("before"/"after"), $3 = extra Mudlet.ini lines
    local home="$OUT/$2-home"
    mkdir -p "$home/.config/mudlet"
    # uiTourShown skips the first-run UI tour; $3 carries per-session settings,
    # e.g. appearance=2 for dark mode (themes the app chrome, not the console)
    printf '[General]\nuiTourShown=true\n%s\n' "$3" > "$home/.config/mudlet/Mudlet.ini"
    HOME=$home QT_QPA_PLATFORM=xcb "$1" --profile "Mudlet Tutorial" &
    local mudlet_pid=$!
    # --profile opens a small window; normalize the geometry as soon as the
    # window exists so the profile finishes loading at full size on camera
    # (1040 leaves room for the openbox titlebar). head -1 is fine at startup -
    # re-query if the demo opens more Mudlet windows later.
    local wid=""
    for _ in $(seq 30); do
        wid=$(xdotool search --onlyvisible --class -- mudlet | head -1)
        [ -n "$wid" ] && break
        sleep 1
    done
    xdotool windowmove "$wid" 0 0 windowsize "$wid" 1920 1040
    sleep 10        # profile load
    mark "$2 loaded"
    import -window root "$OUT/shots/$2-01-loaded.png"
    sleep 3         # hold the loaded state on camera

    # ---- demo steps go here: xdotool + mark + a screenshot after each ----
    # The tutorial profile ships the `lua` alias, so Lua runs from the command line:
    # xdotool type 'lua print("demo")'; xdotool key Return
    # sleep 4       # hold each money shot ~4 s: the raw hold caps what the edit can keep
    # mark "$2 print output"; import -window root "$OUT/shots/$2-02-step.png"

    kill $mudlet_pid 2>/dev/null
    sleep 2
}

session "$BEFORE_BIN" before ''
session "$AFTER_BIN" after ''
# For a setting-toggled change, pass the same binary twice and vary the INI instead:
#   session "$BIN" before ''
#   session "$BIN" after 'appearance=2'

kill -INT $FFMPEG_PID 2>/dev/null
sleep 3             # let ffmpeg flush; SIGTERM/SIGKILL truncates the file
kill $XVFB_PID 2>/dev/null
ffmpeg -y -loglevel error -i "$OUT/raw.mkv" -c copy "$OUT/raw.mp4"
echo "recording: $OUT/raw.mp4  screenshots: $OUT/shots/"
```

## Post-production

The raw capture is mostly dead time (launch waits, typing, pauses).
Aggressive trimming is mandatory, not optional:

- Cut launch/load waits entirely and never show typing character by
  character - each configuration step becomes a 3-6 second glimpse of its
  finished state.
- Cut redundant repeats, static holds between clicks, and scrollback tours;
  one concise pass per money shot, held 3-4 seconds.
- Keep it as short as the evidence allows - a simple demo can be well under
  a minute. Cap it at roughly 1-2 minutes unless the content genuinely
  demands more.
- Do **not** use ffmpeg's `mpdecimate` to "auto-trim" - it also collapses the
  deliberate hold-and-read moments. Cut explicit timestamp ranges instead.

Cut each interesting range into its own segment and burn its caption in the
same pass, then concatenate. Two things about ffmpeg that will bite you
otherwise: `-ss` must come **after** `-i` (before it, seeking snaps to
keyframes and can land ~20 s away), and a subclip's `t` does not restart at
zero, so timed `enable='between(t,...)'` caption windows misfire - use one
unconditional caption per segment instead.

```bash
# BEFORE segment with a slim banner across the top edge
ffmpeg -y -i raw.mp4 -ss 18 -to 42 -vf "\
drawtext=text='BEFORE - issue #1234':fontsize=30:fontcolor=white:\
box=1:boxcolor=0xB00020@0.85:boxborderw=10:x=(w-text_w)/2:y=8" \
  -c:v libx264 -preset fast -pix_fmt yuv420p seg1-before.mp4

# AFTER segment: same, with boxcolor=0x1B7F3B@0.85 and the AFTER text

printf "file 'seg1-before.mp4'\nfile 'seg2-after.mp4'\n" > list.txt
ffmpeg -y -f concat -safe 0 -i list.txt -c copy demo.mp4
```

If `drawtext` fails with "Cannot find a valid font", append
`:fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf` (path varies by
distro).

Label rules (learned from review feedback):

- Burn labels in with `drawtext` - don't type banner text into Mudlet's
  command line (wastes runtime, gets wiped by console clears, pollutes the
  buffer).
- Keep banners **compact**: a slim strip at the top edge for BEFORE/AFTER,
  small captions (fontsize ~24) in the lower third for individual steps,
  positioned away from the console output. Never use full-screen-centre
  overlays that cover or out-shout the evidence.
- Evidence first: a caption must never appear before the output it describes.
- A top-edge banner covers the window title bar. When the title bar itself is
  evidence (window title, version string), drop that segment's banner to just
  below the menu bar (e.g. `y=70`) instead.

## Verify before delivering

Extract frames and check every money shot survived the trim:

```bash
ffmpeg -i demo.mp4 -vf fps=1 frames/%03d.png
```

Frames that land on the black transition between the two sessions look
empty - check the neighbouring frames before concluding the trim is broken.
Keep the untrimmed raw recording alongside locally in case a re-edit is
needed. Attach the final `.mp4` to the PR or issue (GitHub accepts mp4
uploads in the comment box; at these settings the file is roughly 1 MB per
minute).

## Gotchas

Driving the UI deterministically:

- Never click game tiles in the connection dialog by coordinates - the list
  is shuffled every run. `--profile "Mudlet Tutorial"` skips the dialog
  entirely; the keyboard path is: click into the list, `Home` selects the
  tutorial (pinned first), `Down` reaches regular games, `Return` connects.
- Recent development builds show a "Welcome to Mudlet!" starter dialog before
  the games list even with `uiTourShown=true` - dismiss it via "Skip - show
  me the games list" first. The `--profile` path skips it entirely.
- `--profile` pointed at a hand-made bare profile directory does not install
  the default packages, so the `lua` alias is missing. Use the tutorial
  profile, or create the profile once through the dialog.
- The tutorial profile paints its "Welcome Adventurer!" mini-quest box into
  the main console on load. Fine for most demos, but when the demo needs a
  pristine console, create a plain profile through the dialog instead.
- Don't trust hardcoded coordinates, including any in this document - UI
  positions shift with version, theme, and window size. Screenshot first,
  look, then click.
- Alt+letter menu shortcuts get swallowed by the command line - open menus by
  clicking the menu bar.
- After submitting a command, the command line keeps it selected, so simply
  typing the next command replaces it. Never click into the command line
  between commands - the click drops the cursor mid-text and the next command
  interleaves into garbage.
- Lua runtime errors do not echo to the main console by default - to show a
  failure on screen, use `print(pcall(...))` rather than letting the call
  error silently.
- Visit at most one modal dialog (e.g. Preferences) per Mudlet session and
  relaunch for the next check - input focus after Escape-closing a modal is
  unreliable under a bare Xvfb setup.
- A fresh `HOME` per session resurrects every first-run behaviour (tours,
  toasts, badges). That is often the point of the demo, but seed the ones you
  don't want off via `Mudlet.ini` before launch.

Recording plumbing:

- Xvfb, ffmpeg, and Mudlet must all live inside one script invocation (or a
  `setsid`-detached process group): background processes started from a
  foreground shell die when that shell call returns in most CI and agent
  sandboxes.
- Stop ffmpeg with SIGINT and give it ~3 seconds to flush - SIGTERM/SIGKILL
  truncates the file. Record to `.mkv` (truncation-safe), then remux to
  `.mp4` with `-c copy`.
- 12 fps with `-preset ultrafast` is plenty for UI demos and costs almost no
  CPU. Record at 1920x1080 or higher and bump the UI font for legibility -
  high resolution is essentially free offline.
- Recording each session as its own file and concatenating at the end is a
  fine alternative to one continuous capture, and lets you re-shoot one half
  without redoing the other.
