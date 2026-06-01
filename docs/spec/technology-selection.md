# Colason - 技術選定書

## 1. 基本方針

**C++ Qt6 アプリシェル + QWebEngine（Chromium）ハイブリッドアーキテクチャ**

- **ネイティブ部分（Qt6 Widgets）:** MainWindow、サイドバー、カスタムタイトルバー（メニューバー＋ウィンドウコントロール）、ダイアログ
- **エディタ部分（QWebEngineView）:** TipTap/ProseMirror WYSIWYG + CodeMirror 6 ソースモード
- **ブリッジ:** QWebChannel による C++ ↔ JavaScript 双方向通信

---

## 2. 言語・フレームワーク

| カテゴリ | 技術 | バージョン | 理由 |
|----------|------|------------|------|
| アプリシェル言語 | **C++20** | MSVC (VS 18) | concepts、ranges等モダンC++機能 |
| エディタ言語 | **TypeScript** | 5.x | 型安全なJavaScript |
| GUI（ネイティブ） | **Qt 6.8+** | aqtinstall (`c:/Qt/6.8.3/msvc2022_64`) | 最も成熟したC++ GUIフレームワーク |
| GUI（エディタ） | **Vanilla TypeScript** | - | TipTapはフレームワーク非依存で動作。シンプルさ優先 |
| ビルド（C++） | **CMake** | 3.25+ | Qt6標準ビルドシステム |
| ビルド（JS） | **Vite** | 6.x (npm) | 高速バンドラ。HMR対応で開発効率が高い |
| パッケージ（C++） | **vcpkg** | manifest mode | C++ライブラリ用 (spdlog, nlohmann-json等)。Qt本体はaqtinstallで別途導入 |
| パッケージ（JS） | **npm** | 10+ | Node.js v22標準パッケージマネージャ |

---

## 3. Qtモジュール一覧

> **導入方法**: aqtinstall (プリビルドバイナリ) — `c:/Qt/6.8.3/msvc2022_64`
> vcpkgソースビルドは断念済み (後述「10. リスクと対策」参照)

| モジュール | 用途 | 導入状態 |
|-----------|------|---------|
| Qt6::Core | ファイルI/O、Signal/Slot、JSON、基盤機能 | 導入済み |
| Qt6::Gui | フォント・画像等の描画基盤 | 導入済み |
| Qt6::Widgets | MainWindow、サイドバー、カスタムタイトルバー、メニュー、ダイアログ | 導入済み |
| Qt6::WebEngineWidgets | QWebEngineView（Chromium埋込みエディタ） | 導入済み |
| Qt6::WebChannel | C++ ↔ JavaScript 双方向通信 | 導入済み |
| Qt6::Svg | SVGエクスポート | 導入済み |
| Qt6::PrintSupport | PDF出力 | 導入済み |
| Qt6::Concurrent | バックグラウンド処理（ファイル監視、自動保存等） | 導入済み |
| Qt6::Qml | QML基盤 (WebEngine依存) | 導入済み |
| Qt6::Quick | Qt Quick (WebEngine依存) | 導入済み |
| Qt6::Network | ネットワーク通信基盤 | 導入済み |
| Qt6::OpenGL | OpenGLレンダリング基盤 (WebEngine依存) | 導入済み |
| Qt6::LinguistTools | 国際化（i18n）対応 | - |

---

## 4. 主要ライブラリ

### 4.1 C++側

| ライブラリ | 用途 | 入手 |
|-----------|------|------|
| **spdlog** | 構造化ログ出力 | vcpkg |
| **nlohmann/json** | 設定JSON読み書き・ブリッジ通信データ形式 | vcpkg |

### 4.2 JavaScript/TypeScript側（editor/ サブプロジェクト）

| ライブラリ | 用途 |
|-----------|------|
| **@tiptap/core + @tiptap/pm** | WYSIWYGエディタ本体（ProseMirrorベース） |
| **@tiptap/starter-kit** | 基本拡張セット（見出し、太字、斜体、リスト等） |
| **@tiptap/extension-table** | テーブル編集 |
| **@tiptap/extension-code-block-lowlight** | コードブロック + シンタックスハイライト |
| **@tiptap/extension-task-list** | タスクリスト（チェックボックス） |
| **@tiptap/extension-\*** | その他各種拡張（必要に応じて段階的に追加） |
| **mermaid** | ダイアグラムレンダリング（フローチャート、シーケンス図等） |
| **katex** | 数式レンダリング（LaTeX記法） |
| **highlight.js (lowlight)** | コードシンタックスハイライト |
| **@codemirror/view + lang-markdown** | ソースモード用エディタ（CodeMirror 6） |
| **vite** | ビルドツール・開発サーバ |
| **TypeScript** | 型安全性の確保 |

---

## 5. 二層アーキテクチャの理由

### 5.1 なぜWebベースエディタなのか

- **Typora自体がElectron（Chromium）ベース**であり、Web技術によるWYSIWYG Markdownエディタは実績がある
- **QTextEditでのWYSIWYG再実装は現実的でない**。カーソル管理、Selection制御、インライン装飾（太字中の斜体、リンク内のコード等）の複雑さは非常に高い
- **TipTap/ProseMirrorは10年以上の実績**があるエディタエンジンで、Notion、GitLab等の大規模サービスで使用されている
- **mermaid.js、KaTeX、highlight.js**はすべてJavaScriptライブラリであり、Web環境でネイティブに動作する。外部CLIプロセス起動が不要

### 5.2 なぜネイティブシェルなのか

- **Qt Widgetsはファイルツリー、メニュー、ダイアログ等のOS統合UIに最適**。ネイティブのルック&フィールを提供
- **ファイルシステム操作、プロセス管理、システム統合**はC++が最も適している
- 完全なElectronアプリと比較して**メモリ使用量を抑制**できる（UIシェル部分がネイティブ）

### 5.3 ブリッジの信頼性

- **QWebChannelはQt公式サポート**のC++ ↔ JavaScript通信機構で信頼性が高い
- Signal/Slotの仕組みをJavaScript側に公開でき、双方向のリアクティブ通信が可能

---

## 6. ディレクトリ構成

```
colason/
├── CMakeLists.txt           # ルートCMake
├── CMakePresets.json        # CMakeプリセット
├── vcpkg.json               # C++依存関係
├── docs/
│   └── spec/                # 設計ドキュメント
├── src/                     # C++ソース
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── app/                 # (将来のColasonApplication用、現在は空)
│   ├── ui/                  # MainWindow, MenuBarManager, QuickOpenDialog, StatusBarManager(現在未使用)
│   │   ├── sidebar/         # SidebarContainer, FileExplorer, Outline, DocumentList, FileIconProvider
│   │   └── dialogs/         # (将来のダイアログ用、現在は空)
│   ├── bridge/              # EditorBridge, OutlineBridge, ThemeBridge, SearchBridge
│   ├── core/                # DocumentManager, AutoSave, DraftRecovery, ThemeManager, GlobalSearchManager, RecentFilesManager等
│   └── utils/               # (将来のユーティリティ用、現在は空)
├── editor/                  # TypeScript/Webエディタ (サブプロジェクト)
│   ├── package.json
│   ├── tsconfig.json
│   ├── vite.config.ts
│   └── src/
│       ├── index.ts         # エントリポイント
│       ├── editor.ts        # TipTap設定・初期化
│       ├── bridge.ts        # QWebChannel JavaScript側 + colasonAPI
│       ├── source-mode.ts   # CodeMirror 6 ソースモード + Markdown↔HTML変換
│       ├── keybindings.ts   # エディタキーバインドモード (default/vim/emacs)
│       ├── extensions/      # MermaidBlock, KaTeXBlock, KaTeXInline カスタム拡張
│       └── themes/          # エディタCSSテーマ (将来用。現在はThemeManager.cpp内にC++文字列で定義)
├── resources/               # Qt リソース
│   ├── colason.qrc
│   ├── icons/
│   ├── themes/              # QSSテーマ (将来のカスタムテーマ用。現在は ThemeManager.cpp 内にC++文字列で定義)
│   └── i18n/                # 翻訳 (ja, en)
└── tests/                   # テスト
```

---

## 7. ビルド手順

### 7.1 前提条件

- **Visual Studio 18** (Community または Professional) + 「C++によるデスクトップ開発」ワークロード
- **Node.js v22+** / npm 10+
- **CMake 3.25+**

### 7.2 Qt6インストール (aqtinstall)

```bash
pip install aqtinstall
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 \
  --outputdir c:/Qt \
  --modules qtwebengine qtwebchannel qtpositioning
```

インストール先: `c:/Qt/6.8.3/msvc2022_64`

> **注**: vcpkgによるqtwebengineソースビルドは断念済み (ビルド時間1-3時間、ディスク20-30GB)。
> aqtinstall経由のプリビルドバイナリを使用する。

### 7.3 エディタ（JavaScript）ビルド

```bash
cd colason/editor
npm install
npm run build   # → editor/dist/ に出力される
```

### 7.4 C++ビルド

```bash
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
VCPKG_ROOT="<vcpkg-root>"
QT_DIR="c:/Qt/6.8.3/msvc2022_64"

"$CMAKE" -S . -B build \
  -G "Visual Studio 18 2026" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-windows \
  -DVCPKG_MANIFEST_MODE=OFF \
  -DCMAKE_PREFIX_PATH="$QT_DIR;$VCPKG_ROOT/installed/x64-windows"

"$CMAKE" --build build --config Debug
```

---

## 8. vcpkg.json

> **注**: Qt本体 (WebEngine含む) はvcpkgではなくaqtinstall経由で導入。vcpkgはC++ライブラリのみ管理。
> vcpkgソースビルドでのqtwebengine導入は**断念済み** (ビルド時間1-3時間、ディスク使用量20-30GB)。
> `C:/bt/` に残っているvcpkg buildtreesは削除してディスクを解放可能。

```json
{
    "name": "colason",
    "version-string": "0.1.0",
    "description": "Typora-compatible WYSIWYG Markdown editor",
    "dependencies": [
        "nlohmann-json",
        "spdlog",
        "gtest"
    ]
}
```

---

## 9. CMakePresets.json

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "default",
            "generator": "Visual Studio 18 2026",
            "architecture": { "value": "x64" },
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_TOOLCHAIN_FILE": "<vcpkg-root>/scripts/buildsystems/vcpkg.cmake",
                "VCPKG_TARGET_TRIPLET": "x64-windows"
            }
        }
    ]
}
```

---

## 10. リスクと対策

| リスク | 影響 | 対策 | 状態 |
|--------|------|------|------|
| ~~qtwebengine vcpkgビルド時間~~ | ~~開発遅延~~ | aqtinstallプリビルドに移行 | **解決済み** |
| QWebChannel通信レイテンシ | UI応答性の低下 | JSONメッセージの最小化、バッチ更新による通信回数削減 | 監視中 |
| TipTap拡張の複雑さ | 開発工数の増加 | MVP段階では基本拡張のみ実装し、段階的に追加 | 進行中 |
| Chromium（QWebEngine）メモリ使用量 | 非機能要件NF-004違反の可能性 | プロファイリングによる監視、不要なChromium機能の無効化 | 未検証 |
