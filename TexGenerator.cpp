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

	InitPos();


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

	g_hmShadowViewProj = pTexEffect->GetParameterByName(NULL, "g_mShadowVP");
	g_hBias = pTexEffect->GetParameterByName(NULL, "biaspos");
}


void VTGenerator::shutdown()
{
	
	SAFE_RELEASE(pTexEffect);

	SAFE_RELEASE(TextureCache);

	SAFE_RELEASE(pNewRT);
	SAFE_RELEASE(pNewDS);

	SAFE_RELEASE(pRT);
	
	SAFE_RELEASE(pOrinRT);
	SAFE_RELEASE(pOrinDS);
}



void VTGenerator::Init()
{
	pDevice->CreateTexture(4096, 4096, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &TextureCache, NULL);
	
	pDevice->CreateTexture(128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);

	pRT->GetSurfaceLevel(0, &pNewRT);

	pDevice->CreateDepthStencilSurface(128, 128, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, true, &pNewDS, NULL);

	D3DXCreateTeapot(pDevice, &mesh, NULL);

	mesh->GetVertexBuffer(&vb);
	mesh->GetIndexBuffer(&ib);
	
	const D3DVERTEXELEMENT9 g_VBDecl_Geometry[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0},
		{1, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
		D3DDECL_END()
	};




	pDevice->CreateVertexBuffer( sizeof(D3DXVECTOR3) * 64 * 64,  0,  0, D3DPOOL_MANAGED, &instancevb, NULL);

	D3DXVECTOR3* pverts;
	instancevb->Lock(0, 0, (void**)&pverts, 0);
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
		{
			pverts[i + j * 64] = D3DXVECTOR3((i - 32)*8.0f, 1.0f, (j - 32)*8.0f);
		}

	instancevb->Unlock();

	pDevice->CreateVertexDeclaration(g_VBDecl_Geometry, &pvdcl);


	stride = mesh->GetNumBytesPerVertex();

	vertexcount = mesh->GetNumVertices();
	
	facecount = mesh->GetNumFaces();
	
	//D3DXCreateBox(pDevice, 2.0f, 2.0f, 2.0f, &mesh, NULL);
	pOrinRT = nullptr;
	pOrinDS = nullptr;

}

void VTGenerator::InitPos()
{

	m_center = D3DXVECTOR3(0.0f, 0.f, 0.0f);

	m_lightdir = D3DXVECTOR3(1.0f, 1.0f, 0.0f);

	D3DXVec3Normalize(&m_lightdir, &m_lightdir);
	
	m_up = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	
	D3DXVec3Cross(&m_left, &m_lightdir, &m_up);

	D3DXVec3Normalize(&m_left, &m_left);

	D3DXVec3Cross(&m_up, &m_left, &m_lightdir);

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

	pDevice->Clear(0, NULL,  D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 255, 255, 255), 1.0f, 0);

}

int VTGenerator::cullArray(D3DXVECTOR3 center, float halfsize)
{
	return -1;
}

void VTGenerator::updateTexture(int texpage, int textadr)
{
	int levelmask = 0xff000000;
	int level = (textadr&levelmask) >> 24;

	int bias = textadr & 0x00ffffff;

	int xbias = bias % 4096;
	int ybias = bias / 4096;

	float basecellsize = 0.5f;
	float texhalfsize = basecellsize * 512.0f;

	float levelsize = basecellsize * (1 << level);
	float halfesize = levelsize / 2.0f;

	Begin();

	D3DXMATRIX mview;
	D3DXMATRIX mproj;
	D3DXMATRIX mvp;

	D3DXVECTOR3 vat = m_center + m_left*(xbias*levelsize - texhalfsize + halfesize) + m_up*(ybias*levelsize - texhalfsize + halfesize);

	D3DXVECTOR3 veye = vat + m_lightdir * 1000.0f;
	
	D3DXVECTOR3 vup = m_up;

	D3DXMatrixLookAtLH(&mview, &veye, &vat, &vup);

	float realsize = levelsize * 128.0f / 126.0f;

	D3DXMatrixOrthoLH(&mproj, levelsize, levelsize, 1.0f, 5000.0f);

	mvp = mview * mproj;

	pTexEffect->SetMatrix(g_hmShadowViewProj, &mvp);

	pTexEffect->Begin(nullptr, 0);
	pTexEffect->BeginPass(5);

	pDevice->SetVertexDeclaration(pvdcl);
	pDevice->SetStreamSource(0, vb, 0, stride);
	pDevice->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | 64 * 64));

	pDevice->SetStreamSource(1, instancevb, 0, sizeof(D3DXVECTOR3));
	pDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul);

	pDevice->SetIndices(ib);
	
	pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertexcount, 0, facecount);
	

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
	
	rect.right	= 128 + biasx * 128;
	rect.top = 4096 - 128 - biasy * 128;
		
	pDevice->StretchRect(pNewRT,NULL, psurf, &rect, D3DTEXF_NONE);
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


