//--------------------------------------------------------------------------------------
// File: sparseShadow.fx
//
// The effect file for the sparseShadow sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

texture g_ShadowTexture;              // Color texture for mesh

float    g_fTime;                   // App's time in seconds

float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
float4x4 g_mShadowVP;
float4x4 g_mShadowM;

float3 biaspos;

//--------------------------------------------------------------------------------------
// Texture samplers
//--------------------------------------------------------------------------------------
sampler ShadowSampler = 
sampler_state
{
    Texture = <g_ShadowTexture>;
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
	addressu = border;
	addressv = border;
	bordercolor = 0xffffffff;
	
};




//--------------------------------------------------------------------------------------
// Vertex shader output structure
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 TextureUV  : TEXCOORD0;  // vertex texture coords 
};


//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT RenderSceneVS( float4 vPos : POSITION, 
                         float3 vNormal : NORMAL,
                         float2 vTexCoord0 : TEXCOORD0 )
{
    VS_OUTPUT Output;    
    
    // Transform the position from object space to homogeneous projection space
    Output.Position = mul(vPos + float4(biaspos, 0.0f), g_mShadowVP);
    // Just copy the texture coordinate through
	Output.TextureUV = mul(vPos, g_mShadowM);
    
    return Output;    
}


//--------------------------------------------------------------------------------------
// Pixel shader output structure
//--------------------------------------------------------------------------------------
struct PS_OUTPUT
{
    float4 RGBColor : COLOR0;  // Pixel color    
};


//--------------------------------------------------------------------------------------
// This shader outputs the pixel's color by modulating the texture's
// color with diffuse material color
//--------------------------------------------------------------------------------------
PS_OUTPUT RenderDepthPS( VS_OUTPUT In ) 
{ 
    PS_OUTPUT Output;

    Output.RGBColor = 0.0f;
    return Output;
}


VS_OUTPUT RenderShadowVS(float4 vPos : POSITION,
	float3 vNormal : NORMAL,
	float2 vTexCoord0 : TEXCOORD0)
{
	VS_OUTPUT Output;

	// Transform the position from object space to homogeneous projection space
	Output.Position = mul(vPos, g_mWorldViewProjection);
	// Just copy the texture coordinate through
	Output.TextureUV = mul(vPos, g_mShadowM);

	return Output;
}


PS_OUTPUT RenderShadowPS(VS_OUTPUT In)
{
	PS_OUTPUT Output;
	float shadow = tex2Dproj(ShadowSampler, In.TextureUV).r;
	Output.RGBColor = shadow * 0.7f + 0.3f;
	return Output;
}

//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {          
		colorwriteenable = 0x00;

        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader  = compile ps_3_0 RenderDepthPS();
    }

	pass P1
	{
		colorwriteenable = 0x0f;

		VertexShader = compile vs_3_0 RenderShadowVS();
		PixelShader = compile ps_3_0 RenderShadowPS();
	}
}
