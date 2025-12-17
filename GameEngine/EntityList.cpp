#include "pch.h"
#include "EntityList.h"
#include "main.h"
#include "dx_tools.h"
#include "PlayerController.h"
#include "LevelManager.h"
#include "monster_zombie.h"
#include "render.h"


// =====================================================
// 通常モンスター（ゾンビ）をエンティティリストに追加
// ・初期位置
// ・当たり判定
// ・思考関数
// をここで設定する
// =====================================================
void AddMonster(std::string name, Vector3 pos)
{
	// エンティティ構造体を初期化
	ENTITY ent{};
	ent.name = name;

	// 初期ワールド座標
	ent.position = pos;

	// 回転角（今回は使用しないためゼロ）
	ent.angles = Vector3::Zero;

	// 当たり判定サイズ（立方体）
	ent.size = Vector3(1.0f, 1.f, 1.f);

	// 使用するスプライト（テクスチャ）
	ent.model = "data\\textures\\monsters\\mino.dds";

	// 思考関数（毎フレームの挙動を制御）
	ent.think = monster_zombie_think;

	// 次回思考時刻（即時実行）
	ent.next_think = Time;

	// バウンディングボックス初期化
	ent.bbox.Center = pos;
	ent.bbox.Extents = ent.size / 2;

	// ビルボード描画フラグ（常にカメラを向く）
	ent.sprite = true;

	// 体力
	ent.health = 2;

	// エンティティリストに追加
	EntityList.push_back(ent);

	// 初期位置をホームポジションとして保存
	ent.homePos = pos;
}



// =====================================================
// Boss エンティティを追加
// ・サイズ・体力が大きい
// ・ビルボードではなく立体描画
// ・専用思考関数を使用
// =====================================================
void AddBoss(std::string name, Vector3 pos)
{
	ENTITY ent;

	ent.name = name;

	// 初期位置
	ent.position = pos;

	// 回転角初期化
	ent.angles = Vector3::Zero;

	// Boss は通常モンスターより大きい
	ent.size = Vector3(2.f, 2.f, 2.f);

	// Boss 用テクスチャ
	ent.model = "data\\textures\\monsters\\Boss.dds";

	// Boss の体力
	ent.health = 20;

	// Boss 専用の思考関数
	ent.think = boss_think;

	// 初回は即時実行しない
	ent.next_think = 0;

	// スプライトではなく立体オブジェクト
	ent.sprite = false;

	// 当たり判定設定
	ent.bbox.Center = pos;
	ent.bbox.Extents = ent.size * 0.5f;

	// 念のため明示的に false
	ent.sprite = false;

	// エンティティリストに追加
	EntityList.push_back(ent);
}



// =====================================================
// エンティティ削除用の共通関数
// ・selfID を利用して現在処理中のエンティティを削除
// =====================================================
void SUB_Remove()
{
	EntityList.erase(EntityList.begin() + selfID);
}



// =====================================================
// 全エンティティの描画 & 更新処理
// ・描画
// ・当たり判定更新
// ・思考関数（AI）実行
// を 1 フレーム内でまとめて行う
// =====================================================
void RenderEntityList(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// -------------------------------------------------
	// エンティティごとのループ
	// -------------------------------------------------
	for (int i = 0; i < EntityList.size(); i++)
	{
		// -----------------------------
		// モデル（テクスチャ指定）がある場合のみ描画
		// -----------------------------
		if (EntityList[i].model.size())
		{
			// メッシュ未生成なら生成
			if (!EntityList[i].mesh)
			{
				EntityList[i].mesh =
					GeometricPrimitive::CreateBox(context, EntityList[i].size);
			}

			// テクスチャ未ロードならロード
			if (!EntityList[i].texture)
			{
				std::wstring texture_path(
					EntityList[i].model.begin(),
					EntityList[i].model.end()
				);

				if (CreateDDSTextureFromFile(
					device,
					texture_path.c_str(),
					nullptr,
					EntityList[i].texture.ReleaseAndGetAddressOf()) != S_OK)
				{
					printf("ERROR LOADING TEXTURE: %s\n",
						EntityList[i].model.c_str());

					// フォールバック用ダミーテクスチャ
					GenerateTexture(
						device,
						EntityList[i].texture.ReleaseAndGetAddressOf());
				}
			}

			// -----------------------------
			// 描画処理
			// -----------------------------
			if (EntityList[i].mesh && EntityList[i].texture)
			{
				// ワールド変換（位置のみ）
				Matrix worldMatrix =
					Matrix::CreateTranslation(EntityList[i].position);

				// スプライト（ビルボード）の場合はカメラ向きに回転
				if (EntityList[i].sprite)
				{
					XMMATRIX rotationMatrix =
						LookInDirection(
							EntityList[i].position,
							m_cameraPos,
							Vector3::Up);

					worldMatrix =
						Matrix(rotationMatrix) * worldMatrix;
				}

				// シェーダ設定
				m_effect->Apply(context);
				context->IASetInputLayout(m_inputLayout);
				m_effect->SetWorld(worldMatrix);
				m_effect->SetView(m_view);
				m_effect->SetProjection(m_proj);
				m_effect->SetTexture(EntityList[i].texture.Get());

				// メッシュ描画
				EntityList[i].mesh->Draw(
					m_effect.get(),
					m_inputLayout);
			}
		}

		// -------------------------------------------------
		// 当たり判定の更新
		// -------------------------------------------------
		EntityList[i].bbox.Center = EntityList[i].position;
		EntityList[i].bbox.Extents = EntityList[i].size / 2;

		// -------------------------------------------------
		// 思考関数（AI）の実行
		// ・self / selfID を設定してから呼び出す
		// -------------------------------------------------
		selfID = i;
		self = &EntityList[i];

		if (EntityList[i].think != nullptr &&
			EntityList[i].next_think < Time)
		{
			EntityList[i].think();
		}

		// （以下は実験・デバッグ用コード群）
	}
}



			//Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, -m_pitch, 0.f);
			//DirectX::XMVECTOR rotationQuaternion = DirectX::XMLoadFloat4(&q);
			//Vector3 cameraForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			//Vector3 cameraUp = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), q);



			//Vector3 cubePosition = Vector3(0, 1, 0); // Replace with the cube's position
			//Vector3 cubeForward = cameraForward;    // Negative view direction to face the camera

			//// Compute the right and up vectors for the cube's orientation
			//Vector3 cubeRight = cameraUp.Cross(cubeForward);
			//Vector3 cubeUp = cubeForward.Cross(cubeRight);
			//cubeRight.Normalize();
			//cubeUp.Normalize();

			//Matrix cubeTransform(
			//	cubeRight.x, cubeRight.y, cubeRight.z, 0.0f,
			//	cubeUp.x, cubeUp.y, cubeUp.z, 0.0f,
			//	cubeForward.x, cubeForward.y, cubeForward.z, 0.0f,
			//	cubePosition.x, cubePosition.y, cubePosition.z, 1.0f
			//);
			//for (auto& it : cubeVerts)
			//{
			//	it.position = Vector3::Transform(it.position, cubeTransform);
			//}

			//XMVECTOR cameraPosition = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f); // Replace this with your actual camera position
			//XMVECTOR cameraTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // Replace this with your actual camera target
			//XMVECTOR cameraUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // Replace this with your actual camera up vector


			//context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
			//context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			//context->RSSetState(m_states->CullClockwise());

			////m_effect->Apply(context);
			////context->IASetInputLayout(m_inputLayout);

			////primitiveBatch->Begin();
			//////VertexPositionColor v1(Vector3(0.f, 0.5f, 0.5f), Colors::Yellow);
			//////VertexPositionColor v2(Vector3(0.5f, -0.5f, 0.5f), Colors::Yellow);
			//////VertexPositionColor v3(Vector3(-0.5f, -0.5f, 0.5f), Colors::Yellow);
			//////primitiveBatch->DrawTriangle(v1, v2, v3);

			////VertexPositionColor v1(EntityList[i].position + EntityList[i].size, Colors::Yellow);
			////VertexPositionColor v2(EntityList[i].position + Vector3(-EntityList[i].size.x, EntityList[i].size.y, EntityList[i].size.z), Colors::Yellow);
			////VertexPositionColor v3(EntityList[i].position - EntityList[i].size, Colors::Yellow);
			////VertexPositionColor v4(EntityList[i].position - Vector3(-EntityList[i].size.x, EntityList[i].size.y, EntityList[i].size.z), Colors::Yellow);
			////primitiveBatch->DrawTriangle(v1, v2, v3);
			//////primitiveBatch->DrawQuad(v1, v2, v3, v4);
			////primitiveBatch->End();



			//LevelObjects.push_back(GeometricPrimitive::CreateCustom(context, cubeVerts, cubeIndices));

			//GeometricPrimitive::CreateBox(context, Vector3(1, 1, 1), true, false);

