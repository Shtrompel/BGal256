#ifndef _GLOBALS
#define _GLOBALS

typedef int t_clothtype;
constexpr t_clothtype CLOTHTYPE_HEAD = 0;
constexpr t_clothtype CLOTHTYPE_SHIRT = 1;
constexpr t_clothtype CLOTHTYPE_PANTS = 2;
constexpr t_clothtype CLOTHTYPE_SHOES = 3;
constexpr int CLOTHTYPE_COUNT = 4;

// Colors
static const int COLOR_PRIMARY_INPUT[3] = { 56, 85, 116 };   // #385574
static const int COLOR_PRIMARY_OUTPUT[3] = { 232, 108, 86 }; // #e86c56
static const int COLOR_BACKGROUND_LIGHT[3] = { 243, 240, 226 }; // #f3f0e2
static const int COLOR_ACCENT_RED[3] = { 209, 58, 82 };         // #d13a52
static const int COLOR_ACCENT_YELLOW[3] = { 250, 218, 71 };     // #fada47
static const int COLOR_ACCENT_BLUE[3] = { 25, 25, 225 };     // #fada47

#endif // _GLOBALS