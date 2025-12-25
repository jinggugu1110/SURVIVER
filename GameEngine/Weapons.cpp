#include "pch.h"
#include "Weapons.h"
#include "main.h"
#include "LevelManager.h"
#include "dx_tools.h"
#include "InputManager.h"
#include "EntityList.h"
#include "PlayerController.h"
#include "ParticleSystem.h"
#include "SoundManager.h"


namespace weapon
{
	void AddBulletHole(Vector3 pos, Vector3 size)
	{
		ENTITY ent;
		ent.name = "bullet_hole";
		ent.position = pos;
		ent.size = size;
		ent.model = "data\\textures\\decals\\hole.dds";
		ent.sprite = false;
		ent.next_think = Time + 35.f;
		ent.think = SUB_Remove;

		EntityList.push_back(ent);
	}

	void AddRecoil(Vector3 velocity, float pitch, float yaw)
	{
		player::PlayerVelocity = player::PlayerVelocity + velocity;
		RecoilPitch = pitch;
		RecoilYaw = yaw;
	}

	void FireBullet(Vector3 from, Vector3 dir)
	{
		
		float entMinDist = 9999.f;
		float minDist = 9999.f;
		Vector3 hit = Vector3::Zero;
		int EntID = -1;

		for (int i = 0; i < EntityList.size(); i++)
		{
			float shotDist;
			if (EntityList[i].bbox.Intersects(m_cameraPos, dir, shotDist))
			{
				if (entMinDist > shotDist)
				{
					entMinDist = shotDist;
					hit = m_cameraPos + dir * entMinDist;
					EntID = i;
				}
			}
		}

		for (int i = 0; i < LevelBBoxes.size(); i++)
		{
			float shotDist;
			if (LevelBBoxes[i].Intersects(m_cameraPos, dir, shotDist))
			{
				if (minDist > shotDist)
				{
					minDist = shotDist;
					hit = m_cameraPos + dir * minDist;
				}
			}
		}

		if (hit != Vector3::Zero)
		{
			//if (entMinDist < minDist) //we hit entity
			if (entMinDist < minDist && EntID != -1)
			{
				//if (EntID != -1)
				//{
				//	EntityList[EntID].think = SUB_Remove;
				//	EntityList[EntID].next_think = 0;
				//}
				if (g_sound)
					g_sound->PlayHit();
				ENTITY& ent = EntityList[EntID];
				ent.health -= 1;
				for (int j = 0; j < random_int(3, 10); j++)
				{
					particle_system::Emit(hit, Vector3(random_float(-1.f, 1.f), random_float(-1.f, 1.f), random_float(-1.f, 1.f)), 1.5f);
				}
				//if (ent.health <= 0)
				//{
				//	if (ent.name.find("Boss") != std::string::npos)
				//	{
				//		GameState = GAME_CLEAR;   // š Ÿà??—˜
				//	}
				//
				//	ent.think = SUB_Remove;
				//	ent.next_think = 0;
				//}
				if (EntityList[EntID].health <= 0)
				{
					if (EntityList[EntID].name == "Boss")
					{
						ExplodeAllMonsters();
						GameState = GAME_CLEAR;
					}

					EntityList[EntID].think = SUB_Remove;
					EntityList[EntID].next_think = 0;
				}
			}
			else //we hit wall =(
			{
				Vector3 size = Vector3(0.05f, 0.05f, 0.05f);
				hit = hit + dir * size.x * 0.4f;
				AddBulletHole(hit, size);
				for (int j = 0; j < random_int(3, 10); j++)
				{
					particle_system::Emit(hit, Vector3(random_float(-1.f, 1.f), random_float(-1.f, 1.f), random_float(-1.f, 1.f)), 1.5f);
				}
			}
		}
	}

	void Attack()
	{
		if (!input::Attack)
		{
			AttackHeld = false;
			return;
		}
		if (AttackHeld) return;
		AttackHeld = true;
		
		Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, -m_pitch, 0.f);
		DirectX::XMVECTOR rotationQuaternion = DirectX::XMLoadFloat4(&q);
		DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		forward = DirectX::XMVector3Rotate(forward, rotationQuaternion);
		Vector3 cameraForward = forward;
		cameraForward.Normalize();

		for (int j = 0; j < 6; j++)
		{
			cameraForward = forward;
			cameraForward = cameraForward + Vector3(random_float(-0.05f, 0.05f), random_float(-0.05f, 0.05f), random_float(-0.05f, 0.05f));
			cameraForward.Normalize();
			FireBullet(m_cameraPos, cameraForward);
			if (g_sound)
				g_sound->PlayShot();
		}

		AddRecoil(-cameraForward * 0.00001f, random_float(0.5f, 2.f), random_float(-1.f, 1.f));
	}


	void ApplyRecoil()
	{
		RecoilPitch = Lerp(RecoilPitch, 0.f, RECOIL_DECREASE_SPEED * DeltaTime);
		RecoilYaw = Lerp(RecoilYaw, 0.f, RECOIL_DECREASE_SPEED * DeltaTime);

		m_pitch = m_pitch + RecoilPitch * DeltaTime;
		m_yaw = m_yaw + RecoilYaw * DeltaTime;
	}

	void WeaponFrame()
	{
		Attack();
		ApplyRecoil();
		
	}

}