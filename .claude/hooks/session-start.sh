#!/bin/bash
# SessionStart hook for Claude Code on the web: provision everything needed to
# compile Mudlet natively on the Ubuntu 24.04 session container.
#
# The container filesystem is cached after this hook completes, so the
# expensive steps (apt, Qt download, ccache warm-up build) only run when the
# cache is cold; on a warm container every step short-circuits and the hook
# finishes in well under a minute.
set -euo pipefail

# Local checkouts (desktop/CLI) manage their own toolchain - do nothing there.
if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

SUDO=""
if [ "$(id -u)" != "0" ]; then
  # -E keeps the proxy environment that downloads depend on in this container
  SUDO="sudo -E"
fi

# Mudlet needs Qt >= 6.8.2 but Ubuntu 24.04 only packages 6.4, so Qt comes
# from aqtinstall (same as .github/workflows/build-mudlet.yml) and everything
# else from apt. qtkeychain-qt6-dev is built against the distro Qt 6.4, which
# is fine: Qt guarantees binary compatibility across 6.x minor releases, and
# CI relies on the same mix.
QT_VERSION=6.9.0
QT_DIR="/opt/qt/${QT_VERSION}/gcc_64"

if ! dpkg -s qtkeychain-qt6-dev >/dev/null 2>&1; then
  echo "Installing apt build dependencies..."
  ${SUDO} apt-get update -qq
  DEBIAN_FRONTEND=noninteractive ${SUDO} apt-get install -y --no-install-recommends \
    ccache \
    g++ \
    libassimp-dev \
    libboost-dev \
    libcurl4-openssl-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libhunspell-dev \
    liblua5.1-0-dev \
    libpcre2-dev \
    libpugixml-dev \
    libpulse-dev \
    libsecret-1-dev \
    libspeechd-dev \
    libsqlite3-dev \
    libssl-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-0 \
    libyajl-dev \
    libzip-dev \
    lua5.1 \
    luarocks \
    mesa-common-dev \
    mold \
    pkg-config \
    qtkeychain-qt6-dev
fi

if [ ! -d "${QT_DIR}" ]; then
  echo "Installing Qt ${QT_VERSION} via aqtinstall..."
  if [ ! -x /opt/aqt-venv/bin/aqt ]; then
    ${SUDO} mkdir -p /opt/aqt-venv /opt/qt
    ${SUDO} chown "$(id -un)" /opt/aqt-venv /opt/qt
    python3 -m venv /opt/aqt-venv
    /opt/aqt-venv/bin/pip install --quiet aqtinstall
  fi
  # aqt drops aqtinstall.log in the current directory - keep it out of the repo
  (cd /opt/qt && /opt/aqt-venv/bin/aqt install-qt linux desktop "${QT_VERSION}" \
    linux_gcc_64 -O /opt/qt -m qt5compat qtmultimedia qtspeech)
fi

# Test-suite and UI-driving dependencies: xvfb and xcb libraries for the
# busted run (the aqt Qt's xcb platform needs libxcb-cursor0 and
# libxcb-shape0, which Ubuntu's own Qt would have pulled in), gstreamer for
# Qt Multimedia, and the docs/demo-videos.md toolchain (openbox, xdotool,
# imagemagick, ffmpeg) for driving and recording the real UI headlessly.
if ! dpkg -s libxcb-shape0 >/dev/null 2>&1; then
  echo "Installing test-suite apt dependencies..."
  DEBIAN_FRONTEND=noninteractive ${SUDO} apt-get install -y --no-install-recommends \
    xvfb \
    libgstreamer-plugins-base1.0-0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xinerama0 \
    xdotool \
    openbox \
    imagemagick \
    ffmpeg
fi

# Coverage tooling for the improve-test-coverage skill (gcovr for the report,
# jq for its check-lines.sh helper). Separate guard: containers cached before
# this block existed still pick it up.
if ! command -v gcovr >/dev/null 2>&1 || ! command -v jq >/dev/null 2>&1; then
  echo "Installing coverage tooling..."
  ${SUDO} apt-get update -qq
  DEBIAN_FRONTEND=noninteractive ${SUDO} apt-get install -y --no-install-recommends \
    gcovr \
    jq
fi

# Lua rocks that LuaGlobal.lua and the test harnesses load, mirroring the CI
# list in .github/workflows/build-mudlet.yml. The egress proxy blocks GitHub
# codeload tarballs (403) while git clones are allowed, so rocks whose
# rockspecs point at tarballs are built from a git checkout instead.
rock_from_git() {
  local spec=$1 repo=$2 ref=$3 dir
  dir="$(mktemp -d)"
  git clone -q --depth 1 --branch "${ref}" "${repo}" "${dir}" || return 1
  (cd "${dir}" && curl -fsS -O "https://luarocks.org/${spec}.rockspec" \
    && luarocks --lua-version 5.1 make --local "${spec}.rockspec" >/dev/null)
}

ensure_rock() {
  local rock=$1 version=${2:-}
  luarocks --lua-version 5.1 show "${rock}" >/dev/null 2>&1 && return 0
  local out url spec repo ref
  for _ in 1 2 3 4 5; do
    # shellcheck disable=SC2086
    out=$(luarocks --lua-version 5.1 install --local "${rock}" ${version} 2>&1) && return 0
    url=$(grep -oE 'https://github.com/[^ ]+/archive/[^ ]+\.tar\.gz' <<<"${out}" | head -1)
    [ -n "${url}" ] || { echo "${out}" | tail -3; return 1; }
    spec=$(grep -oE 'https://luarocks.org/[A-Za-z0-9_.-]+\.rockspec' <<<"${out}" | head -1 | sed 's|.*/||; s|\.rockspec$||')
    repo="$(sed -E 's|(https://github.com/[^/]+/[^/]+)/archive/.*|\1|' <<<"${url}").git"
    ref=$(sed -E 's|.*/archive/(.*)\.tar\.gz|\1|; s|^refs/tags/||' <<<"${url}")
    rock_from_git "${spec:-${rock}}" "${repo}" "${ref}" || return 1
  done
  return 1
}

for rock in LuaFileSystem lpeg lua-zip lrexlib-pcre2 luautf8 lua-yajl argparse lunajson busted; do
  ensure_rock "${rock}" || echo "WARNING: could not install rock ${rock}"
done
# CI pins this version; 2.8.0 breaks DB.lua's PRAGMA table_info handling
ensure_rock LuaSQL-SQLite3 2.6.1 || echo "WARNING: could not install rock LuaSQL-SQLite3"

# Let every shell in the session find the aqt-installed Qt without extra
# flags, and the --local rocks (the C++ functional tests load LuaGlobal.lua,
# which needs them on the Lua path).
if [ -n "${CLAUDE_ENV_FILE:-}" ]; then
  echo "export CMAKE_PREFIX_PATH=\"${QT_DIR}\${CMAKE_PREFIX_PATH:+:\$CMAKE_PREFIX_PATH}\"" >> "${CLAUDE_ENV_FILE}"
  eval "$(luarocks path --local --lua-version 5.1)"
  {
    echo "export LUA_PATH='${LUA_PATH}'"
    echo "export LUA_CPATH='${LUA_CPATH}'"
  } >> "${CLAUDE_ENV_FILE}"
fi

# PR-review tooling for the agent. Installed at user scope, so it lands in
# the cached container state like everything else. Non-fatal: a marketplace
# outage must not block the session.
if command -v claude >/dev/null 2>&1; then
  if ! claude plugin list 2>/dev/null | grep -q 'pr-review-toolkit'; then
    claude plugin marketplace add anthropics/claude-plugins-official || true
    claude plugin install pr-review-toolkit@claude-plugins-official || true
  fi
fi

# The repo is cloned fresh each session, so submodules are always missing even
# on a warm container.
if [ -n "${CLAUDE_PROJECT_DIR:-}" ]; then
  git -C "${CLAUDE_PROJECT_DIR}" submodule update --init --recursive

  # Configure the default tree every session (cheap) so it already exists
  # with Qt and the mold linker wired in - linking dominates a warm rebuild,
  # and mold cuts the link tail dramatically (see PR #9927; until its
  # top-level set_alternate_linker() move merges, the flag only reaches the
  # main mudlet binary, afterwards every target).
  cd "${CLAUDE_PROJECT_DIR}"
  cmake --preset linux-debug-nosan -DCMAKE_PREFIX_PATH="${QT_DIR}" -DUSE_ALTERNATE_LINKER=mold

  # Warm the compiler cache once per container image. CMakeLists.txt wires
  # ccache in automatically, so this one cold build (~20 min) makes every
  # later session's build mostly cache hits (a few minutes including linking).
  # The build tree itself is discarded with the session; only ccache persists.
  CCACHE_DIR_PATH="$(ccache -k cache_dir 2>/dev/null || echo "${HOME}/.cache/ccache")"
  CACHE_KB="$(du -sk "${CCACHE_DIR_PATH}" 2>/dev/null | cut -f1 || echo 0)"
  if [ "${CACHE_KB:-0}" -lt 102400 ]; then
    echo "Cold ccache - running warm-up build..."
    cmake --build --preset linux-debug-nosan
  fi
fi
