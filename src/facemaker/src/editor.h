#ifndef SHANDALAR_FACEMAKER_EDITOR_H
#define SHANDALAR_FACEMAKER_EDITOR_H

typedef enum FacemakerEditorAction {
  FACEMAKER_EDITOR_NONE,
  FACEMAKER_EDITOR_REDRAW,
  FACEMAKER_EDITOR_COMPLETE,
  FACEMAKER_EDITOR_CANCEL
} FacemakerEditorAction;

int facemaker_editor_create(const char *asset_root, const char *writable_root,
                            int width, int height, int download_on_save);
void facemaker_editor_destroy(void);
void facemaker_editor_render(void);
FacemakerEditorAction facemaker_editor_pointer_move(float x, float y);
FacemakerEditorAction facemaker_editor_pointer_button(int button, int pressed);
FacemakerEditorAction facemaker_editor_key(int key, int pressed, int repeat);
int facemaker_editor_save(const char *path);

#endif
