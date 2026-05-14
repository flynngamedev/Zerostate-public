// ZeroState Ball Sim Visualiser -- DirectX 11
//
// Colours:
//   Box    -- gray wireframe
//   Spikes -- gray spheres
//   Ball   -- white-grey sphere
//
// Controls:
//   Left mouse drag -- rotate camera
//   Scroll          -- zoom
//   R               -- reseed (fresh RDSEED entropy)
//   ESC             -- quit
//
// Compile (Developer Command Prompt):
//   cl /O2 /std:c++17 /EHsc visualiser.cpp physics.cpp entropy.cpp quantum.cpp
//      /Fe:visualiser.exe /I. d3d11.lib d3dcompiler.lib dxgi.lib user32.lib
//      /link /SUBSYSTEM:WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _USE_MATH_DEFINES
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include "physics.h"
#include "entropy.h"
#include "quantum.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace DirectX;
using std::min;
using std::max;

// ── Window ────────────────────────────────────────────────────────────────

static const int W = 1280;
static const int H = 720;
static const wchar_t* TITLE = L"ZeroState Visualiser";

// ── DX11 globals ──────────────────────────────────────────────────────────

static ID3D11Device*           g_dev       = nullptr;
static ID3D11DeviceContext*    g_ctx       = nullptr;
static IDXGISwapChain*         g_swap      = nullptr;
static ID3D11RenderTargetView* g_rtv       = nullptr;
static ID3D11DepthStencilView* g_dsv       = nullptr;
static ID3D11VertexShader*     g_vs        = nullptr;
static ID3D11PixelShader*      g_ps        = nullptr;
static ID3D11InputLayout*      g_layout    = nullptr;
static ID3D11Buffer*           g_cbuf      = nullptr;
static ID3D11RasterizerState*  g_rsWire    = nullptr;
static ID3D11RasterizerState*  g_rsSolid   = nullptr;
static ID3D11DepthStencilState*g_dss       = nullptr;

// ── Camera ────────────────────────────────────────────────────────────────

static float g_yaw   = 0.4f;
static float g_pitch = 0.3f;
static float g_zoom  = 220.0f;
static bool  g_drag  = false;
static float g_lastX = 0, g_lastY = 0;

// ── Sim state ─────────────────────────────────────────────────────────────

static physics::SimState g_sim;
static quantum::Pool     g_pool;

// ── Vertex / constant buffer ──────────────────────────────────────────────

struct Vertex {
    float x, y, z;
    float r, g, b;
};

struct CBuf {
    XMMATRIX mvp;
};

// ── Shaders ───────────────────────────────────────────────────────────────

static const char* SHADER_SRC = R"(
cbuffer CB : register(b0) {
    float4x4 mvp;
};

struct VSIn  { float3 pos : POSITION; float3 col : COLOR; };
struct VSOut { float4 pos : SV_POSITION; float3 col : COLOR; };

VSOut VS(VSIn v) {
    VSOut o;
    o.pos = mul(float4(v.pos, 1.0f), mvp);
    o.col = v.col;
    return o;
}

float4 PS(VSOut v) : SV_TARGET {
    return float4(v.col, 1.0f);
}
)";

// ── Geometry helpers ──────────────────────────────────────────────────────

static void push_line(std::vector<Vertex>& verts,
    float x0,float y0,float z0,
    float x1,float y1,float z1,
    float r,float g,float b)
{
    verts.push_back({x0,y0,z0,r,g,b});
    verts.push_back({x1,y1,z1,r,g,b});
}

// Box wireframe -- 12 edges
static void build_box(std::vector<Vertex>& v) {
    float s = (float)BOX_SIZE;
    float r=0.45f,g=0.45f,b=0.45f;
    // bottom
    push_line(v, 0,0,0, s,0,0, r,g,b);
    push_line(v, s,0,0, s,0,s, r,g,b);
    push_line(v, s,0,s, 0,0,s, r,g,b);
    push_line(v, 0,0,s, 0,0,0, r,g,b);
    // top
    push_line(v, 0,s,0, s,s,0, r,g,b);
    push_line(v, s,s,0, s,s,s, r,g,b);
    push_line(v, s,s,s, 0,s,s, r,g,b);
    push_line(v, 0,s,s, 0,s,0, r,g,b);
    // pillars
    push_line(v, 0,0,0, 0,s,0, r,g,b);
    push_line(v, s,0,0, s,s,0, r,g,b);
    push_line(v, s,0,s, s,s,s, r,g,b);
    push_line(v, 0,0,s, 0,s,s, r,g,b);
}

// Sphere as lat/lon lines
static void build_sphere(std::vector<Vertex>& v,
    float cx, float cy, float cz, float rad,
    float r, float g, float b,
    int stacks=10, int slices=16)
{
    for (int i = 0; i < stacks; i++) {
        float a0 = (float)i     / stacks * (float)M_PI - (float)M_PI/2;
        float a1 = (float)(i+1) / stacks * (float)M_PI - (float)M_PI/2;
        float y0 = sinf(a0)*rad, y1 = sinf(a1)*rad;
        float r0 = cosf(a0)*rad, r1 = cosf(a1)*rad;
        for (int j = 0; j < slices; j++) {
            float t0 = (float)j     / slices * 2*(float)M_PI;
            float t1 = (float)(j+1) / slices * 2*(float)M_PI;
            // lat ring at a0
            push_line(v,
                cx+r0*cosf(t0), cy+y0, cz+r0*sinf(t0),
                cx+r0*cosf(t1), cy+y0, cz+r0*sinf(t1),
                r,g,b);
            // lon line
            push_line(v,
                cx+r0*cosf(t0), cy+y0, cz+r0*sinf(t0),
                cx+r1*cosf(t0), cy+y1, cz+r1*sinf(t0),
                r,g,b);
        }
    }
}

// ── DX11 init ─────────────────────────────────────────────────────────────

static ID3D11Buffer* make_vbuf(const std::vector<Vertex>& verts) {
    if (verts.empty()) return nullptr;
    D3D11_BUFFER_DESC bd{};
    bd.Usage     = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = (UINT)(verts.size() * sizeof(Vertex));
    D3D11_SUBRESOURCE_DATA sd{ verts.data() };
    ID3D11Buffer* buf = nullptr;
    g_dev->CreateBuffer(&bd, &sd, &buf);
    return buf;
}

static bool dx_init(HWND hwnd) {
    // Swap chain
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount       = 1;
    sd.BufferDesc.Width  = W;
    sd.BufferDesc.Height = H;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_swap, &g_dev, &fl, &g_ctx);
    if (FAILED(hr)) return false;

    // RTV
    ID3D11Texture2D* bb = nullptr;
    g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
    bb->Release();

    // Depth buffer
    D3D11_TEXTURE2D_DESC dd{};
    dd.Width  = W; dd.Height = H; dd.MipLevels = 1; dd.ArraySize = 1;
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.SampleDesc.Count = 1;
    dd.Usage    = D3D11_USAGE_DEFAULT;
    dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* dt = nullptr;
    g_dev->CreateTexture2D(&dd, nullptr, &dt);
    g_dev->CreateDepthStencilView(dt, nullptr, &g_dsv);
    dt->Release();

    // Compile shaders
    ID3DBlob *vsBlob=nullptr, *psBlob=nullptr, *err=nullptr;
    D3DCompile(SHADER_SRC, strlen(SHADER_SRC), nullptr, nullptr, nullptr,
        "VS","vs_4_0",0,0,&vsBlob,&err);
    if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }
    D3DCompile(SHADER_SRC, strlen(SHADER_SRC), nullptr, nullptr, nullptr,
        "PS","ps_4_0",0,0,&psBlob,&err);
    if (err) { OutputDebugStringA((char*)err->GetBufferPointer()); err->Release(); }

    g_dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vs);
    g_dev->CreatePixelShader (psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_ps);

    // Input layout
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    g_dev->CreateInputLayout(ied, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_layout);
    vsBlob->Release(); psBlob->Release();

    // Constant buffer
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage     = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbd.ByteWidth = sizeof(CBuf);
    g_dev->CreateBuffer(&cbd, nullptr, &g_cbuf);

    // Rasteriser -- wireframe
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_WIREFRAME;
    rd.CullMode = D3D11_CULL_NONE;
    g_dev->CreateRasterizerState(&rd, &g_rsWire);
    rd.FillMode = D3D11_FILL_SOLID;
    g_dev->CreateRasterizerState(&rd, &g_rsSolid);

    // Depth stencil
    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable    = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc      = D3D11_COMPARISON_LESS;
    g_dev->CreateDepthStencilState(&dsd, &g_dss);

    // Viewport
    D3D11_VIEWPORT vp{ 0,0,(float)W,(float)H,0,1 };
    g_ctx->RSSetViewports(1, &vp);

    return true;
}

// ── Draw call ─────────────────────────────────────────────────────────────

static void draw_verts(const std::vector<Vertex>& verts,
    D3D11_PRIMITIVE_TOPOLOGY topo, XMMATRIX mvp)
{
    if (verts.empty()) return;

    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    g_ctx->Map(g_cbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    ((CBuf*)ms.pData)->mvp = XMMatrixTranspose(mvp);
    g_ctx->Unmap(g_cbuf, 0);
    g_ctx->VSSetConstantBuffers(0, 1, &g_cbuf);

    // Upload verts
    ID3D11Buffer* vb = make_vbuf(verts);
    if (!vb) return;
    UINT stride = sizeof(Vertex), offset = 0;
    g_ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    g_ctx->IASetPrimitiveTopology(topo);
    g_ctx->Draw((UINT)verts.size(), 0);
    vb->Release();
}

// ── Frame ─────────────────────────────────────────────────────────────────

static void render_frame() {
    // Advance sim
    physics::tick(g_sim);

    // Clear
    float bg[4] = { 0.06f,0.06f,0.06f,1.0f };
    g_ctx->ClearRenderTargetView(g_rtv, bg);
    g_ctx->ClearDepthStencilView(g_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    g_ctx->OMSetRenderTargets(1, &g_rtv, g_dsv);
    g_ctx->OMSetDepthStencilState(g_dss, 0);
    g_ctx->IASetInputLayout(g_layout);
    g_ctx->VSSetShader(g_vs, nullptr, 0);
    g_ctx->PSSetShader(g_ps, nullptr, 0);
    g_ctx->RSSetState(g_rsWire);

    // Camera
    float camX = g_zoom * cosf(g_pitch) * sinf(g_yaw);
    float camY = g_zoom * sinf(g_pitch);
    float camZ = g_zoom * cosf(g_pitch) * cosf(g_yaw);
    float cx   = (float)BOX_SIZE * 0.5f;

    XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(camX+cx, camY+cx, camZ+cx, 0),
        XMVectorSet(cx, cx, cx, 0),
        XMVectorSet(0,1,0,0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(60.0f), (float)W/H, 0.1f, 2000.0f);
    XMMATRIX vp   = view * proj;

    // Box
    {
        std::vector<Vertex> box;
        build_box(box);
        draw_verts(box, D3D11_PRIMITIVE_TOPOLOGY_LINELIST, vp);
    }

    // Spikes
    {
        std::vector<Vertex> spikes;
        for (int i = 0; i < NUM_SPIKES; i++) {
            build_sphere(spikes,
                (float)g_sim.spikes[i].pos.x,
                (float)g_sim.spikes[i].pos.y,
                (float)g_sim.spikes[i].pos.z,
                (float)g_sim.spikes[i].radius,
                0.38f, 0.38f, 0.38f, 6, 10);
        }
        draw_verts(spikes, D3D11_PRIMITIVE_TOPOLOGY_LINELIST, vp);
    }

    // Ball
    {
        std::vector<Vertex> ball;
        build_sphere(ball,
            (float)g_sim.pos.x,
            (float)g_sim.pos.y,
            (float)g_sim.pos.z,
            3.0f,
            0.88f, 0.88f, 0.88f, 12, 18);
        draw_verts(ball, D3D11_PRIMITIVE_TOPOLOGY_LINELIST, vp);
    }

    // Title bar with live stats
    wchar_t title[256];
    swprintf(title, 256,
        L"ZeroState Visualiser  |  vel: %.2f mph  |  bounce: %d  |  pos: (%.1f, %.1f, %.1f)  |  [R] reseed  [ESC] quit",
        g_sim.speed, g_sim.bounce,
        g_sim.pos.x, g_sim.pos.y, g_sim.pos.z);
    SetWindowTextW(GetForegroundWindow(), title);

    g_swap->Present(1, 0);
}

// ── Window proc ───────────────────────────────────────────────────────────

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) PostQuitMessage(0);
        if (wp == 'R') {
            g_sim = physics::init_state();  // reseed
        }
        return 0;

    case WM_LBUTTONDOWN:
        g_drag  = true;
        g_lastX = (float)LOWORD(lp);
        g_lastY = (float)HIWORD(lp);
        SetCapture(hwnd);
        return 0;

    case WM_LBUTTONUP:
        g_drag = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (g_drag) {
            float x = (float)LOWORD(lp);
            float y = (float)HIWORD(lp);
            g_yaw   += (x - g_lastX) * 0.005f;
            g_pitch += (y - g_lastY) * 0.005f;
            g_pitch  = max(-1.4f, min(1.4f, g_pitch));            g_lastX  = x;
            g_lastY  = y;
        }
        return 0;

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        g_zoom -= delta * 0.1f;
        g_zoom  = max(50.0f, min(500.0f, g_zoom));        return 0;
    }

    case WM_SIZE:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── Entry point ───────────────────────────────────────────────────────────

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    // Register window class
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ZSVis";
    RegisterClassExW(&wc);

    // Create window
    RECT r = { 0,0,W,H };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowExW(0, L"ZSVis", TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right-r.left, r.bottom-r.top,
        nullptr, nullptr, hInst, nullptr);

    if (!dx_init(hwnd)) {
        MessageBoxW(hwnd, L"DirectX 11 init failed.", L"Error", MB_OK);
        return 1;
    }

    // Init pool + sim
    quantum::pool_init(g_pool);
    g_sim = physics::init_state();

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg{};
    while (true) {
        while (PeekMessage(&msg, nullptr, 0,0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        render_frame();
    }
done:
    return 0;
}
