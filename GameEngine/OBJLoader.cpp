#include "pch.h"
#include "OBJLoader.h"


// =====================================================
// OBJ ローダ（map 版）
// ・マテリアル（usemtl）ごとに
//   頂点配列 / インデックス配列を分離して保持する
// ・主に「レベル / 静的メッシュ」用途
// =====================================================
bool LoadOBJ(const std::string& filename,
    std::vector<std::string>& textureNames,
    std::map<std::string, GeometricPrimitive::VertexCollection>& vertexDataMap,
    std::map<std::string, GeometricPrimitive::IndexCollection>& indexDataMap,
    bool flip_faces)
{
    // -------------------------------------------------
    // OBJ ファイルを開く
    // -------------------------------------------------
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cout << "1Failed to open the OBJ file: " << filename << std::endl;
        return false;
    }

    // 現在使用中のマテリアル名
    // usemtl が出現しない場合に備えたデフォルト値
    std::string currentTextureName = "default";

    // OBJ 内の生データを一時的に保持
    std::vector<XMFLOAT3> positions;   // 頂点位置（v）
    std::vector<XMFLOAT3> normals;     // 法線（vn）
    std::vector<XMFLOAT2> texcoords;   // UV 座標（vt）

    // =================================================
    // OBJ ファイルを 1 行ずつ解析
    // =================================================
    while (!file.eof())
    {
        std::string line;
        std::getline(file, line);
        std::istringstream iss(line);

        std::string type;
        iss >> type;

        // -----------------------------
        // 頂点座標（v）
        // -----------------------------
        if (type == "v")
        {
            XMFLOAT3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        // -----------------------------
        // テクスチャ座標（vt）
        // -----------------------------
        else if (type == "vt")
        {
            XMFLOAT2 texcoord;
            iss >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        }
        // -----------------------------
        // 法線（vn）
        // -----------------------------
        else if (type == "vn")
        {
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        // -----------------------------
        // 使用マテリアル切り替え（usemtl）
        // -----------------------------
        else if (type == "usemtl")
        {
            iss >> currentTextureName;

            // 新しいマテリアル名ならリストに追加
            if (std::find(textureNames.begin(), textureNames.end(),
                currentTextureName) == textureNames.end())
            {
                textureNames.push_back(currentTextureName);
            }
        }
        // -----------------------------
        // 面定義（f）
        // 三角形前提で 3 頂点処理
        // -----------------------------
        else if (type == "f")
        {
            for (int i = 0; i < 3; ++i)
            {
                // OBJ の f フォーマット：v / vt / vn
                int indexPosition, indexTexCoord, indexNormal;
                char separator;

                iss >> indexPosition >> separator
                    >> indexTexCoord >> separator
                    >> indexNormal;

                // 頂点データ構築
                VertexData vertexData;
                vertexData.position = positions[indexPosition - 1];
                vertexData.texcoord = texcoords[indexTexCoord - 1];
                vertexData.normal = normals[indexNormal - 1];

                // -------------------------------------------------
                // マテリアル単位での配列が未作成なら生成
                // -------------------------------------------------
                if (vertexDataMap.find(currentTextureName) == vertexDataMap.end())
                {
                    vertexDataMap[currentTextureName] =
                        GeometricPrimitive::VertexCollection();
                    indexDataMap[currentTextureName] =
                        GeometricPrimitive::IndexCollection();
                }

                auto& vertices = vertexDataMap[currentTextureName];
                auto& indices = indexDataMap[currentTextureName];

                // -------------------------------------------------
                // 既存頂点と一致するかチェック
                // （position / normal / UV の完全一致）
                // -------------------------------------------------
                int index = -1;
                for (size_t j = 0; j < vertices.size(); ++j)
                {
                    if (Vector3(vertices[j].position) == Vector3(vertexData.position) &&
                        Vector3(vertices[j].normal) == Vector3(vertexData.normal) &&
                        Vector2(vertices[j].textureCoordinate) == Vector2(vertexData.texcoord))
                    {
                        index = static_cast<int>(j);
                        break;
                    }
                }

                // -------------------------------------------------
                // 新規頂点なら追加
                // -------------------------------------------------
                if (index == -1)
                {
                    vertices.push_back(
                        GeometricPrimitive::VertexType(
                            vertexData.position,
                            vertexData.normal,
                            vertexData.texcoord)
                    );
                    index = static_cast<int>(vertices.size() - 1);
                }

                // インデックス登録
                indices.push_back(index);
            }
        }
    }

    file.close();

    // -------------------------------------------------
    // 面反転（右手系 / 左手系対応用）
    // -------------------------------------------------
    if (flip_faces)
    {
        for (auto& indexDataPair : indexDataMap)
        {
            std::reverse(
                indexDataPair.second.begin(),
                indexDataPair.second.end()
            );
        }
    }

    return true;
}



// =====================================================
// OBJ ローダ（array 版）
// ・マテリアル順に配列で保持
// ・主に動的オブジェクト / 個別モデル用途
// =====================================================
void LoadOBJ(const std::string& filename,
    std::vector<std::string>& textureNames,
    std::vector<GeometricPrimitive::VertexCollection>& vertexDataArray,
    std::vector<GeometricPrimitive::IndexCollection>& indexDataArray,
    bool flip_faces)
{
    // -------------------------------------------------
    // OBJ ファイルを開く
    // -------------------------------------------------
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cout << "2Failed to open the OBJ file: " << filename << std::endl;
        return;
    }

    // 現在のマテリアル名
    std::string currentTextureName = "default";

    // 生データ格納
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texcoords;

    // =================================================
    // OBJ 解析ループ
    // =================================================
    while (!file.eof())
    {
        std::string line;
        std::getline(file, line);
        std::istringstream iss(line);

        std::string type;
        iss >> type;

        // 頂点位置
        if (type == "v")
        {
            XMFLOAT3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        // UV
        else if (type == "vt")
        {
            XMFLOAT2 texcoord;
            iss >> texcoord.x >> texcoord.y;
            texcoords.push_back(texcoord);
        }
        // 法線
        else if (type == "vn")
        {
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        // マテリアル切り替え
        else if (type == "usemtl")
        {
            iss >> currentTextureName;

            // デバッグ用出力（マテリアル名と配列サイズ）
            std::cout << currentTextureName << vertexDataArray.size() << std::endl;

            // 新規マテリアルなら配列追加
            if (std::find(textureNames.begin(), textureNames.end(),
                currentTextureName) == textureNames.end())
            {
                textureNames.push_back(currentTextureName);
                vertexDataArray.push_back(GeometricPrimitive::VertexCollection());
                indexDataArray.push_back(GeometricPrimitive::IndexCollection());
            }
        }
        // 面定義
        else if (type == "f")
        {
            for (int i = 0; i < 3; ++i)
            {
                int indexPosition, indexTexCoord, indexNormal;
                char separator;

                iss >> indexPosition >> separator
                    >> indexTexCoord >> separator
                    >> indexNormal;

                VertexData vertexData;
                vertexData.position = positions[indexPosition - 1];
                vertexData.texcoord = texcoords[indexTexCoord - 1];
                vertexData.normal = normals[indexNormal - 1];

                // -------------------------------------------------
                // 現在のマテリアル配列の末尾に対して
                // 既存頂点チェック
                // -------------------------------------------------
                int index = -1;
                auto& vertices = vertexDataArray.back();

                for (size_t j = 0; j < vertices.size(); ++j)
                {
                    if (Vector3(vertices[j].position) == Vector3(vertexData.position) &&
                        Vector3(vertices[j].normal) == Vector3(vertexData.normal) &&
                        Vector2(vertices[j].textureCoordinate) == Vector2(vertexData.texcoord))
                    {
                        index = static_cast<int>(j);
                        break;
                    }
                }

                // 新規頂点なら追加
                if (index == -1)
                {
                    vertices.push_back(
                        GeometricPrimitive::VertexType(
                            vertexData.position,
                            vertexData.normal,
                            vertexData.texcoord)
                    );
                    index = static_cast<int>(vertices.size() - 1);
                }

                // インデックス登録
                indexDataArray.back().push_back(index);
            }
        }
    }

    file.close();

    // 面反転処理
    if (flip_faces)
    {
        for (int i = 0; i < indexDataArray.size(); i++)
        {
            std::reverse(
                indexDataArray[i].begin(),
                indexDataArray[i].end()
            );
        }
    }
}
