#pragma once

#include "config.h"

#ifdef DISP_BW
#include <GxEPD2_BW.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_750_T7
#define MAX_HEIGHT(EPD) (EPD::HEIGHT)
#endif
#ifdef DISP_3C
#define DISP_WIDTH 800
#define DISP_HEIGHT 480
#include <GxEPD2_3C.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_3C
#define GxEPD2_DRIVER_CLASS GxEPD2_750c_Z08
#define MAX_HEIGHT(EPD) (EPD::HEIGHT / 2)
#endif
#ifdef DISP_7C
#define DISP_WIDTH 800
#define DISP_HEIGHT 480
#include <GxEPD2_7C.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_7C
#define GxEPD2_DRIVER_CLASS GxEPD2_730c_GDEY073D46
#define MAX_HEIGHT(EPD) (EPD::HEIGHT / 4)
#endif

// Define the Alignment enum with bit flags
enum Alignment : uint8_t
{
    Left = 1 << 0,   // 0001
    Right = 1 << 1,  // 0010
    Top = 1 << 2,    // 0100
    Bottom = 1 << 3, // 1000

    HorizontalCenter = Left | Right,           // 0011
    VerticalCenter = Top | Bottom,             // 1100
    Center = HorizontalCenter | VerticalCenter // 1111
};

enum Color : uint16_t
{
    Black = GxEPD_BLACK,
    White = GxEPD_WHITE,

#if defined(DISP_3C) || defined(DISP_7C)
    Red = GxEPD_RED,
#endif

#if defined(DISP_7C)
    Green = GxEPD_GREEN,
    Blue = GxEPD_BLUE,
    Yellow = GxEPD_YELLOW,
    Orange = GxEPD_ORANGE,
#endif

    // some controllers for b/w EPDs support grey levels
    DarkGrey = GxEPD_DARKGREY,
    LightGrey = GxEPD_LIGHTGREY
};

struct TextSize
{
    uint16_t width;
    uint16_t height;
};

struct Rect : public TextSize
{
    uint16_t x;
    uint16_t y;
};