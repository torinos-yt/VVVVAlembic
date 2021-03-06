#pragma once
#include <Alembic\AbcGeom\IPolyMesh.h>
#include <Alembic\AbcGeom\IXform.h>
#include <Alembic\AbcGeom\ICurves.h>
#include <Alembic\AbcGeom\IPoints.h>
#include <Alembic\AbcGeom\ICamera.h>
#include <Alembic\AbcGeom\INuPatch.h>

#include <Alembic\Abc\All.h>
#include <Alembic\Util\All.h>
#include <Alembic\AbcCoreOgawa\All.h>

#include "abcrGeom.h"
#include "abcrScene.h"


using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::ComponentModel::Composition;

using namespace VVVV::PluginInterfaces::V1;
using namespace VVVV::PluginInterfaces::V2;

using namespace VVVV::Core::Logging;

using namespace VVVV::DX11;

using namespace SlimDX;
using namespace SlimDX::Direct3D11;

using DX11Buffer = SlimDX::Direct3D11::Buffer;

using namespace FeralTic::DX11;
using namespace FeralTic::DX11::Resources;
using namespace FeralTic::DX11::Geometry;

using namespace Alembic;
using namespace Alembic::AbcGeom;
using namespace Alembic::Abc;

using namespace std;


namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    [PluginInfo(Name = "AlembicFile", Category = "DX11.Geometry Alembic", Tags = "")]
    public ref class VVVVAlembicReader : public IPluginEvaluate, IDX11ResourceHost
    {
    public:

        [Input("Filename", IsSingle = true, AutoValidate = false, StringType = StringType::Filename)]
        Pin<String^>^ FPath;

        [Input("Time", IsSingle = true, DefaultValue = 0.0, MinValue = 0.0)]
        IDiffSpread<float>^ FTime;

        [Input("Reload", IsSingle = true, IsBang = true)]
        ISpread<bool>^ FReload;

        [Output("Geometry Out")]
        ISpread<DX11Resource<IDX11Geometry^>^>^ FOutgeo;

        [Output("Message")]
        ISpread<String^>^ FMessage;

        [Import()]
        ILogger^ FLogger;

        virtual void Evaluate(int SpreadMax) override;
        virtual void Update(DX11RenderContext^ context) override;
        virtual void Destroy(DX11RenderContext^ context, bool force) override;

    private:

        abcrGeom* m_root;

        IArchive* m_archive;
        IPolyMesh* m_mesh;

        bool FFirst = true;

        DataStream^ vertexStream;
        size_t vertexSize = Pos3Norm3Tex2Vertex::VertexSize;
    };

    value struct AbcScene
    {
        abcrScene* m_Scene;
    };

    [PluginInfo(Name = "AlembicScene", Category = "Alembic", Tags = "")]
    public ref class VVVVAlembicScene : public IPluginEvaluate, IDX11ResourceDataRetriever
    {
    public:

        [Input("Filename", IsSingle = true, AutoValidate = false, StringType = StringType::Filename)]
        Pin<String^>^ FPath;

        [Input("Time", IsSingle = true, DefaultValue = 0.0, MinValue = 0.0)]
        IDiffSpread<float>^ FTime;

        [Input("Reload", IsSingle = true, IsBang = true)]
        ISpread<bool>^ FReload;

        [Output("Scene Out")]
        ISpread<AbcScene>^ FOutScene;

        [Output("End Time")]
        ISpread<float>^ FDulation;

        [Output("Names")]
        ISpread<String^>^ FNames;

        [Import()]
        ILogger^ FLogger;

        [Import()]
        IPluginHost^ FHost;

        virtual property DX11RenderContext^ AssignedContext;

        virtual event DX11RenderRequestDelegate^ RenderRequest;

        virtual void Evaluate(int SpreadMax) override;

    private:

        bool FFirst = true;

    };

    [PluginInfo(Name = "Point", Category = "3D Alembic", Tags = "")]
    public ref class VVVVAlembicPoint : public IPluginEvaluate
    {
    public:

        [Input("Scene In", IsSingle = true)]
        Pin<AbcScene>^ FInScene;

        [Output("Out", Order = 0)]
        ISpread<ISpread<Vector3D>^>^ FOutPoints;

        [Output("Transform Out", Order = 2)]
        ISpread<Matrix4x4>^ FOutMat;

        [Output("Names", Order = 3)]
        ISpread<String^>^ FNames;

        [Import()]
        ILogger^ FLogger;

        virtual void Evaluate(int SpreadMax) override;

    private:

        bool FFirst = true;

    };

    [PluginInfo(Name = "Curve", Category = "3D Alembic", Tags = "")]
    public ref class VVVVAlembicCurve : public IPluginEvaluate
    {
    public:

        [Input("Scene In", IsSingle = true)]
        Pin<AbcScene>^ FInScene;

        [Output("Out", Order = 0)]
        ISpread<ISpread<Vector3D>^>^ FOutPoints;

        [Output("Curve Count", Order = 2)]
        ISpread<int>^ FOutCnt;

        [Output("End Point", Order = 3, Visibility = PinVisibility::OnlyInspector)]
        ISpread<float>^ FOutIndex;

        [Output("Transform Out", Order = 4)]
        ISpread<Matrix4x4>^ FOutMat;

        [Output("Names", Order = 5)]
        ISpread<String^>^ FNames;

        [Import()]
        ILogger^ FLogger;

        virtual void Evaluate(int SpreadMax) override;

    private:

        bool FFirst = true;

    };

    [PluginInfo(Name = "Mesh", Category = "DX11.Geometry Alembic", Tags = "")]
    public ref class VVVVAlembicPolyMesh : public IPluginEvaluate, IDX11ResourceHost
    {
    public:

        [Input("Scene In", IsSingle = true)]
        Pin<AbcScene>^ FInScene;

        [Output("Geometry Out")]
        ISpread<DX11Resource<DX11VertexGeometry^>^>^ FOutGeo;

        [Output("Transform Out", Order = 2)]
        ISpread<Matrix4x4>^ FOutMat;

        [Output("Names", Order = 3)]
        ISpread<String^>^ FNames;

        [Import()]
        ILogger^ FLogger;

        virtual void Evaluate(int SpreadMax) override;
        virtual void Update(DX11RenderContext^ context) override;
        virtual void Destroy(DX11RenderContext^ context, bool force) override;

    private:

        bool FFirst = true;

    };

    [PluginInfo(Name = "Camera", Category = "Transform Alembic", Tags = "")]
    public ref class VVVVAlembicCamera : public IPluginEvaluate
    {
    public:

        [Input("Scene In", IsSingle = true)]
        Pin<AbcScene>^ FInScene;

        [Output("View", Order = 1)]
        ISpread<Matrix4x4>^ FOutView;

        [Output("Projection", Order = 2)]
        ISpread<Matrix4x4>^ FOutProj;

        [Output("Names", Order = 3)]
        ISpread<String^>^ FNames;

        [Import()]
        ILogger^ FLogger;

        virtual void Evaluate(int SpreadMax) override;

    private:

        bool FFirst = true;

    };

}
}
}