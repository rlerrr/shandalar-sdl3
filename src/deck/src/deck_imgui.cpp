#include "deck_imgui.h"

#include "deck_core.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <stdint.h>
#include <string.h>

namespace {

constexpr int kLogicalWidth = 1024;
constexpr int kLogicalHeight = 768;
constexpr int kBrowserX = 8;
constexpr int kBrowserY = 610;
constexpr int kBrowserWidth = 1008;
constexpr int kBrowserHeight = 158;
constexpr int kFullCardX = 8;
constexpr int kFullCardY = 92;
constexpr int kFullCardWidth = 288;
constexpr int kFullCardHeight = 432;
constexpr int kCardWidth = 120;
constexpr int kCardHeight = 120;
constexpr int kCellWidth = 144;
constexpr int kVisibleCards = 7;
constexpr int kWheelCards = 3;
constexpr int kTextureCacheSize = 32;
constexpr const char *kBrowserCardPayload = "DECK_BROWSER_CARD";
constexpr const char *kDeckCardPayload = "DECK_SURFACE_CARD";

struct CachedCardTexture {
  int csvid = -1;
  uint64_t last_used = 0;
  SDL_Texture *texture = nullptr;
};

CachedCardTexture g_card_textures[kTextureCacheSize];
SDL_Renderer *g_renderer = nullptr;
uint64_t g_frame_number = 0;
int g_browser_first = 0;
int g_active_deck_index = -1;

ImVec4 rgb(unsigned char r, unsigned char g, unsigned char b)
{
  return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
}

void apply_windows98_style()
{
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.ChildRounding = 0.0f;
  style.FrameRounding = 0.0f;
  style.PopupRounding = 0.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabRounding = 0.0f;
  style.TabRounding = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.WindowPadding = ImVec2(4.0f, 4.0f);
  style.FramePadding = ImVec2(4.0f, 2.0f);
  style.ItemSpacing = ImVec2(4.0f, 4.0f);
  style.ScrollbarSize = 16.0f;
  style.GrabMinSize = 12.0f;

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = rgb(0, 0, 0);
  colors[ImGuiCol_TextDisabled] = rgb(128, 128, 128);
  colors[ImGuiCol_WindowBg] = rgb(192, 192, 192);
  colors[ImGuiCol_ChildBg] = rgb(192, 192, 192);
  colors[ImGuiCol_PopupBg] = rgb(192, 192, 192);
  colors[ImGuiCol_Border] = rgb(0, 0, 0);
  colors[ImGuiCol_FrameBg] = rgb(192, 192, 192);
  colors[ImGuiCol_FrameBgHovered] = rgb(210, 210, 210);
  colors[ImGuiCol_FrameBgActive] = rgb(160, 160, 160);
  colors[ImGuiCol_TitleBg] = rgb(0, 0, 128);
  colors[ImGuiCol_TitleBgActive] = rgb(0, 0, 128);
  colors[ImGuiCol_TitleBgCollapsed] = rgb(128, 128, 128);
  colors[ImGuiCol_MenuBarBg] = rgb(192, 192, 192);
  colors[ImGuiCol_ScrollbarBg] = rgb(192, 192, 192);
  colors[ImGuiCol_ScrollbarGrab] = rgb(160, 160, 160);
  colors[ImGuiCol_ScrollbarGrabHovered] = rgb(128, 128, 128);
  colors[ImGuiCol_ScrollbarGrabActive] = rgb(96, 96, 96);
  colors[ImGuiCol_CheckMark] = rgb(0, 0, 0);
  colors[ImGuiCol_SliderGrab] = rgb(160, 160, 160);
  colors[ImGuiCol_SliderGrabActive] = rgb(96, 96, 96);
  colors[ImGuiCol_Button] = rgb(192, 192, 192);
  colors[ImGuiCol_ButtonHovered] = rgb(210, 210, 210);
  colors[ImGuiCol_ButtonActive] = rgb(160, 160, 160);
  colors[ImGuiCol_Header] = rgb(0, 0, 128);
  colors[ImGuiCol_HeaderHovered] = rgb(0, 0, 160);
  colors[ImGuiCol_HeaderActive] = rgb(0, 0, 128);
  colors[ImGuiCol_Separator] = rgb(128, 128, 128);
  colors[ImGuiCol_ResizeGrip] = rgb(192, 192, 192);
  colors[ImGuiCol_ResizeGripHovered] = rgb(160, 160, 160);
  colors[ImGuiCol_ResizeGripActive] = rgb(128, 128, 128);
  colors[ImGuiCol_DragDropTarget] = rgb(0, 0, 128);
}

void destroy_card_textures()
{
  for (CachedCardTexture &entry : g_card_textures) {
    SDL_DestroyTexture(entry.texture);
    entry = CachedCardTexture{};
  }
}

SDL_Texture *card_texture(int csvid)
{
  CachedCardTexture *slot = nullptr;
  uint32_t pixels[kCardWidth * kCardHeight];

  for (CachedCardTexture &entry : g_card_textures) {
    if (entry.csvid == csvid && entry.texture) {
      entry.last_used = g_frame_number;
      return entry.texture;
    }
    if (!slot || !entry.texture || entry.last_used < slot->last_used)
      slot = &entry;
  }

  if (!slot || !deck_ui_render_small_card(csvid, pixels, kCardWidth, kCardHeight))
    return nullptr;
  if (!slot->texture) {
    slot->texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STATIC, kCardWidth, kCardHeight);
    if (!slot->texture)
      return nullptr;
    SDL_SetTextureScaleMode(slot->texture, SDL_SCALEMODE_LINEAR);
  }
  if (!SDL_UpdateTexture(slot->texture, nullptr, pixels, kCardWidth * sizeof(uint32_t)))
    return nullptr;
  slot->csvid = csvid;
  slot->last_used = g_frame_number;
  return slot->texture;
}

void handle_browser_keyboard()
{
  int selected = deck_ui_selected_index();
  int count = deck_ui_available_count();
  int next = selected;
  if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
    ++next;
  if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
    --next;
  if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
    next += 7;
  if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
    next -= 7;
  if (count > 0) {
    if (next < 0)
      next = 0;
    if (next >= count)
      next = count - 1;
  }
  if (next != selected) {
    deck_ui_select_available_index(next);
    if (next < g_browser_first)
      g_browser_first = next;
    else if (next >= g_browser_first + kVisibleCards)
      g_browser_first = next - kVisibleCards + 1;
  }
  if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) &&
      selected >= 0)
    deck_ui_add_card(deck_ui_available_csvid(selected));
}

void draw_browser()
{
  const int count = deck_ui_available_count();
  const int selected = deck_ui_selected_index();
  const int max_first = count > kVisibleCards ? count - kVisibleCards : 0;
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoSavedSettings;

  if (g_browser_first > max_first)
    g_browser_first = max_first;
  if (g_browser_first < 0)
    g_browser_first = 0;
  ImGui::SetNextWindowPos(ImVec2((float)kBrowserX, (float)kBrowserY));
  ImGui::SetNextWindowSize(ImVec2((float)kBrowserWidth, (float)kBrowserHeight));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
  if (ImGui::Begin("Card Browser", nullptr, flags)) {
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
      handle_browser_keyboard();

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
      const ImGuiIO &io = ImGui::GetIO();
      float wheel = io.MouseWheelH != 0.0f ? io.MouseWheelH : -io.MouseWheel;
      if (wheel > 0.0f)
        g_browser_first += kWheelCards;
      else if (wheel < 0.0f)
        g_browser_first -= kWheelCards;
      if (g_browser_first < 0)
        g_browser_first = 0;
      if (g_browser_first > max_first)
        g_browser_first = max_first;
    }

    int last = g_browser_first + kVisibleCards;
    if (last > count)
      last = count;
    for (int index = g_browser_first; index < last; ++index) {
      const int slot = index - g_browser_first;
      const int csvid = deck_ui_available_csvid(index);
      SDL_Texture *texture = card_texture(csvid);
      ImGui::SetCursorPos(ImVec2((float)(slot * kCellWidth), 0.0f));
      ImGui::PushID(index);
      ImGui::TextUnformatted(deck_ui_card_name(csvid));
      ImGui::SetCursorPosX((float)(slot * kCellWidth));
      if (index == selected)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.78f, 0.55f, 0.18f, 1.0f));
      const bool clicked = texture &&
          ImGui::ImageButton("card", (ImTextureID)(intptr_t)texture,
                             ImVec2((float)kCardWidth, (float)kCardHeight));
      if (index == selected)
        ImGui::PopStyleColor();
      if (clicked) {
        deck_ui_select_available_index(index);
        ImGui::SetWindowFocus();
      }
      if (ImGui::IsItemHovered()) {
        deck_ui_preview_card(csvid);
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
          deck_ui_add_card(csvid);
      }
      if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload(kBrowserCardPayload, &csvid, sizeof(csvid));
        ImGui::Image((ImTextureID)(intptr_t)texture,
                     ImVec2((float)kCardWidth, (float)kCardHeight));
        ImGui::EndDragDropSource();
      }
      ImGui::PopID();
    }
    if (max_first > 0) {
      ImGui::SetCursorPos(ImVec2(8.0f, 136.0f));
      ImGui::SetNextItemWidth((float)kBrowserWidth - 24.0f);
      ImGui::SliderInt("##card_scroll", &g_browser_first, 0, max_first, "",
                       ImGuiSliderFlags_AlwaysClamp);
    }
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
}

bool payload_is(const char *type)
{
  const ImGuiPayload *payload = ImGui::GetDragDropPayload();
  return payload && payload->IsDataType(type);
}

void draw_browser_drop_target()
{
  if (!payload_is(kDeckCardPayload))
    return;
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;
  ImGui::SetNextWindowPos(ImVec2((float)kBrowserX, (float)kBrowserY));
  ImGui::SetNextWindowSize(ImVec2((float)kBrowserWidth, (float)kBrowserHeight));
  ImGui::Begin("Browser Drop Target", nullptr, flags);
  ImGui::InvisibleButton("browser_drop_target", ImGui::GetContentRegionAvail());
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kDeckCardPayload)) {
      if (payload->DataSize == sizeof(int))
        deck_ui_remove_card(*(const int *)payload->Data);
    }
    ImGui::EndDragDropTarget();
  }
  ImGui::End();
}

void draw_deck_surface_interactions()
{
  constexpr int surface_x = 304;
  constexpr int surface_y = 0;
  constexpr int surface_width = 720;
  constexpr int surface_height = 524;
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;

  ImGui::SetNextWindowPos(ImVec2((float)surface_x, (float)surface_y));
  ImGui::SetNextWindowSize(ImVec2((float)surface_width, (float)surface_height));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Deck Surface Actions", nullptr, flags);

  if (payload_is(kBrowserCardPayload)) {
    ImGui::InvisibleButton("deck_drop_target", ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload(kBrowserCardPayload)) {
        if (payload->DataSize == sizeof(int))
          deck_ui_add_card(*(const int *)payload->Data);
      }
      ImGui::EndDragDropTarget();
    }
  } else {
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const int count = deck_ui_deck_count();
    int hovered_index = -1;
    int hovered_csvid = -1;
    int hovered_x = 0;
    int hovered_y = 0;
    int hovered_w = 0;
    int hovered_h = 0;

    for (int index = 0; index < count; ++index) {
      int csvid;
      int amount;
      int x;
      int y;
      int width;
      int height;
      if (!deck_ui_deck_entry(index, &csvid, &amount, &x, &y, &width, &height))
        continue;
      if (mouse.x >= x && mouse.x < x + width &&
          mouse.y >= y && mouse.y < y + height) {
        hovered_index = index;
        hovered_csvid = csvid;
        hovered_x = x;
        hovered_y = y;
        hovered_w = width;
        hovered_h = height;
      }
    }

    int item_index = g_active_deck_index >= 0 ? g_active_deck_index : hovered_index;
    int csvid = hovered_csvid;
    int amount;
    int x = hovered_x;
    int y = hovered_y;
    int width = hovered_w;
    int height = hovered_h;
    if (item_index >= 0 &&
        deck_ui_deck_entry(item_index, &csvid, &amount, &x, &y, &width, &height)) {
      ImGui::SetCursorPos(ImVec2((float)(x - surface_x), (float)(y - surface_y)));
      ImGui::PushID(item_index);
      ImGui::InvisibleButton("deck_card", ImVec2((float)width, (float)height));
      if (ImGui::IsItemHovered())
        deck_ui_preview_card(csvid);
      if (ImGui::IsItemHovered() &&
          ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        deck_ui_remove_card(csvid);
      if (ImGui::IsItemActive())
        g_active_deck_index = item_index;
      if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        SDL_Texture *texture = card_texture(csvid);
        ImGui::SetDragDropPayload(kDeckCardPayload, &csvid, sizeof(csvid));
        if (texture)
          ImGui::Image((ImTextureID)(intptr_t)texture,
                       ImVec2((float)width, (float)height));
        ImGui::EndDragDropSource();
      }
      ImGui::PopID();
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        !payload_is(kDeckCardPayload))
      g_active_deck_index = -1;
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void draw_full_card_context_menu()
{
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;
  ImGui::SetNextWindowPos(ImVec2((float)kFullCardX, (float)kFullCardY));
  ImGui::SetNextWindowSize(ImVec2((float)kFullCardWidth, (float)kFullCardHeight));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Full Card Actions", nullptr, flags);
  ImGui::InvisibleButton("full_card", ImGui::GetContentRegionAvail());
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    const int selected = deck_ui_selected_index();
    if (selected >= 0)
      deck_ui_add_card(deck_ui_available_csvid(selected));
  }
  if (ImGui::BeginPopupContextItem("full_card_actions")) {
    bool expanded = deck_ui_expanded_text() != 0;
    if (ImGui::MenuItem("Expand text box", nullptr, expanded))
      deck_ui_set_expanded_text(!expanded);
    ImGui::EndPopup();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace

extern "C" int deck_imgui_init(SDL_Window *window, SDL_Renderer *renderer)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsLight();
  apply_windows98_style();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = nullptr;
  g_renderer = renderer;
  return ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) &&
         ImGui_ImplSDLRenderer3_Init(renderer);
}

extern "C" void deck_imgui_shutdown(void)
{
  destroy_card_textures();
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  g_renderer = nullptr;
}

extern "C" int deck_imgui_process_event(SDL_Renderer *renderer, const SDL_Event *event)
{
  SDL_Event logical_event;
  if (!event)
    return 0;
  logical_event = *event;
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    SDL_RenderCoordinatesFromWindow(renderer, event->motion.x, event->motion.y,
                                    &logical_event.motion.x, &logical_event.motion.y);
  } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
             event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    SDL_RenderCoordinatesFromWindow(renderer, event->button.x, event->button.y,
                                    &logical_event.button.x, &logical_event.button.y);
  }
  ImGui_ImplSDL3_ProcessEvent(&logical_event);
  const ImGuiIO &io = ImGui::GetIO();
  switch (event->type) {
  case SDL_EVENT_MOUSE_MOTION:
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
  case SDL_EVENT_MOUSE_WHEEL:
    return io.WantCaptureMouse;
  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP:
  case SDL_EVENT_TEXT_INPUT:
    return io.WantCaptureKeyboard;
  default:
    return 0;
  }
}

extern "C" void deck_imgui_begin_frame(void)
{
  float mouse_x;
  float mouse_y;

  ++g_frame_number;
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)kLogicalWidth, (float)kLogicalHeight);
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  SDL_GetMouseState(&mouse_x, &mouse_y);
  SDL_RenderCoordinatesFromWindow(g_renderer, mouse_x, mouse_y, &mouse_x, &mouse_y);
  io.AddMousePosEvent(mouse_x, mouse_y);
  ImGui::NewFrame();
  deck_ui_preview_card(-1);
  if (deck_ui_is_interactive()) {
    draw_full_card_context_menu();
    draw_browser();
    draw_deck_surface_interactions();
    draw_browser_drop_target();
  }
}

extern "C" void deck_imgui_render(SDL_Renderer *renderer)
{
  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}
