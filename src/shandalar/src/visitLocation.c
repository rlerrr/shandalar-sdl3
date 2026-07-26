#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#include "defs.h"
#include "shandalar.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"
#include "magic/src/duel_engine.h"
#include "shandalar_global_strings.h"
#include "deckdll/src/drm.h"
#include "deckdll/src/magsnd.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"

extern int global_screen_width;
extern int global_screen_height;
extern HWND g_main_window_hwnd;
extern HANDLE g_main_thread_handle;
extern HPALETTE g_palette_handle;

extern int g_menu_render_guard;
extern int g_mouse_x;
extern int g_mouse_y;
extern int g_mouse_button_down_mask;
extern int g_mouse_button_mask_snapshot;
extern int g_mouse_x_snapshot;
extern int g_mouse_y_snapshot;

extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;

extern DIBSurface *g_graphics_pages[10];

extern char g_ui_message_buffer[0x1000];

extern int g_text_menu_abort_requested;
extern card_data_t global_cards_data[];
extern int g_card_count;
extern int g_default_palette_fade_steps;
extern int g_adventure_world_exit_requested;
extern char g_last_parsed_deck_path[0x110];
extern int g_reveal_all_world_info;
extern int g_current_quest_giver_town_index;
extern int g_done_text_table_entry;
extern int g_world_player_tile_x;
extern int g_world_player_tile_y;
extern int g_duel_selection_pending;
extern char g_text_lines[249][300];
extern int g_loadsave_skip_esc;
extern jmp_buf g_adventure_session_restart_jump_buffer;
extern jmp_buf g_adventure_world_exit_jump_buffer;
extern char g_itoa_buffer[0x20];

extern int card_dummy(int player, int card, event_t event);

typedef struct
{
  int first;
  int second;
} HintPair;

extern int g_hint_difficulty_masks[0x100];
extern HintPair g_hint_card_pairs[0x100];

// GLOBAL: SHANDALAR 0x0073ea68
int g_wizard_siege_count;

typedef union
{
  struct
  {
    EncodedImage *normal[12];
    EncodedImage *highlight[12];
    EncodedImage *pressed[12];
    EncodedImage *icon[12];
  } named;
  EncodedImage *by_group[4][12];
} WorldMagicChoiceButtonSpriteBank;

extern WorldMagicChoiceButtonSpriteBank g_world_magic_choice_button_sprite_bank;

typedef struct
{
  EncodedImage *frame[4][9];
  EncodedImage *icon_rows[4][4];
} DialogBoxSpriteBank;

extern DialogBoxSpriteBank g_dialog_box_sprite_bank;
extern EncodedImage *g_icons_sprite_entries[0x18];

extern WorldMagicSlotTimer g_world_magic_slot_timers[0xc];

typedef struct
{
  int x0;
  int y0;
  int x1;
  int y1;
} BuyCardsHitRect;

void LoadPcxResource(int page_number, int x, int y, char *path, void *opaque);
void LoadPcxIntoPage(int page_number, char *path);
void LoadPcxIntoPageOpaque(int page_number, char *path);
void LoadPcxIntoPageNoPalette(char *path);
struct PaletteLog *ReadPalette(char *path, char *out_palette);

void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int src_w, int src_h,
                             FacemakerWindowBounds *src, int src_x, int src_y, int copy_w, int copy_h);
void BlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int width, int height,
                      FacemakerWindowBounds *src, int src_x, int src_y);

int ScaleUiCoordinateFrom320(int value);
int ScaleUiCoordinate(int value);

unsigned int GetWorldTileType(int x, int y);
unsigned int GetWorldTileMagicMask(unsigned int tile_mask);
int single_color_test_bit_to_color_t(int mask);
int ClampIntToRange(int value, int min_value, int max_value);
int MeasureTextLineWidth(char *text);

int PopNormalizedQueuedKeyInput(void);
int RunTextMenuAt(char *menu_text, int left_x, int top_y);
int RunTextMenuAtScaled(char *menu_text, int x_320_scale, unsigned int y_200_scale);
int RunRightClickMenuAndQueueInput(void);
void ClearInputAndWaitForMouseRelease(void);
unsigned int WaitForInputEventUnlessBlocked(void);

void PlaySoundWithPitchAndPan(int sound_id, int volume, int pitch_percent, int pan_percent);
unsigned int LoadSoundWithDriveFallback(char *filename, int channel, int flags);

void RefreshAdventureInterfaceLayout(void);
void *DrawAdventureInterfaceLayout(int force_redraw);
void AddJournalEntry(int entry_type, int entry_arg);
void DelayUiTicks(int delay);

void EnterCastleDungeon(int dungeon_index);
void EnsureAdvfac64Loaded(int state);
WPARAM WINAPI DeckBuilderMain(HWND parent_hwnd, int db_flags_1, int db_flags_2);
void ShowWorldMapScreen(int mode);
void ShowDungeonCluesScreen(int unused);
void ShowDungeonClueDetailScreen(int dungeon_index);
int RestoreAdventureUiPaletteAndFocus(void);
int RunAdventureStatsMenu(void);

void DrawAdventureCard(int card_index, int x, int y, int full_card, char *banner_label);
int internal_rand(int max_exclusive);
int ReadSpriteEntryPointers(EncodedImage **out_sprite_entries, char *sprite_path);
void DrawAdventureCardSized(int card_index, int x, int y, int width, int height, int full_card, char *banner_label);
void ShowTownHintTextPopup(int hint_index);
void DrawUiScaledCenteredTextNoShadow(FacemakerWindowBounds *window, int color_index, int x, int y, char *text);
int RecountDeckCardTotals(void);

int AnimatePaletteToColor(int color_index, int palette_id);
int SetFontStyleSize(int font_slot, unsigned int scaled_size);
int GetFontLineHeight(int font_slot);
void UnloadStatWinDllExports(void);
int ShutdownSharedStartup(void);
int FileExists(const char *filename);
int FreeOpeningMenuSpriteWorkEntries(int work_entry_index_a, int work_entry_index_b);
void PlaySoundEffectOnChannel(char *sound_path, int channel, int volume, int pitch_percent, int pan_percent);
char *BuildCreatureNameWithArticle(int creature_type);
void SetWorldMapPixelFlags(unsigned int mask, int x, int y);
int ParseDeckFileIntoInitialLibrary(char *deck_path, csvid_and_numcards *library_entries, unsigned int color_filter, int speed_filter);

int ApplyPortraitPaletteMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, char *palette_source_path, char *portrait_path);
void BeginSpriteEncodeSession(void);
void FinalizeSpriteEncodeSession(void);
EncodedImage *EncodeSpriteFromPage(int page_number, int src_x, int src_y, int width, int height);
void FreeSpriteBlob(void *sprite_blob);
int HasQueuedKeyInput(void);
int IsAdventureRightClickMenuButtonHit(int x, int y);

void DrawFormattedTextNoShadowCentered(FacemakerWindowBounds *dst, int text_color, int x, int y, char *world_magic_button_sprite, ...);
void DrawFormattedTextShadowedCentered(FacemakerWindowBounds *window, int color_index, int x, int y, char *world_magic_button_sprite, ...);
void DrawFormattedTextShadowed(FacemakerWindowBounds *window, int color_index, int x, int y, char *world_magic_button_sprite, ...);
unsigned int BlitRectByRandomTileOrderInPlace(HDC dst, int dst_x, int dst_y, int width, int height, int tile_w, int tile_h, HDC src);
void BlitRectByStaggeredRandomTileOrder(HDC dst_hdc, int x, int y, int w, int h, int strip_width, int active_strip_count,
                                        int tile_width, int tile_height, HDC src_hdc);

void DestroyAllCardBackgrounds(void);
void DestroyAllBigArts(void);
void DestroyAllSmallArts(void);
void InitializeNewGameState(void);
void PreloadWorldAmbientSounds(void);

int ApproximateDistance(int x, int y);
int GetRelativeWorldQuadrant(int world_x, int world_y);
int AddCardToDeckSorted(int card_id);
int GetRemainingAllowedCardCopies(int card_id);
int IsCardColorCompatibleWithMask(int card_color, int color_mask, int compatibility_level);
int GetCardRarity(int card_id);
int FindCardIndexByCsvid(int csvid);
int FindDeckSlotForQuestColorAndType(unsigned char quest_color, int quest_bitmap_mask);
char *GetCreatureName(int creature_type);
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID world_magic_button_sprite, ...);
char *GetQuestCardClassName(int quest_bitmap_mask);
int RunTownServicesMenu(int town_index);
int DeckContainsCsvid(int csvid);
int FindHintPairIndexForOfferCard(int card_internal_id);
int CalculateCardShopPrice(int card_index);
int FindWorldMagicCardIndex(int world_magic_slot_index);
char *GetTownCardDescription(int town_index);
char *BuildTownDisplayName(int town_index);
extern int g_dungeon_monster_duel_music_csvids[];
void FormatQuestSpellName(char *dst, DWORD dst_size);
unsigned int RunWisemanAdviceSequence(int preferred_color, int new_town_visit, int town_index);
int BuyAnyCardFromTown(int payment_color, int town_index);
char *AppendString(char *dst, char *src);

int RunDuelEngine(unsigned int card_id, int creature_type);
int ExitIfNoUsableDeckCards(void);
ShandalarEntryType PickRandomCreatureTypeForWizardTier(int wizard_color, int creature_tier);
void LoadCreatureDuelDeck(int creature_type, unsigned int name_id, unsigned int color_filter, int speed_filter);
void ClearAndLoadInitialLibraryFromDeckFile(char *deck_path, int library_index, unsigned int color_filter, int speed_filter);
int DrawRandomCardFromInitialLibrary(int library_index);
int SelectAdventureListCardIndex(int player, int *card_ids, int card_count, char *title, int require_card_click, int *out_selection);
void DrawCreaturePortrait(int creature_type, int x_320, int y_200, int tinted, int mode);
void PlayDuelMusic(int tune_index);
void RemoveCardFromDeckById(unsigned int card_id);
unsigned int RunCardBrowser(char *title, unsigned int color_mask, unsigned int type_mask, int reset_filters, int show_done_button);
int QuestCardChooserCallback(int x, int y, int width, int height, int frame_style);
int DrawTiledDialogBoxFrame(int x, int y, int width, int height, int frame_style);
int HandleMainMenuButtonControlEvent(void *control_ptr, int event_type);
int HandlePortraitMainMenuControlEvent(void *control_ptr, int event_type);
void CopyGraphicsRect(FacemakerWindowBounds *src_page, int src_x, int src_y, int width, int height, FacemakerWindowBounds *dst_page,
                      int dst_x, int dst_y);
int RunSaveMenuAndSelectSlot(void);
int RunLoadGameMenu(void);
void SaveGameToSlot(int save_slot_index);
int load_selected_duel_save_slot(int save_slot_index);
int RestoreAdventureUiPaletteAndFocus(void);
void ShowWorldMapScreen(int mode);
void ShowCityInfoScreen(int unused);
void ShowDungeonCluesScreen(int unused);
int ShowStatsWindow(int mode, int highlight);

void DrawTownMenuIconWithTooltip(int center_x, int center_y, int icon_index, int draw_mode, char *tooltip_text);
int BeginMenuContext(void);
int ResetMenuContext(int context_index);
int AddMenuControlsToContext(AdvMenuControl *controls, int control_count, int context_index);
int EndMenuContext(void);
int UpdateMenuControlSelection(int mouse_x, int mouse_y, int allow_activate_on_click);
int QueuePendingMenuActionInput(void);
int RenderCurrentMenuContextControls(void);
int LoadTextSectionLines(const char *filename, const char *section);
int RunTextMenuAtScaled(char *menu_text, int x_320_scale, unsigned int y_200_scale);
void ShowMouseCursorNested(void);
void HideMouseCursorNested(void);
void UpdateMouseSnapshot(void);

AdvMenuRect *PushGraphicsClipRect(AdvMenuRect *saved_clip_rect, FacemakerWindowBounds *page, int x, int y, int width, int height);

void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y, int width, int height, EncodedImage *encoded_image);
void DrawEncodedImageUnscaled(FacemakerWindowBounds *dst, int x, int y, EncodedImage *encoded_image);
void DrawGraphicsLine(FacemakerWindowBounds *window_bounds, int x1, int y1, int x2, int y2, int color_index);
void DrawTextAt(FacemakerWindowBounds *window, int color, int x, int y, char *text, ...);
void DrawWorldUiFormattedText(FacemakerWindowBounds *window, int color_index, int x, int y, char *world_magic_button_sprite, ...);
void DrawCenteredTextLineWithShadow(char *text, int center_x, int y, int color_index);

extern EncodedImage *g_questnew_sprite_entries[4];
extern EncodedImage *g_tips_frame_sprite;
extern EncodedImage *g_tips_icon_sprite;
extern EncodedImage *g_main_menu_button_sprites_normal[4];
extern EncodedImage *g_main_menu_button_sprites_highlight[4];

// GLOBAL: SHANDALAR 0x00582d40
int g_visit_tips_frame_x_by_resolution[3] = {0x40, 0x50, 0x66};
// GLOBAL: SHANDALAR 0x00582d50
int g_visit_tips_frame_y_by_resolution[3] = {0x48, 0x5a, 0x72};
// GLOBAL: SHANDALAR 0x00582d60
int g_visit_tips_left_icon_x_by_resolution[3] = {0xcb, 0xfd, 0x143};
// GLOBAL: SHANDALAR 0x00582d6c
int g_visit_tips_right_icon_x_by_resolution[3] = {0x1af, 0x21a, 0x2b0};
// GLOBAL: SHANDALAR 0x00582d78
int g_visit_tips_icon_y_by_resolution[3] = {0x2f, 0x3c, 0x4d};

int sound_unload(int sound_id);
void LoadLoopingSound(char *filename, int sound_id);
void PlaySoundWithPan(int sound_id, int volume, int pan);

extern int g_world_location_music_track_id;
extern int g_world_location_music_town_index;
extern int g_world_location_music_active;

// GLOBAL: SHANDALAR 0x00591278
char *PTR_s_x_sound_dueltune_wav_00591278[3] = {
    "x:sound\\dueltune.wav",
    "x:sound\\winduel.wav",
    "x:sound\\loseduel.wav",
};

// GLOBAL: SHANDALAR 0x005b7e1c
int g_current_town_slot_index;

// GLOBAL: SHANDALAR 0x00580da4
TownDialogCallback g_town_dialog_callback = (TownDialogCallback)DrawTiledDialogBoxFrame;

// GLOBAL: SHANDALAR 0x0058edd8
int g_wiseman_duel_reward_card_csvids[10] = {0x24d, 0x1b3, 0xc1, 0x1b8, 0x106, 0x1d0, 0x87, 0x1dd, 0xd3, 0xd};
// GLOBAL: SHANDALAR 0x0058ee00
int g_wiseman_general_block_index = -1;
// GLOBAL: SHANDALAR 0x0058ee04
int g_wiseman_city_block_index = -1;

// GLOBAL: SHANDALAR 0x0058f0ac
int g_wiseman_card_choice_result = 0;

extern int g_showlibrary_menu_selection;
// GLOBAL: SHANDALAR 0x0058edd0
int g_wiseman_city_block_active_town_count = 0;

// GLOBAL: SHANDALAR 0x0058f0b0
int g_card_browser_color_filter = 1;
// GLOBAL: SHANDALAR 0x0058f0b4
int g_card_browser_type_filter = 1;
// GLOBAL: SHANDALAR 0x0058f0b8
unsigned int g_card_browser_hover_card = 0xffffffff;
// GLOBAL: SHANDALAR 0x0058f0bc
unsigned int g_card_browser_selected_card = 0;

int RenderCardBrowserDoneButton(AdvMenuControl *control, int mode);
int HandleCardBrowserDoneButton(AdvMenuControl *control);
extern int g_card_browser_done_button_colors[4];

// GLOBAL: SHANDALAR 0x0058f0c0
AdvMenuControl g_card_browser_done_button = {
    0x110, 5, 0x61, 0x1b, 0x110, 5, 0x61, 0x1b, 1, (AdvMenuRenderCallback)RenderCardBrowserDoneButton, (AdvMenuActivateCallback)HandleCardBrowserDoneButton, 1, 0, (char *)g_card_browser_done_button_colors, (char *)(&g_card_browser_done_button_colors[2]), 0, 0, {0, 0, 0, 0}};

// GLOBAL: SHANDALAR 0x0058f114
int g_card_browser_done_button_colors[4] = {0x0d644420, 0x1b, 0x0d644420, 0x1b};

// GLOBAL: SHANDALAR 0x00746b10
EncodedImage *g_card_browser_done_button_sprites[3];

// GLOBAL: SHANDALAR 0x00650048
int g_wiseman_city_block_town_choice_index_a;
// GLOBAL: SHANDALAR 0x0065004c
int g_wiseman_city_block_town_choice_index_b;

// GLOBAL: SHANDALAR 0x00650050
int g_wiseman_city_block_color_index;
// GLOBAL: SHANDALAR 0x00650058
char g_wiseman_city_block_subst_a[0x100];
// GLOBAL: SHANDALAR 0x00650158
int g_wiseman_advice_stage;
// GLOBAL: SHANDALAR 0x00650160
char g_wiseman_city_block_subst_b[0x100];
// GLOBAL: SHANDALAR 0x00650260
int g_wiseman_city_block_world_magic_slot_index;

// GLOBAL: SHANDALAR 0x00650268
int g_card_browser_done_button_value;

// GLOBAL: SHANDALAR 0x0058a888
int g_world_magic_offer_slot_index = -1;
// GLOBAL: SHANDALAR 0x0058a88c
int g_last_town_services_town_index = -1;

// GLOBAL: SHANDALAR 0x005b7da0
int g_pending_quest_color;
// GLOBAL: SHANDALAR 0x005b7da4
int g_pending_quest_type;
// GLOBAL: SHANDALAR 0x005b7da8
int g_town_shop_card_ids[8];
// GLOBAL: SHANDALAR 0x005b7de8
int g_town_shop_card_prices[8];
// GLOBAL: SHANDALAR 0x005b7e08
EncodedImage *g_buy_cards_price_tag_sprite;
// GLOBAL: SHANDALAR 0x005b7e10
EncodedImage *g_buy_cards_highlight_button_sprites[3];
// GLOBAL: SHANDALAR 0x005b7e20
EncodedImage *g_buy_cards_preview_panel_sprite;
// GLOBAL: SHANDALAR 0x005b7e28
EncodedImage *g_buy_cards_pressed_button_sprites[3];
// GLOBAL: SHANDALAR 0x005b7e34
int g_town_shop_cache_town_index;
// GLOBAL: SHANDALAR 0x005b7e38
int g_pending_quest_destination;
// GLOBAL: SHANDALAR 0x005b7e40
EncodedImage *g_buy_cards_done_button_sprites[3];
EncodedImage *g_buy_cards_confirm_button_sprites[3];
// GLOBAL: SHANDALAR 0x005b7e4c
EncodedImage *g_buy_cards_title_panel_sprite;

// GLOBAL: SHANDALAR 0x007483ec
int g_active_town_services_town_index;
// GLOBAL: SHANDALAR 0x007481e0
int g_town_preferred_magic_color;
// GLOBAL: SHANDALAR 0x007481d4
int g_town_hint_pair_index;
// GLOBAL: SHANDALAR 0x007481d0
int g_town_food_price;
// GLOBAL: SHANDALAR 0x007481cc
int g_town_menu_show_tooltips;
// GLOBAL: SHANDALAR 0x007481d8
int g_town_menu_result_state;
// GLOBAL: SHANDALAR 0x007481dc
int g_buy_cards_done_clicked;
// GLOBAL: SHANDALAR 0x007483e8
int g_town_hint_offer_slot_index;

// GLOBAL: SHANDALAR 0x007481f0
char g_town_button_labels[5][0x64];

int VisitTownWiseman(AdvMenuControl *control);

int RenderBuyCardsDoneButton(AdvMenuControl *control, int mode);
int RunBuyCardsConfirmPrompt(int card_id, int price);
extern char g_town_leave_button_mode_colors[];

// GLOBAL: SHANDALAR 0x0058ab88
AdvMenuControl g_buy_cards_done_button_control = {
    0x114,
    0x112,
    0x58,
    0x22,
    0x114,
    0x112,
    0x58,
    0x22,
    1,
    (AdvMenuRenderCallback)RenderBuyCardsDoneButton,
    (AdvMenuActivateCallback)0,
    0,
    0,
    g_town_leave_button_mode_colors + 8,
    g_town_leave_button_mode_colors + 0xc,
    0,
    0,
    {0, 0, 0, 0},
};

// GLOBAL: SHANDALAR 0x0058abdc
char g_town_leave_button_mode_colors[] = "\x1b \0\0\x1b \0\0\x1b \0\0\x1b \0";

// GLOBAL: SHANDALAR 0x0058aeb8
int s_empty_buy_card_banner_0058aeb8 = 0;
// GLOBAL: SHANDALAR 0x0058aebc
int s_empty_buy_card_banner_0058aebc = 0;

int RenderTownIconMenuControl(AdvMenuControl *control, int mode);
int LeaveTownServicesMenu(AdvMenuControl *control);
int BuyFoodFromTown(AdvMenuControl *control);
int VisitTownWiseman(AdvMenuControl *control);
int BuyTownSpecialCardOffer(AdvMenuControl *control);
int OpenTownDeckBuilder(AdvMenuControl *control);
int OpenTownWorldMap(AdvMenuControl *control);
int OpenTownDungeonClues(AdvMenuControl *control);
int OpenTownAdventureStats(AdvMenuControl *control);
int RunTownBuyCardsScreen(AdvMenuControl *control);

// GLOBAL: SHANDALAR 0x0058a890
AdvMenuControl g_town_main_menu_controls[4] = {
    {3, 0x47, 0x39, 0x36, 3, 0x47, 0x39, 0x36, 1, (AdvMenuRenderCallback)HandleMainMenuButtonControlEvent, (AdvMenuActivateCallback)OpenTownDeckBuilder, 0x70, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {3, 0x7e, 0x39, 0x36, 3, 0x7e, 0x39, 0x36, 1, (AdvMenuRenderCallback)HandleMainMenuButtonControlEvent, (AdvMenuActivateCallback)OpenTownWorldMap, 0x71, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {3, 0xb5, 0x39, 0x36, 3, 0xb5, 0x39, 0x36, 1, (AdvMenuRenderCallback)HandleMainMenuButtonControlEvent, (AdvMenuActivateCallback)OpenTownDungeonClues, 0x73, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {3, 0xec, 0x39, 0x36, 3, 0xec, 0x39, 0x36, 1, (AdvMenuRenderCallback)HandlePortraitMainMenuControlEvent, (AdvMenuActivateCallback)OpenTownAdventureStats, 0x74, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

// GLOBAL: SHANDALAR 0x0058a9e0
AdvMenuControl g_town_icon_menu_controls[5] = {
    {0x181, 0x30, 0xaa, 0x5d, 0x181, 0x30, 0xaa, 0x5d, 1, (AdvMenuRenderCallback)RenderTownIconMenuControl, (AdvMenuActivateCallback)OpenTownDeckBuilder, 5, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0x54, 0xea, 0xaa, 0x5d, 0x54, 0xea, 0xaa, 0x5d, 1, (AdvMenuRenderCallback)RenderTownIconMenuControl, (AdvMenuActivateCallback)BuyFoodFromTown, 1, 1, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0x181, 0xea, 0xaa, 0x5d, 0x181, 0xea, 0xaa, 0x5d, 1, (AdvMenuRenderCallback)RenderTownIconMenuControl, (AdvMenuActivateCallback)LeaveTownServicesMenu, 2, 2, g_town_leave_button_mode_colors, g_town_leave_button_mode_colors + 4, 0, 0, {0, 0, 0, 0}},
    {0x54, 0x30, 0xaa, 0x5d, 0x54, 0x30, 0xaa, 0x5d, 1, (AdvMenuRenderCallback)RenderTownIconMenuControl, (AdvMenuActivateCallback)RunTownBuyCardsScreen, 4, 3, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0xea, 0x8d, 0xaa, 0x5d, 0xea, 0x8d, 0xaa, 0x5d, 1, (AdvMenuRenderCallback)RenderTownIconMenuControl, (AdvMenuActivateCallback)VisitTownWiseman, 3, 4, 0, 0, 0, 0, {0, 0, 0, 0}},
};

// GLOBAL: SHANDALAR 0x005a1870
int g_ui_fx_lcg_multipliers[0x14] = {5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79};

#define GET_TOWN_PIC(_i) ((g_town_slots[_i].location_type == 1) ? "village.pic" : "city.pic")

int IsCardAvailable(csvid_t csvid, int expansion);

// FUNCTION: SHANDALAR 0x00428b05
unsigned int GetCardAvailabilityMask(int card_index)
{
  unsigned int mask = IsCardAvailable(global_cards_data[card_index].id, 0);

  if (HasExpansion(2) != 0)
  {
    mask |= IsCardAvailable(global_cards_data[card_index].id, 1);
  }

  if (HasExpansion(4) != 0)
  {
    mask |= IsCardAvailable(global_cards_data[card_index].id, 2);
  }

  return mask;
}

// FUNCTION: SHANDALAR 0x0056615f
int IsWizardColorFeminine(int wizard_color)
{
  if (wizard_color == 1)
  {
    return 0;
  }
  else if (wizard_color == 2)
  {
    return 1;
  }
  else if (wizard_color == 3)
  {
    return 0;
  }
  else if (wizard_color == 4)
  {
    return 1;
  }
  else if (wizard_color == 5)
  {
    return 0;
  }
}

// FUNCTION: SHANDALAR 0x004ceae2
void DestroyCachedCardArt(void)
{
  DestroyAllCardBackgrounds();
  DestroyAllBigArts();
  DestroyAllSmallArts();
}

// FUNCTION: SHANDALAR 0x0042fe00
int DrawVisitScreenOverlays(FacemakerWindowBounds *page)
{
  AdvMenuRect clip_restore;
  AdvMenuRect clip_temp;
  int resolution_index;
  EncodedImage *questnew_entry;
  AdvMenuRect saved_clip_rect;

  questnew_entry = g_questnew_sprite_entries[0];

  switch (global_screen_width)
  {
  case 0x280:
    resolution_index = 0;
    break;
  case 0x320:
    resolution_index = 1;
    break;
  case 0x400:
    resolution_index = 2;
    break;
  }

  saved_clip_rect =
      *PushGraphicsClipRect(&clip_temp, g_page0_window_bounds, ScaleUiCoordinate(0x40), ScaleUiCoordinate(0x30), ScaleUiCoordinateFrom320(0x100), ScaleUiCoordinateFrom320(0x8c));

  DrawEncodedImageResampled(page, ScaleUiCoordinate(0x181), ScaleUiCoordinate(0x135), ScaleUiCoordinate((int)*(short *)((char *)questnew_entry + 4)),
                            ScaleUiCoordinate((int)*(short *)((char *)questnew_entry + 6)), g_questnew_sprite_entries[0]);

  DrawEncodedImageUnscaled(page, g_visit_tips_frame_x_by_resolution[resolution_index], g_visit_tips_frame_y_by_resolution[resolution_index], g_tips_frame_sprite);
  DrawEncodedImageUnscaled(page, g_visit_tips_left_icon_x_by_resolution[resolution_index], g_visit_tips_icon_y_by_resolution[resolution_index], g_tips_icon_sprite);
  DrawEncodedImageUnscaled(page, g_visit_tips_right_icon_x_by_resolution[resolution_index], g_visit_tips_icon_y_by_resolution[resolution_index], g_tips_icon_sprite);

  PushGraphicsClipRect(&clip_restore, g_page0_window_bounds, saved_clip_rect.x, saved_clip_rect.y, saved_clip_rect.width, saved_clip_rect.height);
  return 0;
}

// FUNCTION: SHANDALAR 0x004ed44b
void LoadVisitBackdropAndCopyToPage0(char *pcx_path)
{
  int dst_x;
  int dst_y;
  int dst_w;
  int dst_h;

  dst_x = ScaleUiCoordinate(0x40);
  dst_y = ScaleUiCoordinate(0x30);
  dst_w = ScaleUiCoordinate(0x200);
  dst_h = ScaleUiCoordinate(0x118);

  LoadPcxResource(1, 0, global_screen_height - 0x118, pcx_path, (void *)0);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x118, 0x200, 0x118, g_page1_window_bounds,
                          dst_x, dst_y, dst_w, dst_h);
  DrawVisitScreenOverlays(g_page1_window_bounds);
  BlitGraphicsRect(g_page1_window_bounds, dst_x, dst_y, dst_w, dst_h, g_page0_window_bounds, dst_x, dst_y);
}

// FUNCTION: SHANDALAR 0x004ed525
void AnimateVisitBackdropZoomIn(char *pcx_path)
{
  int scale_h;
  int scale_w;
  int unused_local;
  int scale_step;
  int dst_y;
  int center_y;
  int copy_h;
  int copy_w;

  dst_y = global_screen_height - 0x118;
  LoadPcxResource(1, 0, dst_y, pcx_path, (void *)0);

  copy_w = ScaleUiCoordinateFrom320(0x100);
  copy_h = ScaleUiCoordinateFrom320(0x8c);
  center_y = ScaleUiCoordinateFrom320(0x5e);

  for (scale_step = 2; scale_step <= 8; scale_step = scale_step + 1)
  {
    scale_w = copy_w * scale_step / 8;
    scale_h = copy_h * scale_step / 8;
    unused_local = ((copy_h / 2) * (scale_step - 2) + (8 - scale_step) * (global_screen_height / 3)) / 6;
    StretchBlitGraphicsRect(g_page1_window_bounds, 0, dst_y, 0x200, 0x118, g_page0_window_bounds,
                            global_screen_width / 2 - scale_w / 2, center_y - scale_h / 2, scale_w, scale_h);
  }

  scale_w = copy_w;
  scale_h = copy_h;
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, dst_y, 0x200, 0x118, g_page1_window_bounds,
                          global_screen_width / 2 - scale_w / 2, center_y - scale_h / 2, scale_w, scale_h);
  DrawVisitScreenOverlays(g_page0_window_bounds);
}

// FUNCTION: SHANDALAR 0x004ed6b9
int ShowWizardVisitBackdrop(char *pcx_path)
{
  DIBSurface *src;
  DIBSurface *dst;
  int x;
  int y;
  int w;
  int h;

  x = ScaleUiCoordinate(0x40);
  y = ScaleUiCoordinate(0x30);
  w = ScaleUiCoordinate(0x200);
  h = ScaleUiCoordinate(0x118);

  src = g_graphics_pages[g_page1_window_bounds->page_number];
  dst = g_graphics_pages[g_page0_window_bounds->page_number];

  LoadPcxResource(1, 0, global_screen_height - 0x118, pcx_path, (void *)0);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x118, 0x200, 0x118, g_page1_window_bounds,
                          x, y, w, h);
  DrawVisitScreenOverlays(g_page1_window_bounds);
  BlitRectByStaggeredRandomTileOrder(dst->hTempDC, x, y, w, h, 0x20, 3, 8, 8, src->hTempDC);
  return 0;
}

// FUNCTION: SHANDALAR 0x00561bf4
void TriggerWizardSiegeNewsflash(int wizard_color)
{
  int unused_local;             /* -0x80 */
  int ruled_by_color_count[7];  /* -0x7c */
  int town_index;               /* -0x60 */
  int color;                    /* -0x5c */
  int best_score;               /* -0x58 */
  int nearest_distance;         /* -0x54 */
  int best_color;               /* -0x50 */
  int lair_world_y_by_color[7]; /* -0x4c */
  int lair_world_x_by_color[7]; /* -0x30 */
  int scan_index;               /* -0x14 */
  int creature_tier;            /* -0x10 */
  int dist;                     /* -0x0c */
  int best_town_index;          /* -0x08 */
  int score;                    /* -0x04 */

  if (g_siege_indicator != 0)
  {
    return;
  }

  ClearInputAndWaitForMouseRelease();

  for (scan_index = 0; scan_index < 7; scan_index++)
  {
    lair_world_x_by_color[scan_index] = -1;
    ruled_by_color_count[scan_index] = 0;
  }

  for (scan_index = 0; scan_index < 0x80; scan_index++)
  {
    if (g_town_slots[scan_index].location_type == 4)
    {
      color = single_color_test_bit_to_color_t((int)GetWorldTileMagicMask(GetWorldTileType(g_town_slots[scan_index].world_x, g_town_slots[scan_index].world_y)));
      if (color == wizard_color)
      {
        lair_world_x_by_color[color] = g_town_slots[scan_index].world_x;
        lair_world_y_by_color[color] = g_town_slots[scan_index].world_y;
      }
    }

    if (g_town_slots[scan_index].status_and_ruling_wizard & 0xff00)
    {
      ruled_by_color_count[g_town_slots[scan_index].status_and_ruling_wizard >> 8] += 1;
    }
  }

  unused_local = 0;
  best_score = 0x7fff;

  for (town_index = 0; town_index < 0x80; town_index++)
  {
    if ((((g_town_slots[town_index].status_and_ruling_wizard & 0xff00) || (g_town_slots[town_index].location_type == 4)) ||
         (g_town_slots[town_index].location_type == 1)) ||
        (g_town_slots[town_index].location_type == 5))
    {
      continue;
    }

    nearest_distance = 0x7fff;

    for (scan_index = 1; scan_index < 6; scan_index++)
    {
      if (lair_world_x_by_color[scan_index] == -1)
      {
        continue;
      }

      dist = ApproximateDistance(g_town_slots[town_index].world_x - lair_world_x_by_color[scan_index],
                                 g_town_slots[town_index].world_y - lair_world_y_by_color[scan_index]);
      if (dist < nearest_distance)
      {
        nearest_distance = dist;
        color = scan_index;
      }
    }

    score = (ruled_by_color_count[color] << 5) + internal_rand(0x80);
    if (score < best_score)
    {
      best_score = score;
      best_town_index = town_index;
      best_color = color;
    }
  }

  color = best_color;
  town_index = best_town_index;
  if (color == -1)
  {
    return;
  }

  scan_index = 7;
  switch (g_siege_timer / 128)
  {
  case 0:
    creature_tier = 4;
    break;
  case 1:
    creature_tier = 6;
    break;
  case 2:
    creature_tier = 8;
    break;
  case 3:
    creature_tier = 0xc;
    break;
  default:
    if (g_town_slots[town_index].world_y & 1)
    {
      creature_tier = 0xc;
    }
    else
    {
      creature_tier = 0x10;
    }
    break;
  }

  FreeOpeningMenuSpriteWorkEntries(scan_index, scan_index + 8);
  g_lair_or_monster_slots[scan_index].entry_type = PickRandomCreatureTypeForWizardTier(color, creature_tier);
  g_lair_or_monster_slots[scan_index].world_x = g_town_slots[town_index].world_x * 0x20 + 0x10;
  g_lair_or_monster_slots[scan_index].world_y = g_town_slots[town_index].world_y * 0x20 + 0x10;
  g_lair_or_monster_slots[scan_index].color = color;
  g_wizard_siege_count = g_wizard_siege_count + 1;
  SetWorldMapPixelFlags(0x80, g_town_slots[town_index].world_x, g_town_slots[town_index].world_y);

  PlaySoundEffectOnChannel("x:sound\\newsflash.wav", 0x97, 100, 100, 0);
  AnimateVisitBackdropZoomIn("newsback.pic");

  strcpy(g_ui_message_buffer, gs_newsflash_0077d140[0]);
  if (IsWizardColorFeminine(color) != 0)
  {
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_newsflash_0077d140[2], gs_wizardnames_0077ee70[color],
                                                BuildCreatureNameWithArticle(g_lair_or_monster_slots[scan_index].entry_type), BuildTownDisplayName(town_index));
  }
  else
  {
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_newsflash_0077d140[3], gs_wizardnames_0077ee70[color],
                                                BuildCreatureNameWithArticle(g_lair_or_monster_slots[scan_index].entry_type), BuildTownDisplayName(town_index));
  }

  g_page0_window_bounds->font_slot = 5;
  DrawTextAt(g_page0_window_bounds, 0xbe, 0x140, 0xf7, g_ui_message_buffer);
  g_page0_window_bounds->font_slot = 1;
  ClearInputAndWaitForMouseRelease();
  (void)WaitForInputEventUnlessBlocked();
  RefreshAdventureInterfaceLayout();
  g_siege_indicator = 1;
}

// FUNCTION: SHANDALAR 0x00430fb5
void DrawUiScaledCenteredText(char *text, int center_x_320, int y_200, int color_index)
{
  center_x_320 = (center_x_320 * global_screen_width) / 0x140;
  y_200 = (y_200 * global_screen_height) / 0xf0;
  DrawCenteredTextLineWithShadow(text, center_x_320, y_200, color_index);
}

// FUNCTION: SHANDALAR 0x004313cf
void DrawUiScaledSprite(FacemakerWindowBounds *dst, int x_320, int y_200, int w_320, int h_200, EncodedImage *sprite)
{
  DrawEncodedImageResampled(dst, (x_320 * global_screen_width) / 0x140, (y_200 * global_screen_height) / 0xf0,
                            (w_320 * global_screen_width) / 0x140, (h_200 * global_screen_height) / 0xf0, sprite);
}

// FUNCTION: SHANDALAR 0x005661d9
int IsCreatureTypeFeminine(int creature_type)
{
  if ((creature_type < 0) || ((int)gs_creature_name_count_00593934 - 1 < creature_type))
  {
    return 0;
  }
  if (g_shandalar_monster_definitions[creature_type].plural[0x32] == 'f')
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: SHANDALAR 0x0056bfa4
void RemoveDeckSlot(int deck_slot_index)
{
  int i;

  for (i = deck_slot_index + 1; i < 500; i = i + 1)
  {
    deck[i - 1] = deck[i];
  }
  deck[499] = -1;
}

// FUNCTION: SHANDALAR 0x004f2aa1
void DrawTownMenuIconWithTooltip(int center_x, int center_y, int icon_index, int draw_mode, char *tooltip_text)
{
  EncodedImage *bubble_right; /* -0x24 */
  int line_height;            /* -0x20 */
  EncodedImage *bubble_left;  /* -0x1c */
  EncodedImage *bubble_mid;   /* -0x18 */
  EncodedImage *tmp_sprite;   /* -0x14 */
  int icon_x;                 /* -0x10 */
  int tile_index;             /* -0x0c */
  int bubble_width;           /* -0x08 */
  int is_resampled_mode;      /* -0x04 */

  is_resampled_mode = (draw_mode == 2);
  icon_x = center_x - 0x2b;
  center_y = center_y - 0x26;

  switch (draw_mode)
  {
  case 0:
    draw_mode = 0;
    break;
  case 1:
    draw_mode = 1;
    break;
  case 2:
    draw_mode = 1;
    break;
  case 3:
    return;
  }

  if (is_resampled_mode == 0)
  {
    DrawEncodedImageUnscaled(g_page0_window_bounds, icon_x + 8, center_y + 6, g_icons_sprite_entries[icon_index]);
    DrawEncodedImageUnscaled(g_page0_window_bounds, icon_x, center_y, g_dialog_box_sprite_bank.icon_rows[draw_mode][0]);
  }
  else
  {
    tmp_sprite = g_icons_sprite_entries[icon_index];
    DrawEncodedImageResampled(g_page1_window_bounds, icon_x + 0xa, center_y + 8, (int)tmp_sprite->width - 4, (int)tmp_sprite->height - 2,
                              g_icons_sprite_entries[icon_index]);

    tmp_sprite = g_dialog_box_sprite_bank.icon_rows[draw_mode][0];
    DrawEncodedImageResampled(g_page1_window_bounds, icon_x + 2, center_y + 2, (int)tmp_sprite->width - 4, (int)tmp_sprite->height - 2,
                              g_dialog_box_sprite_bank.icon_rows[draw_mode][0]);

    BlitGraphicsRect(g_page1_window_bounds, icon_x, center_y, (int)tmp_sprite->width, (int)tmp_sprite->height, g_page0_window_bounds, icon_x,
                     center_y);
  }

  if (g_town_menu_show_tooltips != 0)
  {
    line_height = GetFontLineHeight(g_page0_window_bounds->font_slot) + ScaleUiCoordinate(8);

    bubble_left = g_dialog_box_sprite_bank.icon_rows[draw_mode][2];
    bubble_mid = g_dialog_box_sprite_bank.icon_rows[draw_mode][1];
    bubble_right = g_dialog_box_sprite_bank.icon_rows[draw_mode][3];

    bubble_width = ClampIntToRange(MeasureTextLineWidth(tooltip_text) + 0x20, 0x60, 0x3e7);

    for (tile_index = 0; tile_index < bubble_width / 0x40; tile_index = tile_index + 1)
    {
      DrawEncodedImageResampled(g_page0_window_bounds, (center_x - bubble_width / 2) + tile_index * 0x40, center_y + 0x38, 0x40, line_height,
                                bubble_mid);
    }

    DrawEncodedImageResampled(g_page0_window_bounds, center_x + bubble_width / 2 - 0x40, center_y + 0x38, 0x40, line_height,
                              g_dialog_box_sprite_bank.icon_rows[draw_mode][1]);
    DrawEncodedImageResampled(g_page0_window_bounds, center_x - bubble_width / 2 - 8, center_y + 0x38, (int)bubble_left->width, line_height,
                              g_dialog_box_sprite_bank.icon_rows[draw_mode][2]);
    DrawEncodedImageResampled(g_page0_window_bounds, center_x + bubble_width / 2, center_y + 0x38, (int)bubble_right->width, line_height,
                              g_dialog_box_sprite_bank.icon_rows[draw_mode][3]);

    DrawFormattedTextNoShadowCentered(g_page0_window_bounds, 0xff, center_x, center_y + line_height / 2 + 0x38, tooltip_text);
  }
}

// FUNCTION: SHANDALAR 0x004f23bc
void FormatQuestSpellName(char *dst, DWORD dst_size)
{
  FormatMessageFromStringStripCarriageReturns(dst, dst_size, gs_spellname_primary_0077e6e0, gs_spellnames_0077e220[g_current_quest_color],
                                              GetQuestCardClassName(1 << (g_current_quest_destination & 3)));
}

// FUNCTION: SHANDALAR 0x004ef0a8
int RenderTownIconMenuControl(AdvMenuControl *control, int mode)
{
  int in_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || ((control->x + control->width) < g_mouse_x))
    {
      in_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || ((control->y + control->height) < g_mouse_y))
    {
      in_bounds = 0;
    }
    else
    {
      in_bounds = 1;
    }

    if (in_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawTownMenuIconWithTooltip(control->x + control->width / 2, control->y + control->height / 2, control->selection_value - 1, mode,
                              g_town_button_labels[control->data_value]);

  if ((mode == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    PlaySoundWithPitchAndPan(0x12, 100, 100, 0);
    control->on_activate(control);
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x004ef07e
int LeaveTownServicesMenu(AdvMenuControl *control)
{
  (void)control;
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_town_menu_result_state = -1;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f042c
int BuyFoodFromTown(AdvMenuControl *control)
{
  (void)control;
  if (g_town_food_price <= Gold)
  {
    g_food = g_food + 10;
    Gold = Gold - g_town_food_price;
  }

  DrawAdventureInterfaceLayout(1);
  g_page0_window_bounds->page_number = 1;
  LoadVisitBackdropAndCopyToPage0(GET_TOWN_PIC(g_current_town_slot_index));
  g_page0_window_bounds->page_number = 0;

  // 0x0058aa34 == &g_town_icon_menu_controls[1]
  (void)RenderTownIconMenuControl(&g_town_icon_menu_controls[1], 1);
  return 0;
}

// FUNCTION: SHANDALAR 0x004f04c5
int VisitTownWiseman(AdvMenuControl *control)
{
  char quest_spell_name[100];        /* -0x78 */
  TownDialogCallback saved_callback; /* -0x14 */
  int temp_flag;                     /* -0x10 */
  int town_index;                    /* -0x0c */
  int preferred_color;               /* -0x08 */
  int quest_time_units;              /* -0x04 */

  town_index = g_active_town_services_town_index;
  preferred_color = g_town_preferred_magic_color;
  saved_callback = g_town_dialog_callback;

  if ((g_town_slots[town_index].status_and_ruling_wizard & 4) != 0)
  {
    AnimateVisitBackdropZoomIn("wiseman3.pic");
    strcpy(g_ui_message_buffer, gs_citywiseman_0074d800[0]);
    g_page0_window_bounds->font_slot = 1;
    DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                              ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 4, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    (void)WaitForInputEventUnlessBlocked();
    AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
    g_town_menu_result_state = -2;
    return 0;
  }

  if (((g_current_quest_destination == -1) && (g_pending_quest_destination != -1)) &&
      (((g_shandalar_difficulty + 3) * 0x10) <
       (g_quest_restock_timer - g_town_slots[town_index].quest_restock_timer)))
  {
    AnimateVisitBackdropZoomIn("wiseman3.pic");
    g_current_quest_destination = g_pending_quest_destination;
    g_current_quest_color = g_pending_quest_color;
    g_current_quest_type = g_pending_quest_type;

    if ((g_current_quest_type == 0) || (g_current_quest_type == 2))
    {
      SetWorldMapPixelFlags(0x80, g_town_slots[g_current_quest_destination].world_x, g_town_slots[g_current_quest_destination].world_y);
      g_current_quest_data = GetRelativeWorldQuadrant(g_town_slots[g_current_quest_destination].world_x, g_town_slots[g_current_quest_destination].world_y);

      FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_citywiseman_0074d800[1],
                                                  gs_directions_00765d50[g_current_quest_data], BuildTownDisplayName(g_current_quest_destination));

      if (g_current_quest_type == 0)
      {
        temp_flag = 0;
        strcat(g_ui_message_buffer, gs_citywiseman_0074d800[2]);
      }
      else
      {
        temp_flag = 1;
        sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_citywiseman_0074d800[3], gs_amuletnames_0077d090[g_current_quest_color]);
      }
      quest_time_units = 0x20;
    }
    else if (g_current_quest_type == 1)
    {
      SetWorldMapPixelFlags(0x80, g_town_slots[g_current_quest_destination].world_x, g_town_slots[g_current_quest_destination].world_y);
      g_current_quest_data = GetRelativeWorldQuadrant(g_town_slots[g_current_quest_destination].world_x, g_town_slots[g_current_quest_destination].world_y);

      FormatQuestSpellName(quest_spell_name, 100);

      FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_citywiseman_0074d800[4],
                                                  quest_spell_name, gs_directions_00765d50[g_current_quest_data], BuildTownDisplayName(g_current_quest_destination));
      temp_flag = 1;
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_citywiseman_0074d800[5], gs_amuletnames_0077d090[g_current_quest_color]);
      quest_time_units = 0x28;
    }
    else
    {
      if (g_current_quest_type < 0)
      {
        if (g_shandalar_monster_definitions[-g_current_quest_type].plural[0x33] == '\x12')
        {
          sprintf(g_ui_message_buffer, gs_citywiseman_0074d800[6], GetCreatureName(-g_current_quest_type));
        }
        else
        {
          sprintf(g_ui_message_buffer, gs_citywiseman_0074d800[7], GetCreatureName(-g_current_quest_type));
        }

        if (g_town_slots[town_index].location_type == 1)
        {
          temp_flag = (int)g_shandalar_monster_definitions[-g_current_quest_type].plural[0x33] / 7 + 1;
          if (temp_flag == 1)
          {
            sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_citywiseman_0074d800[8],
                    gs_amuletnames_0077d090[g_current_quest_color]);
          }
          else
          {
            FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_citywiseman_0074d800[9],
                                                        temp_flag, gs_amuletnames_plural_0077edd0[g_current_quest_color]);
          }
        }
        else
        {
          temp_flag = 0;
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_citywiseman_0074d800[10], GetTownCardDescription(town_index));
        }

        quest_time_units = 0x18;
      }
    }

    strcat(g_ui_message_buffer, gs_citywiseman_0074d800[0xb]);
    g_town_dialog_callback = (TownDialogCallback)QuestCardChooserCallback;

    if (RunTextMenuAtScaled(g_ui_message_buffer, 0x2d, 0x24) != 0)
    {
      g_current_quest_destination = -1;
      g_current_quest_type = 0;
    }
    else
    {
      LoadVisitBackdropAndCopyToPage0("wiseman3.pic");
      g_current_quest_giver_town_index = town_index;
      g_current_quest_deadline = quest_time_units * 2 + g_quest_restock_timer - 1;
      g_town_slots[town_index].quest_restock_timer = g_quest_restock_timer;

      sprintf(g_ui_message_buffer, gs_citywiseman_0074d800[0xc], quest_time_units / 8);
      (void)RunTextMenuAtScaled(g_ui_message_buffer, 0x48, 0x48);
      SetWorldMapPixelFlags(0x80, g_town_slots[town_index].world_x, g_town_slots[town_index].world_y);
      AddJournalEntry(JOURNAL_ENTRY_QUEST_ACCEPTED, g_current_quest_type);
    }

    g_town_dialog_callback = saved_callback;
  }
  else
  {
    (void)RunWisemanAdviceSequence(preferred_color, (g_last_town_services_town_index != town_index), town_index);
  }

  g_last_town_services_town_index = town_index;
  (void)DrawAdventureInterfaceLayout(1);
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f18c0
int BuyTownSpecialCardOffer(AdvMenuControl *control)
{
  int town_index;
  int preferred_color;

  (void)control;
  town_index = g_active_town_services_town_index;
  preferred_color = g_town_preferred_magic_color;
  BuyAnyCardFromTown(preferred_color, town_index);

  g_town_slots[town_index].special_card_restock_timer = g_quest_restock_timer;
  RefreshAdventureInterfaceLayout();
  RecountDeckCardTotals();
  DrawAdventureInterfaceLayout(1);

  /* Branchless selection of which town backdrop we use (0 or +0xc). */
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f1951
int OpenTownDeckBuilder(AdvMenuControl *control)
{
  int town_index;

  (void)control;
  town_index = g_active_town_services_town_index;
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  DeckBuilderMain(g_main_window_hwnd, 1, 3);
  RestoreAdventureUiPaletteAndFocus();
  RefreshAdventureInterfaceLayout();
  DrawAdventureInterfaceLayout(1);
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f19e9
int OpenTownWorldMap(AdvMenuControl *control)
{
  int town_index;

  (void)control;
  town_index = g_active_town_services_town_index;
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  ShowWorldMapScreen(0);
  RefreshAdventureInterfaceLayout();
  DrawAdventureInterfaceLayout(1);
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f1a67
int OpenTownDungeonClues(AdvMenuControl *control)
{
  int town_index;

  (void)control;
  town_index = g_active_town_services_town_index;
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  ShowDungeonCluesScreen(1);
  RefreshAdventureInterfaceLayout();
  DrawAdventureInterfaceLayout(1);
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f1b4e
int OpenTownAdventureStats(AdvMenuControl *control)
{
  int town_index;

  (void)control;
  town_index = g_active_town_services_town_index;
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  ClearInputAndWaitForMouseRelease();
  RunAdventureStatsMenu();
  RefreshAdventureInterfaceLayout();
  DrawAdventureInterfaceLayout(1);
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  return 0;
}

// FUNCTION: SHANDALAR 0x004f0d09
int RunTownBuyCardsScreen(AdvMenuControl *control)
{
  char *accept_scan;   /* -0x134 */
  int deck_slot;       /* -0x130 */
  int accepted;        /* -0x12c */
  char *accept_cursor; /* -0x128 */
  int mouse_y;         /* -0x124 */
  (void)control;
  int mouse_x;               /* -0x120 */
  int title_y_200;           /* -0x11c */
  int title_line_h;          /* -0x118 */
  int center_x_320;          /* -0x114 */
  int title_text_w;          /* -0x110 */
  char *title;               /* -0x10c */
  int loop_i;                /* -0x108 */
  int colors[3];             /* -0x104 */
  EncodedImage *sprites[13]; /* -0xf8 */
  int menu_context;          /* -0xc4 */
  int tile_w;                /* -0xc0 */
  int selected_idx;          /* -0xbc */
  int mana_mask;             /* -0xb8 */
  int key;                   /* -0xb4 */
  int idx;                   /* -0xb0 */
  int y_base;                /* -0xac */
  int item_count;            /* -0xa8 */
  int town_index;            /* -0xa4 */
  BuyCardsHitRect rects[10]; /* -0xa0 */

  town_index = g_active_town_services_town_index;
  item_count = 0;

  ClearInputAndWaitForMouseRelease();

  mana_mask = (int)GetWorldTileMagicMask(GetWorldTileType(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y));

  item_count = g_town_slots[town_index].location_type + 3;
  if (g_world_magic_slot_timers[1].town_index == 0)
  {
    item_count = item_count + 1;
  }

  if (item_count == 0)
  {
    g_town_shop_card_ids[item_count] = single_color_test_bit_to_color_t(mana_mask) - 1;
    g_town_shop_card_prices[item_count] = 0x28;
    item_count = item_count + 1;
  }

  colors[0] = 0x1b;
  colors[1] = 0x1a;
  colors[2] = 0x1a;

  ReadSpriteEntryPointers(&sprites[0], "BuyButtons.spr");
  g_buy_cards_preview_panel_sprite = sprites[0];
  g_buy_cards_title_panel_sprite = sprites[1];
  g_buy_cards_price_tag_sprite = sprites[2];

  for (loop_i = 0; loop_i < 3; loop_i = loop_i + 1)
  {
    g_buy_cards_highlight_button_sprites[loop_i] = (&sprites[6])[loop_i];
  }

  for (loop_i = 0; loop_i < 3; loop_i = loop_i + 1)
  {
    g_buy_cards_pressed_button_sprites[loop_i] = (&sprites[9])[loop_i];
  }

  LoadPcxIntoPage(1, "smBuybttn.pic");
  g_page1_window_bounds->font_slot = 7;
  BeginSpriteEncodeSession();
  for (loop_i = 0; loop_i < 3; loop_i = loop_i + 1)
  {
    g_buy_cards_confirm_button_sprites[loop_i] = EncodeSpriteFromPage(1, loop_i * 0x5a + 1, 1, 0x59, 0x23);
  }
  for (loop_i = 0; loop_i < 3; loop_i = loop_i + 1)
  {
    SetFontStyleSize(7, (loop_i == 2) ? 9 : 11);
    DrawFormattedTextNoShadowCentered(g_page1_window_bounds, colors[loop_i], loop_i * 0x5a + 0x2e, 0x10, "%s", (char *)g_done_text_table_entry);
    g_buy_cards_done_button_sprites[loop_i] = EncodeSpriteFromPage(1, loop_i * 0x5a + 1, 1, 0x59, 0x23);
  }
  FinalizeSpriteEncodeSession();

  LoadPcxResource(1, 0, global_screen_height - 0x118, "buycards.pic", (void *)0);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x118, 0x200, 0x118, g_page0_window_bounds,
                          ScaleUiCoordinateFrom320(0x20), ScaleUiCoordinateFrom320(0x18), ScaleUiCoordinateFrom320(0x100), ScaleUiCoordinateFrom320(0x8c));
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x118, 0x200, 0x118, g_page1_window_bounds,
                          ScaleUiCoordinateFrom320(0x20), ScaleUiCoordinateFrom320(0x18), ScaleUiCoordinateFrom320(0x100), ScaleUiCoordinateFrom320(0x8c));
  DrawVisitScreenOverlays(g_page0_window_bounds);

  if (g_town_hint_pair_index != -1)
  {
    ShowTownHintTextPopup(g_town_hint_pair_index);
  }

  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  if (g_buy_cards_done_button_control.x == g_buy_cards_done_button_control.base_x)
  {
    g_buy_cards_done_button_control.x = ScaleUiCoordinate(g_buy_cards_done_button_control.x);
    g_buy_cards_done_button_control.y = ScaleUiCoordinate(g_buy_cards_done_button_control.y);
    g_buy_cards_done_button_control.width = ScaleUiCoordinate(g_buy_cards_done_button_control.width);
    g_buy_cards_done_button_control.height = ScaleUiCoordinate(g_buy_cards_done_button_control.height);
  }
  AddMenuControlsToContext(&g_buy_cards_done_button_control, 1, menu_context);

redraw_buy_cards_screen:
  LoadPcxIntoPage(1, "buycards.pic");
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x200, 0x118, g_page0_window_bounds, ScaleUiCoordinateFrom320(0x20), ScaleUiCoordinateFrom320(0x18), ScaleUiCoordinateFrom320(0x100),
                          ScaleUiCoordinateFrom320(0x8c));
  DrawVisitScreenOverlays(g_page0_window_bounds);

  RenderCurrentMenuContextControls();
  (void)UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);

  center_x_320 = 0xa0;
  title_y_200 = 0x1f;
  title = gs_visit_citybuy_0077f1d0[0];
  g_page0_window_bounds->font_slot = 4;
  title_line_h = GetFontLineHeight(g_page0_window_bounds->font_slot);
  title_text_w = MeasureTextLineWidth(title);
  DrawEncodedImageResampled(g_page0_window_bounds, global_screen_width / 2 - title_text_w / 2 - 0x14,
                            (title_y_200 * global_screen_height) / 0xf0 - title_line_h, title_text_w + 0x28, title_line_h * 3, g_buy_cards_title_panel_sprite);
  DrawUiScaledCenteredText(title, center_x_320, title_y_200, 0x1b);

  y_base = 0x40;
  tile_w = 0xf4 / item_count;
  memset(rects, 0xff, 0xa0);

  for (idx = item_count - 1; idx >= 0; idx = idx - 1)
  {
    if (g_town_shop_card_ids[idx] != -1)
    {
      DrawUiScaledSprite(g_page0_window_bounds, tile_w * idx + 0x2c, (g_town_shop_card_ids[idx] & 7) + y_base - 0xc, tile_w - 4, 0xc, g_buy_cards_price_tag_sprite);

      sprintf(g_ui_message_buffer, gs_visit_citybuy_0077f1d0[1], g_town_shop_card_prices[idx]);
      g_page0_window_bounds->font_slot = 1;
      DrawUiScaledCenteredText(g_ui_message_buffer, tile_w * idx + tile_w / 2 + 0x29, (g_town_shop_card_ids[idx] & 7) + y_base - 8, 0x1b);

      DrawAdventureCard(g_town_shop_card_ids[idx],
                        ClampIntToRange(tile_w * idx + tile_w / 2 + 0x12, 0, global_screen_width - 0x62),
                        (g_town_shop_card_ids[idx] & 7) + y_base + 4, 0, (char *)&s_empty_buy_card_banner_0058aeb8);

      rects[idx].x0 = ScaleUiCoordinateFrom320(ClampIntToRange(tile_w * idx + tile_w / 2 + 0x12, 0, global_screen_width - 0x62));
      rects[idx].y0 = ScaleUiCoordinateFrom320((g_town_shop_card_ids[idx] & 7) + y_base + 4);
      rects[idx].x1 = rects[idx].x0 + ScaleUiCoordinateFrom320(0x30);
      rects[idx].y1 = rects[idx].y0 + ScaleUiCoordinateFrom320(0x30);
    }
  }

  g_buy_cards_done_clicked = 0;
  while (g_buy_cards_done_clicked == 0)
  {
    selected_idx = -1;
    UpdateMouseSnapshot();
    mouse_x = g_mouse_x_snapshot;
    mouse_y = g_mouse_y_snapshot;

    if (g_mouse_button_down_mask != 0)
    {
      for (idx = 0; idx < item_count; idx = idx + 1)
      {
        if ((rects[idx].x0 <= mouse_x) && (mouse_x <= rects[idx].x1) &&
            (rects[idx].y0 <= mouse_y) && (mouse_y <= rects[idx].y1))
        {
          selected_idx = idx;
          break;
        }
      }

      if (selected_idx == -1)
      {
        (void)UpdateMenuControlSelection(mouse_x, mouse_y, g_mouse_button_down_mask);
        continue;
      }

      key = RunBuyCardsConfirmPrompt(g_town_shop_card_ids[idx], g_town_shop_card_prices[idx]);
      (void)DrawAdventureInterfaceLayout(1);

      accept_cursor = gs_visit_citybuy_0077f1d0[3];
      accepted = key == -2;
      if (key > 0)
      {
        accepted = 0;
        while (*accept_cursor != '\0')
        {
          accept_scan = accept_cursor;
          accept_cursor = accept_cursor + 1;
          if ((int)(signed char)*accept_scan == key)
          {
            accepted = 1;
          }
        }
      }

      if ((accepted != 0) && (g_town_shop_card_prices[idx] <= Gold))
      {
        Gold = Gold - g_town_shop_card_prices[idx];
        deck_slot = AddCardToDeckSorted((unsigned int)g_town_shop_card_ids[idx]);
        deck[deck_slot] = deck[deck_slot] | 0x4000;
        g_town_shop_card_ids[idx] = -1;
        g_town_shop_card_prices[idx] = 0;

        if (g_town_hint_offer_slot_index == idx)
        {
          g_town_hint_pair_index = -1;
        }

        g_town_slots[town_index].card_restock_timers[idx] =
            internal_rand(5) * (g_shandalar_difficulty + 2) + g_quest_restock_timer;
        RecountDeckCardTotals();
        (void)DrawAdventureInterfaceLayout(1);
      }
      goto redraw_buy_cards_screen;
    }

    (void)UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
  }

  (void)EndMenuContext();
  RefreshAdventureInterfaceLayout();
  (void)DrawAdventureInterfaceLayout(1);
  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  g_town_menu_result_state = -2;
  FreeSpriteBlob(g_buy_cards_preview_panel_sprite);
  FreeSpriteBlob(g_buy_cards_done_button_sprites[0]);
  return 0;
}

int RunBuyCardsConfirmPrompt(int card_id, int price)
{
  int key;
  int modal_x;
  int modal_y;
  int modal_w;
  int modal_h;
  int accept_x;
  int cancel_x;
  int button_y;
  int button_w;
  int button_h;
  int text_y;
  int accept_state;
  int cancel_state;
  int text_offset;
  int restore_h;

  sprintf(g_ui_message_buffer, gs_visit_citybuy_0077f1d0[2], price);
  modal_x = ScaleUiCoordinateFrom320(0xdc) / 2;
  modal_y = ScaleUiCoordinateFrom320(0x34) / 2;
  modal_w = ScaleUiCoordinateFrom320(0xc5) / 2;
  modal_h = ScaleUiCoordinateFrom320(0x10f) / 2;

  button_w = ScaleUiCoordinate((int)g_buy_cards_confirm_button_sprites[0]->width);
  button_h = ScaleUiCoordinate((int)g_buy_cards_confirm_button_sprites[0]->height);
  accept_x = global_screen_width / 2 - button_w - ScaleUiCoordinate(4);
  cancel_x = global_screen_width / 2 + ScaleUiCoordinate(4);
  button_y = modal_y + modal_h + ScaleUiCoordinate(8);
  restore_h = button_y + button_h - modal_y;

  BlitGraphicsRect(g_page0_window_bounds, modal_x, modal_y, modal_w, restore_h,
                   g_page1_window_bounds, modal_x, modal_y);
  DrawEncodedImageResampled(g_page0_window_bounds, modal_x, modal_y, modal_w, modal_h, g_buy_cards_preview_panel_sprite);
  DrawAdventureCardSized(card_id, 0x7a, 0x29, 0x4b, 0x70, 1, (char *)&s_empty_buy_card_banner_0058aebc);
  DrawUiScaledCenteredTextNoShadow(g_page0_window_bounds, 0x1b, 0x140, 0x47, g_ui_message_buffer);
  DrawEncodedImageResampled(g_page0_window_bounds, accept_x, button_y, button_w, button_h,
                            g_buy_cards_confirm_button_sprites[0]);
  DrawEncodedImageResampled(g_page0_window_bounds, cancel_x, button_y, button_w, button_h,
                            g_buy_cards_confirm_button_sprites[0]);
  text_y = button_y + button_h / 2 - GetFontLineHeight(g_page0_window_bounds->font_slot) / 2;
  DrawFormattedTextNoShadowCentered(g_page0_window_bounds, 0x1b,
                                    accept_x + button_w / 2, text_y, "Yes");
  DrawFormattedTextNoShadowCentered(g_page0_window_bounds, 0x1b,
                                    cancel_x + button_w / 2, text_y, "No");

  ClearInputAndWaitForMouseRelease();
  do
  {
    UpdateMouseSnapshot();
    accept_state = ((accept_x <= g_mouse_x_snapshot) && (g_mouse_x_snapshot <= accept_x + button_w) &&
                    (button_y <= g_mouse_y_snapshot) && (g_mouse_y_snapshot <= button_y + button_h))
                       ? ((g_mouse_button_mask_snapshot & 1U) ? 2 : 1)
                       : 0;
    cancel_state = ((cancel_x <= g_mouse_x_snapshot) && (g_mouse_x_snapshot <= cancel_x + button_w) &&
                    (button_y <= g_mouse_y_snapshot) && (g_mouse_y_snapshot <= button_y + button_h))
                       ? ((g_mouse_button_mask_snapshot & 1U) ? 2 : 1)
                       : 0;
    DrawEncodedImageResampled(g_page0_window_bounds, accept_x, button_y, button_w, button_h,
                              g_buy_cards_confirm_button_sprites[accept_state]);
    DrawEncodedImageResampled(g_page0_window_bounds, cancel_x, button_y, button_w, button_h,
                              g_buy_cards_confirm_button_sprites[cancel_state]);
    text_offset = (accept_state == 2) ? ScaleUiCoordinate(1) : 0;
    text_y = button_y + button_h / 2 - GetFontLineHeight(g_page0_window_bounds->font_slot) / 2 + text_offset;
    DrawFormattedTextNoShadowCentered(g_page0_window_bounds, (accept_state == 0) ? 0x1b : 0x1a,
                                      accept_x + button_w / 2 + text_offset, text_y, "Yes");
    text_offset = (cancel_state == 2) ? ScaleUiCoordinate(1) : 0;
    text_y = button_y + button_h / 2 - GetFontLineHeight(g_page0_window_bounds->font_slot) / 2 + text_offset;
    DrawFormattedTextNoShadowCentered(g_page0_window_bounds, (cancel_state == 0) ? 0x1b : 0x1a,
                                      cancel_x + button_w / 2 + text_offset, text_y, "No");
    if ((g_mouse_button_mask_snapshot & 1U) != 0)
    {
      if (accept_state != 0)
      {
        BlitGraphicsRect(g_page1_window_bounds, modal_x, modal_y, modal_w, restore_h,
                         g_page0_window_bounds, modal_x, modal_y);
        return -2;
      }
      if (cancel_state != 0)
      {
        BlitGraphicsRect(g_page1_window_bounds, modal_x, modal_y, modal_w, restore_h,
                         g_page0_window_bounds, modal_x, modal_y);
        return 0;
      }
    }
    DelayUiTicks(1);
  } while (HasQueuedKeyInput() == 0);

  key = PopNormalizedQueuedKeyInput();
  BlitGraphicsRect(g_page1_window_bounds, modal_x, modal_y, modal_w, restore_h,
                   g_page0_window_bounds, modal_x, modal_y);
  return key;
}

// FUNCTION: SHANDALAR 0x004f0b50
int RenderBuyCardsDoneButton(AdvMenuControl *control, int mode)
{
  int mouse_inside;           /* -0x1c */
  int y_top;                  /* -0x18 */
  int x_left;                 /* -0x14 */
  int h_scaled;               /* -0x10 */
  int w_scaled;               /* -0x0c */
  EncodedImage *sprite;       /* -0x08 */
  FacemakerWindowBounds *dst; /* -0x04 */

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || ((control->x + control->width) < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || ((control->y + control->height) < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }

    if (mouse_inside == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  if (mode != 2)
  {
    dst = g_page0_window_bounds;
  }
  else
  {
    dst = g_page1_window_bounds;
  }
  sprite = g_buy_cards_done_button_sprites[0];
  w_scaled = ScaleUiCoordinateFrom320((int)sprite->width) / 2;
  h_scaled = ScaleUiCoordinateFrom320((int)sprite->height) / 2;
  x_left = global_screen_width / 2 - w_scaled / 2;
  y_top = ScaleUiCoordinateFrom320(0x112) / 2;

  DrawEncodedImageResampled(dst, x_left, y_top, w_scaled, h_scaled, g_buy_cards_done_button_sprites[mode]);

  if (mode == 2)
  {
    PlaySoundWithPitchAndPan(0x12, 100, 100, 0);
    BlitGraphicsRect(g_page1_window_bounds, x_left, y_top, w_scaled, h_scaled, g_page0_window_bounds, x_left, y_top);
    g_buy_cards_done_clicked = 1;
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00418a2d
int DeckContainsCsvid(int csvid)
{
  int i;

  for (i = 0; i < 500; i++)
  {
    if (global_cards_data[deck[i] & 0xfff].id == csvid)
    {
      return 1;
    }
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x0041879d
int FindHintPairIndexForOfferCard(int card_internal_id)
{
  int candidate_hint_pair_count;
  int deck_scan_index;
  int hint_pair_index;
  int is_hint_second_in_deck;
  int result_hint_pair_index;
  int deck_card_internal_id;
  int is_hint_first_in_deck;
  int candidate_hint_pair_indices[256];

  result_hint_pair_index = -1;
  candidate_hint_pair_count = 0;
  hint_pair_index = 0;

  for (; hint_pair_index < 0x100; hint_pair_index = hint_pair_index + 1)
  {
    if (g_hint_card_pairs[hint_pair_index].second != -1)
    {
      is_hint_first_in_deck = DeckContainsCsvid(g_hint_card_pairs[hint_pair_index].first);
      is_hint_second_in_deck = DeckContainsCsvid(g_hint_card_pairs[hint_pair_index].second);

      if ((is_hint_first_in_deck == 0) || (is_hint_second_in_deck == 0))
      {
        if ((g_hint_difficulty_masks[hint_pair_index] & (1 << (unsigned char)g_shandalar_difficulty)) != 0)
        {
          if ((global_cards_data[card_internal_id].id == g_hint_card_pairs[hint_pair_index].first) && (is_hint_second_in_deck != 0))
          {
            candidate_hint_pair_indices[candidate_hint_pair_count] = hint_pair_index;
            candidate_hint_pair_count = candidate_hint_pair_count + 1;
          }

          if ((g_hint_card_pairs[hint_pair_index].second == global_cards_data[card_internal_id].id) && (is_hint_first_in_deck != 0))
          {
            candidate_hint_pair_indices[candidate_hint_pair_count] = hint_pair_index;
            candidate_hint_pair_count = candidate_hint_pair_count + 1;
          }
        }
      }
    }
    else
    {
      if (global_cards_data[card_internal_id].id == g_hint_card_pairs[hint_pair_index].first)
      {
        if ((DeckContainsCsvid(g_hint_card_pairs[hint_pair_index].first) == 0) &&
            ((g_hint_difficulty_masks[hint_pair_index] & (1 << (unsigned char)g_shandalar_difficulty)) != 0))
        {
          for (deck_scan_index = 0; deck_scan_index < 500; deck_scan_index = deck_scan_index + 1)
          {
            deck_card_internal_id = deck[deck_scan_index] & 0xfff;

            if (((unsigned char)global_cards_data[deck_card_internal_id].type & 1) != 0)
            {
              if (((signed char)global_cards_data[card_internal_id].color &
                   (signed char)global_cards_data[deck_card_internal_id].color) != 0)
              {
                result_hint_pair_index = hint_pair_index;
                break;
              }
            }
          }
        }
      }
    }
  }

  if (candidate_hint_pair_count == 0)
  {
    return -1;
  }
  else
  {
    return candidate_hint_pair_indices[internal_rand(candidate_hint_pair_count)];
  }
}

// FUNCTION: SHANDALAR 0x004f1bcc
int SellPrice(int card_index)
{
  int tile_magic_mask;
  int price;

  tile_magic_mask = GetWorldTileMagicMask(GetWorldTileType(g_town_slots[g_town_shop_cache_town_index].world_x,
                                                           g_town_slots[g_town_shop_cache_town_index].world_y));
  price = CalculateCardShopPrice(card_index);
  price = (g_town_slots[g_town_shop_cache_town_index].location_type + 2) * price;

  if (((int)(char)global_cards_data[card_index].color != tile_magic_mask) &&
      ((char)global_cards_data[card_index].color != 0))
  {
    if (IsCardColorCompatibleWithMask(tile_magic_mask, (int)(char)global_cards_data[card_index].color, 3) != 0)
    {
      price = (price * 4) / 3;
    }
    else
    {
      price = (price * 3) / 2;
    }
  }

  price = (price / 0x32) * 5;
  return price;
}

// FUNCTION: SHANDALAR 0x004f1cce
int CalculateCardShopPrice(int card_index)
{
  int base_value;
  int creature_value;
  int price;

  price = 0x28;
  switch (global_cards_data[card_index].type)
  {
  case 2:
  case 66:
    if (((int)global_cards_data[card_index].power & 0xffffbfff) + 2 != 0)
    {
      base_value = ((int)global_cards_data[card_index].power & 0xffffbfff) + 3;
    }
    else
    {
      base_value = (int)global_cards_data[card_index].power;
    }

    if (((int)global_cards_data[card_index].toughness & 0xffffbfff) + 2 != 0)
    {
      creature_value = ((int)global_cards_data[card_index].toughness & 0xffffbfff) + 3;
    }
    else
    {
      creature_value = (int)global_cards_data[card_index].toughness;
    }

    price = (global_cards_data[card_index].ai_base_value + base_value) * creature_value * 5;
    if ((global_cards_data[card_index].static_ability & 0x1f) != 0)
    {
      price = (price * 3) / 2;
    }
    if ((global_cards_data[card_index].static_ability & 0x200) != 0)
    {
      price = (price * 3) / 2;
    }
    if (global_cards_data[card_index].code_pointer != card_dummy)
    {
      price = (price * 3) / 2;
    }
    if ((global_cards_data[card_index].static_ability & 0x1c0U) != 0)
    {
      price = (price * 3) / 2;
    }
    if ((global_cards_data[card_index].extra_ability & 8) != 0)
    {
      price = price * 3;
    }
    if ((global_cards_data[card_index].extra_ability & 0x10) != 0)
    {
      price = price * 3;
    }

    price = price / ((char)global_cards_data[card_index].cc[0] + abs((int)(char)global_cards_data[card_index].cc[1]) + 1);
    break;

  case 4:
    price = (abs((int)(char)global_cards_data[card_index].cc[1]) + (char)global_cards_data[card_index].cc[0] + global_cards_data[card_index].ai_base_value + 1) * 25;
    if ((global_cards_data[card_index].extra_ability & 3) != 0)
    {
      price *= 2;
    }
    break;

  case 8:
    price = (abs((int)(char)global_cards_data[card_index].cc[1]) + (char)global_cards_data[card_index].cc[0] + global_cards_data[card_index].ai_base_value + 1) * 20;
    break;

  case 64:
    price = 0xfa / (global_cards_data[card_index].ai_base_value + abs((int)(char)global_cards_data[card_index].cc[1]) + 2);

    if (global_cards_data[card_index].extra_ability & 3)
    {
      // Nada
    }

    break;

  case 16:
  case 32:
    price = global_cards_data[card_index].ai_base_value * 0x14 + (abs((int)(char)global_cards_data[card_index].cc[1]) + (char)global_cards_data[card_index].cc[0] + 1) * 40;
    if ((char)global_cards_data[card_index].cc[1] == -1)
    {
      price = (price * 3) / 2;
    }
    break;
  }

  switch (GetCardRarity(card_index))
  {
  case 2:
    price = ClampIntToRange(price * 2, 100, 9999);
    break;
  case 3:
    price = ClampIntToRange(price << 2, 200, 9999);
    break;
  case 4:
    price = ClampIntToRange(price << 3, 200, 9999);
    break;
  }

  if ((global_cards_data[card_index].extra_ability & 0x200) != 0)
  {
    price = price << 1;
  }
  if ((global_cards_data[card_index].extra_ability & 0x400) != 0)
  {
    price = (price * 3) / 2;
  }

  return price;
}

// FUNCTION: SHANDALAR 0x004ef1ea
int RunTownServicesMenu(int town_index)
{
  /* Misc temps that live below the hint-pair locals in the original stack frame. */
  unsigned int tmp_mask;        // ebp - 0x94
  unsigned int save_slot_index; // ebp - 0x90
  int menu_context;             // ebp - 0x8c
  int rand_slot;                // ebp - 0x88
  int hint_pair_count;          // ebp - 0x84
  struct
  {
    int hint_pair_index;
    int slot_index;
  } hint_pairs[8]; // ebp - 0x80

  /* Additional locals used by the full town-services loop (matching stack size). */
  int retry_count;              // ebp - 0x40
  int distance;                 // ebp - 0x3c
  unsigned int tile_magic_mask; // ebp - 0x38
  int ok;                       // ebp - 0x34
  int right_click_action;       // ebp - 0x30
  int scan_color;               // ebp - 0x2c
  int slot_i;                   // ebp - 0x28
  int rand_mask;                // ebp - 0x24
  int tmp_cost;                 // ebp - 0x20
  int tmp_j;                    // ebp - 0x1c
  int slot_count;               // ebp - 0x18
  int quest_button_enabled;     // ebp - 0x14
  int tmp_i;                    // ebp - 0x10
  int card_id;                  // ebp - 0xc
  int tmp_k;                    // ebp - 0x8
  int tmp_l;                    // ebp - 0x4

  ClearInputAndWaitForMouseRelease();

  g_active_town_services_town_index = town_index;

  tile_magic_mask = GetWorldTileMagicMask(GetWorldTileType(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y));

  slot_count = g_town_slots[g_active_town_services_town_index].location_type + 3;
  if (g_world_magic_slot_timers[1].town_index == 0)
  {
    slot_count = slot_count + 1;
  }
  slot_count = MIN(slot_count, 8);

  /* Pick a "preferred" color (stored in g_town_preferred_magic_color) that the player can actually cast. */
  card_id = 0xffffffff;
  for (slot_i = 0; slot_i < 199; slot_i = slot_i + 1)
  {
    scan_color = internal_rand(5) + 1;
    if (((g_amulet_inventory[scan_color - 1] != 0) || (0x50 < slot_i)) &&
        ((tile_magic_mask & (1U << (unsigned char)scan_color)) != 0))
    {
      card_id = scan_color;
    }
  }

  g_town_preferred_magic_color = card_id;
  g_town_hint_pair_index = -1;
  g_town_shop_cache_town_index = town_index;

  /* Town quest/encounter seed setup + build shop offer cache if we haven't visited this town yet (g_last_town_services_town_index). */
  if (town_index != g_last_town_services_town_index)
  {
    /* Choose a quest/encounter target for this visit. */
    if ((internal_rand(3) != 0 || (g_town_slots[town_index].status_and_ruling_wizard & 1) != 0) &&
        g_town_slots[town_index].location_type == 1)
    {
      quest_button_enabled = 0;
      do
      {
        g_pending_quest_destination = internal_rand(0x80);
        tmp_i = ApproximateDistance(g_town_slots[town_index].world_x - g_town_slots[g_pending_quest_destination].world_x,
                                    g_town_slots[town_index].world_y - g_town_slots[g_pending_quest_destination].world_y);
        quest_button_enabled = quest_button_enabled + 1;
      } while ((quest_button_enabled < 1000) && (((g_town_slots[g_pending_quest_destination].location_type <= 1) || (g_town_slots[g_pending_quest_destination].location_type == 4)) ||
                                                 (g_town_slots[g_pending_quest_destination].location_type == 5) ||
                                                 (tmp_i < 8) ||
                                                 ((quest_button_enabled / 0x10 + 0x10) < tmp_i) ||
                                                 ((g_town_slots[g_pending_quest_destination].status_and_ruling_wizard & 0xff01U) != 0)));

      if (internal_rand(2) != 0)
      {
        g_pending_quest_type = 0;
      }
      else
      {
        g_pending_quest_type = 2;
      }

      if (quest_button_enabled >= 1000)
      {
        if ((g_town_slots[town_index].status_and_ruling_wizard & 1) != 0)
        {
          g_pending_quest_destination = -1;
        }
        else
        {
          goto choose_lair_or_monster;
        }
      }

      scan_color = internal_rand(5) + 1;
      if ((IsCardColorCompatibleWithMask(1 << (unsigned char)scan_color, 1 << (unsigned char)card_id, 3) == 0) &&
          (internal_rand(2) != 0))
      {
        g_pending_quest_type = 1;
        g_pending_quest_color = scan_color;
      }
    }
    else
    {
    choose_lair_or_monster:
      scan_color = (unsigned int)internal_rand(6);
      if (g_lair_or_monster_slots[scan_color].entry_type > SHANDALAR_ENTRY_LAIR)
      {
        g_pending_quest_destination = town_index;
        g_pending_quest_type = -g_lair_or_monster_slots[scan_color].entry_type;
      }
      else
      {
        g_pending_quest_destination = -1;
      }

      if ((internal_rand(4) == 0) || (g_pending_quest_destination == -1))
      {
        g_pending_quest_destination = town_index;
        g_pending_quest_type = -PickRandomCreatureTypeForWizardTier(card_id, (internal_rand(8) + 2) * 2);
        FreeOpeningMenuSpriteWorkEntries(0, 8);
        g_lair_or_monster_slots[0].entry_type = SHANDALAR_ENTRY_NONE;
      }
    }

    if (g_pending_quest_type != 1)
    {
      scan_color = GetWorldTileMagicMask(GetWorldTileType(g_town_slots[g_pending_quest_destination].world_x, g_town_slots[g_pending_quest_destination].world_y));
      do
      {
        g_pending_quest_color = internal_rand(5) + 1;
      } while ((scan_color & (1U << (unsigned char)g_pending_quest_color)) == 0);
    }

    if ((g_pending_quest_type == 0) && (g_amulet_inventory[g_pending_quest_color - 1] == 0))
    {
      g_pending_quest_type = 2;
    }

    if ((g_town_slots[town_index].status_and_ruling_wizard & 8))
    {
      for (slot_i = 0; slot_i < slot_count; slot_i = slot_i + 1)
      {
        if (g_quest_restock_timer - g_town_slots[town_index].card_restock_timers[slot_i] >=
            (g_shandalar_difficulty + 3) * 5)
        {
          g_town_shop_card_ids[slot_i] = g_town_slots[town_index].card_slots[slot_i];
        }
        else
        {
          g_town_shop_card_ids[slot_i] = 0xffffffff;
        }
      }
    }
    else
    {
      for (slot_i = 0; slot_i < 8; slot_i = slot_i + 1)
      {
        g_town_shop_card_ids[slot_i] = 0xffffffff;
      }
    }

    scan_color = internal_rand(slot_count);
    for (slot_i = 0; slot_i < slot_count; slot_i = slot_i + 1)
    {
      if (g_town_shop_card_ids[slot_i] != -1 ||
          (g_shandalar_difficulty + 3) * 5 >
              g_quest_restock_timer - g_town_slots[town_index].card_restock_timers[slot_i])
      {
        continue;
      }

      rand_mask = 1 << (unsigned char)internal_rand(7);
      do
      {
        if (slot_i == scan_color)
        {
          g_town_shop_card_ids[slot_i] = distance = internal_rand(5);
        }
        else
        {
          g_town_shop_card_ids[slot_i] = distance = internal_rand(g_card_count - 0x39);
        }

        ok = 0;
        for (right_click_action = 1; right_click_action < 6; right_click_action = right_click_action + 1)
        {
          if ((tile_magic_mask & (1U << (unsigned char)right_click_action)) != 0)
          {
            if (IsCardColorCompatibleWithMask(1 << (unsigned char)right_click_action, (int)(signed char)global_cards_data[distance].color,
                                              (right_click_action & 1) ? 1 : 3) != 0)
            {
              ok = 1;
            }
          }
        }

        if ((slot_i & 1) != 0)
        {
          if (((unsigned char)global_cards_data[distance].type & 0x40) != 0)
          {
            ok = 0;
          }
        }
        if ((global_cards_data[distance].extra_ability & 0x900) != 0)
        {
          ok = 0;
        }
        if (((unsigned char)global_cards_data[distance].expansion & 0xc1) == 0)
        {
          ok = 0;
        }
        if (GetCardRarity(distance) > ((slot_i % 3) + 1))
        {
          ok = 0;
        }

      } while (GetRemainingAllowedCardCopies(distance) <= 0 || ok == 0 || (global_cards_data[distance].extra_ability & 0x180) != 0);
    }

    /* Pick an optional "hint pair" tied to the available cards. */
    hint_pair_count = 0;
    for (slot_i = 0; slot_i < slot_count; slot_i = slot_i + 1)
    {
      tmp_j = FindHintPairIndexForOfferCard(g_town_shop_card_ids[slot_i]);
      if (tmp_j != -1)
      {
        hint_pairs[hint_pair_count].hint_pair_index = tmp_j;
        hint_pairs[hint_pair_count].slot_index = slot_i;
        hint_pair_count = hint_pair_count + 1;
      }
    }
    if (hint_pair_count != 0)
    {
      rand_slot = internal_rand(hint_pair_count);
      g_town_hint_pair_index = hint_pairs[rand_slot].hint_pair_index;
      g_town_hint_offer_slot_index = hint_pairs[rand_slot].slot_index;
    }

    /* Price each offer. */
    for (slot_i = 0; slot_i < 8; slot_i = slot_i + 1)
    {
      distance = g_town_shop_card_ids[slot_i];
      if (distance == -1)
      {
        continue;
      }

      tmp_cost = CalculateCardShopPrice(distance);
      tmp_cost = (g_town_slots[town_index].location_type + 2) * tmp_cost;

      if ((tile_magic_mask & (unsigned int)(signed char)global_cards_data[distance].color) == 0)
      {
        if (((signed char)global_cards_data[distance].color != 0) &&
            (IsCardColorCompatibleWithMask((int)tile_magic_mask, (int)(signed char)global_cards_data[distance].color, 3) != 0))
        {
          tmp_cost = (tmp_cost * 3) / 2;
        }
        else
        {
          tmp_cost = tmp_cost << 1;
        }
      }

      g_town_shop_card_prices[slot_i] = ClampIntToRange(((tmp_cost / 0x32) * 5), 5, 1000);
    }

    g_town_slots[town_index].status_and_ruling_wizard |= 8;
  }

  /* Food cost(?) depends on location_type. */
  g_town_food_price = tmp_l = (g_town_slots[town_index].location_type + 2) * 5;

  sound_unload(0x12);
  (void)LoadSoundWithDriveFallback("x:sound\\button.wav", 0x12, 0);

loop:
  if (slot_count == 0)
  {
    g_town_shop_card_ids[slot_count] = single_color_test_bit_to_color_t((int)tile_magic_mask) - 1;
    g_town_shop_card_prices[slot_count] = 0x28;
    slot_count = slot_count + 1;
  }

  g_page0_window_bounds->font_slot = 4;
  strcpy(g_ui_message_buffer, BuildTownDisplayName(town_index));
  DrawUiScaledCenteredText(g_ui_message_buffer, 0xa0, 0x1c,
                           (g_town_slots[town_index].location_type == 1 ? 0xfe : 0xe0));
  g_page0_window_bounds->font_slot = 1;

  /* Fill sprite pointers (the render callback expects these). */
  for (slot_i = 0; slot_i < 4; slot_i = slot_i + 1)
  {
    g_town_main_menu_controls[slot_i].mode_data[0] = (int)g_main_menu_button_sprites_normal[slot_i];
    g_town_main_menu_controls[slot_i].mode_data[1] = (int)g_main_menu_button_sprites_highlight[slot_i];
    g_town_main_menu_controls[slot_i].mode_data[2] = (int)g_main_menu_button_sprites_highlight[slot_i];
    g_town_main_menu_controls[slot_i].mode_data[3] = (int)g_main_menu_button_sprites_normal[slot_i];
  }

  if (g_town_icon_menu_controls[0].x == g_town_icon_menu_controls[0].base_x)
  {
    for (slot_i = 0; slot_i < 4; slot_i = slot_i + 1)
    {
      g_town_main_menu_controls[slot_i].x = ScaleUiCoordinate(g_town_main_menu_controls[slot_i].base_x);
      g_town_main_menu_controls[slot_i].y = ScaleUiCoordinate(g_town_main_menu_controls[slot_i].base_y);
      g_town_main_menu_controls[slot_i].width = ScaleUiCoordinate(g_town_main_menu_controls[slot_i].base_width);
      g_town_main_menu_controls[slot_i].height = ScaleUiCoordinate(g_town_main_menu_controls[slot_i].base_height);
    }

    for (slot_i = 0; slot_i < 5; slot_i = slot_i + 1)
    {
      g_town_icon_menu_controls[slot_i].x = ScaleUiCoordinate(g_town_icon_menu_controls[slot_i].base_x);
      g_town_icon_menu_controls[slot_i].y = ScaleUiCoordinate(g_town_icon_menu_controls[slot_i].base_y);
      g_town_icon_menu_controls[slot_i].width = ScaleUiCoordinate(g_town_icon_menu_controls[slot_i].base_width);
      g_town_icon_menu_controls[slot_i].height = ScaleUiCoordinate(g_town_icon_menu_controls[slot_i].base_height);
    }
  }

  retry_count = BeginMenuContext();
  (void)ResetMenuContext(retry_count);
  (void)AddMenuControlsToContext(g_town_main_menu_controls, 4, retry_count);
  (void)AddMenuControlsToContext(g_town_icon_menu_controls, 5, retry_count);

  /* Build button labels. */
  strcpy(g_town_button_labels[0], gs_cityscreen_buttons_0077f5e0[0]);
  sprintf(g_town_button_labels[1], gs_cityscreen_buttons_0077f5e0[1], g_town_food_price);
  strcpy(g_town_button_labels[2], gs_cityscreen_buttons_0077f5e0[2]);
  if (g_town_hint_pair_index != -1)
  {
    strcpy(g_town_button_labels[3], gs_cityscreen_buttons_0077f5e0[3]);
  }
  else
  {
    strcpy(g_town_button_labels[3], gs_cityscreen_buttons_0077f5e0[4]);
  }
  g_town_button_labels[4][0] = '\0';
  tmp_k = 0;
  g_town_icon_menu_controls[4].on_activate = (AdvMenuActivateCallback)VisitTownWiseman;

  if (g_town_slots[town_index].location_type != 1)
  {
    if (((card_id != -1) && (g_amulet_inventory[card_id - 1] != 0)) && (g_town_slots[town_index].trade_color_and_type != 0) &&
        ((g_quest_restock_timer - g_town_slots[town_index].special_card_restock_timer) > ((g_shandalar_difficulty + 3) * 2 * 9)))
    {
      strcpy(g_ui_message_buffer, "");
      FormatMessageFromStringStripCarriageReturns(g_town_button_labels[4], 0x64, gs_cityscreen_buttons_0077f5e0[5], gs_amuletnames_0077d090[card_id],
                                                  GetTownCardDescription(town_index));
      g_town_icon_menu_controls[4].on_activate = (AdvMenuActivateCallback)BuyTownSpecialCardOffer;
    }
    else if (((g_current_quest_destination == -1) && (g_pending_quest_destination != -1)) &&
             (((g_shandalar_difficulty + 3) * 0x10) <
              (g_quest_restock_timer - g_town_slots[town_index].quest_restock_timer)))
    {
      strcpy(g_town_button_labels[4], gs_cityscreen_buttons_0077f5e0[6]);
      tmp_k = 1;
    }
    else
    {
      strcpy(g_town_button_labels[4], gs_cityscreen_buttons_0077f5e0[7]);
    }
  }
  else
  {
    if (((g_current_quest_destination == -1) && (g_pending_quest_destination != -1)) &&
        (((g_shandalar_difficulty + 3) * 0x10) <
         (g_quest_restock_timer - g_town_slots[town_index].quest_restock_timer)))
    {
      strcpy(g_town_button_labels[4], gs_cityscreen_buttons_0077f5e0[6]);
    }
    else
    {
      strcpy(g_town_button_labels[4], gs_cityscreen_buttons_0077f5e0[7]);
    }
  }

  if (g_town_button_labels[4][0])
  {
    g_town_icon_menu_controls[4].state = 1;
  }
  else
  {
    g_town_icon_menu_controls[4].state = 3;
  }

  g_town_menu_show_tooltips = 1;
  (void)RenderCurrentMenuContextControls();
  g_town_menu_show_tooltips = 0;

  g_town_menu_result_state = 0;

  while (g_town_menu_result_state == 0)
  {
    UpdateMouseSnapshot();
    (void)UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
    if (((g_mouse_button_down_mask & 2U) != 0) ||
        (((g_mouse_button_down_mask & 1U) != 0) &&
         (IsAdventureRightClickMenuButtonHit(g_mouse_x_snapshot, g_mouse_y_snapshot) != 0)))
    {
      menu_context = RunRightClickMenuAndQueueInput();

      switch (menu_context)
      {
      case 4:
        AnimatePaletteToColor(0, g_default_palette_fade_steps);
        DeckBuilderMain(g_main_window_hwnd, 1, 3);
        (void)RestoreAdventureUiPaletteAndFocus();
        RefreshAdventureInterfaceLayout();
        break;
      case 5:
        ClearInputAndWaitForMouseRelease();
        ShowWorldMapScreen(0);
        RefreshAdventureInterfaceLayout();
        break;
      case 6:
        ClearInputAndWaitForMouseRelease();
        ShowCityInfoScreen(1);
        RefreshAdventureInterfaceLayout();
        break;
      case 7:
        ClearInputAndWaitForMouseRelease();
        ShowDungeonCluesScreen(1);
        RefreshAdventureInterfaceLayout();
        break;
      case 8:
        RunAdventureStatsMenu();
        RefreshAdventureInterfaceLayout();
        break;
      case 9:
        ClearInputAndWaitForMouseRelease();
        ShowStatsWindow(0, -1);
        RefreshAdventureInterfaceLayout();
        break;
      case 2:
        g_loadsave_skip_esc = 1;
        save_slot_index = RunLoadGameMenu();
        if (save_slot_index != -1)
        {
          (void)load_selected_duel_save_slot(save_slot_index);
        }
        LoadPcxIntoPageNoPalette("advfac64.pic");
        RefreshAdventureInterfaceLayout();
        (void)DrawAdventureInterfaceLayout(1);
        g_loadsave_skip_esc = 0;
        if (save_slot_index != -1)
        {
          (void)EndMenuContext();
          longjmp(g_adventure_session_restart_jump_buffer, 0);
        }
        break;
      case 1:
        tmp_mask = (unsigned int)RunSaveMenuAndSelectSlot();
        if ((int)tmp_mask != -1)
        {
          SaveGameToSlot((int)tmp_mask);
        }
        LoadPcxIntoPageNoPalette("advfac64.pic");
        RefreshAdventureInterfaceLayout();
        break;
      case 3:
        g_page0_window_bounds->font_slot = 4;
        (void)LoadTextSectionLines("ADVstrings.txt", "SHUTDOWN");
        strcpy(g_ui_message_buffer, g_text_lines[0]);
        if (RunTextMenuAtScaled(g_ui_message_buffer, 100, 0x50) == 1)
        {
          g_adventure_world_exit_requested = 1;
        }
        else
        {
          RefreshAdventureInterfaceLayout();
        }
        SaveGameToSlot(3);
        if (g_adventure_world_exit_requested != 0)
        {
          (void)EndMenuContext();
          longjmp(g_adventure_world_exit_jump_buffer, 0);
        }
        break;
      }

      (void)DrawAdventureInterfaceLayout(1);
      (void)EndMenuContext();
      AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
      // This can't be nested loop because the "inner" loop overlaps the "outer"
      goto loop;
    }
  }

  (void)EndMenuContext();
  if (g_town_menu_result_state == -2)
  {
    goto loop;
  }

  /* Persist cached offers back into the town slot. */
  for (slot_i = 0; slot_i < 8; slot_i = slot_i + 1)
  {
    g_town_slots[town_index].card_slots[slot_i] = g_town_shop_card_ids[slot_i];
  }
  g_last_town_services_town_index = town_index;
  (void)sound_stop(0xf);
  sound_unload(0xf);
  return 0;
}

// FUNCTION: SHANDALAR 0x00568320
int RunDuelEngine(unsigned int card_id, int creature_type)
{
  HDC desktop_dc;         /* ebp - 0x1c */
  int was_topmost;        /* ebp - 0x18 */
  int deck_slot;          /* ebp - 0x0c */
  DWORD thread_exit_code; /* ebp - 0x04 */

  (void)card_id;

  // TODO: Remove. We win every duel currently
  return 1;

  DAT_00742fc0 = 1;
  DestroyCachedCardArt();

  thread_exit_code = 2;

  if (thread_exit_code == 2)
  {
    if ((GetWindowLong(g_main_window_hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0)
    {
      was_topmost = 1;
    }
    else
    {
      was_topmost = 0;
    }

    if (was_topmost != 0)
    {
      SetWindowPos(g_main_window_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    thread_exit_code = RunDuelEngineThreadProc((LPVOID)creature_type);

    if (was_topmost != 0)
    {
      SetWindowPos(g_main_window_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    if (g_one_deck_mode == 0)
    {
      for (deck_slot = 0; deck_slot < 500; deck_slot = deck_slot + 1)
      {
        if (deck[deck_slot] != -1)
        {
          deck[deck_slot] &= -32769;
        }
      }
    }
    else
    {
      OutputDebugString("OneDeck ONEDECK ONE DECK\n");
    }

    g_next_duel_card_id = -1;
    g_opponent_starting_card_id_1 = g_next_duel_card_id;
    g_opponent_starting_card_id_2 = g_opponent_starting_card_id_1;
    g_duel_ai_mode_state = 0;
    for (deck_slot = 0; deck_slot < 4; deck_slot = deck_slot + 1)
    {
      g_ai_combat_value_weights[deck_slot] = 8;
    }

    if (g_restore_deck_after_duel != 0)
    {
      memcpy(deck, g_deck_restore_buffer, 2000);
    }

    g_duel_active = 0;
    DAT_00742fc0 = 1;
    g_attacking_creature_count = 0;
    g_duel_selection_pending = g_attacking_creature_count;
    g_duel_extra_turn_player = -1;
    g_current_phase = 0;

    ShowWindow(g_main_window_hwnd, SW_SHOW);
    SetForegroundWindow(g_main_window_hwnd);
    BringWindowToTop(g_main_window_hwnd);
    SetFocus(g_main_window_hwnd);

    desktop_dc = GetDC((HWND)0);
    SelectPalette(desktop_dc, g_palette_handle, 0);
    RealizePalette(desktop_dc);
    ReleaseDC((HWND)0, desktop_dc);

    LoadPcxIntoPageNoPalette("advfac64.pic");
    ReadPalette("todpal.tr", (char *)0);
    SelectPalette(g_graphics_pages[0]->hTempDC, g_palette_handle, 0);
    RealizePalette(g_graphics_pages[0]->hTempDC);
    PreloadWorldAmbientSounds();
    g_next_duel_card_id = -1;
    g_next_duel_life_delta = 0;
    return g_duel_exit_code;
  }
  else
  {
    g_next_duel_card_id = -1;
    g_next_duel_life_delta = 0;
    return thread_exit_code;
  }

  return -1;
}

// FUNCTION: SHANDALAR 0x005636ab
int ExitIfNoUsableDeckCards(void)
{
  int deck_slot;
  int usable_card_count;
  int font_size;

  usable_card_count = 0;
  for (deck_slot = 0; deck_slot < 500; deck_slot = deck_slot + 1)
  {
    if ((deck[deck_slot] != -1) && ((deck[deck_slot] & 0x4000) == 0) && (4 < (deck[deck_slot] & 0xfff)))
    {
      usable_card_count = usable_card_count + 1;
    }
  }

  if (usable_card_count == 0)
  {
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    LoadPcxIntoPageOpaque(1, "uth-arz.pic");
    strcpy(g_ui_message_buffer, "");
    g_page1_window_bounds->font_slot = 5;
    if ((global_screen_width == 0x280) || (global_screen_width == 0x320))
    {
      font_size = 0x14;
    }
    else
    {
      font_size = 0x10;
    }
    SetFontStyleSize(5, (unsigned int)ScaleUiCoordinate(font_size));

    strcpy(g_ui_message_buffer, gs_questfailed_0077c580[0]);
    strcat(g_ui_message_buffer, gs_questfailed_0077c580[1]);
    strcat(g_ui_message_buffer, gs_questfailed_0077c580[4]);
    strcat(g_ui_message_buffer, gs_questfailed_0077c580[5]);

    DrawFormattedTextShadowedCentered(g_page1_window_bounds, 0xea, 0x140, 0x1e0 - GetFontLineHeight(g_page1_window_bounds->font_slot) * 7, g_ui_message_buffer);

    StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    g_adventure_world_exit_requested = 1;
    UnloadStatWinDllExports();
    ShutdownSharedStartup();
    exit(0);
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x0056139e
ShandalarEntryType PickRandomCreatureTypeForWizardTier(int wizard_color, int creature_tier)
{
  int creature_type;
  int tries;

  tries = 0;
  do
  {
    creature_type = internal_rand((int)gs_creature_name_count_00593934 - 1) + 1;
    tries++;
  } while (tries < 999 && (g_shandalar_monster_definitions[creature_type].tier != creature_tier ||
                           (wizard_color != 0 && ((1 << (unsigned char)wizard_color) & (int)(signed char)g_shandalar_monster_definitions[creature_type].color_mask) == 0)));

  return (tries == 999) ? SHANDALAR_ENTRY_LAIR : (ShandalarEntryType)creature_type;
}

// FUNCTION: SHANDALAR 0x0050a5f1
void LoadCreatureDuelDeck(int creature_type, unsigned int name_id, unsigned int color_filter, int speed_filter)
{
  (void)name_id;

  strcpy(g_ui_message_buffer, "decks\\0");
  if (g_shandalar_monster_definitions[creature_type].deck_number < 100)
  {
    strcat(g_ui_message_buffer, "0");
    if (g_shandalar_monster_definitions[creature_type].deck_number < 10)
    {
      strcat(g_ui_message_buffer, "0");
    }
  }

  strcat(g_ui_message_buffer,
         _itoa(g_shandalar_monster_definitions[creature_type].deck_number, g_itoa_buffer, 10));
  strcat(g_ui_message_buffer, ".dck");

  if (FileExists(g_ui_message_buffer) != 0)
  {
    ClearAndLoadInitialLibraryFromDeckFile(g_ui_message_buffer, 1, color_filter, speed_filter);
    g_opponent_initial_library_index = 1;
  }
  else
  {
    ClearAndLoadInitialLibraryFromDeckFile("decks\\0179.dck", 1, color_filter, speed_filter);
    g_opponent_initial_library_index = 1;
  }
}

// FUNCTION: SHANDALAR 0x005222ae
void ClearAndLoadInitialLibraryFromDeckFile(char *deck_path, int library_index, unsigned int color_filter, int speed_filter)
{
  int i;

  for (i = 0; i < 200; i = i + 1)
  {
    g_initial_library[library_index][i].numcards = 0;
    g_initial_library[library_index][i].csvid = g_initial_library[library_index][i].numcards;
  }
  ParseDeckFileIntoInitialLibrary(deck_path, g_initial_library[library_index], color_filter, speed_filter);

  return;
}

// FUNCTION: SHANDALAR 0x00417e11
int ParseDeckFileIntoInitialLibrary(char *deck_path, csvid_and_numcards *library_entries, unsigned int color_filter, int speed_filter)
{
  unsigned int entry_index;            /* ebp-0x22c */
  unsigned int last_basic_entry_index; /* ebp-0x228 */
  int unused_224;                      /* ebp-0x224 */
  int card_index;                      /* ebp-0x220 */
  int csvid;                           /* ebp-0x21c */
  int numcards;                        /* ebp-0x218 */
  int header_line_index;               /* ebp-0x214 */
  char *space;                         /* ebp-0x210 */
  int total_cards;                     /* ebp-0x20c */
  int current_speed_filter;            /* ebp-0x208 */
  unsigned int current_color_filter;   /* ebp-0x204 */
  char line[500];                      /* ebp-0x200 */
  int deck_type;                       /* ebp-0xc */
  FILE *f;                             /* ebp-0x8 */
  int scan_ret;                        /* ebp-0x4 */

  strcpy(g_last_parsed_deck_path, deck_path);
  f = fopen(deck_path, "rt\0");
  if (f == (FILE *)0)
  {
    return 0;
  }

  deck_type = 0;
  header_line_index = deck_type;
  entry_index = (unsigned int)header_line_index;

  scan_ret = fscanf(f, "%[^\n]", line);
  scan_ret = fscanf(f, "%[\n]", line);

  current_color_filter = 0;
  current_speed_filter = -1;
  last_basic_entry_index = 0xffffffff;
  unused_224 = 0;
  total_cards = 0;
  numcards = -1;
  csvid = numcards;

  do
  {
    scan_ret = fscanf(f, "%[^\n]\0\0", line);
    if (line[0] == '.')
    {
      if (line[1] != 'v')
      {
        sscanf(line + 1, "%d %d\0\0", &csvid, &numcards);
        total_cards = total_cards + numcards;

        if (((current_color_filter == 0) || ((color_filter & current_color_filter) != 0)) && ((current_speed_filter == -1) || (current_speed_filter == speed_filter)))
        {
          library_entries[entry_index].csvid = csvid;
          library_entries[entry_index].numcards = numcards;

          card_index = FindCardIndexByCsvid(csvid);
          if (numcards == 0)
          {
            numcards = g_shandalar_difficulty;
          }

          if (card_index != -1)
          {
            if (card_index <= 4)
            {
              last_basic_entry_index = entry_index;
            }

            if (((unsigned char)global_cards_data[card_index].type & 1) != 0)
            {
              unused_224 = unused_224 + (int)entry_index;
            }
          }
          else
          {
            if ((last_basic_entry_index != 0xffffffff) &&
                (library_entries[last_basic_entry_index].numcards > numcards))
            {
              library_entries[last_basic_entry_index].numcards =
                  library_entries[last_basic_entry_index].numcards - (int)((entry_index & 1) + numcards) / 2;
            }
          }
        }
      }
      else
      {
        space = strchr(line, ' ');
        if (space != (char *)0)
        {
          *space = '\0';
        }

        if (_strcmpi(line, ".vNONE\0") == 0)
        {
          current_color_filter = 1;
        }
        if (_strcmpi(line, ".vBLACK") == 0)
        {
          current_color_filter = 2;
        }
        if (_strcmpi(line, ".vBLUE\0") == 0)
        {
          current_color_filter = 4;
        }
        if (_strcmpi(line, ".vRED\0\0") == 0)
        {
          current_color_filter = 0x10;
        }
        if (_strcmpi(line, ".vGREEN") == 0)
        {
          current_color_filter = 8;
        }
        if (_strcmpi(line, ".vWHITE") == 0)
        {
          current_color_filter = 0x20;
        }

        if (_strcmpi(line, ".vFAST\0") == 0)
        {
          current_speed_filter = 0;
        }
        if (_strcmpi(line, ".vLARGE") == 0)
        {
          current_speed_filter = 1;
        }
        if (_strcmpi(line, ".vDIRECT\0\0\0") == 0)
        {
          current_speed_filter = 2;
        }
        if (_strcmpi(line, ".vARTIFACT\0") == 0)
        {
          current_speed_filter = 6;
        }
      }
      entry_index = entry_index + 1;
    }
    else
    {
      header_line_index = header_line_index + 1;
      if (header_line_index == 5)
      {
        deck_type = atoi(line + 1);
      }
      else if ((header_line_index == 6) && (strcmp(line + 1, "4th Edition") != 0))
      {
        deck_type = -2;
      }
    }
    scan_ret = fscanf(f, "%[\n]\0\0\0", line);
  } while ((((int)entry_index < 200) && (total_cards < 500) && (scan_ret != -1)) &&
           ((csvid != 0) || (numcards != 0)));

  fclose(f);

  if (total_cards < 0x28)
  {
    deck_type = -3;
  }
  if (((total_cards >= 500) || ((int)entry_index >= 200)) && (scan_ret != -1))
  {
    deck_type = -4;
  }
  if (0x37 < deck_type)
  {
    deck_type = -1;
  }

  return deck_type;
}

// FUNCTION: SHANDALAR 0x004f6886
void DrawCreaturePortrait(int creature_type, int x_320, int y_200, int tinted, int mode)
{
  DIBSurface *dst_page;                /* ebp-0x30 */
  DIBSurface *src_page;                /* ebp-0x2c */
  int dst_right;                       /* ebp-0x28 */
  int dst_bottom;                      /* ebp-0x24 */
  EncodedImage *face_sprite;           /* ebp-0x20 */
  char face_path[24];                  /* ebp-0x1c */
  EncodedImage *portrait_frame_sprite; /* ebp-0x4 */

  LoadPcxIntoPage(1, (tinted != 0) ? "prdfrmc.pic" : "prdfrmb.pic");
  g_page1_window_bounds->font_slot = 4;
  g_page0_window_bounds->font_slot = 4;

  if (tinted != 0)
  {
    ApplyPortraitPaletteMap(g_page1_window_bounds, 1, 0x15a, 0xb3, 0x23, "prdblk.pic", "advfac64.pic");
  }

  BeginSpriteEncodeSession();
  portrait_frame_sprite = EncodeSpriteFromPage(1, 1, 0x15a, 0xb3, 0x23);
  FinalizeSpriteEncodeSession();

  dst_right = global_screen_width - ScaleUiCoordinate(0xb4);
  dst_bottom = global_screen_height - ScaleUiCoordinate(0xb4);

  if (mode == 1)
  {
    g_page0_window_bounds->page_number = 1;
  }

  DrawEncodedImageResampled(g_page0_window_bounds, x_320 - ScaleUiCoordinate((int)portrait_frame_sprite->width / 2), y_200,
                            ScaleUiCoordinate((int)portrait_frame_sprite->width),
                            ScaleUiCoordinate((int)portrait_frame_sprite->height), portrait_frame_sprite);

  strcpy(g_ui_message_buffer, GetCreatureName(creature_type));
  DrawFormattedTextNoShadowCentered(g_page0_window_bounds, (tinted == 0) ? 0xe6 : 0xae, x_320,
                                    y_200 + ScaleUiCoordinate((int)portrait_frame_sprite->height / 2), g_ui_message_buffer);

  g_page0_window_bounds->font_slot = 1;
  if (mode == 1)
  {
    g_page0_window_bounds->page_number = 0;
  }

  StretchBlitGraphicsRect(g_page1_window_bounds, 1, 0xa5, 0xb4, 0xb4, g_page1_window_bounds, dst_right, dst_bottom, ScaleUiCoordinate(0xb4),
                          ScaleUiCoordinate(0xb4));

  sprintf(face_path, "faces\\%03d.pic", creature_type);
  LoadPcxResource(1, 0, global_screen_height - 0xf0, face_path, (void *)0);
  BeginSpriteEncodeSession();
  face_sprite = EncodeSpriteFromPage(1, 0, global_screen_height - 0xf0, 0x8a, 0xaa);
  FinalizeSpriteEncodeSession();

  DrawEncodedImageResampled(g_page1_window_bounds, dst_right + ScaleUiCoordinate(0x15), dst_bottom + ScaleUiCoordinate(5), ScaleUiCoordinate(0x8a),
                            ScaleUiCoordinate(0xaa), face_sprite);

  if (tinted != 0)
  {
    ApplyPortraitPaletteMap(g_page1_window_bounds, dst_right, dst_bottom, (unsigned int)ScaleUiCoordinate(0xb4), ScaleUiCoordinate(0xb4), "prdblk.pic",
                            "advfac64.pic");
  }

  if (mode == 0)
  {
    BlitGraphicsRect(g_page1_window_bounds, dst_right, dst_bottom, ScaleUiCoordinate(0xb4), ScaleUiCoordinate(0xb4), g_page0_window_bounds,
                     x_320 - ScaleUiCoordinate(0x5a), y_200 + ScaleUiCoordinate(0x2d));
  }
  else
  {
    src_page = g_graphics_pages[g_page1_window_bounds->page_number];
    dst_page = g_graphics_pages[g_page0_window_bounds->page_number];

    BlitGraphicsRect(g_page1_window_bounds, dst_right, dst_bottom, ScaleUiCoordinate(0xb4), ScaleUiCoordinate(0xb4), g_page1_window_bounds,
                     x_320 - ScaleUiCoordinate(0x5a), y_200 + ScaleUiCoordinate(0x2d));
    BlitRectByRandomTileOrderInPlace(dst_page->hTempDC, x_320 - ScaleUiCoordinate(0x5a), y_200 + ScaleUiCoordinate(0x2d), ScaleUiCoordinate(0xb4),
                                     ScaleUiCoordinate(0xb4), 3, 3, src_page->hTempDC);

    if (mode == 1)
    {
      BlitRectByRandomTileOrderInPlace(dst_page->hTempDC,
                                       x_320 - ScaleUiCoordinate((int)portrait_frame_sprite->width / 2 + -0xb), y_200,
                                       ScaleUiCoordinate(portrait_frame_sprite->width + -0x16),
                                       ScaleUiCoordinate((int)portrait_frame_sprite->height), 2, 2, src_page->hTempDC);
    }
  }

  FreeSpriteBlob(face_sprite);
  FreeSpriteBlob(portrait_frame_sprite);
}

// FUNCTION: SHANDALAR 0x005629c3
void PlayDuelMusic(int tune_index)
{
  if (g_world_location_music_track_id != -1)
  {
    sound_unload(0x10);
  }
  g_world_location_music_track_id = 0x15;
  g_world_location_music_town_index = -1;
  LoadLoopingSound(PTR_s_x_sound_dueltune_wav_00591278[tune_index], 0x10);
  PlaySoundWithPan(0x10, 0x80, 0);
  g_world_location_music_active = 1;
}

// FUNCTION: SHANDALAR 0x004f6d32
void RemoveCardFromDeckById(unsigned int card_id)
{
  int deck_slot;

  for (deck_slot = 0; deck_slot < 0x1f4; deck_slot = deck_slot + 1)
  {
    if (((unsigned int)deck[deck_slot] & 0xfff) == card_id)
    {
      RemoveDeckSlot(deck_slot);
      break;
    }
  }
}

// FUNCTION: SHANDALAR 0x005224a0
void ShowPlayer1LibraryMenu(int unused)
{
  (void)unused;
  SelectAdventureListCardIndex(g_active_player, global_library[1], 500, gs_showlibrary_text_0074bcc0.accept_keys, 0, &g_showlibrary_menu_selection);
}

// FUNCTION: SHANDALAR 0x0053114a
void WisemanChooseLairCreatureAndSetupDuel(unsigned char amulet_color)
{
  int aiStack_50[16];
  int chosen_creature_type;
  int i;
  int count;

  /* The original overwrites the param stack slot with a precomputed bitmask. */
  *(int *)&amulet_color = 1 << amulet_color;

  strcpy(g_ui_message_buffer, gs_lair_0077e180[0x11]);
  count = 0;

  for (i = 1; i < (int)gs_creature_name_count_00593934; i = i + 1)
  {
    if ((*(int *)&amulet_color & (int)(signed char)g_shandalar_monster_definitions[i].plural[0x36]) != 0)
    {
      aiStack_50[count] = i;
      count = count + 1;
      strcat(g_ui_message_buffer, " ");
      strcat(g_ui_message_buffer, GetCreatureName(i));
      strcat(g_ui_message_buffer, ".\n");
    }
  }

  chosen_creature_type = RunTextMenuAt(g_ui_message_buffer, 100, 0x46);
  if (chosen_creature_type != -1)
  {
    chosen_creature_type = aiStack_50[chosen_creature_type];
    LoadCreatureDuelDeck(chosen_creature_type, -1, 0, -1);
    for (i = 0; i < 500; i = i + 1)
    {
      global_library[1][i] = DrawRandomCardFromInitialLibrary(g_opponent_initial_library_index);
    }
    ShowPlayer1LibraryMenu(g_opponent_initial_library_index);
  }
}

// FUNCTION: SHANDALAR 0x0053128c
int CountTextNewlines(char *text)
{
  char *saved_text;
  int lines;

  lines = 0;

  while ((int)(signed char)*text != 0)
  {
    saved_text = text;
    text = text + 1;
    if ((int)(signed char)*saved_text == 10)
    {
      lines = lines + 1;
    }
  }

  return lines;
}

// FUNCTION: SHANDALAR 0x005312d7
int SplitTextOnFourNewlines(char *text_block, char **out_line_ptrs)
{
  int count;

  count = 1;
  out_line_ptrs[0] = text_block;

  while (*text_block != '\0')
  {
    if (*(unsigned int *)text_block == 0x0A0A0A0AU)
    {
      out_line_ptrs[count] = text_block + 4;
      count = count + 1;
    }
    text_block = text_block + 1;
  }

  out_line_ptrs[count] = text_block;
  return count;
}

// FUNCTION: SHANDALAR 0x0053133f
int PickUndefeatedWizardColor(void)
{
  int selection;       /* ebp-0xc */
  int wizard_index;    /* ebp-0x8 */
  int available_count; /* ebp-0x4 */

  available_count = 0;

  for (wizard_index = 1; wizard_index <= 5; wizard_index = wizard_index + 1)
  {
    if ((g_defeated_wizards_bitmap & (1U << (unsigned char)wizard_index)) == 0)
    {
      available_count = available_count + 1;
    }
  }

  selection = internal_rand(available_count);

  for (wizard_index = 1; (wizard_index <= 5) && (selection != 0); wizard_index = wizard_index + 1)
  {
    if ((g_defeated_wizards_bitmap & (1U << (unsigned char)wizard_index)) == 0)
    {
      selection = selection - 1;
    }
  }

  return wizard_index;
}

// FUNCTION: SHANDALAR 0x005313e3
int ComputeWisemanCityBlockValue(int color_index)
{
  int score;            /* ebp-0x1c */
  int color_bit;        /* ebp-0x18 */
  int duel_win_count;   /* ebp-0x14 */
  int tmp;              /* ebp-0x10: param copy / min clamp */
  int duel_log_index;   /* ebp-0x0c */
  int creature_group;   /* ebp-0x08 */
  int town_match_count; /* ebp-0x04 */

  creature_group = color_index * 7;
  color_bit = 1 << (unsigned char)color_index;
  score = (int)(signed char)g_shandalar_monster_definitions[creature_group].tier;

  if (creature_group <= 0x24)
  {
    if ((creature_group % 7) != 0)
    {
      score = score + g_shandalar_difficulty * 2;
      goto after_difficulty_adjust;
    }
  }

  if (creature_group <= 0x24)
  {
    if ((creature_group % 7) == 0)
    {
      score = score + g_shandalar_difficulty * 5;
      goto after_difficulty_adjust;
    }
  }

  if (creature_group < 0x37)
  {
    score = score + g_shandalar_difficulty * 2;
    goto after_difficulty_adjust;
  }

  if (creature_group >= 0x37)
  {
    score = score + g_shandalar_difficulty * 50;
  }

after_difficulty_adjust:

  if ((int)(signed char)g_shandalar_monster_definitions[creature_group].encounter_type == 0xb)
  {
    for (duel_log_index = 0; duel_log_index < 10; duel_log_index = duel_log_index + 1)
    {
      if ((g_world_magic_bitmap & (1 << (unsigned char)duel_log_index)) != 0)
      {
        score = score + 1;
      }
    }
  }

  if ((int)(signed char)g_shandalar_monster_definitions[creature_group].encounter_type == 0xc)
  {
    score = score + 10;
    duel_win_count = 0;
    town_match_count = 0;

    for (duel_log_index = 0; duel_log_index < 1000; duel_log_index = duel_log_index + 1)
    {
      if ((int)(signed char)g_duel_victory_log[duel_log_index] == 0)
      {
        break;
      }
      if (((int)(signed char)g_duel_victory_log[duel_log_index] >> 4) == color_bit)
      {
        duel_win_count = duel_win_count + 1;
      }
    }

    score = score - duel_win_count;

    for (tmp = 0; tmp < 0x80; tmp = tmp + 1)
    {
      if (g_town_slots[tmp].status_and_ruling_wizard & 0xff00)
      {
        if (((g_town_slots[tmp].status_and_ruling_wizard >> 8) - 1) == duel_log_index)
        {
          town_match_count = town_match_count + 1;
        }
      }
    }

    score = score + town_match_count * g_shandalar_difficulty;

    score = MAX(score, g_shandalar_difficulty * 5 + 0x14);
  }

  return score;
}

// FUNCTION: SHANDALAR 0x00531616
int CountTownsRuledByWizardColor(int wizard_color)
{
  int town_index;       /* ebp-0x8 */
  int ruled_town_count; /* ebp-0x4 */

  ruled_town_count = 0;
  wizard_color = 1 << (unsigned char)wizard_color;

  for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
  {
    if (g_town_slots[town_index].status_and_ruling_wizard & 0xff00)
    {
      if (((g_town_slots[town_index].status_and_ruling_wizard >> 8) - 1) == wizard_color)
      {
        ruled_town_count = ruled_town_count + 1;
      }
    }
  }

  return ruled_town_count;
}

// FUNCTION: SHANDALAR 0x00531698
int FillWisemanCityBlockTownNameSubstitutions(char *out_a, char *out_b, int choose_new_towns)
{
  int town_index;
  int active_town_index;

  if (choose_new_towns != 0)
  {
    g_wiseman_city_block_active_town_count = 0;
    for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
    {
      if ((g_town_slots[town_index].status_and_ruling_wizard & 1) != 0)
      {
        g_wiseman_city_block_active_town_count = g_wiseman_city_block_active_town_count + 1;
      }
    }

    if (g_wiseman_city_block_active_town_count >= 2)
    {
      g_wiseman_city_block_town_choice_index_a = internal_rand(g_wiseman_city_block_active_town_count);
      g_wiseman_city_block_town_choice_index_b = internal_rand(g_wiseman_city_block_active_town_count);
      while (g_wiseman_city_block_town_choice_index_b == g_wiseman_city_block_town_choice_index_a)
      {
        g_wiseman_city_block_town_choice_index_b = internal_rand(g_wiseman_city_block_active_town_count);
      }
    }
    else
    {
      return -1;
    }
  }

  if (g_wiseman_city_block_active_town_count >= 2)
  {
    active_town_index = 0;
    for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
    {
      if ((g_town_slots[town_index].status_and_ruling_wizard & 1) != 0)
      {
        if (active_town_index == g_wiseman_city_block_town_choice_index_a)
        {
          strcpy(out_a, BuildTownDisplayName(town_index));
          active_town_index = active_town_index + 1;
        }
        else if (active_town_index == g_wiseman_city_block_town_choice_index_b)
        {
          strcpy(out_b, BuildTownDisplayName(town_index));
          active_town_index = active_town_index + 1;
        }
        else
        {
          active_town_index = active_town_index + 1;
        }
      }
    }
    return 2;
  }
  else
  {
    return -1;
  }
}

// FUNCTION: SHANDALAR 0x0053182b
int PickOpenWorldMagicSlot(void)
{
  int selection;       /* ebp-0xc */
  int slot_index;      /* ebp-0x8 */
  int open_slot_count; /* ebp-0x4 */

  open_slot_count = 0;
  for (slot_index = 0; slot_index < 0xc; slot_index = slot_index + 1)
  {
    if ((g_world_magic_bitmap & (1U << (unsigned char)slot_index)) == 0)
    {
      open_slot_count = open_slot_count + 1;
    }
  }

  if (open_slot_count == 0)
  {
    return -1;
  }

  selection = internal_rand(open_slot_count);

  for (slot_index = 0; (slot_index < 0xc) && (selection != 0); slot_index = slot_index + 1)
  {
    if ((g_world_magic_bitmap & (1U << (unsigned char)slot_index)) == 0)
    {
      selection = selection - 1;
    }
  }

  return slot_index;
}

// FUNCTION: SHANDALAR 0x004f2232
int PickWisemanCastleDungeonSlotForHint(void)
{
  int dungeon_index;      /* ebp-0x10 */
  int best_score;         /* ebp-0xc */
  int best_dungeon_index; /* ebp-0x8 */
  int score;              /* ebp-0x4: reused for distance and score */

  best_score = 0x7fff;
  best_dungeon_index = -1;
  dungeon_index = 0;

  for (; dungeon_index < 0xf; dungeon_index = dungeon_index + 1)
  {
    if ((g_castle_dungeon_slots[dungeon_index].card_slot_1 == -1) ||
        (g_castle_dungeon_slots[dungeon_index].clues_bitmap == 7))
    {
      continue;
    }

    score = ApproximateDistance(g_world_player_tile_x - g_castle_dungeon_slots[dungeon_index].world_x,
                                g_world_player_tile_y - g_castle_dungeon_slots[dungeon_index].world_y);

    score = internal_rand(score) + score / 2;

    if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 0x200) != 0)
    {
      score = score * 3;
    }

    if (best_score > score)
    {
      best_dungeon_index = dungeon_index;
      best_score = score;
    }
  }

  return best_dungeon_index;
}

// FUNCTION: SHANDALAR 0x00508970
void RevealRandomCastleDungeonClue(int dungeon_index)
{
  int clue_index;

  do
  {
    clue_index = internal_rand(3);
  } while ((g_castle_dungeon_slots[dungeon_index].clues_bitmap & (1U << (unsigned char)clue_index)) != 0);

  g_castle_dungeon_slots[dungeon_index].clues_bitmap |= 1 << (unsigned char)clue_index;
  ShowDungeonClueDetailScreen(dungeon_index);
}

// FUNCTION: SHANDALAR 0x005318f5
unsigned int RunWisemanAdviceSequence(int preferred_color, int new_town_visit, int town_index)
{
  char quest_spell_name[100];

  int previous_city_block_index;
  int city_page_count;
  int city_page_index;
  char *city_page_ptrs[10];

  int city_line_count;
  char *text_page;
  int general_page_count;
  int general_page_index;
  char *general_page_ptrs[10];

  int general_line_count;
  char *general_text_page;
  int force_current_advice_stage;
  int selected_deck_index;
  int selected_deck_slot;
  int deck_scan_index;
  int dungeon_index;
  unsigned int deck_card_ids[500];

  force_current_advice_stage = 0;
  AnimateVisitBackdropZoomIn("wiseman3.pic");

  if (new_town_visit == 0)
  {
    force_current_advice_stage = 1;
    switch (g_wiseman_advice_stage)
    {
    case 0:
      break;
    case 1:
      goto LAB_00531afb;
    case 2:
      goto LAB_00531ee7;
    }
  }

LAB_0053197c:
  if (((internal_rand(5) == 0) || (force_current_advice_stage != 0)) || (new_town_visit == 0))
  {
    if ((g_wiseman_general_block_index == -1) || (new_town_visit != 0))
    {
      g_wiseman_general_block_index = rand() % 0xc;
    }

    general_text_page = gs_advblock_general_0077c9e0[g_wiseman_general_block_index];
    general_page_count = SplitTextOnFourNewlines(general_text_page, general_page_ptrs) - 1;

    for (general_page_index = 0; general_page_index < general_page_count; general_page_index = general_page_index + 1)
    {
      general_text_page = general_page_ptrs[general_page_index];
      general_page_ptrs[general_page_index + 1][-3] = '\0';

      general_line_count = CountTextNewlines(general_text_page);
      g_page0_window_bounds->font_slot = 1;

      DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * general_line_count, general_text_page);

      ClearInputAndWaitForMouseRelease();
      WaitForInputEventUnlessBlocked();
      general_page_ptrs[general_page_index + 1][-3] = '\n';
      LoadVisitBackdropAndCopyToPage0("wiseman3.pic");
    }

    g_wiseman_advice_stage = 0;
    return 1;
  }

LAB_00531afb:
  if ((internal_rand(5) == 0) || (force_current_advice_stage != 0))
  {
    previous_city_block_index = g_wiseman_city_block_index;
    if ((g_wiseman_city_block_index == -1) || (new_town_visit != 0))
    {
      g_wiseman_city_block_index = rand() % 3;
      g_wiseman_city_block_color_index = PickUndefeatedWizardColor();
      g_wiseman_city_block_world_magic_slot_index = PickOpenWorldMagicSlot();
      FillWisemanCityBlockTownNameSubstitutions(g_wiseman_city_block_subst_a, g_wiseman_city_block_subst_b, 1);
    }

    text_page = gs_advblock_city_0074c930[g_wiseman_city_block_index];
    if (g_wiseman_city_block_index == 0)
    {
      if (IsWizardColorFeminine(g_wiseman_city_block_color_index))
      {
        text_page = gs_advblock_city_0074c930[3];
      }
    }

    city_page_count = SplitTextOnFourNewlines(text_page, city_page_ptrs) - 1;
    g_page0_window_bounds->font_slot = 1;

    for (city_page_index = 0; city_page_index < city_page_count; city_page_index = city_page_index + 1)
    {
      text_page = city_page_ptrs[city_page_index];
      city_page_ptrs[city_page_index + 1][-3] = '\0';

      city_line_count = CountTextNewlines(text_page);

      if (city_page_index != (city_page_count - 1))
      {
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * city_line_count, text_page);
      }
      else if (g_wiseman_city_block_index == 0)
      {
        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, text_page,
                                                    gs_wizardnames_0077ee70[g_wiseman_city_block_color_index],
                                                    ComputeWisemanCityBlockValue(g_wiseman_city_block_color_index),
                                                    CountTownsRuledByWizardColor(g_wiseman_city_block_color_index));
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * city_line_count, g_ui_message_buffer);
      }
      else if (g_wiseman_city_block_index == 1)
      {
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * city_line_count, text_page, g_wiseman_city_block_subst_a,
                                  g_wiseman_city_block_subst_b);
      }
      else if (g_wiseman_city_block_index == 2)
      {
        if (g_wiseman_city_block_world_magic_slot_index == -1)
        {
          strcpy(g_wiseman_city_block_subst_a, "");
        }
        else
        {
          strcpy(g_wiseman_city_block_subst_a, gs_worldmagic_names_00780660[g_wiseman_city_block_world_magic_slot_index]);
        }
        strcpy(g_wiseman_city_block_subst_b,
               BuildTownDisplayName(g_world_magic_slot_timers[g_wiseman_city_block_world_magic_slot_index].town_index));
        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, text_page, g_wiseman_city_block_subst_a, g_wiseman_city_block_subst_b);
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * city_line_count, g_ui_message_buffer);
      }

      ClearInputAndWaitForMouseRelease();
      WaitForInputEventUnlessBlocked();

      city_page_ptrs[city_page_index + 1][-3] = '\n';
      LoadVisitBackdropAndCopyToPage0("wiseman3.pic");
    }

    g_wiseman_advice_stage = 1;
    return 1;
  }

  g_wiseman_advice_stage = 2;

  /* Quest/status block + special actions. */
LAB_00531ee7:
  if ((internal_rand(5) < 2) || (force_current_advice_stage != 0))
  {
    if (g_current_quest_destination != -1)
    {
      if ((g_current_quest_type == 0) || (g_current_quest_type == 2) ||
          ((g_current_quest_type == 1) &&
           (FindDeckSlotForQuestColorAndType((unsigned char)g_current_quest_color,
                                             1 << (g_current_quest_destination & 3)))))
      {
        g_current_quest_data =
            GetRelativeWorldQuadrant(g_town_slots[g_current_quest_destination].world_x,
                                     g_town_slots[g_current_quest_destination].world_y);

        if (g_current_quest_type != 0)
        {
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_wiseman_0074d840[0],
                                                      gs_amuletnames_0077d090[g_current_quest_color],
                                                      gs_directions_00765d50[g_current_quest_data],
                                                      BuildTownDisplayName(g_current_quest_destination));
        }
        else
        {
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_wiseman_0074d840[1],
                                                      gs_directions_00765d50[g_current_quest_data],
                                                      BuildTownDisplayName(g_current_quest_destination));
        }

        g_page0_window_bounds->font_slot = 1;
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 2, g_ui_message_buffer);
        ClearInputAndWaitForMouseRelease();
        WaitForInputEventUnlessBlocked();
        return 0;
      }

      if (g_current_quest_type == 1)
      {
        FormatQuestSpellName(quest_spell_name, 100);
        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_wiseman_0074d840[2],
                                                    BuildTownDisplayName(g_current_quest_destination), quest_spell_name);

        g_page0_window_bounds->font_slot = 1;
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 3, g_ui_message_buffer);
        ClearInputAndWaitForMouseRelease();
        WaitForInputEventUnlessBlocked();
        return 0;
      }

      if (g_current_quest_type < 0)
      {
        if (g_current_quest_type <= -100)
        {
          g_current_quest_data =
              GetRelativeWorldQuadrant(g_town_slots[g_current_quest_destination].world_x,
                                       g_town_slots[g_current_quest_destination].world_y);
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_wiseman_0074d840[3],
                                                      gs_directions_00765d50[g_current_quest_data],
                                                      BuildTownDisplayName(g_current_quest_destination));
        }
        else
        {
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_wiseman_0074d840[4],
                                                      BuildTownDisplayName(g_current_quest_destination),
                                                      BuildCreatureNameWithArticle(-g_current_quest_type));
        }

        g_page0_window_bounds->font_slot = 1;
        DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                  ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 3, g_ui_message_buffer);
        ClearInputAndWaitForMouseRelease();
        WaitForInputEventUnlessBlocked();
        return 0;
      }
    }
    new_town_visit = 0;
  }

  g_wiseman_advice_stage = 0;
  strcpy(g_ui_message_buffer, gs_wiseman_0074d840[5]);

  switch ((g_town_slots[town_index].world_x % 2) + (g_town_slots[town_index].world_y % 3))
  {
  case 0:
    strcat(g_ui_message_buffer, gs_wiseman_0074d840[6]);
    g_page0_window_bounds->font_slot = 1;
    DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                              ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 7, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    force_current_advice_stage = 1;
    LoadVisitBackdropAndCopyToPage0("wiseman3.pic");
    goto LAB_0053197c;
    break;

  case 1:
    dungeon_index = PickWisemanCastleDungeonSlotForHint();
    if (dungeon_index != -1)
    {
      strcat(g_ui_message_buffer, gs_wiseman_0074d840[7]);
      g_page0_window_bounds->font_slot = 1;
      DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                                ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 7, g_ui_message_buffer);
      ClearInputAndWaitForMouseRelease();
      WaitForInputEventUnlessBlocked();
      RevealRandomCastleDungeonClue(dungeon_index);
      RefreshAdventureInterfaceLayout();
      return 0;
    }
  case 2:
    if (g_next_duel_card_id == -1)
    {
      strcat(g_ui_message_buffer, gs_wiseman_0074d840[8]);
      g_next_duel_card_id = 2;
    }
    else
    {
      strcat(g_ui_message_buffer, gs_wiseman_0074d840[9]);
      g_food = g_food + 0x19;
    }
    g_page0_window_bounds->font_slot = 1;
    DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                              ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 7, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    return 0;

  LAB_005324c3:
  case 3:
    sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_wiseman_0074d840[10], gs_wiseman_0074d840[preferred_color + 10]);
    g_page0_window_bounds->font_slot = 1;
    DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                              ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 7, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    WisemanChooseLairCreatureAndSetupDuel((unsigned char)preferred_color);
    RefreshAdventureInterfaceLayout();
    return 0;

  case 4:
    if (g_next_duel_card_id == -1)
    {
      g_next_duel_card_id = FindCardIndexByCsvid(g_wiseman_duel_reward_card_csvids[(internal_rand(2) - 2) + preferred_color * 2]);
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_wiseman_0074d840[0x10], global_cards_data[g_next_duel_card_id].name);
    }
    else
    {
      goto LAB_005324c3;
    }
    g_page0_window_bounds->font_slot = 1;
    DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                              ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 7, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    return 0;
  default:
    sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_wiseman_0074d840[0x11], gs_amuletnames_plural_0077edd0[preferred_color]);
    g_page0_window_bounds->font_slot = 1;
    DrawFormattedTextShadowed(g_page0_window_bounds, 0xc1, ScaleUiCoordinate(0x50),
                              ScaleUiCoordinate(0x13c) - GetFontLineHeight(g_page0_window_bounds->font_slot) * 7, g_ui_message_buffer);

    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();

    g_amulet_inventory[preferred_color - 1] -= 3;
    for (deck_scan_index = 0; deck_scan_index < 500; deck_scan_index = deck_scan_index + 1)
    {
      deck_card_ids[deck_scan_index] = deck[deck_scan_index];
      if (deck_card_ids[deck_scan_index] != 0xffffffff)
      {
        deck_card_ids[deck_scan_index] = deck_card_ids[deck_scan_index] & 0xfff;
      }
    }

    selected_deck_index = SelectAdventureListCardIndex(g_active_player, (int *)deck_card_ids, 500, gs_wiseman_0074d840[0x12], 1, &g_wiseman_card_choice_result);
    if ((selected_deck_index != -1) &&
        ((selected_deck_slot = AddCardToDeckSorted(deck_card_ids[selected_deck_index])) != -1))
    {
      deck[selected_deck_slot] = deck[selected_deck_slot] | 0x4000;
    }
    RefreshAdventureInterfaceLayout();
    return 0;
  }
}

// FUNCTION: SHANDALAR 0x005328e4
int RenderCardBrowserDoneButton(AdvMenuControl *control, int mode)
{
  int in_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || ((control->x + control->width) < g_mouse_x))
    {
      in_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || ((control->y + control->height) < g_mouse_y))
    {
      in_bounds = 0;
    }
    else
    {
      in_bounds = 1;
    }
    if (in_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  if (mode == 2)
  {
    DrawEncodedImageResampled(g_page1_window_bounds, control->x, control->y, control->width, control->height, g_card_browser_done_button_sprites[mode]);
    BlitGraphicsRect(g_page1_window_bounds, control->x, control->y, control->width, control->height, g_page0_window_bounds, control->x, control->y);
  }
  else
  {
    DrawEncodedImageResampled(g_page0_window_bounds, control->x, control->y, control->width, control->height, g_card_browser_done_button_sprites[mode]);
  }

  if (mode == 2 && control->on_activate != (AdvMenuActivateCallback)0)
  {
    (void)control->on_activate(control);
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x00532a7c
int HandleCardBrowserDoneButton(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_card_browser_done_button_value = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x00532aae
int CompareCardNamesForQsort(const int *a, const int *b)
{
  return strcoll(global_cards_data[*a].name, global_cards_data[*b].name);
}

// FUNCTION: SHANDALAR 0x00532aeb
unsigned int RunCardBrowser(char *title, unsigned int color_mask, unsigned int type_mask, int reset_filters, int show_done_button)
{
  int card_list_text_color;
  int rarity_text_color;
  int type_text_color;
  int filtered_type_text_color;
  int color_text_color;
  int all_colors_text_color;
  unsigned int filtered_count;
  int sorted_index;
  unsigned int filtered_card_ids[300];

  /* [0..2] button colors, [3] menu context */
  int button_colors[4];                /* [ebp-0x150] */
  int owned_copies;                    /* [ebp-0x140] */
  int rarity_filter;                   /* [ebp-0x13c] */
  int type_filter_index;               /* [ebp-0x138] */
  int mouse_button_or_index;           /* [ebp-0x134] */
  int deck_slot;                       /* [ebp-0x130] */
  int y;                               /* [ebp-0x12c] */
  int card_index;                      /* [ebp-0x128] */
  int x;                               /* [ebp-0x124] */
  unsigned int draw_index_to_card[64]; /* [ebp-0x120] */
  int draw_count;                      /* [ebp-0x20] */
  int max_lines_per_col;               /* [ebp-0x1c] */
  int filter_top_y;                    /* [ebp-0x18] */
  int filter_line_h;                   /* [ebp-0x14] */
  unsigned int card_color_mask;        /* [ebp-0x10] */
  int needs_redraw;                    /* [ebp-0xc] */
  int list_top_y;                      /* [ebp-0x8] */
  int list_line_h;                     /* [ebp-0x4] */

  needs_redraw = 1;

  ClearInputAndWaitForMouseRelease();

  if (reset_filters != 0)
  {
    LoadPcxResource(1, 0, global_screen_height - 0x1e0, "tradscrn.pic", (void *)0);
    StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x1e0, 0x280, 0x1e0, g_page1_window_bounds, 0, 0, global_screen_width,
                            global_screen_height);
    CopyGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page0_window_bounds, 0, 0);
  }

  if (show_done_button != 0)
  {
    button_colors[0] = 0x1a;
    button_colors[1] = 0x19;
    button_colors[2] = 0x19;

    LoadPcxIntoPage(1, "tradbut1.pic");
    BeginSpriteEncodeSession();
    g_page1_window_bounds->font_slot = 7;

    for (card_index = 0; card_index < 3; card_index = card_index + 1)
    {
      SetFontStyleSize(7, (card_index == 2) ? 9 : 11);
      DrawFormattedTextNoShadowCentered(g_page1_window_bounds, button_colors[card_index], card_index * 0x62 + 0x32, 0xd, "%s",
                                        (char *)g_done_text_table_entry);
      g_card_browser_done_button_sprites[card_index] = EncodeSpriteFromPage(1, card_index * 0x62 + 1, 1, 0x61, 0x1b);
    }

    FinalizeSpriteEncodeSession();

    if (g_card_browser_done_button.base_x == g_card_browser_done_button.x)
    {
      g_card_browser_done_button.x = ScaleUiCoordinate(g_card_browser_done_button.x);
      g_card_browser_done_button.y = ScaleUiCoordinate(g_card_browser_done_button.y);
      g_card_browser_done_button.width = ScaleUiCoordinate(g_card_browser_done_button.width);
      g_card_browser_done_button.height = ScaleUiCoordinate(g_card_browser_done_button.height);
    }

    button_colors[3] = BeginMenuContext();
    (void)ResetMenuContext(button_colors[3]);
    (void)AddMenuControlsToContext(&g_card_browser_done_button, 1, button_colors[3]);
    (void)RenderCurrentMenuContextControls();
  }

  if (type_mask != 0xffffffff)
  {
    g_card_browser_type_filter = (int)type_mask;
    type_mask = 1U << (unsigned char)type_mask;
    if (type_mask == 0x10)
    {
      type_mask |= 0x20;
    }
  }
  else if (reset_filters != 0)
  {
    type_mask = 0;
    g_card_browser_type_filter = 1;
  }

  if ((color_mask != 0) && (reset_filters != 0))
  {
    g_card_browser_color_filter = single_color_test_bit_to_color_t((int)color_mask);
  }

  if (reset_filters != 0)
  {
    g_card_browser_hover_card = 0xffffffff;
  }

loop:
  if (needs_redraw != 0)
  {
    CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xd), ScaleUiCoordinate(0x8c), ScaleUiCoordinate(0xb3), ScaleUiCoordinate(0x132),
                     g_page0_window_bounds, ScaleUiCoordinate(0xd), ScaleUiCoordinate(0x8c));
    CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xd), ScaleUiCoordinate(0x8c), ScaleUiCoordinate(0xb3), ScaleUiCoordinate(0x132),
                     g_page0_window_bounds, ScaleUiCoordinate(0x1c0), ScaleUiCoordinate(0x8c));
    CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xb4), ScaleUiCoordinate(0x1bf), ScaleUiCoordinate(0x118), ScaleUiCoordinate(0x1d),
                     g_page0_window_bounds, ScaleUiCoordinate(0xb4), ScaleUiCoordinate(0x1bf));
    CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xe9), ScaleUiCoordinate(0x158), ScaleUiCoordinate(0xd4), ScaleUiCoordinate(0x68),
                     g_page0_window_bounds, ScaleUiCoordinate(0xe9), ScaleUiCoordinate(0x158));
    CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0x1db), ScaleUiCoordinate(0x10), ScaleUiCoordinate(0x7c), ScaleUiCoordinate(0x5e),
                     g_page0_window_bounds, ScaleUiCoordinate(0x1db), ScaleUiCoordinate(0x10));
    CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xe4), ScaleUiCoordinate(0x27), ScaleUiCoordinate(0xb8), ScaleUiCoordinate(0x19),
                     g_page0_window_bounds, ScaleUiCoordinate(0xe4), ScaleUiCoordinate(0x27));
  }
  needs_redraw = 0;
  CopyGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xe8), ScaleUiCoordinate(0x157), ScaleUiCoordinate(0xd2), ScaleUiCoordinate(0x66),
                   g_page0_window_bounds, ScaleUiCoordinate(0xe8), ScaleUiCoordinate(0x157));
  g_page0_window_bounds->font_slot = 4;
  DrawTextAt(g_page0_window_bounds, 0x16, 0x140, 0x34, title);
  g_page0_window_bounds->font_slot = 4;

  /* Draw color filter list (left). */
  x = ScaleUiCoordinateFrom320(0x6b) / 2;
  y = ScaleUiCoordinateFrom320(0x45) / 2;
  y = y - ((GetFontLineHeight(g_page0_window_bounds->font_slot) * 6) / 2);

  for (card_index = 0; card_index < 6; card_index = card_index + 1)
  {
    if (card_index == 0)
    {
      if (card_index == g_card_browser_color_filter)
      {
        all_colors_text_color = 0x1b;
      }
      else if (color_mask != 0)
      {
        all_colors_text_color = 0;
      }
      else
      {
        all_colors_text_color = 0x62;
      }

      DrawCenteredTextLineWithShadow(gs_browse_0074da20[0], x, y, all_colors_text_color);
    }
    else
    {
      if (card_index == g_card_browser_color_filter)
      {
        color_text_color = 0x1b;
      }
      else if (color_mask != 0)
      {
        color_text_color = 0;
      }
      else
      {
        color_text_color = 0x62;
      }

      DrawCenteredTextLineWithShadow(gs_browse_0074da20[card_index], x, y, color_text_color);
    }
    y = y + GetFontLineHeight(g_page0_window_bounds->font_slot);
  }

  /* Draw type/rarity filter list (right). */
  x = ScaleUiCoordinateFrom320(0x215) / 2;
  y = ScaleUiCoordinateFrom320(0x45) / 2;
  y = y - ((GetFontLineHeight(g_page0_window_bounds->font_slot) * 6) / 2);
  filter_top_y = y;
  filter_line_h = GetFontLineHeight(g_page0_window_bounds->font_slot);

  for (card_index = 0; card_index < 6; card_index = card_index + 1)
  {
    if (g_card_browser_color_filter != 0)
    {
      if (card_index == g_card_browser_type_filter)
      {
        filtered_type_text_color = 0x1b;
      }
      else if (type_mask != 0)
      {
        filtered_type_text_color = 0x62;
      }
      else
      {
        filtered_type_text_color = 0x62;
      }

      DrawCenteredTextLineWithShadow(gs_browse_0074da20[6 + card_index], x, y, filtered_type_text_color);
    }
    else if (card_index < 2)
    {
      if (card_index == g_card_browser_type_filter)
      {
        type_text_color = 0x1b;
      }
      else if (type_mask != 0)
      {
        type_text_color = 0x62;
      }
      else
      {
        type_text_color = 0x62;
      }

      DrawCenteredTextLineWithShadow(gs_browse_0074da20[6 + card_index], x, y, type_text_color);
    }
    else
    {
      if (card_index == g_card_browser_type_filter)
      {
        rarity_text_color = 0x1b;
      }
      else if (type_mask != 0)
      {
        rarity_text_color = 0x62;
      }
      else
      {
        rarity_text_color = 0x62;
      }

      DrawCenteredTextLineWithShadow(gs_browse_0074da20[11 + card_index], x, y, rarity_text_color);
    }

    y = y + GetFontLineHeight(g_page0_window_bounds->font_slot);
  }

  /* Type filter index used for type-bit selection. */
  type_filter_index = g_card_browser_type_filter;
  if ((g_card_browser_color_filter == 0) && (g_card_browser_type_filter >= 2))
  {
    rarity_filter = g_card_browser_type_filter - 1;
    type_filter_index = 6;
  }

  g_page0_window_bounds->font_slot = 1;
  DrawTextAt(g_page0_window_bounds, 0xff, 0xfe, 0x1ce, "%d", Gold);
  for (card_index = 0; card_index < 5; card_index = card_index + 1)
  {
    DrawTextAt(g_page0_window_bounds, 0xff, card_index * 35 + 305, 0x1ce, "%d", g_amulet_inventory[card_index]);
  }

  /* Filter list hit-test geometry is stored in filter_top_y / filter_line_h. */

  /* Card list geometry. */
  max_lines_per_col = (ScaleUiCoordinateFrom320(0x11e) / 2) / GetFontLineHeight(g_page0_window_bounds->font_slot);
  draw_count = 0;
  x = ScaleUiCoordinateFrom320(0x66) / 2;
  y = ScaleUiCoordinateFrom320(0x99) / 2;
  list_top_y = y;
  list_line_h = GetFontLineHeight(g_page0_window_bounds->font_slot);

  /* Build filtered list. */
  filtered_count = 0;
  for (card_index = 0; card_index < g_card_count - 57; card_index = card_index + 1)
  {
    if ((g_card_browser_color_filter == 0) && (type_filter_index == 6))
    {
      if (ClampIntToRange((int)(signed char)global_cards_data[card_index].rarity, 1, 3) != rarity_filter)
      {
        continue;
      }
    }

    card_color_mask = (unsigned int)(signed char)global_cards_data[card_index].color;
    if (card_color_mask == 0)
    {
      card_color_mask = 1;
    }

    if (((card_color_mask & (1U << (unsigned char)g_card_browser_color_filter)) == 0) ||
        (((unsigned int)global_cards_data[card_index].type & (1U << (unsigned char)type_filter_index)) == 0))
    {
      continue;
    }

    if (((global_cards_data[card_index].extra_ability & 0x900) != 0) && (g_reveal_all_world_info == 0))
    {
      continue;
    }

    if (GetCardAvailabilityMask(card_index) == 0)
    {
      continue;
    }

    if ((color_mask != 0) && ((card_color_mask & color_mask) == 0))
    {
      continue;
    }

    if ((type_mask != 0) && (((unsigned int)global_cards_data[card_index].type & type_mask) == 0))
    {
      continue;
    }

    if (((unsigned int)global_cards_data[card_index].type == 'B') && (type_filter_index == 6))
    {
      continue;
    }

    filtered_card_ids[filtered_count] = (unsigned int)card_index;
    filtered_count = filtered_count + 1;
  }

  qsort(filtered_card_ids, filtered_count, 4, (int(__cdecl *)(const void *, const void *))CompareCardNamesForQsort);

  for (sorted_index = 0; (int)filtered_count > sorted_index; sorted_index = sorted_index + 1)
  {
    card_index = filtered_card_ids[sorted_index];

    /* Count copies owned in the 500-slot deck array. */
    owned_copies = 0;
    for (deck_slot = 0; deck_slot < 500; deck_slot = deck_slot + 1)
    {
      if (((unsigned int)deck[deck_slot] & 0xfffU) == (unsigned int)card_index)
      {
        owned_copies = owned_copies + 1;
      }
    }

    strcpy(g_ui_message_buffer, global_cards_data[card_index].name);
    if (owned_copies != 0)
    {
      strcat(g_ui_message_buffer, "-");
      strcat(g_ui_message_buffer, _itoa(owned_copies, g_itoa_buffer, 10));
    }

    if (g_card_browser_hover_card == 0xffffffff)
    {
      g_card_browser_hover_card = (unsigned int)card_index;
    }

    if ((unsigned int)card_index == g_card_browser_hover_card)
    {
      card_list_text_color = 0x1b;
    }
    else
    {
      if (GetRemainingAllowedCardCopies((unsigned int)card_index) > 0)
      {
        card_list_text_color = 0x54;
      }
      else
      {
        card_list_text_color = 3;
      }
    }

    DrawCenteredTextLineWithShadow(g_ui_message_buffer, x, y, card_list_text_color);
    draw_index_to_card[draw_count] = (unsigned int)card_index;
    draw_count = draw_count + 1;
    y = y + GetFontLineHeight(g_page0_window_bounds->font_slot);

    if (draw_count == max_lines_per_col)
    {
      x = ScaleUiCoordinateFrom320(0x219) / 2;
      y = ScaleUiCoordinateFrom320(0x99) / 2;
    }
  }

  if (g_card_browser_hover_card != 0xffffffff)
  {
    g_card_browser_selected_card = g_card_browser_hover_card;
    DrawAdventureCardSized((int)g_card_browser_selected_card, 0x7b, 0x33, 0x4b, 0x70, 1, "");
  }

  ShowMouseCursorNested();
  g_card_browser_done_button_value = -1;

  while (1)
  {
    UpdateMouseSnapshot();
    if (show_done_button != 0 && g_mouse_button_down_mask != 0)
    {
      (void)UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
      if (0 < g_card_browser_done_button_value)
      {
        (void)EndMenuContext();
        FreeSpriteBlob(g_card_browser_done_button_sprites[0]);
        ClearInputAndWaitForMouseRelease();
        DestroyCachedCardArt();
        return 0xffffffff;
      }
      else
      {
        mouse_button_or_index = g_mouse_button_down_mask;
        break;
      }
    }
    else if (g_mouse_button_down_mask != 0)
    {
      mouse_button_or_index = g_mouse_button_down_mask;
      break;
    }
    else if (show_done_button != 0)
    {
      (void)UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
    }
  }

  x = g_mouse_x_snapshot;
  y = g_mouse_y_snapshot;
  ClearInputAndWaitForMouseRelease();
  HideMouseCursorNested();

  /* OK click region returns the current selected card. */
  if (((ScaleUiCoordinateFrom320(0x66) / 2 < y) && (y < ScaleUiCoordinateFrom320(0x146) / 2)) &&
      ((ScaleUiCoordinateFrom320(0xf6) / 2 < x) && (x < ScaleUiCoordinateFrom320(0x18c) / 2)))
  {
    if (show_done_button != 0)
    {
      (void)EndMenuContext();
      FreeSpriteBlob(g_card_browser_done_button_sprites[0]);
    }
    DestroyCachedCardArt();
    return g_card_browser_selected_card;
  }

  /* Done button rect check: click on the button exits the browser. */
  if (show_done_button != 0 &&
      x >= g_card_browser_done_button.x &&
      x <= g_card_browser_done_button.x + g_card_browser_done_button.width &&
      y >= g_card_browser_done_button.y &&
      y <= g_card_browser_done_button.y + g_card_browser_done_button.height)
  {
    (void)EndMenuContext();
    FreeSpriteBlob(g_card_browser_done_button_sprites[0]);
    DestroyCachedCardArt();
    return 0xffffffff;
  }

  if (mouse_button_or_index)
  {
    if (y < ScaleUiCoordinateFrom320(0x6d) / 2)
    {
      /* Filter list selection: index = (mouse_y - filter_top_y) / line_h. */
      mouse_button_or_index = (y - filter_top_y) / filter_line_h;

      if (x < ScaleUiCoordinate(0xac))
      {
        if ((mouse_button_or_index >= 0) && (mouse_button_or_index < 6) && ((color_mask == 0) || ((1U << (unsigned char)mouse_button_or_index) == color_mask)))
        {
          g_card_browser_color_filter = mouse_button_or_index;
          g_card_browser_hover_card = 0xffffffff;
          needs_redraw = 1;
        }
      }
      else
      {
        if ((ScaleUiCoordinate(0x1d6) < x) && (mouse_button_or_index >= 0) && (mouse_button_or_index < 6))
        {
          g_card_browser_type_filter = mouse_button_or_index;
          g_card_browser_hover_card = 0xffffffff;
          needs_redraw = 1;
        }
      }
    }
    else
    {
      if ((ScaleUiCoordinateFrom320(0x96) / 2) < y)
      {
        /* Card list selection. */
        mouse_button_or_index = (y - list_top_y) / list_line_h;
        mouse_button_or_index = mouse_button_or_index + ((x / (global_screen_width / 2)) * max_lines_per_col);
        if (mouse_button_or_index < draw_count)
        {
          g_card_browser_hover_card = draw_index_to_card[mouse_button_or_index];
        }
      }
    }
    goto loop;
  }

  if (show_done_button != 0)
  {
    (void)EndMenuContext();
  }
  FreeSpriteBlob(g_card_browser_done_button_sprites[0]);
  DestroyCachedCardArt();
  return 0xffffffff;
}

// FUNCTION: SHANDALAR 0x00530b7e
int QuestCardChooserCallback(int x, int y, int width, int height, int frame_style)
{
  // TODO(decomp): Callback used by the card-chooser UI while a "quest card marking" flow is active.
  return 0;
}

// FUNCTION: SHANDALAR 0x00530b90
int BuyAnyCardFromTown(int payment_color, int town_index)
{
  int tile_magic_index;              // ebp - 0x30
  unsigned int card_id;              // ebp - 0x2c
  TownDialogCallback saved_callback; // ebp - 0x28
  int required;                      // ebp - 0x24
  int total_mana;                    // ebp - 0x20
  int can_afford;                    // ebp - 0x1c
  int menu_result;                   // ebp - 0x18
  int random_color;                  // ebp - 0x14
  int first_time;                    // ebp - 0x10
  int i;                             // ebp - 0x0c
  int copies;                        // ebp - 0x08
  int price;                         // ebp - 0x04

  first_time = 1;
  saved_callback = g_town_dialog_callback;
  g_town_dialog_callback = (TownDialogCallback)QuestCardChooserCallback;

  while (1)
  {
    if (town_index != -1)
    {
      card_id = RunCardBrowser(gs_buyanycard_0074ccd0[5], g_town_slots[town_index].trade_color_and_type & 0xff,
                               (g_town_slots[town_index].trade_color_and_type >> 8) - 1, first_time, 1);
    }
    else
    {
      card_id = RunCardBrowser(gs_buyanycard_0074ccd0[5], 0, 0xffffffff, first_time, 1);
    }
    first_time = 0;

    if (payment_color == 0)
    {
      total_mana = 0;
      for (i = 0; i < 5; i = i + 1)
      {
        total_mana = total_mana + g_amulet_inventory[i];
      }
    }

    if (card_id == 0xffffffff)
    {
      break;
    }

    if (payment_color == -2)
    {
      can_afford = 1;
      if ((int)card_id > 4)
      {
        copies = 1;
      }
      else
      {
        copies = 4;
      }
    }
    else
    {
      required = GetCardRarity(card_id);

      switch (required)
      {
      case 1:
        if ((int)card_id > 4)
        {
          copies = 1;
        }
        else
        {
          copies = 4;
        }
        break;
      case 2:
        copies = 1;
        break;
      default:
        copies = 1;
      }

      if ((required < 2) && ((global_cards_data[card_id].extra_ability & 0x400) != 0))
      {
        required = required + 1;
      }

      price = CalculateCardShopPrice(card_id);
      price = price << 2;

      tile_magic_index = single_color_test_bit_to_color_t((int)GetWorldTileMagicMask(GetWorldTileType(g_world_player_x / 32, g_world_player_y / 32)));

      if (((tile_magic_index & (int)(signed char)global_cards_data[card_id].color) == 0) &&
          ((signed char)global_cards_data[card_id].color != 0))
      {
        if (IsCardColorCompatibleWithMask(tile_magic_index, (int)(signed char)global_cards_data[card_id].color, 3) != 0)
        {
          price = (price * 3) / 2;
        }
        else
        {
          price = price << 1;
        }
      }

      price = (price * 0x4e2) / (internal_rand(((g_world_player_y & 0x1f) * (g_world_player_x & 0x1f)) / 10) + 0x2ee);

      price = ClampIntToRange((price / 0x32) * 5, 5, 1000);

      price = copies * price;

      price = MAX(required * 0x32, price);

      if (payment_color >= 0)
      {
        if (required == 1)
        {
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_buyanycard_0074ccd0[0], copies, global_cards_data[card_id].name,
                                                      gs_amuletnames_0077d090[payment_color]);
        }
        else
        {
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_buyanycard_0074ccd0[1], copies, global_cards_data[card_id].name, required,
                                                      gs_amuletnames_plural_0077edd0[payment_color]);
        }
      }
      else
      {
        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_buyanycard_0074ccd0[2], copies, global_cards_data[card_id].name, price);
      }

      can_afford = 1;
      if ((0 < payment_color) && (g_amulet_inventory[payment_color - 1] < required))
      {
        can_afford = 0;
      }
      if ((payment_color == 0) && (total_mana < required))
      {
        can_afford = 0;
      }
      if ((payment_color == -1) && (price > Gold))
      {
        can_afford = 0;
      }

      if (can_afford != 0)
      {
        (void)AppendString(g_ui_message_buffer, gs_buyanycard_0074ccd0[3]);
      }
      else
      {
        (void)AppendString(g_ui_message_buffer, gs_buyanycard_0074ccd0[4]);
      }

      menu_result = RunTextMenuAt(g_ui_message_buffer, ScaleUiCoordinate(0xea), ScaleUiCoordinate(0x160));
      if ((menu_result != 0) || (can_afford == 0))
      {
        continue;
      }

      can_afford = 0;
      if ((0 < payment_color) && (required <= g_amulet_inventory[payment_color - 1]))
      {
        can_afford = 1;
        g_amulet_inventory[payment_color - 1] -= required;
      }
      if ((payment_color == 0) && (required <= total_mana))
      {
        can_afford = 1;
        do
        {
          do
          {
            random_color = internal_rand(5);
          } while (g_amulet_inventory[random_color] == 0);
          required = required - 1;
          g_amulet_inventory[random_color] = g_amulet_inventory[random_color] - 1;
        } while (required != 0);
      }
      if ((payment_color == -1) && (price <= Gold))
      {
        can_afford = 1;
        Gold = Gold - price;
      }
    }

    if (can_afford == 0)
    {
      break;
    }
    for (i = 0; copies > i; i = i + 1)
    {
      random_color = AddCardToDeckSorted(card_id);
      if (random_color != -1)
      {
        deck[random_color] = deck[random_color] | 0x4000;
      }
    }

    (void)RecountDeckCardTotals();
    DestroyCachedCardArt();
  }

  DestroyCachedCardArt();
  g_town_dialog_callback = saved_callback;
  return 0;
}

// FUNCTION: SHANDALAR 0x004eda50
int VisitTownSlot(int town_index)
{
  unsigned int journal_param;              // ebp - 0x94
  int world_quadrant;                      // ebp - 0x90
  int tile_color_index;                    // ebp - 0x8c
  int menu_result;                         // ebp - 0x88
  size_t msg_len;                          // ebp - 0x84
  int amulet_name;                         // ebp - 0x80
  EncodedImage *world_magic_button_sprite; // ebp - 0x7c

  int confirm_choice;          // ebp - 0x78
  int first_card_browser_draw; // ebp - 0x74

  char *pics2[5];          // ebp - 0x70
  char *pics[5];           // ebp - 0x58
  int world_magic_delta_y; // ebp - 0x48
  int world_magic_delta_x; // ebp - 0x44

  int duel_deck_seed;             // ebp - 0x40
  int selected_card_id;           // ebp - 0x3c
  int duel_wizard_color;          // ebp - 0x38
  EncodedImage *sprite;           // ebp - 0x34
  int ante_card_id;               // ebp - 0x30
  int found_deck_slot;            // ebp - 0x2c
  int duel_ante_slot;             // ebp - 0x28
  int world_magic_slot_index;     // ebp - 0x24
  int world_magic_text_y;         // ebp - 0x20
  int deck_card_index;            // ebp - 0x1c
  int world_magic_center_x;       // ebp - 0x18
  int duel_creature_tier_or_type; // ebp - 0x14
  int world_magic_distance;       // ebp - 0x10
  int distance;                   // ebp - 0xc
  int quest_handled;              // ebp - 0x8
  int duel_menu_result;           // ebp - 0x4

  g_current_town_slot_index = town_index;
  ClearInputAndWaitForMouseRelease();

  // NOTE: byte 1 of status_and_ruling_wizard is treated as the "ruling wizard" byte by the original code.
  if (g_town_slots[town_index].status_and_ruling_wizard & 0xff00)
  {
    // Duel path: wizard-controlled towns/dungeons.
    AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
    EnsureAdvfac64Loaded(1);

    for (duel_ante_slot = 0; duel_ante_slot < 0x10; duel_ante_slot = duel_ante_slot + 1)
    {
      global_ante_cards[1][duel_ante_slot] = 0xffffffff;
      global_ante_cards[0][duel_ante_slot] = global_ante_cards[1][duel_ante_slot];
    }

    (void)ExitIfNoUsableDeckCards();
    duel_deck_seed = g_town_slots[town_index].world_y;
    for (duel_ante_slot = 0; duel_ante_slot < g_town_slots[town_index].world_x % 3 + 1; duel_ante_slot = duel_ante_slot + 1)
    {
      do
      {
        do
        {
          found_deck_slot = duel_deck_seed % 500;
          duel_deck_seed = duel_deck_seed + 7;
        } while (deck[found_deck_slot] == -1);
      } while ((deck[found_deck_slot] & 0x4000) != 0 || ((deck[found_deck_slot] & 0xfff) <= 4));

      global_ante_cards[0][duel_ante_slot] = deck[found_deck_slot] & 0xfff;
      DrawAdventureCard(deck[found_deck_slot] & 0xfff, duel_ante_slot * 0x28 + 0x60, duel_ante_slot * 3 + 0x80, 1,
                        gs_visit_0077c4f0[0]);
    }

    duel_wizard_color = g_town_slots[town_index].status_and_ruling_wizard >> 8;
    switch (g_siege_timer / 0x80)
    {
    case 0:
      duel_creature_tier_or_type = 4;
      break;
    case 1:
      duel_creature_tier_or_type = 6;
      break;
    case 2:
      duel_creature_tier_or_type = 8;
      break;
    case 3:
      duel_creature_tier_or_type = 0xc;
      break;
    case 4:
      duel_creature_tier_or_type = 0x10;
      break;
    default:
      if ((g_town_slots[town_index].world_y & 1) != 0)
      {
        duel_creature_tier_or_type = 0x12;
      }
      else
      {
        duel_creature_tier_or_type = 0xe;
      }
      break;
    }

    duel_creature_tier_or_type = PickRandomCreatureTypeForWizardTier(duel_wizard_color, duel_creature_tier_or_type);
    LoadCreatureDuelDeck(duel_creature_tier_or_type, 0xffffffff, 0, -1);

    do
    {
      do
      {
        global_ante_cards[1][0] = DrawRandomCardFromInitialLibrary(g_opponent_initial_library_index);
      } while (global_ante_cards[1][0] <= 4);
    } while ((global_cards_data[global_ante_cards[1][0]].extra_ability & 0x100) != 0);

    DrawAdventureCard((int)global_ante_cards[1][0], 0xe0, 0x40, 1, gs_visit_0077c4f0[1]);
    DrawCreaturePortrait(duel_creature_tier_or_type, 0xa0, 0x20, 1, 2);

    if (IsWizardColorFeminine(duel_wizard_color) != 0)
    {
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[2], gs_wizardnames_0077ee70[duel_wizard_color]);
    }
    else
    {
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[3], gs_wizardnames_0077ee70[duel_wizard_color]);
    }

    if (IsCreatureTypeFeminine(duel_creature_tier_or_type) != 0)
    {
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_visit_0077c4f0[4], GetCreatureName(duel_creature_tier_or_type));
      strcat(g_ui_message_buffer, gs_visit_0077c4f0[6]);
    }
    else
    {
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_visit_0077c4f0[5], GetCreatureName(duel_creature_tier_or_type));
      strcat(g_ui_message_buffer, gs_visit_0077c4f0[7]);
    }

    if (RunTextMenuAtScaled(g_ui_message_buffer, 0x78, 0x38) == 1)
    {
      g_opponent_starting_card_id_2 = FindCardIndexByCsvid(
          *(&g_dungeon_monster_duel_music_csvids[(duel_wizard_color - 1) * 3] + internal_rand(3)));
      g_lair_or_monster_slots[6].color = duel_wizard_color;
      g_lair_or_monster_slots[6].entry_type = (ShandalarEntryType)duel_creature_tier_or_type;

      selected_card_id = FindCardIndexByCsvid(g_shandalar_monster_definitions[duel_creature_tier_or_type].deck_number);
      LoadCreatureDuelDeck(duel_creature_tier_or_type, (unsigned int)selected_card_id, 0, -1);

      g_current_encounter_color = duel_wizard_color;
      g_encounter_opening_hand_size_modifier = 0;
      g_current_encounter_strength = 3;

      duel_menu_result = RunDuelEngine((unsigned int)selected_card_id, duel_creature_tier_or_type);
      if (duel_menu_result == 1)
      {
        PlayDuelMusic(1);
        g_town_slots[town_index].status_and_ruling_wizard &= ~0xff00;

        LoadPcxIntoPageOpaque(1, "celeb.pic");
        StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);

        sprintf(g_ui_message_buffer, gs_visit_0077c4f0[8], BuildTownDisplayName(town_index));
        AddCardToDeckSorted((unsigned int)global_ante_cards[1][0]);
        RunTextMenuAtScaled(g_ui_message_buffer, 0x14, 0x14);
        AddJournalEntry(JOURNAL_ENTRY_CITY_SAVED, town_index);
      }
      else
      {
        PlayDuelMusic(2);
        for (duel_ante_slot = 0; duel_ante_slot < 3; duel_ante_slot = duel_ante_slot + 1)
        {
          ante_card_id = global_ante_cards[0][duel_ante_slot];
          if (ante_card_id == -1)
          {
            continue;
          }

          LoadPcxIntoPageOpaque(1, "losedul2.pic");
          StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);
          DrawAdventureCard(ante_card_id, 0x17, 0x50, 1, gs_visit_0077c4f0[9]);
          ClearInputAndWaitForMouseRelease();
          WaitForInputEventUnlessBlocked();
          RemoveCardFromDeckById((unsigned int)ante_card_id);
        }
      }
    }

    EnsureAdvfac64Loaded(0);
    sound_stop(0x10);
    DestroyCachedCardArt();
    return 0;
  }

  // non-duel
  if (g_town_slots[town_index].location_type == 4)
  {
    pics[0] = "0246.pic";
    pics[1] = "0364.pic";
    pics[2] = "0335.pic";
    pics[3] = "0737.pic";
    pics[4] = "0028.pic";

    g_current_encounter_color = (int)single_color_test_bit_to_color_t(GetWorldTileMagicMask(GetWorldTileType(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y)));

    ShowWizardVisitBackdrop(pics[g_current_encounter_color - 1]);
    g_page0_window_bounds->font_slot = 1;

    if (IsWizardColorFeminine(g_current_encounter_color))
    {
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[10], gs_wizardnames_0077ee70[g_current_encounter_color]);
    }
    else
    {
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[0xb], gs_wizardnames_0077ee70[g_current_encounter_color]);
    }

    strcat(g_ui_message_buffer, gs_visit_0077c4f0[0xc]);

    if (RunTextMenuAtScaled(g_ui_message_buffer, 0x2a, 0x1a) == 1)
    {
      sound_stop(0x10);
      EnterCastleDungeon(g_current_encounter_color - 1);
      if (((g_defeated_wizards_bitmap & (1 << ((unsigned char)g_current_encounter_color))) == 0) && (g_shandalar_difficulty == 3))
      {
        TriggerWizardSiegeNewsflash(g_current_encounter_color);
      }
    }

    DestroyCachedCardArt();
    return 0;
  }
  else if (g_town_slots[town_index].location_type == 5)
  {
    pics2[0] = "0246.pic";
    pics2[1] = "0364.pic";
    pics2[2] = "0335.pic";
    pics2[3] = "0737.pic";
    pics2[4] = "0028.pic";

    g_current_encounter_color = single_color_test_bit_to_color_t(GetWorldTileMagicMask(GetWorldTileType(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y)));

    ShowWizardVisitBackdrop(pics2[g_current_encounter_color - 1]);
    if (IsWizardColorFeminine(g_current_encounter_color))
    {
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[13], gs_wizardnames_0077ee70[g_current_encounter_color]);
    }
    else
    {
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[14], gs_wizardnames_0077ee70[g_current_encounter_color]);
    }
    strcat(g_ui_message_buffer, gs_visit_0077c4f0[0xf]);
    RunTextMenuAtScaled(g_ui_message_buffer, 0x2a, 0x1a);

    sound_stop(0x10);
    DestroyCachedCardArt();
    return 0;
  }

  if ((g_current_quest_destination == town_index) && ((g_current_quest_type >= 0) || (g_current_quest_type < -100)))
  {
    AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
    quest_handled = 0;
    AddJournalEntry(JOURNAL_ENTRY_SPELL_FOUND, g_current_quest_type);

    if ((g_current_quest_type == 0) || (g_current_quest_type == 2))
    {
      strcpy(g_ui_message_buffer, gs_visit_0077c4f0[0x10]);
      if (g_current_quest_type == 0)
      {
        strcat(g_ui_message_buffer, gs_visit_0077c4f0[0x11]);
        sound_unload(0xf);
        LoadSoundWithDriveFallback("x:sound\\manalink.wav", 0xf, 0);
        PlaySoundWithPitchAndPan(0xf, 100, 100, 0);
        g_town_slots[g_current_quest_destination].status_and_ruling_wizard =
            g_town_slots[g_current_quest_destination].status_and_ruling_wizard | 1;
      }
      else if (g_current_quest_type == 2)
      {
        sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_visit_0077c4f0[0x12], gs_amuletnames_0077d090[g_current_quest_color]);
        sound_unload(0xf);
        LoadSoundWithDriveFallback("x:sound\\reward.wav", 0xf, 0);
        PlaySoundWithPitchAndPan(0xf, 100, 100, 0);
        g_amulet_inventory[g_current_quest_color - 1] += 1;
      }

      g_page0_window_bounds->font_slot = 1;
      RunTextMenuAtScaled(g_ui_message_buffer, 0x50, 0x50);
      quest_handled = 1;
      g_current_quest_destination = -1;
      RefreshAdventureInterfaceLayout();
      DrawAdventureInterfaceLayout(1);
    }

    if ((g_current_quest_type == 1) &&
        FindDeckSlotForQuestColorAndType((unsigned char)g_current_quest_color,
                                         1 << (g_current_quest_destination & 3)))
    {
      found_deck_slot = FindDeckSlotForQuestColorAndType((unsigned char)g_current_quest_color,
                                                         (1 << (g_current_quest_destination & 3)));
      g_amulet_inventory[g_current_quest_color - 1] += 1;

      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[0x13], global_cards_data[deck[found_deck_slot - 1] & 0xfff].name);

      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_visit_0077c4f0[0x14], gs_amuletnames_0077d090[g_current_quest_color]);

      g_page0_window_bounds->font_slot = 1;
      g_text_menu_abort_requested = 1;
      RunTextMenuAtScaled(g_ui_message_buffer, 0x50, 0x50);

      g_town_slots[g_current_quest_destination].status_and_ruling_wizard =
          g_town_slots[g_current_quest_destination].status_and_ruling_wizard | 1;
      quest_handled = 1;
      RemoveDeckSlot(found_deck_slot - 1);
      WaitForInputEventUnlessBlocked();
      g_current_quest_destination = -1;
      RefreshAdventureInterfaceLayout();
      DrawAdventureInterfaceLayout(1);
    }

    if (g_current_quest_type < -100)
    {
      if (g_town_slots[g_current_quest_destination].location_type > 1)
      {
        first_card_browser_draw = 1;
        g_current_quest_type = g_current_quest_type + 100;
        deck_card_index = (int)g_shandalar_monster_definitions[-g_current_quest_type].plural[0x33] / 7 + 1;
        if (IsCreatureTypeFeminine(-g_current_quest_type))
        {
          sprintf(g_ui_message_buffer, gs_visit_0077c4f0[0x15], GetCreatureName(-g_current_quest_type));
        }
        else
        {
          sprintf(g_ui_message_buffer, gs_visit_0077c4f0[0x16], GetCreatureName(-g_current_quest_type));
        }

        sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_visit_0077c4f0[0x17], GetTownCardDescription(g_current_quest_destination));
        RunTextMenuAtScaled(g_ui_message_buffer, 0x50, 0x50);

        // TODO(decomp): This branch continues into the large "choose a card" UI (RunCardBrowser) and then
        // marks the selected card slot with 0x4000.
        g_town_dialog_callback = (TownDialogCallback)QuestCardChooserCallback;
        selected_card_id = -1;
        while (selected_card_id == -1)
        {
          selected_card_id = RunCardBrowser(gs_visit_0077c4f0[24], g_town_slots[town_index].trade_color_and_type & 0xff, (g_town_slots[town_index].trade_color_and_type >> 8) - 1, first_card_browser_draw, 0);
          first_card_browser_draw = 0;
          if (selected_card_id != -1)
          {
            strcpy(g_ui_message_buffer, gs_visit_0077c4f0[25]);
          retry:
            confirm_choice = RunTextMenuAt(g_ui_message_buffer, ScaleUiCoordinate(0xf4) + 10, ScaleUiCoordinate(0x15c) + 10);
            if (confirm_choice < 0)
            {
              goto retry;
            }

            if (confirm_choice == 0)
            {
              found_deck_slot = AddCardToDeckSorted(selected_card_id);
              if (found_deck_slot != -1)
              {
                deck[found_deck_slot] |= 0x4000;
              }
            }
            else
            {
              selected_card_id = -1;
            }
            DelayUiTicks(0xf);
          }
        }
        g_town_dialog_callback = (TownDialogCallback)DrawTiledDialogBoxFrame;
      }
      else
      {
        g_current_quest_type = g_current_quest_type + 100;
        deck_card_index = (int)g_shandalar_monster_definitions[-g_current_quest_type].plural[0x33] / 7 + 1;
        if (IsCreatureTypeFeminine(-g_current_quest_type))
        {
          sprintf(g_ui_message_buffer, gs_visit_0077c4f0[26], GetCreatureName(-g_current_quest_type));
        }
        else
        {
          sprintf(g_ui_message_buffer, gs_visit_0077c4f0[27], GetCreatureName(-g_current_quest_type));
        }

        if (deck_card_index == 1)
        {
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_visit_0077c4f0[0x12], gs_amuletnames_0077d090[g_current_quest_color]);
        }
        else
        {
          FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_visit_0077c4f0[0x1c], deck_card_index, gs_amuletnames_plural_0077edd0[g_current_quest_color]);
        }

        RunTextMenuAtScaled(g_ui_message_buffer, 0x50, 0x50);
        g_amulet_inventory[g_current_quest_color - 1] += deck_card_index;
        g_town_slots[g_current_quest_destination].status_and_ruling_wizard =
            g_town_slots[g_current_quest_destination].status_and_ruling_wizard | 1;
      }

      quest_handled = 1;
      if (g_last_town_services_town_index == g_current_quest_destination)
      {
        g_last_town_services_town_index = -1;
      }
      g_current_quest_destination = -1;
      RefreshAdventureInterfaceLayout();
      DrawAdventureInterfaceLayout(1);
    }

    if (quest_handled != 0)
    {
      journal_param = (unsigned int)town_index | 0x80;
    }
    else
    {
      journal_param = (unsigned int)town_index;
    }
    AddJournalEntry(JOURNAL_ENTRY_TOWN, (int)journal_param);
  }
  else
  {
    AddJournalEntry(JOURNAL_ENTRY_TOWN, town_index);
  }

  g_town_slots[town_index].status_and_ruling_wizard = g_town_slots[town_index].status_and_ruling_wizard | 2;
  g_town_slots[town_index].visit_count = g_town_slots[town_index].visit_count + 1;

  AnimateVisitBackdropZoomIn(GET_TOWN_PIC(town_index));
  // TODO(decomp): Town services UI (shops/healer/clues/map/stats/etc.).
  // This is currently not decompiled in-tree; keep the call to preserve control flow.
  RunTownServicesMenu(town_index);

  g_town_slots[town_index].last_visit_timer = g_quest_restock_timer;

  if ((g_town_slots[town_index].location_type == 3) || (g_town_slots[town_index].location_type == 2))
  {
    distance = -1;
    sprite = (EncodedImage *)999;
    for (world_magic_slot_index = 0; world_magic_slot_index < 0xc; world_magic_slot_index = world_magic_slot_index + 1)
    {
      if (g_world_magic_slot_timers[world_magic_slot_index].town_index == 0)
      {
        continue;
      }

      world_magic_distance = ApproximateDistance(g_town_slots[town_index].world_x - g_town_slots[g_world_magic_slot_timers[world_magic_slot_index].town_index].world_x,
                                                 g_town_slots[town_index].world_y - g_town_slots[g_world_magic_slot_timers[world_magic_slot_index].town_index].world_y);
      if (world_magic_distance < (int)sprite)
      {
        sprite = (EncodedImage *)world_magic_distance;
        distance = world_magic_slot_index;
      }
    }

    if ((distance != -1) && ((g_world_magic_bitmap & (1 << ((unsigned char)distance & 0xff))) == 0))
    {
      amulet_name = 0x14f;
      msg_len = 0x19e;
      tile_color_index = 0x4c;
      menu_result = 0x9a;

      LoadPcxIntoPage(1, "worlbak1.pic");
      StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);

      found_deck_slot = FindWorldMagicCardIndex(distance);
      world_magic_text_y = 0x28;
      world_magic_text_y += 0x10;

      world_magic_button_sprite = g_world_magic_choice_button_sprite_bank.named.normal[distance];

      world_magic_text_y = ((menu_result - tile_color_index) - world_magic_button_sprite->height) / 2 + tile_color_index;
      world_magic_center_x =
          (((int)msg_len - amulet_name) - world_magic_button_sprite->width) / 2 + amulet_name;

      DrawEncodedImageResampled(g_page0_window_bounds, ScaleUiCoordinate(world_magic_center_x),
                                ScaleUiCoordinate(world_magic_text_y), ScaleUiCoordinate(world_magic_button_sprite->width),
                                ScaleUiCoordinate(world_magic_button_sprite->height), g_world_magic_choice_button_sprite_bank.named.normal[distance]);

      world_magic_text_y += world_magic_button_sprite->height + 0x10;
      world_magic_text_y /= 2;
      world_magic_center_x += world_magic_button_sprite->width / 2;
      world_magic_center_x /= 2;

      g_page0_window_bounds->font_slot = 4;
      DrawTextAt(g_page0_window_bounds, 0x7b, 0x176, 0x4b, gs_visit_0077c4f0[0x1d]);

      world_magic_text_y = world_magic_text_y - 5;
      strcpy(g_ui_message_buffer, gs_worldmagic_names_00780660[distance]);
      DrawUiScaledCenteredText(g_ui_message_buffer, world_magic_center_x, world_magic_text_y, 0x40);

      world_magic_text_y = world_magic_text_y + 8;
      sprintf(g_ui_message_buffer, gs_visit_0077c4f0[0x1e], Scards[distance].worldmagic_price / 2);
      DrawUiScaledCenteredText(g_ui_message_buffer, world_magic_center_x, world_magic_text_y, 0x7b);

      world_magic_text_y = world_magic_text_y + 8;
      strcpy(g_ui_message_buffer, gs_worldmagic_explains_0074b8f0[distance]);
      DrawWorldUiFormattedText(g_page0_window_bounds, 0x7b, ScaleUiCoordinateFrom320(world_magic_center_x), ScaleUiCoordinateFrom320(world_magic_text_y), g_ui_message_buffer);

      world_magic_text_y += 0x10;

      /* Matching: the original early-outs for "this town doesn't sell it" (distance != 0) via a jump. */
      if (sprite)
      {
        // These are calcualted but unused
        world_magic_delta_x = g_town_slots[g_world_magic_slot_timers[distance].town_index].world_x - g_town_slots[town_index].world_x;
        world_magic_delta_y = g_town_slots[g_world_magic_slot_timers[distance].town_index].world_y - g_town_slots[town_index].world_y;

        /* Message: where to find it (direction + town name). */
        world_quadrant = GetRelativeWorldQuadrant(g_town_slots[g_world_magic_slot_timers[distance].town_index].world_x,
                                                  g_town_slots[g_world_magic_slot_timers[distance].town_index].world_y);

        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_visit_0077c4f0[0x1f], gs_directions_00765d50[world_quadrant], BuildTownDisplayName(g_world_magic_slot_timers[distance].town_index));

        world_magic_text_y = world_magic_text_y + 8;
        DrawUiScaledCenteredText(g_ui_message_buffer, world_magic_center_x, world_magic_text_y, 0x8d);
        world_magic_text_y = world_magic_text_y + 8;
        g_world_magic_offer_slot_index = distance;
        ClearInputAndWaitForMouseRelease();
        WaitForInputEventUnlessBlocked();
      }
      else if ((Scards[distance].worldmagic_price / 2) <= Gold)
      {
        /* Purchase prompt. */
        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_visit_0077c4f0[0x20], gs_worldmagic_names_00780660[distance],
                                                    BuildTownDisplayName(g_world_magic_slot_timers[distance].town_index), Scards[distance].worldmagic_price / 2);
        strcat(g_ui_message_buffer, gs_visit_0077c4f0[0x21]);

        ClearInputAndWaitForMouseRelease();
        found_deck_slot = RunTextMenuAtScaled(
            g_ui_message_buffer,
            ((((unsigned int)(global_screen_width - 0x280) < 1) ? 0xffffffce : 0) + 0xbe),
            0x88);
        g_world_magic_offer_slot_index = distance;

        if (found_deck_slot == 1)
        {
          Gold = Gold - Scards[distance].worldmagic_price / 2;
          g_world_magic_bitmap = g_world_magic_bitmap | (1 << (unsigned char)g_world_magic_offer_slot_index);
          g_world_magic_slot_timers[g_world_magic_offer_slot_index].town_index = 0;
          AddJournalEntry(JOURNAL_ENTRY_WORLD_MAGIC_LEARNED, distance);
        }
        g_world_magic_offer_slot_index = -1;
      }
      else
      {
        /* Not enough gold. */
        world_magic_text_y = world_magic_text_y + 8;
        strcpy(g_ui_message_buffer, gs_visit_0077c4f0[0x22]);
        DrawUiScaledCenteredText(g_ui_message_buffer, world_magic_center_x, world_magic_text_y, 0xbe);
        world_magic_text_y = world_magic_text_y + 8;
        ClearInputAndWaitForMouseRelease();
        WaitForInputEventUnlessBlocked();
      }
    }
  }

  DestroyCachedCardArt();

  if (g_current_quest_deadline < g_quest_restock_timer)
  {
    g_current_quest_destination = -1;
  }
}
