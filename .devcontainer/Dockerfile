# Codespaces base (Ubuntu 22.04 + non-root vscode user pre-installed)
FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

ENV DEBIAN_FRONTEND=noninteractive

# ---- C++ / CMake / Ninja ----
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    gdb \
    lldb \
    clang \
    clang-format \
    clang-tidy \
    && rm -rf /var/lib/apt/lists/*

# ---- Qt6 (system packages — vcpkg/Chromium ビルド回避で容量を 8-15GB → 1.5GB に圧縮) ----
RUN apt-get update && apt-get install -y --no-install-recommends \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-webengine-dev \
    qt6-webengine-dev-tools \
    qt6-svg-dev \
    libqt6webenginecore6 \
    libqt6webenginewidgets6 \
    libqt6webchannel6 \
    qt6-l10n-tools \
    libgl1-mesa-dev \
    libxkbcommon-dev \
    libfontconfig1-dev \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-keysyms1 \
    libxcb-shape0 \
    libxcb-xkb1 \
    && rm -rf /var/lib/apt/lists/*

# ---- C++ libs (vcpkg dependencies as system packages) ----
RUN apt-get update && apt-get install -y --no-install-recommends \
    nlohmann-json3-dev \
    libspdlog-dev \
    libgtest-dev \
    googletest \
    && rm -rf /var/lib/apt/lists/*

# ---- Editor frontend (TipTap/Vite) は features の Node.js でカバー ----

# ---- Default workdir + ownership ----
WORKDIR /workspaces

# Codespaces overrides USER at runtime; this ensures clean default
USER vscode
