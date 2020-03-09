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

    bool abcrScene::open(const string& path, DX11RenderContext^ context)
    {
        m_archive = IArchive(AbcCoreOgawa::ReadArchive(), path,
            Alembic::Abc::ErrorHandler::kQuietNoopPolicy);

        if (!m_archive.valid()) return false;

        m_top.reset( new abcrGeom(m_archive.getTop(), context) );
        
        nameMap->Clear();
        fullnameMap->Clear();
        abcrGeom::setUpDocRecursive(m_top, static_cast<Dictionary<String^, abcrPtr>^>(nameMap),
                                    static_cast<Dictionary<String^, abcrPtr>^>(fullnameMap));
        
        m_minTime = m_top->m_minTime;
        m_maxTime = m_top->m_maxTime;

        currentIdx = -1;

        return true;
    }

    bool abcrScene::updateSample(float time)
    {
        if (!m_top) return false;

        auto timeSample = m_archive.getTimeSampling(1);
        auto maxSample = m_archive.getMaxNumSamplesForTimeSamplingIndex(1);
        auto nSample = maxSample / timeSample->getTimeSamplingType().getNumSamplesPerCycle();
        auto idx = timeSample->getNearIndex(time, nSample).first;

        if (currentIdx != idx)
        {
            currentIdx = idx;

            Imath::M44f m;
            m.makeIdentity();
            m_top->updateTimeSample(time, m);

            return true;
        }

        return false;

    }

    //XForm
    bool abcrScene::getSample(const string& path, Matrix4x4& xform)
    {
        return false;
    }

    //Points
    bool abcrScene::getSample(const string& path, ISpread<Vector3D>^& points)
    {
        return false;
    }

    //Curves
    bool abcrScene::getSample(const string& path, ISpread<ISpread<Vector3D>^>^& curves)
    {
        return false;
    }

    //PolyMesh
    bool abcrScene::getSample(const string& path, DX11VertexGeometry^ geom)
    {
        return false;
    }

    //Camera
    bool abcrScene::getSample(const string& path, Matrix4x4& view, Matrix4x4& proj)
    {
        return false;
    }

}
}
}