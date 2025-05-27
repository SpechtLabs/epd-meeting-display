#pragma once

#include <string>
#include "display_buffer.h"
#include "components/display_config.h"

class IDisplayComponent
{
public:
    virtual ~IDisplayComponent() = default;
    virtual void render(time_t now) const = 0;
    virtual bool hasToRedraw(time_t now) const = 0;
};

class DisplayComponent : public IDisplayComponent
{
protected:
    DisplayBuffer *buffer;
    int x;
    int y;
    int width;
    int height;

#if defined(DISP_3C) || defined(DISP_7C)
    Color accentColor;
#endif

public:
#if defined(DISP_3C) || defined(DISP_7C)
    DisplayComponent(DisplayBuffer *buffer, int offsetX, int offsetY, int width, int height, Color accentColor) : buffer(buffer), x(offsetX), y(offsetY), width(width), height(height), accentColor(accentColor) {}
#else
    DisplayComponent(DisplayBuffer *buffer, int offsetX, int offsetY, int width, int height) : buffer(buffer), x(offsetX), y(offsetY), width(width), height(height) {}
#endif
    virtual void render(time_t now) const override;
    virtual bool hasToRedraw(time_t now) const override;
};
