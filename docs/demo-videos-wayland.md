# Recording demo videos on Wayland

`docs/demo-videos.md` is the demo video workflow; this file only replaces its
recording step, for the rare change where recording under Xvfb would prove
nothing - window geometry, client-side decorations, fractional scaling, or
anything else that goes through Qt's `wayland` platform plugin. Everything
else there, from planning the money shots to trimming and labelling, applies
unchanged.

Reach for this last. Xvfb is the default and works fine from a Wayland desktop
(see that file's prerequisites), and a developer who has hit a Wayland-specific
bug is already sitting in a Wayland session, where GNOME's own screen recorder
is a far shorter path. This rig is for driving Mudlet unattended on a Wayland
machine, where taking over the real session is not an option.

Xvfb has no Wayland counterpart, because a Wayland compositor *is* the display
server: the compositor itself has to run headless, and neither `xdotool` nor
ffmpeg's `x11grab` can reach it once it does. GNOME covers all three needs -
`gnome-shell --headless` provides the virtual monitor,
`org.gnome.Mutter.RemoteDesktop` injects keyboard and pointer events, and
`org.gnome.Mutter.ScreenCast` publishes the monitor as a PipeWire node that
GStreamer records. It needs no root and no seat of its own, so it runs
alongside your real session. Verified on GNOME 46.

Packages: `gnome-shell`, `dbus-x11` (for `dbus-launch`), `python3-gi`,
`gstreamer1.0-tools`, `gstreamer1.0-pipewire`, `gstreamer1.0-plugins-good`,
`gstreamer1.0-plugins-ugly` (for `x264enc`), `ffmpeg`.

That is two files: a driver holding the D-Bus sessions open and taking one
command per line on a fifo, and a recording script mirroring the X11 one.

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

The plumbing gotchas in `docs/demo-videos.md` apply here too, with the obvious
substitutions: keep everything inside one script invocation or a
`setsid`-detached process group (a killed run otherwise leaves a gnome-shell
holding the Wayland socket, plus its dbus-daemon, and the next run cannot
start), and stop `gst-launch-1.0` with SIGINT and ~3 seconds to flush rather
than SIGTERM, which truncates the file.
