// SPDX-License-Identifier: MIT
// ZenClock — Shared UI utilities

#include "ui_utils.h"

#include <stdio.h>

void fmt_bytes(char *buf, const size_t buf_size, const size_t bytes)
{
  if (bytes >= (size_t) 1024 * 1024)
  {
    snprintf(buf, buf_size, "%.1f MB", (float) bytes / (1024.0f * 1024.0f));
  }
  else
  {
    snprintf(buf, buf_size, "%u KB", (unsigned) (bytes / 1024u));
  }
}

int ui_circ_prev(const int index, const int count)
{
  return (index - 1 + count) % count;
}

int ui_circ_next(const int index, const int count)
{
  return (index + 1) % count;
}
