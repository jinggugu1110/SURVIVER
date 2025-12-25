#include "pch.h"
#include "SoundManager.h"

using namespace DirectX;

void SoundManager::Initialize()
{
    m_audio = std::make_unique<AudioEngine>();
    //title
    m_titleBgm = std::make_unique<DirectX::SoundEffect>(
        m_audio.get(),
        L"data/sound/title_bgm.wav"
    );
	//click
    m_click = std::make_unique<DirectX::SoundEffect>(
        m_audio.get(),
        L"data/sound/click.wav"
    );

    m_shot = std::make_unique<SoundEffect>(m_audio.get(), L"data/sound/shot.wav");
    m_hit = std::make_unique<SoundEffect>(m_audio.get(), L"data/sound/hit.wav");
    //m_explosion = std::make_unique<SoundEffect>(m_audio.get(), L"data/sound/explosion.wav");
}

void SoundManager::Update()
{
    if (m_audio)
        m_audio->Update();
}

void SoundManager::PlayTitleBGM()
{
    if (!m_bgmInstance)
    {
        m_bgmInstance = m_titleBgm->CreateInstance();
        m_bgmInstance->Play(true); // true = loop
    }
}

void SoundManager::StopBGM()
{
    if (m_bgmInstance)
    {
        m_bgmInstance->Stop();
        m_bgmInstance.reset();
    }
}
void SoundManager::PlayClick()
{
    if (m_click)
        m_click->Play();
}


void SoundManager::PlayShot()
{
    if (m_shot) m_shot->Play();
}

void SoundManager::PlayHit()
{
    if (m_hit) m_hit->Play();
}

void SoundManager::PlayExplosion()
{
    if (m_explosion) m_explosion->Play();
}
