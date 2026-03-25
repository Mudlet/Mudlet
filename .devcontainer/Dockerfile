# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.208.0/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-13, debian-12, debian-11, ubuntu-24.04, ubuntu-22.04
ARG VARIANT=debian-13
FROM mcr.microsoft.com/devcontainers/cpp:${VARIANT}
ENV DBUS_SESSION_BUS_ADDRESS="autolaunch:" DISPLAY=":1" LANG="en_US.UTF-8" LANGUAGE="en_US.UTF-8"

# Add deb-multimedia repo for libpcre3-dev (not available in Debian 13+)
RUN echo "deb http://www.deb-multimedia.org trixie main" >> /etc/apt/sources.list \
    && apt-get update -oAcquire::AllowInsecureRepositories=true \
    && apt-get install -y --allow-unauthenticated deb-multimedia-keyring \
    && rm -rf /var/lib/apt/lists/*

# Install C++ dependencies
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends build-essential git libglu1-mesa-dev \
        libgl1-mesa-dev liblua5.1-0-dev zlib1g-dev libhunspell-dev libpcre3-dev libpcre2-dev \
        libzip-dev libboost-dev libyajl-dev libpulse-dev libsecret-1-dev lua-rex-pcre2 \
        lua-filesystem lua-zip lua-sql-sqlite3 libxkbcommon-dev qmake6-bin qt6-base-dev \
        libqt6opengl6-dev qt6-multimedia-dev qt6-tools-dev qtkeychain-qt6-dev luarocks ccache \
        libpugixml-dev libqt6core5compat6-dev qt6-speech-dev ninja-build cmake libzstd-dev \
        libsqlite3-dev libassimp-dev firefox-esr \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Lua dependencies (lua-sql-sqlite3 and lua-rex-pcre2 are already installed via apt)
RUN luarocks install luautf8 \
    && luarocks install lua-yajl \
    && luarocks install lrexlib-pcre \
    && luarocks install busted
