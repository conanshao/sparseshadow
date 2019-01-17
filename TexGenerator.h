#pragma once

#include <list>

class TextureAl;
class VTGenerator
{
private:
	IDirect3DTexture9* TextureCache;

	IDirect3DTexture9* pDS;

	IDirect3DDevice9* pDevice;

	IDirect3DSurface9* pOrinRT;
	IDirect3DSurface9* pOrinDS;

	IDirect3DSurface9* pNewRT;
	IDirect3DSurface9* pNewDS;

private:

	std::list<int> TexPagePool;

	void Init();


	void InitTex();

	void DrawFullScreenQuad();

	IDirect3DVertexBuffer9* pvb;
	IDirect3DIndexBuffer9* pib;

	ID3DXEffect * pTexEffect;

	D3DXHANDLE                  g_hmWorldViewProjection;
	D3DXHANDLE					g_hTex;

	D3DXVECTOR3				m_center;
	D3DXVECTOR3				m_lightdir;
	D3DXVECTOR3				m_up;
	D3DXVECTOR3				m_left;



public:

	void beginRender(D3DXMATRIX mat, IDirect3DTexture9* pterraintex);
	void endRender();

	int getPageIndex();
	void recycleIndex(int pageindex);

	IDirect3DTexture9 * getTex();

	VTGenerator(IDirect3DDevice9* pD3DDevice);

	void Begin();

	void Begin(IDirect3DTexture9* ptex);

	void updateTexture(int texpage, int texadr);

	void TestupdateTexture(int textadr,IDirect3DTexture9* ptex);

	void End();

	void saveTexture();

	void shutdown();

};
