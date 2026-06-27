#include "platform/asset_manager.h"

#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <errno.h>
#include <sys/stat.h>
#endif

#define ASSET_REQUEST_LIMIT 256

typedef struct AssetRequest {
  char relative_path[260];
  ShandalarAssetState state;
} AssetRequest;

static char g_asset_root[512] = ".";
static char g_last_error[512];
static AssetRequest g_requests[ASSET_REQUEST_LIMIT];

static void normalize_slashes(char *path)
{
  while (*path) {
    if (*path == '\\')
      *path = '/';
    ++path;
  }
}

#ifdef __EMSCRIPTEN__
static void set_status(const char *status)
{
  char escaped[1024];
  char script[1200];
  size_t i;
  size_t out = 0;

  if (!status)
    status = "";

  for (i = 0; status[i] && out + 2 < sizeof(escaped); ++i) {
    if (status[i] == '\\' || status[i] == '\'')
      escaped[out++] = '\\';
    escaped[out++] = status[i];
  }
  escaped[out] = '\0';

  snprintf(script, sizeof(script),
           "if (typeof Module !== 'undefined' && Module.setStatus) Module.setStatus('%s');",
           escaped);
  emscripten_run_script(script);
}
#else
static void set_status(const char *status)
{
  (void)status;
}
#endif

static AssetRequest *find_request(const char *relative_path)
{
  int i;
  char normalized[260];
  snprintf(normalized, sizeof(normalized), "%s", relative_path ? relative_path : "");
  normalize_slashes(normalized);
  for (i = 0; i < ASSET_REQUEST_LIMIT; ++i) {
    if (g_requests[i].relative_path[0] && strcmp(g_requests[i].relative_path, normalized) == 0)
      return &g_requests[i];
  }
  return 0;
}

static AssetRequest *alloc_request(const char *relative_path)
{
  int i;
  char normalized[260];
  snprintf(normalized, sizeof(normalized), "%s", relative_path ? relative_path : "");
  normalize_slashes(normalized);
  for (i = 0; i < ASSET_REQUEST_LIMIT; ++i) {
    if (!g_requests[i].relative_path[0]) {
      snprintf(g_requests[i].relative_path, sizeof(g_requests[i].relative_path), "%s", normalized);
      g_requests[i].state = SHANDALAR_ASSET_NOT_REQUESTED;
      return &g_requests[i];
    }
  }
  snprintf(g_last_error, sizeof(g_last_error), "asset request table is full");
  return 0;
}

void shandalar_assets_set_root(const char *root)
{
  snprintf(g_asset_root, sizeof(g_asset_root), "%s", (root && root[0]) ? root : ".");
  normalize_slashes(g_asset_root);
#ifdef __EMSCRIPTEN__
  mkdir(g_asset_root, 0777);
#endif
}

const char *shandalar_assets_root(void) { return g_asset_root; }

int shandalar_assets_join_path(char *out, unsigned int out_size, const char *relative_path)
{
  if (!out || !out_size || !relative_path)
    return 0;
  snprintf(out, out_size, "%s/%s", g_asset_root, relative_path);
  normalize_slashes(out);
  return 1;
}

static int asset_file_exists(const char *relative_path)
{
  char path[1024];
  FILE *f;
  if (!shandalar_assets_join_path(path, sizeof(path), relative_path))
    return 0;
  f = fopen(path, "rb");
  if (!f)
    return 0;
  fclose(f);
  return 1;
}

#ifdef __EMSCRIPTEN__
static void ensure_parent_dirs(const char *path)
{
  char tmp[1024];
  char *p;
  snprintf(tmp, sizeof(tmp), "%s", path ? path : "");
  normalize_slashes(tmp);
  for (p = tmp + 1; *p; ++p) {
    if (*p == '/') {
      *p = '\0';
      if (tmp[0])
        mkdir(tmp, 0777);
      *p = '/';
    }
  }
}

static int write_fetch_to_file(const char *path, const void *data, size_t size)
{
  FILE *file;
  ensure_parent_dirs(path);
  file = fopen(path, "wb");
  if (!file)
    return 0;
  if (fwrite(data, 1, size, file) != size) {
    fclose(file);
    return 0;
  }
  fclose(file);
  return 1;
}

static void fetch_succeeded(emscripten_fetch_t *fetch)
{
  AssetRequest *request = (AssetRequest *)fetch->userData;
  if (request) {
    char path[1024];
    shandalar_assets_join_path(path, sizeof(path), request->relative_path);
    if (write_fetch_to_file(path, fetch->data, (size_t)fetch->numBytes)) {
      request->state = SHANDALAR_ASSET_READY;
    } else {
      request->state = SHANDALAR_ASSET_ERROR;
      snprintf(g_last_error, sizeof(g_last_error), "failed to cache %s: errno %d", path, errno);
    }
  }
  emscripten_fetch_close(fetch);
}

static void fetch_failed(emscripten_fetch_t *fetch)
{
  AssetRequest *request = (AssetRequest *)fetch->userData;
  if (request)
    request->state = SHANDALAR_ASSET_ERROR;
  snprintf(g_last_error, sizeof(g_last_error), "failed to fetch %s: HTTP %d", fetch->url, fetch->status);
  emscripten_fetch_close(fetch);
}
#endif

ShandalarAssetState shandalar_assets_prefetch(const char *relative_path)
{
  AssetRequest *request;
  if (!relative_path || !relative_path[0])
    return SHANDALAR_ASSET_ERROR;
  request = find_request(relative_path);
  if (!request)
    request = alloc_request(relative_path);
  if (!request)
    return SHANDALAR_ASSET_ERROR;
  if (request->state == SHANDALAR_ASSET_READY || request->state == SHANDALAR_ASSET_PENDING)
    return request->state;
  if (asset_file_exists(request->relative_path)) {
    request->state = SHANDALAR_ASSET_READY;
    return request->state;
  }
#ifdef __EMSCRIPTEN__
  {
    char url[1024];
    char status[512];
    emscripten_fetch_attr_t attr;
    shandalar_assets_join_path(url, sizeof(url), request->relative_path);
    snprintf(status, sizeof(status), "Loading %s", request->relative_path);
    set_status(status);
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
    attr.onsuccess = fetch_succeeded;
    attr.onerror = fetch_failed;
    attr.userData = request;
    request->state = SHANDALAR_ASSET_PENDING;
    emscripten_fetch(&attr, url);
    return request->state;
  }
#else
  snprintf(g_last_error, sizeof(g_last_error), "asset not found: %s", request->relative_path);
  request->state = SHANDALAR_ASSET_ERROR;
  return request->state;
#endif
}

ShandalarAssetState shandalar_assets_fetch_blocking(const char *relative_path)
{
  ShandalarAssetState state = shandalar_assets_prefetch(relative_path);
#ifdef __EMSCRIPTEN__
  while (state == SHANDALAR_ASSET_PENDING) {
    emscripten_sleep(1);
    state = shandalar_assets_state(relative_path);
  }
#endif
  if (state != SHANDALAR_ASSET_PENDING)
    set_status("");
  return state;
}

ShandalarAssetState shandalar_assets_state(const char *relative_path)
{
  AssetRequest *request = find_request(relative_path);
  if (request)
    return request->state;
  return asset_file_exists(relative_path) ? SHANDALAR_ASSET_READY : SHANDALAR_ASSET_NOT_REQUESTED;
}

int shandalar_assets_is_ready(const char *relative_path)
{
  return shandalar_assets_state(relative_path) == SHANDALAR_ASSET_READY;
}

void shandalar_assets_update(void) {}
const char *shandalar_assets_last_error(void) { return g_last_error; }

ShandalarAssetState shandalar_assets_prefetch_small_art_cat(void)
{
  return shandalar_assets_prefetch("CardArt/SmallArt.cat");
}

ShandalarAssetState shandalar_assets_prefetch_med_art_cat(void)
{
  return shandalar_assets_prefetch("CardArt/MedArt.cat");
}
