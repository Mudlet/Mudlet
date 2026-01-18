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

download_sentry_cli() {
    local os="$1"
    local arch="$2"
    local file="$3"
    local url=""

    if [[ "$os" == "Darwin" ]]; then
        if [[ "$arch" == "x86_64" ]]; then
            url="https://github.com/getsentry/sentry-cli/releases/download/2.58.2/sentry-cli-Darwin-x86_64"
        elif [[ "$arch" == "arm64" ]]; then
            url="https://github.com/getsentry/sentry-cli/releases/download/2.58.2/sentry-cli-Darwin-arm64"
        fi
    elif [[ "$os" == "Linux" ]]; then
        if [[ "$arch" == "x86_64" ]]; then
            url="https://github.com/getsentry/sentry-cli/releases/download/2.58.2/sentry-cli-Linux-x86_64"
        fi
    elif [[ "$os" == "MINGW"* || "$os" == "MSYS"* || "$os" == "CYGWIN"* ]]; then
        if [[ "$arch" == "x86_64" ]]; then
            url="https://github.com/getsentry/sentry-cli/releases/download/2.58.2/sentry-cli-Windows-x86_64.exe"
        elif [[ "$arch" == "i686" || "$arch" == "i386" ]]; then
            url="https://github.com/getsentry/sentry-cli/releases/download/2.58.2/sentry-cli-Windows-i686.exe"
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
elif [[ "$OS" == "MINGW"* || "$OS" == "MSYS"* ]]; then
    PDB_FILE="${MUDLET_EXEC%.exe}.pdb"
    [[ -f "$PDB_FILE" ]] && FILES_TO_UPLOAD+=("$PDB_FILE")
fi

for f in "${FILES_TO_UPLOAD[@]}"; do
    echo "Uploading $f to Sentry..."
    ./sentry-cli debug-files upload "$f" --project "mudlet"
done

# Use MSYSTEM variable for MSYS2 detection (consistent with other CI scripts)
# and MSYSTEM_PREFIX for the path (supports MINGW64, CLANG64, UCRT64, etc.)
if [[ -n "$MSYSTEM" && -n "$MSYSTEM_PREFIX" ]]; then
    MINGW_BIN="${MSYSTEM_PREFIX}/bin"

    echo ""
    echo "=== Converting Qt DWARF debug files to PDB for Sentry ==="

    # Download cv2pdb (converts MinGW DWARF to PDB format)
    # Use 64-bit version to handle large debug files like Qt6Core (198MB)
    CV2PDB_URL="https://github.com/rainers/cv2pdb/releases/download/v0.54/cv2pdb-0.54.zip"
    echo "Downloading cv2pdb..."
    curl -sL "$CV2PDB_URL" -o cv2pdb.zip
    unzip -q cv2pdb.zip
    CV2PDB="./cv2pdb64.exe"

    if [[ -d "$MINGW_BIN" && -x "$CV2PDB" ]]; then
        # Use absolute Windows path for PDB files (cv2pdb is native Windows app)
        PDB_DIR="$(pwd)/qt_pdbs"
        mkdir -p "$PDB_DIR"
        WIN_PDB_DIR="$(cygpath -w "$PDB_DIR")"
        PDB_FILES=()

        # Function to convert a DLL to PDB format
        convert_dll_to_pdb() {
            local dll="$1"
            local source_dir="$2"

            if [[ -f "$dll" ]]; then
                local dll_name=$(basename "$dll")
                local base_name="${dll_name%.dll}"
                local debug_file="${source_dir}/${base_name}.debug"
                local pdb_file="${PDB_DIR}/${base_name}.pdb"

                # Check if companion .debug file exists (contains DWARF debug info)
                if [[ -f "$debug_file" ]]; then
                    echo "Converting $dll_name to PDB..."
                    echo "  Debug file: ${base_name}.debug ($(stat -c%s "$debug_file") bytes)"

                    # cv2pdb converts DWARF to PDB format
                    # Usage: cv2pdb -l<debug-file> <dll> [<output-dll>] [<pdb>]
                    # Convert paths to Windows format for native Windows cv2pdb.exe
                    local win_debug_file=$(cygpath -w "$debug_file")
                    local win_dll=$(cygpath -w "$dll")
                    local win_pdb_file="${WIN_PDB_DIR}\\${base_name}.pdb"
                    if "$CV2PDB" "-l${win_debug_file}" "$win_dll" "$win_dll" "$win_pdb_file" 2>"${pdb_file}.err"; then
                        if [[ -f "$pdb_file" ]]; then
                            local pdb_size=$(stat -c%s "$pdb_file")
                            echo "  Generated PDB: ${base_name}.pdb ($pdb_size bytes)"
                            PDB_FILES+=("$pdb_file")
                        else
                            echo "  Warning: PDB file not created"
                        fi
                    else
                        echo "  cv2pdb failed: $(cat "${pdb_file}.err" 2>/dev/null || echo 'unknown error')"
                    fi
                    rm -f "${pdb_file}.err"
                else
                    echo "Skipping $dll_name (no .debug file)"
                fi
            fi
        }

        # Process main Qt6 DLLs from bin directory
        for dll in "$MINGW_BIN"/Qt6*.dll; do
            convert_dll_to_pdb "$dll" "$MINGW_BIN"
        done

        # Process Qt plugin DLLs from share/qt6/plugins subdirectories
        # These plugins can appear in crash stack traces and need debug symbols
        QT_PLUGINS_DIR="${MSYSTEM_PREFIX}/share/qt6/plugins"
        if [[ -d "$QT_PLUGINS_DIR" ]]; then
            echo ""
            echo "=== Converting Qt plugin debug files ==="

            # Plugin directories and their DLLs (based on what Mudlet uses)
            declare -A PLUGIN_DLLS=(
                ["generic"]="qtuiotouchplugin"
                ["iconengines"]="qsvgicon"
                ["imageformats"]="qgif qicns qico qjp2 qjpeg qmng qsvg qtga qtiff qwbmp qwebp"
                ["multimedia"]="ffmpegmediaplugin windowsmediaplugin"
                ["networkinformation"]="qglib qnetworklistmanager"
                ["platforms"]="qwindows"
                ["styles"]="qmodernwindowsstyle"
                ["texttospeech"]="qtexttospeech_mock qtexttospeech_sapi"
                ["tls"]="qcertonlybackend qopensslbackend qschannelbackend"
            )

            for plugin_dir in "${!PLUGIN_DLLS[@]}"; do
                plugin_path="${QT_PLUGINS_DIR}/${plugin_dir}"
                if [[ -d "$plugin_path" ]]; then
                    for plugin_name in ${PLUGIN_DLLS[$plugin_dir]}; do
                        dll="${plugin_path}/${plugin_name}.dll"
                        convert_dll_to_pdb "$dll" "$plugin_path"
                    done
                fi
            done
        fi

        if [[ ${#PDB_FILES[@]} -gt 0 ]]; then
            echo ""
            echo "Uploading ${#PDB_FILES[@]} PDB files to Sentry..."
            ./sentry-cli debug-files upload "${PDB_FILES[@]}" --project "mudlet"
            echo "Qt PDB symbols uploaded successfully"
        else
            echo "No Qt PDB files were generated"
        fi

        rm -rf "$PDB_DIR"
    elif [[ ! -x "$CV2PDB" ]]; then
        echo "Warning: cv2pdb not found at $CV2PDB, skipping Qt debug symbols"
    fi

    strip --strip-debug "$MUDLET_EXEC"
fi

rm -f sentry-cli
