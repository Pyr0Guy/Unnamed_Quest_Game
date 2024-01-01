#if !defined(WIN32_GAME_H)

typedef struct
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
} win32_OffScreenBuffer;

typedef struct
{
    int Width;
    int Height;
} win32_WindowDimension;

typedef struct
{
    int X;
    int Y;
    u8 LButtonStatus;
    u8 RButtonStatus;
    int DeltaWheel;
} win32_MouseInfo;

typedef struct
{
    int SamplesPerSecond;
    u32 RunningSampleIndex;
    int BytesPerSample;
    int SecondBufferSize;
    f32 tSine;
    int LatencySampleCount;
} win32_SoundOutput;

#define WIN32_GAME_H
#endif