# 13. 付録

## よく使う Qt クラス早見表

| Qt クラス | C# 相当 | 用途 |
|----------|---------|------|
| `QString` | `string` | 文字列 |
| `QStringList` | `List<string>` | 文字列リスト |
| `QMap<K,V>` | `Dictionary<K,V>` | 辞書 |
| `QList<T>` | `List<T>` | 汎用リスト |
| `QFile` | `FileStream` | ファイル操作 |
| `QTextStream` | `StreamReader/Writer` | テキスト読み書き |
| `QFileInfo` | `FileInfo` | ファイル情報 |
| `QDir` | `DirectoryInfo` | ディレクトリ操作 |
| `QSettings` | `Registry` / `Properties.Settings` | 設定永続化 |
| `QTimer` | `System.Timers.Timer` | タイマー |
| `QMainWindow` | `Form` | メインウィンドウ |
| `QWidget` | `Control` | 基底UIコンポーネント |
| `QSplitter` | `SplitContainer` | 分割パネル |
| `QTreeView` | `TreeView` | ツリー表示 |
| `QMenuBar` | `MenuStrip` | メニューバー |
| `QAction` | `ToolStripMenuItem` | メニュー項目 |
| `QShortcut` | `KeyBinding` | ショートカットキー |
| `QFileDialog` | `OpenFileDialog` / `SaveFileDialog` | ファイルダイアログ |
| `QMessageBox` | `MessageBox` | メッセージボックス |
| `QWebEngineView` | `WebView2` | 埋め込みブラウザ |
| `QWebChannel` | (なし, SignalR に近い) | C++↔JS 通信 |
| `QJsonDocument` | `JsonSerializer` | JSON 処理 |
| `QVariant` | `object` | 汎用型 |

---

## キーボードショートカット

| ショートカット | 機能 |
|--------------|------|
| Ctrl+N | 新規文書 |
| Ctrl+O | ファイルを開く |
| Ctrl+S | 保存 |
| Ctrl+Shift+S | 名前を付けて保存 |
| Ctrl+P | クイックオープン |
| Ctrl+Shift+1 | サイドバー: ドキュメント一覧 |
| Ctrl+Shift+2 | サイドバー: ファイルエクスプローラ |
| Ctrl+Shift+3 | サイドバー: 見出しアウトライン |
| Ctrl+B | 太字 |
| Ctrl+I | 斜体 |
| Ctrl+U | 下線 |
| Ctrl+Z | 元に戻す |
| Ctrl+Shift+Z | やり直し |
| Ctrl+= | ズームイン |
| Ctrl+- | ズームアウト |
| Ctrl+0 | ズームリセット |

---

## 用語集

| 用語 | 説明 |
|------|------|
| **TipTap** | ProseMirror をラップした WYSIWYG エディタフレームワーク |
| **ProseMirror** | TipTap の内部で使われるリッチテキストエディタエンジン |
| **CodeMirror** | テキストエディタコンポーネント (ソースモードで使用) |
| **QWebEngine** | Qt に内蔵された Chromium ベースのブラウザエンジン |
| **QWebChannel** | C++ オブジェクトを JavaScript に公開する Qt の通信プロトコル |
| **QSS** | Qt Style Sheet。CSS に似た構文で Qt ウィジェットのスタイルを定義 |
| **MOC** | Meta-Object Compiler。Qt の signals/slots を C++ で使うためのコード生成ツール |
| **FOUC** | Flash of Unstyled Content。ページ読込時に一瞬スタイルなし状態が見えること |
| **Signal** | Qt のイベント機構。C# の event に相当 |
| **Slot** | Signal を受信する関数。C# の event handler に相当 |
| **emit** | Signal を発火するキーワード。C# の Event?.Invoke() に相当 |
| **Q_OBJECT** | signals/slots を使うクラスに必要な Qt マクロ |
| **Q_INVOKABLE** | QWebChannel 経由で JS から呼び出し可能にするマクロ |
| **Vite** | TypeScript/JavaScript のバンドラー兼開発サーバー |
| **marked** | Markdown → HTML 変換ライブラリ |
| **DWM** | Desktop Window Manager。Windows のウィンドウ管理 API |

---

## 参考リンク

- [Qt 6 ドキュメント](https://doc.qt.io/qt-6/)
- [TipTap ドキュメント](https://tiptap.dev/docs)
- [ProseMirror ガイド](https://prosemirror.net/docs/guide/)
- [CodeMirror 6 ドキュメント](https://codemirror.net/docs/)
- [marked ドキュメント](https://marked.js.org/)
- [Mermaid ドキュメント](https://mermaid.js.org/)
- [KaTeX ドキュメント](https://katex.org/docs/api.html)
- [CMake ドキュメント](https://cmake.org/cmake/help/latest/)
- [vcpkg ドキュメント](https://learn.microsoft.com/en-us/vcpkg/)

---

[← 前へ: トラブルシューティング](12-troubleshooting.md) | [トップに戻る →](README.md)
