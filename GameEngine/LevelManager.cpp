#include "pch.h"
#include "LevelManager.h"
#include "OBJLoader.h"
#include "dx_tools.h"
#include "render.h"
#include "EntityList.h"
#include "PlayerController.h"
#include "monster_zombie.h" 
#include "ParticleSystem.h"


// -----------------------------------------------------
// 攻撃型モンスターの残数
// Boss のウェーブ制御に使用
// -----------------------------------------------------
int g_AttackerCount = 0;

// 最後に敵を全滅させた時刻
// 次ウェーブ生成のクールダウン判定に使用
float g_LastClearTime = 0.0f;



// =====================================================
// OBJ ファイルからレベル用の当たり判定（BoundingBox）を抽出
// ・モンスターや情報オブジェクトは除外
// ・地形・壁のみを LevelBBoxes に登録する
// =====================================================
void extractBoundingBoxes(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // 現在処理中のオブジェクトの頂点リスト
    std::vector<Vector3> vertices;

    std::string line;
    std::string prefix;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        iss >> prefix;

        // -------------------------------------------------
        // 頂点データ（v）
        // -------------------------------------------------
        if (prefix == "v")
        {
            Vector3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        }

        // -------------------------------------------------
        // オブジェクト区切り（o）
        // OBJ 内の各オブジェクトごとに BoundingBox を生成
        // -------------------------------------------------
        if (prefix == "o")
        {
            std::string objectName;
            iss >> objectName;

            // モンスターや info 系オブジェクトは
            // 当たり判定として扱わない
            if (objectName.find("monster_") != std::string::npos ||
                objectName.find("info_") != std::string::npos)
            {
                vertices.clear();
            }
            else
            {
                // これまでに集めた頂点から BoundingBox を作成
                if (!vertices.empty())
                {
                    Vector3 minPoint, maxPoint;
                    minPoint = maxPoint = vertices[0];

                    // 最小・最大座標を計算
                    for (const auto& vertex : vertices)
                    {
                        if (vertex.x < minPoint.x) minPoint.x = vertex.x;
                        if (vertex.y < minPoint.y) minPoint.y = vertex.y;
                        if (vertex.z < minPoint.z) minPoint.z = vertex.z;

                        if (vertex.x > maxPoint.x) maxPoint.x = vertex.x;
                        if (vertex.y > maxPoint.y) maxPoint.y = vertex.y;
                        if (vertex.z > maxPoint.z) maxPoint.z = vertex.z;
                    }

                    BoundingBox box;
                    box.Extents = (maxPoint - minPoint) / 2;
                    box.Center = (minPoint + maxPoint) / 2;

                    // レベル用当たり判定として登録
                    LevelBBoxes.push_back(box);
                }
                vertices.clear();
            }
        }
    }

    file.close();
}



// =====================================================
// レベル解放処理
// ・テクスチャ
// ・メッシュ
// ・当たり判定
// ・エンティティ
// をすべて初期化する
// =====================================================
void UnloadLevel()
{
    CurrentLevel = "";
    LevelBBoxes.clear();

    // テクスチャ解放
    for (int i = 0; i < LevelTextures.size(); i++)
    {
        LevelTextures[i].ReleaseAndGetAddressOf();
    }
    LevelTextures.clear();

    // メッシュ解放
    for (int i = 0; i < LevelObjects.size(); i++)
    {
        LevelObjects[i].release();
    }
    LevelObjects.clear();

    // 既存エンティティを全削除
    for (int i = 0; i < EntityList.size(); i++)
    {
        EntityList[i].think = SUB_Remove;
        EntityList[i].next_think = 0;
    }
}



// Boss 存在フラグ
bool g_HasBoss = false;

// Boss のワールド座標
Vector3 g_BossPos = Vector3::Zero;


// =====================================================
// レベル読み込み処理
// ・OBJ 読み込み
// ・地形生成
// ・モンスター / Boss / プレイヤー開始位置配置
// =====================================================
void LoadLevel(ID3D11Device* device, ID3D11DeviceContext* context, std::string mapName)
{
    UnloadLevel();
    CurrentLevel = mapName;

    // -------------------------------------------------
    // OBJ 読み込み
    // -------------------------------------------------
    std::vector<std::string> textureNames;
    std::map<std::string, GeometricPrimitive::VertexCollection> vertexDataMap;
    std::map<std::string, GeometricPrimitive::IndexCollection> indexDataMap;

    std::string objFilename = mapName + ".obj";
    if (!LoadOBJ(objFilename, textureNames, vertexDataMap, indexDataMap, true))
    {
        objFilename = "data/levels/" + mapName + ".obj";
        LoadOBJ(objFilename, textureNames, vertexDataMap, indexDataMap, true);
    }

    // 当たり判定生成
    extractBoundingBoxes(objFilename);

    // -------------------------------------------------
    // OBJ 内オブジェクトの種類別処理
    // -------------------------------------------------
    for (int i = 0; i < vertexDataMap.size(); i++)
    {
        // -------------------------------
        // モンスター配置
        // -------------------------------
        if (textureNames[i].find("monster_") != std::string::npos)
        {
            Vector3 pos = vertexDataMap.at(textureNames[i])[0].position;
            AddMonster(textureNames[i], pos);

            // 距離が離れている頂点ごとに追加配置
            for (int j = 0; j < vertexDataMap.at(textureNames[i]).size(); j++)
            {
                Vector3 newPos = vertexDataMap.at(textureNames[i])[j].position;
                if (Vector3::Distance(pos, newPos) > 1.5f)
                {
                    pos = newPos;
                    AddMonster(textureNames[i], newPos);
                }
            }
            continue;
        }

        // -------------------------------
        // Boss 配置
        // -------------------------------
        if (textureNames[i].find("Boss") != std::string::npos)
        {
            Vector3 bpos = vertexDataMap.at(textureNames[i])[0].position;
            AddBoss(textureNames[i], bpos);

            g_BossPos = bpos;
            g_HasBoss = true;
            continue;
        }

        // -------------------------------
        // プレイヤー開始位置
        // -------------------------------
        if (textureNames[i].find("info_player_start") != std::string::npos)
        {
            g_PlayerSpawnPos = vertexDataMap.at(textureNames[i])[0].position;
            continue;
        }

        // -------------------------------
        // テクスチャ生成
        // -------------------------------
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
        std::string texture_path = textureNames[i] + ".dds";

        if (CreateDDSTextureFromFile(device, STR2WSTR(texture_path).c_str(),
            nullptr, texture.ReleaseAndGetAddressOf()) != S_OK)
        {
            texture_path = PATH_TEXTURES_TILES + texture_path;
            if (CreateDDSTextureFromFile(device, STR2WSTR(texture_path).c_str(),
                nullptr, texture.ReleaseAndGetAddressOf()) != S_OK)
            {
                std::cout << "ERROR LOADING TEXTURE: " << texture_path << std::endl;
                GenerateTexture(device, texture.ReleaseAndGetAddressOf());
            }
        }

        LevelTextures.push_back(texture);

        // -------------------------------
        // メッシュ生成
        // -------------------------------
        LevelObjects.push_back(
            GeometricPrimitive::CreateCustom(
                context,
                vertexDataMap.at(textureNames[i]),
                indexDataMap.at(textureNames[i])
            )
        );
    }
}



// =====================================================
// 敵ウェーブ生成
// ・Boss を中心に球面上へ均等配置
// ・一部を攻撃型モンスターとして設定
// =====================================================
void SpawnWave()
{
    const int ATTACKER_COUNT = 20;
    const int TOTAL = 50;
    const float RADIUS = 4.0f;

    for (int i = 0; i < TOTAL; i++)
    {
        // 球面分布用パラメータ
        float y = 1.0f - 2.0f * (i + 0.5f) / TOTAL;
        y = std::clamp(y, -0.95f, 0.95f);

        float radiusXZ = sqrtf(1.0f - y * y);
        const float golden = XM_PI * (3.0f - sqrtf(5.0f));
        float theta = golden * i;

        Vector3 offset(
            RADIUS * radiusXZ * cosf(theta),
            RADIUS * y,
            RADIUS * radiusXZ * sinf(theta)
        );

        Vector3 pos = g_BossPos + offset;

        AddMonster("monster_zombie", pos);

        ENTITY& e = EntityList.back();

        // 球面パラメータ保存
        e.angles.x = y;
        e.angles.y = theta;

        e.think = monster_zombie_think;
        e.next_think = Time;
        e.velocity = Vector3::Zero;

        // 攻撃型フラグ
        e.isAttacker = (i < ATTACKER_COUNT);
    }
}



// =====================================================
// レベル描画処理
// =====================================================
void RenderLevel(ID3D11DeviceContext* context)
{
    for (int i = 0; i < LevelObjects.size(); i++)
    {
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout);
        m_effect->SetWorld(m_world);
        m_effect->SetView(m_view);
        m_effect->SetProjection(m_proj);
        m_effect->SetTexture(LevelTextures[i].Get());

        LevelObjects[i]->Draw(m_effect.get(), m_inputLayout);
    }
}



// =====================================================
// 全モンスター即時爆破（演出・デバッグ用）
// =====================================================
void ExplodeAllMonsters()
{
    for (int i = 0; i < EntityList.size(); i++)
    {
        ENTITY& e = EntityList[i];

        if (e.think == monster_zombie_think)
        {
            particle_system::EmitExplosion(
                e.position,
                25,     // 粒子数
                3.0f,   // 初速
                0.6f    // 寿命
            );

            e.think = SUB_Remove;
            e.next_think = 0;
        }
    }
}
