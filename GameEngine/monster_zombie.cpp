#include "pch.h"
#include "monster_zombie.h"
#include "main.h"
#include "PlayerController.h"
#include "EntityList.h"
#include "LevelManager.h"
#include "render.h"
#include "ParticleSystem.h"


// -----------------------------------------------------
// 物理・挙動用のグローバル定数
// -----------------------------------------------------

// 重力加速度（Y 軸方向に影響）
float GRAVITY = 0.900f;

// 摩擦係数（現在は未使用だが、地上移動用に残してある）
float FRICTION = 25.0f;

// 移動速度（非攻撃時の基準速度）
float MOVE_SPEED = 2.0f;

// プレイヤーから視認できているかどうかのフラグ
bool visible = false;



// =====================================================
// ゾンビ（モンスター）AI のメイン思考関数
// 毎フレーム呼ばれ、行動・移動・攻撃判定を行う
// =====================================================
void monster_zombie_think()
{
    // -------------------------------------------------
    // 毎フレーム思考を継続させるため、次回 think を現在時刻に設定
    // （エンジン側のスケジューラ用） 
    // -------------------------------------------------
    self->next_think = Time;

    // =====================================================
    // 1) プレイヤーからの可視判定（視線チェック）
    //    プレイヤーとモンスターの間に障害物があるかを調べる
    // =====================================================
    visible = true;

    // プレイヤー中心 → モンスター位置への方向ベクトル
    Vector3 viewDir = player::PlayerCollider.Center - self->position;

    // ゼロベクトル対策（正規化前の長さチェック）
    if (viewDir.LengthSquared() > 0.0001f)
        viewDir.Normalize();

    // プレイヤーまでの距離（初期値は最大）
    float playerDist = FLT_MAX;

    // プレイヤーとのレイ判定（最初にプレイヤーに当たる距離を取得）
    if (player::PlayerCollider.Intersects(self->position, viewDir, playerDist))
    {
        // レベル内の全 BoundingBox（壁・地形）との遮蔽判定
        for (int i = 0; i < (int)LevelBBoxes.size(); i++)
        {
            float dist;
            if (LevelBBoxes[i].Intersects(self->position, viewDir, dist))
            {
                // 壁がプレイヤーより手前にある場合、不可視
                if (dist < playerDist)
                {
                    visible = false;
                    break;
                }
            }
        }
    }

    // =====================================================
    // 2) 攻撃型モンスターの処理
    //    ・プレイヤーが見えている時のみ攻撃
    //    ・放物線弾道で突撃
    //    ・時間切れ / 壁衝突 / プレイヤー命中で消滅
    // =====================================================
    if (self->isAttacker && visible)
    {
        // すでに発射済みかどうか（速度があるかで判定）
        bool hasLaunched = self->velocity.LengthSquared() > 0.0001f;

        // -------------------------------------------------
        // 2-1) 攻撃開始時（初回のみ）
        //      放物線の初速を計算して設定する
        // -------------------------------------------------
        if (self->velocity.LengthSquared() < 0.0001f)
        {
            // 発射開始位置
            Vector3 start = self->position;

            // 発射瞬間のプレイヤー位置をロックオン
            Vector3 target = player::PlayerCollider.Center;

            // 飛行時間（弾道全体の長さ）
            float flightTime = 1.2f;

            // 重力ベクトル（Y 軸下向き）
            Vector3 gravity(0, -GRAVITY, 0);

            // 放物線の公式から初速度を逆算
            Vector3 baseV =
                (target - start - 0.5f * gravity * flightTime * flightTime) / flightTime;

            // 横方向のばらつきを作るための直交ベクトル
            Vector3 lateral(baseV.z, 0, -baseV.x);
            if (lateral.LengthSquared() > 0.0001f)
                lateral.Normalize();

            // ランダムな散らばり（弾のばらつき）
            float spread = ((rand() % 100) / 100.0f - 0.5f) * 5.f;

            // 最終的な初速を設定
            self->velocity = baseV + lateral * spread;

            // ★ angles.z を「寿命タイマー」として流用
            //   新しい変数を増やさずに、消滅時間を管理している
            self->angles.z = Time + 2.0f;
        }
        else
        {
            // -------------------------------------------------
            // 2-2) 寿命切れ判定
            // -------------------------------------------------
            if (Time > self->angles.z)
            {
                // 爆発エフェクト生成
                particle_system::EmitExplosion(self->position, 20, 3.0f, 1.0f);

                // エンティティ削除
                self->think = SUB_Remove;
                self->next_think = 0;
                return;
            }
        }

        // -------------------------------------------------
        // 2-3) 重力適用 & 次フレームの予測位置
        // -------------------------------------------------
        self->velocity.y -= GRAVITY * DeltaTime;
        Vector3 nextPos = self->position + self->velocity * DeltaTime;

        // -------------------------------------------------
        // 2-4) 壁衝突判定（予測位置）
        // -------------------------------------------------
        BoundingBox nextBox;
        nextBox.Center = nextPos;
        nextBox.Extents = self->size / 2;

        for (int i = 0; i < (int)LevelBBoxes.size(); i++)
        {
            if (LevelBBoxes[i].Intersects(nextBox))
            {
                particle_system::EmitExplosion(self->position, 20, 3.0f, 1.0f);
                self->think = SUB_Remove;
                self->next_think = 0;
                return;
            }
        }

        // -------------------------------------------------
        // 2-5) プレイヤー命中判定
        // -------------------------------------------------
        BoundingBox monsterBox;
        monsterBox.Center = nextPos;
        monsterBox.Extents = self->size / 2;

        if (monsterBox.Intersects(player::PlayerCollider))
        {
            // プレイヤーにダメージ
            player::PlayerHealth -= 1;

            // ゲームオーバー判定
            if (player::PlayerHealth <= 0)
            {
                player::PlayerHealth = 0;
                GameState = GAME_FAIL;
                g_CrtShutdown = true;
                g_CrtTime = 0.0f;
            }

            // 命中エフェクト
            particle_system::EmitExplosion(self->position, 20, 3.0f, 0.8f);

            // 攻撃体を削除
            self->think = SUB_Remove;
            self->next_think = 0;
            return;
        }

        // -------------------------------------------------
        // 2-6) 位置更新の確定
        // -------------------------------------------------
        self->position = nextPos;
        self->bbox.Center = self->position;
        return;
    }

    // =====================================================
    // 3) 非攻撃型モンスターの処理
    //    ・Boss の周囲を球面上で周回
    //    ・螺旋運動にならないよう方向ベクトルを正規化
    // =====================================================
    if (!self->isAttacker || !self->velocity.LengthSquared() > 0.0001f)
    {
        const float BASE_RADIUS = 5.0f;
        const float ORBIT_SPEED = 0.8f;

        // 周回角度（theta）の更新
        self->angles.y += ORBIT_SPEED * DeltaTime;

        // y 成分（-1 ～ 1 を想定）
        float y = self->angles.x;
        float theta = self->angles.y;

        // XZ 平面での半径を計算
        float radiusXZ = sqrtf(1.0f - y * y);
        if (radiusXZ < 0.05f)
            radiusXZ = 0.05f;

        // 球面上の方向ベクトルを生成
        Vector3 dir(
            radiusXZ * cosf(theta),
            y,
            radiusXZ * sinf(theta)
        );

        // ★ 正規化しないと半径が壊れるため必須
        dir.Normalize();

        // 半径に呼吸のような揺らぎを加える
        float pulse = 0.5f * sinf(Time * 0.8f + theta);
        float radius = BASE_RADIUS + pulse * 5.0;

        // ★ Boss を中心とした最終位置
        self->position = g_BossPos + dir * radius;

        self->bbox.Center = self->position;
        self->next_think = Time;
        return;
    }
}



// =====================================================
// Boss の思考関数
// ・回転処理
// ・全敵撃破後の波次リスポーン制御
// =====================================================
void boss_think()
{
    // Boss の回転演出
    self->angles.x += XM_PIDIV2 * DeltaTime;
    self->bbox.Center = self->position;

    // -------------------------------------------------
    // 波次（Wave）更新条件
    // ・攻撃型モンスターが全滅
    // ・クールダウン時間経過
    // -------------------------------------------------
    if (g_AttackerCount == 0 &&
        Time - g_LastClearTime > CLEAR_COOLDOWN)
    {
        // 既存のゾンビを全削除
        for (int i = 0; i < EntityList.size(); i++)
        {
            // think 関数が monster_zombie_think のものを対象とする
            if (EntityList[i].think == monster_zombie_think)
            {
                EntityList[i].think = SUB_Remove;
                EntityList[i].next_think = 0;
            }
        }

        // 新しい敵ウェーブ生成
        SpawnWave();
        g_LastClearTime = Time;
    }

    self->next_think = Time;
}
