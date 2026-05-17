/*******************************************************************************************
*
*   MyWorld - Weather System (Rain, Thunderstorms)
*
********************************************************************************************/

#include "types.h"
#include <stdlib.h>
#include <math.h>

//----------------------------------------------------------------------------------
// Weather Globals
//----------------------------------------------------------------------------------
WeatherState weather = { WEATHER_CLEAR, 0, 0, 0, 0, 0 };
RainDrop rainDrops[MAX_RAIN_DROPS] = { 0 };

//----------------------------------------------------------------------------------
// Weather Initialization
//----------------------------------------------------------------------------------
void InitWeather(void)
{
    weather.type = WEATHER_CLEAR;
    weather.duration = 30.0f + (float)(rand() % 60);
    weather.transitionTimer = 0;
    weather.rainAlpha = 0;
    weather.lightningFlash = 0;
    weather.thunderTimer = 0;

    for (int i = 0; i < MAX_RAIN_DROPS; i++) {
        rainDrops[i].x = (float)(rand() % SCREEN_WIDTH);
        rainDrops[i].y = (float)(rand() % SCREEN_HEIGHT);
        rainDrops[i].speed = 400.0f + (float)(rand() % 200);
        rainDrops[i].length = 8.0f + (float)(rand() % 8);
    }
}

//----------------------------------------------------------------------------------
// Weather Update
//----------------------------------------------------------------------------------
void UpdateWeather(float dt)
{
    // Update weather duration and transitions
    weather.duration -= dt;
    if (weather.duration <= 0) {
        // Transition to next weather
        if (weather.type == WEATHER_CLEAR) {
            // 40% chance of thunder, 60% rain
            weather.type = (rand() % 100 < 40) ? WEATHER_THUNDER : WEATHER_RAIN;
            weather.duration = WEATHER_MIN_DURATION + (float)(rand() % (int)(WEATHER_MAX_DURATION - WEATHER_MIN_DURATION));
        } else {
            weather.type = WEATHER_CLEAR;
            weather.duration = WEATHER_MIN_DURATION * 2.0f + (float)(rand() % (int)(WEATHER_MAX_DURATION));
        }
        weather.transitionTimer = 3.0f; // 3 second transition
    }

    // Smooth transition for rain alpha
    float targetAlpha = 0;
    if (weather.type == WEATHER_RAIN || weather.type == WEATHER_THUNDER) {
        targetAlpha = 1.0f;
    }
    if (weather.transitionTimer > 0) {
        weather.transitionTimer -= dt;
        float t = 1.0f - (weather.transitionTimer / 3.0f);
        if (t > 1.0f) t = 1.0f;
        weather.rainAlpha = weather.rainAlpha + (targetAlpha - weather.rainAlpha) * t * dt * 3.0f;
    } else {
        weather.rainAlpha = targetAlpha;
    }

    // Update rain drops
    if (weather.rainAlpha > 0.01f) {
        float windX = sinf((float)GetTime() * 0.3f) * 30.0f;
        for (int i = 0; i < MAX_RAIN_DROPS; i++) {
            RainDrop *r = &rainDrops[i];
            r->y += r->speed * dt;
            r->x += windX * dt;

            if (r->y > SCREEN_HEIGHT) {
                r->y = -r->length;
                r->x = (float)(rand() % (SCREEN_WIDTH + 100)) - 50;
            }
            if (r->x < -50) r->x = SCREEN_WIDTH + 50;
            if (r->x > SCREEN_WIDTH + 50) r->x = -50;
        }
    }

    // Lightning (thunderstorms only)
    if (weather.type == WEATHER_THUNDER) {
        if (weather.lightningFlash > 0) {
            weather.lightningFlash -= dt * 4.0f;
            if (weather.lightningFlash < 0) weather.lightningFlash = 0;
        }

        // Random lightning
        if ((float)rand() / RAND_MAX < LIGHTNING_CHANCE * dt) {
            weather.lightningFlash = 1.0f;
            weather.thunderTimer = 0.5f + (float)(rand() % 100) / 100.0f * 1.5f;
        }

        // Thunder sound (delayed after lightning)
        if (weather.thunderTimer > 0) {
            weather.thunderTimer -= dt;
            if (weather.thunderTimer <= 0) {
                PlaySoundHurt(); // reuse hurt sound for thunder
            }
        }
    } else {
        weather.lightningFlash = 0;
        weather.thunderTimer = 0;
    }
}

//----------------------------------------------------------------------------------
// Weather Rendering
//----------------------------------------------------------------------------------
void DrawWeather(void)
{
    if (weather.rainAlpha < 0.01f) return;

    unsigned char a = (unsigned char)(180 * weather.rainAlpha);
    Color rainColor = { 150, 170, 220, a };

    for (int i = 0; i < MAX_RAIN_DROPS; i++) {
        RainDrop *r = &rainDrops[i];
        int x1 = (int)r->x;
        int y1 = (int)r->y;
        int x2 = x1 + 1;
        int y2 = y1 + (int)r->length;
        DrawLine(x1, y1, x2, y2, rainColor);
    }

    // Lightning flash overlay
    if (weather.lightningFlash > 0) {
        unsigned char flashA = (unsigned char)(200 * weather.lightningFlash);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){255, 255, 240, flashA});
    }
}

//----------------------------------------------------------------------------------
// Weather Effects
//----------------------------------------------------------------------------------
float GetWeatherLightModifier(void)
{
    // Rain reduces light level slightly
    if (weather.type == WEATHER_RAIN) return -0.15f;
    if (weather.type == WEATHER_THUNDER) return -0.25f;
    return 0;
}
