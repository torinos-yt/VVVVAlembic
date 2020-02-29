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

        this->isValid = false;
    }

    abcrScene::~abcrScene() 
    {
        nameMap->Clear();
        fullnameMap->Clear();

        if (m_top) m_top->reset();
        if (m_archive->valid()) m_archive->reset();
    }

    bool abcrScene::open(String^ path, DX11RenderContext^ context, ISpread<String^>^% names)
    {
        IArchive archive = IArchive(AbcCoreOgawa::ReadArchive(), marshal_as<string>(path),
            Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
        m_archive = &archive;

        if (!m_archive->valid())
        {
            isValid = false;
            return false;
        }

        shared_ptr<abcrGeom> top = shared_ptr<abcrGeom>(new abcrGeom(m_archive->getTop(), context));
        m_top = &top;
        
        {
            nameMap->Clear();
            fullnameMap->Clear();

            abcrGeom::setUpDocRecursive(*m_top, nameMap, fullnameMap);
        }
        
        m_minTime = m_top->get()->m_minTime;
        m_maxTime = m_top->get()->m_maxTime;
        
        names->SliceCount = 1;
        SpreadExtensions::AssignFrom(names, fullnameMap->Keys);

        isValid = true;
        return true;
    }

    void abcrScene::updateSample(float time)
    {

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