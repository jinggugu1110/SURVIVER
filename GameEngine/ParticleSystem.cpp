#include "pch.h"
#include "ParticleSystem.h"
#include "main.h"
#include "dx_tools.h"

namespace particle_system
{
    void Emit(const XMFLOAT3& emitterPosition, const XMFLOAT3& direction, float lifetime)
    {
        Particle particle;
        particle.position = emitterPosition;
        particle.size = Vector3(random_float(0.03f, 0.05f), random_float(0.03f, 0.05f), random_float(0.03f, 0.05f));
        //particle.size = Vector3(0.3f, 0.3f, 0.3f);
        particle.velocity = direction; // Set the initial velocity
        particle.lifetime = Time + lifetime; // Set the lifetime of the particle (e.g., 1 second)
        Particles.push_back(particle);
    }


    void RenderParticles(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        context->OMSetDepthStencilState(m_states->DepthNone(), 0);
        context->RSSetState(m_states->CullNone());
        for (int i = 0; i < Particles.size(); i++)
        {
            if (Particles[i].lifetime > Time)
            {
                Particles[i].position = Particles[i].position + Particles[i].velocity * 4.f * DeltaTime;
               // Particles[i].size = Particles[i].size * (1 - DeltaTime * 20.f);
                auto lol = GeometricPrimitive::CreateBox(context, Particles[i].size); //THIS WILL CREATE A MEMORY LEAK OVER TIME
                //auto lol = GeometricPrimitive::CreateSphere(context, 0.15f);

                Matrix worldMatrix = Matrix::CreateTranslation(Particles[i].position);
                XMMATRIX rotationMatrix = LookInDirection(Particles[i].position, m_cameraPos, Vector3::Up);
                worldMatrix = Matrix(rotationMatrix) * worldMatrix;
                //lol->Draw(worldMatrix, m_view, m_proj, Colors::White, m_texture.Get());
                //replace color change
                float t = (Particles[i].lifetime - Time); // ™”—]??
                XMVECTOR color = XMVectorSet(1.0f, t, 1.0f, 1.0f); // ‰© ¨ ?
                XMVECTOR bloomColor = XMVectorSet(2.0f, 1.2f, 0.2f, 1.0f);
                lol->Draw(worldMatrix, m_view, m_proj, color, m_texture.Get());


            }
            else Particles.erase(Particles.begin() + i);
        }
    }
    void particle_system::EmitExplosion(const Vector3& pos, int count, float speed, float lifetime)
    {
        
        printf("Emit particle\n"); 
        for (int i = 0; i < count; i++)
        {
            // ‹…–ÊŠ÷•ûŒü
            float theta = XM_2PI * random_float(0.0f, 1.0f);
            float phi = XM_PI * random_float(0.0f, 1.0f);

            Vector3 dir(
                sinf(phi) * cosf(theta),
                cosf(phi),
                sinf(phi) * sinf(theta)
            );

            dir.Normalize();
            dir *= speed * random_float(0.5f, 1.0f);

            Emit(
                XMFLOAT3(pos.x, pos.y, pos.z),
                XMFLOAT3(dir.x, dir.y, dir.z),
                lifetime
            );
        }
    }
}