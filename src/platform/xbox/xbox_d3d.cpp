/*==========================================================================
  xbox_d3d.cpp - Direct3D 8 renderer for zelda3 Xbox port
==========================================================================*/
#include "xbox_platform.h"
#include "xbox_menu.h"
extern "C" {
#include "../../util.h"
}

LPDIRECT3D8             g_pD3D = NULL;
LPDIRECT3DDEVICE8       g_pDevice = NULL;
LPDIRECT3DTEXTURE8      g_pFrameTex = NULL;
LPDIRECT3DVERTEXBUFFER8 g_pQuadVB = NULL;

struct QuadVert { float x, y, z, rhw, u, v; };
#define FVF_QUAD (D3DFVF_XYZRHW | D3DFVF_TEX1)

#define TEX_W 512
#define TEX_H 512

static uint8_t s_framebuf[TEX_W * TEX_H * 4];
static int     s_frame_w = 256;
static int     s_frame_h = 224;
static int     s_disp_w = 640;
static int     s_disp_h = 480;
static int     s_dispmode = 0; /* 0=4:3  1=FILL  2=16:9 */

/*==========================================================================
  Morton swizzle
==========================================================================*/
static unsigned s_MortonInterleave(unsigned n)
{
    n = (n | (n << 8)) & 0x00FF00FF;
    n = (n | (n << 4)) & 0x0F0F0F0F;
    n = (n | (n << 2)) & 0x33333333;
    n = (n | (n << 1)) & 0x55555555;
    return n;
}

static void SwizzleLinearToXbox(const uint8_t* src, int src_pitch,
    uint8_t* dst, int w, int h)
{
    const uint32_t* s = (const uint32_t*)src;
    uint32_t* d = (uint32_t*)dst;
    int src_stride = src_pitch / 4;
    for (int y = 0; y < h; y++) {
        unsigned my = s_MortonInterleave((unsigned)y) << 1;
        for (int x = 0; x < w; x++) {
            unsigned mx = s_MortonInterleave((unsigned)x);
            d[mx | my] = s[y * src_stride + x];
        }
    }
}

/*==========================================================================
  Getters / setters
==========================================================================*/
extern "C" int  Xbox_D3D_GetDispW(void) { return s_disp_w; }
extern "C" int  Xbox_D3D_GetDispH(void) { return s_disp_h; }
extern "C" int  Xbox_D3D_GetDispMode(void) { return s_dispmode; }
extern "C" void Xbox_D3D_SetDispMode(int v)
{
    s_dispmode = v % 3;
    char b[64];
    _snprintf(b, sizeof(b), "zelda3: D3D DispMode=%d\n", s_dispmode);
    OutputDebugStringA(b);
}

/*==========================================================================
  Quad helper - respects display mode
==========================================================================*/
static void GetGameQuadVerts(QuadVert* verts, float u1, float v1)
{
    float x0, y0, x1, y1;
    float dw = (float)s_disp_w;
    float dh = (float)s_disp_h;

    if (s_dispmode == 1) {
        /* FILL - stretch to fill entire display */
        x0 = 0.0f; y0 = 0.0f; x1 = dw; y1 = dh;
    }
    else if (s_dispmode == 2) {
        /* 16:9 - PPU renders extra world, fill display */
        x0 = 0.0f; y0 = 0.0f; x1 = dw; y1 = dh;
    }
    else {
        /* 4:3 - correct SNES pixel aspect (8:7), bars on sides */
        float game_w = (float)s_frame_w * (8.0f / 7.0f);
        float game_h = (float)s_frame_h;
        float scale = dh / game_h;
        float disp_w = game_w * scale;
        x0 = (dw - disp_w) * 0.5f; x1 = x0 + disp_w;
        y0 = 0.0f; y1 = dh;
    }
    verts[0] = { x0, y0, 0.0f, 1.0f, 0.0f, 0.0f };
    verts[1] = { x1, y0, 0.0f, 1.0f, u1,   0.0f };
    verts[2] = { x0, y1, 0.0f, 1.0f, 0.0f, v1 };
    verts[3] = { x1, y1, 0.0f, 1.0f, u1,   v1 };
}

/*==========================================================================
  Init / Shutdown
==========================================================================*/
static HRESULT Xbox_D3D_CreateFrameTex(void)
{
    if (g_pFrameTex) { g_pFrameTex->Release(); g_pFrameTex = NULL; }
    return g_pDevice->CreateTexture(TEX_W, TEX_H, 1, 0,
        D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &g_pFrameTex);
}

HRESULT Xbox_D3D_Init(void)
{
    HRESULT hr;

    /* Detect best video mode */
    DWORD vidFlags = XGetVideoFlags();
    DWORD avPack = XGetAVPack();

    bool has720p = (avPack == XC_AV_PACK_HDTV) && (vidFlags & XC_VIDEO_FLAGS_HDTV_720p);
    bool has480p = (vidFlags & XC_VIDEO_FLAGS_HDTV_480p) != 0;
    bool isWide = (vidFlags & XC_VIDEO_FLAGS_WIDESCREEN) != 0;

    DWORD presentFlags = 0;

    if (has720p) {
        s_disp_w = 1280; s_disp_h = 720;
        presentFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
        OutputDebugStringA("zelda3: Video mode: 720p\n");
    }
    else if (has480p) {
        s_disp_w = 640; s_disp_h = 480;
        presentFlags = D3DPRESENTFLAG_PROGRESSIVE | (isWide ? D3DPRESENTFLAG_WIDESCREEN : 0);
        OutputDebugStringA("zelda3: Video mode: 480p\n");
    }
    else {
        s_disp_w = 640; s_disp_h = 480;
        presentFlags = D3DPRESENTFLAG_INTERLACED | (isWide ? D3DPRESENTFLAG_WIDESCREEN : 0);
        OutputDebugStringA("zelda3: Video mode: 480i\n");
    }

    g_pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_pD3D) return E_FAIL;

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = s_disp_w;
    pp.BackBufferHeight = s_disp_h;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = FALSE;
    pp.EnableAutoDepthStencil = FALSE;
    pp.FullScreen_RefreshRateInHz = 60;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    pp.Flags = presentFlags;

    hr = g_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_pDevice);
    if (FAILED(hr)) { g_pD3D->Release(); g_pD3D = NULL; return hr; }

    g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    g_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    g_pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
    g_pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    g_pDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    g_pDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    g_pDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

    hr = Xbox_D3D_CreateFrameTex();
    if (FAILED(hr)) {
        if (g_pDevice) { g_pDevice->Release(); g_pDevice = NULL; }
        if (g_pD3D) { g_pD3D->Release();    g_pD3D = NULL; }
        return hr;
    }

    OutputDebugStringA("zelda3: D3D8 init OK\n");
    return S_OK;
}
extern "C" void Xbox_D3D_Shutdown(void)
{
    if (g_pQuadVB) { g_pQuadVB->Release();   g_pQuadVB = NULL; }
    if (g_pFrameTex) { g_pFrameTex->Release(); g_pFrameTex = NULL; }
    if (g_pDevice) { g_pDevice->Release();   g_pDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release();      g_pD3D = NULL; }
}

/* RendererFuncs table stubs - Initialize/Destroy not used on Xbox
   (D3D init/shutdown done directly) but required by struct layout. */
static bool Xbox_D3D_Initialize(struct SDL_Window* window) { (void)window; return true; }
static void Xbox_D3D_Destroy(void) {}

/*==========================================================================
  BeginDraw / EndDraw
==========================================================================*/
void Xbox_D3D_BeginDraw(int width, int height, uint8_t** pixels, int* pitch)
{
    s_frame_w = width < TEX_W ? width : TEX_W;
    s_frame_h = height < TEX_H ? height : TEX_H;
    *pixels = s_framebuf;
    *pitch = TEX_W * 4;
}

void Xbox_D3D_EndDraw(void)
{
    if (!g_pDevice || !g_pFrameTex) return;

    D3DLOCKED_RECT lr;
    if (FAILED(g_pFrameTex->LockRect(0, &lr, NULL, 0))) return;

    SwizzleLinearToXbox(s_framebuf, TEX_W * 4,
        (uint8_t*)lr.pBits, s_frame_w, s_frame_h);
    g_pFrameTex->UnlockRect(0);

    float u1 = (float)s_frame_w / (float)TEX_W;
    float v1 = (float)s_frame_h / (float)TEX_H;
    QuadVert verts[4];
    GetGameQuadVerts(verts, u1, v1);

    g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (SUCCEEDED(g_pDevice->BeginScene())) {
        g_pDevice->SetTexture(0, g_pFrameTex);
        g_pDevice->SetVertexShader(FVF_QUAD);
        g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(QuadVert));
        /* Draw toast/menu overlay - Xbox_Menu_Draw handles both */
        Xbox_Menu_Draw();
        g_pDevice->EndScene();
    }
    g_pDevice->Present(NULL, NULL, NULL, NULL);
}

/*==========================================================================
  Overlays
==========================================================================*/
void Xbox_D3D_DrawDimOverlay(void)
{
    if (!g_pDevice) return;

    float u1 = (float)s_frame_w / (float)TEX_W;
    float v1 = (float)s_frame_h / (float)TEX_H;
    QuadVert verts[4];
    GetGameQuadVerts(verts, u1, v1);

    g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    if (SUCCEEDED(g_pDevice->BeginScene())) {
        g_pDevice->SetTexture(0, g_pFrameTex);
        g_pDevice->SetVertexShader(FVF_QUAD);
        g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(QuadVert));

        struct DimVert { float x, y, z, rhw; DWORD c; };
        float dw = (float)s_disp_w, dh = (float)s_disp_h;
        DimVert dim[4] = {
            {0, 0,  0,1,0xA0000000},
            {dw,0,  0,1,0xA0000000},
            {0, dh, 0,1,0xA0000000},
            {dw,dh, 0,1,0xA0000000},
        };
        g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        g_pDevice->SetTexture(0, NULL);
        g_pDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dim, sizeof(DimVert));
        g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pDevice->EndScene();
    }
    g_pDevice->Present(NULL, NULL, NULL, NULL);
}

void Xbox_D3D_DrawMenuOverlay(void)
{
    if (!g_pDevice) return;

    float u1 = (float)s_frame_w / (float)TEX_W;
    float v1 = (float)s_frame_h / (float)TEX_H;
    QuadVert verts[4];
    GetGameQuadVerts(verts, u1, v1);

    g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    if (SUCCEEDED(g_pDevice->BeginScene())) {
        g_pDevice->SetTexture(0, g_pFrameTex);
        g_pDevice->SetVertexShader(FVF_QUAD);
        g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(QuadVert));
        Xbox_Menu_Draw();
        g_pDevice->EndScene();
    }
    g_pDevice->Present(NULL, NULL, NULL, NULL);
}

extern "C" const struct RendererFuncs kXboxRendererFuncs = {
    Xbox_D3D_Initialize,
    Xbox_D3D_Destroy,
    Xbox_D3D_BeginDraw,
    Xbox_D3D_EndDraw,
};