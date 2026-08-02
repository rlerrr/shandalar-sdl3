#include <windows.h>
#include <commctrl.h>

#include "magic/src/duel_engine.h"

typedef struct {
  unsigned char type;
  short id;
  short x, y, width, height;
  DWORD style;
} ControlSpec;

enum { CT_STATIC, CT_BUTTON, CT_EDIT, CT_LIST, CT_ICON, CT_FULLCARD,
       CT_BIGCHOICE, CT_UPDOWN };

#define C(t, id, x, y, w, h, s) {t, id, x, y, w, h, s}
#define RADIO 9
#define GROUP 7
#define CHECK BS_AUTOCHECKBOX
#define OWNER BS_OWNERDRAW
#define SX(value) ((value) * 5 / 2)
#define SY(value) ((value) * 19 / 8)

static const char *control_class(unsigned char type)
{
  static const char *classes[] = {
      "STATIC", "BUTTON", "EDIT", "LISTBOX",
      CLASS_MAGICTHEME_ICON_BUTTON, CLASS_MAGICGAME_FULL_CARD,
      CLASS_MAGICGAME_BIG_CARD_CHOICE, "msctls_updown32"};
  return classes[type];
}

static const char *control_text(int id)
{
  switch (id) {
  case IDOK: return "OK";
  case IDCANCEL: return "Cancel";
  case 1060: return "Duel Options";
  case 1061: return "Layout";
  case 1062: return "Standard";
  case 1063: return "Advanced";
  case 1065: return "Show cue cards";
  case 1066: return "Show power/toughness on small cards";
  case 1067: return "Show abilities on small cards";
  case 1069: return "White";
  case 1070: return "Blue";
  case 1071: return "Black";
  case 1072: return "Red";
  case 1073: return "Green";
  case 1074: return "Deck color";
  case 1075: return "Your territory background";
  case 1076: return "Line drawing";
  case 1077: return "Pattern";
  case 1078: return "Mana symbols";
  case 1079: return "See next draws at end of duel";
  case 1080: return "Show coin flip animations";
  case 1092: return "Take a mulligan";
  case 1171: return "Change word:";
  case 1172: return "To:";
  case 1221: return "Play first";
  case 1222: return "Draw first";
  case 1255: return "Next round";
  case 1256: return "Quit Gauntlet";
  default: return "";
  }
}

static HWND make_dialog(HINSTANCE instance, HWND parent, const char *title,
                        int width, int height, DWORD style,
                        const ControlSpec *specs, size_t count)
{
  RECT desktop;
  HWND dialog;
  size_t i;
  int pixel_width = SX(width);
  int pixel_height = SY(height) +
                     (((style & WS_CAPTION) == WS_CAPTION) ?
                          GetSystemMetrics(SM_CYCAPTION) : 0);
  GetClientRect(GetDesktopWindow(), &desktop);
  dialog = shandalar_win32_create_dialog(instance, parent, title,
                                         (desktop.right - pixel_width) / 2,
                                         (desktop.bottom - pixel_height) / 2,
                                         pixel_width, pixel_height);
  if (dialog)
    SetWindowLongA(dialog, GWL_STYLE, style);
  for (i = 0; dialog && i < count; ++i) {
    const ControlSpec *s = &specs[i];
    const char *text = control_text(s->id);
    if (s->type == CT_BIGCHOICE && text[0] == '\0')
      text = NULL;
    CreateWindowExA(0, control_class(s->type), text,
                    WS_CHILD | WS_VISIBLE | s->style, SX(s->x),
                    SY(s->y), SX(s->width),
                    SY(s->height), dialog, (HMENU)(intptr_t)s->id, instance, NULL);
  }
  return dialog;
}

static HWND create_duel_dialog(HINSTANCE instance, LPCSTR template_name,
                               HWND parent)
{
  UINT id = (UINT)(uintptr_t)template_name & 0xffffu;
  const ControlSpec *specs = NULL;
  size_t count = 0;
  int width = 180, height = 120;
  const char *title = "";
  DWORD style = 0x80800840;
  static const ControlSpec d220[] = {
      C(CT_STATIC,1101,27,11,113,56,0), C(CT_EDIT,1100,64,73,40,12,WS_BORDER),
      C(CT_BUTTON,IDOK,27,95,50,14,OWNER), C(CT_BUTTON,IDCANCEL,88,95,50,14,OWNER)};
  static const ControlSpec d222[] = {
      C(CT_ICON,1120,85,33,20,22,0), C(CT_ICON,1122,136,73,20,22,0),
      C(CT_ICON,1124,111,124,20,22,0), C(CT_ICON,1123,58,124,20,22,0),
      C(CT_ICON,1121,33,73,20,22,0), C(CT_ICON,1125,86,80,20,22,0),
      C(CT_STATIC,1126,76,57,39,8,0), C(CT_STATIC,1127,126,97,39,8,0),
      C(CT_STATIC,1128,101,148,39,8,0), C(CT_STATIC,1129,48,148,39,8,0),
      C(CT_STATIC,1130,23,97,39,8,0), C(CT_STATIC,1131,73,104,47,8,0),
      C(CT_STATIC,1133,13,7,165,8,0), C(CT_BUTTON,IDOK,40,182,46,14,OWNER),
      C(CT_BUTTON,IDCANCEL,103,182,46,14,OWNER)};
  static const ControlSpec d223[] = {C(CT_BIGCHOICE,1021,111,7,102,98,0)};
  static const ControlSpec d224[] = {
      C(CT_FULLCARD,1030,7,7,182,244,0), C(CT_BUTTON,-1,206,7,45,79,GROUP),
      C(CT_BUTTON,1032,210,17,34,10,RADIO|WS_GROUP), C(CT_BUTTON,1033,210,28,30,10,RADIO),
      C(CT_BUTTON,1034,210,39,35,10,RADIO), C(CT_BUTTON,1035,210,50,29,10,RADIO),
      C(CT_BUTTON,1036,210,61,35,10,RADIO), C(CT_BUTTON,1864,210,72,35,10,RADIO),
      C(CT_BUTTON,-1,261,7,94,79,GROUP), C(CT_BUTTON,1037,266,17,46,10,RADIO|WS_GROUP),
      C(CT_BUTTON,1038,266,28,41,10,RADIO), C(CT_BUTTON,1039,266,39,61,10,RADIO),
      C(CT_BUTTON,1040,266,50,45,10,RADIO), C(CT_BUTTON,1041,266,61,86,10,RADIO),
      C(CT_BUTTON,1042,266,72,32,10,RADIO), C(CT_LIST,1031,206,90,143,137,WS_BORDER|WS_VSCROLL|LBS_STANDARD),
      C(CT_BUTTON,IDOK,214,237,50,14,BS_DEFPUSHBUTTON), C(CT_BUTTON,IDCANCEL,291,237,50,14,0)};
  static const ControlSpec d225[] = {
      C(CT_STATIC,1060,7,11,166,8,0), C(CT_BUTTON,1061,7,25,166,37,GROUP),
      C(CT_BUTTON,1062,16,35,157,10,RADIO|WS_GROUP), C(CT_BUTTON,1063,16,45,157,10,RADIO),
      C(CT_BUTTON,1080,7,66,166,10,CHECK), C(CT_BUTTON,1065,7,78,166,9,CHECK),
      C(CT_BUTTON,1067,7,89,166,9,CHECK), C(CT_BUTTON,1066,7,100,166,9,CHECK),
      C(CT_BUTTON,1079,7,111,166,9,CHECK), C(CT_BUTTON,1075,7,125,166,78,GROUP),
      C(CT_BUTTON,1069,16,138,43,10,RADIO|WS_GROUP), C(CT_BUTTON,1070,16,148,46,10,RADIO),
      C(CT_BUTTON,1071,16,158,45,10,RADIO), C(CT_BUTTON,1072,16,169,44,10,RADIO),
      C(CT_BUTTON,1073,16,178,48,10,RADIO), C(CT_BUTTON,1074,16,188,108,10,RADIO),
      C(CT_BUTTON,1076,80,140,85,10,RADIO|WS_GROUP), C(CT_BUTTON,1077,80,151,85,10,RADIO),
      C(CT_BUTTON,1078,80,162,85,10,RADIO), C(CT_BUTTON,IDOK,20,212,50,14,OWNER),
      C(CT_BUTTON,IDCANCEL,92,212,50,14,OWNER)};
  static const ControlSpec d227[] = {
      C(CT_STATIC,1090,7,7,172,8,0), C(CT_STATIC,1095,7,32,69,8,0),
      C(CT_STATIC,1097,7,45,73,114,0), C(CT_STATIC,1094,93,32,95,8,0),
      C(CT_STATIC,1096,93,45,73,114,0), C(CT_STATIC,1091,203,7,99,26,SS_CENTER),
      C(CT_BUTTON,1092,211,52,73,20,OWNER), C(CT_STATIC,1093,209,86,86,28,0),
      C(CT_BUTTON,IDOK,210,164,73,20,OWNER)};
  static const ControlSpec d234[] = {
      C(CT_BUTTON,1171,113,21,66,88,GROUP), C(CT_BUTTON,1172,186,21,66,88,GROUP),
      C(CT_BUTTON,1173,121,36,55,10,RADIO|WS_GROUP), C(CT_BUTTON,1174,121,50,55,10,RADIO),
      C(CT_BUTTON,1175,121,64,55,10,RADIO), C(CT_BUTTON,1176,121,78,55,10,RADIO),
      C(CT_BUTTON,1177,121,92,55,10,RADIO), C(CT_BUTTON,1178,192,36,55,10,RADIO|WS_GROUP),
      C(CT_BUTTON,1179,192,50,55,10,RADIO), C(CT_BUTTON,1180,192,64,55,10,RADIO),
      C(CT_BUTTON,1181,192,78,55,10,RADIO), C(CT_BUTTON,1182,192,92,55,10,RADIO),
      C(CT_STATIC,1170,7,13,92,134,0), C(CT_BUTTON,IDOK,118,133,50,14,OWNER),
      C(CT_BUTTON,IDCANCEL,179,133,50,14,OWNER)};
  static const ControlSpec d235[] = {
      C(CT_STATIC,1201,7,6,118,8,0), C(CT_STATIC,1203,7,20,54,77,0),
      C(CT_STATIC,1202,7,105,118,8,0), C(CT_STATIC,1200,7,119,54,77,0)};
  static const ControlSpec d239[] = {
      C(CT_STATIC,1240,120,47,62,19,0), C(CT_EDIT,1232,185,48,40,14,WS_BORDER),
      C(CT_UPDOWN,1231,223,48,8,14,0), C(CT_STATIC,1233,237,51,37,8,0),
      C(CT_STATIC,1241,143,67,24,8,0), C(CT_STATIC,1238,173,66,25,11,WS_BORDER),
      C(CT_STATIC,1242,143,79,93,8,0), C(CT_STATIC,1239,239,78,25,11,WS_BORDER),
      C(CT_STATIC,1243,120,101,42,8,0), C(CT_EDIT,1234,164,98,40,14,WS_BORDER),
      C(CT_UPDOWN,1235,204,98,9,14,0), C(CT_STATIC,1236,218,101,37,8,0),
      C(CT_STATIC,1244,120,125,89,23,0), C(CT_STATIC,1237,212,127,25,11,WS_BORDER),
      C(CT_BUTTON,IDOK,130,155,50,14,OWNER), C(CT_BUTTON,IDCANCEL,192,155,50,14,OWNER),
      C(CT_STATIC,1230,11,22,103,172,0)};
  static const ControlSpec d240[] = {
      C(CT_STATIC,1051,7,7,137,9,0), C(CT_STATIC,1052,7,31,137,8,0)};
  static const ControlSpec d243[] = {
      C(CT_STATIC,1210,7,36,110,20,0), C(CT_STATIC,1211,7,7,110,8,0)};
  static const ControlSpec d244[] = {
      C(CT_STATIC,1220,6,18,104,22,0), C(CT_BUTTON,1221,33,76,50,14,OWNER),
      C(CT_BUTTON,1222,33,101,50,14,OWNER), C(CT_STATIC,1223,6,43,104,28,0)};
  static const ControlSpec d246[] = {
      C(CT_STATIC,1251,68,15,58,17,0), C(CT_STATIC,1253,77,37,48,71,0),
      C(CT_STATIC,1250,59,113,66,8,0), C(CT_STATIC,1254,77,125,48,71,0),
      C(CT_STATIC,1252,6,5,59,155,0), C(CT_BUTTON,1255,8,167,50,14,OWNER),
      C(CT_BUTTON,1256,8,184,50,14,OWNER)};
  static const ControlSpec d249[] = {C(CT_STATIC,1026,1,1,62,44,0)};

#define D(n,w,h) do { specs=d##n; count=sizeof(d##n)/sizeof(d##n[0]); width=w; height=h; } while (0)
  switch (id) {
  case 220: D(220,186,116); break; case 222: D(222,193,206); break;
  case 223: D(223,220,169); break; case 224: D(224,363,258); title="Browse (pick a card)"; break;
  case 225: D(225,180,233); break; case 227: D(227,316,206); break;
  case 233: width=181; height=79; title="<title>"; break;
  case 234: D(234,264,162); break; case 235: D(235,132,207); break;
  case 239: D(239,283,218); break; case 240: D(240,151,124); break;
  case 243: D(243,124,61); break; case 244: D(244,117,150); break;
  case 246: D(246,132,207); break; case 249: D(249,64,53); break;
  default: return NULL;
  }
#undef D
  return make_dialog(instance, parent, title, width, height, style, specs, count);
}

void duel_register_dialog_resources(void)
{
  static const UINT ids[] = {220,222,223,224,225,227,233,234,235,239,240,243,244,246,249};
  size_t i;
  for (i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
    shandalar_win32_register_dialog_factory(ids[i], create_duel_dialog);
}
