#define max(a, b) (((a) > (b)) ? (a) : (b))

internal void RenderGradient(game_OffScreenBuffer *Buffer, int XOffset, int YOffset)
{
    // Bytes:  ## BB GG RR
    u8 *Row = (u8 *)Buffer->Memory;
    for (int y = 0; y < Buffer->Height; y++)
    {
        u32 *Pixel = (u32 *)Row;
        for (int x = 0; x < Buffer->Width; x++)
        {
            u8 Red = (x + XOffset);
            u8 Green = (y + YOffset);

            *Pixel++ = ((Green << 16) | (Red << 8));
        }

        Row += Buffer->Pitch;
    }
}

internal void GameOutputSound(game_SoundOutput *SoundBuffer, int ToneHz)
{
    local_persist f32 tSine;
    s16 ToneVolume = 400;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    s16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
        f32 SineValue = sinf(tSine);
        s16 SampleValue = (s16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi * 1.0f / (f32)WavePeriod;
    }
}

internal void GameUpdateAndRender(game_OffScreenBuffer *Buffer,
                                  game_SoundOutput *SoundBuffer,
                                  b32 Buttons[])
{
    local_persist int RedOffset = 0;
    local_persist int GreenOffset = 0;
    local_persist int ToneHz = 256;

    if (Buttons[KEY_LEFT] == 1)
    {
        RedOffset -= 4;
    }

    if (Buttons[KEY_RIGHT] == 1)
    {
        RedOffset += 4;
    }

    if (Buttons[KEY_UP] == 1)
    {
        GreenOffset -= 4;
    }

    if (Buttons[KEY_DOWN] == 1)
    {
        GreenOffset += 4;
    }

    ToneHz = max(1, 512 + (int)(256.0f * ((f32)GreenOffset / 300.0f)));

    // TODO: Add Sample Offsets
    GameOutputSound(SoundBuffer, ToneHz);
    RenderGradient(Buffer, RedOffset, GreenOffset);
}