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

# Let every shell in the session find the aqt-installed Qt without extra flags.
if [ -n "${CLAUDE_ENV_FILE:-}" ]; then
  echo "export CMAKE_PREFIX_PATH=\"${QT_DIR}\${CMAKE_PREFIX_PATH:+:\$CMAKE_PREFIX_PATH}\"" >> "${CLAUDE_ENV_FILE}"
fi

# The repo is cloned fresh each session, so submodules are always missing even
# on a warm container.
if [ -n "${CLAUDE_PROJECT_DIR:-}" ]; then
  git -C "${CLAUDE_PROJECT_DIR}" submodule update --init --recursive

  # Warm the compiler cache once per container image. CMakeLists.txt wires
  # ccache in automatically, so this one cold build (~20 min) makes every
  # later session's build mostly cache hits (a few minutes including linking).
  # The build tree itself is discarded with the session; only ccache persists.
  CCACHE_DIR_PATH="$(ccache -k cache_dir 2>/dev/null || echo "${HOME}/.cache/ccache")"
  CACHE_KB="$(du -sk "${CCACHE_DIR_PATH}" 2>/dev/null | cut -f1 || echo 0)"
  if [ "${CACHE_KB:-0}" -lt 102400 ]; then
    echo "Cold ccache - running warm-up build..."
    cd "${CLAUDE_PROJECT_DIR}"
    cmake --preset linux-debug-nosan -DCMAKE_PREFIX_PATH="${QT_DIR}"
    cmake --build --preset linux-debug-nosan
  fi
fi

echo "Mudlet build environment ready (Qt ${QT_VERSION} at ${QT_DIR})"
