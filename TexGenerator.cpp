#include "DXUT.h"
#include "DXUTmisc.h"
#include "TexGenerator.h"
//#include "TextureA1.h"
#include "SDKmisc.h"

VTGenerator::VTGenerator(IDirect3DDevice9* pD3DDevice)
{
	
	pDevice = pD3DDevice;

	Init();

	InitTex();


	for (int i = 0; i < 1024; i++)
	{

		TexPagePool.push_back(i);
	}

}


int VTGenerator::getPageIndex()
{
	

	if ( TexPagePool.size() == 0)
	{
		return -1;
	}
	else
	{
		int texpage = *(TexPagePool.begin());

		TexPagePool.erase(TexPagePool.begin());

		return texpage;
	}
}

void VTGenerator::recycleIndex(int pageindex)
{
	if (pageindex < 0 || pageindex >= 1024)
	{
		//assert(0)
	}
	else
	{
		TexPagePool.push_back(pageindex);
	}
}

void VTGenerator::InitTex()
{
	
	WCHAR str[MAX_PATH];
	DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;
	DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"sparseShadow.fx");
	D3DXCreateEffectFromFile(pDevice, str, NULL, NULL, dwShaderFlags,
		NULL, &pTexEffect, NULL);

	g_hmWorldViewProjection = pTexEffect->GetParameterByName(NULL, "g_mShadowVP");

}


void VTGenerator::shutdown()
{
	
	SAFE_RELEASE(pTexEffect);

	SAFE_RELEASE(TextureCache);

	SAFE_RELEASE(pNewRT);
	SAFE_RELEASE(pNewDS);

	SAFE_RELEASE(pDS);
	
	SAFE_RELEASE(pOrinRT);
	SAFE_RELEASE(pOrinDS);
}

void VTGenerator::DrawFullScreenQuad()
{
	struct QuadVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 uv;
	};

	QuadVertex v[4];

	v[0].pos = D3DXVECTOR3(-512.0f,0.0f, -512.0f);
	v[0].uv = D3DXVECTOR2(0.0f, 1.0f);

	v[1].pos = D3DXVECTOR3(-512.0f, 0.0f, 512.0f);
	v[1].uv = D3DXVECTOR2(0.0f, 0.0f);

	v[2].pos = D3DXVECTOR3(512.0f, 0.0f, -512.0f);
	v[2].uv = D3DXVECTOR2(1.0f, 1.0f);

	v[3].pos = D3DXVECTOR3(512.0f, 0.0f, 512.0f);
	v[3].uv = D3DXVECTOR2(1.0f, 0.0f);


	pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v, sizeof(QuadVertex));

}

void VTGenerator::Init()
{
	pDevice->CreateTexture(4096, 4096, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24X8, D3DPOOL_DEFAULT, &TextureCache, NULL);
	
	pDevice->CreateTexture(128, 128, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24X8, D3DPOOL_DEFAULT, &pDS, NULL);

	pDS->GetSurfaceLevel(0, &pNewDS);

	pDevice->CreateRenderTarget(128, 128, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, false, &pNewRT, NULL);

	pOrinRT = nullptr;
	pOrinDS = nullptr;
}


void VTGenerator::Begin()
{
	if (pOrinRT == nullptr)
	{
		pDevice->GetRenderTarget(0, &pOrinRT);
		pDevice->GetDepthStencilSurface(&pOrinDS);
	}

	pDevice->SetRenderTarget(0, pNewRT);
	pDevice->SetDepthStencilSurface(pNewDS);

	pDevice->Clear(0, NULL,  D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
}

void VTGenerator::updateTexture(int texpage, int textadr)
{

	int levelmask = 0xff000000;
	int level = (textadr&levelmask) >> 24;

	int bias = textadr & 0x00ffffff;

	int xbias = bias % 4096;
	int ybias = bias / 4096;

	float basecellsize = 0.5f;
	float texhalfsize = 256.0f;

	float levelsize = basecellsize * (1 << level);
	float halfesize = levelsize / 2.0f;

	Begin();

	D3DXMATRIX mview;
	D3DXMATRIX mproj;
	D3DXMATRIX mvp;

	D3DXVECTOR3 vat = m_center + m_left*(xbias*levelsize - texhalfsize + halfesize) + m_up*(ybias*levelsize - texhalfsize + halfesize);

	D3DXVECTOR3 veye = vat - m_lightdir * 1000.0f;
	
	D3DXVECTOR3 vup = m_up;

	D3DXMatrixLookAtLH(&mview, &veye, &vat, &vup);

	D3DXMatrixOrthoLH(&mproj, levelsize, levelsize, 1.0f, 5000.0f);

	mvp = mview * mproj;

	pTexEffect->SetMatrix(g_hmWorldViewProjection, &mvp);

	pTexEffect->Begin(nullptr, 0);
	pTexEffect->BeginPass(0);

	DrawFullScreenQuad();

	pTexEffect->EndPass();
	pTexEffect->End();
	//
	End();


	//stretch texture to the cacheTexture
	
	IDirect3DSurface9* psurf;
	TextureCache->GetSurfaceLevel(0,&psurf);

	int biasx = texpage %  32;
	int biasy = texpage / 32;

	RECT rect;
	rect.left	= 0 + biasx*128;
	rect.bottom	= 4096 -  biasy * 128;
	//rect.bottom = 0 + biasy * 128;

	rect.right	= 128 + biasx * 128;
	rect.top = 4096 - 128 - biasy * 128;
	//rect.top = 128 + biasy * 128;
		
	pDevice->StretchRect(pNewDS,NULL, psurf, &rect, D3DTEXF_LINEAR);
	SAFE_RELEASE(psurf);
		
}


IDirect3DTexture9* VTGenerator::getTex()
{
	return TextureCache;
}

void VTGenerator::End()
{

	pDevice->SetRenderTarget(0, pOrinRT);
	pDevice->SetDepthStencilSurface(pOrinDS);

}


