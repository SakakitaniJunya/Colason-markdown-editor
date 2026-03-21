# Colason - ビルド・起動手順

## 前提条件

| ツール | バージョン | 備考 |
|--------|-----------|------|
| Visual Studio 2026 | 18.x | C++ デスクトップ開発ワークロード |
| CMake | 3.25+ | VS同梱のものを使用 |
| Qt6 | 6.8.3 | aqtinstall経由でインストール |
| vcpkg | 最新 | spdlog, nlohmann_json等のサードパーティ依存 |
| Node.js | 18+ | エディタ (TypeScript) のビルドに必要 |
| Python | 3.10+ | aqtinstall に必要 |

---

## 1. Qt6のインストール (aqtinstall)

```bash
pip install aqtinstall

# 基本モジュール + WebEngine + WebChannel + Positioning
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 \
  --outputdir c:/Qt \
  --modules qtwebengine qtwebchannel qtpositioning
```

インストール先: `c:/Qt/6.8.3/msvc2022_64`

### 含まれるモジュール
Core, Gui, Widgets, WebEngineWidgets, WebEngineCore, WebChannel, Svg, PrintSupport, Concurrent, Qml, Quick, QuickWidgets, Network, OpenGL, Positioning

---

## 2. エディタ (TypeScript) のビルド

```bash
cd editor
npm install
npm run build
```

`editor/dist/` にバンドルが生成される (~570KB)。

---

## 3. CMake Configure

```bash
CMAKE="c:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
VCPKG_ROOT="c:/Users/junya.sakakitani/source/vcpkg"
QT_DIR="c:/Qt/6.8.3/msvc2022_64"

"$CMAKE" -S . -B build \
  -G "Visual Studio 18 2026" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-windows \
  -DVCPKG_MANIFEST_MODE=OFF \
  -DCMAKE_PREFIX_PATH="$QT_DIR;$VCPKG_ROOT/installed/x64-windows"
```

### 確認ポイント
Configure出力に以下が表示されること:
```
-- Qt6WebEngineWidgets_FOUND: 1
-- Qt6WebChannel_FOUND: 1
-- Qt6Quick_FOUND: 1
```

`Qt6WebEngineWidgets_FOUND: 0` の場合は `qtpositioning` モジュールが不足している可能性がある。

---

## 4. ビルド

```bash
"$CMAKE" --build build --config Release
```

出力: `build/src/Release/colason.exe`

---

## 5. ランタイムDLLの配置

`colason.exe` と同じディレクトリに以下を配置する必要がある。

### Qt DLL (c:/Qt/6.8.3/msvc2022_64/bin/ からコピー)
- `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`
- `Qt6WebEngineCore.dll`, `Qt6WebEngineWidgets.dll`
- `Qt6WebChannel.dll`, `Qt6Network.dll`, `Qt6OpenGL.dll`
- `Qt6Quick.dll`, `Qt6QuickWidgets.dll`, `Qt6Qml.dll`, `Qt6QmlModels.dll`
- `Qt6Positioning.dll`, `Qt6PrintSupport.dll`, `Qt6Svg.dll`, `Qt6Concurrent.dll`
- その他 WebEngineCore が間接依存する DLL 多数

> **簡易方法**: `c:/Qt/6.8.3/msvc2022_64/bin/` からデバッグ版 (`*d.dll`) を除く全DLLをコピー

### プラグイン
```
platforms/qwindows.dll        ← c:/Qt/6.8.3/msvc2022_64/plugins/platforms/
styles/qmodernwindowsstyle.dll ← c:/Qt/6.8.3/msvc2022_64/plugins/styles/
imageformats/qgif.dll         ← c:/Qt/6.8.3/msvc2022_64/plugins/imageformats/
imageformats/qico.dll
imageformats/qjpeg.dll
imageformats/qsvg.dll
tls/*.dll                     ← c:/Qt/6.8.3/msvc2022_64/plugins/tls/
```

### WebEngine リソース
```
QtWebEngineProcess.exe  ← c:/Qt/6.8.3/msvc2022_64/bin/
resources/              ← c:/Qt/6.8.3/msvc2022_64/resources/ (ディレクトリごとコピー)
translations/           ← c:/Qt/6.8.3/msvc2022_64/translations/qtwebengine_locales/
```

### qt.conf
`colason.exe` と同じディレクトリに作成:
```ini
[Paths]
Plugins = .
```

---

## 6. 起動

```bash
./build/src/Release/colason.exe
```

> **注意**: Git Bash等のMSYS環境から直接起動するとDLLパス解決に失敗する場合がある。その場合は `cmd.exe` やエクスプローラーから起動する。

---

## 条件付きコンパイル

- `#ifdef HAS_WEBENGINE` で全WebEngineコードをガード
- WebEngine未検出時はプレースホルダー (QLabel) で起動可能
- CMakeが `find_package` の結果に基づき `HAS_WEBENGINE=1` を自動定義
- Windows DWM API (`dwmapi`) をリンク: カスタムフレームレスウィンドウ、テーマ連動タイトルバー色に使用

---

## トラブルシューティング

### WebEngineWidgets が見つからない
```
Qt6WebEngineWidgets_FOUND: 0
```
→ `Qt6Positioning` が不足。aqtinstallで `qtpositioning` モジュールを追加インストール:
```bash
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 \
  --outputdir c:/Qt --modules qtpositioning
```

### 起動時に STATUS_DLL_NOT_FOUND (0xC0000135)
→ Qt DLL が不足。`c:/Qt/6.8.3/msvc2022_64/bin/` から全リリースDLLをコピー。

### Git Bash から起動できない
→ MSYS環境の共有ライブラリ解決の問題。`cmd.exe` やエクスプローラーから起動する。
