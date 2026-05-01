// Issue #5: Vertical Slice 境界の検出 (Phase 1: warn level / Phase 4 で error 化)
//
// 原則: 各 feature の内部ファイルへの直接 import を禁止し、`features/<name>/index.ts`
// 経由でのみ参照可能にする。共通ユーティリティは `common/` に集約。
//
// 例) ❌ NG: `features/editor` から `features/preview/ui/foo.ts` を import
//     ✅ OK: `features/editor` から `features/preview` を index.ts 経由で import
//     ✅ OK: 任意の場所から `common/` を import
import tseslint from '@typescript-eslint/parser';
import importPlugin from 'eslint-plugin-import';

const sliceFeatures = ['editor', 'preview', 'file', 'theme', 'settings'];

/**
 * `from` slice が `to` slice の内部 (index.ts 以外) を直接 import するのを禁止する restriction を生成。
 */
function buildRestriction() {
  const zones = [];
  for (const from of sliceFeatures) {
    for (const to of sliceFeatures) {
      if (from === to) continue;
      zones.push({
        target: `./src/features/${from}/**/*`,
        from: `./src/features/${to}/!(index).ts`,
        message: `Cross-slice deep import is forbidden: features/${from} -> features/${to}/* (use features/${to}/index.ts instead)`,
      });
    }
  }
  return zones;
}

export default [
  {
    files: ['src/**/*.{ts,tsx}'],
    languageOptions: {
      parser: tseslint,
      parserOptions: {
        ecmaVersion: 2022,
        sourceType: 'module',
      },
    },
    plugins: {
      import: importPlugin,
    },
    rules: {
      // Phase 1: warn のみ (Phase 4 で error に昇格)
      'import/no-restricted-paths': [
        'warn',
        {
          basePath: '.',
          zones: buildRestriction(),
        },
      ],
    },
  },
];
