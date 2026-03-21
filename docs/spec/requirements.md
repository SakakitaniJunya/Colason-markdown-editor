# Colason - 要件定義書

> Typora互換 WYSIWYGマークダウンエディタ

## 1. プロジェクト概要

### 1.1 プロジェクト名
**Colason** (コラソン)

### 1.2 目的
Typoraと同等のUI・機能を持つWYSIWYGマークダウンエディタを開発する。
Mermaid.js等のダイアグラム描画、エクスプローラツリー、目次ツリーを含む全機能を再現する。

### 1.3 対象プラットフォーム
- Windows 10/11 (優先)
- macOS, Linux (将来対応)

---

## 2. 機能要件

### 2.1 コアエディタ機能

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| F-001 | WYSIWYGライブプレビュー | 必須 | Markdown記法を入力と同時にインラインレンダリング。分割ペインなし | **実装済み** -- TipTap/ProseMirror |
| F-002 | ソースコードモード | 必須 | `Ctrl+/` でMarkdownソースとWYSIWYG表示を切替 | **実装済み** -- CodeMirror 6 (`source-mode.ts`) |
| F-003 | フォーカスモード | 必須 | 現在の段落のみハイライトし、他をフェードアウト | **実装済み** -- `EditorBridge::toggleFocusMode()` |
| F-004 | タイプライターモード | 必須 | アクティブ行を画面中央に固定 | **実装済み** -- `EditorBridge::toggleTypewriterMode()` |
| F-005 | スペルチェック | 中 | 英語・日本語対応のスペルチェック | 未実装 |
| F-006 | 文字数・単語カウント | 必須 | ステータスバーは現在非表示。将来的に復活予定 | **実装済み** -- `wordCountChanged` シグナル。StatusBarManager は未使用 |
| F-007 | 検索・置換 | 必須 | `Ctrl+F` / `Ctrl+H` によるドキュメント内検索・置換 | **実装済み** -- `SearchBridge` + JS側検索UI |
| F-008 | グローバル検索 | 必須 | `Ctrl+Shift+F` でフォルダ全体を横断検索 | **実装済み** -- `GlobalSearchManager` |
| F-009 | Markdownペースト | 必須 | クリップボードのMarkdownテキストをWYSIWYG要素に自動変換して貼り付け | **実装済み** -- `looksLikeMarkdown()` 検出 + 自動変換 |

### 2.2 Markdown対応

#### 2.2.1 標準Markdown / GFM

| ID | 機能 | 優先度 | 状態 |
|----|------|--------|------|
| M-001 | 見出し (ATX / Setext) | 必須 | **実装済み** -- TipTap Heading + StarterKit |
| M-002 | 太字・斜体・取り消し線 | 必須 | **実装済み** -- StarterKit + Strike |
| M-003 | 順序付き/無し リスト | 必須 | **実装済み** -- StarterKit |
| M-004 | タスクリスト (チェックボックス) | 必須 | **実装済み** -- TaskList + TaskItem |
| M-005 | 引用ブロック | 必須 | **実装済み** -- StarterKit Blockquote |
| M-006 | コードブロック (フェンス/インデント) | 必須 | **実装済み** -- CodeBlockLowlight |
| M-007 | シンタックスハイライト (~100言語) | 必須 | **実装済み** -- highlight.js (lowlight) 全言語 |
| M-008 | インラインコード | 必須 | **実装済み** -- StarterKit Code |
| M-009 | 水平線 | 必須 | **実装済み** -- StarterKit HorizontalRule |
| M-010 | リンク・画像 | 必須 | **実装済み** -- Link + Image |
| M-011 | テーブル (GFM形式) | 必須 | **実装済み** -- Table + TableRow + TableHeader + TableCell |
| M-012 | 脚注 | 必須 | 未実装 |
| M-013 | YAML Front Matter | 必須 | 未実装 |

#### 2.2.2 拡張Markdown

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| M-100 | 数式 (LaTeX/KaTeX) | 必須 | `$$...$$` ブロック数式、`$...$` インライン数式 | **実装済み** -- `katex-block.ts` + `katex-inline.ts` |
| M-101 | Mermaidダイアグラム | **必須** | フローチャート、シーケンス図、ガントチャート、マインドマップ、タイムライン、ER図、状態遷移図 | **実装済み** -- `mermaid-block.ts` + mermaid.js |
| M-102 | シーケンス図 (js-sequence) | 中 | js-sequence互換 | 未実装 (Mermaid で代替可能) |
| M-103 | フローチャート (flowchart.js) | 中 | flowchart.js互換 | 未実装 (Mermaid で代替可能) |
| M-104 | 目次 (TOC) | 必須 | `[toc]` タグで自動生成 | 未実装 |
| M-105 | 上付き・下付き文字 | 必須 | `^上付き^` / `~下付き~` | **実装済み** -- Superscript + Subscript 拡張 |
| M-106 | ハイライト | 必須 | `==ハイライト==` | **実装済み** -- Highlight 拡張 |
| M-107 | 絵文字 | 必須 | `:emoji:` 記法のオートコンプリート | 未実装 |
| M-108 | Callout / アラート | 中 | GitHub形式 `> [!NOTE]` 等 | 未実装 |
| M-109 | インラインHTML | 必須 | HTML直接記述のレンダリング | 未実装 |

### 2.3 ダイアグラム機能 (独自要件: 詳細)

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| D-001 | Mermaid フローチャート | 必須 | `graph TD/LR` 記法 | **実装済み** -- mermaid.js 全図種対応 |
| D-002 | Mermaid シーケンス図 | 必須 | `sequenceDiagram` | **実装済み** |
| D-003 | Mermaid ガントチャート | 必須 | `gantt` | **実装済み** |
| D-004 | Mermaid クラス図 | 必須 | `classDiagram` | **実装済み** |
| D-005 | Mermaid 状態遷移図 | 必須 | `stateDiagram-v2` | **実装済み** |
| D-006 | Mermaid ER図 | 必須 | `erDiagram` | **実装済み** |
| D-007 | Mermaid マインドマップ | 必須 | `mindmap` | **実装済み** |
| D-008 | Mermaid タイムライン | 必須 | `timeline` | **実装済み** |
| D-009 | Mermaid パイチャート | 必須 | `pie` | **実装済み** |
| D-010 | Mermaid Git グラフ | 中 | `gitGraph` | **実装済み** |
| D-011 | ダイアグラムのライブプレビュー | 必須 | コードブロック編集中にリアルタイムレンダリング | **実装済み** -- NodeView + 300ms デバウンス |
| D-012 | ダイアグラムのSVGエクスポート | 中 | 個別ダイアグラムをSVG/PNG保存 | **実装済み** -- `diagramExportReady` シグナル |
| D-013 | ダイアグラムのズーム・パン | 中 | 大きな図のスクロール・拡大操作 | 未実装 |

### 2.4 サイドバー・ナビゲーション (独自要件: 詳細)

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| N-001 | エクスプローラツリー | **必須** | フォルダ階層表示。ドラッグ&ドロップ移動対応 | **基本実装済み** -- `FileExplorerPanel` + `QFileSystemModel`。カスタムSVGアイコン (`FileIconProvider`: file, file-text, folder, folder-open)。プレースホルダページ ("No folder opened" + Open Folder ボタン)。フォルダヘッダ (大文字フォルダ名表示)。D&D移動は未実装 |
| N-002 | ファイルリスト | 必須 | フラットなファイル一覧表示 | **実装済み** -- `DocumentListPanel` |
| N-003 | 目次 (アウトライン) ツリー | **必須** | ドキュメント見出し構造をツリー表示 | **実装済み** -- `OutlinePanel` |
| N-004 | 目次のクリック遷移 | **必須** | 目次項目クリックで該当箇所にスクロール | **実装済み** -- `colasonAPI.scrollToHeading()` 経由 |
| N-005 | 目次の現在位置ハイライト | **必須** | スクロール位置に応じてアクティブ見出しを自動ハイライト | **実装済み** -- `OutlineBridge::activeHeadingChanged` |
| N-006 | 目次の折りたたみ | 必須 | 階層的な展開/折りたたみ | 未実装 |
| N-007 | 目次のキーワード検索 | 必須 | アウトラインのフィルタリング | 未実装 |
| N-008 | サイドバー切替ショートカット | 必須 | `Ctrl+Shift+1/2/3` でパネル切替 | **実装済み** -- `MainWindow::setupUI()` でショートカット登録。タブバー表示、デフォルト幅220px |
| N-009 | ファイルツリーの外部変更監視 | 必須 | ファイルシステム変更をリアルタイム反映 | **実装済み** -- `QFileSystemModel` がOS側の変更を自動検知 |
| N-010 | クイックオープン | 必須 | `Ctrl+P` でファジー検索によるファイル切替 | **実装済み** -- `QuickOpenDialog` |
| N-011 | エクスプローラのコンテキストメニュー | 必須 | 新規ファイル、リネーム、削除、コピー等 | **部分実装** -- 「Open in File Manager」「Copy Path」のみ。新規ファイル/リネーム/削除は未実装 |
| N-012 | ファイルソート | 中 | 名前/更新日/作成日でソート | 未実装 |

### 2.5 ファイル管理

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| FM-001 | フォルダを開く | 必須 | プロジェクトフォルダの読み込み | **実装済み** -- `FileExplorerPanel` |
| FM-002 | 最近使ったファイル/フォルダ | 必須 | 履歴管理 | **実装済み** -- `RecentFilesManager` + メニュー表示 |
| FM-003 | オートセーブ | 必須 | 設定可能なインターバル (デフォルト5分) | **実装済み** -- `AutoSaveManager` (300秒) |
| FM-004 | 未保存ドラフト復元 | 必須 | クラッシュ時の復元機能 | **実装済み** -- `DraftRecoveryManager` |
| FM-005 | 画像のドラッグ&ドロップ挿入 | 必須 | 画像ファイルの自動コピーとパス挿入 | **実装済み** -- `ImageManager` |
| FM-006 | 画像パスの設定 | 中 | 相対/絶対パス、カスタムフォルダ | **実装済み** -- `PreferencesManager::imageAssetSubfolder()` |
| FM-007 | 内部ファイルリンク | 中 | 他のMarkdownファイルへのリンク・遷移 | 未実装 |

### 2.6 エクスポート・インポート

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| E-001 | PDF出力 | 必須 | ブックマーク付き | **実装済み** -- `ExportManager::exportToPdf()` |
| E-002 | HTML出力 (スタイル付き) | 必須 | テーマCSS込みのHTML | **実装済み** -- `ExportManager::exportToHtml(includeStyle=true)` |
| E-003 | HTML出力 (スタイルなし) | 中 | 素のHTML | **実装済み** -- `ExportManager::exportToHtml(includeStyle=false)` |
| E-004 | 画像出力 (PNG) | 中 | ドキュメント全体を画像化 | **実装済み** -- `ExportManager::exportToPng()` |
| E-005 | Word (.docx) 出力 | 低 | Pandoc連携 | 未実装 |
| E-006 | .docx / .tex 等のインポート | 低 | Pandoc連携 | 未実装 |

### 2.7 テーマ・外観

| ID | 機能 | 優先度 | 説明 | 状態 |
|----|------|--------|------|------|
| T-001 | 組み込みテーマ (7種) | 必須 | ライト系3種+ダーク系4種 | **実装済み** -- Light, Dark, GitHub Light, GitHub Dark, Sepia, Nord, Dracula の7テーマ。全テーマで hljs シンタックスハイライト、mermaid ブロック、katex ブロック/インライン、カスタム chevron アイコン (ダーク用/ライト用SVG) のスタイルを完備。View > Theme サブメニューから選択可能 |
| T-002 | CSSベースのカスタムテーマ | 必須 | ユーザーCSSファイル読み込み | **実装済み** -- `ThemeManager::loadCustomTheme()` |
| T-003 | ダーク/ライトモード自動切替 | 必須 | OS設定に連動 | **実装済み** -- `QStyleHints::colorSchemeChanged` 監視 + `ThemeManager::detectSystemTheme()` |
| T-004 | ユーザーCSS上書き | 中 | `base.user.css` による全テーマ共通カスタマイズ | 未実装 |

### 2.8 ショートカットキー

| 操作 | キー |
|------|------|
| ソースコード切替 | `Ctrl+/` |
| 太字 | `Ctrl+B` |
| 斜体 | `Ctrl+I` |
| 下線 | `Ctrl+U` |
| 取り消し線 | `Alt+Shift+5` |
| 見出し1-6 | `Ctrl+1` ~ `Ctrl+6` |
| 段落 | `Ctrl+0` |
| リスト | `Ctrl+Shift+]` |
| タスクリスト | `Ctrl+Shift+X` |
| コードブロック | `Ctrl+Shift+K` |
| 数式ブロック | `Ctrl+Shift+M` |
| 表の挿入 | `Ctrl+T` |
| リンクの挿入 | `Ctrl+K` |
| 画像の挿入 | `Ctrl+Shift+I` |
| 検索 | `Ctrl+F` |
| 置換 | `Ctrl+H` |
| グローバル検索 | `Ctrl+Shift+F` |
| クイックオープン | `Ctrl+P` |
| アウトライン | `Ctrl+Shift+1` |
| ファイルリスト | `Ctrl+Shift+2` |
| ファイルツリー | `Ctrl+Shift+3` |
| フルスクリーン | `F11` |
| 全ショートカットはカスタマイズ可能 | 設定ファイルで変更 |

---

## 3. 非機能要件

| ID | 項目 | 要件 |
|----|------|------|
| NF-001 | 起動時間 | 3秒以内 (コールドスタート) |
| NF-002 | 入力レスポンス | 16ms以下 (60fps) |
| NF-003 | ファイルサイズ | 10MB以上のMarkdownファイルを扱える |
| NF-004 | メモリ使用量 | アイドル時 200MB以下 |
| NF-005 | インストーラ | NSIS / MSI形式 |
| NF-006 | 自動アップデート | アプリ内で通知・更新 |
| NF-007 | アクセシビリティ | キーボード完全操作、スクリーンリーダー基本対応 |
| NF-008 | i18n | 日本語・英語の多言語対応 |

---

## 4. 制約事項

- ライセンス: オープンソース (MIT License)
- Typoraとの完全互換を目指すが、独自の拡張も許容する
- v1.0ではWindows対応を優先し、macOS/Linuxは後続リリースで対応

---

## 5. 開発フェーズ

| Phase | 範囲 | 目標 | 状態 |
|-------|------|------|------|
| **Phase 0: 環境構築** | CMake + Vite + QWebEngine + QWebChannel | ビルド環境確立 | **完了** |
| **Phase 1: エディタコア (MVP)** | TipTap + Bridge + CodeMirror + 基本Markdown | 基本的な編集が可能 | **完了** |
| **Phase 2: サイドバー** | FileExplorer + Outline + DocumentList | サイドバーナビゲーション | **完了** -- FileExplorerPanel, OutlinePanel, DocumentListPanel 実装済み |
| **Phase 3: ダイアグラム・数式** | Mermaid.js + KaTeX | 視覚的要素の対応 | **完了** -- mermaid-block, katex-block, katex-inline 実装済み |
| **Phase 4: ファイル管理・検索** | DocumentManager + Search + AutoSave + DraftRecovery | 実用レベルのファイル管理 | **完了** -- 全 Manager クラス実装済み |
| **Phase 5: エクスポート・テーマ** | PDF/HTML/PNG出力 + 6テーマ + OS連動 | 出力機能の充実 | **完了** -- ExportManager + ThemeManager (6テーマ) 実装済み |
| **Phase 6: 仕上げ** | i18n + パフォーマンス + インストーラ + 未実装機能 | プロダクション品質 | 未着手 -- 脚注、YAML FrontMatter、TOC、絵文字、スペルチェック等が残存 |