// SPDX-License-Identifier: MIT
// ZenClock — DJB2 hash table for O(1) SSID → password lookup

#include "wifi_priv.h"
#include <string.h>

// ============================================================
// DJB2 hash — simple, fast, good distribution for short strings
// ============================================================
static uint32_t djb2_hash(const char *str)
{
  uint32_t hash = 5381;
  int c;
  while ((c = (unsigned char)*str++))
  {
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  }
  return hash;
}

static uint32_t get_index(const char *ssid)
{
  return djb2_hash(ssid) % SSID_MAP_CAPACITY;
}

// ============================================================
// Public API
// ============================================================

void ssid_map_init(ssid_map_t *map)
{
  memset(map, 0, sizeof(ssid_map_t));
}

bool ssid_map_put(ssid_map_t *map, const char *ssid, const char *password)
{
  if (map->count >= SSID_MAP_CAPACITY)
  {
    return false; // Full
  }

  uint32_t idx = get_index(ssid);

  // Open addressing — linear probing
  for (int i = 0; i < SSID_MAP_CAPACITY; i++)
  {
    uint32_t probe = (idx + i) % SSID_MAP_CAPACITY;
    ssid_map_entry_t *e = &map->buckets[probe];

    if (!e->occupied)
    {
      // Empty slot
      strncpy(e->ssid, ssid, SSID_MAX_LEN - 1);
      e->ssid[SSID_MAX_LEN - 1] = '\0';
      strncpy(e->password, password, PASS_MAX_LEN - 1);
      e->password[PASS_MAX_LEN - 1] = '\0';
      e->occupied = true;
      map->count++;
      return true;
    }
    else if (strcmp(e->ssid, ssid) == 0)
    {
      // Update existing
      strncpy(e->password, password, PASS_MAX_LEN - 1);
      e->password[PASS_MAX_LEN - 1] = '\0';
      return true;
    }
  }

  return false; // Should not reach here if count < capacity
}

const char *ssid_map_get(const ssid_map_t *map, const char *ssid)
{
  uint32_t idx = get_index(ssid);

  for (int i = 0; i < SSID_MAP_CAPACITY; i++)
  {
    uint32_t probe = (idx + i) % SSID_MAP_CAPACITY;
    const ssid_map_entry_t *e = &map->buckets[probe];

    if (!e->occupied)
    {
      return NULL; // Empty slot = not found
    }
    if (strcmp(e->ssid, ssid) == 0)
    {
      return e->password;
    }
  }

  return NULL;
}

bool ssid_map_contains(const ssid_map_t *map, const char *ssid)
{
  return ssid_map_get(map, ssid) != NULL;
}

void ssid_map_clear(ssid_map_t *map)
{
  memset(map, 0, sizeof(ssid_map_t));
}
