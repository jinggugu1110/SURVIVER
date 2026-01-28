#include "pch.h"
#include "gui_hud.h"
#include "gui_style.h"
#include "imgui/imgui.h"
#include "PlayerController.h"
#include "LevelManager.h" 
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <algorithm>

using namespace DirectX;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;

//--------------------------------------------------
// For Boss marker UI
// ボスマーカー用
//--------------------------------------------------
extern int WindowWidth;
extern int WindowHeight;
extern DirectX::SimpleMath::Matrix m_view;
extern DirectX::SimpleMath::Matrix m_proj;

//--------------------------------------------------
// For "Mission Start" UI
// ミッション開始表示用
//--------------------------------------------------
static float s_missionTextTimer = -1.0f;      // < 0 : not visible / 非表示
static constexpr float kMissionTextTime = 2.0f; // total display time (sec)
void TriggerMissionStartUI()
{
    s_missionTextTimer = 0.0f;
}
// 表示時間（秒）
// 
// damage flash (HP decrease)
static int   s_prevHP = -1;
static float s_damageFlashT = -1.0f;   // <0 = inactive
static constexpr float kDamageFlashTime = 0.5f; // 0.15~0.25 

//--------------------------------------------------
// World position -> Screen position
// ワールド座標をスクリーン座標へ変換
//--------------------------------------------------
static bool WorldToScreen(
    const DirectX::SimpleMath::Vector3& world,
    DirectX::SimpleMath::Vector2& outScreen
)
{
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    // World -> Clip space
    // ワールド座標 → クリップ空間
    XMVECTOR p = XMVectorSet(world.x, world.y, world.z, 1.0f);
    Matrix viewProj = m_view * m_proj;
    XMVECTOR clip = XMVector4Transform(p, viewProj);

    // Use absolute W to keep direction stable even when behind camera
    // 背面でも方向を安定させるため |w| を使用
    float w = XMVectorGetW(clip);
    float wAbs = fabsf(w);
    if (wAbs <= 0.0001f) return false;

    // Normalized Device Coordinates
    // 正規化デバイス座標（NDC）
    float ndcX = XMVectorGetX(clip) / wAbs;
    float ndcY = XMVectorGetY(clip) / wAbs;

    // NDC -> Screen space
    // NDC → スクリーン座標
    outScreen.x = (ndcX * 0.5f + 0.5f) * (float)WindowWidth;
    outScreen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * (float)WindowHeight; // Y-axis inverted
    // Y軸反転

    return true;
}

void DrawHud()
{
    ImGui::PushFont(font_hud);

    auto io = ImGui::GetIO();

    //--------------------------------------------------
    // Player HP
    // プレイヤーHP表示
    //--------------------------------------------------
    ImGui::SetCursorPos(ImVec2(10, io.DisplaySize.y - ImGui::GetFontSize()));
    if (player::PlayerHealth > 0)
    {
        ImGui::Text(("HP:" + std::to_string(player::PlayerHealth)).c_str());
    }
    else
    {
        ImGui::Text("Disconnected");
    }
    // Detect HP decrease (start flash)
    if (s_prevHP < 0) s_prevHP = player::PlayerHealth; // first time init

    if (player::PlayerHealth < s_prevHP)
    {
        s_damageFlashT = 0.0f; // trigger
    }
    s_prevHP = player::PlayerHealth;

    //--------------------------------------------------
    // Mission Start Text
    // ミッション開始テキスト
    //--------------------------------------------------
    if (s_missionTextTimer >= 0.0f)
    {
        s_missionTextTimer += ImGui::GetIO().DeltaTime;

        float alpha = 1.0f;

        // Fade out during last 0.5 seconds
        // 最後の0.5秒でフェードアウト
        if (s_missionTextTimer > kMissionTextTime - 0.5f)
        {
            float t = (s_missionTextTimer - (kMissionTextTime - 0.5f)) / 0.5f;
            alpha = 1.0f - std::clamp(t, 0.0f, 1.0f);
        }

        if (s_missionTextTimer >= kMissionTextTime)
        {
            s_missionTextTimer = -1.0f; // End display / 表示終了
        }
        else
        {
            auto* bg = ImGui::GetBackgroundDrawList();
            auto* dl = ImGui::GetForegroundDrawList();

			//black background
            bg->AddRectFilled(
                ImVec2(0, 0),
                io.DisplaySize,
                IM_COL32(0, 0, 0, (int)(alpha * 120))
            );
            const char* text = "MISSION START";
            ImGui::SetWindowFontScale(2.0f);
            ImVec2 size = ImGui::CalcTextSize(text);
            ImVec2 pos(
                (float)WindowWidth * 0.5f - size.x * 0.5f,
                (float)WindowHeight * 0.5f
            );

            dl->AddText(
                pos,
                IM_COL32(255, 255, 255, (int)(alpha * 200)),
                text
            );
            ImGui::SetWindowFontScale(1.0f);
        }
    }
    // Draw damage flash overlay
    if (s_damageFlashT >= 0.0f)
    {
        s_damageFlashT += ImGui::GetIO().DeltaTime;

        float t = s_damageFlashT / kDamageFlashTime;
        t = std::clamp(t, 0.0f, 1.0f);

        // alpha from strong -> 0
        float alpha = (1.0f - t) * 0.45f; // ?度：0.3~0.6自己?

        if (s_damageFlashT >= kDamageFlashTime)
        {
            s_damageFlashT = -1.0f;
        }
        else
        {
            ImGuiIO& io = ImGui::GetIO();
            auto* bg = ImGui::GetBackgroundDrawList();
            bg->AddRectFilled(
                ImVec2(0, 0),
                io.DisplaySize,
                IM_COL32(255, 0, 0, (int)(alpha * 255))
            );
        }
    }
    //--------------------------------------------------
    // Boss Marker (during gameplay)
    // ボスマーカー（プレイ中のみ）
    //--------------------------------------------------
    if (g_HasBoss)
    {
        auto* dl = ImGui::GetForegroundDrawList();

        Vector2 boss2D;
        bool projected = WorldToScreen(g_BossPos, boss2D);

        // Check if boss is in front of camera using W sign
        // W成分の符号でカメラ前方・背面を判定
        bool inFront = (XMVectorGetW(
            XMVector4Transform(
                XMVectorSet(g_BossPos.x, g_BossPos.y, g_BossPos.z, 1.0f),
                m_view * m_proj)) > 0.0f);

        // On-screen check with margin
        // マージン付き画面内判定
        const float margin = 25.0f;
        bool onScreen =
            inFront &&
            boss2D.x >= margin && boss2D.x <= (float)WindowWidth - margin &&
            boss2D.y >= margin && boss2D.y <= (float)WindowHeight - margin;

        // Distance to boss
        // ボスまでの距離
        float dist = (g_BossPos - player::PlayerPosition).Length();

        if (onScreen)
        {
            // Boss visible on screen
            // ボスが画面内に存在
            dl->AddCircleFilled(ImVec2(boss2D.x, boss2D.y), 6.0f, IM_COL32(255, 80, 80, 255));
            dl->AddText(
                ImVec2(boss2D.x + 10, boss2D.y - 10),
                IM_COL32(255, 255, 255, 160),
                (std::string("Target ") + std::to_string((int)dist) + "m").c_str()
            );
        }
        else
        {
            // Boss is off-screen
            // ボスが画面外に存在
            Vector2 edgePos;

            if (!inFront)
            {
                // Boss is behind player: force indicator to top edge
                // 背面時：画面上端に固定表示
                edgePos.x = (float)WindowWidth * 0.5f;
                edgePos.y = margin;

                dl->AddCircleFilled(ImVec2(edgePos.x, edgePos.y), 7.0f, IM_COL32(255, 80, 80, 255));
                dl->AddCircle(ImVec2(edgePos.x, edgePos.y), 10.0f, IM_COL32(255, 255, 255, 200), 0, 2.0f);
                dl->AddText(
                    ImVec2(edgePos.x + 10, edgePos.y - 10),
                    IM_COL32(255, 255, 255, 160),
                    (std::string("Target ") + std::to_string((int)dist) + "m").c_str()
                );
            }
            else
            {
                // In front but outside screen: clamp to screen edge
                // 前方だが画面外：画面端にクランプ
                if (ClampToScreenEdge(boss2D, edgePos, margin))
                {
                    dl->AddCircleFilled(ImVec2(edgePos.x, edgePos.y), 7.0f, IM_COL32(255, 80, 80, 255));
                    dl->AddCircle(ImVec2(edgePos.x, edgePos.y), 10.0f, IM_COL32(255, 255, 255, 200), 0, 2.0f);
                    dl->AddText(
                        ImVec2(edgePos.x + 10, edgePos.y - 10),
                        IM_COL32(255, 255, 255, 160),
                        (std::string("Target ") + std::to_string((int)dist) + "m").c_str()
                    );
                }
            }
        }
    }

    ImGui::PopFont();
}

//--------------------------------------------------
// Clamp off-screen position to screen edge
// 画面外座標を画面端へクランプ
//--------------------------------------------------
static bool ClampToScreenEdge(
    const Vector2& screenPos,   // Target position (may be off-screen)
    // 対象スクリーン座標（画面外の可能性あり）
    Vector2& outPos,            // Output position
    // 出力座標
    float margin                // Margin from screen edge
    // 画面端からの余白
)
{
    const float w = (float)WindowWidth;
    const float h = (float)WindowHeight;

    // Screen center
    // 画面中心
    Vector2 center(w * 0.5f, h * 0.5f);
    Vector2 d = screenPos - center;

    // Avoid division by zero
    // ゼロ除算防止
    if (fabsf(d.x) < 1e-6f && fabsf(d.y) < 1e-6f)
    {
        outPos = center;
        return true;
    }

    float left = margin;
    float right = w - margin;
    float top = margin;
    float bottom = h - margin;

    // Compute intersection with screen rectangle
    // 方向ベクトルと画面矩形の交点を計算
    float tx = 1e9f, ty = 1e9f;

    if (fabsf(d.x) > 1e-6f)
    {
        float t1 = (left - center.x) / d.x;
        float t2 = (right - center.x) / d.x;
        if (t1 > 0) tx = std::min(tx, t1);
        if (t2 > 0) tx = std::min(tx, t2);
    }

    if (fabsf(d.y) > 1e-6f)
    {
        float t1 = (top - center.y) / d.y;
        float t2 = (bottom - center.y) / d.y;
        if (t1 > 0) ty = std::min(ty, t1);
        if (t2 > 0) ty = std::min(ty, t2);
    }

    float t = std::min(tx, ty);
    if (t >= 1e8f) return false;

    outPos = center + d * t;

    // Final safety clamp
    // 最終安全クランプ
    outPos.x = std::clamp(outPos.x, left, right);
    outPos.y = std::clamp(outPos.y, top, bottom);

    return true;
}
