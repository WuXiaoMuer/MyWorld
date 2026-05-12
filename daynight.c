#include "types.h"

void InitDayNight(void)
{
    dayNight.timeOfDay = 0.35f;
    dayNight.daySpeed = 0.008f;
    dayNight.lightLevel = 1.0f;
}

void UpdateDayNight(float dt)
{
    dayNight.timeOfDay += dayNight.daySpeed * dt;
    if (dayNight.timeOfDay >= 1.0f) dayNight.timeOfDay -= 1.0f;

    float t = dayNight.timeOfDay;
    if (t < 0.2f) dayNight.lightLevel = 0.2f;
    else if (t < 0.3f) dayNight.lightLevel = 0.2f + (t - 0.2f) / 0.1f * 0.8f;
    else if (t < 0.7f) dayNight.lightLevel = 1.0f;
    else if (t < 0.8f) dayNight.lightLevel = 1.0f - (t - 0.7f) / 0.1f * 0.8f;
    else dayNight.lightLevel = 0.2f;
}

Color GetSkyColor(void)
{
    float l = dayNight.lightLevel;
    unsigned char r = (unsigned char)(10 + l * 125);
    unsigned char g = (unsigned char)(10 + l * 196);
    unsigned char b = (unsigned char)(40 + l * 195);
    return (Color){r, g, b, 255};
}
