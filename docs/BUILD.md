# Colason ビルド・起動ガイド

## 前提条件

| ツール | パス |
|--------|------|
| CMake | `c:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe` |
| Visual Studio | 18 (2026) Community/Professional |
| Qt6 (aqtinstall) | `c:/Qt/6.8.3/msvc2022_64` |
| vcpkg | `<vcpkg-root>` |
| Node.js + npm | エディタビルド用 |

### Qt6 モジュール (aqtinstall でインストール)

```bash
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 \
  --outputdir c:/Qt \
  --modules qtwebengine qtwebchannel qtpositioning
```

必要モジュール: Core, Gui, Widgets, WebEngineWidgets, WebEngineCore, WebChannel,
Svg, PrintSupport, Concurrent, Qml, Quick, QuickWidgets, Network, OpenGL, Positioning

---

## ビルド手順

### 変数の設定 (bash)

```bash
CMAKE="c:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
VCPKG_ROOT="<vcpkg-root>"
QT_DIR="c:/Qt/6.8.3/msvc2022_64"
PROJECT="<colason-repo>"
```

### 1. CMake Configure

```bash
"$CMAKE" -S "$PROJECT" -B "$PROJECT/build" \
  -G "Visual Studio 18 2026" -A x64 \
  -DVCPKG_MANIFEST_MODE=OFF \
  "-DCMAKE_PREFIX_PATH=$QT_DIR;$VCPKG_ROOT/installed/x64-windows"
```

正常なら以下が表示される:
```
Qt6WebEngineWidgets_FOUND: 1
Qt6WebChannel_FOUND: 1
Qt6Quick_FOUND: 1
```

### 2. ビルド

```bash
"$CMAKE" --build "$PROJECT/build" --config Release
```

出力: `build/src/Release/colason.exe`

### 3. DLL デプロイ (初回 or Qt モジュール追加時)

```bash
"$QT_DIR/bin/windeployqt6.exe" --release \
  --no-translations --no-system-d3d-compiler \
  "$PROJECT/build/src/Release/colason.exe"
```

これにより以下がコピーされる:
- Qt DLL群 (Qt6Core, Qt6Gui, Qt6Widgets, Qt6WebEngineCore, ...)
- プラグイン (platforms/, styles/, imageformats/, tls/, position/, ...)
- QtWebEngineProcess.exe, resources/, qml/
- opengl32sw.dll

### 4. エディタビルド (初回 or editor/ 変更時)

```bash
cd "$PROJECT/editor" && npm install && npm run build
```

出力: `editor/dist/` (index.html + assets/)

### 5. 起動

```bash
"$PROJECT/build/src/Release/colason.exe"
```

---

## ワンライナー (再ビルド + 起動)

```bash
CMAKE="c:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" && "$CMAKE" --build <colason-repo>/build --config Release && <colason-repo>/build/src/Release/colason.exe
```

---

## トラブルシューティング

### Qt6WebEngineWidgets_FOUND: 0
- `qtpositioning` モジュールがインストールされているか確認
- CMakeLists.txt で Network, OpenGL, Qml, Quick, QuickWidgets, Positioning が WebEngine より前に find_package されているか確認
- CMakeCache.txt を削除して再 configure

### LNK1104: colason.exe を開けない
- 既に colason.exe が実行中。`taskkill /F /IM colason.exe` で終了してからビルド

### DLL が見つからないエラー
- windeployqt6.exe を再実行

### エディタが表示されない (白い画面)
- `editor/dist/` が存在するか確認
- `npm run build` を editor/ ディレクトリで実行

---

## アーキテクチャ

```
colason.exe (C++20 / Qt6)
  |
  +-- MainWindow
  |     +-- SidebarContainer (QTabWidget)
  |     |     +-- DocumentListPanel (文書リスト)
  |     |     +-- FileExplorerPanel (ファイルツリー)
  |     |     +-- OutlinePanel (見出し一覧)
  |     +-- QWebEngineView (エディタ)
  |     +-- MenuBarManager
  |     +-- StatusBarManager
  |
  +-- Bridges (QWebChannel)
  |     +-- EditorBridge
  |     +-- OutlineBridge
  |     +-- SearchBridge
  |     +-- ThemeBridge
  |
  +-- Core Managers
        +-- DocumentManager
        +-- AutoSaveManager
        +-- ThemeManager (6 themes: dark/light/github/sepia/nord/dracula)
        +-- PreferencesManager
        +-- ExportManager (PDF/HTML)
        +-- ImageManager
        +-- RecentFilesManager
        +-- DraftRecoveryManager
        +-- GlobalSearchManager
```

### エディタ (TypeScript / Vite)

```
editor/
  +-- src/
  |     +-- index.ts          (エントリポイント)
  |     +-- editor.ts          (Tiptap エディタ生成)
  |     +-- bridge.ts          (QWebChannel ブリッジ)
  |     +-- source-mode.ts     (CodeMirror 6 ソースモード)
  |     +-- keybindings.ts     (vim/emacs キーバインド)
  |     +-- extensions/        (Mermaid, KaTeX)
  |     +-- themes/base.css    (ベーススタイル)
  +-- dist/                    (ビルド出力)
```

## キーボードショートカット

| キー | 機能 |
|------|------|
| Ctrl+Shift+1 | 文書リスト表示 |
| Ctrl+Shift+2 | ファイルツリー表示 |
| Ctrl+Shift+3 | アウトライン表示 |
| Ctrl+P | クイックオープン |
| Ctrl+B | 太字 |
| Ctrl+I | 斜体 |
| Ctrl+S | 保存 |
| Ctrl+Shift+S | 名前を付けて保存 |
| Ctrl+N | 新規ドキュメント |
| Ctrl+O | ファイルを開く |
| Ctrl+= | ズームイン |
| Ctrl+- | ズームアウト |
