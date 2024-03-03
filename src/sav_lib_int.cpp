#ifdef SAV_LIB_INT

#define STRING_BUFFER 1024
#define TIMING_STAT_AVG_COUNT 32

struct sdl_state
{
    SDL_Window *Window;
    vec2 WindowSize;
    vec2 WindowOrigSize;
    b32 WindowSizeChanged;

    b32 Borderless;
    rect WindowRectBeforeBorderless;

    u64 PerfCounterFreq;
    u64 LastCounter;
    f64 PrevDelta;
    f64 DeltaSamples[TIMING_STAT_AVG_COUNT];
    int CurrentTimingStatSample;
    f64 AvgDelta;

    UINT DesiredSchedulerMS;
    b32 SleepIsGranular;
    b32 LimitFPS;
    f64 TargetFPS;
    f64 TargetDelta;
};

struct win32_state
{
    HANDLE DumpMemoryFileHandle;
    HANDLE DumpMemoryMap;
    void *DumpMemoryBlock;
};

// TODO: Technically all these could be flags in one u8 array. But I'm not sure if it's slower to do bitwise. Need to profile.
struct input_state
{
    u8 CurrentKeyStates[SDL_NUM_SCANCODES];
    u8 PreviousKeyStates[SDL_NUM_SCANCODES];
    u8 RepeatKeyStates[SDL_NUM_SCANCODES];

    vec2 MousePos;
    vec2 MouseRelPos;

    b32 IsRelMouse;

    u8 CurrentMouseButtonStates[SDL_BUTTON_X2 + 1];
    u8 PreviousMouseButtonStates[SDL_BUTTON_X2 + 1];
    u8 ClickMouseButtonStates[SDL_BUTTON_X2 + 1];

    i32 MouseWheel;
};

struct game_code
{
    b32 IsValid;
    HMODULE Dll;
    FILETIME LastWriteTime;

    simple_string SourceDllPath;
    simple_string TempDllPath;
    simple_string LockFilePath;

    simple_string FuncName;
    void *UpdateAndRenderFunc;
};

struct gl_state
{
    u32 DefaultShader;
    u32 DefaultVBO;
    u32 DefaultVAO;
    u32 DefaultEBO;
    int MaxVertexCount;
    int MaxIndexCount;

    u32 DefaultTextureGlid;

    mat4 Projection;
    mat4 ModelView;

    b32 RenderTextureActive;
    rect CurrentRenderTextureScreenRect;
    sav_render_texture CurrentRenderTexture;

    b32 DrawReady;

    u32 CurrentShader;
};

// SECTION: Internal state
static_g sdl_state gSdlState;
static_g win32_state gWin32State;
static_g gl_state gGlState;
static_g input_state gInputState;
static_g game_code gGameCode;
static_g u64 gCurrentFrame;

// SECTION: Internal functions header

static_i u32 BuildBasicShader();

#endif
