// SPDX-License-Identifier: MIT
// ZenClock — Shared UI utilities

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** Format byte count as human-readable KB or MB. */
  void fmt_bytes(char *buf, size_t buf_size, size_t bytes);

  /** Circular previous: (index - 1 + count) % count. */
  int ui_circ_prev(int index, int count);

  /** Circular next: (index + 1) % count. */
  int ui_circ_next(int index, int count);

#ifdef __cplusplus
}
#endif
