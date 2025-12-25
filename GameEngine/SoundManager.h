#pragma once
#include <Audio.h>
#include <memory>


class SoundManager
{
public:
    void Initialize();
    void Update();
    void PlayTitleBGM();
    void StopBGM();

    void PlayClick();
    void PlayShot();
    void PlayHit();
    void PlayExplosion();

private:
    std::unique_ptr<DirectX::AudioEngine> m_audio;
    std::unique_ptr<DirectX::SoundEffect> m_titleBgm;
    std::unique_ptr<DirectX::SoundEffectInstance> m_bgmInstance;
    std::unique_ptr<DirectX::SoundEffect> m_click;
    std::unique_ptr<DirectX::SoundEffect> m_shot;
    std::unique_ptr<DirectX::SoundEffect> m_hit;
    std::unique_ptr<DirectX::SoundEffect> m_explosion;
};

