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
        [PluginInfo(Name = "Alembic File", Category = "DX11.Geometry Alembic", Tags = "")]
        public ref class VVVVAlembicReader : public IPluginEvaluate, IDX11ResourceHost
        {
        public:

            [Input("Filename", IsSingle = true, StringType = StringType::Filename )]
            Pin<String^>^ FPath;

            [Input("Time", IsSingle = true, DefaultValue = 0.0)]
            IDiffSpread<float>^ FTime;

            [Input("Reload", IsSingle = true, IsBang = true)]
            ISpread<bool>^ FReload;

            [Output("Geometry Out")]
            ISpread<DX11Resource<IDX11Geometry^>^>^ FOutgeo;

            [Output("Message")]
            ISpread<String^>^ FMessage;

            [Import()]
            ILogger^ FLogger;

            void Evaluate(int SpreadMax) override;
            void Update(DX11RenderContext^ context) override;
            void Destroy(DX11RenderContext^ context, bool force) override;

        private:

            IArchive *m_archive;
            IPolyMesh *m_mesh;

            bool FFirst = true;

            DataStream^ vertexStream;
            size_t vertexSize = Pos3Norm3Tex2Vertex::VertexSize;
        };
    }
}


string ToporogyArray[5] =
{
    "kConstantTopology",     //0
    "kHomogenousTopology",   //1
    "kHomogeneousTopology",  //2
    "kHeterogenousTopology", //3
    "kHeterogeneousTopology" //4
};
