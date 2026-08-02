#include "platform/winuser_imgui_bridge.h"

#include "platform/winuser_compat.h"
#include "platform/winuser_sdl_compositor.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace {

SDL_Renderer *g_renderer;

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

void draw_window_tree(SDL_Renderer *renderer, HWND window, ImDrawList *draw_list)
{
  char class_name[32] = {};
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
      GetClassNameA(window, class_name, sizeof(class_name));
      if (SDL_strcasecmp(class_name, "#32770") == 0) {
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
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    draw_window_tree(renderer, child, draw_list);
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
  RECT rect = {};
  int minimum;
  int maximum;
  int position;
  const float extent = static_cast<float>(GetSystemMetrics(SM_CYHSCROLL));
  if (!(GetWindowLongA(window, GWL_STYLE) & WS_HSCROLL) ||
      !GetScrollRange(window, SB_HORZ, &minimum, &maximum) || maximum <= minimum)
    return;
  GetWindowRect(window, &rect);
  position = GetScrollPos(window, SB_HORZ);
  ImGui::PushID(window);
  ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.left),
                                   static_cast<float>(rect.bottom) - extent));
  if (ImGui::ArrowButton("##scroll-left", ImGuiDir_Left))
    SendMessageA(window, WM_HSCROLL, SB_LINELEFT, 0);
  ImGui::SameLine(0.0f, 0.0f);
  ImGui::SetNextItemWidth(static_cast<float>(rect.right - rect.left) - extent * 2.0f);
  if (ImGui::SliderInt("##scroll-position", &position, minimum, maximum, "",
                       ImGuiSliderFlags_AlwaysClamp))
    SendMessageA(window, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), 0);
  ImGui::SameLine(0.0f, 0.0f);
  if (ImGui::ArrowButton("##scroll-right", ImGuiDir_Right))
    SendMessageA(window, WM_HSCROLL, SB_LINERIGHT, 0);
  ImGui::PopID();
}

void draw_builtin_control(HWND window)
{
  if (!IsWindowVisible(window) || !IsWindowEnabled(window))
    return;
  char class_name[32] = {};
  char text[1024] = {};
  RECT rect = {};
  GetClassNameA(window, class_name, sizeof(class_name));
  GetWindowTextA(window, text, sizeof(text));
  GetWindowRect(window, &rect);
  float width = static_cast<float>(rect.right - rect.left);
  float height = static_cast<float>(rect.bottom - rect.top);
  if (width <= 0.0f || height <= 0.0f)
    return;

  ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)));
  ImGui::PushID(window);
  LONG style = GetWindowLongA(window, GWL_STYLE);
  if (SDL_strcasecmp(class_name, "BUTTON") == 0) {
    LONG type = style & BS_TYPEMASK;
    if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX) {
      bool checked = SendMessageA(window, BM_GETCHECK, 0, 0) == BST_CHECKED;
      if (ImGui::Checkbox(text[0] ? text : "##check", &checked)) {
        SendMessageA(window, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        SetFocus(window);
        send_control_command(window, BN_CLICKED);
      }
    } else if (type == 9) {
      bool checked = SendMessageA(window, BM_GETCHECK, 0, 0) == BST_CHECKED;
      if (ImGui::RadioButton(text[0] ? text : " ", checked)) {
        HWND group = window;
        HWND candidate;
        while ((candidate = GetWindow(group, GW_HWNDPREV)) != nullptr) {
          group = candidate;
          if (GetWindowLongA(group, GWL_STYLE) & WS_GROUP)
            break;
        }
        for (candidate = group; candidate; candidate = GetWindow(candidate, GW_HWNDNEXT)) {
          if (candidate != group && (GetWindowLongA(candidate, GWL_STYLE) & WS_GROUP))
            break;
          char sibling_class[32] = {};
          GetClassNameA(candidate, sibling_class, sizeof(sibling_class));
          if (SDL_strcasecmp(sibling_class, "BUTTON") == 0 &&
              (GetWindowLongA(candidate, GWL_STYLE) & BS_TYPEMASK) == 9)
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
    ImGui::TextUnformatted(text);
  } else if (SDL_strcasecmp(class_name, "EDIT") == 0) {
    ImGuiInputTextFlags flags = 0;
    if (style & ES_READONLY) flags |= ImGuiInputTextFlags_ReadOnly;
    bool changed;
    ImGui::SetNextItemWidth(width);
    if (style & ES_MULTILINE)
      changed = ImGui::InputTextMultiline("##edit", text, sizeof(text), ImVec2(width, height), flags);
    else
      changed = ImGui::InputText("##edit", text, sizeof(text), flags);
    if (changed) {
      SetWindowTextA(window, text);
      send_control_command(window, EN_CHANGE);
    }
    if (ImGui::IsItemFocused()) SetFocus(window);
  } else if (SDL_strcasecmp(class_name, "LISTBOX") == 0) {
    if (ImGui::BeginListBox("##list", ImVec2(width, height))) {
      int count = static_cast<int>(SendMessageA(window, LB_GETCOUNT, 0, 0));
      bool multiple = (style & LBS_MULTIPLESEL) != 0;
      for (int i = 0; i < count; ++i) {
        text[0] = '\0';
        SendMessageA(window, LB_GETTEXT, i, (LPARAM)text);
        bool selected = multiple ? SendMessageA(window, LB_GETSEL, i, 0) > 0 :
                                   SendMessageA(window, LB_GETCURSEL, 0, 0) == i;
        ImGui::PushID(i);
        if (ImGui::Selectable(text, selected)) {
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
    int position = static_cast<short>(SendMessageA(window, WM_USER + 104, 0, 0));
    if (ImGui::ArrowButton("##up", ImGuiDir_Up))
      SendMessageA(window, WM_USER + 103, 0, MAKELONG(position + 1, 0));
    if (ImGui::ArrowButton("##down", ImGuiDir_Down))
      SendMessageA(window, WM_USER + 103, 0, MAKELONG(position - 1, 0));
  }
  ImGui::PopID();
}

void draw_control_tree(HWND window, HWND modal)
{
  if (!IsWindowVisible(window))
    return;
  if (modal && window == modal)
    return;
  bool disabled_by_modal = modal && !is_descendant_or_self(window, modal);
  if (disabled_by_modal) {
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
    ImGui::BeginDisabled();
  }
  if (is_builtin_control(window))
    draw_builtin_control(window);
  else
    draw_window_scrollbars(window);
  if (disabled_by_modal) {
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
  }
  HWND child = GetWindow(window, GW_CHILD);
  if (child) child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    draw_control_tree(child, modal);
}

void draw_window_overlay_tree(HWND window, ImDrawList *draw_list)
{
  char class_name[32] = {};
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
      GetClassNameA(window, class_name, sizeof(class_name));
      if (SDL_strcasecmp(class_name, "#32770") == 0) {
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
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    draw_window_overlay_tree(child, draw_list);
}

void draw_dialog_caption(HWND dialog)
{
  char title[512] = {};
  RECT rect = {};
  int caption_height;
  if (!dialog || !GetWindowRect(dialog, &rect))
    return;
  caption_height = GetSystemMetrics(SM_CYCAPTION);
  GetWindowTextA(dialog, title, sizeof(title));
  ImGui::PushID(dialog);
  ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.left),
                                   static_cast<float>(rect.top)));
  ImGui::InvisibleButton("##dialog-caption",
                         ImVec2(static_cast<float>(rect.right - rect.left - caption_height),
                                static_cast<float>(caption_height)));
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
                           ImVec2(static_cast<float>(rect.right),
                                  static_cast<float>(rect.top + caption_height)),
                           IM_COL32(35, 70, 120, 255));
  draw_list->AddText(ImVec2(static_cast<float>(rect.left + 6),
                            static_cast<float>(rect.top + 2)),
                     IM_COL32_WHITE, title[0] ? title : "Dialog");
  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    POINT position = {rect.left + static_cast<int>(delta.x),
                      rect.top + static_cast<int>(delta.y)};
    HWND parent = GetParent(dialog);
    if (parent)
      ScreenToClient(parent, &position);
    MoveWindow(dialog, position.x, position.y, rect.right - rect.left,
               rect.bottom - rect.top, TRUE);
  }
  ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(rect.right - caption_height),
                                   static_cast<float>(rect.top)));
  if (ImGui::Button("X", ImVec2(static_cast<float>(caption_height),
                                 static_cast<float>(caption_height))))
    PostMessageA(dialog, WM_CLOSE, 0, 0);
  ImGui::PopID();
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
  draw_dialog_caption(modal);
  draw_popup_menu();
  ImGui::End();
  ImGui::PopStyleVar();
}

}  // namespace

extern "C" int shandalar_win32_imgui_init(SDL_Window *window, SDL_Renderer *renderer)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsClassic();
  ImGui::GetIO().IniFilename = nullptr;
  g_renderer = renderer;
  return shandalar_win32_sdl_compositor_init(renderer) &&
         ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) &&
         ImGui_ImplSDLRenderer3_Init(renderer);
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
}
