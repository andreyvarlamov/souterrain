#ifndef VA_COLORS_H
#define VA_COLORS_H

#include "va_types.h"

// NOTE: With little endian, 4 byte RGBA is 4 bytes in this order: A B G R
// This way an array of colors, can be read as a number of u32s correctly. And vice versa.
union color
{
    struct
    {
        u8 A;
        u8 B;
        u8 G;
        u8 R;
    };
    /* u8 D[4]; Maybe don't want this because order is confusing */
    u32 C32;
};

inline color
Color(u8 R, u8 G, u8 B, u8 A)
{
    color Color;
    Color.R = R;
    Color.G = G;
    Color.B = B;
    Color.A = A;
    return Color;
}

inline color
Color(u8 R, u8 G, u8 B)
{
    return Color(R, G, B, 0xFF);
}

inline color
Color(u32 Color32)
{
    color Color;
    Color.C32 = Color32;
    return Color;
}

inline color
ColorAlpha(color Color, u8 A)
{
    Color.A = A;
    return Color;
}

// NOTE: https://www.w3.org/wiki/CSS3/Color/Extended_color_keywords
#define VA_ALICEBLUE Color(0xF0F8FFFF)
#define VA_ANTIQUEWHITE Color(0xFAEBD7FF)
#define VA_AQUA Color(0x00FFFFFF)
#define VA_AQUAMARINE Color(0x7FFFD4FF)
#define VA_AZURE Color(0xF0FFFFFF)
#define VA_BEIGE Color(0xF5F5DCFF)
#define VA_BISQUE Color(0xFFE4C4FF)
#define VA_BLACK Color(0x000000FF)
#define VA_BLANCHEDALMOND Color(0xFFEBCDFF)
#define VA_BLUE Color(0x0000FFFF)
#define VA_BLUEVIOLET Color(0x8A2BE2FF)
#define VA_BROWN Color(0xA52A2AFF)
#define VA_BURLYWOOD Color(0xDEB887FF)
#define VA_CADETBLUE Color(0x5F9EA0FF)
#define VA_CHARTREUSE Color(0x7FFF00FF)
#define VA_CHOCOLATE Color(0xD2691EFF)
#define VA_CORAL Color(0xFF7F50FF)
#define VA_CORNFLOWERBLUE Color(0x6495EDFF)
#define VA_CORNSILK Color(0xFFF8DCFF)
#define VA_CRIMSON Color(0xDC143CFF)
#define VA_CYAN Color(0x00FFFFFF)
#define VA_DARKBLUE Color(0x00008BFF)
#define VA_DARKCYAN Color(0x008B8BFF)
#define VA_DARKGOLDENROD Color(0xB8860BFF)
#define VA_DARKGRAY Color(0xA9A9A9FF)
#define VA_DARKGREEN Color(0x006400FF)
#define VA_DARKGREY Color(0xA9A9A9FF)
#define VA_DARKKHAKI Color(0xBDB76BFF)
#define VA_DARKMAGENTA Color(0x8B008BFF)
#define VA_DARKOLIVEGREEN Color(0x556B2FFF)
#define VA_DARKORANGE Color(0xFF8C00FF)
#define VA_DARKORCHID Color(0x9932CCFF)
#define VA_DARKRED Color(0x8B0000FF)
#define VA_DARKSALMON Color(0xE9967AFF)
#define VA_DARKSEAGREEN Color(0x8FBC8FFF)
#define VA_DARKSLATEBLUE Color(0x483D8BFF)
#define VA_DARKSLATEGRAY Color(0x2F4F4FFF)
#define VA_DARKSLATEGREY Color(0x2F4F4FFF)
#define VA_DARKTURQUOISE Color(0x00CED1FF)
#define VA_DARKVIOLET Color(0x9400D3FF)
#define VA_DEEPPINK Color(0xFF1493FF)
#define VA_DEEPSKYBLUE Color(0x00BFFFFF)
#define VA_DIMGRAY Color(0x696969FF)
#define VA_DIMGREY Color(0x696969FF)
#define VA_DODGERBLUE Color(0x1E90FFFF)
#define VA_FIREBRICK Color(0xB22222FF)
#define VA_FLORALWHITE Color(0xFFFAF0FF)
#define VA_FORESTGREEN Color(0x228B22FF)
#define VA_FUCHSIA Color(0xFF00FFFF)
#define VA_GAINSBORO Color(0xDCDCDCFF)
#define VA_GHOSTWHITE Color(0xF8F8FFFF)
#define VA_GOLD Color(0xFFD700FF)
#define VA_GOLDENROD Color(0xDAA520FF)
#define VA_GRAY Color(0x808080FF)
#define VA_GREEN Color(0x008000FF)
#define VA_GREENYELLOW Color(0xADFF2FFF)
#define VA_GREY Color(0x808080FF)
#define VA_HONEYDEW Color(0xF0FFF0FF)
#define VA_HOTPINK Color(0xFF69B4FF)
#define VA_INDIANRED Color(0xCD5C5CFF)
#define VA_INDIGO Color(0x4B0082FF)
#define VA_IVORY Color(0xFFFFF0FF)
#define VA_KHAKI Color(0xF0E68CFF)
#define VA_LAVENDER Color(0xE6E6FAFF)
#define VA_LAVENDERBLUSH Color(0xFFF0F5FF)
#define VA_LAWNGREEN Color(0x7CFC00FF)
#define VA_LEMONCHIFFON Color(0xFFFACDFF)
#define VA_LIGHTBLUE Color(0xADD8E6FF)
#define VA_LIGHTCORAL Color(0xF08080FF)
#define VA_LIGHTCYAN Color(0xE0FFFFFF)
#define VA_LIGHTGOLDENRODYELLOW Color(0xFAFAD2FF)
#define VA_LIGHTGRAY Color(0xD3D3D3FF)
#define VA_LIGHTGREEN Color(0x90EE90FF)
#define VA_LIGHTGREY Color(0xD3D3D3FF)
#define VA_LIGHTPINK Color(0xFFB6C1FF)
#define VA_LIGHTSALMON Color(0xFFA07AFF)
#define VA_LIGHTSEAGREEN Color(0x20B2AAFF)
#define VA_LIGHTSKYBLUE Color(0x87CEFAFF)
#define VA_LIGHTSLATEGRAY Color(0x778899FF)
#define VA_LIGHTSLATEGREY Color(0x778899FF)
#define VA_LIGHTSTEELBLUE Color(0xB0C4DEFF)
#define VA_LIGHTYELLOW Color(0xFFFFE0FF)
#define VA_LIME Color(0x00FF00FF)
#define VA_LIMEGREEN Color(0x32CD32FF)
#define VA_LINEN Color(0xFAF0E6FF)
#define VA_MAGENTA Color(0xFF00FFFF)
#define VA_MAROON Color(0x800000FF)
#define VA_MEDIUMAQUAMARINE Color(0x66CDAAFF)
#define VA_MEDIUMBLUE Color(0x0000CDFF)
#define VA_MEDIUMORCHID Color(0xBA55D3FF)
#define VA_MEDIUMPURPLE Color(0x9370DBFF)
#define VA_MEDIUMSEAGREEN Color(0x3CB371FF)
#define VA_MEDIUMSLATEBLUE Color(0x7B68EEFF)
#define VA_MEDIUMSPRINGGREEN Color(0x00FA9AFF)
#define VA_MEDIUMTURQUOISE Color(0x48D1CCFF)
#define VA_MEDIUMVIOLETRED Color(0xC71585FF)
#define VA_MIDNIGHTBLUE Color(0x191970FF)
#define VA_MINTCREAM Color(0xF5FFFAFF)
#define VA_MISTYROSE Color(0xFFE4E1FF)
#define VA_MOCCASIN Color(0xFFE4B5FF)
#define VA_NAVAJOWHITE Color(0xFFDEADFF)
#define VA_NAVY Color(0x000080FF)
#define VA_OLDLACE Color(0xFDF5E6FF)
#define VA_OLIVE Color(0x808000FF)
#define VA_OLIVEDRAB Color(0x6B8E23FF)
#define VA_ORANGE Color(0xFFA500FF)
#define VA_ORANGERED Color(0xFF4500FF)
#define VA_ORCHID Color(0xDA70D6FF)
#define VA_PALEGOLDENROD Color(0xEEE8AAFF)
#define VA_PALEGREEN Color(0x98FD98FF)
#define VA_PALETURQUOISE Color(0xAFEEEEFF)
#define VA_PALEVIOLETRED Color(0xDB7093FF)
#define VA_PAPAYAWHIP Color(0xFFEFD5FF)
#define VA_PEACHPUFF Color(0xFFDAB9FF)
#define VA_PERU Color(0xCD853FFF)
#define VA_PINK Color(0xFFC0CDFF)
#define VA_PLUM Color(0xDDA0DDFF)
#define VA_POWDERBLUE Color(0xB0E0E6FF)
#define VA_PURPLE Color(0x800080FF)
#define VA_RED Color(0xFF0000FF)
#define VA_ROSYBROWN Color(0xBC8F8FFF)
#define VA_ROYALBLUE Color(0x4169E1FF)
#define VA_SADDLEBROWN Color(0x8B4513FF)
#define VA_SALMON Color(0xFA8072FF)
#define VA_SANDYBROWN Color(0xF4A460FF)
#define VA_SEAGREEN Color(0x2E8B57FF)
#define VA_SEASHELL Color(0xFFF5EEFF)
#define VA_SIENNA Color(0xA0522DFF)
#define VA_SILVER Color(0xC0C0C0FF)
#define VA_SKYBLUE Color(0x87CEEBFF)
#define VA_SLATEBLUE Color(0x6A5ACDFF)
#define VA_SLATEGRAY Color(0x708090FF)
#define VA_SLATEGREY Color(0x708090FF)
#define VA_SNOW Color(0xFFFAFAFF)
#define VA_SPRINGGREEN Color(0x00FF7FFF)
#define VA_STEELBLUE Color(0x4682B4FF)
#define VA_TAN Color(0xD2B48CFF)
#define VA_TEAL Color(0x008080FF)
#define VA_THISTLE Color(0xD8BFD8FF)
#define VA_TOMATO Color(0xFF6347FF)
#define VA_TURQUOISE Color(0x40E0D0FF)
#define VA_SADDLEBROWN Color(0x8B4513FF)
#define VA_VIOLET Color(0xEE82EEFF)
#define VA_WHEAT Color(0xF5DEB3FF)
#define VA_WHITE Color(0xFFFFFFFF)
#define VA_WHITESMOKE Color(0xF5F5F5FF)
#define VA_YELLOW Color(0xFFFF00FF)
#define VA_YELLOWGREEN Color(0x9ACD32FF)

#endif
