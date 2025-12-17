#include "pch.h"
#include "monster_zombie.h"
#include "main.h"
#include "PlayerController.h"
#include "EntityList.h"
#include "LevelManager.h"
#include "render.h"
#include "ParticleSystem.h"



//float STEP_HEIGHT = 0.5f;
float GRAVITY = 0.900f;
float FRICTION = 25.0f;
float MOVE_SPEED = 2.0f;
bool visible = false;



void monster_zombie_think()
{
    // 每帧都让它继续更新（调度系统用）
    self->next_think = Time;

    // =====================================================
    // 1) 可见性（保留你的逻辑）
    // =====================================================
    visible = true;

    Vector3 viewDir = player::PlayerCollider.Center - self->position;
    if (viewDir.LengthSquared() > 0.0001f)
        viewDir.Normalize();

    float playerDist = FLT_MAX;

    if (player::PlayerCollider.Intersects(self->position, viewDir, playerDist))
    {
        for (int i = 0; i < (int)LevelBBoxes.size(); i++)
        {
            float dist;
            if (LevelBBoxes[i].Intersects(self->position, viewDir, dist))
            {
                if (dist < playerDist)
                {
                    visible = false;
                    break;
                }
            }
        }
    }

    // =====================================================
    // 2) 攻击怪：可见才发射抛物线
    //    超时/撞墙/命中 -> 删除
    // =====================================================
    if (self->isAttacker && visible)
    {
        bool hasLaunched = self->velocity.LengthSquared() > 0.0001f;

        // ===== 还没开始攻击 → 像普通怪一样绕 boss =====

        // 2-1) 第一次进入攻击：初始化弹道（只做一次）
        if (self->velocity.LengthSquared() < 0.0001f)
        {
            Vector3 start = self->position;
            Vector3 target = player::PlayerCollider.Center; // 锁定发射瞬间

            float flightTime = 1.2f;
            Vector3 gravity(0, -GRAVITY, 0);

            Vector3 baseV =
                (target - start - 0.5f * gravity * flightTime * flightTime) / flightTime;

            Vector3 lateral(baseV.z, 0, -baseV.x);
            if (lateral.LengthSquared() > 0.0001f)
                lateral.Normalize();

            float spread = ((rand() % 100) / 100.0f - 0.5f) * 5.f;
            self->velocity = baseV + lateral * spread;

            // ★ 超时截止时间存在 angles.z（不新增变量）
            self->angles.z = Time + 2.0f;
        }
        else
        {
            // 2-2) 超时删除（angles.z 是 expireTime）
            if (Time > self->angles.z)
            {
                particle_system::EmitExplosion(self->position, 20, 3.0f, 1.0f);
                self->think = SUB_Remove;
                self->next_think = 0;
                return;
            }
        }

        // 2-3) 重力 + 预测位置
        self->velocity.y -= GRAVITY * DeltaTime;
        Vector3 nextPos = self->position + self->velocity * DeltaTime;

        // 2-4) 撞墙删除
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

        // 2-5) 命中玩家删除
        BoundingBox monsterBox;
        monsterBox.Center = nextPos;
        monsterBox.Extents = self->size / 2;

        if (monsterBox.Intersects(player::PlayerCollider))
        {
            player::PlayerHealth -= 1;
            if (player::PlayerHealth <= 0)
            {
                player::PlayerHealth = 0;
                GameState = GAME_FAIL;
                g_CrtShutdown = true;
                g_CrtTime = 0.0f;
            }
            particle_system::EmitExplosion(self->position, 20, 3.0f, 0.8f);

            self->think = SUB_Remove;
            self->next_think = 0;
            return;
        }

        // 2-6) 应用位置
        self->position = nextPos;
        self->bbox.Center = self->position;
        return;
    }

    // =====================================================
    // 3) 非攻击怪：绕 Boss（球面，不螺旋）
    // =====================================================
    if (!self->isAttacker||!self->velocity.LengthSquared() > 0.0001f)
    {
        const float BASE_RADIUS = 5.0f;
        const float ORBIT_SPEED = 0.8f;
        
        // 更新角度（只改 theta）
        self->angles.y += ORBIT_SPEED * DeltaTime;

        float y = self->angles.x;   // [-1,1]
        float theta = self->angles.y;

        // 计算单位方向（关键）
        float radiusXZ = sqrtf(1.0f - y * y);
        if (radiusXZ < 0.05f)
            radiusXZ = 0.05f;
        Vector3 dir(
            radiusXZ * cosf(theta),
            y,
            radiusXZ * sinf(theta)
        );
        dir.Normalize();   // ★ 必须

        // 呼吸半径（只影响长度）
        float pulse = 0.5f * sinf(Time * 0.8f + theta);
        float radius = BASE_RADIUS + pulse*5.0;

        // ★ 最终位置：唯一来源
        self->position = g_BossPos + dir * radius;

        self->bbox.Center = self->position;
        self->next_think = Time;
        return;
    }
}



void boss_think()
{
	self->angles.x += XM_PIDIV2 * DeltaTime;
	self->bbox.Center = self->position;

	// ===== 波次刷新条件 =====
	if (g_AttackerCount == 0 &&
		Time - g_LastClearTime > CLEAR_COOLDOWN)
	{
        for (int i = 0; i < EntityList.size(); i++)
        {
            // monster_zombie 的判定方式：用 think 函数
            if (EntityList[i].think == monster_zombie_think)
            {
                EntityList[i].think = SUB_Remove;
                EntityList[i].next_think = 0;
            }
        }
		SpawnWave();
		g_LastClearTime = Time;
	}

	self->next_think = Time;
}
