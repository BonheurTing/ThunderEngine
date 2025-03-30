/**
* 使用情景1：希望场景中画一个opaque球，一个translucent球
* 美术：创建一个mesh球; 创建myopaque.tsf; 创建一个材质mymaterial1，挂myopaque.tsf
*/
//myopaque.tsf
Shader "myopaque"
{
    Properties
    {
        Color _Color = (1,1,1,1) { Name : "Color", Category : "Render" }
        Texture2D _MainTex = "white" { Name : "Albedo(RGB)"}
        float _Metallic = 0.0 { Name : "Metallic" }
        float _Roughness = 0.5 { Name : "Roughness", Range : (0, 1) }
    }
    SubShader
    {
        Tags { MeshDrawType : "Opaque" }

        #pragma stage vertex DefaultVert
        #pragma stage fragment frag

        #pragma variant EnableIBL

        void frag (Input IN, inout SurfaceOutputStandard o)
        {
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb;
            o.Metallic = _Metallic;
            o.Smoothness = _Glossiness;
            o.Alpha = c.a;
        }
    }
}

/**
* 使用情景2：希望场景中画一个translucent球, 用双pass实现
* 美术：创建一个mesh球; 创建mytranslucent.tsf; 创建一个材质mymaterial2，挂mytranslucent.tsf
*/
//mytranslucent.tsf
Shader "mytranslucent"
{
    Properties
    {
        Texture2D _MainTex = "white" { Name : "Albedo(RGB)"}
        float _Emissive = 0.0 { Name : "Emissive" }
    }
    SubShader
    {
        Name "TransparentPreDepth"
        Tags { MeshDrawType : "Transparent" }

        #pragma stage vertex DefaultVert
        #pragma stage fragment frag

        #pragma depthOp ZWrite

        void frag (v2f) {}
    }
    SubShader
    {
        Name "Transparent"
        Tags { MeshDrawType : "Transparent" }

        #pragma stage vertex vert
        #pragma stage fragment frag

        void frag (Input IN, inout SurfaceOutputStandard o)
        {
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex);
            o.Albedo = c.rgb;
            o.Emissive = _Emissive;
        }
    }
}

/**
* 使用情景3：实现vxgi的gi渲染算法
*/
//vxgi.tsf
Shader "VXGI"
{
    Properties
    {
        int _TraceStepCount = 5 { Name : "StepCount" , Visible : false }
    }
    SubShader
    {
        Name "Voxelize"
        Tags { MeshDrawType : "Voxelize" }

        #pragma vertex vert_gbuffer
        #pragma fragment frag_gbuffer

        v2f vert_gbuffer() {}
        void frag_gbuffer(v2f) {}
    }
    SubShader
    {
        Name "Trace"

        #pragma vertex vert_trace
        #pragma fragment frag_trace

        v2f vert_trace() {}
        void frag_trace(v2f) {}
    }
    SubShader
    {
        Name "Lighting"

        #pragma vertex vert_lighting
        #pragma fragment frag_lighting

        v2f vert_lighting() {}
        void frag_lighting(v2f) {}
    }
}

//RDG.h
class IRDGPass
{
    virtual Render() = 0;
}
class IMeshDrawPass : public IRDGPass
{
public:
    void BuildMeshBatch()
    {
        for(obj : scene->allobjects)
        {
            shader = obj->getMaterial()->getShader();
            for (pass in shader->getPasses())
            {
                if (pass->getMeshDrawType() == mMeshDrawType)
                {
                    addMeshBatch(obj, mat);
                }
            }
        }
    }

    EMeshDrawType mMeshDrawType;
}

//VXGI.cpp
class VXGIVoxelizePass : public IMeshDrawPass
{
    VXGIVoxelizePass() : mMeshDrawType(EMeshDrawType_Voxelize) {}
}
class VXGITracePass : public IRDGPass
{
    VXGITracePass() : IRDGPass("VXGI", "Trace"){}
}
class VXGILightingPass : public IRDGPass
{
    VXGILightingPass() : IRDGPass("VXGI", "Lighting"){}
}

void Render()
{
    ...
    pass1 = RDG->AddMeshDrawPass(new GBufferPass);
    pass2 = RDG->AddMeshDrawPass(new TranslucentPass);
    
    pass2->AddDependency(pass1);
    ...
    if(configfile->enableLumen)
    {
        ...
    }
    else if(configfile->enableVxgi)
    {
        pass3 = RDG->AddMeshDrawPass(new VXGIVoxelizePass);
        pass4 = RDG->AddPass(new VXGITracePass);
        pass5 = RDG->AddPass(new VXGIlightingPass);
        pass3->AddDependency(pass2);
        pass4->AddDependency(pass3);
        pass5->AddDependency(pass4);
    }
    ...
}