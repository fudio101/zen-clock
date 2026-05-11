# Settings Manager

> **[AI Context]** This component provides an interface for reading and writing persistent application configurations to the Non-Volatile Storage (NVS).
> It initializes the NVS system at boot and acts as a central hub for all persistent settings like UI theme.

## Architecture

```
components/settings/
├── settings.h         ← Public API for NVS operations
├── settings.c         ← NVS initialization and read/write logic
└── CMakeLists.txt     ← ESP-IDF component registration (requires nvs_flash)
```

- **NVS Flash Init**: Handled completely inside `settings_init()`. Automatically formats the NVS partition if it gets corrupted or contains an incompatible version.
- **Keys and Namespaces**: Hardcoded internally (`zenclock` namespace) to keep the public API clean and type-safe.

## Public API

Header: [`settings.h`](settings.h)

| Function | Description |
|---|---|
| `settings_init()` | Initializes the NVS flash partition. Must be called early in `app_main()` before any reads/writes. |
| `settings_get_theme_light()` | Returns the saved theme mode (`true` for light, `false` for dark). Returns `false` by default if not set. |
| `settings_set_theme_light(is_light)` | Saves the specified theme mode to NVS and commits the change. |

## Flow

1. `app_main` calls `settings_init()`
2. `app_main` calls `settings_get_theme_light()` to get the stored theme and passes it to `ui_init()`
3. UI or hardware buttons trigger a theme change via `settings_set_theme_light(new_theme)`

## Rules for AI Agents

1. **Never call `nvs_open`/`nvs_set_*` directly** from `main.c` or other components. Use this API to maintain a clean abstraction.
2. **Add new settings as explicit getters/setters** in `settings.h` (e.g., `settings_get_timezone()`, `settings_set_timezone()`). Do not expose raw NVS handles.
3. **Always handle default values** gracefully inside the getter functions if the NVS key is not found (`ESP_ERR_NVS_NOT_FOUND`).
