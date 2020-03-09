#pragma once
using namespace SlimDX;
using namespace SlimDX::Direct3D11;

namespace FeralTic
{
namespace DX11
{
namespace Geometry
{
    public value class Pos3Norm3Tex2Col3Vertex
    {
    public:
        Vector3 Position;
        Vector3 Normals;
        Vector2 TexCoords;
        Vector3 Colors;

        static cli::array<InputElement>^ Layout = 
                    {
                        InputElement("POSITION", 0, SlimDX::DXGI::Format::R32G32B32_Float, 0, 0),
                        InputElement("NORMAL", 0, SlimDX::DXGI::Format::R32G32B32_Float, 12, 0),
                        InputElement("TEXCOORD", 0, SlimDX::DXGI::Format::R32G32_Float, 24, 0),
                        InputElement("COLOR", 0, SlimDX::DXGI::Format::R32G32B32_Float, 32, 0),
                    };

        
        static property int VertexSize
        {
            int get() { return sizeof(Pos3Norm3Tex2Col3Vertex); }
        };

    };

    public value class Pos3Norm3Tex2Col4Vertex
    {
    public:
        Vector3 Position;
        Vector3 Normals;
        Vector2 TexCoords;
        Vector4 Colors;

        static cli::array<InputElement>^ Layout = 
                    {
                        InputElement("POSITION", 0, SlimDX::DXGI::Format::R32G32B32_Float, 0, 0),
                        InputElement("NORMAL", 0, SlimDX::DXGI::Format::R32G32B32_Float, 12, 0),
                        InputElement("TEXCOORD", 0, SlimDX::DXGI::Format::R32G32_Float, 24, 0),
                        InputElement("COLOR", 0, SlimDX::DXGI::Format::R32G32B32A32_Float, 32, 0),
                    };

        static property int VertexSize
        {
            int get() { return sizeof(Pos3Norm3Tex2Col4Vertex); }
        };

    };
}
}
}