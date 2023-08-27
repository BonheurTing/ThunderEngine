{
    "ShaderName": "PBR",
    "ShaderSource": "shaders.hlsl",
    "Properties":
    [
        {
            "Name": "EmissiveTexture",
            "DisplayName": "Emissive",
            "Type": "Texture2D",
			"Format": "Float4",
            "Default": "White"
        },
        {
            "Name": "Intensity",
            "DisplayName": "Emissive Intensity",
            "Type": "Integer",
            "Range": "0, 10",
            "Default": "1"
        },
        {
            "Name": "EmissiveColor",
            "DisplayName": "Emissive Color",
            "Type": "Color",
            "Default": "(1,1,1,1)"
        }
    ],
    "ShaderParameters":
    [
        {
            "Name": "TintColor",
            "Type": "Float4",
            "Default": "(1,1,1,1)"
        }
    ],
    "RenderState":
    {
        "Blend": "Opaque",
        "LOD": "100",
        "Cull": "Front"
    },
    "Passes":
    {
        "GBuffer":
        {
            "PassVariants":
            [
                {
                    "Name": "EnableLumen",
                    "Default": "True",
                    "Fallback": "False"
                }
            ],
            "Vertex":
            {
                "EntryPoint": "VSMain",
                "StageVariants":
                [
                    {
                        "Name": "EnableVertexAnimation",
                        "Default": "True",
                        "Fallback": "False"
                    }
                ]
            },
            "Pixel":
            {
                "EntryPoint": "PSMain",
                "StageVariants":
                [
                    {
                        "Name": "EnableIBL",
                        "Default": "True",
                        "Fallback": "False",
                        "Texture": "SkylightCubemap",
                        "Visible": "False"
                    }
                ]
            }
        },
        "Lighting":
        {
        }
    }
}