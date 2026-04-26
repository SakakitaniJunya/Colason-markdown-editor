#!/usr/bin/env bash
set -euo pipefail

echo "==> Colason post-create setup"

# ---------- Claude Code CLI ----------
if ! command -v claude >/dev/null 2>&1; then
  echo "==> Installing Claude Code"
  curl -fsSL https://claude.ai/install.sh | bash
  # ensure ~/.local/bin is on PATH for future sessions
  if ! grep -q ".local/bin" "$HOME/.bashrc"; then
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
  fi
fi

# ---------- editor/ (Vite TS frontend) ----------
if [ -f "editor/package.json" ]; then
  echo "==> Installing editor/ npm deps"
  (cd editor && npm install)
fi

# ---------- CMake configure (Ninja, system Qt6 — no vcpkg) ----------
# vcpkg path is intentionally NOT set so find_package() uses apt-installed Qt6
echo "==> CMake configure"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug || {
  echo "(initial configure failed — check Qt6 detection logs above)"
}

# ---------- Quick environment summary ----------
echo
echo "==> Environment ready"
echo "    CMake:    $(cmake --version | head -1)"
echo "    Ninja:    $(ninja --version)"
echo "    GCC:      $(gcc --version | head -1)"
echo "    Qt6:      $(dpkg -l qt6-base-dev | tail -1 | awk '{print $3}')"
echo "    Node:     $(node --version)"
echo "    Claude:   $(claude --version 2>/dev/null || echo 'run `claude` to authenticate first')"
echo
echo "Build:    cmake --build build"
echo "Run UI:   QT_QPA_PLATFORM=xcb ./build/src/Colason  (need X11 forward)"
echo "Tests:    ctest --test-dir build --output-on-failure"
echo
echo "Free tier tip: stop the codespace when not coding —"
echo "  https://github.com/codespaces (Stop) or `gh codespace stop`"
