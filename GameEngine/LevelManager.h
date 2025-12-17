#pragma once

inline std::vector<std::unique_ptr<DirectX::GeometricPrimitive>> LevelObjects;
inline std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> LevelTextures;
inline std::vector<BoundingBox> LevelBBoxes;
inline Vector3 g_PlayerSpawnPos;


extern bool g_HasBoss;
extern DirectX::SimpleMath::Vector3 g_BossPos;

extern int g_AttackerCount;
extern float g_LastClearTime;

const int   MAX_MONSTERS = 100;
const float CLEAR_COOLDOWN = 3.5f; // X •b

void LoadLevel(ID3D11Device* device, ID3D11DeviceContext* context, std::string mapName);
void SpawnWave();
void RenderLevel(ID3D11DeviceContext* context);
void ExplodeAllMonsters();
inline std::string CurrentLevel;
void UnloadLevel();