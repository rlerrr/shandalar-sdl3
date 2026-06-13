#ifndef CATALOG_H
#define CATALOG_H

#include "platform/gdi_compat.h"
#include "platform/legacy_io.h"
#include "inttypes.h"
#include <stddef.h>
#include "mystdbool.h"

int Catalog_Open(const char *catalog_path);
size_t Catalog_ReadEntry(int catalog_id, const char *entry_name, void **out_data);
BOOL Catalog_Unlock(void *unused);

extern CRITICAL_SECTION global_critical_section_for_catalog;

#endif
