{
    "ShaderName": "PBR",
    "ShaderSource": "shaders.hlsl",
    "Properties":
    [
        {
            "Name": "EmissiveTexture",
            "DisplayName": "Emissive",
            "Type": "Texture2D",
            "Default": "White"
        },
        {
            "Name": "Intensity",
            "DisplayName": "Emissive Intensity",
            "Type": "Integer",
            "Range": "0, 10",
            "Default": 1
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
        "LOD": 100,
        "Cull": "Front"
    },
	"Passes":
    {
        "GBuffer":
        {
            "PassVariants":
            [
            ],
            "Vertex":
            {
                "EntryPoint": "VSMain",
                "EntryVariants":
                [
                ],
            }
            "Pixel":
            {
                "EntryPoint": "PSMain",
                "EntryVariants":
                [
                ],
            }
            "PassParameters":
            [
                {
                    "Name": "StepLength",
                    "Type": "Float",
                    "Default": "1"
                }
            ]
        },
        "Lighting":
        {
        }
    }
}