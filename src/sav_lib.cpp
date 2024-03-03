#include "sav_lib.h"

#include "va_util.h"
#include "va_sstring.h"
#include "va_linmath.h"

#include <windows.h>

#define GLAD_GLAPI_EXPORT
#include <glad/glad.h>

#include <sdl2/SDL.h>
#include <sdl2/SDL_mixer.h>
#include <sdl2/SDL_image.h>
#include <sdl2/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#define SAV_LIB_INT
#include "sav_lib_int.cpp"

// ------------------------
// SECTION: Internal functions
// ------------------------

static_i void *
Win32AllocMemory(size_t Size)
{
    void *Memory = VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (Memory == NULL)
    {
        printf("Could not virtual alloc.\n");
    }
    
    return Memory;
}

static_i FILETIME
Win32GetFileModifiedTime(const char *FilePath)
{
    FILETIME LastWriteTime = {};
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(FilePath, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return LastWriteTime;
}

static_i b32
Win32LoadGameCode(game_code *GameCode)
{
    CopyFile(GameCode->SourceDllPath.D, GameCode->TempDllPath.D, FALSE);
    
    GameCode->Dll = LoadLibraryA(GameCode->TempDllPath.D);
    
    if (GameCode->Dll)
    {
        GameCode->UpdateAndRenderFunc = GetProcAddress(GameCode->Dll, GameCode->FuncName.D);

        GameCode->IsValid = (bool) GameCode->UpdateAndRenderFunc;
        GameCode->LastWriteTime = Win32GetFileModifiedTime(GameCode->SourceDllPath.D);

        return true;
    }

    return false;
}

static_i void
Win32UnloadGameCode(game_code *GameCode)
{
    if (GameCode->Dll)
    {
        FreeLibrary(GameCode->Dll);
        GameCode->Dll = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRenderFunc = 0;
}

static_i b32
Win32ReloadGameCode(game_code *GameCode)
{
    FILETIME DllNewWriteTime = Win32GetFileModifiedTime(GameCode->SourceDllPath.D);
    int CompareResult = CompareFileTime(&DllNewWriteTime, &GameCode->LastWriteTime);
    
    if (CompareResult == 1)
    {
        DWORD LockFileAttrib = GetFileAttributes(GameCode->LockFilePath.D);
        bool LockFilePresent = (LockFileAttrib != INVALID_FILE_ATTRIBUTES);
        
        SYSTEMTIME DllLastWriteTimeSystem;
        int Result = FileTimeToSystemTime(&GameCode->LastWriteTime, &DllLastWriteTimeSystem);
                        
        SYSTEMTIME DllNewWriteTimeSystem;
        Result = FileTimeToSystemTime(&DllNewWriteTime, &DllNewWriteTimeSystem);

        printf("Old: %02d:%02d:%02d:%03d | New: %02d:%02d:%02d:%03d. Result: %d. Lock file present: %d\n",
               DllLastWriteTimeSystem.wHour,
               DllLastWriteTimeSystem.wMinute,
               DllLastWriteTimeSystem.wSecond,
               DllLastWriteTimeSystem.wMilliseconds,
               DllNewWriteTimeSystem.wHour,
               DllNewWriteTimeSystem.wMinute,
               DllNewWriteTimeSystem.wSecond,
               DllNewWriteTimeSystem.wMilliseconds,
               CompareResult,
               LockFilePresent);
 
        // NOTE: Check lock file is not present (if present - rebuild is not done yet)
        if (!LockFilePresent)
        {
            Win32UnloadGameCode(GameCode);
            return Win32LoadGameCode(GameCode);
        }
    }

    return false;
}

inline f64
GetAvgDelta(f64 *Samples, int SampleCount)
{
    f64 Accum = 0.0f;
    for (int i = 0; i < SampleCount; i++)
    {
        Accum += Samples[i];
    }
    return Accum / TIMING_STAT_AVG_COUNT;
}

inline void
UseProgram(u32 ShaderID)
{
    glUseProgram(ShaderID);
}

// ------------------------
// SECTION: External functions
// ------------------------

// SECTION: Game memory
game_memory
AllocGameMemory(size_t Size)
{
    game_memory GameMemory;
    GameMemory.Size = Megabytes(128);
    GameMemory.Data = Win32AllocMemory(GameMemory.Size);
    return GameMemory;
}

void
DumpGameMemory(game_memory GameMemory)
{
    win32_state *Win32State = &gWin32State;
    game_code *GameCode = &gGameCode;

    if (Win32State->DumpMemoryBlock == NULL)
    {
        simple_string Dir = GetDirectoryFromPath(GameCode->SourceDllPath.D);
        simple_string DllNameNoExt = GetFilenameFromPath(GameCode->SourceDllPath.D, false);
        simple_string DumpFileName = CatStrings(DllNameNoExt.D, "_mem.savdump");
        simple_string DumpFilePath = CatStrings(Dir.D, DumpFileName.D);

        Win32State->DumpMemoryFileHandle = CreateFileA(DumpFilePath.D, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    
        LARGE_INTEGER MaxSize;
        MaxSize.QuadPart = GameMemory.Size;
        Win32State->DumpMemoryMap = CreateFileMapping(Win32State->DumpMemoryFileHandle,
                                                      0, PAGE_READWRITE, MaxSize.HighPart, MaxSize.LowPart, 0);
        Win32State->DumpMemoryBlock = MapViewOfFile(Win32State->DumpMemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, GameMemory.Size);
    }
    
    CopyMemory(Win32State->DumpMemoryBlock, GameMemory.Data, GameMemory.Size);
}

void
ReloadGameMemoryDump(game_memory GameMemory)
{
    win32_state *Win32State = &gWin32State;
    
    if (Win32State->DumpMemoryBlock)
    {
        CopyMemory(GameMemory.Data, Win32State->DumpMemoryBlock, GameMemory.Size);
    }
}

//
// NOTE: Game code hot reload
//

b32
InitGameCode(const char *DllPath, const char *FuncName, void **UpdateAndRenderFunc)
{
    game_code *GameCode = &gGameCode;
    
    simple_string Dir = GetDirectoryFromPath(DllPath);
    simple_string DllNameNoExt = GetFilenameFromPath(DllPath, false);
    simple_string TempDllName = CatStrings(DllNameNoExt.D, "_temp.Dll");
    simple_string LockFileName = CatStrings(DllNameNoExt.D, ".lock");
    GameCode->SourceDllPath = SimpleString(DllPath);
    GameCode->TempDllPath = CatStrings(Dir.D, TempDllName.D);
    GameCode->LockFilePath = CatStrings(Dir.D,  LockFileName.D);
    GameCode->FuncName = SimpleString(FuncName);
    
    int Loaded = Win32LoadGameCode(GameCode);

    if (Loaded)
    {
        *UpdateAndRenderFunc = GameCode->UpdateAndRenderFunc;
    }
    else
    {
        *UpdateAndRenderFunc = 0;
    }

    return Loaded;
}

b32
ReloadGameCode(void **UpdateAndRenderFunc)
{
    game_code *GameCode = &gGameCode;

    b32 Reloaded = Win32ReloadGameCode(GameCode);

    if (Reloaded)
    {
        if (GameCode->IsValid)
        {
            *UpdateAndRenderFunc = GameCode->UpdateAndRenderFunc;
        }
        else
        {
            *UpdateAndRenderFunc = 0;
        }
    }

    return Reloaded;
}

// SECTION: Program state/sdl window
b32
InitWindow(const char *WindowName, int WindowWidth, int WindowHeight)
{
    sdl_state *SdlState = &gSdlState;
    
    if (SDL_Init(SDL_INIT_VIDEO) == 0)
    {
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SdlState->Window = SDL_CreateWindow(WindowName,
                                            3160, 40,
                                            WindowWidth, WindowHeight,
                                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        if (SdlState->Window)
        {
            int SDLImageFlags = IMG_INIT_JPG | IMG_INIT_PNG;
            int IMGInitResult = IMG_Init(SDLImageFlags);
            if (!(IMGInitResult & SDLImageFlags))
            {
                printf("SDL failed to init SDL_image\n");
                return false;
            }

            if (TTF_Init() != 0)
            {
                printf("SDL failed to init SDL_ttf\n");
                return false;
            }

            int ActualWidth, ActualHeight;
            SDL_GetWindowSize(SdlState->Window, &ActualWidth, &ActualHeight);
            SdlState->WindowSize = Vec2((f32) ActualWidth, (f32) ActualHeight);
            SdlState->WindowOrigSize = SdlState->WindowSize;
            
            SDL_GLContext GlContext = SDL_GL_CreateContext(SdlState->Window);

            if (GlContext)
            {
                gladLoadGLLoader(SDL_GL_GetProcAddress);
                printf("OpenGL loaded\n");
                printf("Vendor: %s\n", glGetString(GL_VENDOR));
                printf("Renderer: %s\n", glGetString(GL_RENDERER));
                printf("Version: %s\n\n", glGetString(GL_VERSION));

                SdlState->PerfCounterFreq = SDL_GetPerformanceFrequency();

                TIMECAPS DevCaps;
                MMRESULT DevCapsGetResult = timeGetDevCaps(&DevCaps, sizeof(DevCaps));
                if (DevCapsGetResult == MMSYSERR_NOERROR)
                {
                    TraceLog("Available DevCaps Range: [%u, %u] msx", DevCaps.wPeriodMin, DevCaps.wPeriodMax);
                }

                // TODO: If needed handle other wPeriodMin for other systems
                SdlState->DesiredSchedulerMS = 1;
                SdlState->SleepIsGranular = (timeBeginPeriod(SdlState->DesiredSchedulerMS) == TIMERR_NOERROR);
                TraceLog("Sleep is granular: %d", SdlState->SleepIsGranular);

                srand((unsigned) time(NULL));

                // SECTION: GL INIT

                // TODO: Don't do the following things in this function
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                u32 White = 0xFFFFFFFF;
                sav_texture DefaultTexture = SavLoadTextureFromData(&White, 1, 1);
                gl_state *GlState = &gGlState;
                GlState->DefaultTextureGlid = DefaultTexture.Glid;

                GlState->DefaultShader = BuildBasicShader();
                GlState->MaxVertexCount = 65536;
                GlState->MaxIndexCount = 393216;
                PrepareGpuData(&GlState->DefaultVBO,
                               &GlState->DefaultVAO,
                               &GlState->DefaultEBO,
                               GlState->MaxVertexCount,
                               GlState->MaxIndexCount);

                GlState->CurrentShader = GlState->DefaultShader;
                UseProgram(GlState->CurrentShader);
                SetProjectionMatrix(Mat4(1.0f));
                SetModelViewMatrix(Mat4(1.0f));

                SDL_GL_SetSwapInterval(0);
            }
            else
            {
                printf("SDL failed to create GL context\n");
                return false;
            }

        }
        else
        {
            printf("SDL failed to create Window\n");
            return false;
        }
    }
    else
    {
        printf("SDL failed to init\n");
        return false;
    }

    return true;
}

void
PollEvents(b32 *Quit)
{
    sdl_state *SdlState = &gSdlState;
    input_state *InputState = &gInputState;

    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
    {
        InputState->RepeatKeyStates[i] = 0;

        InputState->PreviousKeyStates[i] = InputState->CurrentKeyStates[i];
    }

    for (int i = 0; i < SDL_BUTTON_X2 + 1; i++)
    {
        InputState->ClickMouseButtonStates[i] = 0;

        InputState->PreviousMouseButtonStates[i] = InputState->CurrentMouseButtonStates[i];
    }

    InputState->MouseWheel = 0;

    SdlState->WindowSizeChanged = false;
    
    SDL_Event Event;
    while(SDL_PollEvent(&Event))
    {
        switch (Event.type)
        {
            case SDL_QUIT:
            {
                *Quit = true;
            } break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                InputState->CurrentKeyStates[Event.key.keysym.scancode] = (Event.type == SDL_KEYDOWN);
                InputState->RepeatKeyStates[Event.key.keysym.scancode] = Event.key.repeat;
            } break;

            case SDL_MOUSEMOTION:
            {
                // NOTE: It seems it's better to update mouse position every frame
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                InputState->CurrentMouseButtonStates[Event.button.button] = (Event.type == SDL_MOUSEBUTTONDOWN);
                if (Event.type == SDL_MOUSEBUTTONDOWN)
                {
                    InputState->ClickMouseButtonStates[Event.button.button] = Event.button.clicks;
                }
            } break;
            case SDL_MOUSEWHEEL:
            {
                // TODO: Maybe deal with Event.wheel.direction field on other platforms
                InputState->MouseWheel += Event.wheel.y; // NOTE: Add y, because it's likely there were more than one Event between frames
            } break;

            case SDL_WINDOWEVENT:
            {
                if (Event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    SdlState->WindowSize = Vec2((f32) Event.window.data1, (f32) Event.window.data2);
                    SdlState->WindowSizeChanged = true;
                }
            } break;
            default: break;
        }
    }

    int MouseX, MouseY, MouseRelX, MouseRelY;
    SDL_GetMouseState(&MouseX, &MouseY);
    SDL_GetRelativeMouseState(&MouseRelX, &MouseRelY);
    InputState->MousePos = Vec2((f32) MouseX, (f32) MouseY);
    InputState->MouseRelPos = Vec2((f32) MouseRelX, (f32) MouseRelY);
}

void
Quit()
{
    sdl_state *SdlState = &gSdlState;
    if (SdlState->SleepIsGranular)
    {
        timeEndPeriod(SdlState->DesiredSchedulerMS);
    }

    if (SdlState->Window)
    {
        SDL_DestroyWindow(SdlState->Window);
    }

    SDL_Quit();
}

void
SetTargetFPS(f64 FPS)
{
    sdl_state *SdlState= &gSdlState;
    SdlState->LimitFPS = true;
    SdlState->TargetFPS = FPS;
    SdlState->TargetDelta = 1.0 / SdlState->TargetFPS;
    TraceLog("Target FPS: %.0f; %.6f\n", SdlState->TargetFPS, SdlState->TargetDelta);
}

void
BeginFrameTiming()
{
    sdl_state *SdlState = &gSdlState;
    input_state *InputState = &gInputState;

    if (SdlState->LastCounter)
    {
        u64 CurrentCounter = SDL_GetPerformanceCounter();
        u64 CounterElapsed = CurrentCounter - SdlState->LastCounter;
        SdlState->LastCounter = CurrentCounter;
        SdlState->PrevDelta = (f64) CounterElapsed / SdlState->PerfCounterFreq;

        SdlState->DeltaSamples[SdlState->CurrentTimingStatSample++] = SdlState->PrevDelta;
        if (SdlState->CurrentTimingStatSample >= TIMING_STAT_AVG_COUNT)
        {
            SdlState->AvgDelta = GetAvgDelta(SdlState->DeltaSamples, TIMING_STAT_AVG_COUNT);
            SdlState->CurrentTimingStatSample = 0;
        }
    }
    else
    {
        SdlState->LastCounter = SDL_GetPerformanceCounter();
    }

    gCurrentFrame++;
}

void
EndFrameTiming()
{
    sdl_state *SdlState = &gSdlState;

    if (SdlState->LimitFPS)
    {
        u64 CounterElapsed = SDL_GetPerformanceCounter() - SdlState->LastCounter;
        f64 ElapsedMS = (f64) CounterElapsed / SdlState->PerfCounterFreq;

#if 0
        f64 FrameTimeUtilization = ElapsedMS / SdlState->TargetDelta;
        TraceLog("Frame time utilization: %f", FrameTimeUtilization * 100.0);
#endif

        u64 TargetElapsed = (u64)(SdlState->TargetDelta * SdlState->PerfCounterFreq);

        int SleepForMS = (int) (1000.0*(SdlState->TargetDelta - ElapsedMS)) - 2;
        if (SleepForMS > 1)
        {
            Sleep((DWORD) SleepForMS);

            CounterElapsed = SDL_GetPerformanceCounter() - SdlState->LastCounter;
            if (CounterElapsed > TargetElapsed)
            {
                TraceLog("!!!!!!!!!!! SLEEP MISSED TARGET BY %f SEC !!!!!!!!!!!!", (f64) (CounterElapsed - TargetElapsed) / SdlState->PerfCounterFreq);
            }
        }

        // int Spins = 0;
        while (CounterElapsed < TargetElapsed)
        {
            CounterElapsed = SDL_GetPerformanceCounter() - SdlState->LastCounter;
            // Spins++;
        }

        // Noop;
    }
}

// SECTION: SDL window
void SetWindowTitle(const char *title) { SDL_SetWindowTitle(gSdlState.Window, title); }
vec2 GetWindowSize() { return gSdlState.WindowSize; }
vec2 GetWindowOrigSize() { return gSdlState.WindowOrigSize; }
b32 WindowSizeChanged() { return gSdlState.WindowSizeChanged; }

void
SetWindowBorderless(b32 Borderless)
{
    sdl_state *SdlState = &gSdlState;

    if (Borderless)
    {
        int X, Y;
        SDL_GetWindowPosition(SdlState->Window, &X, &Y);
        SdlState->WindowRectBeforeBorderless = Rect((f32) X, (f32) Y, SdlState->WindowSize);
    }
    
    SDL_SetWindowBordered(SdlState->Window, (SDL_bool) !Borderless);
    
    if (Borderless)
    {
        SDL_MaximizeWindow(SdlState->Window);
    }
    else
    {
        SDL_RestoreWindow(SdlState->Window);

        // TODO: This is very hacky, maybe there's a better way to do this with SDL
        if (SdlState->WindowRectBeforeBorderless.Width > 0)
        {
            SDL_SetWindowSize(SdlState->Window, (int) SdlState->WindowRectBeforeBorderless.Width, (int) SdlState->WindowRectBeforeBorderless.Height);
            SDL_SetWindowPosition(SdlState->Window, (int) SdlState->WindowRectBeforeBorderless.X, (int) SdlState->WindowRectBeforeBorderless.Y);
            SdlState->WindowRectBeforeBorderless = Rect(0);
        }
    }
}

void
ToggleWindowBorderless()
{
    sdl_state *SdlState = &gSdlState;
    SdlState->Borderless = !SdlState->Borderless;
    SetWindowBorderless(SdlState->Borderless);
}

// SECTION: Input helpers
b32 KeyDown(int Key)
{
    return (b32) gInputState.CurrentKeyStates[Key];
}
b32 KeyPressed(int Key)
{
    return (b32) (gInputState.CurrentKeyStates[Key] && !gInputState.PreviousKeyStates[Key]);
}
b32 KeyReleased(int Key)
{
    return (b32) (!gInputState.CurrentKeyStates[Key] && gInputState.PreviousKeyStates[Key]);
}
b32 KeyRepeat(int Key)
{
    return (b32) gInputState.RepeatKeyStates[Key];
}
b32 KeyPressedOrRepeat(int Key)
{
    return (b32) ((gInputState.CurrentKeyStates[Key] && !gInputState.PreviousKeyStates[Key]) || gInputState.RepeatKeyStates[Key]);
}

b32 GetMouseRelativeMode()
{
    return (b32) SDL_GetRelativeMouseMode();
}
void SetMouseRelativeMode(b32 Enabled)
{
    gInputState.IsRelMouse = Enabled;
    SDL_SetRelativeMouseMode((SDL_bool) gInputState.IsRelMouse);
}
vec2 GetMousePos()
{
    return gInputState.MousePos;
}
vec2 GetMouseRelPos()
{
    return gInputState.MouseRelPos;
}
vec2 GetMouseLogicalPos()
{
    vec2 P = Vec2((f32) gInputState.MousePos.X, (f32) gInputState.MousePos.Y);
    gl_state *GlState = &gGlState;
    if (GlState->RenderTextureActive)
    {
        P = ScreenToRectCoords(GlState->CurrentRenderTextureScreenRect,
                               (f32) GlState->CurrentRenderTexture.Texture.Width,
                               (f32) GlState->CurrentRenderTexture.Texture.Height,
                               P);
    }
    return P;
}
b32 MouseDown(int Button)
{
    return (b32) gInputState.CurrentMouseButtonStates[Button];
}
b32 MousePressed(int Button)
{
    return (b32) (gInputState.CurrentMouseButtonStates[Button] && !gInputState.PreviousMouseButtonStates[Button]);
}

b32 MouseReleased(int Button)
{
    return (b32) (!gInputState.CurrentMouseButtonStates[Button] && gInputState.PreviousMouseButtonStates[Button]);
}

b32 MouseClicks(int Button, int Clicks)
{
    return (b32) (gInputState.ClickMouseButtonStates[Button] == Clicks);
}
i32 MouseWheel()
{
    return gInputState.MouseWheel;
}

// SECTION: Timing
u64 GetCurrentFrame()
{
    return gCurrentFrame;
}
f64 GetDeltaFixed()
{
    return gSdlState.TargetDelta; // TODO: Fixed framerate game logic
}
f64 GetDeltaPrev()
{
    return gSdlState.PrevDelta;
}
f64 GetDeltaAvg()
{
    return gSdlState.AvgDelta;
}
f64 GetFPSPrev()
{
    if (gSdlState.PrevDelta > 0.0) return 1.0 / gSdlState.PrevDelta;
    else return 0.0;
}
f64 GetFPSAvg()
{
    if (gSdlState.AvgDelta > 0.0) return 1.0 / gSdlState.AvgDelta;
    else return 0.0;
}

// SECTION: Audio
b32
InitAudio()
{
    if (SDL_Init(SDL_INIT_AUDIO) == 0)
    {
        if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 2048) == 0)
        {
            printf("Audio device loaded\n");
            return true;
        }
        else
        {
            printf("SDL failed to open audio device\n");
            return true;
        }
    }
    else
    {
        printf("SDL failed to init audio subsystem\n");
        return false;
    }
}

music_stream
LoadMusicStream(const char *FilePath)
{
    music_stream Result;

    Mix_Music *MixMusic = Mix_LoadMUS(FilePath);

    Result.Music = (void *) MixMusic;
    
    return Result;
}

sound_chunk
LoadSoundChunk(const char *FilePath)
{
    sound_chunk Result;

    Mix_Chunk *MixChunk = Mix_LoadWAV(FilePath);

    Result.Sound = (void *) MixChunk;
    
    return Result;
}

b32 PlayMusicStream(music_stream Stream)
{
    return (Mix_PlayMusic((Mix_Music *) Stream.Music, 0) == 0);
}
b32 PlaySoundChunk(sound_chunk Chunk)
{
    int Result = Mix_PlayChannel(2, (Mix_Chunk *) Chunk.Sound, 0);
    return (Result != -1);
}
void FreeMusicStream(music_stream Stream)
{
    Mix_FreeMusic((Mix_Music *) Stream.Music);
}
void FreeSoundChunk(sound_chunk Chunk)
{
    Mix_FreeChunk((Mix_Chunk *) Chunk.Sound);
}

// SECTION: Drawing
static_i u32
BuildShadersFromStr(const char *VertSource, const char *FragSource)
{
    u32 VertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertShader, 1, &VertSource, NULL);
    glCompileShader(VertShader);
    
    int Success;
    char InfoLog[512];
    glGetShaderiv(VertShader, GL_COMPILE_STATUS, &Success);
    if (!Success)
    {
        glGetShaderInfoLog(VertShader, 512, NULL, InfoLog);
        TraceLog("Vertex shader compilation error:\n%s\n\n", InfoLog);
    }
    
    u32 FragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragShader, 1, &FragSource, NULL);
    glCompileShader(FragShader);
    
    glGetShaderiv(FragShader, GL_COMPILE_STATUS, &Success);
    if (!Success)
    {
        glGetShaderInfoLog(FragShader, 512, NULL, InfoLog);
        TraceLog("Fragment shader compilation error:\n%s\n\n", InfoLog);
    }
    
    u32 Program = glCreateProgram();
    glAttachShader(Program, VertShader);
    glAttachShader(Program, FragShader);
    glLinkProgram(Program);

    glGetProgramiv(Program, GL_LINK_STATUS, &Success);
    if (!Success)
    {
        glGetProgramInfoLog(Program, 512, NULL, InfoLog);
        TraceLog("Linking error:\n%s\n\n", InfoLog);
    }
    glDeleteShader(VertShader);
    glDeleteShader(FragShader);

    return Program;
}

u32
BuildBasicShader()
{
    const char *VertSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 vertPosition;\n"
        "layout (location = 1) in vec4 vertTexCoord;\n"
        "layout (location = 2) in vec4 vertColor;\n"
        "out vec4 fragTexCoord;\n"
        "out vec4 fragColor;\n"
        "uniform mat4 mvp;\n"
        "void main()\n"
        "{\n"
        "   fragTexCoord = vertTexCoord;\n"
        "   fragColor = vertColor;\n"
        "   gl_Position = mvp * vec4(vertPosition, 1.0);\n"
        "}\0";
    
    const char *FragSource =
        "#version 330 core\n"
        "in vec4 fragTexCoord;\n"
        "in vec4 fragColor;\n"
        "out vec4 finalColor;\n"
        "uniform sampler2D texture0;\n"
        "void main()\n"
        "{\n"
        "   vec4 texelColor = texture(texture0, fragTexCoord.xy);\n"
        "   finalColor = fragColor * texelColor;\n"
        "}\n\0";
     
    return BuildShadersFromStr(VertSource, FragSource);
}

sav_shader
BuildCustomShader(const char *VertPath, const char *FragPath)
{
    TraceLog("Building custom shader program with shaders: %s, %s", VertPath, FragPath);
    
    char *VertSource = SavReadTextFile(VertPath);
    char *FragSource = SavReadTextFile(FragPath);

    u32 ShaderID = BuildShadersFromStr(VertSource, FragSource);

    SavFreeString(&VertSource);
    SavFreeString(&FragSource);

    sav_shader Result;
    Result.Glid = ShaderID;
    
    return Result;
}

void
DeleteShader(sav_shader *Shader)
{
    glDeleteProgram(Shader->Glid);
    Shader->Glid = 0;
}

void
BeginShaderMode(sav_shader Shader)
{
    gGlState.CurrentShader = Shader.Glid;
    UseProgram(Shader.Glid);
}

void
EndShaderMode()
{
    gGlState.CurrentShader = gGlState.DefaultShader;
    UseProgram(gGlState.DefaultShader);
}

static_i int
GetUniformLocation(u32 Shader, const char *UniformName)
{
    int UniformLocation = glGetUniformLocation(gGlState.CurrentShader, UniformName);

    #ifdef SAV_DEBUG
    if (UniformLocation == -1)
    {
        GLenum Error = glGetError();
        TraceLog("Failed to get uniform \"%s\" for ShaderID %d. Error code: %d", UniformName, Shader, Error);
        // InvalidCodePath;
    }
    #endif

    return UniformLocation;
}

void
SetUniformMat4(const char *UniformName, f32 *Value)
{
    int UniformLocation = GetUniformLocation(gGlState.CurrentShader, UniformName);
    glUniformMatrix4fv(UniformLocation, 1, false, Value);
}

void
SetUniformVec4(const char *UniformName, f32 *Value)
{
    int UniformLocation = GetUniformLocation(gGlState.CurrentShader, UniformName);
    glUniform4fv(UniformLocation, 1, Value);
}

void
SetUniformI(const char *UniformName, int Value)
{
    int UniformLocation = GetUniformLocation(gGlState.CurrentShader, UniformName);
    glUniform1i(UniformLocation, Value);
}

void
BindTextureSlot(int Slot, sav_texture Texture)
{
    glActiveTexture(GL_TEXTURE0 + Slot);
    glBindTexture(GL_TEXTURE_2D, Texture.Glid);
}

void
UnbindTextureSlot(int Slot)
{
    glActiveTexture(GL_TEXTURE0 + Slot);
    glBindTexture(GL_TEXTURE_2D, gGlState.DefaultTextureGlid);
}

void
ClearBackground(color Color)
{
    vec4 C = ColorV4(Color);
    glClearColor(C.R, C.G, C.B, C.A);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void
BeginDraw()
{
    sdl_state *SdlState = &gSdlState;
    glViewport(0, 0, (int) SdlState->WindowSize.X, (int) SdlState->WindowSize.Y);
    SetProjectionMatrix(Mat4GetOrthographicProjection(0.0f,
                                                      SdlState->WindowSize.X,
                                                      SdlState->WindowSize.Y,
                                                      0.0f, -1.0f, 1.0f));
    gGlState.RenderTextureActive = false;
    gGlState.DrawReady = true;

}

void
EndDraw()
{
    gGlState.DrawReady = false;
}

void
PrepareGpuData(u32 *VBO, u32 *VAO, u32 *EBO, int MaxVertCount, int MaxIndexCount)
{
    size_t BytesPerVertex = (3 + 4 + 4) * sizeof(float);
    
    glGenVertexArrays(1, VAO);
    Assert(*VAO);
                    
    glGenBuffers(1, VBO);
    Assert(*VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, MaxVertCount * BytesPerVertex, 0, GL_DYNAMIC_DRAW);

    glBindVertexArray(*VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *) (MaxVertCount * sizeof(vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *) (MaxVertCount * (sizeof(vec3) + sizeof(vec4))));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, EBO);
    Assert(*EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndexCount * sizeof(u32), 0, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void
DrawVertices(vec3 *Positions, vec4 *TexCoords, vec4 *Colors, u32 *Indices, int VertexCount, int IndexCount)
{
    gl_state *GlState = &gGlState;

    Assert(GlState->DrawReady);
    Assert(Positions);
    Assert(TexCoords);
    Assert(Colors);
    Assert(VertexCount > 0);
    Assert(VertexCount < GlState->MaxVertexCount);
    Assert(IndexCount < GlState->MaxIndexCount);
    
    glBindBuffer(GL_ARRAY_BUFFER, GlState->DefaultVBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, VertexCount*sizeof(vec3), Positions);
    glBufferSubData(GL_ARRAY_BUFFER, GlState->MaxVertexCount*sizeof(vec3), VertexCount*sizeof(vec4), TexCoords);
    glBufferSubData(GL_ARRAY_BUFFER, GlState->MaxVertexCount*(sizeof(vec3)+sizeof(vec4)), VertexCount*sizeof(vec4), Colors);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GlState->DefaultEBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, IndexCount * sizeof(float), Indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glBindVertexArray(GlState->DefaultVAO);
    glDrawElements(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void
FlipTexCoords(vec2 *TexCoords)
{
    for (int i = 0; i < 4; i++)
    {
        TexCoords[i].Y = 1.0f - TexCoords[i].Y;
    }
}

void
NormalizeTexCoords(sav_texture Texture, vec2 *TexCoords)
{
    f32 ooWidth = 1.0f / Texture.Width;
    f32 ooHeight = 1.0f / Texture.Height;
    
    for (int i = 0; i < 4; i++)
    {
        TexCoords[i].X *= ooWidth;
        TexCoords[i].Y *= ooHeight;
    }
}

void
GetTexCoordsForTex(sav_texture Texture, rect R, vec2 *TexCoords)
{
    RectGetPoints(R, TexCoords);
    NormalizeTexCoords(Texture, TexCoords);
    FlipTexCoords(TexCoords);
}

void
SavSwapBuffers()
{
    SDL_GL_SwapWindow(gSdlState.Window);
}

void
DrawTexture(sav_texture Texture, rect Dest, rect Source, vec2 Origin, f32 Rotation, color Color)
{
    vec3 AbsOrigin = Vec3(Dest.X, Dest.Y, 0.0f);
    
    Dest.X -= Origin.X;
    Dest.Y -= Origin.Y;
    
    vec3 Positions[4];
    RectGetPoints(Dest, Positions);
    if (Rotation != 0.0f)
    {
        Rotate4PointsAroundOrigin(Positions, AbsOrigin, Rotation);
    }
    vec2 TexCoords[4];
    GetTexCoordsForTex(Texture, Source, TexCoords);
    vec4 TexCoordsV4[4];
    for (int i = 0; i < ArrayCount(TexCoords); i++)
    {
        TexCoordsV4[i] = Vec4(TexCoords[i], 0, 0);
    }
    vec4 C = ColorV4(Color);
    vec4 Colors[] = { C, C, C, C };
    u32 Indices[] = { 0, 1, 2, 2, 3, 0 };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture.Glid);

    gl_state *GlState = &gGlState;
    DrawVertices(Positions, TexCoordsV4, Colors, Indices, ArrayCount(Positions), ArrayCount(Indices));

    glBindTexture(GL_TEXTURE_2D, gGlState.DefaultTextureGlid);
}

void
DrawRect(rect Rect, color Color)
{
    vec3 Positions[4];
    RectGetPoints(Rect, Positions);
    vec4 TexCoords[4] = {};
    vec4 C = ColorV4(Color);
    vec4 Colors[] = { C, C, C, C };
    u32 Indices[] = { 0, 1, 2, 2, 3, 0 };

    DrawVertices(Positions, TexCoords, Colors, Indices, ArrayCount(Positions), ArrayCount(Indices));
}

void
BeginCameraMode(camera_2d *Camera)
{
    SetModelViewMatrix(Mat4GetCamera2DView(Camera->Target, Camera->Zoom, Camera->Rotation, Camera->Offset));
}

void
EndCameraMode()
{
    SetModelViewMatrix(Mat4(1.0f));
}

void
SetProjectionMatrix(mat4 Projection)
{
    gl_state *GlState = &gGlState;
    GlState->Projection = Projection;
    mat4 MVP = GlState->Projection * GlState->ModelView;
    SetUniformMat4("mvp", &MVP.E[0][0]);
}

void
SetModelViewMatrix(mat4 ModelView)
{
    gl_state *GlState = &gGlState;
    GlState->ModelView = ModelView;
    mat4 MVP = GlState->Projection * GlState->ModelView;
    SetUniformMat4("mvp", &MVP.E[0][0]);
}

vec2
CameraWorldToScreen(camera_2d *Camera, vec2 World)
{
    mat4 View = Mat4GetCamera2DView(Camera->Target, Camera->Zoom, Camera->Rotation, Camera->Offset);
    vec4 Result = View * Vec4(World, 0.0f, 1.0f);
    return Vec2(Result);
}

vec2
CameraScreenToWorld(camera_2d *Camera, vec2 Screen)
{
    mat4 ViewInv = Mat4GetCamera2DViewInv(Camera->Target, Camera->Zoom, Camera->Rotation, Camera->Offset);
    vec4 Result = ViewInv * Vec4(Screen, 0.0f, 1.0f);
    return Vec2(Result);
}

vec2
CameraScreenToWorldRel(camera_2d *Camera, vec2 Screen)
{
    mat4 ViewInvRel = Mat4GetCamera2DViewInvRel(Camera->Zoom, Camera->Rotation);
    vec4 Result = ViewInvRel * Vec4(Screen, 0.0f, 1.0f);
    return Vec2(Result);
}

vec2
ScreenToRectCoords(rect ScreenR, f32 ScaledW, f32 ScaledH, vec2 ScreenCoords)
{
    sdl_state *SdlState = &gSdlState;

    vec2 RectCoords = Vec2(((ScreenCoords.X - ScreenR.X) / ScreenR.Width) * ScaledW,
                           ((ScreenCoords.Y - ScreenR.Y) / ScreenR.Height) * ScaledH);
    
    return RectCoords;
}

vec2
RectToScreenCoords(rect ScreenR, f32 ScaledW, f32 ScaledH, vec2 RectCoords)
{
    sdl_state *SdlState = &gSdlState;

    vec2 ScreenCoords = Vec2((RectCoords.X / ScaledW) * ScreenR.Width + ScreenR.X,
                             (RectCoords.Y / ScaledH) * ScreenR.Height + ScreenR.Y);
    
    return ScreenCoords;
}

inline f32
ExponentialInterpolation(f32 Min, f32 Max, f32 T)
{
    f32 LogMin = log(Min);
    f32 LogMax = log(Max);
    f32 Lerp = LogMin + (LogMax - LogMin) * T;
    
    f32 Result = exp(Lerp);
    return Result;
}

inline f32
ExponentialInterpolationWhereIs(f32 Min, f32 Max, f32 V)
{
    f32 LogMin = log(Min);
    f32 LogMax = log(Max);
    f32 LogV = log(V);

    f32 Result;
    
    if (LogV >= LogMax)
    {
        Result = 1.0f;
    }
    else if (LogV <= LogMin)
    {
        Result = 0.0f;
    }
    else
    {
        Result = (LogV - LogMin) / (LogMax - LogMin);
    }
    
    return Result;
}

void
CameraIncreaseLogZoom(camera_2d *Camera, f32 Delta)
{
    Camera->ZoomLog += Delta;
    if (Camera->ZoomLog > 1.0f)
    {
        Camera->ZoomLog = 1.0f;
    }
    else if (Camera->ZoomLog < 0.0f)
    {
        Camera->ZoomLog = 0.0f;
    }

    Camera->Zoom = ExponentialInterpolation(Camera->ZoomMin, Camera->ZoomMax, Camera->ZoomLog);
}

void
CameraInitLogZoomSteps(camera_2d *Camera, f32 Min, f32 Max, int StepCount)
{
    Assert(StepCount > 1 && StepCount < CAMERA_MAX_ZOOM_STEPS);
    
    f32 StepDelta = 1.0f / (StepCount - 1);
    f32 LogZoomNeutral = ExponentialInterpolationWhereIs(Min, Max, 1.0f);

    int iClosestToNeutral = 0;
    f32 DistToNeutral = FLT_MAX;

    f32 CurrentDelta = 0.0f;
    for (int i = 0; i < StepCount; i++)
    {
        f32 CurrDistToNeutral = AbsF(CurrentDelta - LogZoomNeutral);
        if (CurrDistToNeutral < DistToNeutral)
        {
            DistToNeutral = CurrDistToNeutral;
            iClosestToNeutral = i;
        }
        
        Camera->ZoomLogSteps[Camera->ZoomLogStepsCount++] = CurrentDelta;
        CurrentDelta += StepDelta;
    }

    Camera->ZoomMin = Min;
    Camera->ZoomMax = Max;
    Camera->ZoomLogStepsCurrent = iClosestToNeutral;
    Camera->ZoomLogSteps[iClosestToNeutral] = LogZoomNeutral;
    CameraIncreaseLogZoomSteps(Camera, 0);
}

void
CameraIncreaseLogZoomSteps(camera_2d *Camera, int Steps)
{
    Camera->ZoomLogStepsCurrent += Steps;
    if (Camera->ZoomLogStepsCurrent < 0) Camera->ZoomLogStepsCurrent = 0;
    if (Camera->ZoomLogStepsCurrent > (Camera->ZoomLogStepsCount - 1)) Camera->ZoomLogStepsCurrent = (Camera->ZoomLogStepsCount - 1);

    Camera->Zoom = ExponentialInterpolation(Camera->ZoomMin, Camera->ZoomMax, Camera->ZoomLogSteps[Camera->ZoomLogStepsCurrent]);
}

// SECTION: Image/texture loading
sav_image
SavLoadImage(const char *Path)
{
    SDL_Surface *OriginalSurface = IMG_Load(Path);
    if (!OriginalSurface)
    {
        const char *Error = SDL_GetError();
        TraceLog("Could not load image at %s:\n%s", Path, Error);
        InvalidCodePath;
    }
    else
    {
        TraceLog("Loaded image at %s", Path);
    }
    
    SDL_Surface *RGBASurface = SDL_ConvertSurfaceFormat(OriginalSurface, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(OriginalSurface);

    if (RGBASurface->pitch != RGBASurface->format->BytesPerPixel * RGBASurface->w)
    {
        TraceLog("SDL Loaded image, but format could not be converted.");
        InvalidCodePath;
    }

    // NOTE: Flip rows of pixels, because opengl expects textures to have bottom rows first
    {
        SDL_Surface *Surface = RGBASurface;
        char *Pixels = (char *) Surface->pixels;
        for (int y = 0; y < Surface->h / 2; y++)
        {
            for (int x = 0; x < Surface->pitch; x++)
            {
                int oppY = Surface->h - y  - 1;
                char Temp = Pixels[oppY * Surface->pitch + x];
                Pixels[oppY * Surface->pitch + x] = Pixels[y * Surface->pitch + x];
                Pixels[y * Surface->pitch + x] = Temp;
            }
        }
    }

    sav_image Image = {};
    Image.Data = RGBASurface->pixels;
    Image.Width = RGBASurface->w;
    Image.Height = RGBASurface->h;
    Image.Pitch = RGBASurface->pitch;
    Image._dataToFree = RGBASurface;
    return Image;
}

void
SavFreeImage(sav_image *Image)
{
    SDL_FreeSurface((SDL_Surface *) Image->_dataToFree);
    Image->Data = 0;
    Image->_dataToFree = 0;
}

void
SavSaveImage(const char *Path, void *Data, int Width, int Height, b32 Flip, u32 RMask, u32 GMask, u32 BMask, u32 AMask)
{
    SDL_Surface *Surface = SDL_CreateRGBSurfaceFrom(Data,
                                                    Width,
                                                    Height,
                                                    32,
                                                    Width * 4,
                                                    RMask,
                                                    GMask,
                                                    BMask,
                                                    AMask);

    // NOTE: Flip rows of pixels. Opengl gives pixels starting from left bottom corner
    if (Flip)
    {
        char *Pixels = (char *) Surface->pixels;
        for (int y = 0; y < Surface->h / 2; y++)
        {
            for (int x = 0; x < Surface->pitch; x++)
            {
                int oppY = Surface->h - y  - 1;
                char Temp = Pixels[oppY * Surface->pitch + x];
                Pixels[oppY * Surface->pitch + x] = Pixels[y * Surface->pitch + x];
                Pixels[y * Surface->pitch + x] = Temp;
            }
        }
    }
    
    if (IMG_SavePNG(Surface, Path) != 0)
    {
        TraceLog("SDL failed to save image to %s", Path);
    }
}

sav_texture
SavLoadTexture(const char *Path)
{
    sav_image Image = SavLoadImage(Path);
    sav_texture Texture = SavLoadTextureFromImage(Image);
    SavFreeImage(&Image);
    return Texture;
}

sav_texture
SavLoadTextureFromImage(sav_image Image)
{
    return SavLoadTextureFromData(Image.Data, Image.Width, Image.Height);
}

sav_texture
SavLoadTextureFromData(void *Data, int Width, int Height)
{
    u32 Glid;
    glGenTextures(1, &Glid);
    Assert(Glid);
    glBindTexture(GL_TEXTURE_2D, Glid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, Data);

    // TODO: Set these dynamically
    // glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // ---------
    glBindTexture(GL_TEXTURE_2D, 0);

    sav_texture Texture = {};
    Texture.Glid = Glid;
    Texture.Width = Width;
    Texture.Height = Height;
    return Texture;
}

void
SavSetTextureWrapMode(sav_texture Texture, tex_wrap_mode WrapMode)
{
    glBindTexture(GL_TEXTURE_2D, Texture.Glid);
    switch (WrapMode)
    {
        case SAV_CLAMP_TO_EDGE:
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } break;

        case SAV_REPEAT:
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        } break;

        default: break;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void
SavSetTextureFilterMode(sav_texture Texture, tex_filter_mode FilterMode)
{
    glBindTexture(GL_TEXTURE_2D, Texture.Glid);
    switch (FilterMode)
    {
        case SAV_LINEAR:
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        } break;

        case SAV_NEAREST:
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } break;

        default: break;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

sav_render_texture
SavLoadRenderTexture(int Width, int Height, b32 FilterNearest)
{
    u32 FBO;
    u32 Texture;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    GLenum FilterType = FilterNearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilterType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FilterType);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture, 0);

    u32 DepthRBO;
    glGenRenderbuffers(1, &DepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, DepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width,  Height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, DepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        TraceLog("GL Framebuffer is not complete.");
        InvalidCodePath;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    sav_render_texture RenderTexture;
    RenderTexture.FBO = FBO;
    RenderTexture.DepthRBO = DepthRBO;
    RenderTexture.Texture.Glid = Texture;
    RenderTexture.Texture.Width = Width;
    RenderTexture.Texture.Height = Height;
    return RenderTexture;
}

void
SavDeleteRenderTexture(sav_render_texture *RenderTexture)
{
    glDeleteTextures(1, &RenderTexture->Texture.Glid);
    glDeleteRenderbuffers(1, &RenderTexture->DepthRBO);
    glDeleteFramebuffers(1, &RenderTexture->FBO);

    RenderTexture->FBO = 0;
    RenderTexture->DepthRBO = 0;
    RenderTexture->Texture.Glid = 0;
    RenderTexture->Texture.Width = 0;
    RenderTexture->Texture.Height = 0;
}

void
BeginTextureMode(sav_render_texture RenderTexture, rect RenderTextureScreenRect)
{
    glBindFramebuffer(GL_FRAMEBUFFER, RenderTexture.FBO);
    glViewport(0, 0, RenderTexture.Texture.Width, RenderTexture.Texture.Height);
    SetProjectionMatrix(Mat4GetOrthographicProjection(0.0f,
                                                      (f32) RenderTexture.Texture.Width,
                                                      (f32) RenderTexture.Texture.Height,
                                                      0.0f, -1.0f, 1.0f));
    gl_state *GlState = &gGlState;
    GlState->RenderTextureActive = true;
    GlState->CurrentRenderTextureScreenRect = RenderTextureScreenRect;
    GlState->CurrentRenderTexture = RenderTexture;
    GlState->DrawReady = true;
}

void
EndTextureMode()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gGlState.RenderTextureActive = false;
    gGlState.DrawReady = false;
}

// SECTION: Fonts and text rendering
u32
LoadTextureFromFont(void *Data, u32 Width, u32 Height)
{
    u32 Glid;
    glGenTextures(1, &Glid);
    Assert(Glid);
    glBindTexture(GL_TEXTURE_2D, Glid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, Data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return Glid;
}

sav_font *
SavLoadFont(memory_arena *Arena, const char *Path, u32 PointSize)
{
    // NOTE: Allocate memory for the font info and glyph infos
    sav_font *Font = MemoryArena_PushStruct(Arena, sav_font);
    *Font = {};

    Font->GlyphCount = 128;
    Font->GlyphInfos = MemoryArena_PushArray(Arena, Font->GlyphCount, glyph_info);

    // NOTE: Allocate temp memory for the rendered glyphs (just the pointers, image bytes are handled by SDL)
    MemoryArena_Freeze(Arena);
    SDL_Surface **GlyphImages = MemoryArena_PushArray(Arena, Font->GlyphCount, SDL_Surface *);
    for (int GlyphIndex = 0;
         GlyphIndex < Font->GlyphCount;
         ++GlyphIndex)
    {
        Font->GlyphInfos[GlyphIndex] = {};
        GlyphImages[GlyphIndex] = {};
    }

    // NOTE: Use platform layer to load font
    TTF_Font *SdlFont = TTF_OpenFont(Path, PointSize);
    if (SdlFont == 0)
    {
        const char *Error = SDL_GetError();
        TraceLog("SDL failed to load ttf font at %s:\n%s", Path, Error);
        return 0;
    }
    Font->PointSize = (f32)PointSize;
    Font->Height = TTF_FontHeight(SdlFont);
    int BytesPerPixel = 4;

    // NOTE: Render each glyph into an image, get glyph metrics and get max glyph width to allocate bytes for the atlas
    int MaxGlyphWidth = 0;
    int MaxGlyphHeight = 0;
    for (u8 GlyphChar = 32;
         GlyphChar < Font->GlyphCount;
         ++GlyphChar)
    {
        glyph_info *GlyphInfo = Font->GlyphInfos + GlyphChar;

        TTF_GlyphMetrics(SdlFont, (char) GlyphChar,
                                 &GlyphInfo->MinX, &GlyphInfo->MaxX, &GlyphInfo->MinY, &GlyphInfo->MaxY, &GlyphInfo->Advance);

        SDL_Surface *GlyphImage = TTF_RenderGlyph_Blended(SdlFont, (char) GlyphChar, SDL_Color { 255, 255, 255, 255 });
        if (GlyphImage->w > MaxGlyphWidth)
        {
            MaxGlyphWidth = GlyphImage->w;
        }
        if (GlyphImage->h > MaxGlyphHeight)
        {
            MaxGlyphHeight = GlyphImage->h;
        }
        Assert(GlyphImage->format->BytesPerPixel == BytesPerPixel);

        GlyphImages[GlyphChar] = GlyphImage;
    }

    //
    // NOTE: Allocate memory for the font atlas
    //
    
    // NOTE: 12x8 = 96 -> for the 95 visible glyphs
    u32 AtlasColumns = 12;
    u32 AtlasRows = 8;
    u32 AtlasPxWidth = AtlasColumns * MaxGlyphWidth;
    u32 AtlasPxHeight = AtlasRows * MaxGlyphHeight;
    u32 AtlasPitch = BytesPerPixel * AtlasPxWidth;
    u8 *AtlasBytes = MemoryArena_PushArrayAndZero(Arena, AtlasPitch * AtlasPxHeight, u8);
    
    // NOTE: Blit each glyph surface to the atlas surface
    u32 CurrentGlyphIndex = 0;
    for (int GlyphChar = 32;
         GlyphChar < Font->GlyphCount;
         ++GlyphChar)
    {
        SDL_Surface *GlyphImage = GlyphImages[GlyphChar];
        Assert(GlyphImage->w > 0);
        Assert(GlyphImage->w <= MaxGlyphWidth);
        Assert(GlyphImage->h > 0);
        Assert(GlyphImage->h <= MaxGlyphHeight);
        Assert(GlyphImage->format->BytesPerPixel == 4);
        Assert(GlyphImage->pixels);
        
        glyph_info *GlyphInfo = Font->GlyphInfos + GlyphChar;

        u32 CurrentAtlasCol = CurrentGlyphIndex % AtlasColumns;
        u32 CurrentAtlasRow = CurrentGlyphIndex / AtlasColumns;
        u32 AtlasPxX = CurrentAtlasCol * MaxGlyphWidth;
        u32 AtlasPxY = CurrentAtlasRow * Font->Height;
        size_t AtlasByteOffset = (AtlasPxY * AtlasPxWidth + AtlasPxX) * BytesPerPixel;

        // NOTE: Hack solution to texture bleed in some fonts. 127 is DEL char, some fonts put a big rectangle for that, that fills
        //       the whole glyph height, and can bleed into surrounding glyphs.
        if (GlyphChar != 127)
        {
            u8 *Dest = AtlasBytes + AtlasByteOffset;
            u8 *Source = (u8 *) GlyphImage->pixels;
            for (int GlyphPxY = 0;
                 GlyphPxY < GlyphImage->h;
                 ++GlyphPxY)
            {
                u8 *DestByte = (u8 *) Dest;
                u8 *SourceByte = (u8 *) Source;
            
                for (int GlyphPxX = 0;
                     GlyphPxX < GlyphImage->w;
                     ++GlyphPxX)
                {
                    for (int PixelByte = 0;
                         PixelByte < BytesPerPixel;
                         ++PixelByte)
                    {
                        *DestByte++ = *SourceByte++;
                    }
                }

                Dest += AtlasPitch;
                Source += GlyphImage->pitch;
            }
        }

        // NOTE:Use the atlas position and width/height to calculate UVs for each glyph
        // NOTE: It seems that SDL_ttf embeds MinX into the rendered glyph, but also it's ignored if it's less than 0
        // Need to shift where to place glyph if MinX is negative, but if not negative, it's already included
        // in the rendered glyph. This works but seems very finicky
        u32 GlyphTexWidth = ((GlyphInfo->MinX >= 0) ? (GlyphInfo->MaxX) : (GlyphInfo->MaxX - GlyphInfo->MinX));
        u32 GlyphTexHeight = MaxGlyphHeight;
        
        f32 OneOverAtlasPxWidth = 1.0f / (f32) AtlasPxWidth;
        f32 OneOverAtlasPxHeight = 1.0f / (f32) AtlasPxHeight;
        f32 UVLeft = (f32) AtlasPxX * OneOverAtlasPxWidth;
        f32 UVTop = (f32) AtlasPxY * OneOverAtlasPxHeight;
        f32 UVRight = (f32) (AtlasPxX + GlyphTexWidth) * OneOverAtlasPxWidth;
        f32 UVBottom = (f32) (AtlasPxY + GlyphTexHeight) * OneOverAtlasPxHeight;
        GlyphInfo->GlyphUVs[0] = Vec2(UVLeft, UVTop);
        GlyphInfo->GlyphUVs[1] = Vec2(UVLeft, UVBottom);
        GlyphInfo->GlyphUVs[2] = Vec2(UVRight, UVBottom);
        GlyphInfo->GlyphUVs[3] = Vec2(UVRight, UVTop);

        SDL_FreeSurface(GlyphImage);

        CurrentGlyphIndex++;
    }

    Font->AtlasGlid = LoadTextureFromFont(AtlasBytes, AtlasPxWidth, AtlasPxHeight);

    simple_string TempFileName = GetFilenameFromPath(Path, false);
    simple_string TempFile = SimpleStringF("%s-%.0f.png", TempFileName.D, Font->PointSize);
    simple_string SaveToPath = CatStrings("temp/", TempFile.D);
    SavSaveImage(SaveToPath.D, AtlasBytes, AtlasPxWidth, AtlasPxHeight, false, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    MemoryArena_Unfreeze(Arena);
    TTF_CloseFont(SdlFont);
    
    return Font;
}

void
DrawString(const char *String,
           sav_font *Font, f32 PointSize, f32 X, f32 Y, f32 MaxWidth,
           color Color, b32 DrawBg, color BgColor,
           memory_arena *TransientArena)
{
    f32 SizeRatio = PointSize / Font->PointSize;

    int StringCount;
    for (StringCount = 0;
         String[StringCount] != '\0';
         ++StringCount) {}
    
    MemoryArena_Freeze(TransientArena);

    u8 *WrapPoints = MemoryArena_PushArrayAndZero(TransientArena, StringCount, u8);
    
    if (MaxWidth > 0.0f)
    {
        int PrevSpaceI = 0;
        f32 WrapXAccum = 0.0f;
        for (int StringI = 0; StringI < StringCount; StringI++)
        {
            char Glyph = String[StringI];

            glyph_info *GlyphInfo = Font->GlyphInfos + Glyph;

            WrapXAccum += SizeRatio * GlyphInfo->Advance;

            if (Glyph == ' ')
            {
                PrevSpaceI = StringI;
            }

            if (WrapXAccum > MaxWidth)
            {
                if (PrevSpaceI != 0)
                {
                    WrapPoints[PrevSpaceI] = 1;
                    StringI = PrevSpaceI;
                    WrapXAccum = 0.0f;
                    PrevSpaceI = 0;
                }
                else
                {
                    WrapPoints[StringI] = 2;
                    WrapXAccum = 0.0f;
                }
            }
        }
    }
    
    f32 CurrentX = X;
    f32 MaxX = X;
    f32 CurrentY = Y;

    int StringVisibleCount = 0;
    for (int StringI = 0; StringI < StringCount; ++StringI)
    {
        if (String[StringI] != '\n' && WrapPoints[StringI] != 1)
        {
            StringVisibleCount++;
        }
    }

    int VertexCount = StringVisibleCount * 4;
    int IndexCount = StringVisibleCount * 6;
    vec3 *Vertices = MemoryArena_PushArray(TransientArena, VertexCount, vec3);
    vec4 *Colors = MemoryArena_PushArray(TransientArena, VertexCount, vec4);
    vec4 *TexCoords = MemoryArena_PushArray(TransientArena, VertexCount, vec4); 
    u32 *Indices = MemoryArena_PushArray(TransientArena, IndexCount, u32);

    int CurrentVertexIndex = 0;
    int CurrentTexCoordIndex = 0;
    int CurrentColorIndex = 0;
    int CurrentIndexIndex = 0;
    for (int StringIndex = 0;
         StringIndex < StringCount;
         ++StringIndex)
    {
        char Glyph = String[StringIndex];

        f32 sHeight = SizeRatio * Font->Height;
        
        if (Glyph == '\n' || WrapPoints[StringIndex] > 0)
        {
            if (CurrentX > MaxX)
            {
                MaxX = CurrentX;
            }
            CurrentX = X;
            CurrentY += sHeight;
            if (WrapPoints[StringIndex] != 2)
            {
                continue;
            }
        }

        Assert(Glyph >= 32 && Glyph < Font->GlyphCount);
        glyph_info *GlyphInfo = Font->GlyphInfos + Glyph;

        f32 sMinX = SizeRatio * GlyphInfo->MinX;
        f32 sMaxX = SizeRatio * GlyphInfo->MaxX;

        f32 PxX = ((sMinX >= 0) ? CurrentX : (CurrentX + sMinX));
        f32 PxY = CurrentY;
        f32 PxWidth = (f32) ((sMinX >= 0) ? sMaxX : (sMaxX - sMinX));
        f32 PxHeight = sHeight;

        int BaseVertexIndex = CurrentVertexIndex;
        Vertices[CurrentVertexIndex++] = Vec3(PxX, PxY, 0);
        Vertices[CurrentVertexIndex++] = Vec3(PxX, PxY + PxHeight, 0);
        Vertices[CurrentVertexIndex++] = Vec3(PxX + PxWidth, PxY + PxHeight, 0);
        Vertices[CurrentVertexIndex++] = Vec3(PxX + PxWidth, PxY, 0);

        vec4 C = ColorV4(Color);
        Colors[CurrentColorIndex++] = C;
        Colors[CurrentColorIndex++] = C;
        Colors[CurrentColorIndex++] = C;
        Colors[CurrentColorIndex++] = C;

        for (int GlyphUVIndex = 0;
             GlyphUVIndex < 4;
             ++GlyphUVIndex)
        {
            TexCoords[CurrentTexCoordIndex++] = Vec4(GlyphInfo->GlyphUVs[GlyphUVIndex], 0, 0);
        }

        u32 IndicesToCopy[] = {
            0, 1, 3,  3, 1, 2
        };

        for (int IndexToCopyIndex = 0;
             IndexToCopyIndex < ArrayCount(IndicesToCopy);
             ++IndexToCopyIndex)
        {
            Indices[CurrentIndexIndex++] = BaseVertexIndex + IndicesToCopy[IndexToCopyIndex];
        }

        CurrentX += SizeRatio * GlyphInfo->Advance;
    }
    Assert(CurrentVertexIndex == VertexCount);
    Assert(CurrentColorIndex == VertexCount);
    Assert(CurrentTexCoordIndex == VertexCount);
    Assert(CurrentIndexIndex == IndexCount);

    gl_state *GlState = &gGlState;
    
    if (DrawBg)
    {
        f32 Pad = 2.0f;
        f32 PxLeft = Max(0, X - Pad);
        f32 PxTop = Max(0, Y - Pad);
        MaxX = Max(CurrentX, MaxX);
        f32 PxRight = MaxX + Pad;
        f32 PxBottom = CurrentY + SizeRatio * Font->Height + Pad;

        vec3 BgVertices[4];
        BgVertices[0] = Vec3(PxLeft, PxTop, 0);
        BgVertices[1] = Vec3(PxLeft, PxBottom, 0);
        BgVertices[2] = Vec3(PxRight, PxBottom, 0);
        BgVertices[3] = Vec3(PxRight, PxTop, 0);

        vec4 C = ColorV4(BgColor);
        vec4 BgColors[4];
        BgColors[0] = C;
        BgColors[1] = C;
        BgColors[2] = C;
        BgColors[3] = C;

        vec4 BgTexCoords[4] = {};

        u32 BgIndices[] = {
            0, 1, 3,  3, 1, 2
        };
        DrawVertices(BgVertices, BgTexCoords, BgColors, BgIndices, ArrayCount(BgVertices), ArrayCount(BgIndices));
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Font->AtlasGlid);

    DrawVertices(Vertices, TexCoords, Colors, Indices, VertexCount, IndexCount);

    glBindTexture(GL_TEXTURE_2D, gGlState.DefaultTextureGlid);

    MemoryArena_Unfreeze(TransientArena);
}

// SECTION: GUI
b32
CheckPointInRect(vec2 P, rect R)
{
    vec2 MinR = RectGetMin(R);
    vec2 MaxR = RectGetMax(R);
    return (P.X > MinR.X && P.Y > MinR.Y && P.X < MaxR.X && P.Y < MaxR.Y);
}

b32
GuiButtonRect(rect R)
{
    vec2 MouseP = GetMouseLogicalPos();

    b32 MouseInRect = CheckPointInRect(MouseP, R);
    if (MouseInRect)
    {
        color C = ColorAlpha(VA_BLACK, 128);
        if (MouseDown(SDL_BUTTON_LEFT))
        {
            C = ColorAlpha(VA_BLACK, 200);
        }
        DrawRect(R, C);
        return MouseReleased(SDL_BUTTON_LEFT);
    }

    return false;
}

// SECTION: File IO
char *
SavReadTextFile(const char *Path)
{
    FILE *File;
    fopen_s(&File, Path, "rb");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        Assert(FileSize > 0);
        fseek(File, 0, SEEK_SET);

        char *Result = (char *) malloc(FileSize + 1);
        if (Result)
        {
            size_t ElementsRead = fread(Result, FileSize, 1, File);
            Assert(ElementsRead == 1);
            Result[FileSize] = '\0';

            fclose(File);

            return Result;
        }
        else
        {
            TraceLog("Failed to alloc memory when reading file at %s", Path);
        }
    }
    else
    {
        TraceLog("Failed to open file at %s", Path);
    }

    return 0;
}

void
SavFreeString(char **Text)
{
    free(*Text);
    *Text = 0;
}


// SECTION: Misc
const char *
TextFormat(const char *Format, ...)
{
    static_p char Result[STRING_BUFFER];
    
    va_list VarArgs;
    va_start(VarArgs, Format);
    vsprintf_s(Result, STRING_BUFFER - 1, Format, VarArgs);
    va_end(VarArgs);

    return Result;
}

void
TraceLog(const char *Format, ...)
{
    char FormatBuf[STRING_BUFFER];
    sprintf_s(FormatBuf, "[F %06zu] %s\n", gCurrentFrame, Format);
    
    va_list VarArgs;
    va_start(VarArgs, Format);
    vprintf_s(FormatBuf, VarArgs);
    va_end(VarArgs);
}

int
GetRandomValue(int Min, int Max)
{
    return (Min + rand() % (Max - Min));
}

f32
GetRandomFloat()
{
    return (rand() / (f32) RAND_MAX);
}
