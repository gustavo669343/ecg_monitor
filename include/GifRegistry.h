#pragma once
#include <Arduino.h>

#include "SampleGif.h"
#include "Give_infoGif.h"
#include "HappyGif.h"
#include "Idle1Gif.h"
#include "NormalGif.h"
#include "PettedGif.h"
#include "SadGif.h"
#include "SpeakGif.h"

struct GifEntry {
    const char* name;
    const uint8_t* data;
    size_t size;
    uint8_t defaultScale;
};

const GifEntry GIF_REGISTRY[] = {
    { "give_info", give_info_gif, give_info_gif_size, 2 },
    { "happy", happy_gif, happy_gif_size, 2 },
    { "idle1", idle1_gif, idle1_gif_size, 2 },
    { "normal", normal_gif, normal_gif_size, 2 },
    { "petted", petted_gif, petted_gif_size, 2 },
    { "sad", sad_gif, sad_gif_size, 2 },
    { "speak", speak_gif, speak_gif_size, 2 },
    { "star", sample_star_gif, sample_star_gif_size, 8 },
};

const size_t GIF_REGISTRY_COUNT = sizeof(GIF_REGISTRY) / sizeof(GIF_REGISTRY[0]);

inline const GifEntry* findGifByName(const char* name) {
    if (!name) return nullptr;
    for (size_t i = 0; i < GIF_REGISTRY_COUNT; i++) {
        if (strcasecmp(GIF_REGISTRY[i].name, name) == 0) {
            return &GIF_REGISTRY[i];
        }
    }
    return nullptr;
}
