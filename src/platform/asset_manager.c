#include "platform/asset_manager.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <errno.h>
#include <sys/stat.h>
#endif

#define ASSET_REQUEST_LIMIT 256

typedef struct AssetRequest
{
  char relative_path[260];
  ShandalarAssetState state;
  double start_time;
} AssetRequest;

static char g_asset_root[512] = ".";
static char g_last_error[512];
static AssetRequest g_requests[ASSET_REQUEST_LIMIT];

#ifdef __EMSCRIPTEN__
static void lowercase_path(char *path);
#endif

static void normalize_slashes(char *path)
{
  while (*path)
  {
    if (*path == '\\')
      *path = '/';
    ++path;
  }
}

static int case_insensitive_equal(const char *a, const char *b)
{
  while (*a && *b)
  {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
      return 0;
    ++a;
    ++b;
  }
  return *a == *b;
}

static int case_insensitive_starts_with(const char *text, const char *prefix)
{
  while (*prefix)
  {
    if (tolower((unsigned char)*text) != tolower((unsigned char)*prefix))
      return 0;
    ++text;
    ++prefix;
  }
  return 1;
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

  for (i = 0; status[i] && out + 2 < sizeof(escaped); ++i)
  {
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
  for (i = 0; i < ASSET_REQUEST_LIMIT; ++i)
  {
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
  for (i = 0; i < ASSET_REQUEST_LIMIT; ++i)
  {
    if (!g_requests[i].relative_path[0])
    {
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

static int validate_asset_file(const char *path, const char *relative_path)
{
  FILE *f;
  int valid = 1;
  long file_size;
  f = fopen(path, "rb");
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  fseek(f, 0, SEEK_SET);
  {
    const char *ext = strrchr(relative_path, '.');
    if (ext && case_insensitive_equal(ext, ".pcx"))
    {
      unsigned char header[4];
      valid = (fread(header, 1, sizeof(header), f) == sizeof(header)) &&
              header[0] == 0x0a && header[1] <= 5 && header[3] == 8;
    }
    else if (!ext && case_insensitive_starts_with(relative_path, "FaceArt/"))
    {
      int first_sprite_size = 0;
      valid = file_size >= 16 &&
              fread(&first_sprite_size, 1, sizeof(first_sprite_size), f) == sizeof(first_sprite_size) &&
              first_sprite_size >= 16 && first_sprite_size <= file_size;
    }
  }
  fclose(f);
  if (!valid)
    remove(path);
  return valid;
}

static int asset_file_exists(const char *relative_path)
{
  char path[1024];
  if (!shandalar_assets_join_path(path, sizeof(path), relative_path))
    return 0;
  if (validate_asset_file(path, relative_path))
    return 1;
#ifdef __EMSCRIPTEN__
  {
    char *filename = strrchr(path, '/');
    if (filename)
    {
      char *p = filename + 1;
      while (*p) { *p = (char)tolower((unsigned char)*p); ++p; }
      if (validate_asset_file(path, relative_path))
        return 1;
    }
  }
  lowercase_path(path);
  return validate_asset_file(path, relative_path);
#else
  return 0;
#endif
}

#ifdef __EMSCRIPTEN__
static int ensure_parent_directories(const char *path)
{
  char parent[1024];
  char *cursor;
  char *slash;

  if (!path)
    return 0;
  snprintf(parent, sizeof(parent), "%s", path);
  slash = strrchr(parent, '/');
  if (!slash)
    return 1;
  *slash = '\0';

  cursor = parent;
  if (*cursor == '/')
    ++cursor;
  while ((cursor = strchr(cursor, '/')) != NULL)
  {
    *cursor = '\0';
    if (parent[0] != '\0')
      mkdir(parent, 0777);
    *cursor++ = '/';
  }
  if (parent[0] != '\0')
    mkdir(parent, 0777);
  return 1;
}

static int write_fetch_to_file(const char *path, const void *data, size_t size)
{
  FILE *file = fopen(path, "wb");
  if (!file && ensure_parent_directories(path))
    file = fopen(path, "wb");
  if (!file)
    return 0;
  if (fwrite(data, 1, size, file) != size)
  {
    fclose(file);
    return 0;
  }
  fclose(file);
  return 1;
}

static void lowercase_path(char *path)
{
  while (*path)
  {
    *path = (char)tolower((unsigned char)*path);
    ++path;
  }
}

static void fetch_succeeded(emscripten_fetch_t *fetch)
{
  AssetRequest *request = (AssetRequest *)fetch->userData;
  if (request)
  {
    char path[1024];
    shandalar_assets_join_path(path, sizeof(path), request->relative_path);
    if (fetch->status < 200 || fetch->status >= 300)
    {
      request->state = SHANDALAR_ASSET_ERROR;
      snprintf(g_last_error, sizeof(g_last_error), "failed to fetch %s: HTTP %d", fetch->url, fetch->status);
    }
    else
    {
      if (!write_fetch_to_file(path, fetch->data, (size_t)fetch->numBytes))
      {
        request->state = SHANDALAR_ASSET_ERROR;
        snprintf(g_last_error, sizeof(g_last_error), "failed to write %s: errno %d", path, errno);
      }
      else if (!asset_file_exists(request->relative_path))
      {
        request->state = SHANDALAR_ASSET_ERROR;
        snprintf(g_last_error, sizeof(g_last_error), "failed to validate %s: errno %d", path, errno);
      }
      else
      {
        request->state = SHANDALAR_ASSET_READY;
      }
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
  if (asset_file_exists(request->relative_path))
  {
    request->state = SHANDALAR_ASSET_READY;
    return request->state;
  }
#ifdef __EMSCRIPTEN__
  {
    char url[1024];
    emscripten_fetch_attr_t attr;
    shandalar_assets_join_path(url, sizeof(url), request->relative_path);
    lowercase_path(url);
    request->start_time = emscripten_get_now();
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
  {
    int status_shown = 0;
    while (state == SHANDALAR_ASSET_PENDING)
    {
      emscripten_sleep(1);
      state = shandalar_assets_state(relative_path);
      if (!status_shown && state == SHANDALAR_ASSET_PENDING)
      {
        AssetRequest *req = find_request(relative_path);
        if (req && (emscripten_get_now() - req->start_time) >= 500.0)
        {
          char status[512];
          snprintf(status, sizeof(status), "Loading %s", req->relative_path);
          set_status(status);
          status_shown = 1;
        }
      }
    }
  }
#endif
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
