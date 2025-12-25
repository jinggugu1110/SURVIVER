#pragma once

class SoundManager;
namespace gui
{
	void RenderGUI(ID3D11Device* device, ID3D11DeviceContext* context, SoundManager* sound);
}