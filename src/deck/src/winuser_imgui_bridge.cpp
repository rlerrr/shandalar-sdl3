#include "platform/winuser_imgui_bridge.h"

#include "platform/winuser_compat.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <stdlib.h>

namespace {

struct WindowTexture {
  WindowTexture *next;
  HWND window;
  SDL_Texture *texture;
  int width;
  int height;
  uint32_t *pixels;
};

WindowTexture *g_textures;
SDL_Window *g_window;
SDL_Renderer *g_renderer;

WindowTexture *find_texture(HWND window)
{
  for (WindowTexture *entry = g_textures; entry; entry = entry->next)
    if (entry->window == window)
      return entry;
  WindowTexture *entry = static_cast<WindowTexture *>(calloc(1, sizeof(*entry)));
  if (!entry)
    return nullptr;
  entry->window = window;
  entry->next = g_textures;
  g_textures = entry;
  return entry;
}

bool is_builtin_control(HWND window)
{
  char name[32] = {};
  GetClassNameA(window, name, sizeof(name));
  return SDL_strcasecmp(name, "BUTTON") == 0 ||
         SDL_strcasecmp(name, "EDIT") == 0 ||
         SDL_strcasecmp(name, "STATIC") == 0 ||
         SDL_strcasecmp(name, "LISTBOX") == 0 ||
         SDL_strcasecmp(name, "COMBOBOX") == 0 ||
         SDL_strcasecmp(name, "SCROLLBAR") == 0;
}

void draw_window_tree(SDL_Renderer *renderer, HWND window, ImDrawList *draw_list)
{
  if (!IsWindowVisible(window))
    return;

  RECT rect = {};
  GetWindowRect(window, &rect);
  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  HDC dc = shandalar_window_backing_dc(window);
  if (!is_builtin_control(window) && dc && width > 0 && height > 0) {
    UpdateWindow(window);
    WindowTexture *entry = find_texture(window);
    if (!entry)
      return;
    if (!entry->texture || entry->width != width || entry->height != height) {
      if (entry->texture)
        SDL_DestroyTexture(entry->texture);
      free(entry->pixels);
      entry->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING, width, height);
      entry->width = width;
      entry->height = height;
      entry->pixels = static_cast<uint32_t *>(
          malloc(static_cast<size_t>(width) * height * sizeof(uint32_t)));
    }
    if (entry->texture && entry->pixels &&
        shandalar_dc_copy_rgba(dc, entry->pixels, width * sizeof(uint32_t))) {
      SDL_UpdateTexture(entry->texture, nullptr, entry->pixels, width * sizeof(uint32_t));
      draw_list->AddImage(reinterpret_cast<ImTextureID>(entry->texture),
                          ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
                          ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)));
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
    SendMessageA(parent, WM_COMMAND,
                 MAKEWPARAM(GetDlgCtrlID(window), notification), (LPARAM)window);
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
  }
  ImGui::PopID();
}

void draw_control_tree(HWND window)
{
  if (!IsWindowVisible(window))
    return;
  if (is_builtin_control(window))
    draw_builtin_control(window);
  HWND child = GetWindow(window, GW_CHILD);
  if (child) child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    draw_control_tree(child);
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
  HWND desktop = GetDesktopWindow();
  HWND window = GetWindow(desktop, GW_CHILD);
  if (window) window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    draw_control_tree(window);
  ImGui::End();
  ImGui::PopStyleVar();
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
  if (io.MouseDown[ImGuiMouseButton_Left]) result |= MK_LBUTTON;
  if (io.MouseDown[ImGuiMouseButton_Right]) result |= MK_RBUTTON;
  if (io.KeyShift) result |= MK_SHIFT;
  return result;
}

void forward_mouse(void)
{
  ImGuiIO &io = ImGui::GetIO();
  const ImVec2 mouse = io.MousePos;
  if (mouse.x < 0.0f || mouse.y < 0.0f)
    return;
  shandalar_win32_set_cursor_position(static_cast<int>(mouse.x), static_cast<int>(mouse.y));
  POINT screen = {static_cast<LONG>(mouse.x), static_cast<LONG>(mouse.y)};
  HWND target = WindowFromPoint(screen);
  if (!target || target == GetDesktopWindow())
    return;
  if (is_builtin_control(target))
    return;
  LPARAM local = mouse_lparam(target, mouse);
  WPARAM buttons = mouse_buttons(io);
  SendMessageA(target, WM_MOUSEMOVE, buttons, local);

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    SendMessageA(target, ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ?
                         WM_LBUTTONDBLCLK : WM_LBUTTONDOWN, buttons, local);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    SendMessageA(target, WM_LBUTTONUP, buttons, local);
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    SendMessageA(target, ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right) ?
                         WM_RBUTTONDBLCLK : WM_RBUTTONDOWN, buttons, local);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    SendMessageA(target, WM_RBUTTONUP, buttons, local);
  if (io.MouseWheel != 0.0f) {
    WPARAM wheel = MAKEWPARAM(buttons, static_cast<int>(io.MouseWheel * 120.0f));
    SendMessageA(target, WM_MOUSEWHEEL, wheel, MAKELPARAM(screen.x, screen.y));
  }
}

}  // namespace

extern "C" int shandalar_win32_imgui_init(SDL_Window *window, SDL_Renderer *renderer)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsClassic();
  ImGui::GetIO().IniFilename = nullptr;
  g_window = window;
  g_renderer = renderer;
  return ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) &&
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
  if (!ImGui::GetCurrentContext()) return;
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  RECT rect = {};
  GetClientRect(GetDesktopWindow(), &rect);
  ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(rect.right),
                                      static_cast<float>(rect.bottom));
  ImGui::NewFrame();
}

extern "C" void shandalar_win32_imgui_present(SDL_Renderer *renderer)
{
  if (!renderer || !ImGui::GetCurrentContext())
    return;
  ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
  HWND desktop = GetDesktopWindow();
  HWND window = GetWindow(desktop, GW_CHILD);
  if (window)
    window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    draw_window_tree(renderer, window, draw_list);
  draw_controls();
  forward_mouse();
}

extern "C" void shandalar_win32_imgui_render(SDL_Renderer *renderer)
{
  if (!ImGui::GetCurrentContext()) return;
  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

extern "C" void shandalar_win32_imgui_shutdown(void)
{
  while (g_textures) {
    WindowTexture *next = g_textures->next;
    if (g_textures->texture)
      SDL_DestroyTexture(g_textures->texture);
    free(g_textures->pixels);
    free(g_textures);
    g_textures = next;
  }
  if (ImGui::GetCurrentContext()) {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }
  g_window = nullptr;
  g_renderer = nullptr;
}
