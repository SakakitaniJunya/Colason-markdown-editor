# Colason Architecture

> ADR-004 Vertical Slice + Clean + DDD への段階移行 (Issue #5)。
> 本書は **TS 側 (editor/) の slice 構造** のみを記述する。C++ 側は別 issue / scope。

## 概要

```
editor/src/
├── index.ts                   # エントリ (DOM mount + bootstrap)
├── common/                    # 機能横断ユーティリティ
│   └── infra/
│       └── qwebchannel.ts     # Qt6 WebChannel (LGPL)
└── features/                  # Vertical Slice
    ├── editor/
    │   ├── ui/                # TipTap / DOM ハンドラ
    │   ├── application/       # use case
    │   ├── domain/            # 不変条件 / 値オブジェクト
    │   └── infra/             # 拡張ポイント (extensions/, source-mode 等)
    ├── preview/               # KaTeX / Mermaid プレビュー (ui/...)
    ├── file/                  # 保存・読込・自動保存 (infra/ に C++ FFI 集約)
    ├── theme/
    │   └── ui/themes/         # base.css 等の CSS
    └── settings/              # キーバインド / フォント / ルーラー
```

## 原則

1. **cross-slice import は `index.ts` 経由のみ** — feature の内部実装を別 feature が import しない
2. **C++ FFI 呼び出しは `infra/` 配下に集約** — UI / domain は Qt のことを知らない
3. **共通ユーティリティは `common/`** — `lib/` や `utils/` という名前は避ける (用途が広すぎる)
4. **DDD は適用先を絞る**: 編集ステート (cursor / selection / history) のみ value object 化、CSS / DOM 操作は domain に持ち込まない

## Phase 計画

| Phase | 期間目安 | スコープ | 進捗 |
|---|---|---|---|
| 1 | 1 PR | 構造作成 + 共通ファイル移動 + ARCHITECTURE.md | **本 PR (Issue #5)** |
| 2 | 2-3 PR | bridge.ts / source-mode.ts / table-context-menu.ts を features へ分配 | TODO |
| 3 | 1-2 PR | extensions/ を features/preview/ui/ + features/editor/ui/extensions/ に分割 | TODO |
| 4 | 1 PR | ESLint `no-restricted-paths` を warn → error に昇格 | TODO |

各 Phase は **既存 build / 起動が動く** ことを前提に、small commits で進める。Phase 2 以降のスコープは Phase 1 完了後に Issue 分割する。

## Phase 1 で完了する変更 (本 PR)

- `editor/src/qwebchannel.ts` → `editor/src/common/infra/qwebchannel.ts`
- `editor/src/themes/base.css` → `editor/src/features/theme/ui/themes/base.css`
- 上記 2 件の参照を `bridge.ts` / `index.html` で更新
- `features/{editor,preview,file,theme,settings}/{ui,application,domain,infra}/` のディレクトリスケルトンを作成 (`.gitkeep` 配置)
- `editor/eslint.config.js` を新設し `no-restricted-paths` ルールを **warn level** で配線 (検出のみ、Phase 4 で error 化)

## Phase 1 でやらないこと (Phase 2 以降)

- `bridge.ts` (433 lines) の slice 分割 — 利用側が広いので慎重に Phase 2 で対応
- `editor.ts` の slice 分割 — TipTap 設定の分散を慎重に検討
- `extensions/` の移動 — Phase 3 で features/preview/ui に統合
- `source-mode.ts` / `table-context-menu.ts` / `keybindings.ts` の移動

## 参考

- ADR-004: Vertical Slice + Clean + DDD (DevOps Hub `docs/adr/`)
- ai-agent-development-architecture: `~/project/ai-agent-development-architecture/`
