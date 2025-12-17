#include "pch.h"
#include "gui.h"
#include "main.h"
#include "Game.h"


#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include "PlayerController.h"

#include "gui_style.h"
#include "gui_hud.h"
#include "LevelManager.h"
#include <algorithm>

namespace gui
{

    enum page {
        page_main,
        page_singleplayer,
        //page_multiplayer,

    };
    int CurrentPage;
    bool ShowSettings = false;
    bool ShowLoadMenu = false;
    bool ShowSaveMenu = false;
    bool ShowServerBrowser = false;
    bool ShowHostMenu = false;
    bool ShowCredits = false;
    bool ShowFirstTimeMenu = true;



    void DrawMenu(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        float centerX = (ImGui::GetWindowSize().x - ButtonWidth) * 0.5f;
        ImGui::SetCursorPosX(centerX);
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });
        ImGui::PushStyleColor(ImGuiCol_Border, { 0.f, 0.f, 0.f, 0.f });
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, { 0.f, 0.f, 0.f, 0.f });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
        ImGui::PushFont(font_menu);

        ImGui::SetCursorPosY(ImGui::GetIO().DisplaySize.y - ButtonHeight*2);
        if (CurrentPage == page_main)
        {
            
            if (ImGui::Button("Quit", ImVec2(ButtonWidth, ButtonHeight))) ExitProcess(EXIT_SUCCESS);
            float posY = ImGui::GetCursorPosY();
            ImGui::SetCursorPosY(posY - ButtonHeight * 2 - ButtonHeight * 0.25);
            ImGui::SetCursorPosX(centerX);
            if (ImGui::Button("New Game", ImVec2(ButtonWidth, ButtonHeight))) CurrentPage = page_singleplayer;
            //if (ImGui::Button("MULTIPLAYER", ImVec2(ButtonWidth, ButtonHeight))) CurrentPage = page_multiplayer;
            //if (ImGui::Button("SETTINGS", ImVec2(ButtonWidth, ButtonHeight))) { ShowSettings = !ShowSettings; }
            //if (ImGui::Button("CREDITS", ImVec2(ButtonWidth, ButtonHeight))) ShowCredits = !ShowCredits;
        }
        else if (CurrentPage == page_singleplayer)
        {
            if (ImGui::Button("Back", ImVec2(ButtonWidth, ButtonHeight))) CurrentPage = page_main;
            float posY = ImGui::GetCursorPosY();
            ImGui::SetCursorPosX(centerX);
            ImGui::SetCursorPosY(posY - ButtonHeight * 2 - ButtonHeight * 0.25);
            if (ImGui::Button("Start", ImVec2(ButtonWidth, ButtonHeight))) 
            { 
                UnloadLevel();
                CurrentPage = page_main; 
                LoadLevel(device, context, "e1m1");
                StartNewGame(device, context);
                GameState = GAME_PLAY;
                ShowMenu = false; 
                LockMouse = true; 
                m_mouse->SetMode(Mouse::MODE_RELATIVE);
                
            }
            //if (ImGui::Button("LOAD", ImVec2(ButtonWidth, ButtonHeight))) ShowLoadMenu = !ShowLoadMenu;
            //if (ImGui::Button("SAVE", ImVec2(ButtonWidth, ButtonHeight))) ShowSaveMenu = !ShowSaveMenu;
        }
        //ImGui::Text("lefttop1");
       
        const char* title = "C o R E";

    
        ImGui::PushFont(font_menu);

      
        ImVec2 textSize = ImGui::CalcTextSize(title);
        ImVec2 windowSize = ImGui::GetWindowSize();

       
        float cursorX = (windowSize.x - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(cursorX);
        float cursorY = (windowSize.y * 0.33f);
        ImGui::SetCursorPosY(cursorY);

        // float cursorY = (windowSize.y - textSize.y) * 0.5f;
        // ImGui::SetCursorPosY(cursorY);

     
        ImGui::Text("%s", title);

        ImGui::PopFont();
        // ==== ???示?束 ====
        //else if (CurrentPage == page_multiplayer)
        //{
        //    if (ImGui::Button("BACK", ImVec2(ButtonWidth, ButtonHeight))) CurrentPage = page_main;
        //    float posY = ImGui::GetCursorPosY();
        //    ImGui::SetCursorPosY(posY - ButtonHeight * 5 - ButtonHeight * 0.25);
        //    if (ImGui::Button("BROWSE GAMES", ImVec2(ButtonWidth, ButtonHeight))) { ShowServerBrowser = !ShowServerBrowser; }
        //    if (ImGui::Button("HOST", ImVec2(ButtonWidth, ButtonHeight))) ShowHostMenu = !ShowHostMenu;
        //}

        ImGui::PopFont();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    //fps
    float pollingTime = 0.5f;
    float m_AccumulatedDeltaTime = 0.0f;
    int m_AccumulatedFrameCount = 0;
    int framerate;


    bool init = false;
    bool open = true;
	void RenderGUI(ID3D11Device* device, ID3D11DeviceContext* context)
	{
        if (!init)
        {
            init = true;

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.DisplaySize.x = 100;
            io.DisplaySize.y = 100;
            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            ImGui_ImplWin32_Init(hwnd);
            ImGui_ImplDX11_Init(device, context);
            //StyleValve();
			StyleDeepDark();
            SetupFonts();
        }
        else
        {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            ImGuiIO& io = ImGui::GetIO();

            //if(ShowMenu) ImGui::ShowDemoWindow(&ShowMenu);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 10.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.0f, 8.0f });
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
            if (ImGui::Begin("##gui", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
                ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
                Rescale();
                //FPS
                if (m_AccumulatedDeltaTime >= pollingTime)
                {
                    framerate = (int)((float)m_AccumulatedFrameCount / m_AccumulatedDeltaTime);
                    m_AccumulatedDeltaTime = 0.f;
                    m_AccumulatedFrameCount = 0;
                }
                m_AccumulatedDeltaTime += DeltaTime;
                m_AccumulatedFrameCount++;
                ImGui::Text((std::to_string(framerate) + " FPS").c_str());
                
               
                
                if (GameState == GAME_TITLE)
                {
                    DrawMenu(device, context);
                }
                else if (GameState == GAME_PLAY)
                {
                    DrawHud();
                    //ShowMenu = false;
                }
                else if (GameState == GAME_CLEAR)
                {
                    ImGui::Text("YOU WIN");
                    if (ImGui::Button("RESTART"))
                    {
                        UnloadLevel();
                     
                        LoadLevel(device, context, "e1m1");
                        
                        //EntityList.clear();
                        GameState = GAME_TITLE;
                        ShowMenu = true;
                        LockMouse = false;
                        m_mouse->SetMode(Mouse::MODE_ABSOLUTE);
                        
                    }
                }
                else if (GameState == GAME_FAIL)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    auto* drawList = ImGui::GetBackgroundDrawList();

                    static float blackoutT = 0.0f;
                    blackoutT += DeltaTime;

                    // 0 → 1
                    float t = min(blackoutT / 1.0f, 1.0f);

                    // 中央亮点半径（CRT 断?感）
                    float radius = (1.0f - t) * std:: max(0.05f, io.DisplaySize.y*0.01f);

                    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

                    // 全黑
                    drawList->AddRectFilled(
                        ImVec2(0, 0),
                        io.DisplaySize,
                        IM_COL32(0, 0, 0, 255)
                    );

                    // 中央亮圈（最后一点光）
                    drawList->AddCircleFilled(
                        center,
                        radius,
                        IM_COL32(255, 255, 255, 255),
                        64
                    );

                    ImGui::SetCursorPosY(io.DisplaySize.y * 0.6f);
                    ImGui::SetCursorPosX((io.DisplaySize.x - ButtonWidth) * 0.5f);

                    if (t >= 1.0f)
                    {
                        ImGui::Text("GAME OVER");
                        if (ImGui::Button("RESTART", ImVec2(ButtonWidth, ButtonHeight)))
                        {
                            
                            UnloadLevel(); 
                           
                            // ② 重置 CRT 断?状?（★??）
                            g_CrtShutdown = false;
                            g_CrtTime = 0.0f;
                            // ③ 重置玩家状?（★??）
                            //player::PlayerHealth = 5;        // 或?的最大 HP
                            //player::PlayerVelocity = Vector3::Zero;
                            //blackoutT = 0.0f;   // ★ 忘了?行会出 bug
                            GameState = GAME_TITLE;
                            LoadLevel(device, context, "e1m1");
                            ShowMenu = true;
                            LockMouse = false;
                            m_mouse->SetMode(Mouse::MODE_ABSOLUTE);
                        }
                    }
                }


                ImGui::GetWindowDrawList()->PushClipRectFullScreen();
                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);
            }
            

            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
	}

}