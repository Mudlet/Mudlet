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
    CV2PDB_URL="https://github.com/rainers/cv2pdb/releases/download/v0.54/cv2pdb-0.54.zip"
    echo "Downloading cv2pdb..."
    curl -sL "$CV2PDB_URL" -o cv2pdb.zip
    unzip -q cv2pdb.zip
    CV2PDB="./cv2pdb.exe"

    if [[ -d "$MINGW_BIN" && -x "$CV2PDB" ]]; then
        # Create temporary directory for PDB files
        PDB_DIR=$(mktemp -d)
        PDB_FILES=()

        for dll in "$MINGW_BIN"/Qt6*.dll; do
            if [[ -f "$dll" ]]; then
                dll_name=$(basename "$dll")
                base_name="${dll_name%.dll}"
                debug_file="${MINGW_BIN}/${base_name}.debug"
                pdb_file="${PDB_DIR}/${base_name}.pdb"

                # Check if companion .debug file exists (contains DWARF debug info)
                if [[ -f "$debug_file" ]]; then
                    echo "Converting $dll_name to PDB..."
                    echo "  Debug file: ${base_name}.debug ($(stat -c%s "$debug_file") bytes)"

                    # cv2pdb converts DWARF in PE files to PDB format
                    # Usage: cv2pdb <exe> [<output_exe>] [<pdb>]
                    if "$CV2PDB" "$debug_file" "$pdb_file" 2>"${pdb_file}.err"; then
                        if [[ -f "$pdb_file" ]]; then
                            pdb_size=$(stat -c%s "$pdb_file")
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
        done

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
