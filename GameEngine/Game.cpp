//
// Game.cpp
//
#include "pch.h"
#include "Game.h"
#include "main.h"
#include "render.h"
#include "postprocess.h"

#include "hkOnDevice.h"
#include "InputManager.h"
#include "GameUpdate.h"
#include "Viewmodel.h"
#include "PlayerController.h"
#include "SoundManager.h"
//#include "SkyDome.h"


using namespace DirectX;
using Microsoft::WRL::ComPtr;

bool  g_CrtShutdown = false;
float g_CrtTime = 0.0f;
SoundManager* g_sound = nullptr;//global sound manager pointer

Game::Game() noexcept(false)
{
    m_pitch = 0;
    m_yaw = 0;

    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 120); //120 FPS MAX

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_sound = std::make_unique<SoundManager>();
    m_sound->Initialize();
	g_sound = m_sound.get();//get global pointer
   //if (!g_skyDome)
   //{
   //    auto device = m_deviceResources->GetD3DDevice();
   //    auto context = m_deviceResources->GetD3DDeviceContext();
   //    g_skyDome = std::make_unique<SkyDome>(device, context);
   //}
    // ===== Title ?景提前加? =====
    LoadLevel(
        m_deviceResources->GetD3DDevice(),
        m_deviceResources->GetD3DDeviceContext(),
        "e1m1");
    GameState = GAME_TITLE;
    ShowMenu = true;
    LockMouse = false;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });
    Render();
}

void Game::Update(DX::StepTimer const& timer)
{   
       
        // ?入 TITLE
        if (GameState == GAME_TITLE)
        {
            m_sound->PlayTitleBGM();  // ? Title BGM
        }

    
    //GameUpdate(timer);
     switch (GameState)
    {
     case GAME_TITLE: {
         
        
        
         // 只允??入 & UI
         // ① 世界照常更新（怪物 / boss / 粒子）
         // ★ 世界照常更新（怪、Boss、粒子都会?）
         player::PlayerHealth = 9999;

         GameUpdate(timer);

         // 禁止玩家控制
         //player::InputEnabled = false;

         // UI ?入
         input::MouseProcess();
         input::KeyboardProcess();


         break;
     }
    case GAME_PLAY:
        GameUpdate(timer);   // ★ 原??完整保留
        break;

    case GAME_CLEAR:
        GameUpdate(timer);   // ★ ??更新世界
        input::MouseProcess();
        input::KeyboardProcess();
        break;
    case GAME_FAIL:
        // 不更新世界，只?理 UI / 重?
        if (g_CrtShutdown)
            g_CrtTime += DeltaTime;

        input::MouseProcess();
        input::KeyboardProcess();
        break;
   
    }
     m_sound->Update();
    // TODO: Add game logic here.
}

void StartNewGame(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // 1. ?制退回 TITLE
    //GameState = GAME_TITLE;

    // 2. ?底卸?世界
    UnloadLevel();
    //EntityList.clear();

    // 3. 明?加???（会?置出生点）
    LoadLevel(device, context, "e1m1");
    player::Reset(g_PlayerSpawnPos);
    // 4. 切?到 PLAY（此? Player 已在出生点）
    //GameState = GAME_PLAY;

    ShowMenu = false;
    LockMouse = true;
}
#pragma endregion


#pragma region Frame Render

#include "gui.h"
// Draws the scene.
void Game::Render()
{
    if (m_timer.GetFrameCount() == 0) return;     // Don't try to render anything before the first Update.
    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    Renderer(m_deviceResources->GetD3DDevice(), context, m_deviceResources->GetSwapChain());
    //bloom blur
    if(ShowMenu) postprocess::Run(m_deviceResources->GetD3DDevice(), context, m_deviceResources->GetSwapChain()); //blur
    gui::RenderGUI(m_deviceResources->GetD3DDevice(), context, m_sound.get()); //imgui


    m_deviceResources->PIXEndEvent();
    m_deviceResources->Present(); // Show the new frame.
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::DarkSlateGray);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
    m_deviceResources->PIXEndEvent();
    //g_skyDome.reset();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
    WindowWidth = r.right;
    WindowHeight = r.bottom;
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height)) return;
    CreateWindowSizeDependentResources();
    WindowWidth = width;
    WindowHeight = height;
    // TODO: Game window is being resized.
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = WindowWidth;
    height = WindowHeight;
}
#pragma endregion

#pragma region Direct3D Resources

#include "dx_tools.h"
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_world = Matrix::Identity;
    m_spriteBatch = std::make_unique<SpriteBatch>(context);
    GenerateTexture(device, m_texture.ReleaseAndGetAddressOf()); //Temp texture for testing

    //Main effect for drawing level
    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetWorld(m_world);
    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
    m_effect->SetTexture(m_texture.Get());
    m_effect->SetTextureEnabled(true);

    m_effect->EnableDefaultLighting();          //ugly specular glowing
    m_effect->SetSpecularColor(Colors::Red);  //disable specular glow
    m_effect->SetAmbientLightColor(Colors::AliceBlue);
    m_effect->SetDiffuseColor(Colors::Lavender);

    m_effect->SetFogEnabled(true);
    m_effect->SetFogColor(Colors::Orange);
    m_effect->SetFogStart(25.f);
    m_effect->SetFogEnd(40.f);
    DX::ThrowIfFailed(CreateInputLayoutFromEffect<VertexType>(device, m_effect.get(), &m_inputLayout));

    //Basic drawing fore debug and shit
    primitiveBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);
    basicEffect = std::make_unique<BasicEffect>(device);
    basicEffect->SetVertexColorEnabled(true);
    void const* shaderByteCode;
    size_t byteCodeLength;
    basicEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    HRESULT hr = device->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount, shaderByteCode, byteCodeLength, &inputLayout);
    if (FAILED(hr)) { printf("Unable to create device input layout; result = %d\n", hr); }

    postprocess::Init(device, context, m_deviceResources->GetSwapChain());

    // TODO: Initialize device dependent objects here (independent of window size).
   
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(FOV), float(size.right) / float(size.bottom), 0.01f, RenderDistance);
    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    hkOnDeviceLost();
    // TODO: Add Direct3D resource cleanup here.
}

void Game::OnDeviceRestored()
{
    hkOnDeviceRestored(this);
}

Game::~Game() = default;

#pragma endregion
