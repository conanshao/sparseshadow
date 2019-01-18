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
    MinFilter = LINEAR;
    MagFilter = LINEAR;
	addressu = border;
	addressv = border;
	bordercolor = 0xffffffff;
	
};


texture indirectTex;
sampler IndirectSampler =
sampler_state
{
	Texture = <indirectTex>;
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	MIPMAPLODBIAS = 4;
};


texture indirectMap;
sampler IndirectBiasMapSampler =
sampler_state
{
	Texture = <indirectMap>;
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	MIPMAPLODBIAS = 7;
};

texture CacheTexture;
sampler CacheTextureSampler =
sampler_state
{
	Texture = <CacheTexture>;
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	addressu = border;
	addressv = border;
	bordercolor = 0xffffffff;

};

struct VS_OUTPUTNormal
{
	float4 Position   : POSITION;   // vertex position 
	float2 TextureUV  : TEXCOORD0;  // vertex texture coords 

};

struct VS_COLOR
{
	float4 Position   : POSITION;   // vertex position 
	float4 Color  : COLOR0;  // vertex texture coords 
	float psize : PSIZE;
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
	Output.TextureUV = mul(vPos, g_mShadowVP);
    
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

    Output.RGBColor = 0.3f;
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

	float2 UV = In.TextureUV.xy / In.TextureUV.w;
	float shadow = tex2D(ShadowSampler, UV).r;
	Output.RGBColor = shadow * 0.7f + 0.3f;
	return Output;
}



float mip_map_level(float2 texture_coordinate) // in texel units
{
	float2  dx_vtc = ddx(texture_coordinate);
	float2  dy_vtc = ddy(texture_coordinate);
	float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));
	float mml = 0.5 * log2(delta_max_sqr) + 0.5f;
	return max(0, mml);
}

float4 colorlevel[11] =
{
	float4(1.0f,1.0f,1.0f,1.0f),
	float4(0.0f, 0.0f, 1.0f, 1.0f),
	float4(0.0f, 1.0f, 0.0f, 1.0f),
	float4(1.0f, 0.0f, 0.0f, 1.0f),

	float4(0.5f, 0.5f,0.5f,0.5f),
	float4(0.0f, 0.0f, 0.5f, 0.5f),
	float4(0.0f, 0.5f, 0.0f, 0.5f),
	float4(0.5f, 0.0f, 0.0f, 0.5f),

	float4(1.0f,1.0f,1.0f,1.0f),
	float4(0.0f, 0.0f, 1.0f, 1.0f),
	float4(0.0f, 1.0f, 0.0f, 1.0f)
};



struct PS_OUTPUT2
{
	float4 RGBColor : COLOR0;  // Pixel color    

};

PS_OUTPUT2 RenderFeedbackPS(VS_OUTPUT In)
{
	PS_OUTPUT2 Output;

	float2 UV = In.TextureUV.xy / In.TextureUV.w;

	Output.RGBColor = tex2D(IndirectSampler, UV);

	return Output;
}


PS_OUTPUT RenderFinalPS(VS_OUTPUT In)
{
	PS_OUTPUT Output;

	float w = In.TextureUV.w;

	float2 UV = In.TextureUV.xy / In.TextureUV.w;

	float4 color = tex2D(IndirectBiasMapSampler, UV);

	float level = floor(color.r*255.0f);

	float size = 1024.0f / pow(2.0f, level);

	float2 uvbias = floor(float2(color.g*255.0f, color.b*255.0f));

	uvbias *= 1.0f / 32.0f;

	float2 uv = frac(In.TextureUV * size) / 32.0f;

	float2 finaluv = uvbias + uv;
	
	finaluv.y = 1.0f - finaluv.y;


	//Output.RGBColor = tex2Dproj(CacheTextureSampler, float4(finaluv*w, In.TextureUV.zw) );

	Output.RGBColor = tex2D(CacheTextureSampler, finaluv);

	return Output;
}


VS_COLOR RenderPointVS(float4 vPos : POSITION,
	float4 vColor : COLOR0)
{

	VS_COLOR Output;

	// Transform the position from object space to homogeneous projection space
	float texsize = 1024.0f / pow(2.0f, vPos.z);
	float posx = (vPos.x + 0.5f) / texsize;
	float posy = (vPos.y + 0.5f) / texsize;
	Output.Position = float4(2.0f*posx - 1.0f, 1.0f - 2.0f*posy, 0.5f, 1.0f);
	Output.Color = vColor;
	Output.psize = 1.0f;

	return Output;

}


PS_OUTPUT RenderPointPS(VS_COLOR In)
{
	PS_OUTPUT Output;

	Output.RGBColor = In.Color;

	return Output;
}

//--------------------------------------------------------------------------------------
// Renders scene 
//--------------------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {          
		//colorwriteenable = 0x00;
		zenable = true;
        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader  = compile ps_3_0 RenderDepthPS();
    }

	pass P1
	{
		//colorwriteenable = 0x0f;
		zenable = true;
		VertexShader = compile vs_3_0 RenderShadowVS();
		PixelShader = compile ps_3_0 RenderShadowPS();
	}

	pass P2
	{
		//colorwriteenable = 0x0f;
		zenable = true;
		VertexShader = compile vs_3_0 RenderShadowVS();
		PixelShader = compile ps_3_0 RenderFeedbackPS();
	}

	pass P3
	{
		//colorwriteenable = 0x0f;
		zenable = true;
		VertexShader = compile vs_3_0 RenderShadowVS();
		PixelShader = compile ps_3_0 RenderFinalPS();
	}

	pass P4
	{
		//colorwriteenable = 0x0f;
		zenable = false;
		PointScaleEnable = true;
		VertexShader = compile vs_3_0 RenderPointVS();
		PixelShader = compile ps_3_0 RenderPointPS();
	}
}
