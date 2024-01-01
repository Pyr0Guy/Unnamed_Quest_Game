#if !defined(GAME_H)

typedef struct
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
} game_OffScreenBuffer;

typedef struct
{
    int SamplesPerSecond;
    int SampleCount;
    s16 *Samples;
} game_SoundOutput;

internal void GameUpdateAndRender(game_OffScreenBuffer *Buffer, int RedOffset, int GreenOffset,
                                  game_SoundOutput *SoundBuffer, int ToneHz);

#define GAME_H
#endif