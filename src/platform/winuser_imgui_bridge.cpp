#include "platform/winuser_imgui_bridge.h"

#include "platform/gdi_compat.h"
#include "platform/winuser_compat.h"
#include "platform/winuser_sdl_compositor.h"
#include "facemaker/src/facemaker_types.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cctype>
#include <climits>

namespace {

SDL_Renderer *g_renderer;
struct DialogFont {
  const char *face;
  int pixel_size;
  const char *file;
  ImFont *font;
};
DialogFont g_dialog_fonts[] = {
    {"Benguiat Bk BT", 13, "TT0127M_.TTF", nullptr},
    {"Benguiat Bk BT", 16, "TT0127M_.TTF", nullptr},
    {"Benguiat Bk BT", 19, "TT0127M_.TTF", nullptr},
    {"Benguiat Bk BT", 23, "TT0127M_.TTF", nullptr},
    {"Benguiat Bk BT", 30, "TT0127M_.TTF", nullptr},
    {"Trebuchet MS", 12, "TT0298M_.TTF", nullptr},
    {"MS Sans Serif", 15, "TT0298M_.TTF", nullptr},
};
HWND g_territory_menu_windows[2];
HWND g_listbox_drag_candidate;
HWND g_listbox_drag_target;
/* This cache is an adventure asset and survives duel ImGui contexts. */
ImU32 g_shared_menu_sprite_pixels[64 * 64];
int g_shared_menu_sprite_width;
int g_shared_menu_sprite_height;

bool is_builtin_control(HWND window)
{
  return shandalar_win32_is_builtin_control(window) != 0;
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

bool is_dialog_control(HWND window)
{
  char class_name[32] = {};
  for (window = GetParent(window); window; window = GetParent(window)) {
    GetClassNameA(window, class_name, sizeof(class_name));
    if (SDL_strcasecmp(class_name, "#32770") == 0)
      return true;
  }
  return false;
}

void strip_mnemonic_markers(char *text)
{
  size_t source = 0;
  size_t destination = 0;
  while (text[source]) {
    if (text[source] == '&') {
      if (text[source + 1] == '&') {
        text[destination++] = '&';
        source += 2;
      } else {
        ++source;
      }
    } else {
      text[destination++] = text[source++];
    }
  }
  text[destination] = '\0';
}

bool push_child_clip_rect(HWND window, ImDrawList *draw_list)
{
  RECT client = {};
  POINT top_left = {};
  POINT bottom_right = {};
  if (!GetClientRect(window, &client))
    return false;
  top_left.x = client.left;
  top_left.y = client.top;
  bottom_right.x = client.right;
  bottom_right.y = client.bottom;
  if (!ClientToScreen(window, &top_left) ||
      !ClientToScreen(window, &bottom_right))
    return false;
  draw_list->PushClipRect(ImVec2(static_cast<float>(top_left.x),
                                 static_cast<float>(top_left.y)),
                          ImVec2(static_cast<float>(bottom_right.x),
                                 static_cast<float>(bottom_right.y)), true);
  return true;
}

void draw_window_tree(SDL_Renderer *renderer, HWND window, ImDrawList *draw_list)
{
  if (!IsWindowVisible(window))
    return;

  RECT rect = {};
  GetWindowRect(window, &rect);
  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  if (!is_builtin_control(window) && width > 0 && height > 0) {
    int texture_width = 0;
    int texture_height = 0;
    SDL_Texture *texture =
        shandalar_win32_sdl_compositor_texture(window, &texture_width,
                                                &texture_height);
    if (texture) {
      if (shandalar_win32_should_render_caption(window)) {
        int caption_height = GetSystemMetrics(SM_CYCAPTION);
        draw_list->AddImage(reinterpret_cast<ImTextureID>(texture),
                            ImVec2(static_cast<float>(rect.left),
                                   static_cast<float>(rect.top + caption_height)),
                            ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)),
                            ImVec2(0.0f, 0.0f),
                            ImVec2(1.0f, static_cast<float>(texture_height - caption_height) /
                                            texture_height));
      } else {
        draw_list->AddImage(reinterpret_cast<ImTextureID>(texture),
                            ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
                            ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)));
      }
    }
  }

  HWND child = GetWindow(window, GW_CHILD);
  if (child)
    child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV)) {
    const bool clipped =
        (GetWindowLongA(child, GWL_STYLE) & WS_CHILD) == WS_CHILD &&
        push_child_clip_rect(window, draw_list);
    draw_window_tree(renderer, child, draw_list);
    if (clipped)
      draw_list->PopClipRect();
  }
}

void send_control_command(HWND window, int notification)
{
  HWND parent = GetParent(window);
  if (parent)
    PostMessageA(parent, WM_COMMAND,
                 MAKEWPARAM(GetDlgCtrlID(window), notification), (LPARAM)window);
}

void draw_window_scrollbars(HWND window)
{
  RECT client = {};
  POINT origin = {};
  const LONG style = GetWindowLongA(window, GWL_STYLE);
  int minimum;
  int maximum;
  int position;
  const float horizontal_extent =
      static_cast<float>(GetSystemMetrics(SM_CYHSCROLL));
  const float vertical_extent =
      static_cast<float>(GetSystemMetrics(SM_CXVSCROLL));
  if (!GetClientRect(window, &client) || !ClientToScreen(window, &origin))
    return;

  ImGui::PushID(window);
  if ((style & WS_HSCROLL) &&
      GetScrollRange(window, SB_HORZ, &minimum, &maximum) && maximum > minimum) {
    position = GetScrollPos(window, SB_HORZ);
    ImGui::SetCursorScreenPos(
        ImVec2(static_cast<float>(origin.x),
               static_cast<float>(origin.y + client.bottom)));
    if (ImGui::ArrowButton("##scroll-left", ImGuiDir_Left))
      SendMessageA(window, WM_HSCROLL, SB_LINELEFT, 0);
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(static_cast<float>(client.right) -
                            horizontal_extent * 2.0f);
    if (ImGui::SliderInt("##hscroll-position", &position, minimum, maximum, "",
                         ImGuiSliderFlags_AlwaysClamp))
      SendMessageA(window, WM_HSCROLL,
                   MAKEWPARAM(SB_THUMBPOSITION, position), 0);
    ImGui::SameLine(0.0f, 0.0f);
    if (ImGui::ArrowButton("##scroll-right", ImGuiDir_Right))
      SendMessageA(window, WM_HSCROLL, SB_LINERIGHT, 0);
  }

  if ((style & WS_VSCROLL) &&
      GetScrollRange(window, SB_VERT, &minimum, &maximum) && maximum > minimum) {
    const float slider_height =
        static_cast<float>(client.bottom) - vertical_extent * 2.0f;
    /* ImGui vertical sliders put their minimum at the bottom, opposite to a
     * Win32 scrollbar.  Mirror the display value in both directions so the
     * Win32 minimum remains at the top. */
    position = maximum - (GetScrollPos(window, SB_VERT) - minimum);
    ImGui::SetCursorScreenPos(
        ImVec2(static_cast<float>(origin.x + client.right),
               static_cast<float>(origin.y)));
    if (ImGui::ArrowButton("##scroll-up", ImGuiDir_Up))
      SendMessageA(window, WM_VSCROLL, SB_LINEUP, 0);
    ImGui::SetCursorScreenPos(
        ImVec2(static_cast<float>(origin.x + client.right),
               static_cast<float>(origin.y) + vertical_extent));
    if (ImGui::VSliderInt("##vscroll-position",
                          ImVec2(vertical_extent, slider_height), &position,
                          minimum, maximum, "",
                          ImGuiSliderFlags_AlwaysClamp))
      SendMessageA(window, WM_VSCROLL,
                   MAKEWPARAM(SB_THUMBPOSITION,
                              maximum - (position - minimum)), 0);
    ImGui::SetCursorScreenPos(
        ImVec2(static_cast<float>(origin.x + client.right),
               static_cast<float>(origin.y + client.bottom) - vertical_extent));
    if (ImGui::ArrowButton("##scroll-down", ImGuiDir_Down))
      SendMessageA(window, WM_VSCROLL, SB_LINEDOWN, 0);
  }
  ImGui::PopID();
}

void draw_static_text(const char *text, const RECT &rect, LONG style,
                      ImU32 color)
{
  constexpr LONG static_type_mask = 0x1fL;
  constexpr LONG static_center = 0x01L;
  constexpr LONG static_right = 0x02L;
  constexpr LONG static_simple = 0x0bL;
  constexpr LONG static_left_no_word_wrap = 0x0cL;
  const LONG type = style & static_type_mask;
  const float left = static_cast<float>(rect.left);
  const float top = static_cast<float>(rect.top);
  const float right = static_cast<float>(rect.right);
  const float bottom = static_cast<float>(rect.bottom);
  const float width = right - left;
  const float line_height = ImGui::GetTextLineHeight();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const char *cursor = text;
  float y = top;

  draw_list->PushClipRect(ImVec2(left, top), ImVec2(right, bottom), true);
  while (*cursor && y < bottom) {
    const char *line_start = cursor;
    const char *line_end = cursor;
    const char *last_space = nullptr;

    while (*line_end && *line_end != '\n') {
      const char *candidate_end = line_end + 1;
      if (type != static_simple && type != static_left_no_word_wrap &&
          ImGui::CalcTextSize(line_start, candidate_end).x > width) {
        if (last_space && last_space > line_start)
          line_end = last_space;
        else if (line_end == line_start)
          line_end = candidate_end;
        break;
      }
      if (std::isspace(static_cast<unsigned char>(*line_end)))
        last_space = line_end;
      line_end = candidate_end;
    }

    const char *visible_end = line_end;
    while (visible_end > line_start &&
           std::isspace(static_cast<unsigned char>(visible_end[-1])))
      --visible_end;
    const float line_width = ImGui::CalcTextSize(line_start, visible_end).x;
    float x = left;
    if (type == static_center)
      x += (width - line_width) * 0.5f;
    else if (type == static_right)
      x += width - line_width;
    draw_list->AddText(ImVec2(x, y), color, line_start, visible_end);
    y += line_height;

    cursor = line_end;
    if (*cursor == '\n')
      ++cursor;
    else
      while (*cursor && std::isspace(static_cast<unsigned char>(*cursor)))
        ++cursor;
    if (type == static_simple || type == static_left_no_word_wrap)
      break;
  }
  draw_list->PopClipRect();
}

ImFont *dialog_control_font(HWND window)
{
  LOGFONTA description = {};
  HFONT font = reinterpret_cast<HFONT>(SendMessageA(window, WM_GETFONT, 0, 0));
  if (!font || GetObjectA(font, sizeof(description), &description) != sizeof(description))
    return nullptr;
  const int size = description.lfHeight < 0 ? -description.lfHeight
                                             : description.lfHeight;
  for (const DialogFont &candidate : g_dialog_fonts)
    if (candidate.font && candidate.pixel_size == size &&
        SDL_strcasecmp(candidate.face, description.lfFaceName) == 0)
      return candidate.font;
  return nullptr;
}

ImU32 legacy_static_text_color(HWND window)
{
  HDC dc = shandalar_window_backing_dc(window);
  HWND parent = GetParent(window);
  COLORREF color;
  if (!dc)
    return ImGui::GetColorU32(ImGuiCol_Text);
  if (parent)
    SendMessageA(parent, WM_CTLCOLORSTATIC, (WPARAM)dc, (LPARAM)window);
  color = shandalar_dc_resolve_color(dc, GetTextColor(dc));
  return IM_COL32(color & 0xff, (color >> 8) & 0xff,
                  (color >> 16) & 0xff, 255);
}

void draw_owner_draw_button(HWND window, const RECT &rect,
                            float width, float height)
{
  DRAWITEMSTRUCT item = {};
  HDC dc;
  HWND parent;
  SDL_Texture *texture;
  bool clicked = ImGui::InvisibleButton("##owner-draw", ImVec2(width, height));

  item.CtlType = ODT_BUTTON;
  item.CtlID = static_cast<UINT>(GetDlgCtrlID(window));
  item.itemState = (ImGui::IsItemActive() ? ODS_SELECTED : 0) |
                   (GetFocus() == window ? ODS_FOCUS : 0);
  item.hwndItem = window;
  item.rcItem.right = static_cast<LONG>(width);
  item.rcItem.bottom = static_cast<LONG>(height);
  dc = shandalar_window_backing_dc(window);
  parent = GetParent(window);
  item.hDC = dc;
  if (dc && parent) {
    SendMessageA(parent, WM_DRAWITEM, item.CtlID, (LPARAM)&item);
    shandalar_window_content_changed(window);
  }
  texture = shandalar_win32_sdl_compositor_control_texture(window, nullptr,
                                                            nullptr);
  if (texture) {
    ImGui::GetWindowDrawList()->AddImage(
        reinterpret_cast<ImTextureID>(texture),
        ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
        ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)));
  }
  if (clicked) {
    SetFocus(window);
    send_control_command(window, BN_CLICKED);
  }
}

HWND static_click_proxy(HWND window)
{
  int target_id;
  switch (GetDlgCtrlID(window)) {
  case 1126: target_id = 1120; break;
  case 1127: target_id = 1122; break;
  case 1128: target_id = 1124; break;
  case 1129: target_id = 1123; break;
  case 1130: target_id = 1121; break;
  case 1131: target_id = 1125; break;
  default: return nullptr;
  }
  return GetDlgItem(GetParent(window), target_id);
}

bool draw_coin_flip_title(HWND window, const char *text, const RECT &rect)
{
  if (GetDlgCtrlID(window) != 0x41b)
    return false;

  HWND parent = GetParent(window);
  char parent_class[32] = {};
  RECT parent_rect = {};
  if (!parent ||
      !GetClassNameA(parent, parent_class, sizeof(parent_class)) ||
      SDL_strcasecmp(parent_class, "#32770") != 0 ||
      !GetWindowRect(parent, &parent_rect))
    return false;

  const ImVec2 text_size = ImGui::CalcTextSize(text);
  const float x = (static_cast<float>(parent_rect.left + parent_rect.right) -
                   text_size.x) * 0.5f;
  const float y = static_cast<float>(rect.top);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(ImVec2(x - 10.0f, y - 3.0f),
                           ImVec2(x + text_size.x + 10.0f,
                                  y + text_size.y + 3.0f),
                           IM_COL32(48, 32, 80, 255));
  draw_list->AddText(ImVec2(x + 1.0f, y), IM_COL32_WHITE, text);
  draw_list->AddText(ImVec2(x, y), IM_COL32_WHITE, text);
  return true;
}

void draw_builtin_control(HWND window)
{
  if (!IsWindowVisible(window))
    return;
  char class_name[32] = {};
  char text[1024] = {};
  RECT rect = {};
  GetClassNameA(window, class_name, sizeof(class_name));
  GetWindowTextA(window, text, sizeof(text));
  if (SDL_strcasecmp(class_name, "BUTTON") == 0 ||
      SDL_strcasecmp(class_name, "STATIC") == 0)
    strip_mnemonic_markers(text);
  GetWindowRect(window, &rect);
  float width = static_cast<float>(rect.right - rect.left);
  float height = static_cast<float>(rect.bottom - rect.top);
  if (width <= 0.0f || height <= 0.0f)
    return;

  ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)));
  ImGui::PushID(window);
  ImFont *control_font = dialog_control_font(window);
  if (control_font)
    ImGui::PushFont(control_font);
  const bool use_dialog_text_color = is_dialog_control(window);
  if (use_dialog_text_color)
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_BLACK);
  LONG style = GetWindowLongA(window, GWL_STYLE);
  if (SDL_strcasecmp(class_name, "BUTTON") == 0) {
    LONG type = style & BS_TYPEMASK;
    if (type == BS_OWNERDRAW) {
      draw_owner_draw_button(window, rect, width, height);
    } else if (type == BS_GROUPBOX) {
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      const float label_x = static_cast<float>(rect.left + 10);
      const float label_y = static_cast<float>(rect.top);
      const float border_y = label_y + ImGui::GetTextLineHeight() * 0.5f;
      const float label_right = label_x + ImGui::CalcTextSize(text).x;
      const ImU32 border_color = ImGui::GetColorU32(ImGuiCol_Border);
      draw_list->AddLine(ImVec2(static_cast<float>(rect.left), border_y),
                         ImVec2(label_x - 3.0f, border_y), border_color);
      draw_list->AddLine(ImVec2(label_right + 3.0f, border_y),
                         ImVec2(static_cast<float>(rect.right), border_y),
                         border_color);
      draw_list->AddLine(ImVec2(static_cast<float>(rect.left), border_y),
                         ImVec2(static_cast<float>(rect.left),
                                static_cast<float>(rect.bottom)),
                         border_color);
      draw_list->AddLine(ImVec2(static_cast<float>(rect.right), border_y),
                         ImVec2(static_cast<float>(rect.right),
                                static_cast<float>(rect.bottom)),
                         border_color);
      draw_list->AddLine(ImVec2(static_cast<float>(rect.left),
                                static_cast<float>(rect.bottom)),
                         ImVec2(static_cast<float>(rect.right),
                                static_cast<float>(rect.bottom)),
                         border_color);
      draw_list->AddText(ImVec2(label_x, label_y),
                         ImGui::GetColorU32(ImGuiCol_Text), text);
    } else if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX) {
      bool checked = SendMessageA(window, BM_GETCHECK, 0, 0) == BST_CHECKED;
      if (ImGui::Checkbox(text[0] ? text : "##check", &checked)) {
        SendMessageA(window, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        SetFocus(window);
        send_control_command(window, BN_CLICKED);
      }
    } else if (type == BS_AUTORADIOBUTTON) {
      bool checked = SendMessageA(window, BM_GETCHECK, 0, 0) == BST_CHECKED;
      if (ImGui::RadioButton(text[0] ? text : " ", checked)) {
        HWND group = window;
        HWND candidate;
        while (!(GetWindowLongA(group, GWL_STYLE) & WS_GROUP) &&
               (candidate = GetWindow(group, GW_HWNDNEXT)) != nullptr) {
          group = candidate;
        }
        for (candidate = group; candidate; candidate = GetWindow(candidate, GW_HWNDPREV)) {
          if (candidate != group && (GetWindowLongA(candidate, GWL_STYLE) & WS_GROUP))
            break;
          char sibling_class[32] = {};
          GetClassNameA(candidate, sibling_class, sizeof(sibling_class));
          if (SDL_strcasecmp(sibling_class, "BUTTON") == 0 &&
              (GetWindowLongA(candidate, GWL_STYLE) & BS_TYPEMASK) ==
                  BS_AUTORADIOBUTTON)
            SendMessageA(candidate, BM_SETCHECK,
                         candidate == window ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        SetFocus(window);
        send_control_command(window, BN_CLICKED);
      }
    } else if (ImGui::Button(text[0] ? text : " ", ImVec2(width, height))) {
      SetFocus(window);
      send_control_command(window, BN_CLICKED);
    }
  } else if (SDL_strcasecmp(class_name, "STATIC") == 0) {
    if (!draw_coin_flip_title(window, text, rect)) {
      HWND click_proxy;
      if (style & WS_BORDER) {
        // These bordered static controls are calculated, read-only values.
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 top_left(static_cast<float>(rect.left),
                              static_cast<float>(rect.top));
        const ImVec2 bottom_right(static_cast<float>(rect.right),
                                  static_cast<float>(rect.bottom));
        const ImVec2 text_size = ImGui::CalcTextSize(text);
        draw_list->AddRectFilled(top_left, bottom_right,
                                 IM_COL32(162, 76, 70, 255));
        draw_list->AddRect(top_left, bottom_right,
                           IM_COL32(225, 198, 184, 255));
        draw_list->PushClipRect(top_left, bottom_right, true);
        draw_list->AddText(ImVec2(top_left.x + (width - text_size.x) * 0.5f,
                                  top_left.y + (height - text_size.y) * 0.5f),
                           IM_COL32(242, 248, 247, 255), text);
        draw_list->PopClipRect();
      } else {
        draw_static_text(text, rect, style, legacy_static_text_color(window));
      }
      click_proxy = static_click_proxy(window);
      if (click_proxy && ImGui::InvisibleButton("##static-click-proxy",
                                                ImVec2(width, height)))
        SetFocus(click_proxy);
    }
  } else if (SDL_strcasecmp(class_name, "EDIT") == 0) {
    ImGuiInputTextFlags flags = 0;
    if (style & ES_READONLY) flags |= ImGuiInputTextFlags_ReadOnly;
    HWND next_control = GetWindow(window, GW_HWNDPREV);
    char next_class[32] = {};
    const bool spin_buddy =
        next_control && GetClassNameA(next_control, next_class, sizeof(next_class)) &&
        SDL_strcasecmp(next_class, "msctls_updown32") == 0;
    if (spin_buddy)
      flags |= ImGuiInputTextFlags_CharsDecimal |
               ImGuiInputTextFlags_CharsNoBlank;
    bool changed;
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(20, 20, 20, 255));
    if (style & WS_BORDER)
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::SetNextItemWidth(width);
    if (spin_buddy && !(style & ES_MULTILINE)) {
      float padding_y = (height - ImGui::GetTextLineHeight()) * 0.5f;
      if (padding_y < 0.0f) padding_y = 0.0f;
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                          ImVec2(ImGui::GetStyle().FramePadding.x, padding_y));
    }
    if (style & ES_MULTILINE)
      changed = ImGui::InputTextMultiline("##edit", text, sizeof(text), ImVec2(width, height), flags);
    else
      changed = ImGui::InputText("##edit", text, sizeof(text), flags);
    if (spin_buddy && !(style & ES_MULTILINE))
      ImGui::PopStyleVar();
    if (style & WS_BORDER)
      ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    if (changed) {
      SetWindowTextA(window, text);
      send_control_command(window, EN_CHANGE);
    }
    if (ImGui::IsItemFocused()) SetFocus(window);
  } else if (SDL_strcasecmp(class_name, "LISTBOX") == 0) {
    if (ImGui::BeginListBox("##list", ImVec2(width, height))) {
      const bool hovered = ImGui::IsWindowHovered(
          ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
      if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        g_listbox_drag_candidate = window;
      if (g_listbox_drag_candidate == window &&
          ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        g_listbox_drag_target = window;
      const bool drag_scrolling = g_listbox_drag_target == window;
      if (drag_scrolling)
        ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y);

      int count = static_cast<int>(SendMessageA(window, LB_GETCOUNT, 0, 0));
      bool multiple = (style & LBS_MULTIPLESEL) != 0;
      for (int i = 0; i < count; ++i) {
        text[0] = '\0';
        SendMessageA(window, LB_GETTEXT, i, (LPARAM)text);
        bool selected = multiple ? SendMessageA(window, LB_GETSEL, i, 0) > 0 :
                                   SendMessageA(window, LB_GETCURSEL, 0, 0) == i;
        ImGui::PushID(i);
        if (ImGui::Selectable(text, selected) && !drag_scrolling) {
          if (multiple)
            SendMessageA(window, LB_SETSEL, !selected, i);
          else
            SendMessageA(window, LB_SETCURSEL, i, 0);
          SetFocus(window);
          send_control_command(window, LBN_SELCHANGE);
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            send_control_command(window, LBN_DBLCLK);
        }
        ImGui::PopID();
      }
      ImGui::EndListBox();
    }
  } else if (SDL_strcasecmp(class_name, "COMBOBOX") == 0) {
    int current = static_cast<int>(SendMessageA(window, CB_GETCURSEL, 0, 0));
    char preview[1024] = {};
    if (current >= 0) SendMessageA(window, CB_GETLBTEXT, current, (LPARAM)preview);
    ImGui::SetNextItemWidth(width);
    if (ImGui::BeginCombo("##combo", preview)) {
      int count = static_cast<int>(SendMessageA(window, CB_GETCOUNT, 0, 0));
      for (int i = 0; i < count; ++i) {
        text[0] = '\0';
        SendMessageA(window, CB_GETLBTEXT, i, (LPARAM)text);
        if (ImGui::Selectable(text, current == i)) {
          SendMessageA(window, CB_SETCURSEL, i, 0);
          SetFocus(window);
          send_control_command(window, CBN_SELCHANGE);
        }
      }
      ImGui::EndCombo();
    }
  } else if (SDL_strcasecmp(class_name, "msctls_updown32") == 0) {
    HWND buddy = GetWindow(window, GW_HWNDNEXT);
    RECT buddy_rect = {};
    if (buddy && GetWindowRect(buddy, &buddy_rect)) {
      BOOL invalid = FALSE;
      const int position = static_cast<int>(SendMessageA(
          window, UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(&invalid)));
      ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(buddy_rect.left) - width,
                                        static_cast<float>(rect.top)));
      const bool decrement = ImGui::Button("-", ImVec2(width, height));
      ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.left),
                                        static_cast<float>(rect.top)));
      const bool increment = ImGui::Button("+", ImVec2(width, height));
      if (decrement || increment) {
        int next = position;
        if (increment && next < INT_MAX) ++next;
        if (decrement && next > INT_MIN) --next;
        next = static_cast<int>(SendMessageA(window, UDM_SETPOS32, 0, next));
        SetFocus(buddy);
        if (next != position)
          send_control_command(buddy, EN_CHANGE);
      }
    }
  }
  if (use_dialog_text_color)
    ImGui::PopStyleColor();
  if (control_font)
    ImGui::PopFont();
  ImGui::PopID();
}

void draw_control_tree(HWND window, HWND modal)
{
  if (!IsWindowVisible(window))
    return;
  if (modal && window == modal)
    return;
  bool disabled_by_modal = modal && !is_descendant_or_self(window, modal);
  bool disabled = disabled_by_modal || !IsWindowEnabled(window);
  if (disabled) {
    if (disabled_by_modal)
      ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
    ImGui::BeginDisabled();
  }
  if (is_builtin_control(window)) {
    // Modal controls are submitted later in this same ImGui window.  Allow
    // their items to overlap disabled controls beneath the dialog; otherwise
    // an underlying button can silently consume the dialog's hit area.
    if (disabled_by_modal)
      ImGui::SetNextItemAllowOverlap();
    draw_builtin_control(window);
  } else {
    draw_window_scrollbars(window);
  }
  if (disabled) {
    ImGui::EndDisabled();
    if (disabled_by_modal)
      ImGui::PopStyleVar();
  }
  HWND child = GetWindow(window, GW_CHILD);
  if (child) child = GetWindow(child, GW_HWNDLAST);
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  for (; child; child = GetWindow(child, GW_HWNDPREV)) {
    const bool clipped =
        (GetWindowLongA(child, GWL_STYLE) & WS_CHILD) == WS_CHILD &&
        push_child_clip_rect(window, draw_list);
    draw_control_tree(child, modal);
    if (clipped)
      draw_list->PopClipRect();
  }
}

void draw_popups_of_hidden_owners(SDL_Renderer *renderer, HWND owner,
                                  ImDrawList *draw_list)
{
  HWND child = GetWindow(owner, GW_CHILD);
  if (child)
    child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV)) {
    if (IsWindowVisible(child) && !IsWindowVisible(owner) &&
        (GetWindowLongA(child, GWL_STYLE) & WS_POPUP) == WS_POPUP) {
      draw_window_tree(renderer, child, draw_list);
    } else {
      draw_popups_of_hidden_owners(renderer, child, draw_list);
    }
  }
}

void draw_window_overlay_tree(HWND window, ImDrawList *draw_list)
{
  RECT rect = {};
  if (!window || !IsWindowVisible(window) || !GetWindowRect(window, &rect))
    return;
  if (!is_builtin_control(window)) {
    int texture_width = 0;
    int texture_height = 0;
    SDL_Texture *texture =
        shandalar_win32_sdl_compositor_texture(window, &texture_width,
                                                &texture_height);
    if (texture && texture_width > 0 && texture_height > 0) {
      if (shandalar_win32_should_render_caption(window)) {
        int caption_height = GetSystemMetrics(SM_CYCAPTION);
        draw_list->AddImage(reinterpret_cast<ImTextureID>(texture),
                            ImVec2(static_cast<float>(rect.left),
                                   static_cast<float>(rect.top + caption_height)),
                            ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)),
                            ImVec2(0.0f, 0.0f),
                            ImVec2(1.0f, static_cast<float>(texture_height - caption_height) /
                                             texture_height));
      } else {
        draw_list->AddImage(reinterpret_cast<ImTextureID>(texture),
                            ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
                            ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)));
      }
    }
  }
  HWND child = GetWindow(window, GW_CHILD);
  if (child) child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV)) {
    const bool clipped =
        (GetWindowLongA(child, GWL_STYLE) & WS_CHILD) == WS_CHILD &&
        push_child_clip_rect(window, draw_list);
    draw_window_overlay_tree(child, draw_list);
    if (clipped)
      draw_list->PopClipRect();
  }
}

void draw_window_caption(HWND window)
{
  char title[512] = {};
  RECT rect = {};
  int caption_height;
  const bool has_close_button =
      (GetWindowLongA(window, GWL_STYLE) & WS_SYSMENU) != 0;
  if (!window || !IsWindowVisible(window) || !GetWindowRect(window, &rect))
    return;
  if (!shandalar_win32_should_render_caption(window))
    return;
  caption_height = GetSystemMetrics(SM_CYCAPTION);
  GetWindowTextA(window, title, sizeof(title));
  ImGui::PushID(window);
  ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.left),
                                   static_cast<float>(rect.top)));
  ImGui::InvisibleButton("##dialog-caption",
                         ImVec2(static_cast<float>(rect.right - rect.left -
                                                   (has_close_button ? caption_height : 0)),
                                static_cast<float>(caption_height)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
                           ImVec2(static_cast<float>(rect.right),
                                  static_cast<float>(rect.top + caption_height)),
                           IM_COL32(35, 70, 120, 255));
  draw_list->AddText(ImVec2(static_cast<float>(rect.left + 6),
                            static_cast<float>(rect.top + 2)),
                     IM_COL32_WHITE, title);
  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    POINT position = {rect.left + static_cast<int>(delta.x),
                      rect.top + static_cast<int>(delta.y)};
    HWND parent = GetParent(window);
    if (parent && (GetWindowLongA(window, GWL_STYLE) & WS_CHILD))
      ScreenToClient(parent, &position);
    MoveWindow(window, position.x, position.y, rect.right - rect.left,
               rect.bottom - rect.top, TRUE);
  }
  if (has_close_button) {
    ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.right - caption_height),
                                     static_cast<float>(rect.top)));
    if (ImGui::Button("X", ImVec2(static_cast<float>(caption_height),
                                   static_cast<float>(caption_height))))
      PostMessageA(window, WM_CLOSE, 0, 0);
  }
  ImGui::PopID();
}

void draw_window_caption_tree(HWND window, HWND skip)
{
  HWND child;
  if (!window || !IsWindowVisible(window) || window == skip)
    return;
  draw_window_caption(window);
  child = GetWindow(window, GW_CHILD);
  if (child)
    child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    draw_window_caption_tree(child, skip);
}

bool draw_popup_menu_items(HMENU menu)
{
  int count = shandalar_win32_menu_item_count(menu);
  for (int i = 0; i < count; ++i) {
    UINT flags;
    UINT_PTR id;
    LPCSTR text;
    if (!shandalar_win32_menu_item(menu, i, &flags, &id, &text))
      continue;
    if (flags & MF_SEPARATOR) {
      ImGui::Separator();
      continue;
    }
    char label[1024];
    size_t source_index = 0;
    size_t label_index = 0;
    bool enabled = (flags & (MF_DISABLED | MF_GRAYED)) == 0;
    bool checked = (flags & MF_CHECKED) != 0;
    text = text && text[0] ? text : " ";
    while (text[source_index] && label_index + 1 < sizeof(label)) {
      if (text[source_index] == '&' && text[source_index + 1] != '&') {
        ++source_index;
        continue;
      }
      label[label_index++] = text[source_index++];
      if (text[source_index - 1] == '&' && text[source_index] == '&')
        ++source_index;
    }
    label[label_index] = '\0';
    ImGui::PushID(i);
    if (flags & MF_POPUP) {
      if (ImGui::BeginMenu(label, enabled)) {
        if (draw_popup_menu_items(reinterpret_cast<HMENU>(id))) {
          ImGui::EndMenu();
          ImGui::PopID();
          return true;
        }
        ImGui::EndMenu();
      }
    } else if (ImGui::MenuItem(label, nullptr, checked, enabled)) {
      shandalar_win32_choose_menu_item(id);
      ImGui::PopID();
      return true;
    }
    ImGui::PopID();
  }
  return false;
}

void draw_popup_menu(void)
{
  static HMENU opened_menu;
  HWND owner;
  int x;
  int y;
  HMENU menu = shandalar_win32_active_popup(&owner, &x, &y);
  if (!menu) {
    opened_menu = nullptr;
    return;
  }
  if (menu != opened_menu) {
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)));
    ImGui::OpenPopup("##win32-popup");
    opened_menu = menu;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
  if (ImGui::BeginPopup("##win32-popup")) {
    if (draw_popup_menu_items(menu)) {
      opened_menu = nullptr;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  } else if (opened_menu == menu) {
    shandalar_win32_dismiss_popup();
    opened_menu = nullptr;
  }
  ImGui::PopStyleVar(2);
}

void draw_territory_menu_button(HWND territory, int index,
                                bool interaction_enabled)
{
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
  const ImU32 shadow = IM_COL32(8, 14, 30, 190);
  ImU32 glyph;
  bool hovered;
  bool held;

  if (!territory || !IsWindow(territory) || !IsWindowVisible(territory) ||
      !GetWindowRect(territory, &rect) ||
      rect.right - rect.left < static_cast<int>(size + inset * 2.0f) ||
      rect.bottom - rect.top < static_cast<int>(size + inset * 2.0f))
    return;

  position = ImVec2(static_cast<float>(rect.right) - size - inset,
                    static_cast<float>(rect.bottom) - size - inset);
  ImGui::PushID(index);
  ImGui::SetCursorScreenPos(position);
  if (!interaction_enabled)
    ImGui::BeginDisabled();
  ImGui::InvisibleButton("##territory-menu-button", ImVec2(size, size));
  hovered = interaction_enabled && ImGui::IsItemHovered();
  held = interaction_enabled && ImGui::IsItemActive();
  if (interaction_enabled && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    popup_point.x = rect.right - rect.left -
                    static_cast<LONG>(inset + size * 0.5f);
    popup_point.y = rect.bottom - rect.top -
                    static_cast<LONG>(inset + size);
    SendMessageA(territory, WM_RBUTTONDOWN, 0,
                 MAKELPARAM(popup_point.x, popup_point.y));
  }
  if (!interaction_enabled)
    ImGui::EndDisabled();
  ImGui::PopID();

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
  draw_list->AddPolyline(plaque_points, 8, border, ImDrawFlags_Closed, 1.0f);
  if (g_shared_menu_sprite_width > 0 && g_shared_menu_sprite_height > 0) {
    const float icon_size = 22.0f;
    const float icon_left = position.x + (size - icon_size) * 0.5f;
    const float icon_top = position.y + (size - icon_size) * 0.5f;
    for (int y = 0; y < g_shared_menu_sprite_height; ++y) {
      int x = 0;
      while (x < g_shared_menu_sprite_width) {
        const ImU32 color =
            g_shared_menu_sprite_pixels[y * g_shared_menu_sprite_width + x];
        if (color == 0) {
          ++x;
          continue;
        }
        int run_end = x + 1;
        while (run_end < g_shared_menu_sprite_width &&
               g_shared_menu_sprite_pixels[y * g_shared_menu_sprite_width +
                                            run_end] == color)
          ++run_end;
        draw_list->AddRectFilled(
            ImVec2(icon_left + icon_size * x / g_shared_menu_sprite_width,
                   icon_top + icon_size * y / g_shared_menu_sprite_height),
            ImVec2(icon_left + icon_size * run_end / g_shared_menu_sprite_width,
                   icon_top + icon_size * (y + 1) /
                                  g_shared_menu_sprite_height),
            color);
        x = run_end;
      }
    }
  } else {
    for (int line_index = 0; line_index < 3; ++line_index) {
      const float y = position.y + 10.0f + line_index * 5.0f;
      draw_list->AddLine(ImVec2(position.x + 9.0f, y + 1.0f),
                         ImVec2(position.x + 21.0f, y + 1.0f), shadow, 2.0f);
      draw_list->AddLine(ImVec2(position.x + 9.0f, y),
                         ImVec2(position.x + 21.0f, y), glyph, 2.0f);
    }
  }
}

void draw_controls(void)
{
  RECT desktop_rect = {};
  GetClientRect(GetDesktopWindow(), &desktop_rect);
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(desktop_rect.right),
                                  static_cast<float>(desktop_rect.bottom)));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
                           ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;
  ImGui::Begin("##win32-controls", nullptr, flags);
  HWND modal = shandalar_win32_active_dialog();
  if (modal)
    BringWindowToTop(modal);
  HWND desktop = GetDesktopWindow();
  HWND window = GetWindow(desktop, GW_CHILD);
  if (window) window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    draw_control_tree(window, modal);
  if (modal) {
    draw_window_overlay_tree(modal, ImGui::GetWindowDrawList());
    draw_control_tree(modal, nullptr);
  }
  window = GetWindow(desktop, GW_CHILD);
  if (window)
    window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    draw_window_caption_tree(window, modal);
  draw_window_caption(modal);
  for (int index = 0; index < 2; ++index)
    draw_territory_menu_button(g_territory_menu_windows[index], index,
                               modal == nullptr);
  draw_popup_menu();
  ImGui::End();
  ImGui::PopStyleVar();
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    g_listbox_drag_candidate = nullptr;
    g_listbox_drag_target = nullptr;
  }
}

}  // namespace

extern "C" int shandalar_win32_imgui_init(SDL_Window *window, SDL_Renderer *renderer)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsClassic();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.Fonts->AddFontDefault();
  const char *font_root = shandalar_default_font_root();
  for (DialogFont &candidate : g_dialog_fonts) {
    candidate.font = nullptr;
    if (font_root && *font_root) {
      char path[1024];
      SDL_snprintf(path, sizeof(path), "%s/%s", font_root, candidate.file);
      candidate.font = io.Fonts->AddFontFromFileTTF(
          path, static_cast<float>(candidate.pixel_size));
    }
  }
  g_renderer = renderer;
  return shandalar_win32_sdl_compositor_init(renderer) &&
         ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) &&
         ImGui_ImplSDLRenderer3_Init(renderer);
}

extern "C" void shandalar_win32_imgui_set_safe_area_padding(float horizontal,
                                                               float vertical)
{
  if (!ImGui::GetCurrentContext())
    return;
  ImGui::GetStyle().DisplaySafeAreaPadding = ImVec2(horizontal, vertical);
}

extern "C" int shandalar_win32_imgui_process_event(SDL_Renderer *renderer,
                                                     const SDL_Event *event)
{
  SDL_Event logical_event;
  if (!event || !ImGui::GetCurrentContext()) return 0;
  logical_event = *event;
  if (event->type == SDL_EVENT_MOUSE_MOTION)
    SDL_RenderCoordinatesFromWindow(renderer, event->motion.x, event->motion.y,
                                    &logical_event.motion.x, &logical_event.motion.y);
  else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    SDL_RenderCoordinatesFromWindow(renderer, event->button.x, event->button.y,
                                    &logical_event.button.x, &logical_event.button.y);
  ImGui_ImplSDL3_ProcessEvent(&logical_event);
  return 1;
}

extern "C" void shandalar_win32_imgui_begin_frame(void)
{
  float window_x;
  float window_y;
  float logical_x;
  float logical_y;
  if (!ImGui::GetCurrentContext()) return;
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  RECT rect = {};
  GetClientRect(GetDesktopWindow(), &rect);
  ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(rect.right),
                                      static_cast<float>(rect.bottom));
  SDL_GetMouseState(&window_x, &window_y);
  if (SDL_RenderCoordinatesFromWindow(g_renderer, window_x, window_y,
                                      &logical_x, &logical_y))
    ImGui::GetIO().AddMousePosEvent(logical_x, logical_y);
  ImGui::NewFrame();
}

extern "C" void shandalar_win32_imgui_present(SDL_Renderer *renderer)
{
  if (!renderer || !ImGui::GetCurrentContext())
    return;
  shandalar_win32_sdl_compositor_update();
  ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
  HWND desktop = GetDesktopWindow();
  HWND window = GetWindow(desktop, GW_CHILD);
  if (window)
    window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    draw_window_tree(renderer, window, draw_list);
  draw_popups_of_hidden_owners(renderer, desktop, draw_list);
  draw_controls();
}

extern "C" void shandalar_win32_imgui_render(SDL_Renderer *renderer)
{
  if (!ImGui::GetCurrentContext()) return;
  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

extern "C" void shandalar_win32_imgui_shutdown(void)
{
  shandalar_win32_sdl_compositor_shutdown();
  if (ImGui::GetCurrentContext()) {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }
  g_renderer = nullptr;
  for (DialogFont &candidate : g_dialog_fonts)
    candidate.font = nullptr;
  g_territory_menu_windows[0] = nullptr;
  g_territory_menu_windows[1] = nullptr;
  g_listbox_drag_candidate = nullptr;
  g_listbox_drag_target = nullptr;
}

extern "C" void shandalar_win32_imgui_set_menu_sprite(
    const void *encoded_sprite, const unsigned char *palette_bgra)
{
  const EncodedImage *sprite = static_cast<const EncodedImage *>(encoded_sprite);
  const unsigned char *span;
  int row;

  g_shared_menu_sprite_width = 0;
  g_shared_menu_sprite_height = 0;
  if (!sprite || !palette_bgra || sprite->width <= 0 || sprite->height <= 0 ||
      sprite->width > 64 || sprite->height > 64)
    return;

  SDL_memset(g_shared_menu_sprite_pixels, 0,
             sizeof(g_shared_menu_sprite_pixels));
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
        g_shared_menu_sprite_pixels[y * sprite->width + offset + x] =
            IM_COL32(color[2], color[1], color[0], 255);
      }
    }
    span += length;
  }
  g_shared_menu_sprite_width = sprite->width;
  g_shared_menu_sprite_height = sprite->height;
}

extern "C" void shandalar_win32_imgui_register_territory_menu(HWND territory)
{
  int index;
  if (!territory)
    return;
  for (index = 0; index < 2; ++index) {
    if (g_territory_menu_windows[index] == territory)
      return;
  }
  for (index = 0; index < 2; ++index) {
    if (!g_territory_menu_windows[index] ||
        !IsWindow(g_territory_menu_windows[index])) {
      g_territory_menu_windows[index] = territory;
      return;
    }
  }
}

extern "C" void shandalar_win32_imgui_unregister_territory_menu(HWND territory)
{
  for (int index = 0; index < 2; ++index) {
    if (g_territory_menu_windows[index] == territory)
      g_territory_menu_windows[index] = nullptr;
  }
}
