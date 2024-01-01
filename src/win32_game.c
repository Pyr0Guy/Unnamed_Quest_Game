/*
    TODO: THIS IS NOT A FINAL PLATFORM LAYER

    - Saves game location
    - Handle executable file
    - Asset loading path
    - Multi-Threading
    - Raw Input
    - Sleep/timeBeginPeriod a.k.a DELTATIME
    - ClipCursor() (for multimonitor support)
    - FullScreen
    - WM_SETCURSOR
    - Перепеши коменты на английском долбаёб
    - WM_ACTIVEAPP (for when application is not active)
    - OpenGL?????
    - GetKeyboardLayout (for русской keyboard and other shit)
*/
#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi 3.14159265359f

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef s32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#include "win32_keyboard_keys.h"
#include "game.h"
#include "game.c"

#include <Windows.h>
#include <windowsx.h>
#include <dsound.h>

#include <uxtheme.h>

#include "win32_game.h"

global_variable b32 G_Running;
global_variable win32_OffScreenBuffer G_BackBuffer;
global_variable win32_MouseInfo G_MouseData;
global_variable IDirectSoundBuffer *G_SecondBuffer;
global_variable b32 G_ButtonBuffer[0xFE] = {0};

// Я вообще не ебу что тут написано, но это позволит использовать эту библиотеку на большом количестве систем
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void win32_InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (!DSoundLibrary)
    {
        OutputDebugStringA("Library dosent load\n");
        return;
    }

    direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    IDirectSound *DirectSound;

    HRESULT Error = DirectSoundCreate(0, &DirectSound, 0);
    if (!DirectSoundCreate && !SUCCEEDED(Error))
    {
        OutputDebugStringA("Direct sound doesnt load\n");
        return;
    }

    WAVEFORMATEX WaveFormat = {0};
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = 2;
    WaveFormat.nSamplesPerSec = SamplesPerSecond;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.cbSize = 0;
    WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

    if (SUCCEEDED(DirectSound->lpVtbl->SetCooperativeLevel(DirectSound, Window, DSSCL_PRIORITY)))
    {
        DSBUFFERDESC BufferDescription = {0};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
        IDirectSoundBuffer *PrimaryBuffer;
        HRESULT Error = DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &BufferDescription, &PrimaryBuffer, 0);
        if (SUCCEEDED(Error))
        {
            HRESULT Error = PrimaryBuffer->lpVtbl->SetFormat(PrimaryBuffer, &WaveFormat);
            if (SUCCEEDED(Error))
            {
                OutputDebugStringA("Primary buffer format was set\n");
            }
        }
    }
    DSBUFFERDESC BufferDescription = {0};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = 0;
    BufferDescription.dwBufferBytes = BufferSize;
    BufferDescription.lpwfxFormat = &WaveFormat;
    HRESULT CreateBufferError = DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &BufferDescription, &G_SecondBuffer, 0);
    if (SUCCEEDED(CreateBufferError))
    {
        OutputDebugStringA("Buffer Created!\n");
    }
    else
    {
        OutputDebugStringA("Cant create Buffer\n");
    }
}

internal win32_WindowDimension GetWindowDimension(HWND Window)
{
    win32_WindowDimension Result;

    RECT WindowRect;
    GetClientRect(Window, &WindowRect);

    Result.Width = WindowRect.right - WindowRect.left;
    Result.Height = WindowRect.bottom - WindowRect.top;

    return Result;
}

internal void Win32_ResizeDIBSection(win32_OffScreenBuffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width * BytesPerPixel;
}

internal void Win32_DisplayBufferInWindow(win32_OffScreenBuffer *Buffer, HDC DeviceContex, int WindowWidth, int WindowHeight)
{

    StretchDIBits(DeviceContex,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

LRESULT CALLBACK Win32_MainWindowCallBack(HWND Window,
                                          UINT Message,
                                          WPARAM WParam,
                                          LPARAM LParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
    case WM_SIZE:
    {
    }
    break;
    case WM_DESTROY:
    {
        G_Running = FALSE;
        OutputDebugStringA("WM_DESTROY\n");
    }
    break;
    case WM_CLOSE:
    {
        G_Running = FALSE;
        OutputDebugStringA("WM_CLOSE\n");
    }
    break;
    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContex = BeginPaint(Window, &Paint);

        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;

        win32_WindowDimension Dimension = GetWindowDimension(Window);

        Win32_DisplayBufferInWindow(&G_BackBuffer, DeviceContex, Width, Height);
        EndPaint(Window, &Paint);
    }
    break;

    case WM_MOUSEWHEEL:
    {
        // -120 Колесо прокрутили вниз, 120 Колесо прокрутили вверх
        G_MouseData.DeltaWheel = GET_WHEEL_DELTA_WPARAM(WParam);
    }
    break;
    case WM_MOUSEMOVE:
    {
        G_MouseData.X = GET_X_LPARAM(LParam);
        G_MouseData.Y = GET_Y_LPARAM(LParam);
    }
    break;
    case WM_LBUTTONDOWN:
    {
        OutputDebugStringA("Left is DOWN\n");
        G_MouseData.LButtonStatus = 1;
    }
    break;
    case WM_LBUTTONUP:
    {
        OutputDebugStringA("Left is UP\n");
        G_MouseData.LButtonStatus = 0;
    }
    break;
    case WM_RBUTTONDOWN:
    {
        OutputDebugStringA("Right is DOWN\n");
        G_MouseData.RButtonStatus = 1;
    }
    break;
    case WM_RBUTTONUP:
    {
        OutputDebugStringA("Right is UP\n");
        G_MouseData.RButtonStatus = 0;
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        u32 VKCode = WParam;
        b32 Released = ((LParam & (1 << 30)) != 0);
        b32 Pressed = ((LParam & (1 << 31)) == 0);
        if (Pressed != Released)
        {
            if (Pressed)
            {
                G_ButtonBuffer[VKCode] = 1;
            }

            if (Released)
            {
                G_ButtonBuffer[VKCode] = 0;
            }
        }

        // Альт + Ф4 не работал вообще, из за того что я использовал WM_SYSKEYDOWN и WM_SYSKEYUP поэтому пришлось обрабатывать его вручную
        b32 AltKeyIsDown = (LParam & (1 << 29));
        if ((VKCode == VK_F4) && AltKeyIsDown)
        {
            G_Running = FALSE;
        }
    }
    break;

    default:
    {
        Result = DefWindowProcA(Window, Message, WParam, LParam);
    }
    break;
    }

    return Result;
}

internal void Win32_ClearBuffer(win32_SoundOutput *SourceBuffer)
{
    VOID *RegionOne;
    DWORD RegionOneSize;
    VOID *RegionTwo;
    DWORD RegionTwoSize;
    if (SUCCEEDED(G_SecondBuffer->lpVtbl->Lock(G_SecondBuffer, 0, SourceBuffer->SecondBufferSize,
                                               &RegionOne, &RegionOneSize,
                                               &RegionTwo, &RegionTwoSize,
                                               0)))
    {
        u8 *DestSample = (u8 *)RegionOne;
        for (DWORD ByteIndex = 0; ByteIndex < RegionOneSize; ByteIndex++)
        {
            *DestSample++ = 0;
        }

        DestSample = (u8 *)RegionTwo;
        for (DWORD ByteIndex = 0; ByteIndex < RegionTwoSize; ByteIndex++)
        {
            *DestSample++ = 0;
        }

        G_SecondBuffer->lpVtbl->Unlock(G_SecondBuffer, RegionOne, RegionOneSize, RegionTwo, RegionTwoSize);
    }
}

internal void Win32_FillSoundBuffer(win32_SoundOutput *SoundOutput, DWORD ByteToLock, DWORD ByteToWrite,
                                    game_SoundOutput *SourceBuffer)
{
    VOID *RegionOne;
    DWORD RegionOneSize;
    VOID *RegionTwo;
    DWORD RegionTwoSize;
    if (SUCCEEDED(G_SecondBuffer->lpVtbl->Lock(G_SecondBuffer, ByteToLock, ByteToWrite,
                                               &RegionOne, &RegionOneSize,
                                               &RegionTwo, &RegionTwoSize,
                                               0)))
    {
        DWORD RegionOneSampleCount = RegionOneSize / SoundOutput->BytesPerSample;
        s16 *SourceSample = SourceBuffer->Samples;
        s16 *DestSample = (s16 *)RegionOne;

        for (DWORD SampleIndex = 0; SampleIndex < RegionOneSampleCount; SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;

            SoundOutput->RunningSampleIndex++;
        }

        DestSample = (s16 *)RegionTwo;
        DWORD RegionTwoSampleCount = RegionTwoSize / SoundOutput->BytesPerSample;

        for (DWORD SampleIndex = 0; SampleIndex < RegionTwoSampleCount; SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;

            SoundOutput->RunningSampleIndex++;
        }

        G_SecondBuffer->lpVtbl->Unlock(G_SecondBuffer, RegionOne, RegionOneSize, RegionTwo, RegionTwoSize);
    }
}

int WINAPI WinMain(HINSTANCE Instance,
                   HINSTANCE PrevInstance,
                   PSTR CommandLine,
                   int ShowCode)
{
    LARGE_INTEGER PerfCounterFrenFrequencyRes;
    QueryPerformanceFrequency(&PerfCounterFrenFrequencyRes);
    s64 PerfCounterFrenFrequency = PerfCounterFrenFrequencyRes.QuadPart;

    WNDCLASSA WindowClass = {0};

    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32_MainWindowCallBack;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "GameTestClass";
    WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);

    if (!RegisterClassA(&WindowClass))
    {
        OutputDebugStringA("Window class is no registrate\n");
        return -1;
    }

    HWND Window = CreateWindowExA(0,
                                  WindowClass.lpszClassName,
                                  "Game",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  0,
                                  0,
                                  Instance,
                                  0);

    if (!Window)
    {
        OutputDebugStringA("Window handle is no registrate\n");
        return -2;
    }

    HDC DeviceContex = GetDC(Window);
    Win32_ResizeDIBSection(&G_BackBuffer, 1280, 720);

    win32_SoundOutput SoundOutput = {0};
    SoundOutput.SamplesPerSecond = 48000;
    SoundOutput.BytesPerSample = sizeof(s16) * 2;
    SoundOutput.SecondBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
    SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

    win32_InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondBufferSize);
    Win32_ClearBuffer(&SoundOutput);
    G_SecondBuffer->lpVtbl->Play(G_SecondBuffer, 0, 0, DSBPLAY_LOOPING);

    G_Running = TRUE;

    s16 *Samples = (s16 *)VirtualAlloc(0, SoundOutput.SecondBufferSize, MEM_COMMIT, PAGE_READWRITE);

    LARGE_INTEGER LastCounter;
    QueryPerformanceCounter(&LastCounter);

    s64 LastCycleCount = __rdtsc();

    while (G_Running)
    {

        MSG Message;
        while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT)
            {
                G_Running = FALSE;
            }

            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        }

        DWORD ByteToLock;
        DWORD TargeCursor;
        DWORD BytesToWrite;
        DWORD PlayCursor;
        DWORD WriteCursor;
        b32 SoundIsValid = FALSE;

        if (SUCCEEDED(G_SecondBuffer->lpVtbl->GetCurrentPosition(G_SecondBuffer, &PlayCursor, &WriteCursor)))
        {
            ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondBufferSize);
            TargeCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondBufferSize);

            if (ByteToLock > TargeCursor)
            {
                BytesToWrite = (SoundOutput.SecondBufferSize - ByteToLock);
                BytesToWrite += TargeCursor;
            }
            else
            {
                BytesToWrite = TargeCursor - ByteToLock;
            }

            SoundIsValid = TRUE;
        }

        game_SoundOutput SoundBuffer = {0};
        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
        SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
        SoundBuffer.Samples = Samples;

        // GAME RENDER
        game_OffScreenBuffer Buffer = {0};
        Buffer.Memory = G_BackBuffer.Memory;
        Buffer.Pitch = G_BackBuffer.Pitch;
        Buffer.Height = G_BackBuffer.Height;
        Buffer.Width = G_BackBuffer.Width;

        // MAIN GAME LOOP
        GameUpdateAndRender(&Buffer, &SoundBuffer, G_ButtonBuffer);

        if (SoundIsValid)
        {
            Win32_FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
        }

        win32_WindowDimension Dimension = GetWindowDimension(Window);
        Win32_DisplayBufferInWindow(&G_BackBuffer, DeviceContex, Dimension.Width, Dimension.Height);

        s64 EndCycleCount = __rdtsc();

        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);

        // How much time elapsed since start
        s64 CycleElapsed = (EndCycleCount - LastCycleCount);
        s32 MegaCyclePerFrame = (s32)CycleElapsed / (100000);

        s64 CounterElapsed = (EndCounter.QuadPart - LastCounter.QuadPart);
        s32 MilisecondPerFrame = (s32)((1000 * CounterElapsed) / PerfCounterFrenFrequency);
        s32 FPS = (PerfCounterFrenFrequency / CounterElapsed);

#if 0
        char Buffer[60];
        wsprintf(Buffer, "%dms/f, %dFPS, %dmc/f\n", MilisecondPerFrame, FPS, MegaCyclePerFrame);
        OutputDebugStringA(Buffer);
#endif
        LastCounter = EndCounter;
        LastCycleCount = EndCycleCount;
    }

    return 0;
}