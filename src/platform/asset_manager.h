#ifndef SHANDALAR_ASSET_MANAGER_H
#define SHANDALAR_ASSET_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ShandalarAssetState {
  SHANDALAR_ASSET_NOT_REQUESTED = 0,
  SHANDALAR_ASSET_PENDING = 1,
  SHANDALAR_ASSET_READY = 2,
  SHANDALAR_ASSET_ERROR = 3
} ShandalarAssetState;

void shandalar_assets_set_root(const char *root);
const char *shandalar_assets_root(void);
int shandalar_assets_join_path(char *out, unsigned int out_size, const char *relative_path);
ShandalarAssetState shandalar_assets_prefetch(const char *relative_path);
ShandalarAssetState shandalar_assets_state(const char *relative_path);
int shandalar_assets_is_ready(const char *relative_path);
void shandalar_assets_update(void);
const char *shandalar_assets_last_error(void);

ShandalarAssetState shandalar_assets_prefetch_small_art_cat(void);
ShandalarAssetState shandalar_assets_prefetch_med_art_cat(void);

#ifdef __cplusplus
}
#endif

#endif
