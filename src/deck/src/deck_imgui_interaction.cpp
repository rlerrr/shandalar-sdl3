#include "deck/src/deck_imgui_interaction.h"

#include "platform/winuser_compat.h"
#include "platform/winuser_imgui_bridge.h"
#include "platform/winuser_sdl_compositor.h"
#include "facemaker/src/facemaker_types.h"

#include <SDL3/SDL.h>
#include <imgui.h>

extern "C" HWND global_decksurface_hwnd;

namespace {

#ifndef WM_APP
#define WM_APP 0x8000
#endif

#define WM_DECKBUILDER_DRAG_SELL_HOVER (WM_APP + 0x442)

HWND g_left_down_target;
HWND g_deckbuilder_drag_hover_title;
ImVec2 g_left_down_position;
bool g_left_drag_started;
bool g_deck_menu_button_left_down;
ImU32 g_deck_menu_sprite_pixels[64 * 64];
int g_deck_menu_sprite_width;
int g_deck_menu_sprite_height;

void draw_deck_menu_sprite(ImDrawList *draw_list, const ImVec2 &position,
                           float button_size)
{
  const float icon_size = 22.0f;
  const float icon_left = position.x + (button_size - icon_size) * 0.5f;
  const float icon_top = position.y + (button_size - icon_size) * 0.5f;
  int y;

  for (y = 0; y < g_deck_menu_sprite_height; ++y) {
    int x = 0;
    while (x < g_deck_menu_sprite_width) {
      const ImU32 color =
          g_deck_menu_sprite_pixels[y * g_deck_menu_sprite_width + x];
      int run_end;
      float x0;
      float x1;
      float y0;
      float y1;
      if (color == 0) {
        ++x;
        continue;
      }
      run_end = x + 1;
      while (run_end < g_deck_menu_sprite_width &&
             g_deck_menu_sprite_pixels[y * g_deck_menu_sprite_width +
                                       run_end] == color)
        ++run_end;
      x0 = icon_left + icon_size * x / g_deck_menu_sprite_width;
      x1 = icon_left + icon_size * run_end / g_deck_menu_sprite_width;
      y0 = icon_top + icon_size * y / g_deck_menu_sprite_height;
      y1 = icon_top + icon_size * (y + 1) / g_deck_menu_sprite_height;
      draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color);
      x = run_end;
    }
  }
}

void draw_deck_surface_menu_button()
{
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
      ImGuiWindowFlags_NoBringToFrontOnFocus;
  const float size = 30.0f;
  const float inset = 5.0f;
  const float corner = 4.0f;
  RECT rect = {};
  POINT popup_point;
  ImVec2 position;
  ImVec2 plaque_min;
  ImVec2 plaque_max;
  ImVec2 plaque_points[8];
  ImDrawList *draw_list;
  ImU32 fill;
  ImU32 border;
  ImU32 shadow;
  ImU32 glyph;
  const ImVec2 mouse = ImGui::GetIO().MousePos;
  bool hovered;
  bool held;
  int line_index;

  if (!global_decksurface_hwnd || !IsWindow(global_decksurface_hwnd) ||
      !GetWindowRect(global_decksurface_hwnd, &rect) ||
      rect.right - rect.left < static_cast<int>(size + inset * 2.0f) ||
      rect.bottom - rect.top < static_cast<int>(size + inset * 2.0f))
    return;

  position = ImVec2(static_cast<float>(rect.right) - size - inset,
                    static_cast<float>(rect.top) + inset);
  ImGui::SetNextWindowPos(position);
  ImGui::SetNextWindowSize(ImVec2(size, size));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  if (!ImGui::Begin("##deck-surface-menu-button", nullptr, window_flags)) {
    ImGui::End();
    ImGui::PopStyleVar();
    return;
  }

  hovered = mouse.x >= position.x && mouse.x < position.x + size &&
            mouse.y >= position.y && mouse.y < position.y + size;
  held = hovered && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left];
  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    g_deck_menu_button_left_down = true;
    popup_point.x = rect.right - rect.left -
                    static_cast<LONG>(inset + size * 0.5f);
    popup_point.y = static_cast<LONG>(inset + size);
    SendMessageA(global_decksurface_hwnd, WM_RBUTTONDOWN, 0,
                 MAKELPARAM(popup_point.x, popup_point.y));
  }

  draw_list = ImGui::GetWindowDrawList();
  plaque_min = ImVec2(position.x + 1.0f, position.y + 1.0f);
  plaque_max = ImVec2(position.x + size - 1.0f, position.y + size - 1.0f);
  plaque_points[0] = ImVec2(plaque_min.x + corner, plaque_min.y);
  plaque_points[1] = ImVec2(plaque_max.x - corner, plaque_min.y);
  plaque_points[2] = ImVec2(plaque_max.x, plaque_min.y + corner);
  plaque_points[3] = ImVec2(plaque_max.x, plaque_max.y - corner);
  plaque_points[4] = ImVec2(plaque_max.x - corner, plaque_max.y);
  plaque_points[5] = ImVec2(plaque_min.x + corner, plaque_max.y);
  plaque_points[6] = ImVec2(plaque_min.x, plaque_max.y - corner);
  plaque_points[7] = ImVec2(plaque_min.x, plaque_min.y + corner);

  fill = held ? IM_COL32(32, 43, 75, 248)
              : hovered ? IM_COL32(66, 83, 125, 246)
                        : IM_COL32(43, 57, 94, 238);
  border = hovered ? IM_COL32(190, 211, 220, 255)
                   : IM_COL32(107, 134, 166, 255);
  shadow = IM_COL32(8, 14, 30, 190);
  glyph = hovered ? IM_COL32(225, 237, 239, 255)
                  : IM_COL32(157, 181, 201, 255);

  for (int point_index = 0; point_index < 8; ++point_index) {
    plaque_points[point_index].x += 1.0f;
    plaque_points[point_index].y += 2.0f;
  }
  draw_list->AddConvexPolyFilled(plaque_points, 8, shadow);
  for (int point_index = 0; point_index < 8; ++point_index) {
    plaque_points[point_index].x -= 1.0f;
    plaque_points[point_index].y -= 2.0f;
  }
  draw_list->AddConvexPolyFilled(plaque_points, 8, fill);
  draw_list->AddPolyline(plaque_points, 8, border,
                         ImDrawFlags_Closed, 1.0f);

  if (g_deck_menu_sprite_width > 0 && g_deck_menu_sprite_height > 0) {
    draw_deck_menu_sprite(draw_list, position, size);
  } else {
    for (line_index = 0; line_index < 3; ++line_index) {
      const float y = position.y + 10.0f + line_index * 5.0f;
      draw_list->AddLine(ImVec2(position.x + 9.0f, y + 1.0f),
                         ImVec2(position.x + 21.0f, y + 1.0f), shadow, 2.0f);
      draw_list->AddLine(ImVec2(position.x + 9.0f, y),
                         ImVec2(position.x + 21.0f, y), glyph, 2.0f);
    }
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

bool left_drag_threshold_crossed(const ImVec2 &mouse)
{
  const float threshold = ImGui::GetIO().MouseDragThreshold;
  const float dx = mouse.x - g_left_down_position.x;
  const float dy = mouse.y - g_left_down_position.y;
  return dx * dx + dy * dy >= threshold * threshold;
}

bool is_descendant_or_self(HWND window, HWND ancestor)
{
  while (window) {
    if (window == ancestor)
      return true;
    window = GetParent(window);
  }
  return false;
}

bool is_card_drag_source(HWND window)
{
  char name[64] = {};
  GetClassNameA(window, name, sizeof(name));
  return SDL_strcasecmp(name, "MAGICDECK_CardClass") == 0 ||
         SDL_strcasecmp(name, "MAGICDECK_HorzListClass") == 0;
}

HWND find_child_window_by_class(HWND window, const char *wanted_class)
{
  HWND child;
  char class_name[64] = {};
  for (child = GetWindow(window, GW_CHILD); child;
       child = GetWindow(child, GW_HWNDNEXT)) {
    HWND found;
    GetClassNameA(child, class_name, sizeof(class_name));
    if (SDL_strcasecmp(class_name, wanted_class) == 0)
      return child;
    found = find_child_window_by_class(child, wanted_class);
    if (found)
      return found;
  }
  return nullptr;
}

HWND find_deckbuilder_title_window(HWND window)
{
  HWND root = window;
  HWND found;
  while (root && GetParent(root))
    root = GetParent(root);
  found = root ? find_child_window_by_class(root, "MAGICDECK_TitleClass")
               : nullptr;
  if (found)
    return found;
  for (root = GetWindow(GetDesktopWindow(), GW_CHILD); root;
       root = GetWindow(root, GW_HWNDNEXT)) {
    found = find_child_window_by_class(root, "MAGICDECK_TitleClass");
    if (found)
      return found;
  }
  return nullptr;
}

void notify_drag_source(HWND title, HWND source)
{
  POINT source_point;
  if (!title || !source)
    return;
  if (g_deckbuilder_drag_hover_title &&
      g_deckbuilder_drag_hover_title != title &&
      IsWindow(g_deckbuilder_drag_hover_title))
    SendMessageA(g_deckbuilder_drag_hover_title,
                 WM_DECKBUILDER_DRAG_SELL_HOVER, 0, 0);
  g_deckbuilder_drag_hover_title = title;
  source_point.x = static_cast<LONG>(g_left_down_position.x);
  source_point.y = static_cast<LONG>(g_left_down_position.y);
  ScreenToClient(source, &source_point);
  SendMessageA(title, WM_DECKBUILDER_DRAG_SELL_HOVER, (WPARAM)source,
               MAKELPARAM(source_point.x, source_point.y));
}

void clear_drag_hover(void)
{
  if (g_deckbuilder_drag_hover_title &&
      IsWindow(g_deckbuilder_drag_hover_title))
    SendMessageA(g_deckbuilder_drag_hover_title,
                 WM_DECKBUILDER_DRAG_SELL_HOVER, 0, 0);
  g_deckbuilder_drag_hover_title = nullptr;
}

void draw_drag_preview(void)
{
  char class_name[64] = {};
  RECT rect = {};
  ImVec2 mouse;
  ImVec2 uv0(0.0f, 0.0f);
  ImVec2 uv1(1.0f, 1.0f);
  ImVec2 size;
  ImVec2 offset;
  SDL_Texture *texture;
  int texture_width;
  int texture_height;

  if (!g_left_drag_started || !g_left_down_target ||
      !IsWindow(g_left_down_target) ||
      !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
    return;
  texture = shandalar_win32_sdl_compositor_texture(
      g_left_down_target, &texture_width, &texture_height);
  if (!texture || texture_width <= 0 || texture_height <= 0)
    return;
  GetWindowRect(g_left_down_target, &rect);
  GetClassNameA(g_left_down_target, class_name, sizeof(class_name));
  size = ImVec2(static_cast<float>(texture_width),
                static_cast<float>(texture_height));
  offset = ImVec2(g_left_down_position.x - rect.left,
                  g_left_down_position.y - rect.top);
  if (SDL_strcasecmp(class_name, "MAGICDECK_HorzListClass") == 0) {
    int content_height = texture_height - GetSystemMetrics(SM_CYHSCROLL);
    int card_width = content_height > 0 ? content_height : texture_height;
    int local_x = static_cast<int>(g_left_down_position.x) - rect.left;
    int source_x = card_width > 0 ? (local_x / card_width) * card_width : 0;
    if (source_x + card_width > texture_width)
      source_x = texture_width - card_width;
    if (source_x < 0)
      source_x = 0;
    uv0 = ImVec2(static_cast<float>(source_x) / texture_width, 0.0f);
    uv1 = ImVec2(static_cast<float>(source_x + card_width) / texture_width,
                 static_cast<float>(content_height) / texture_height);
    size = ImVec2(static_cast<float>(card_width),
                  static_cast<float>(content_height));
    offset.x -= source_x;
  }
  mouse = ImGui::GetIO().MousePos;
  ImGui::GetForegroundDrawList()->AddImage(
      reinterpret_cast<ImTextureID>(texture),
      ImVec2(mouse.x - offset.x, mouse.y - offset.y),
      ImVec2(mouse.x - offset.x + size.x, mouse.y - offset.y + size.y),
      uv0, uv1, IM_COL32(255, 255, 255, 210));
}

LPARAM mouse_lparam(HWND window, const ImVec2 &mouse)
{
  POINT point = {static_cast<LONG>(mouse.x), static_cast<LONG>(mouse.y)};
  ScreenToClient(window, &point);
  return MAKELPARAM(point.x, point.y);
}

WPARAM mouse_buttons(const ImGuiIO &io)
{
  WPARAM result = 0;
  if (io.MouseDown[ImGuiMouseButton_Left])
    result |= MK_LBUTTON;
  if (io.MouseDown[ImGuiMouseButton_Right])
    result |= MK_RBUTTON;
  if (io.KeyShift)
    result |= MK_SHIFT;
  return result;
}

void forward_mouse(void)
{
  ImGuiIO &io = ImGui::GetIO();
  bool completed_drag_release = false;
  const ImVec2 mouse = io.MousePos;
  if (mouse.x < 0.0f || mouse.y < 0.0f)
    return;
  shandalar_win32_set_cursor_position(static_cast<int>(mouse.x),
                                      static_cast<int>(mouse.y));
  if (g_deck_menu_button_left_down) {
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      g_deck_menu_button_left_down = false;
    return;
  }
  POINT screen = {static_cast<LONG>(mouse.x), static_cast<LONG>(mouse.y)};
  HWND target = GetCapture();
  if (!target)
    target = WindowFromPoint(screen);
  HWND modal = shandalar_win32_active_dialog();
  WPARAM buttons = mouse_buttons(io);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && g_left_down_target) {
    HWND source = g_left_down_target;
    ImVec2 down = g_left_down_position;
    bool was_dragged = g_left_drag_started || left_drag_threshold_crossed(mouse);
    g_left_down_target = nullptr;
    g_left_drag_started = false;
    // A stationary card click is selection/double-click input, not a drag.
    // Sending the legacy button-down handler would remove and reinsert the
    // card, changing its deck-surface order before a second click can arrive.
    if (was_dragged) {
      shandalar_win32_set_cursor_position(static_cast<int>(down.x),
                                          static_cast<int>(down.y));
      shandalar_win32_set_system_drag_endpoint(static_cast<int>(mouse.x),
                                                static_cast<int>(mouse.y));
      SendMessageA(source, WM_LBUTTONDOWN, buttons | MK_LBUTTON,
                   mouse_lparam(source, down));
      shandalar_win32_clear_system_drag_endpoint();
      shandalar_win32_set_cursor_position(static_cast<int>(mouse.x),
                                          static_cast<int>(mouse.y));
      SendMessageA(source, WM_LBUTTONUP, buttons & ~MK_LBUTTON,
                   mouse_lparam(source, mouse));
    }
    clear_drag_hover();
    completed_drag_release = true;
  }
  if (!target || target == GetDesktopWindow())
    return;
  if (modal && !is_descendant_or_self(target, modal)) {
    g_left_down_target = nullptr;
    g_left_drag_started = false;
    BringWindowToTop(modal);
    return;
  }
  if (shandalar_win32_is_builtin_control(target))
    return;
  LPARAM local = mouse_lparam(target, mouse);
  if (g_left_down_target && io.MouseDown[ImGuiMouseButton_Left] &&
      !g_left_drag_started && left_drag_threshold_crossed(mouse))
    g_left_drag_started = true;
  if (g_left_drag_started && g_left_down_target &&
      io.MouseDown[ImGuiMouseButton_Left])
    notify_drag_source(find_deckbuilder_title_window(g_left_down_target),
                       g_left_down_target);
  PostMessageA(target, WM_MOUSEMOVE, buttons, local);

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (ImGui::IsAnyItemHovered()) {
      g_left_down_target = nullptr;
      g_left_drag_started = false;
    } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
               shandalar_win32_accepts_double_click(target)) {
      g_left_down_target = nullptr;
      g_left_drag_started = false;
      SendMessageA(target, WM_LBUTTONDBLCLK, buttons, local);
    } else if (!is_card_drag_source(target)) {
      g_left_down_target = nullptr;
      g_left_drag_started = false;
      SendMessageA(target, WM_LBUTTONDOWN, buttons | MK_LBUTTON, local);
    } else {
      g_left_down_target = target;
      g_left_down_position = mouse;
      g_left_drag_started = false;
    }
  }
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
      !completed_drag_release)
    SendMessageA(target, WM_LBUTTONUP, buttons & ~MK_LBUTTON, local);
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    SendMessageA(target,
                 ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right) &&
                         shandalar_win32_accepts_double_click(target)
                     ? WM_RBUTTONDBLCLK
                     : WM_RBUTTONDOWN,
                 buttons, local);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    SendMessageA(target, WM_RBUTTONUP, buttons, local);
  if (io.MouseWheel != 0.0f) {
    WPARAM wheel =
        MAKEWPARAM(buttons, static_cast<int>(io.MouseWheel * 120.0f));
    SendMessageA(target, WM_MOUSEWHEEL, wheel,
                 MAKELPARAM(screen.x, screen.y));
  }
}

}  // namespace

extern "C" void shandalar_deck_imgui_set_menu_sprite(
    const void *encoded_sprite, const unsigned char *palette_bgra)
{
  const EncodedImage *sprite = static_cast<const EncodedImage *>(encoded_sprite);
  const unsigned char *span;
  int row;

  shandalar_win32_imgui_set_menu_sprite(encoded_sprite, palette_bgra);

  g_deck_menu_sprite_width = 0;
  g_deck_menu_sprite_height = 0;
  if (!sprite || !palette_bgra || sprite->width <= 0 || sprite->height <= 0 ||
      sprite->width > 64 || sprite->height > 64)
    return;

  SDL_memset(g_deck_menu_sprite_pixels, 0,
             sizeof(g_deck_menu_sprite_pixels));
  span = sprite->spans;
  for (row = 0; row < sprite->row_count; ++row) {
    const int y = sprite->first_row + row;
    int offset = *span++;
    int length;
    int x;
    if (offset == 0xff)
      continue;
    length = *span++;
    if (length == 0xfe)
      length = *span++;
    for (x = 0; x < length; ++x) {
      const unsigned int palette_index = span[x];
      if (palette_index != 0 && y >= 0 && y < sprite->height &&
          offset + x >= 0 && offset + x < sprite->width) {
        const unsigned char *color = palette_bgra + palette_index * 4;
        g_deck_menu_sprite_pixels[y * sprite->width + offset + x] =
            IM_COL32(color[2], color[1], color[0], 255);
      }
    }
    span += length;
  }
  g_deck_menu_sprite_width = sprite->width;
  g_deck_menu_sprite_height = sprite->height;
}

extern "C" void shandalar_deck_imgui_interaction_present(void)
{
  if (!ImGui::GetCurrentContext())
    return;
  draw_deck_surface_menu_button();
  draw_drag_preview();
  forward_mouse();
}

extern "C" void shandalar_deck_imgui_interaction_shutdown(void)
{
  clear_drag_hover();
  g_left_down_target = nullptr;
  g_left_drag_started = false;
  g_deck_menu_button_left_down = false;
  g_deck_menu_sprite_width = 0;
  g_deck_menu_sprite_height = 0;
}
