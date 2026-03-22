# 8. データフロー図解

## ファイルを開く

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant MW as MainWindow
    participant DM as DocumentManager
    participant JS as colasonAPI (JS)
    participant TT as TipTap

    User->>MW: Ctrl+O
    MW->>MW: QFileDialog でファイル選択
    MW->>DM: openDocument(path)
    DM->>DM: QFile で UTF-8 読込
    DM-->>MW: content (Markdown)
    MW->>JS: runJavaScript("setMarkdown(...)")
    JS->>JS: simpleMarkdownToHtml(md)
    JS->>TT: editor.commands.setContent(html)
    TT-->>User: ドキュメント表示
```

---

## ファイルを保存する

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant MW as MainWindow
    participant JS as colasonAPI (JS)
    participant DM as DocumentManager
    participant DR as DraftRecoveryManager

    User->>MW: Ctrl+S
    MW->>JS: runJavaScript("getMarkdown()")
    JS->>JS: htmlToSimpleMarkdown(editor.getHTML())
    JS-->>MW: markdown (コールバック / 非同期)
    MW->>DM: saveDocument(markdown)
    DM->>DM: QFile で UTF-8 書込
    MW->>DR: deleteAllDrafts()
    MW->>MW: updateTitle() → "*" 削除
```

---

## エディタ内容変更時

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant TT as TipTap
    participant BR as bridge.ts
    participant EB as EditorBridge
    participant OB as OutlineBridge
    participant DM as DocumentManager
    participant SC as SidebarContainer
    participant MW as MainWindow

    User->>TT: 文字入力
    TT->>BR: onUpdate コールバック
    par 並列通知
        BR->>EB: contentChanged(html)
        EB->>DM: setContent(html)
    and
        BR->>EB: documentDirty(true)
        EB->>DM: setDirty(true)
        EB->>MW: updateTitle() → "*" 追加
    and
        BR->>EB: wordCountChanged(words, chars)
    and
        BR->>OB: headingsChanged(json)
        OB->>SC: updateOutline(json)
    end
```

---

## テーマ変更

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant MW as MainWindow
    participant PM as PreferencesManager
    participant TM as ThemeManager
    participant App as qApp (Qt)
    participant DWM as Win32 DWM
    participant JS as Web エディタ

    User->>MW: メニューで "Dark" 選択
    MW->>PM: setTheme("dark")
    PM->>TM: emit themeChanged("dark")
    TM->>TM: テーマデータ取得 (css, qss)
    TM->>MW: emit themeChanged("dark", css, qss)
    par 並列適用
        MW->>App: setStyleSheet(qss) → Qt UI
    and
        MW->>DWM: DwmSetWindowAttribute → タイトルバー色
    and
        MW->>JS: runJavaScript → style 要素更新
    end
```

---

## 自動保存

```mermaid
sequenceDiagram
    participant ASM as AutoSaveManager
    participant MW as MainWindow
    participant JS as colasonAPI (JS)
    participant DM as DocumentManager
    participant SB as StatusBar

    Note over ASM: 5分間隔タイマー発火
    ASM->>MW: emit getContentRequested()
    MW->>JS: runJavaScript("getContent()")
    JS-->>MW: html (コールバック)
    MW->>ASM: onContentReceived(html)
    ASM->>DM: saveDocument(html)
    ASM->>SB: emit autoSaved()
    SB->>SB: "Auto-saved" 表示
```

---

## 画像ドラッグ&ドロップ

```mermaid
sequenceDiagram
    actor User as ユーザー
    participant MW as MainWindow
    participant IM as ImageManager
    participant JS as colasonAPI (JS)
    participant TT as TipTap

    User->>MW: 画像ファイルをドロップ
    MW->>MW: dropEvent() → 拡張子判定
    MW->>IM: handleImageDrop(path, docDir)
    IM->>IM: assets/ にコピー + 一意なファイル名
    IM-->>MW: 相対パス
    MW->>JS: executeCommand("insertImage", {src, alt})
    JS->>TT: editor.chain().setImage({src, alt}).run()
    TT-->>User: 画像表示
```

---

## 起動時のドラフト復旧

```mermaid
flowchart TD
    A["アプリ起動"] --> B["500ms 待機"]
    B --> C["checkDraftRecovery()"]
    C --> D{"ドラフトあり?"}
    D -->|No| E["通常起動"]
    D -->|Yes| F["QMessageBox 表示<br/>未保存のドラフトがあります"]
    F --> G{"ユーザーの選択"}
    G -->|Yes| H["readDraft()<br/>→ setEditorHtml(content)<br/>→ setDirty(true)"]
    G -->|No| I["deleteAllDrafts()"]
    G -->|Ignore| J["何もしない<br/>次回起動時に再表示"]
```

---

[← 前へ: ブリッジ通信](07-bridge.md) | [次へ: テーマシステム →](09-theme-system.md)
