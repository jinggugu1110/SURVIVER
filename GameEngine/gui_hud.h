#pragma once



static bool WorldToScreen(const DirectX::SimpleMath::Vector3& world, DirectX::SimpleMath::Vector2& outScreen);

void DrawHud();

static bool ClampToScreenEdge(const Vector2& screenPos, Vector2& outPos, float margin);

void TriggerMissionStartUI();
