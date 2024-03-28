#pragma once
const char* VSSourceGLSL = R"(
        #version 450
        vec2 positions[3] = vec2[](
            vec2(0, 0.5),
            vec2(-0.5, -0.5),
            vec2(0.5, -0.5)
        );

        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );

        layout(location = 0) out vec4 fragColor;

        void main()
        {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            fragColor = vec4(colors[gl_VertexIndex], 1.0);
        }
    )";

    const char* PSSourceGLSL = R"(
        #version 450

        layout(location = 0) in vec4 fragColor;
        
        layout(location = 0) out vec4 outColor;

        void main()
        {
            outColor = fragColor;
        }
    )";

    const char* VSSource = R"(
        struct PSInput 
        { 
            float4 Pos   : SV_POSITION; 
            float3 Color : COLOR; 
        };

        void main(in  uint    VertId : SV_VertexID,
                  out PSInput PSIn) 
        {
            float4 Pos[3];
            Pos[0] = float4(-0.5, -0.5, 0.0, 1.0);
            Pos[1] = float4( 0.0, +0.5, 0.0, 1.0);
            Pos[2] = float4(+0.5, -0.5, 0.0, 1.0);

            float3 Col[3];
            Col[0] = float3(1.0, 0.0, 0.0); // red
            Col[1] = float3(0.0, 1.0, 0.0); // green
            Col[2] = float3(0.0, 0.0, 1.0); // blue

            PSIn.Pos   = Pos[VertId];
            PSIn.Color = Col[VertId];
        }
    )";
    const char* PSSource = R"(
        struct PSInput 
        { 
            float4 Pos   : SV_POSITION; 
            float3 Color : COLOR; 
        };

        struct PSOutput
        { 
            float4 Color : SV_TARGET; 
        };

        void main(in  PSInput  PSIn,
                  out PSOutput PSOut)
        {
            PSOut.Color = float4(PSIn.Color.rgb, 1.0);
        }
    )";