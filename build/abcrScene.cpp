#include "abcrScene.h"

namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    abcrScene::abcrScene()
    {
        this->nameMap = gcnew Dictionary<String^, abcrPtr>();
        this->fullnameMap = gcnew Dictionary<String^, abcrPtr>();
    }

    abcrScene::~abcrScene() 
    {
        nameMap->Clear();
        fullnameMap->Clear();

        if (m_top) m_top.reset();
        if (m_archive.valid()) m_archive.reset();
    }

    bool abcrScene::open(String^ path, DX11RenderContext^ context, ISpread<String^>^% names)
    {
        m_archive = IArchive(AbcCoreOgawa::ReadArchive(), marshal_as<string>(path),
            Alembic::Abc::ErrorHandler::kQuietNoopPolicy);

        if (!m_archive.valid()) return false;

        m_top = shared_ptr<abcrGeom>(new abcrGeom(m_archive.getTop(), context));
        
        {
            nameMap->Clear();
            fullnameMap->Clear();

            abcrGeom::setUpDocRecursive(m_top, static_cast<Dictionary<String^, abcrPtr>^>(nameMap), static_cast<Dictionary<String^, abcrPtr>^>(fullnameMap));
        }
        
        m_minTime = m_top->m_minTime;
        m_maxTime = m_top->m_maxTime;
        
        names->SliceCount = 1;
        SpreadExtensions::AssignFrom(names, fullnameMap->Keys);

        return true;
    }

    void abcrScene::updateSample(float time)
    {
        if (!m_top) return;

        Imath::M44f m;
        m.makeIdentity();
        m_top->updateTimeSample(time, m);
    }

    //XForm
    bool abcrScene::getSample(const String^& path, Matrix4x4& xform)
    {
        return false;
    }

    //Points
    bool abcrScene::getSample(const String^& path, ISpread<Vector3D>^& points)
    {
        return false;
    }

    //Curves
    bool abcrScene::getSample(const String^& path, ISpread<ISpread<Vector3D>^>^& curves)
    {
        return false;
    }

    //PolyMesh
    bool abcrScene::getSample(const String^& path, DX11VertexGeometry^ geom)
    {
        return false;
    }

    //Camera
    bool abcrScene::getSample(const String^& path, Matrix4x4& view, Matrix4x4& proj)
    {
        return false;
    }

}
}
}