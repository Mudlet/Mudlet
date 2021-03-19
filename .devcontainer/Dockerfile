# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.134.0/containers/cpp/.devcontainer/base.Dockerfile
ARG VARIANT="buster"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# Desktop Debian (https://github.com/microsoft/vscode-dev-containers/blob/master/script-library/docs/desktop-lite.md#usage)
COPY library-scripts/desktop-lite-debian.sh /tmp/library-scripts/
RUN apt-get update && bash /tmp/library-scripts/desktop-lite-debian.sh automatic mudlet && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
ENV DBUS_SESSION_BUS_ADDRESS="autolaunch:" DISPLAY=":1" LANG="en_US.UTF-8" LANGUAGE="en_US.UTF-8"
ENTRYPOINT ["/usr/local/share/desktop-init.sh"]
CMD ["sleep", "infinity"]

# Install C++ dependencies
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends build-essential git cmake            \
         liblua5.1-dev zlib1g-dev libhunspell-dev libpcre3-dev libzip-dev libboost-dev \
         libyajl-dev libpulse-dev lua-rex-pcre lua-filesystem lua-zip lua-sql-sqlite3  \
         qt5-default qtmultimedia5-dev qttools5-dev luarocks ccache libpugixml-dev     \
         libqt5texttospeech5-dev qtspeech5-flite-plugin qtspeech5-speechd-plugin       \
         libqt5opengl5-dev                                                             \
    && apt-get clean                                        \
    && rm -rf /var/lib/apt/lists/*

# Install Lua dependencies
RUN luarocks install luautf8     \
    && luarocks install lua-yajl \
    && luarocks install busted
