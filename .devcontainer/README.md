# Colason Devcontainer (Codespaces)

Mac ローカルに Qt6 + QtWebEngine (10-15GB) を入れずに開発するための Codespaces 設定。

## 使い方

1. <https://github.com/SakakitaniJunya/Colason-markdown-editor> を開く
2. **Code → Codespaces → Create codespace on main**
3. 初回ビルドで 5-8 分待つ (Dockerfile の apt + Qt6 取得)
4. ターミナルで `claude` を実行して認証

## 構成

| ファイル | 役割 |
|---|---|
| `devcontainer.json` | VSCode 拡張・ポート転送・ホスト要件 (2-core / 4GB / 32GB ストレージ) |
| `Dockerfile` | Ubuntu 22.04 + apt で Qt6 base/webengine/svg + clang/cmake/ninja |
| `post-create.sh` | Claude Code 導入 / editor/ npm install / cmake configure |

## 容量内訳 (実測値の見込み)

| 内訳 | 容量 |
|---|---|
| Ubuntu base + dev tools | ~600MB |
| Qt6 base/webengine/tools (apt) | ~1.5GB |
| Node.js + npm deps (editor/) | ~300MB |
| ビルド成果物 (build/) | ~500MB |
| 合計 | **~3GB** (Codespaces 32GB ストレージに余裕) |

## 無料枠の使い方

- 個人アカウント: **60h/月** (2-core 4GB マシン)
- `gh codespace stop` で停止 → 課金止まる
- `gh codespace list --json name,state,lastUsedAt` で状態確認
- 30 日無操作で自動削除 (設定で 7 日に短縮可能)

## UI 動作確認

QtWebEngine ベースなので Codespaces 内で起動しても画面は見えない。3 つの選択肢:

1. **オフスクリーンテスト**: `QT_QPA_PLATFORM=offscreen` で起動 → ログだけで挙動検証 (CI 向け)
2. **VS Code Desktop で接続**: Codespaces のアプリを Mac の VSCode で開き、デスクトップアプリの起動確認は Mac 側に X11 (XQuartz) で転送 → やや手間
3. **GitHub Actions でクロスビルド**: タグ push で macOS/Windows/Linux のバイナリを生成 (これが本筋)

## トラブルシュート

- **CMake が Qt6 を見つけない**: `Dockerfile` の apt が走り終わっているか確認 → `dpkg -l qt6-base-dev`
- **WebEngine 関連エラー**: Codespaces のメモリが足りない可能性 → 4-core 8GB に上げる (無料枠 30h/月に減る)
- **vcpkg を使いたい**: `VCPKG_ROOT` 環境変数を export すれば CMakeLists.txt の vcpkg toolchain が有効化される
