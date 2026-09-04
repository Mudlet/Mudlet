# Recording before & after demo videos

Record a before & after demo video of a Mudlet bug fix or UI change:
headless recording on Linux - under Xvfb by default, or in a headless GNOME
session when the behaviour is Wayland-specific - then trimming and labelling
the result for attaching to a pull request or issue.

## When to use

Use this skill to produce visual evidence that a change works: a short video
showing the broken behaviour ("before") followed by the fixed behaviour
("after"). These videos are attached to pull requests and issues so reviewers
and reporters can see the fix without building the branch themselves.

## Platform support

This workflow is currently **Linux only**. Additions documenting an equivalent
macOS or Windows workflow are very welcome - please extend this file.

There are two recording backends, and picking one is not a question of which
display server your own desktop runs:

- **X11 (Xvfb)** - the default; use it for everything. Xvfb is its own X
  server, so it works just as well while you are logged into a Wayland
  desktop, provided both toolkits are pointed at X11 (see the prerequisites).
- **Wayland (headless gnome-shell)** - only when the behaviour under test is
  itself Wayland-specific: window geometry, client-side decorations,
  fractional scaling, or anything else that goes through Qt's `wayland`
  platform plugin, where a recording made under Xvfb would prove nothing. It
  is a heavier rig with more moving parts.

## Prerequisites

- X11 backend packages: `xvfb`, `openbox`, `xdotool`, `imagemagick` (for
  `import`), `ffmpeg`.
- Wayland backend packages: `gnome-shell`, `dbus-x11` (for `dbus-launch`),
  `python3-gi`, `gstreamer1.0-tools`, `gstreamer1.0-pipewire`,
  `gstreamer1.0-plugins-good`, `gstreamer1.0-plugins-ugly` (for `x264enc`),
  `ffmpeg`.
- Two Mudlet binaries: a **before** build (the base branch, e.g. the
  merge-base with `development`) and an **after** build (your branch). Git
  worktrees are the easiest way to keep both build trees alive at once. When
  the change is toggled by a setting rather than code, one binary is enough.

Recording under Xvfb **from a Wayland desktop** additionally needs
`QT_QPA_PLATFORM=xcb` and `GDK_BACKEND=x11` (the reference script sets both on
Mudlet). Xvfb is an X server, and neither toolkit targets it by itself there:
Qt takes the wayland plugin over `xvfb-run`'s `DISPLAY`, and the GTK3 platform
theme Qt loads under GNOME calls `gtk_init()`, which exits the process outright
when it cannot open a display.

## Workflow overview

1. **Plan the money shots first.** Decide exactly what on-screen evidence
   proves the bug and the fix, and script the fewest steps that produce it.
2. **Record** both sessions on a virtual display with one continuous
   capture (backend script below). Take a screenshot after every step; the script's
   `mark` helper prints each step's offset into the video - keep that output,
   it is your trim timeline.
3. **Verify the screenshots** before moving on. If a step missed (wrong
   coordinates, dialog in the way), fix and re-record - don't try to salvage
   a broken take in editing.
4. **Trim and label** (post-production section below).
5. **Verify the final video** by extracting frames, then attach it to the PR.

## Recording on X11 (Xvfb)

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

# also fires on interruption, so a killed run doesn't leave a stale Xvfb
# behind to block the next one
cleanup() {
    kill "${MUDLET_PID:-}" 2>/dev/null
    if kill -INT "${FFMPEG_PID:-}" 2>/dev/null; then sleep 3; fi
    kill "${OPENBOX_PID:-}" "${XVFB_PID:-}" 2>/dev/null
}
trap cleanup EXIT
trap 'exit 1' INT TERM   # route interrupts through the EXIT trap

Xvfb "$DPY" -screen 0 "${SIZE}x24" &
XVFB_PID=$!
sleep 2
openbox &          # window decorations look better and title bars double as evidence
OPENBOX_PID=$!
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
    # GDK_BACKEND matters on a Wayland desktop: Qt's GTK3 platform theme calls
    # gtk_init(), which kills the process outright if GTK looks for wayland
    HOME=$home QT_QPA_PLATFORM=xcb GDK_BACKEND=x11 "$1" --profile "Mudlet Tutorial" &
    MUDLET_PID=$!    # global so the EXIT trap can reach it
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

    # When scripting a mouse drag as many small `xdotool mousemove --sync` steps,
    # skip any waypoint whose rounded coordinates equal the previous one. --sync
    # does not wait for arrival - it polls for the pointer to move away from where
    # it was, so a no-op warp silently burns the full retry budget (500 x 30 ms =
    # 15 s of frozen video per duplicate). Eased paths (smoothstep etc.) rounded
    # to integers produce exactly such duplicates at both ends of short drags.

    kill $MUDLET_PID 2>/dev/null
    sleep 2
}

session "$BEFORE_BIN" before ''
session "$AFTER_BIN" after ''
# For a setting-toggled change, pass the same binary twice and vary the INI instead:
#   session "$BIN" before ''
#   session "$BIN" after 'appearance=2'

kill -INT $FFMPEG_PID 2>/dev/null
sleep 3             # let ffmpeg flush; SIGTERM/SIGKILL truncates the file
FFMPEG_PID=""
ffmpeg -y -loglevel error -i "$OUT/raw.mkv" -c copy "$OUT/raw.mp4"
echo "recording: $OUT/raw.mp4  screenshots: $OUT/shots/"
# the EXIT trap shuts down openbox and Xvfb
```

## Recording on Wayland (headless gnome-shell)

Xvfb has no Wayland counterpart, because a Wayland compositor *is* the display
server: the compositor itself has to run headless, and neither `xdotool` nor
ffmpeg's `x11grab` can reach it once it does. GNOME covers all three needs -
`gnome-shell --headless` provides the virtual monitor,
`org.gnome.Mutter.RemoteDesktop` injects keyboard and pointer events, and
`org.gnome.Mutter.ScreenCast` publishes the monitor as a PipeWire node that
GStreamer records. It needs no root and no seat of its own, so it runs
alongside your real session. Verified on GNOME 46.

That is two files: a driver holding the D-Bus sessions open and taking one
command per line on a fifo, and a recording script mirroring the X11 one above.

```python
#!/usr/bin/python3
"""Drive a headless gnome-shell: screencast out, keyboard and pointer in.

  wayland-drive.py <fifo> <node-file>

Publishes the virtual monitor as a PipeWire node (id written to <node-file>,
for gst-launch to read) and then executes one command per line read from
<fifo>, until "quit":

  type lua print("demo")     click [left|right]
  key Return                 move 960 540
  combo Super Up             quit

Needs python3-gi, and DBUS_SESSION_BUS_ADDRESS pointing at the bus the
headless shell was started on.
"""
import sys
import time
from gi.repository import Gio, GLib

MUTTER_RD = "org.gnome.Mutter.RemoteDesktop"
MUTTER_SC = "org.gnome.Mutter.ScreenCast"
bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)


def call(dest, path, iface, method, params=None, reply=None):
    return bus.call_sync(dest, path, iface, method, params, reply,
                         Gio.DBusCallFlags.NONE, 10000, None)


def rd(session, method, params=None):
    return call(MUTTER_RD, session, MUTTER_RD + ".Session", method, params)


# evdev keycodes (linux/input-event-codes.h), laid out by keyboard row - Mutter
# takes those, not X keysyms
KEYCODE = {" ": 57, "\\": 43}
for base, chars in ((2, "1234567890-="), (16, "qwertyuiop[]"),
                    (30, "asdfghjkl;'`"), (44, "zxcvbnm,./")):
    KEYCODE.update({c: base + i for i, c in enumerate(chars)})
SHIFTED = dict(zip("!@#$%^&*()_+{}:\"~<>?|", "1234567890-=[];'`,./\\"))
SHIFTED.update({c.upper(): c for c in "abcdefghijklmnopqrstuvwxyz"})
NAMED = {"Return": 28, "Tab": 15, "Escape": 1, "BackSpace": 14, "Delete": 111,
         "Up": 103, "Down": 108, "Left": 105, "Right": 106, "Home": 102,
         "End": 107, "Shift": 42, "Ctrl": 29, "Alt": 56, "Super": 125}
BUTTON = {"left": 0x110, "middle": 0x112, "right": 0x111}


def wait_for_shell(seconds=60):
    """The wayland socket appears well before Mutter owns its D-Bus names."""
    for _ in range(seconds * 2):
        owned = bus.call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus",
                              "org.freedesktop.DBus", "NameHasOwner",
                              GLib.Variant("(s)", (MUTTER_RD,)),
                              GLib.VariantType("(b)"), Gio.DBusCallFlags.NONE,
                              10000, None).unpack()[0]
        if owned:
            return
        time.sleep(0.5)
    sys.exit(f"{MUTTER_RD} never appeared - is the headless shell running?")


def setup(node_file):
    """Associated remote-desktop + screencast sessions on one virtual monitor."""
    wait_for_shell()
    session = call(MUTTER_RD, "/org/gnome/Mutter/RemoteDesktop", MUTTER_RD,
                   "CreateSession", None, GLib.VariantType("(o)")).unpack()[0]
    # the pointer needs a stream to define its coordinate space, and a stream
    # only counts as this session's when the screencast is created against its id
    session_id = bus.call_sync(
        MUTTER_RD, session, "org.freedesktop.DBus.Properties", "Get",
        GLib.Variant("(ss)", (MUTTER_RD + ".Session", "SessionId")),
        GLib.VariantType("(v)"), Gio.DBusCallFlags.NONE, 10000, None).unpack()[0]

    connector = call("org.gnome.Mutter.DisplayConfig", "/org/gnome/Mutter/DisplayConfig",
                     "org.gnome.Mutter.DisplayConfig", "GetCurrentState").unpack()[1][0][0][0]
    # is-recording is what gnome-shell's own recorder passes for a capture that
    # goes to a file rather than to a live remote viewer
    cast = call(MUTTER_SC, "/org/gnome/Mutter/ScreenCast", MUTTER_SC, "CreateSession",
                GLib.Variant("(a{sv})", ({"remote-desktop-session-id":
                                          GLib.Variant("s", session_id),
                                          "is-recording": GLib.Variant("b", True)},)),
                GLib.VariantType("(o)")).unpack()[0]
    stream = call(MUTTER_SC, cast, MUTTER_SC + ".Session", "RecordMonitor",
                  GLib.Variant("(sa{sv})", (connector, {"cursor-mode": GLib.Variant("u", 1)})),
                  GLib.VariantType("(o)")).unpack()[0]

    loop, node = GLib.MainLoop(), {}

    def on_added(conn, sender, path, iface, signal, params):
        node["id"] = params.unpack()[0]
        loop.quit()

    bus.signal_subscribe(None, MUTTER_SC + ".Stream", "PipeWireStreamAdded",
                         stream, None, Gio.DBusSignalFlags.NONE, on_added)
    rd(session, "Start")    # an associated screencast starts with its owner,
    GLib.timeout_add_seconds(15, loop.quit)   # ScreenCast.Session.Start refuses
    loop.run()
    if "id" not in node:
        sys.exit("no PipeWireStreamAdded - screencast did not start")

    with open(node_file, "w") as f:
        f.write(f"{node['id']}\n")
    print(f"monitor {connector}, pipewire node {node['id']}", flush=True)
    return session, stream


def keycode(session, code, pressed):
    rd(session, "NotifyKeyboardKeycode", GLib.Variant("(ub)", (code, pressed)))
    time.sleep(0.02)


def tap(session, code, shift=False):
    if shift:
        keycode(session, NAMED["Shift"], True)
    keycode(session, code, True)
    keycode(session, code, False)
    if shift:
        keycode(session, NAMED["Shift"], False)


def run(session, stream, line):
    verb, _, rest = line.partition(" ")
    if verb == "type":
        for ch in rest:
            base = SHIFTED.get(ch)
            code = KEYCODE.get(base or ch)
            if code is None:
                sys.exit(f"no keycode for {ch!r} - extend the table")
            tap(session, code, shift=base is not None)
    elif verb in ("key", "combo"):
        codes = [NAMED[n] for n in rest.split()]
        if verb == "key":
            for code in codes:
                tap(session, code)
        else:                                  # hold them all, release in reverse
            for code in codes:
                keycode(session, code, True)
            for code in reversed(codes):
                keycode(session, code, False)
    elif verb == "move":
        x, y = rest.split()
        rd(session, "NotifyPointerMotionAbsolute",
           GLib.Variant("(sdd)", (stream, float(x), float(y))))
    elif verb == "click":
        button = BUTTON[rest.strip() or "left"]
        for pressed in (True, False):
            rd(session, "NotifyPointerButton", GLib.Variant("(ib)", (button, pressed)))
            time.sleep(0.05)
    else:
        sys.exit(f"unknown command {line!r}")


if __name__ == "__main__":
    fifo, node_file = sys.argv[1], sys.argv[2]
    session, stream = setup(node_file)
    while True:
        with open(fifo) as f:                  # reopen: each writer's close is an EOF
            for line in f:
                line = line.rstrip("\n")
                if not line:
                    continue
                if line == "quit":
                    sys.exit(0)
                run(session, stream, line)
```

```bash
#!/bin/bash
# Usage: ./record-demo-wayland.sh /path/to/before/mudlet /path/to/after/mudlet
set -u

BEFORE_BIN=$1
AFTER_BIN=$2
OUT=${OUT:-/tmp/mudlet-demo}
WD=${WD:-mudlet-demo}
SIZE=${SIZE:-1920x1080}
DRIVE="$(dirname "$0")/wayland-drive.py"

rm -rf "$OUT" && mkdir -p "$OUT/shots"
mkfifo "$OUT/fifo"

cleanup() {
    kill "${MUDLET_PID:-}" 2>/dev/null
    if kill -INT "${GST_PID:-}" 2>/dev/null; then sleep 3; fi
    kill "${DRIVE_PID:-}" "${GS_PID:-}" 2>/dev/null
    sleep 1
    kill "${DBUS_SESSION_BUS_PID:-}" 2>/dev/null
    rm -f "$XDG_RUNTIME_DIR/$WD" "$XDG_RUNTIME_DIR/$WD.lock"
}
trap cleanup EXIT
trap 'exit 1' INT TERM

# Mutter's remote-desktop and screencast APIs are session-bus names that the
# desktop's own gnome-shell already owns - on a shared bus this would drive the
# real desktop instead of the headless one
eval "$(dbus-launch --sh-syntax)"
export DBUS_SESSION_BUS_ADDRESS DBUS_SESSION_BUS_PID

# a settings store of its own keeps the developer's shell extensions and tweaks
# out of the frame; never export it, Mudlet reads XDG_CONFIG_HOME too
mkdir -p "$OUT/shell-config"
shell_env=(env XDG_CONFIG_HOME="$OUT/shell-config")
"${shell_env[@]}" gsettings set org.gnome.shell disable-user-extensions true
"${shell_env[@]}" gnome-shell --headless --no-x11 --virtual-monitor "$SIZE" \
    --wayland-display "$WD" > "$OUT/gnome-shell.log" 2>&1 &
GS_PID=$!
for _ in $(seq 40); do [ -S "$XDG_RUNTIME_DIR/$WD" ] && break; sleep 1; done
[ -S "$XDG_RUNTIME_DIR/$WD" ] || { echo "gnome-shell did not start"; tail -5 "$OUT/gnome-shell.log"; exit 1; }

export WAYLAND_DISPLAY=$WD
unset DISPLAY       # otherwise Qt connects to the real desktop's X display

"$DRIVE" "$OUT/fifo" "$OUT/node" > "$OUT/drive.log" 2>&1 &
DRIVE_PID=$!
for _ in $(seq 40); do [ -s "$OUT/node" ] && break; sleep 0.5; done
[ -s "$OUT/node" ] || { echo "screencast did not start"; cat "$OUT/drive.log"; exit 1; }
NODE=$(cat "$OUT/node")
drive() { echo "$*" > "$OUT/fifo"; }

gst-launch-1.0 -q pipewiresrc path="$NODE" ! videoconvert ! videorate \
    ! video/x-raw,framerate=12/1 ! x264enc speed-preset=ultrafast tune=zerolatency \
    ! matroskamux ! filesink location="$OUT/raw.mkv" > "$OUT/gst.log" 2>&1 &
GST_PID=$!
T0=$(date +%s)
mark() { echo "[t=$(( $(date +%s) - T0 ))s] $1"; }   # trim timeline
# stills come off the same node - a second consumer is fine while it records
shot() { gst-launch-1.0 -q pipewiresrc path="$NODE" num-buffers=15 ! videoconvert \
             ! pngenc snapshot=true ! filesink location="$OUT/shots/$1.png" 2>>"$OUT/gst.log"; }
sleep 2

session() { # $1 = mudlet binary, $2 = tag ("before"/"after"), $3 = extra Mudlet.ini lines
    local home="$OUT/$2-home"
    mkdir -p "$home/.config/mudlet"
    printf '[General]\nuiTourShown=true\n%s\n' "$3" > "$home/.config/mudlet/Mudlet.ini"
    HOME=$home QT_QPA_PLATFORM=wayland "$1" --profile "Mudlet Tutorial" \
        > "$OUT/$2-mudlet.log" 2>&1 &
    MUDLET_PID=$!
    sleep 25        # profile load

    drive key Shift        # the first key event of a session is swallowed
    sleep 1
    # no xdotool equivalent: on Wayland the window owns its geometry, so ask
    # Mudlet for the size instead of asking a window manager
    drive type 'lua setMainWindowSize(1800, 1000)'
    sleep 1
    drive key Return
    sleep 3
    mark "$2 loaded"; shot "$2-01-loaded"
    sleep 3         # hold the loaded state on camera

    # ---- demo steps go here: drive + mark + a screenshot after each ----
    # drive type 'lua print("demo")'; sleep 1; drive key Return
    # sleep 4       # hold each money shot ~4 s
    # mark "$2 print output"; shot "$2-02-step"
    #
    # Clicks need the pointer to land first - move, sleep 1, then click:
    # drive move 960 540; sleep 1; drive click

    kill $MUDLET_PID 2>/dev/null
    sleep 2
}

session "$BEFORE_BIN" before ''
session "$AFTER_BIN" after ''

drive quit
kill -INT $GST_PID 2>/dev/null
sleep 3
GST_PID=""
ffmpeg -y -loglevel error -i "$OUT/raw.mkv" -c copy "$OUT/raw.mp4"
echo "recording: $OUT/raw.mp4  screenshots: $OUT/shots/"
```

What differs from the X11 rig, all of it learned the hard way:

- **Give the shell a session bus of its own.** Mutter's remote-desktop and
  screencast interfaces are session-bus names your logged-in gnome-shell
  already owns, so on the ambient bus the driver drives your real desktop
  instead of the headless one.
- **`unset DISPLAY` once `WAYLAND_DISPLAY` is set**, or anything that can fall
  back to X11 - Mudlet included - opens its window on your actual screen, and
  the recording captures an empty desktop.
- **Wait for Mutter's D-Bus names, not just the Wayland socket.** The socket
  appears well before `org.gnome.Mutter.RemoteDesktop` is owned; connecting in
  that window fails with `ServiceUnknown`, and the driver then never reads the
  fifo, so the first `drive` command blocks the whole script forever.
- **Start the screencast through the remote-desktop session.** A screencast
  created with `remote-desktop-session-id` refuses
  `ScreenCast.Session.Start` ("Must be started from remote desktop session");
  it starts with its owner's `Start`. The association is not optional either -
  pointer coordinates are expressed relative to a stream of that session.
- **Take stills off the PipeWire node.** `org.gnome.Shell.Screenshot` answers
  "Screenshot is not allowed" on GNOME 46, but a second short `pipewiresrc`
  consumer can grab a PNG while the recorder keeps running.
- **Keycodes are evdev**, from `linux/input-event-codes.h`, not X keysyms -
  hence the table in the driver.
- **The first key event of each Mudlet session is swallowed**, which silently
  eats the `l` of a `lua` command and turns it into an unrecognised one. Send a
  harmless `Shift` after the window settles.
- **Move the pointer, wait ~1 s, then click.** A click issued straight after
  the motion only hovers the widget; the button never registers.
- **Size the window from inside Mudlet**: `lua setMainWindowSize(1800, 1000)`.
  Wayland clients own their geometry, so there is no `xdotool windowsize`
  equivalent to reach in from outside.
- A private `XDG_CONFIG_HOME` plus `disable-user-extensions` keeps *your*
  favourites and tweaks out of frame, but it does **not** remove a distro's
  session-level extensions - Ubuntu's dock and desktop icons still load. A
  full-size Mudlet window covers them.
- One torn frame turned up during a session switch in a long recording and
  never reproduced in controlled clips, so this is a hint rather than a fix: if
  you hit tearing, try `always-copy=true` on `pipewiresrc`, which copies each
  buffer out of PipeWire's pool before the encoder sees it.

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

Recording plumbing (both backends):

- The display server, the recorder, and Mudlet must all live inside one script
  invocation (or a `setsid`-detached process group): background processes
  started from a foreground shell die when that shell call returns in most CI
  and agent sandboxes. The reverse failure exists too - a run killed without
  cleanup leaves a stale Xvfb holding the display, or a stale gnome-shell
  holding the Wayland socket and its dbus-daemon, blocking the next run. Hence
  the EXIT trap in both reference scripts.
- Stop the recorder with SIGINT and give it ~3 seconds to flush -
  SIGTERM/SIGKILL truncates the file. Record to `.mkv` (truncation-safe), then
  remux to `.mp4` with `-c copy`. This applies to `gst-launch-1.0` exactly as
  it does to ffmpeg.
- 12 fps with `-preset ultrafast` is plenty for UI demos and costs almost no
  CPU. Record at 1920x1080 or higher and bump the UI font for legibility -
  high resolution is essentially free offline.
- Recording each session as its own file and concatenating at the end is a
  fine alternative to one continuous capture, and lets you re-shoot one half
  without redoing the other.
