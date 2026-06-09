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

bool TrySleep(void)
{
    float t = dayNight.timeOfDay;
    if (t < 0.2f || t >= 0.8f) {
        dayNight.timeOfDay = 0.25f;
        return true;
    }
    return false;
}

Color GetSkyColor(void)
{
    float l = dayNight.lightLevel;
    float t = dayNight.timeOfDay;

    unsigned char r = (unsigned char)(10 + l * 125);
    unsigned char g = (unsigned char)(10 + l * 196);
    unsigned char b = (unsigned char)(40 + l * 195);

    // Dawn warm shift (timeOfDay 0.20-0.30)
    if (t >= 0.20f && t < 0.30f) {
        float dawnT = (t - 0.20f) / 0.10f;
        float warm = sinf(dawnT * 3.14159f);
        int rr = r + (int)(warm * 80);
        int gg = g + (int)(warm * 30);
        r = (unsigned char)(rr > 255 ? 255 : rr);
        g = (unsigned char)(gg > 255 ? 255 : gg);
    }
    // Dusk warm shift (timeOfDay 0.70-0.80)
    if (t >= 0.70f && t < 0.80f) {
        float duskT = (t - 0.70f) / 0.10f;
        float warm = sinf(duskT * 3.14159f);
        int rr = r + (int)(warm * 100);
        int gg = g + (int)(warm * 40);
        int bb = b - (int)(warm * 20);
        r = (unsigned char)(rr > 255 ? 255 : rr);
        g = (unsigned char)(gg > 255 ? 255 : gg);
        b = (unsigned char)(bb < 0 ? 0 : bb);
    }

    // Darken sky during rain/thunder
    if (weather.rainAlpha > 0.01f) {
        float darkening = weather.rainAlpha * 0.3f;
        r = (unsigned char)(r * (1.0f - darkening));
        g = (unsigned char)(g * (1.0f - darkening));
        b = (unsigned char)(b * (1.0f - darkening * 0.5f));
    }

    return (Color){r, g, b, 255};
}
