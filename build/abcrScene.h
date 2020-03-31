#pragma once
#include <msclr\marshal_cppstd.h>
using namespace msclr::interop;

#include <gcroot.h>

#include <Alembic\Abc\All.h>
#include <Alembic\AbcCoreOgawa\All.h>

#include "abcrGeom.h"

using namespace Alembic;

using namespace VVVV::PluginInterfaces::V1;
using namespace VVVV::PluginInterfaces::V2;

using namespace System;
using namespace System::Collections::Generic;

namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    class abcrScene
    {
        public:

            abcrScene();
            ~abcrScene();

            bool open(const string& path, DX11RenderContext^ context);

            bool updateSample(chrono_t time);

            bool valid() const { return m_top->valid(); };

            inline float getMaxTime() const { return m_maxTime; };
            inline float getMinTime() const { return m_minTime; };
            
            bool getSample(const string& name, Matrix4x4& xform);                     //XForm
            bool getSample(const string& name, ISpread<Vector3D>^& points);           //Points
            bool getSample(const string& name, ISpread<ISpread<Vector3D>^>^& curves); //Cueves
            bool getSample(const string& name, DX11VertexGeometry^ geom);             //PolyMesh
            bool getSample(const string& name, Matrix4x4& view, Matrix4x4& proj);     //Camera

            void getFullNameMap(ISpread<String^>^% names)
            {
                SpreadExtensions::AssignFrom(names, fullnameMap->Keys);
            }

            Dictionary<String^, abcrPtr >::ValueCollection^ getGeomIterator() const
            {
                return this->nameMap->Values;
            }

        private:

            IArchive m_archive;
            shared_ptr<abcrGeom> m_top;

            chrono_t m_minTime;
            chrono_t m_maxTime;

            gcroot<Dictionary<String^, abcrPtr>^> nameMap;
            gcroot<Dictionary<String^, abcrPtr>^> fullnameMap;

            size_t currentIdx;

    };
}
}
}