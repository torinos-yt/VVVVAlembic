#pragma once
#include "abcrGeom.h"

namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    abcrGeom::abcrGeom() : type(UNKNOWN) {}

    abcrGeom::abcrGeom(IObject obj) : m_obj(obj), type(UNKNOWN) {}

    abcrGeom::~abcrGeom()
    {
        m_children.clear();

        if (m_obj)
            m_obj.reset();
    }

    void abcrGeom::setUpNodeRecursive(abcrGeom* geom)
    {
        if (geom == nullptr) return;

        IObject obj = geom->getIObject();
        size_t numChild = obj.getNumChildren();

        for (size_t i = 0; i < numChild; ++i)
        {
            auto* child = geom->newChild(obj.getChild(i));
            setUpNodeRecursive(child);
        }
    }

    abcrGeom* abcrGeom::newChild(const IObject& obj)
    {
        abcrGeom* geom = nullptr;
        if (obj.valid())
        {
            const auto& meta = obj.getMetaData();

            if (AbcGeom::IXformSchema::matches(meta))
            {
                AbcGeom::IXform xform(obj, obj.getName());
                geom = new abcr::XForm(xform);
            }
            else if (AbcGeom::IPointsSchema::matches(meta))
            {
                AbcGeom::IPoints points(obj, obj.getName());
                geom = new abcr::Points(points);
            }
            else if (AbcGeom::ICurvesSchema::matches(meta))
            {
                AbcGeom::ICurves curves(obj, obj.getName());
                geom = new abcr::Curves(curves);
            }
            else if (AbcGeom::IPolyMeshSchema::matches(meta))
            {
                AbcGeom::IPolyMesh pmesh(obj, obj.getName());
                geom = new abcr::PolyMesh(pmesh);
            }
            else
            {
                geom = new abcrGeom();
            }

            if (geom) m_children.emplace_back(geom);
            
        }
        return geom;
    }

}
}
}