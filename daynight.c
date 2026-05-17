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

    // Apply weather light modifier
    float mod = GetWeatherLightModifier();
    dayNight.lightLevel += mod;
    if (dayNight.lightLevel < 0.05f) dayNight.lightLevel = 0.05f;
}

Color GetSkyColor(void)
{
    float l = dayNight.lightLevel;
    unsigned char r = (unsigned char)(10 + l * 125);
    unsigned char g = (unsigned char)(10 + l * 196);
    unsigned char b = (unsigned char)(40 + l * 195);

    // Darken sky during rain/thunder
    if (weather.rainAlpha > 0.01f) {
        float darkening = weather.rainAlpha * 0.3f;
        r = (unsigned char)(r * (1.0f - darkening));
        g = (unsigned char)(g * (1.0f - darkening));
        b = (unsigned char)(b * (1.0f - darkening * 0.5f));
    }

    return (Color){r, g, b, 255};
}
