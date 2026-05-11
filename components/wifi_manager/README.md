# WiFi Manager

> **[AI Context]** This component handles WiFi connectivity with multi-credential support.
> State-machine driven, persistent-task architecture with infinite retry.
> Credentials are provisioned from a CSV file at build time, stored in NVS,
> and matched against scanned networks using a DJB2 hash map for O(1) lookup.

## Architecture

```
wifi_manager/
├── include/wifi_manager.h     ← Public API (the ONLY header consumers include)
├── priv_include/wifi_priv.h   ← Internal: hash table + NVS store + constants
├── src/
│   ├── wifi_manager.c         ← Core: state machine, persistent task, DNS probe
│   ├── wifi_cred_store.c      ← NVS credential CRUD + provisioning from compiled array
│   └── wifi_ssid_map.c        ← DJB2 hash table for O(1) SSID → password lookup
└── CMakeLists.txt
```

## State Machine

```
IDLE ──start()──→ SCANNING ──match──→ CONNECTING ──got IP──→ VERIFYING ──DNS OK──→ CONNECTED
                     ↑  ↑                 │                      │                    │
                     │  │    all failed    │         disconnected │       disconnect   │
                     │  └─────────────────┘←─────────────────────┘                    │
                     │                                                                │
                     │                    RECONNECTING ←───────────────────────────────┘
                     │                    ├─ retry ×3 (same SSID, 2s/4s/8s backoff)
                     └────────────────────┤
                                          └─ 3× failed → full re-scan
```

- **Infinite retry**: SCANNING never gives up. Backoff: 2s → 4s → 8s → 16s → 30s (cap).
- **VERIFYING**: DNS probe (`getaddrinfo`) after Got IP — confirms internet works
  before declaring CONNECTED. Avoids premature SNTP/API calls.
- **Reconnect**: Lost WiFi → retry same SSID 3×, then full re-scan.
- **Only `wifi_manager_stop()` returns to IDLE.**

## Credential Flow

```
wifi_config.csv (user-edited, gitignored)
        │
        ▼  scripts/gen_wifi_creds.py (PlatformIO pre-build)
include/wifi_credentials.gen.h (auto-generated, gitignored)
        │
        ▼  Compiled into firmware
wifi_cred_store_provision()
        │
        ├── Compare WIFI_CRED_HASH with NVS stored hash
        ├── If different → re-provision NVS
        └── If same → skip (already provisioned)
```

## Public API

| Function | Description |
|---|---|
| `wifi_manager_init()` | Init NVS, netif, WiFi driver, create persistent task |
| `wifi_manager_start()` | Wake task → scan → match → connect (infinite retry) |
| `wifi_manager_is_connected()` | True only when state == CONNECTED |
| `wifi_manager_stop()` | Stop WiFi, return task to IDLE |
| `wifi_manager_set_callback(cb)` | Set event callback |
| `wifi_manager_get_state()` | Get current state machine state |
| `wifi_manager_get_ssid()` | Get connected SSID (or NULL) |
| `wifi_cred_add(ssid, pass)` | Add credential to NVS at runtime |
| `wifi_cred_remove(ssid)` | Remove credential from NVS |

## Events

| Event | When |
|---|---|
| `WIFI_MGR_CONNECTING` | Trying a candidate network |
| `WIFI_MGR_GOT_IP` | Got IP, verifying internet (DNS probe) |
| `WIFI_MGR_CONNECTED` | Verified online (DNS probe OK) |
| `WIFI_MGR_DISCONNECTED` | Lost connection |
| `WIFI_MGR_RECONNECTING` | Retrying same SSID before re-scan |
| `WIFI_MGR_SCAN_DONE` | WiFi scan complete |
| `WIFI_MGR_NO_MATCH` | No scan result matched stored credentials |
| `WIFI_MGR_ALL_FAILED` | One scan cycle failed — retrying (informational) |

## States

| State | Description |
|---|---|
| `WIFI_ST_IDLE` | Initialized, waiting for `start()` |
| `WIFI_ST_SCANNING` | Running aggregated scan + credential matching |
| `WIFI_ST_CONNECTING` | Trying candidate APs sequentially |
| `WIFI_ST_VERIFYING` | Got IP, running DNS probe |
| `WIFI_ST_CONNECTED` | Online and operational |
| `WIFI_ST_RECONNECTING` | Lost connection, retrying same SSID |

## Rules for AI Agents

1. **Never call `esp_wifi_*` directly** from main.c — use this API.
2. **Credentials** are managed via `wifi_config.csv` (pre-build) or `wifi_cred_add()` (runtime).
3. **Event callback** fires from background task context — use `lvgl_port_lock()` before UI updates.
4. **`wifi_priv.h`** is internal — do not include from outside this component.
5. **Task is persistent** — created once in `init()`, never deleted. Use `start()`/`stop()` to control.
6. **CONNECTED means verified** — DNS probe passed, internet is working.
