# 10. 対応 Markdown 機能一覧

## ブロック要素

| Markdown 記法 | 機能 | 実装 |
|--------------|------|------|
| `# 見出し` | 見出し H1-H6 | StarterKit |
| 段落 | 自動 | StarterKit |
| ` ```lang ``` ` | コードブロック (シンタックスハイライト付) | CodeBlockLowlight + highlight.js |
| `> 引用` | 引用ブロック | StarterKit |
| `- 項目` | 箇条書きリスト | StarterKit |
| `1. 項目` | 番号付きリスト | StarterKit |
| `- [ ] タスク` | タスクリスト (チェックボックス) | TaskList + TaskItem |
| `\| a \| b \|` | テーブル (カラムリサイズ、セル結合対応) | Table + 関連拡張 |
| `---` | 水平線 | StarterKit |
| ` ```mermaid ``` ` | Mermaid 図 (フローチャート等) | MermaidBlock (カスタム) |
| `$$ E=mc^2 $$` | KaTeX 数式ブロック | KaTeXBlock (カスタム) |

## インライン要素

| Markdown 記法 | 機能 | 実装 |
|--------------|------|------|
| `**太字**` | 太字 | StarterKit |
| `*斜体*` | 斜体 | StarterKit |
| `~~取消線~~` | 取り消し線 | StarterKit |
| `` `コード` `` | インラインコード | StarterKit |
| `[テキスト](URL)` | リンク | Link (autolink 対応) |
| `![alt](src)` | 画像 | Image (Base64 対応) |
| `==ハイライト==` | ハイライト | Highlight + marked カスタム拡張 |
| `^上付き^` | 上付き文字 | Superscript + marked カスタム拡張 |
| `~下付き~` | 下付き文字 | Subscript + marked カスタム拡張 |
| `$x^2$` | インライン数式 | KaTeXInline (カスタム) |
| `<u>下線</u>` | 下線 | Underline |

## スマート機能

| 機能 | 説明 |
|------|------|
| Markdown 貼り付け自動検出 | クリップボードのテキストが Markdown っぽければ自動変換 |
| 画像ドラッグ&ドロップ | 画像ファイルを assets フォルダにコピーして挿入 |
| 画像クリップボード貼り付け | クリップボードの画像を挿入 |
| 自動リンク | URL をリンクに自動変換 |
| タイポグラフィ | `--` → em dash 等の自動変換 |
| ソフト改行 | Shift+Enter |

---

[← 前へ: テーマシステム](09-theme-system.md) | [次へ: 開発タスク別ガイド →](11-dev-guide.md)
