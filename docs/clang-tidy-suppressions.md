# Clang-Tidy Warning Suppressions

Reference for suppressing warnings in this ESP32 embedded C project.

---

## Suppression Methods

### 1. Inline — single line

```c
int x = foo(); // NOLINT(check-name)
```

### 2. Next line

```c
// NOLINTNEXTLINE(check-name)
void my_function(void *arg)
```

Use when the comment cannot go on the same line (e.g. multi-line function signatures).

### 3. Unknown check — suppress all on that line

```c
for (;;) // NOLINT
```

### 4. Global — disable in `.clang-tidy`

```yaml
Checks: "-*,...,-check-name"
```

Use for checks that don't apply to the entire project.

---

## Finding the Check Name

The check name always appears in `[...]` at the end of the warning:

```
wifi_manager.c:42:5: warning: message [bugprone-infinite-loop]
```

Copy that name into `NOLINT(...)`.

---

## Common Warnings

### Endless loop

**Cause:** Intentional `for (;;)` or `while (1)` in FreeRTOS tasks.

**Inline fix:**
```c
for (;;) // NOLINT
{
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

**Note:** `bugprone-infinite-loop` is already disabled globally in `.clang-tidy`. If the warning still appears, check the exact name from the diagnostic and add NOLINT.

---

### Unused parameter — `misc-unused-parameters`

**Cause:** FreeRTOS callbacks require `void *arg` in their signature.

**Fix:** cast to void in the body:
```c
static void my_task(void *arg)
{
    (void)arg;
}
```

---

### Parameter cannot be const — `readability-non-const-parameter`

**Cause:** Callback signature dictated by an external API.

**Inline fix:**
```c
static void on_event(void *arg) // NOLINT(readability-non-const-parameter)
```

---

### Function too complex — `readability-function-cognitive-complexity`

**Cause:** State machines, event handlers, FreeRTOS task loops.

**Inline fix:**
```c
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void wifi_task(void *arg)
```

**Current threshold:** 50 (configured in `.clang-tidy`).

---

### Narrowing conversion — `bugprone-narrowing-conversions`

**Cause:** `strlen()` returns `size_t` (unsigned) passed to an `int` parameter.

**Fix:** explicit cast:
```c
func(..., (int)strlen(s_password), ...);
```

---

### Parameter always equals a constant value

**Cause:** Internal function called from one place with a fixed constant.

**Inline fix at call site:**
```c
ap_count = do_aggregated_scan(s_ap_list, MAX_UNIQUE_APS); // NOLINT
```

**Better fix:** remove the parameter and use the constant directly inside the function.

---

### Enum zero-value init — `*-invalid-enum-default-initialization`

**Cause:** ESP-IDF struct partially initialized; unset enum fields zero-initialized, but the enum has no `0` enumerator.

**Two patterns:**

1. `deconfigure = true` path — other fields irrelevant, no valid value to assign:
```c
// NOLINTNEXTLINE(*-invalid-enum-default-initialization)
const ledc_timer_config_t timer_cfg = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num  = handle->timer_num,
    .deconfigure = true,
};
```

2. Default/optional field (e.g. `clk_src = 0` means "use default"):
```c
// NOLINTNEXTLINE(*-invalid-enum-default-initialization)
const adc_oneshot_unit_init_cfg_t adc_cfg = {
    .unit_id = BAT_ADC_UNIT,
};
```

**Note:** Both are false positives — ESP-IDF uses `0` as an implicit "default" sentinel that isn't a named enumerator.

---

### Null pointer dereference — `clang-analyzer-core.NullDereference`

**Cause:** `void *data` from an ESP event handler can be NULL.

**Fix:** add a null guard:
```c
if (!data) { break; }
const wifi_sta_config_t *cfg = (wifi_sta_config_t *)data;
```

---

### Identifier case style — `readability-identifier-naming`

**Cause:** Variable or function name does not match the configured convention.

**Current config** (`.clang-tidy`):
- Functions, variables: `lower_case`
- Macros, enum constants: `UPPER_CASE`
- Static constants: `UPPER_CASE`

**Fix:** rename to match convention, or suppress:
```c
int myVar; // NOLINT(readability-identifier-naming)
```

---

## Do Not Suppress

| Warning | Reason |
|---|---|
| `cert-err34-c` | Detects string-to-number conversion errors |
| `cert-flp30-c` | Float loop counters cause undefined behavior |
| `clang-analyzer-core.NullDereference` | Real crash on device |
| `bugprone-narrowing-conversions` | Silent data loss |
