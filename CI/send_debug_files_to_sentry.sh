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

if [[ "$OS" == MINGW* ]] || [[ "$OS" == MSYS* ]] || [[ "$OS" == CYGWIN* ]]; then
    strip --strip-debug "$MUDLET_EXEC"
fi

rm -f sentry-cli
