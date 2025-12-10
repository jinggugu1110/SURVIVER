#include "pch.h"
#include "monster_zombie.h"
#include "main.h"
#include "PlayerController.h"
#include "EntityList.h"
#include "LevelManager.h"
#include "render.h"
//#include <cstdlib>   // rand()
//#include <ctime>     // time()
//#include <map>

// randfloat01 - 0.0 to 1.0
//inline float RandFloat01()
//{
//	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
//}
//std::map<int, DirectX::SimpleMath::Vector3> g_monsterTargets;

float STEP_HEIGHT = 0.5f;
float GRAVITY = 0.900f;
float FRICTION = 25.0f;
float MOVE_SPEED = 1.0f;

void monster_zombie_think()
{
	bool visible = true;

	Vector3 dir = m_cameraPos - self->position;
	dir.Normalize();


	float playerDist;
	if (player::PlayerCollider.Intersects(self->position, dir, playerDist))
	{
		float dist;
		for (int i = 0; i < LevelBBoxes.size(); i++)
		{
			if (LevelBBoxes[i].Intersects(self->position, dir, dist))
			{
				if (dist < playerDist)
				{
					visible = false;
					break;
				}
			}
		}
	}


	if (visible)
	{
		dir.y = 0;
		Vector3 separation = Vector3::Zero;

		// ====== ï™?óÕ?éZ ======
		float desiredSeparation = 3.0f; // ä˙ñ]?êlîV?ìIç≈è¨ãó?
		for (auto& other : EntityList)
		{
			if (&other == self) continue; // íµ?é©å»
			if (!other.think) continue;   // õﬂ?éÄñSàΩùŸù¡ìI?êlíµ?

			Vector3 diff = self->position - other.position;
			float dist = diff.Length();
			if (dist > 0 && dist < desiredSeparation)
			{
				diff.Normalize();
				separation += diff / dist; // âzãﬂÅCêÑ?ìæâz?
			}
		}

		// ====== çáõÛí«?òaï™? ======
		Vector3 finalDir = dir + separation * 2.0f; // 2.0f = ï™?óÕ?èdÅCâ¬?
		finalDir.Normalize();

		self->velocity = self->velocity + finalDir * MOVE_SPEED;
	}

	//Friction
	if (self->velocity.x || self->velocity.z)
	{
		float f = 1 - DeltaTime * FRICTION;
		self->velocity.x = self->velocity.x * f;
		self->velocity.z = self->velocity.z * f;
	}


	BoundingBox collider;
	collider.Center = self->position;
	collider.Extents = self->size / 2;

	bool grounded = false;
	float lastHeight = -999999.f;
	for (int i = 0; i < LevelBBoxes.size(); i++)
	{
		if (LevelBBoxes[i].Intersects(collider))
		{
			float groundHeight = LevelBBoxes[i].Center.y + LevelBBoxes[i].Extents.y;
			if (groundHeight > lastHeight)
			{
				lastHeight = groundHeight;
				if (groundHeight - STEP_HEIGHT < self->position.y)
				{
					self->position.y = groundHeight + self->size.y / 2;
					grounded = true;
				}
			}
		}
	}

	if (!grounded)
	{
		self->position.y = self->position.y - 1 * DeltaTime;
	}


	self->position = self->position + self->velocity * DeltaTime;

	BoundingBox playerBox = player::PlayerCollider; // äﬂâ∆?ì≥·¥
	BoundingBox monsterBox;
	monsterBox.Center = self->position;
	monsterBox.Extents = self->size / 2;

	if (monsterBox.Intersects(playerBox))
	{
		player::PlayerHealth -= 1;  // ?éü?ì≥ùJ 10 HP
		if (player::PlayerHealth <= 0)
		{
			player::PlayerHealth = 0;
			printf("Game Over!\n");
			// TODO: êÿ?ìûéÂçÿ?/?é¶GameOveräEñ 
		}

		// ?óπîñ∆àÍ?ì‡ëΩéüù{ååÅCâ¬à»??êlâ¡ó‚ãp??
		self->next_think = Time + 1.0f; // 1 ïbç@çƒ??
	}
	//g_PrimaryRay.direction = Vector3::Down;
	//g_PrimaryRay.origin = self->position;
	//float groundDist;
	//float maxDist = 99.f;
	//for (int i = 0; i < LevelBBoxes.size(); i++)
	//{
	//	if (LevelBBoxes[i].Intersects(self->position, Vector3::Down, groundDist))
	//	{
	//		if (groundDist < maxDist)
	//		{
	//			maxDist = groundDist;
	//			printf("%f\n", groundDist);
	//			self->position.y = LevelBBoxes[i].Center.y + (LevelBBoxes[i].Extents.y / 2) + self->size.y / 2;
	//		}
	//	}
	//}



	//self->position.y = self->position.y + 1.f * DeltaTime;
	//printf("%f\n", self->position.y);
}
//void monster_zombie_think()
//{
//    // ==== êèä˜ñ⁄?ì_ê∂ê¨ ====
//    auto id = self->(uintptr_t)self;
//    if (g_monsterTargets.find(id) == g_monsterTargets.end())
//    {
//        // èâénâªàÍò¢êèä˜ñ⁄?ì_
//        float angle = RandFloat01() * DirectX::XM_2PI;
//        float radius = 20.0f + RandFloat01() * 80.0f; // 20~100 ãó?
//        float x = player::PlayerPosition.x + cosf(angle) * radius;
//        float z = player::PlayerPosition.z + sinf(angle) * radius;
//        float y = player::PlayerPosition.y; // ï€éùç›äﬂâ∆ìIçÇìxïçãﬂ
//
//        g_monsterTargets[id] = { x, y, z };
//    }
//
//    // éÊìæìñëOñ⁄?ì_
//    auto& target = g_monsterTargets[id];
//
//    // ==== à⁄??? ====
//    Vector3 dir = target - self->position;
//    if (dir.Length() < 2.0f) // ìû?ñ⁄? Å® ê∂ê¨êVìIñ⁄?
//    {
//        float angle = RandFloat01() * DirectX::XM_2PI;
//        float radius = 20.0f + RandFloat01() * 80.0f; // 20~100 ãó?
//        float x = player::PlayerPosition.x + cosf(angle) * radius;
//        float z = player::PlayerPosition.z + sinf(angle) * radius;
//        float y = player::PlayerPosition.y;
//
//        target = { x, y, z };
//    }
//
//    dir.Normalize();
//    dir.y = 0; // ï€éùç›êÖïΩñ è„
//    self->velocity = dir * MOVE_SPEED;
//
//    // ==== Friction ====
//    if (self->velocity.x || self->velocity.z)
//    {
//        float f = 1 - DeltaTime * FRICTION;
//        self->velocity.x *= f;
//        self->velocity.z *= f;
//    }
//
//    // ==== èdóÕ & ?ì≥ ====
//    BoundingBox collider;
//    collider.Center = self->position;
//    collider.Extents = self->size / 2;
//
//    bool grounded = false;
//    float lastHeight = -999999.f;
//    for (int i = 0; i < LevelBBoxes.size(); i++)
//    {
//        if (LevelBBoxes[i].Intersects(collider))
//        {
//            float groundHeight = LevelBBoxes[i].Center.y + LevelBBoxes[i].Extents.y;
//            if (groundHeight > lastHeight)
//            {
//                lastHeight = groundHeight;
//                if (groundHeight - STEP_HEIGHT < self->position.y)
//                {
//                    self->position.y = groundHeight + self->size.y / 2;
//                    grounded = true;
//                }
//            }
//        }
//    }
//
//    if (!grounded)
//    {
//        self->position.y = self->position.y - 1 * DeltaTime;
//    }
//
//    // ==== çXêVà íu ====
//    self->position = self->position + self->velocity * DeltaTime;
//}