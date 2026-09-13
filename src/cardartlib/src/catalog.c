#include "catalog.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "assert.h"

// GLOBAL: CARDARTLIB 0x1001d108
// GLOBAL: DRAWCARDLIB 0x10026974
undefined4 g_catalogDuplicateCheckEnabled = 0x00000000;

// GLOBAL: CARDARTLIB 0x101221d0
// GLOBAL: DRAWCARDLIB 0x10124520
CRITICAL_SECTION global_critical_section_for_catalog;

// SIZE 0xc
typedef struct CatalogEntry
{
  int key;
  uint offset;
  uint size;
} CatalogEntry;
STATIC_ASSERT(sizeof(CatalogEntry) == 0xc, CatalogEntry_wrong_size);

// SIZE 0x114
typedef struct Catalog
{
  FILE *file;
  int entry_count;
  CatalogEntry *entries;
  CatalogEntry *cached_entry;
  char path[0x104];
} Catalog;

// GLOBAL: CARDARTLIB 0x10117290
// GLOBAL: DRAWCARDLIB 0x100f2e30
// GLOBAL: DECKDLL 0x101e6f80
// GLOBAL: MAGIC 0x0094ea50
Catalog g_catalogs[5];

uint Catalog_MakeKeyFromPath(const char *path);

// MATCHING
// FUNCTION: CARDARTLIB 0x100019d0
// FUNCTION: DRAWCARDLIB 0x1000b820
int Catalog_Open(const char *catalog_path)
{
  int iVar1;
  int entry_index;
  Catalog *other_catalog;
  Catalog *catalog;
  int other_slot;
  int slot_index;
  FILE *catalog_file;

  slot_index = -1;

  for (other_slot = 0; other_slot < 5; other_slot++)
  {
    if (g_catalogs[other_slot].file == 0)
    {
      slot_index = other_slot;
      break;
    }
  }

  assert((uint)(slot_index != -1), "D:\\Newmagic\\sources\\NedCard\\Catalog.c", 0x43,
         "Too many open Catalogs: Max %d\n", 5);

  catalog = &g_catalogs[slot_index];
  catalog->cached_entry = (CatalogEntry *)0x0;
  strcpy(catalog->path, catalog_path);
  catalog->file = fopen(catalog_path, "rb");
  catalog_file = catalog->file;
  if (catalog_file == (FILE *)0x0)
    return 0;

  fread(&catalog->entry_count, 4, 1, catalog_file);
  catalog->entries = (CatalogEntry *)malloc(catalog->entry_count * sizeof(CatalogEntry));
  fread(catalog->entries, sizeof(CatalogEntry), (size_t)catalog->entry_count, catalog_file);
  if (g_catalogDuplicateCheckEnabled != 0)
  {
    for (other_slot = 0; other_slot < 5; other_slot = other_slot + 1)
    {
      if (slot_index == other_slot)
        continue;

      if (g_catalogs[other_slot].file == 0)
        continue;

      other_catalog = &g_catalogs[other_slot];

      for (entry_index = 0; 0 != catalog->entry_count; entry_index++)
      {
        for (iVar1 = 0; iVar1 < other_catalog->entry_count; iVar1 = iVar1 + 1)
        {
          assert(catalog->entries[entry_index].key - other_catalog->entries[entry_index].key != 0,
                 "D:\\Newmagic\\sources\\NedCard\\Catalog.c", 0x69,
                 "Duplicate short name found in catalogs\n%s entry %d and\n%s entry %d\nShortName value 0x%08lx", catalog->path, entry_index,
                 other_catalog->path, iVar1, catalog->entries[entry_index].key);
        }
      }
    }
  }

  return slot_index + 1;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10001c1a
// FUNCTION: DRAWCARDLIB 0x1000ba6a
bool Catalog_Close(int handle)
{
  if (g_catalogs[--handle].file == 0)
    return 0;
  free(g_catalogs[handle].entries);
  fclose(g_catalogs[handle].file);
  g_catalogs[handle].entries = (CatalogEntry *)0x0;
  g_catalogs[handle].file = (FILE *)0x0;
  g_catalogs[handle].entry_count = 0;
  return 1;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10001cd6
// FUNCTION: DRAWCARDLIB 0x1000bb26
// FUNCTION: DECKDLL 0x1000974a
// FUNCTION: MAGIC 0x0040cd3c
static int CatalogEntry_CompareKey(const int *key, const CatalogEntry *entry)
{
  if (entry->key < *key)
  {
    return 1;
  }
  else if (*key < entry->key)
  {
    return -1;
  }
  else
  {
    return 0;
  }
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10001d26
// FUNCTION: DRAWCARDLIB 0x1000bb76
// FUNCTION: DECKDLL 0x1000979a
// FUNCTION: MAGIC 0x0040cd8c
CatalogEntry *Catalog_FindEntryCached(Catalog *catalog, const char *name)
{
  CatalogEntry *entry;
  uint key;

  key = Catalog_MakeKeyFromPath(name);
  if ((catalog->cached_entry != (CatalogEntry *)0x0) && ((uint)catalog->cached_entry->key == key))
  {
    return catalog->cached_entry;
  }

  entry = (CatalogEntry *)bsearch(&key, catalog->entries, (size_t)catalog->entry_count, sizeof(CatalogEntry),
                                  (int(__cdecl *)(const void *, const void *))CatalogEntry_CompareKey);
  catalog->cached_entry = entry;
  return entry;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10001da1
// FUNCTION: DRAWCARDLIB 0x1000bbf1
// FUNCTION: DECKDLL 0x10009816
// FUNCTION: MAGIC 0x0040ce08
size_t Catalog_ReadEntry(int catalog_handle, const char *name, void **buffer)

{
  Catalog *catalog;
  CatalogEntry *entry;

  catalog = &g_catalogs[catalog_handle + -1];
  entry = Catalog_FindEntryCached(catalog, name);
  if (entry == (CatalogEntry *)0x0)
  {
    return (size_t)-1;
  }

  if (*buffer == (void *)0x0)
  {
    *buffer = malloc(entry->size + 0x10);
  }
  fseek(catalog->file, (long)entry->offset, 0);
  return fread(*buffer, 1, entry->size, catalog->file);
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10001e4a
// FUNCTION: DRAWCARDLIB 0x1000bc9a
// FUNCTION: DECKDLL 0x100098c2
// FUNCTION: MAGIC 0x0040ceb4
uint Catalog_MakeKeyFromPath(const char *path)
{
  int acc_odd;
  char ext_buf[16];
  char dir_buf[256];

  uint key;
  int pad0;
  char filename_buf[12];
  uint multiplier;
  int ch;
  int acc_even;

  multiplier = 3;
  pad0 = 0;
  acc_odd = 0;
  acc_even = 0;
  _splitpath(path, ext_buf, dir_buf, filename_buf, ext_buf);
  path = filename_buf;
  strcat(filename_buf, ext_buf);
  key = (undefined4)(((int)(signed char)path[1] ^ (int)(signed char)path[0]) << 0x18);

  while ((ch = (int)(signed char)*(path++)) != 0)
  {
    if ((multiplier & 1) != 0)
    {
      acc_odd = multiplier * ch + acc_odd;
    }
    else
    {
      acc_even = multiplier * ch + acc_even;
    }
    multiplier = multiplier + 1;
  }

  key = (undefined4)((uint)key | (acc_even * acc_odd & 0xffffffU));
  return (uint)key;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10006bb5
// FUNCTION: DRAWCARDLIB 0x10008635
BOOL Catalog_Unlock(void *unused)
{
  LeaveCriticalSection(&global_critical_section_for_catalog);

  if (unused == 0)
    return 0;
  return 0;
}
