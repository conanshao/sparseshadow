#pragma once

#include <list>

class TextureAl;
class VTGenerator
{
private:
	IDirect3DTexture9* TextureCache;

	IDirect3DTexture9* pRT;

	IDirect3DDevice9* pDevice;

	IDirect3DSurface9* pOrinRT;
	IDirect3DSurface9* pOrinDS;

	IDirect3DSurface9* pNewRT;
	IDirect3DSurface9* pNewDS;

	IDirect3DSurface9* pTestRT;

private:

	std::list<int> TexPagePool;

	void Init();

	void InitPos();

	void InitTex();

	void DrawFullScreenQuad();

	IDirect3DVertexBuffer9* pvb;
	IDirect3DIndexBuffer9* pib;

	ID3DXEffect * pTexEffect;

	D3DXHANDLE                  g_hmShadowViewProj;
	D3DXHANDLE					g_hBias;
	

	D3DXVECTOR3				m_center;
	D3DXVECTOR3				m_lightdir;
	D3DXVECTOR3				m_up;
	D3DXVECTOR3				m_left;

	ID3DXMesh*				mesh;

	D3DXVECTOR3* PosArray;

	int cullArray(D3DXVECTOR3 center, float halfsize);

public:

	int getPageIndex();
	void recycleIndex(int pageindex);

	IDirect3DTexture9 * getTex();

	VTGenerator(IDirect3DDevice9* pD3DDevice);

	void Begin();

	void updateTexture(int texpage, int texadr);

	void End();

	void shutdown();

};
