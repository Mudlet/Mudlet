# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.208.0/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version (use Debian 11/9, Ubuntu 18.04/21.04 on local arm64/Apple Silicon): debian-11, debian-10, debian-9, ubuntu-21.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="bullseye"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# Install C++ dependencies
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends build-essential git liblua5.1-dev  \
        zlib1g-dev libhunspell-dev libpcre3-dev libzip-dev libboost-dev libyajl-dev  \
        libpulse-dev lua-rex-pcre lua-filesystem lua-zip lua-sql-sqlite3 qtbase5-dev \
        qtchooser qt5-qmake qtbase5-dev-tools qtmultimedia5-dev qttools5-dev luarocks\
        ccache libpugixml-dev libqt5texttospeech5-dev qtspeech5-flite-plugin         \
        qtspeech5-speechd-plugin libqt5opengl5-dev ninja-build firefox-esr           \
    && apt-get clean                                        \
    && rm -rf /var/lib/apt/lists/*

# Install Lua dependencies
RUN luarocks install luautf8     \
    && luarocks install lua-yajl \
    && luarocks install busted
