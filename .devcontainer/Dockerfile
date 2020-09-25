# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.134.0/containers/cpp/.devcontainer/base.Dockerfile
ARG VARIANT="buster"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# Install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends build-essential git cmake            \
         liblua5.1-dev zlib1g-dev libhunspell-dev libpcre3-dev libzip-dev libboost-dev \
         libyajl-dev libpulse-dev lua-rex-pcre lua-filesystem lua-zip lua-sql-sqlite3  \
         qt5-default qtmultimedia5-dev qttools5-dev luarocks ccache libpugixml-dev     \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
