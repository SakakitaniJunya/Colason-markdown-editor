/**
 * Application settings persisted to localStorage.
 * Issue #2: includes autoSave toggle (default OFF).
 */

const STORAGE_KEY = 'colason.settings.v1';

export interface AppSettings {
  /** When true, the editor saves the current file every autoSaveIntervalMs while dirty. Default OFF. */
  autoSave: boolean;
  /** Auto-save interval in milliseconds. */
  autoSaveIntervalMs: number;
  /** Crash recovery snapshot interval in milliseconds (always on, even when autoSave is off). */
  recoveryIntervalMs: number;
}

const DEFAULT_SETTINGS: AppSettings = {
  autoSave: false,
  autoSaveIntervalMs: 10_000,
  recoveryIntervalMs: 5_000,
};

let cached: AppSettings | null = null;
const listeners = new Set<(s: AppSettings) => void>();

export function getSettings(): AppSettings {
  if (cached) return cached;
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) {
      const parsed = JSON.parse(raw) as Partial<AppSettings>;
      cached = { ...DEFAULT_SETTINGS, ...parsed };
      return cached;
    }
  } catch (err) {
    console.warn('[Colason] Failed to read settings:', err);
  }
  cached = { ...DEFAULT_SETTINGS };
  return cached;
}

export function updateSettings(patch: Partial<AppSettings>): AppSettings {
  const next = { ...getSettings(), ...patch };
  cached = next;
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(next));
  } catch (err) {
    console.warn('[Colason] Failed to persist settings:', err);
  }
  listeners.forEach((l) => {
    try {
      l(next);
    } catch (err) {
      console.error('[Colason] settings listener threw:', err);
    }
  });
  return next;
}

export function onSettingsChange(listener: (s: AppSettings) => void): () => void {
  listeners.add(listener);
  return () => listeners.delete(listener);
}
