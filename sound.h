#ifndef SOUND_H
#define SOUND_H

#include "types.h"

void InitSounds(void);
void UnloadSounds(void);

void PlaySoundBreak(BlockType block);
void PlaySoundPlace(BlockType block);
void PlaySoundJump(void);
void PlaySoundLand(void);
void PlaySoundHurt(void);
void PlaySoundDeath(void);
void PlaySoundEat(void);
void PlaySoundUIClick(void);
void PlaySoundCraft(void);
void PlaySoundXP(void);
void PlaySoundDrop(void);

#endif
