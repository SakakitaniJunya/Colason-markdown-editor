# Colason 開発ガイド

## 前提条件

| ツール | バージョン | 備考 |
|--------|-----------|------|
| Visual Studio | 2026 (v18) | C++20 デスクトップ開発ワークロード |
| CMake | VS18内蔵 | `Common7/IDE/.../CMake/CMake/bin/cmake.exe` |
| Qt 6.8.3 | **aqtinstall** (事前ビルド済みバイナリ) | `c:/Qt/6.8.3/msvc2022_64` -- vcpkgソースビルドではなくaqtinstallを使用 |
| vcpkg | ローカル | spdlog, nlohmann_json, GTest (Qt以外の依存ライブラリ用) |
| Node.js | 18+ | エディタ (TypeScript) ビルド用 |
| Python 3 | 3.9+ | aqtinstall 用 |

---

## 1. Qt6 のインストール (初回のみ)

vcpkgではなく **aqtinstall** (事前ビルド済みバイナリ) を使用する。

```bash
pip install aqtinstall

# Qt 6.8.3 + WebEngine モジュールをインストール
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 \
  --outputdir c:/Qt \
  --modules qtwebengine qtwebchannel qtpositioning
```

インストール先: `c:/Qt/6.8.3/msvc2022_64`

### インストールされるモジュール

| モジュール | 用途 |
|-----------|------|
| Core, Gui, Widgets | 基本UI |
| WebEngineCore, WebEngineWidgets | TipTapエディタ表示 |
| WebChannel | C++ ↔ JavaScript ブリッジ |
| Qml, Quick, QuickWidgets | WebEngine依存 |
| Network, OpenGL, Positioning | WebEngine依存 |
| Svg, PrintSupport | アイコン・印刷 |
| Concurrent | 非同期処理 |

---

## 2. エディタ (TypeScript) のビルド

```bash
cd editor
npm install
npm run build
```

`editor/dist/` にバンドル (~570KB) が生成される。C++アプリ実行時に参照される。

---

## 3. CMake コンフィグ

```bash
# パス定義
CMAKE="c:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
VCPKG_ROOT="<vcpkg-root>"
QT_DIR="c:/Qt/6.8.3/msvc2022_64"

# キャッシュクリア (ジェネレータ変更時のみ)
# rm -rf build/CMakeCache.txt build/CMakeFiles

# コンフィグ
"$CMAKE" -S . -B build \
  -G "Visual Studio 18 2026" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-windows \
  -DVCPKG_MANIFEST_MODE=OFF \
  -DCMAKE_PREFIX_PATH="$QT_DIR;$VCPKG_ROOT/installed/x64-windows"
```

### 確認ポイント

コンフィグ出力に以下が表示されること:

```
-- Qt6WebEngineWidgets_FOUND: 1
-- Qt6WebChannel_FOUND: 1
-- Qt6Quick_FOUND: 1
```

`Qt6WebEngineWidgets_FOUND: 0` の場合は「トラブルシューティング」セクションを参照。

---

## 4. ビルド

```bash
# Debug ビルド
"$CMAKE" --build build --config Debug

# Release ビルド
"$CMAKE" --build build --config Release
```

出力: `build/src/Debug/colason.exe` または `build/src/Release/colason.exe`

---

## 5. 実行

### 方法A: コマンドラインから起動

Qt DLL へのパスを通して起動する:

```cmd
set PATH=c:\Qt\6.8.3\msvc2022_64\bin;%PATH%
build\src\Debug\colason.exe
```

> **注意**: Git Bash / MSYS2 環境からは Windows DLL のパス解決が動作しない場合がある。`cmd.exe` を使うこと。

### 方法B: DLL をデプロイして起動 (推奨)

`windeployqt` を使って必要なDLL・プラグインをexeの隣にコピーする:

```cmd
set PATH=c:\Qt\6.8.3\msvc2022_64\bin;%PATH%
windeployqt build\src\Release\colason.exe
```

これにより以下が自動コピーされる:
- Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll 等
- Qt6WebEngineCore.dll, QtWebEngineProcess.exe
- platforms/qwindows.dll
- resources/ (WebEngine ICUデータ等)
- translations/

デプロイ後は PATH 設定なしで `colason.exe` を直接実行可能。

### 方法C: Visual Studio から起動

`build/Colason.sln` を開き、`colason` プロジェクトをスタートアッププロジェクトに設定して F5。

デバッグ時のコンソール出力を見るため、現在 `WIN32` フラグは無効化されている (`src/CMakeLists.txt` 12行目)。リリース時に有効化すること。

---

## 6. 条件コンパイル

| マクロ | 条件 | 効果 |
|--------|------|------|
| `HAS_WEBENGINE` | Qt6WebEngineWidgets が見つかった場合 | WebEngine/TipTap エディタを使用 |
| (なし) | WebEngine 未検出時 | プレースホルダ QLabel を表示 |
| `HAS_SPDLOG` | spdlog が見つかった場合 | 構造化ログ出力を使用 |
| `Q_OS_WIN` | Windows環境 | DWM API (フレームレスウィンドウ、タイトルバー色テーマ連動) を有効化 |

### リンクライブラリ (Windows)

`src/CMakeLists.txt` で以下をリンク:

```cmake
target_link_libraries(colason PRIVATE ole32 oleaut32 shell32 dwmapi)
```

- `dwmapi`: DWM API — カスタムフレームレスウィンドウ、`DWMWA_CAPTION_COLOR`/`DWMWA_TEXT_COLOR` によるテーマ連動タイトルバー色

---

## 7. トラブルシューティング

### Qt6WebEngineWidgets_FOUND: 0

**原因**: WebEngineCore の依存モジュール (Positioning 等) が未インストールまたは未検出。

**対処**:
1. `c:/Qt/6.8.3/msvc2022_64/lib/cmake/` に以下が存在するか確認:
   - `Qt6Positioning/`
   - `Qt6Quick/`
   - `Qt6Qml/`
   - `Qt6Network/`
   - `Qt6OpenGL/`
2. 不足していれば aqtinstall で再インストール:
   ```bash
   python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 \
     --outputdir c:/Qt --modules qtwebengine qtwebchannel qtpositioning
   ```
3. CMakeLists.txt で依存モジュールが `WebEngineWidgets` より前に `find_package` されていること:
   ```cmake
   find_package(Qt6 COMPONENTS Network OpenGL Qml Quick QuickWidgets Positioning QUIET)
   find_package(Qt6 COMPONENTS WebEngineCore WebEngineWidgets WebChannel ... QUIET)
   ```

### cmake: command not found (bash環境)

CMake は VS18 内蔵のものを使う。フルパスで指定するか、PATH に追加:

```bash
export CMAKE="c:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
```

### generator mismatch エラー

`build/CMakeCache.txt` に前回のジェネレータ情報が残っている。キャッシュを削除:

```bash
rm -rf build/CMakeCache.txt build/CMakeFiles
```

### DLL が見つからない (実行時)

`Qt6Cored.dll: cannot open shared object file` 等のエラー。

- `cmd.exe` から `set PATH=c:\Qt\6.8.3\msvc2022_64\bin;%PATH%` で起動
- または `windeployqt` で DLL をデプロイ (上記「方法B」参照)

---

## 8. ディレクトリ構成

```
colason/
├── CMakeLists.txt          # ルート CMake (Qt6, vcpkg 検出)
├── build/                  # ビルド出力
├── docs/
│   ├── project/
│   │   ├── plan.md         # プロジェクト計画書
│   │   └── dev-guide.md    # 本書
│   └── spec/               # 設計ドキュメント
│       ├── requirements.md
│       ├── technology-selection.md
│       ├── architecture.md
│       ├── ui-design.md
│       └── feature-design.md
├── editor/                 # TypeScript エディタ (TipTap/ProseMirror)
│   ├── src/
│   ├── dist/               # ビルド済みバンドル
│   ├── package.json
│   └── vite.config.ts
├── resources/
│   ├── colason.qrc         # Qt リソースファイル
│   └── icons/              # SVG アイコン
│       ├── colason.svg             # アプリアイコン
│       ├── chevron-right-dark.svg  # ツリー展開矢印 (ライト系テーマ用)
│       ├── chevron-down-dark.svg
│       ├── chevron-right-light.svg # ツリー展開矢印 (ダーク系テーマ用)
│       ├── chevron-down-light.svg
│       ├── file.svg                # 汎用ファイルアイコン
│       ├── file-text.svg           # テキスト/Markdownファイルアイコン
│       ├── folder.svg              # フォルダアイコン (閉)
│       └── folder-open.svg         # フォルダアイコン (開)
├── src/                    # C++ ソース
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── bridge/             # QWebChannel ブリッジ
│   │   ├── EditorBridge.h/.cpp
│   │   ├── OutlineBridge.h/.cpp
│   │   ├── SearchBridge.h/.cpp
│   │   └── ThemeBridge.h/.cpp
│   ├── core/               # ビジネスロジック
│   │   ├── AutoSaveManager.h/.cpp
│   │   ├── DocumentManager.h/.cpp
│   │   ├── DraftRecoveryManager.h/.cpp
│   │   ├── ExportManager.h/.cpp
│   │   ├── GlobalSearchManager.h/.cpp
│   │   ├── ImageManager.h/.cpp
│   │   ├── PreferencesManager.h/.cpp
│   │   ├── RecentFilesManager.h/.cpp
│   │   └── ThemeManager.h/.cpp
│   ├── ui/                 # Qt Widgets UI
│   │   ├── MainWindow.h/.cpp
│   │   ├── MenuBarManager.h/.cpp
│   │   ├── QuickOpenDialog.h/.cpp
│   │   ├── StatusBarManager.h/.cpp  # 現在未使用 (ステータスバー非表示)
│   │   └── sidebar/
│   │       ├── SidebarContainer.h/.cpp
│   │       ├── DocumentListPanel.h/.cpp
│   │       ├── FileExplorerPanel.h/.cpp
│   │       ├── FileIconProvider.h/.cpp   # カスタムSVGアイコンプロバイダ
│   │       └── OutlinePanel.h/.cpp
│   └── utils/
└── tests/                  # GTest ユニットテスト
```
