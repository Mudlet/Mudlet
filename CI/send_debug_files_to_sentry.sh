#!/bin/bash
###########################################################################
#   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@gmail.com         #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

set -e

SENTRY_CLI_VERSION="3.6.0"

download_sentry_cli() {
    local os="$1"
    local arch="$2"
    local file="$3"
    local url=""
    local base="https://github.com/getsentry/sentry-cli/releases/download/${SENTRY_CLI_VERSION}"

    if [[ "$os" == "Darwin" ]]; then
        if [[ "$arch" == "x86_64" ]]; then
            url="${base}/sentry-cli-Darwin-x86_64"
        elif [[ "$arch" == "arm64" ]]; then
            url="${base}/sentry-cli-Darwin-arm64"
        fi
    elif [[ "$os" == "Linux" ]]; then
        if [[ "$arch" == "x86_64" ]]; then
            url="${base}/sentry-cli-Linux-x86_64"
        fi
    elif [[ "$os" == "MINGW"* || "$os" == "MSYS"* || "$os" == "CYGWIN"* ]]; then
        if [[ "$arch" == "x86_64" ]]; then
            url="${base}/sentry-cli-Windows-x86_64.exe"
        elif [[ "$arch" == "i686" || "$arch" == "i386" ]]; then
            url="${base}/sentry-cli-Windows-i686.exe"
        fi
    fi

    if [[ -z "$url" ]]; then
        echo "Unsupported OS/ARCH: $os / $arch"
        return 1
    fi

    echo "Downloading $url ..."
    curl -L -o "$file" "$url"

    if [[ "$os" != "MINGW"* && "$os" != "MSYS"* && "$os" != "CYGWIN"* ]]; then
        chmod +x "$file"
    fi
    echo "$file ready"
}


if [ -z "$SENTRY_AUTH_TOKEN" ]; then
    echo "[Sentry_upload_debug_files] Missing environment variable: SENTRY_AUTH_TOKEN. Therefore, the debug file upload to sentry.io is canceled."
    exit 0
fi

if [ -z "$1" ]; then
    echo "Usage: $0 <path-to-mudlet-executable>"
    echo "Please provide the path to the Mudlet executable as an argument."
    exit 1
fi

MUDLET_EXEC="$(realpath "$1")"
if [ ! -f "$MUDLET_EXEC" ]; then
    echo "Error: Mudlet executable not found at $MUDLET_EXEC"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OS="$(uname -s)"
ARCH="$(uname -m)"

download_sentry_cli "$OS" "$ARCH" "sentry-cli"
./sentry-cli login --auth-token "$SENTRY_AUTH_TOKEN"

FILES_TO_UPLOAD=("$MUDLET_EXEC")
if [[ "$OS" == "Linux" ]]; then
    DEBUG_FILE="${MUDLET_EXEC}.debug"
    [[ -f "$DEBUG_FILE" ]] && FILES_TO_UPLOAD+=("$DEBUG_FILE")
elif [[ "$OS" == "Darwin" ]]; then
    DEBUG_FILE="${MUDLET_EXEC}.dSYM"
    [[ -d "$DEBUG_FILE" ]] && FILES_TO_UPLOAD+=("$DEBUG_FILE")
elif [[ "$OS" == "MINGW"* ]]; then
    # The PDB shares its debug-id with the shipped mudlet.exe, so uploading both
    # lets Sentry match crash minidumps and symbolicate them (see WithSentry.cmake).
    # The PDB is expected on every Windows sentry build, so fail loudly if it is
    # missing rather than silently uploading only the exe (which would revert to
    # unsymbolicated crash reports if the --pdb link flag is ever lost).
    PDB_FILE="${MUDLET_EXEC%.exe}.pdb"
    if [[ -f "$PDB_FILE" ]]; then
        FILES_TO_UPLOAD+=("$PDB_FILE")
    else
        echo "error: expected PDB at $PDB_FILE not found - Windows crash reports would be unsymbolicated"
        exit 1
    fi
fi

for f in "${FILES_TO_UPLOAD[@]}"; do
    echo "Uploading $f to Sentry..."
    ./sentry-cli debug-files upload "$f" --project "mudlet"
done

# Qt is bundled into the AppImage and the .app, so its frames land in crash
# reports next to Mudlet's own. Sentry can only name them if it holds the
# libraries: the shipped, stripped ones still carry .dynsym and unwind tables,
# which is enough for function names. The DWARF/dSYM companions add file:line
# on top, and are picked up here whenever Qt's debug information component
# happens to be installed - CI does not install it, because qtbase alone is a
# 946MB download. Windows does the equivalent further down, from its MSYS2
# -debug packages.
QT_FILES=()

add_qt_file() {
    if [[ -e "$1" ]]; then
        QT_FILES+=("$1")
    fi
}

# Qt names its Linux companion "Qt6Core.debug" beside "libQt6Core.so.6", while
# objcopy's own convention is "<file>.debug" - accept either.
add_qt_companions() {
    local lib="$1" dir base stem
    dir="$(dirname "$lib")"
    base="$(basename "$lib")"
    stem="${base#lib}"
    stem="${stem%%.so*}"
    add_qt_file "${dir}/${stem}.debug"
    add_qt_file "${lib}.debug"
    add_qt_file "${lib}.dSYM"
}

if [[ "$OS" == "Linux" || "$OS" == "Darwin" ]]; then
    QT_PREFIX="${2:-}"

    if [[ "$OS" == "Linux" ]]; then
        # ldd resolves the whole transitive closure, so Qt modules pulled in by
        # other Qt modules are covered without walking the graph by hand.
        while read -r lib; do
            add_qt_file "$lib"
            add_qt_companions "$lib"
            if [[ -z "$QT_PREFIX" ]]; then
                QT_PREFIX="$(dirname "$(dirname "$lib")")"
            fi
        done < <(ldd "$MUDLET_EXEC" 2>/dev/null | awk '/libQt6/ && $3 ~ /^\// {print $3}')
    else
        # otool reports @rpath-relative install names, so resolve each framework
        # against the Qt prefix and walk their dependencies breadth-first.
        if [[ -z "$QT_PREFIX" ]] && command -v qmake >/dev/null 2>&1; then
            QT_PREFIX="$(qmake -query QT_INSTALL_PREFIX 2>/dev/null || true)"
        fi
        if [[ -n "$QT_PREFIX" ]]; then
            # macOS ships bash 3.2, which has no associative arrays, so the
            # already-walked frameworks are tracked in a delimited string. Safe
            # because the sed below only ever yields bare "QtFoo" names.
            seen_fw=" "
            pending=("$MUDLET_EXEC")
            while [[ ${#pending[@]} -gt 0 ]]; do
                current="${pending[0]}"
                pending=("${pending[@]:1}")
                while read -r name; do
                    [[ -z "$name" || "$seen_fw" == *" ${name} "* ]] && continue
                    seen_fw="${seen_fw}${name} "
                    fw="${QT_PREFIX}/lib/${name}.framework/Versions/A/${name}"
                    [[ -f "$fw" ]] || continue
                    add_qt_file "$fw"
                    add_qt_companions "$fw"
                    add_qt_file "${QT_PREFIX}/lib/${name}.framework.dSYM"
                    pending+=("$fw")
                done < <(otool -L "$current" 2>/dev/null | sed -n 's|.*/\(Qt[A-Za-z0-9]*\)\.framework/.*|\1|p')
            done
        fi
    fi

    # Plugins are loaded at runtime rather than linked, so the walk above never
    # reaches them, yet platform, imageformat and TLS plugins all show up in
    # stack traces. They are small enough to take wholesale.
    if [[ -n "$QT_PREFIX" && -d "${QT_PREFIX}/plugins" ]]; then
        while IFS= read -r -d '' plugin; do
            add_qt_file "$plugin"
            add_qt_companions "$plugin"
        done < <(find "${QT_PREFIX}/plugins" -type f \( -name '*.so' -o -name '*.dylib' \) -print0)
    fi

    if [[ ${#QT_FILES[@]} -gt 0 ]]; then
        echo "Uploading ${#QT_FILES[@]} Qt debug information files to Sentry..."
        ./sentry-cli debug-files upload "${QT_FILES[@]}" --project "mudlet"
    else
        # Not fatal - Mudlet's own frames still symbolicate - but every Qt frame
        # in every Linux and macOS crash report goes unnamed, so say so loudly.
        echo "::warning::no Qt libraries found to upload (QT_PREFIX='${QT_PREFIX}') - Qt frames in crash reports will be unsymbolicated"
    fi
fi

# Qt ships its debug info as separate DWARF ".debug" companions; sentry-cli 3.5.0+
# parses these, so upload them alongside the DLLs they belong to.
# See https://github.com/getsentry/sentry/issues/104738
if [[ -n "$MSYSTEM" && -n "$MSYSTEM_PREFIX" ]]; then
    MINGW_BIN="${MSYSTEM_PREFIX}/bin"
    QT_PLUGINS_DIR="${MSYSTEM_PREFIX}/share/qt6/plugins"

    echo ""
    echo "=== Collecting Qt debug files for Sentry ==="

    DEBUG_FILES=()

    # Upload files only for the Qt6 modules mudlet.exe actually depends on (walk its
    # import table), not every Qt6 DLL in the bin.
    if command -v objdump >/dev/null 2>&1; then
        declare -A seen_dll=()
        pending=("$MUDLET_EXEC")
        # Runtime-loaded Qt plugins can pull in Qt modules that mudlet.exe does not
        # link directly (e.g. the svg imageformat/iconengine plugins pull in Qt6Svg,
        # which is not in our components list). Seed the walk with the plugin DLLs too
        # so their imports get their .debug companions collected as well.
        if [[ -d "$QT_PLUGINS_DIR" ]]; then
            while IFS= read -r -d '' plugin_dll; do
                pending+=("$plugin_dll")
            done < <(find "$QT_PLUGINS_DIR" -type f -name '*.dll' -print0)
        fi
        while [[ ${#pending[@]} -gt 0 ]]; do
            current="${pending[0]}"
            pending=("${pending[@]:1}")
            while IFS= read -r dll; do
                [[ -z "$dll" || -n "${seen_dll[$dll]:-}" ]] && continue
                seen_dll[$dll]=1
                dll_path="$MINGW_BIN/$dll"
                [[ -f "$dll_path" ]] || continue
                pending+=("$dll_path")
                if [[ "$dll" == Qt6*.dll ]]; then
                    # The DLL itself is what ties a crash back to these symbols. crashpad
                    # throws away the CodeView record lld writes - at 25 bytes it is three
                    # short of the struct size crashpad insists on - so minidumps carry no
                    # debug id for any Qt module, only a code id (link timestamp plus image
                    # size). Sentry can turn that code id into the debug id the companion
                    # was uploaded under, but only if it also holds the shipped DLL.
                    DEBUG_FILES+=("$dll_path")
                    # companion may be "<name>.dll.debug" or "<name>.debug"; add once
                    for debug_file in "$MINGW_BIN/${dll}.debug" "$MINGW_BIN/${dll%.dll}.debug"; do
                        [[ -f "$debug_file" && -z "${seen_dll[$debug_file]:-}" ]] && { seen_dll[$debug_file]=1; DEBUG_FILES+=("$debug_file"); }
                    done
                fi
            done < <(objdump -p "$current" 2>/dev/null | sed -n 's/^[[:space:]]*DLL Name:[[:space:]]*//p')
        done
    else
        echo "objdump not found - falling back to uploading all Qt6 debug files"
        for debug_file in "$MINGW_BIN"/Qt6*.debug "$MINGW_BIN"/Qt6*.dll; do
            [[ -f "$debug_file" ]] && DEBUG_FILES+=("$debug_file")
        done
    fi

    # Qt plugins (image formats, platforms, tls, ...) can appear in crash stack
    # traces too, so upload every plugin and companion we can find
    if [[ -d "$QT_PLUGINS_DIR" ]]; then
        while IFS= read -r -d '' debug_file; do
            DEBUG_FILES+=("$debug_file")
        done < <(find "$QT_PLUGINS_DIR" -type f \( -name '*.debug' -o -name '*.dll' \) -print0)
    fi

    if [[ ${#DEBUG_FILES[@]} -gt 0 ]]; then
        echo "Uploading ${#DEBUG_FILES[@]} Qt debug files to Sentry..."
        ./sentry-cli debug-files upload "${DEBUG_FILES[@]}" --project "mudlet"
        echo "Qt debug symbols uploaded successfully"
    else
        echo "No Qt debug files found - are the qt6-*-debug packages installed?"
    fi
fi

rm -f sentry-cli
