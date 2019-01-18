//--------------------------------------------------------------------------------------
// File: sparseShadow.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"
#include "TexGenerator.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 

#pragma comment(lib, "legacy_stdio_definitions.lib")

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CFirstPersonCamera          g_Camera;               // A model viewing camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
ID3DXFont*                  g_pFont9 = NULL;
ID3DXSprite*                g_pSprite9 = NULL;
ID3DXEffect*                g_pEffect9 = NULL;
D3DXHANDLE                  g_hmWorldViewProjection;
D3DXHANDLE					g_hmShadowViewProj;
D3DXHANDLE					g_hmShadow;
D3DXHANDLE					g_hShadowTex;
D3DXHANDLE				    g_hBias;

IDirect3DTexture9* newRT = NULL;
IDirect3DTexture9* sysRT = NULL;
VTGenerator* vtgen = NULL;
ID3DXMesh* mesh = NULL;
IDirect3DTexture9* projMap = NULL;




//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext );
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnD3D9LostDevice( void* pUserContext );
void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

void InitApp();
void RenderText();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"sparseShadow" );
    DXUTCreateDevice( true, 1920, 1080 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                      D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    // No fallback defined by this app, so reject any device that 
    // doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}

void DrawQuad(IDirect3DDevice9* pDevice)
{
	struct QuadVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 uv;
	};

	float quadsize = 128.0f;

	QuadVertex v[4];

	v[0].pos = D3DXVECTOR3(-quadsize, 0.0f, -quadsize);
	v[0].uv = D3DXVECTOR2(0.0f, 1.0f);

	v[1].pos = D3DXVECTOR3(-quadsize, 0.0f, quadsize);
	v[1].uv = D3DXVECTOR2(0.0f, 0.0f);

	v[2].pos = D3DXVECTOR3(quadsize, 0.0f, -quadsize);
	v[2].uv = D3DXVECTOR2(1.0f, 1.0f);

	v[3].pos = D3DXVECTOR3(quadsize, 0.0f, quadsize);
	v[3].uv = D3DXVECTOR2(1.0f, 0.0f);


	pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (void*)v, sizeof(QuadVertex));
}

D3DXVECTOR3 PosArray[64 * 64];


uint32_t * indirectTexData[11];
IDirect3DTexture9* pFeedBackTex;
IDirect3DTexture9* pIndirectMap;
IDirect3DTexture9* pTestTex;

IDirect3DTexture9* indirectMap;

IDirect3DSurface9* pIndirectRT[11];

//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );

    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pFont9 ) );

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;

#ifdef DEBUG_VS
    dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
#endif
#ifdef DEBUG_PS
    dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
#endif
#ifdef D3DXFX_LARGEADDRESS_HANDLE
    dwShaderFlags |= D3DXFX_LARGEADDRESSAWARE;
#endif

    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"sparseShadow.fx" ) );
	ID3DXBuffer* buffer;
    D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags,
                                        NULL, &g_pEffect9, &buffer) ;

	if (buffer)
	{
		char* errorbuf = (char*)buffer->GetBufferPointer();
		OutputDebugStringA(errorbuf);
	}

    g_hmWorldViewProjection = g_pEffect9->GetParameterByName( NULL, "g_mWorldViewProjection" );
	g_hmShadowViewProj = g_pEffect9->GetParameterByName(NULL, "g_mShadowVP");
	g_hmShadow = g_pEffect9->GetParameterByName(NULL, "g_mShadowM");

	g_hShadowTex = g_pEffect9->GetParameterByName(NULL, "g_ShadowTexture");

	g_hBias = g_pEffect9->GetParameterByName(NULL, "biaspos");
    
    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 0.0f, 2.0f, -5.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
	g_Camera.SetScalers(0.01f, 50.0f);

	D3DXCreateTeapot(pd3dDevice, &mesh, NULL);

	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
		{
			PosArray[i + j*64] = D3DXVECTOR3((i - 32)*8.0f, 1.0f, (j - 32)*8.0f);
		}

	vtgen = new VTGenerator(pd3dDevice);


	pd3dDevice->CreateTexture(1024, 1024, 0, 0, D3DFMT_A2R10G10B10, D3DPOOL_MANAGED, &pFeedBackTex, NULL);

	pd3dDevice->CreateTexture(1024, 1024, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pIndirectMap, NULL);

	int levelcount = pFeedBackTex->GetLevelCount();


	for (uint64_t level = 0; level < 11; level++)
	{
		D3DLOCKED_RECT rect;

		pFeedBackTex->LockRect(level, &rect, NULL, 0);

		uint32_t* ptexdata = (uint32_t*)rect.pBits;

		int mipsize = 1024 >> level;
		for (uint32_t j = 0; j < mipsize; j++)
			for (uint32_t i = 0; i < mipsize; i++)
			{
				ptexdata[i + j * mipsize] = (level << 22) | (i << 12) | (j << 2);
			}

		pFeedBackTex->UnlockRect(level);

	}

	D3DXHANDLE hindirect = g_pEffect9->GetParameterByName(NULL, "indirectTex");
	g_pEffect9->SetTexture(hindirect, pFeedBackTex);

	for (int i = 0; i < 11; i++)
	{

		int texwidth = 1024 >> i;

		indirectTexData[i] = new uint32_t[texwidth*texwidth];

		memset(indirectTexData[i], 0xffffffff, sizeof(uint32_t)*texwidth*texwidth);

	}

    return S_OK;
}

IDirect3DSurface9* gRT = NULL;
IDirect3DSurface9* gDS = NULL;
int scale = 8;
//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice,
                                    const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont9 ) V_RETURN( g_pFont9->OnResetDevice() );
    if( g_pEffect9 ) V_RETURN( g_pEffect9->OnResetDevice() );

    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite9 ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont9, g_pSprite9, NULL, NULL, 15 );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f );
    //g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350 );
    g_SampleUI.SetSize( 170, 300 );


	int depthsize = 4096;

	pd3dDevice->CreateTexture(depthsize, depthsize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &projMap, NULL);

	projMap->GetSurfaceLevel(0, &gRT);

	g_pEffect9->SetTexture(g_hShadowTex, projMap);
	
	pd3dDevice->CreateDepthStencilSurface(depthsize, depthsize, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, true,&gDS, NULL);


	D3DXCreateTexture(pd3dDevice, pBackBufferSurfaceDesc->Width / scale, pBackBufferSurfaceDesc->Height / scale, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2R10G10B10, D3DPOOL_DEFAULT, &newRT);
	
	D3DXCreateTexture(pd3dDevice, 1024, 1024, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &indirectMap);

	for (int level = 0; level < 11; level++)
	{
		indirectMap->GetSurfaceLevel(level, &pIndirectRT[level]);
	}


	D3DXCreateTexture(pd3dDevice, pBackBufferSurfaceDesc->Width / scale, pBackBufferSurfaceDesc->Height / scale, 1, 0, D3DFMT_A2R10G10B10, D3DPOOL_SYSTEMMEM, &sysRT);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}

struct Datastr
{
	int visit;
	int xbias;
	int ybias;
	int level;

};

Datastr pageindexVisit[1024];


struct PointVertex
{
	D3DXVECTOR3 pos;
	DWORD color;
};

PointVertex PointArray[10][512];
int vertexnum[10];

void updateIndirTex(IDirect3DDevice9* pdevice)
{
	pdevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
	for (int level = 9; level >= 0; level--)
	{
		if (vertexnum[level] > 0)
		{
			pdevice->SetRenderTarget(0, pIndirectRT[level]);
			pdevice->DrawPrimitiveUP(D3DPT_POINTLIST, vertexnum[level], PointArray[level], sizeof(PointVertex));
		}
	}
}

void InitProcessTex()
{
	static bool init = false;

	if (init == false)
	{
		for (int i = 0; i < 1024; i++)
		{
			pageindexVisit[i].visit = -1;
		}

		int pageindex = vtgen->getPageIndex();

		int xpage = pageindex % 32;
		int ypage = pageindex / 32;
		int compindex = 10 << 16 | (xpage << 8) | ypage;

		pageindexVisit[0].level = 10;
		pageindexVisit[0].xbias = 0;
		pageindexVisit[0].ybias = 0;
		pageindexVisit[0].visit = 0;

		int texadr = (10 << 24) | (0 + 0 * 4096);

		vtgen->updateTexture(pageindex, texadr);

		indirectTexData[10][0] = compindex;

		init = true;
	}

}


IDirect3DSurface9* psysSurf = NULL;
void ProcessFeedback(IDirect3DDevice9* pDevice)
{

	IDirect3DSurface9* pRT;
	IDirect3DSurface9* pOldRT;

	newRT->GetSurfaceLevel(0, &pRT);
	pDevice->GetRenderTarget(0, &pOldRT);

	pDevice->SetRenderTarget(0, pRT);
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 0);

	g_pEffect9->BeginPass(2);
	DrawQuad(pDevice);
	g_pEffect9->EndPass();

	pDevice->SetRenderTarget(0, pOldRT);

	if (psysSurf == nullptr)
	{
		sysRT->GetSurfaceLevel(0, &psysSurf);
	}

	for (int i = 0; i < 10; i++)
	{
		vertexnum[i] = 0;
	}

	pDevice->GetRenderTargetData(pRT, psysSurf);

	InitProcessTex();

	D3DSURFACE_DESC desc;
	psysSurf->GetDesc(&desc);

	D3DLOCKED_RECT rect;
	psysSurf->LockRect(&rect, NULL, D3DLOCK_READONLY);
	uint32_t* pfeedbackdata = (uint32_t *)rect.pBits;

	for (int i = 1; i < 1024; i++)
	{
		if (pageindexVisit[i].visit != -1)
		{
			pageindexVisit[i].visit++;
		}

		if (pageindexVisit[i].visit >= 3)
		{
			vtgen->recycleIndex(i);
			pageindexVisit[i].visit = -1;
			int level = pageindexVisit[i].level;
			int xbias = pageindexVisit[i].xbias;
			int ybias = pageindexVisit[i].ybias;

			int texsize = 1024 >> level;
			indirectTexData[level][xbias + ybias * texsize] = 0xffffffff;

			int& levelindex = vertexnum[level];
			PointArray[level][levelindex].pos = D3DXVECTOR3(xbias, ybias, level);
			PointArray[level][levelindex++].color = 0;
		}
	}

	int countindex = 0;
	for (int i = 0; i < desc.Width*desc.Height; i++)
	{
		if (pfeedbackdata[i] != 0xffffffff)
		{
			int level = pfeedbackdata[i] >> 22;
			int xbias = (pfeedbackdata[i] & 0x003ff000) >> 12;
			int ybias = (pfeedbackdata[i] & 0x00000ffc) >> 2;
			int texadr = (level << 24) | (xbias + ybias * 4096);

			int texsize = 1024 >> level;

			uint32_t& indirectData = indirectTexData[level][xbias + ybias * texsize];
			uint32_t oldlevel = indirectData >> 16;
			if (oldlevel != level)
			{
				int pageindex = vtgen->getPageIndex();

				if (pageindex == -1)
				{
					assert(0);
				}

				int xpage = pageindex % 32;
				int ypage = pageindex / 32;

				int compindex = level << 16 | (xpage << 8) | ypage;
				indirectData = compindex;

				pageindexVisit[pageindex].visit = 0;
				pageindexVisit[pageindex].level = level;
				pageindexVisit[pageindex].xbias = xbias;
				pageindexVisit[pageindex].ybias = ybias;

				vtgen->updateTexture(pageindex, texadr);

				int& levelindex = vertexnum[level];
				PointArray[level][levelindex].pos = D3DXVECTOR3(xbias, ybias, level);
				PointArray[level][levelindex++].color = compindex;
				countindex++;
			}
			else
			{
				int pageindex = ((indirectData & 0x0000ff00) >> 8) | (indirectData & 0x000000ff) << 5;
				pageindexVisit[pageindex].visit = 0;

			}
		}
	}

	psysSurf->UnlockRect();

	g_pEffect9->BeginPass(4);
	updateIndirTex(pDevice);
	g_pEffect9->EndPass();

	
	pDevice->SetRenderTarget(0, pOldRT);
	SAFE_RELEASE(pRT);
	SAFE_RELEASE(pOldRT);

}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and the zbuffer 
    

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
        mWorld = *g_Camera.GetWorldMatrix();
        mProj = *g_Camera.GetProjMatrix();
        mView = *g_Camera.GetViewMatrix();

        mWorldViewProjection =   mView * mProj;

        // Update the effect's variables.  Instead of using strings, it would 
        // be more efficient to cache a handle to the parameter by calling 
        // ID3DXEffect::GetParameterByName
		float size = 1024.0f;

		D3DXVECTOR3 eye = D3DXVECTOR3(size, 1.0f + size, size);
		D3DXVECTOR3 at = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
		D3DXVECTOR3 up = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

		D3DXMATRIX shadowview;
		D3DXMATRIX shadowproj;

		D3DXMATRIX shadowviewproj;
		
		D3DXMatrixLookAtLH(&shadowview, &eye, &at, &up);
		D3DXMatrixOrthoLH(&shadowproj, size, size, 1.0f, 5000.0f);
		shadowviewproj = shadowview * shadowproj;

		D3DXMATRIX shadowm;
		D3DXMatrixIdentity(&shadowm);
		shadowm._11 = 0.5f;
		shadowm._22 = -0.5f;
		shadowm._41 = 0.5f;
		shadowm._42 = 0.5f;

		shadowm = shadowviewproj *shadowm;

		g_pEffect9->SetMatrix(g_hmWorldViewProjection, &mWorldViewProjection );
		g_pEffect9->SetMatrix(g_hmShadowViewProj, &shadowviewproj );
		g_pEffect9->SetMatrix(g_hmShadow, &shadowm);

		IDirect3DSurface9* pOldRT;
		IDirect3DSurface9* pOldDS;
		pd3dDevice->GetRenderTarget(0, &pOldRT);
		pd3dDevice->GetDepthStencilSurface(&pOldDS);
		IDirect3DSurface9* pDS;
		projMap->GetSurfaceLevel(0, &pDS);

#define TEST 1
#if TEST
		g_pEffect9->Begin(NULL, 0);
		ProcessFeedback(pd3dDevice);

		pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0);
		//update indirect tex

		D3DXHANDLE hheight = g_pEffect9->GetParameterByName(NULL, "indirectMap");
		g_pEffect9->SetTexture(hheight, indirectMap);

		D3DXHANDLE hcache = g_pEffect9->GetParameterByName(NULL, "CacheTexture");
		g_pEffect9->SetTexture(hcache, vtgen->getTex());

		g_pEffect9->BeginPass(3);
		DrawQuad(pd3dDevice);
		g_pEffect9->EndPass();

		g_pEffect9->End();

		
#else		
		g_pEffect9->Begin(NULL, 0);
		pd3dDevice->SetRenderTarget(0, gRT);
		pd3dDevice->SetDepthStencilSurface(gDS);
		

		
		pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 255, 255, 255),1.0f, 0);

		g_pEffect9->BeginPass(0);

		for(int i=0;i<64*64;i++)
		{
			D3DXVECTOR3 pos = PosArray[i];
			g_pEffect9->SetRawValue(g_hBias, &pos, 0, sizeof(D3DXVECTOR3));
			g_pEffect9->CommitChanges();
			mesh->DrawSubset(0);
		}

		g_pEffect9->EndPass();


		pd3dDevice->SetRenderTarget(0, pOldRT);
		pd3dDevice->SetDepthStencilSurface(pOldDS);
		pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0);
		g_pEffect9->BeginPass(1);

		
		DrawQuad(pd3dDevice);
		g_pEffect9->EndPass();

		g_pEffect9->End();
		
#endif
		SAFE_RELEASE(pOldRT);
		SAFE_RELEASE(pOldDS);
	

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" ); // These events are to help PIX identify what the code is doing
        RenderText();
        V( g_HUD.OnRender( fElapsedTime ) );
        V( g_SampleUI.OnRender( fElapsedTime ) );
        DXUT_EndPerfEvent();

        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont9 ) g_pFont9->OnLostDevice();
    if( g_pEffect9 ) g_pEffect9->OnLostDevice();
    SAFE_RELEASE( g_pSprite9 );
    SAFE_DELETE( g_pTxtHelper );

	SAFE_RELEASE( projMap );
	SAFE_RELEASE( gRT );
	SAFE_RELEASE( gDS );

}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
    SAFE_RELEASE( g_pEffect9 );
    SAFE_RELEASE( g_pFont9 );


	SAFE_RELEASE(mesh);
}


