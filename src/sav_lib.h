#ifndef SAV_LIB_H
#define SAV_LIB_H

#include "va_types.h"
#include "va_memarena.h"
#include "va_linmath.h"

#ifdef SAV_EXPORTS
#define SAV_API extern "C" __declspec(dllexport)
#else
#define SAV_API extern "C" __declspec(dllimport)
#endif

#define GAME_API extern "C" __declspec(dllexport)

struct game_memory
{
    void *Data;
    size_t Size;
};

struct music_stream
{
    void *Music;
};

struct sound_chunk
{
    void *Sound;
};

struct sav_image
{
    void *Data;

    int Width;
    int Height;
    int Pitch;

    void *_dataToFree;
};

struct sav_texture
{
    u32 Glid;
    int Width;
    int Height;
};

enum { CAMERA_MAX_ZOOM_STEPS = 16 };

struct camera_2d
{
    vec2 Target;
    vec2 Offset;
    f32 Rotation;
    f32 Zoom;

    f32 ZoomMin;
    f32 ZoomMax;
    f32 ZoomLog;
    
    f32 ZoomLogSteps[CAMERA_MAX_ZOOM_STEPS];
    int ZoomLogStepsCurrent;
    int ZoomLogStepsCount;
};

struct glyph_info
{
    vec2 GlyphUVs[4];

    int MinX;
    int MaxX;
    int MinY;
    int MaxY;
    int Advance;
};

struct sav_font
{
    int GlyphCount;
    glyph_info *GlyphInfos;

    u32 AtlasGlid;
    f32 PointSize;
    int Height;
};

struct sav_render_texture
{
    u32 FBO;
    u32 DepthRBO;

    sav_texture Texture;
};

struct sav_shader
{
    u32 Glid;
};

enum tex_wrap_mode
{
    SAV_CLAMP_TO_EDGE,
    SAV_REPEAT
};

enum tex_filter_mode
{
    SAV_LINEAR,
    SAV_NEAREST
};

SAV_API game_memory AllocGameMemory(size_t Size);
SAV_API void DumpGameMemory(game_memory GameMemory);
SAV_API void ReloadGameMemoryDump(game_memory GameMemory);

SAV_API b32 InitGameCode(const char *DllPath, const char *FuncName, void **UpdateAndRenderFunc);
SAV_API b32 ReloadGameCode(void **UpdateAndRenderFunc);

SAV_API b32 InitWindow(const char *Name, int Width, int Height);
SAV_API void PollEvents(b32 *Quit);
SAV_API void Quit();
SAV_API void SetTargetFPS(f64 FPS);
SAV_API void BeginFrameTiming();
SAV_API void EndFrameTiming();

SAV_API void SetWindowTitle(const char *Title);
SAV_API vec2 GetWindowSize();
SAV_API vec2 GetWindowOrigSize();
SAV_API b32 WindowSizeChanged();
SAV_API void SetWindowBorderless(b32 Borderless);
SAV_API void ToggleWindowBorderless();

SAV_API b32 KeyDown(int Key);
SAV_API b32 KeyPressed(int Key);
SAV_API b32 KeyReleased(int Key);
SAV_API b32 KeyRepeat(int Key);
SAV_API b32 KeyPressedOrRepeat(int Key);
SAV_API b32 GetMouseRelativeMode();
SAV_API void SetMouseRelativeMode(b32 Enabled);
SAV_API vec2 GetMousePos();
SAV_API vec2 GetMouseRelPos();
SAV_API b32 MouseDown(int Button);
SAV_API b32 MousePressed(int Button);
SAV_API b32 MouseReleased(int Button);
SAV_API b32 MouseClicks(int Button, int Clicks);
SAV_API i32 MouseWheel();

SAV_API u64 GetCurrentFrame();
SAV_API f64 GetDeltaFixed();
SAV_API f64 GetDeltaPrev();
SAV_API f64 GetDeltaAvg();
SAV_API f64 GetFPSPrev();
SAV_API f64 GetFPSAvg();

SAV_API b32 InitAudio();
SAV_API music_stream LoadMusicStream(const char *FilePath);
SAV_API sound_chunk LoadSoundChunk(const char *FilePath);
SAV_API b32 PlayMusicStream(music_stream Stream);
SAV_API b32 PlaySoundChunk(sound_chunk Chunk);
SAV_API void FreeMusicStream(music_stream Stream);
SAV_API void FreeSoundChunk(sound_chunk Chunk);

SAV_API sav_shader BuildCustomShader(const char *VertPath, const char *FragPath);
SAV_API void DeleteShader(sav_shader *Shader);
SAV_API void BeginShaderMode(sav_shader Shader);
SAV_API void EndShaderMode();
SAV_API void SetUniformMat4(const char *UniformName, f32 *Value);
SAV_API void SetUniformVec4(const char *UniformName, f32 *Value);
SAV_API void SetUniformI(const char *UniformName, int Value);
SAV_API void BindTextureSlot(int Slot, sav_texture Texture);
SAV_API void UnbindTextureSlot(int Slot);
SAV_API void FlipTexCoords(vec2 *TexCoords);
SAV_API void NormalizeTexCoords(sav_texture Texture, vec2 *TexCoords);
SAV_API void GetTexCoordsForTex(sav_texture Texture, rect R, vec2 *TexCoords);
SAV_API void SavSwapBuffers();

SAV_API void ClearBackground(color Color);
SAV_API void BeginDraw();
SAV_API void EndDraw();
SAV_API void PrepareGpuData(u32 *VBO, u32 *VAO, u32 *EBO, int MaxVertexCount, int MaxIndexCount);
SAV_API void DrawVertices(vec3 *Positions, vec4 *TexCoords, vec4 *Colors, u32 *Indices, int VertexCount, int IndexCount);
SAV_API void DrawTexture(sav_texture Texture, rect Dest, rect Source, vec2 Origin, f32 Rotation, color Color);
SAV_API void DrawRect(rect Rect, color Color);

SAV_API void BeginCameraMode(camera_2d *Camera);
SAV_API void EndCameraMode();
SAV_API void SetProjectionMatrix(mat4 Projection);
SAV_API void SetModelViewMatrix(mat4 ModelView);
SAV_API void SetOrthographicProjectionMatrix(f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far);
SAV_API vec2 CameraWorldToScreen(camera_2d *Camera, vec2 World);
SAV_API vec2 CameraScreenToWorld(camera_2d *Camera, vec2 Screen);
SAV_API vec2 CameraScreenToWorldRel(camera_2d *Camera, vec2 Screen);
SAV_API vec2 ScreenToRectCoords(rect ScreenR, f32 ScaledW, f32 ScaledH, vec2 ScreenCoords);
SAV_API vec2 RectToScreenCoords(rect ScreenR, f32 ScaledW, f32 ScaledH, vec2 RectCoords);
SAV_API void CameraIncreaseLogZoom(camera_2d *Camera, f32 Delta);
SAV_API void CameraInitLogZoomSteps(camera_2d *Camera, f32 Min, f32 Max, int StepCount);
SAV_API void CameraIncreaseLogZoomSteps(camera_2d *Camera, int Steps);

SAV_API sav_image SavLoadImage(const char *Path);
SAV_API void SavFreeImage(sav_image *Image);
SAV_API void SavSaveImage(const char *Path, void *Data, int Width, int Height, b32 Flip, u32 RMask, u32 GiMask, u32 BMask, u32 AMask);
SAV_API sav_texture SavLoadTexture(const char *Path);
SAV_API sav_texture SavLoadTextureFromImage(sav_image Image);
SAV_API sav_texture SavLoadTextureFromData(void *ImageData, int Width, int Height);
SAV_API void SavSetTextureWrapMode(sav_texture Texture, tex_wrap_mode WrapMode);
SAV_API void SavSetTextureFilterMode(sav_texture Texture, tex_filter_mode FilterMode);
SAV_API sav_render_texture SavLoadRenderTexture(int Width, int Height, b32 FilterNearest);
SAV_API void SavDeleteRenderTexture(sav_render_texture *RenderTexture);
SAV_API void BeginTextureMode(sav_render_texture RenderTexture, rect RenderTextureScreenRect);
SAV_API void EndTextureMode();

SAV_API sav_font *SavLoadFont(memory_arena *Arena, const char *Path, u32 PointSize);
SAV_API void DrawString(const char *String, sav_font *Font, f32 PointSize, f32 X, f32 Y, f32 MaxWidth, color Color, b32 DrawBg, color BgColor, memory_arena *TransientArena);

SAV_API b32 GuiButtonRect(rect R);

SAV_API char *SavReadTextFile(const char *Path);
SAV_API void SavFreeString(char **Text);

SAV_API const char *TextFormat(const char *Format, ...);
SAV_API void TraceLog(const char *Format, ...);
SAV_API int GetRandomValue(int Min, int Max);
SAV_API f32 GetRandomFloat();

//
// NOTE: QOL inline overloads
//
inline void
DrawTexture(sav_texture Texture, rect Dest, color Color)
{
    DrawTexture(Texture, Dest, Rect(Texture.Width, Texture.Height), Vec2(), 0.0f, Color);
}

inline void
DrawTexture(sav_texture Texture, f32 X, f32 Y)
{
    DrawTexture(Texture,
                Rect(X, Y, (f32) Texture.Width, (f32) Texture.Height),
                Rect(Texture.Width, Texture.Height),
                Vec2(),
                0.0f,
                VA_WHITE);
}

inline void
DrawTexture(sav_texture Texture, f32 X, f32 Y, f32 Scale, color Color)
{
    DrawTexture(Texture,
                Rect(X, Y, Texture.Width * Scale, Texture.Height * Scale),
                Rect(Texture.Width, Texture.Height),
                Vec2(),
                0.0f,
                Color);
}

inline void
DrawTexture(sav_texture Texture, rect Dest, rect Source, color Color)
{
    DrawTexture(Texture, Dest, Source, Vec2(), 0.0f, Color);
}

inline rect
GetTextureRect(int X, int Y, sav_texture Texture)
{
    return Rect(X, Y, Texture.Width, Texture.Height);
}

#endif
