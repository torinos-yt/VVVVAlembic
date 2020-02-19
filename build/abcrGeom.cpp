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

    PolyMesh::PolyMesh(AbcGeom::IPolyMesh pmesh) : abcrGeom(pmesh), m_polymesh(pmesh)
    {
        type = POLYMESH;
        hasRGB = false;
        hasRGBA = false;

        auto geomParam = pmesh.getSchema().getArbGeomParams();
        size_t numParam = geomParam.getNumProperties();
        for (size_t i = 0; i < numParam; ++i)
        {
            auto& head = geomParam.getPropertyHeader(i);

            if (AbcGeom::IC3fGeomParam::matches(head))
            {
                hasRGB = true;
                vertexSize = Pos3Norm3Tex2Col3Vertex::VertexSize;
                Layout = Pos3Norm3Tex2Col3Vertex::Layout;
                m_rgb = AbcGeom::IC3fGeomParam(geomParam, head.getName());
            }
            if (AbcGeom::IC4fGeomParam::matches(head))
            {
                hasRGBA = true;
                vertexSize = Pos3Norm3Tex2Col4Vertex::VertexSize;
                Layout = Pos3Norm3Tex2Col4Vertex::Layout;
                m_rgba = AbcGeom::IC4fGeomParam(geomParam, head.getName());
            }
        }
        if (!hasRGB && !hasRGBA)
        {
            vertexSize = Pos3Norm3Tex2Vertex::VertexSize;
            Layout = Pos3Norm3Tex2Vertex::Layout;
        }
    }

    void PolyMesh::set(DX11RenderContext^ context, DX11VertexGeometry^& geom, chrono_t time)
    {
        ISampleSelector ss(time, ISampleSelector::kNearIndex);

        AbcGeom::IPolyMeshSchema mesh = m_polymesh.getSchema();
        AbcGeom::IPolyMeshSchema::Sample mesh_samp;
        
        mesh.get(mesh_samp, ss);

        //sample some property
        P3fArraySamplePtr m_points = mesh_samp.getPositions();
        Int32ArraySamplePtr m_indices = mesh_samp.getFaceIndices();
        Int32ArraySamplePtr m_faceCounts = mesh_samp.getFaceCounts();

        AbcGeom::IN3fGeomParam N = mesh.getNormalsParam();
        if (N.isIndexed())
        {
            return;
        }
        N3fArraySamplePtr m_norms = N.getExpandedValue(ss).getVals();

        bool IsIndexedUV = false;
        AbcGeom::IV2fGeomParam UV = mesh.getUVsParam();
        AbcGeom::IV2fGeomParam::Sample uvValue;
        V2fArraySamplePtr m_uvs;
        if (UV.isIndexed())
        {
            IsIndexedUV = true;
            uvValue = UV.getIndexedValue(ss);
            m_uvs = uvValue.getVals();
        }
        else
        {
            uvValue = UV.getExpandedValue(ss);
            m_uvs = uvValue.getVals();
        }

        size_t nPts = m_points->size();
        size_t nInds = m_indices->size();
        size_t nFace = m_faceCounts->size();
        if (nPts < 1 || nInds < 1 || nFace < 1)
        {
            return;
        }

        using tri = Imath::Vec3<unsigned int>;
        using triArray = std::vector<tri>;
        triArray m_triangles;

        {
            size_t fBegin = 0;
            size_t fEnd = 0;
            for (size_t face = 0; face < nFace; ++face)
            {
                fBegin = fEnd;
                size_t count = (*m_faceCounts)[face];
                fEnd = fBegin + count;

                if (fEnd > nInds || fEnd < fBegin)
                {
                    return;
                }

                if (count >= 3)
                {
                    m_triangles.push_back(tri((unsigned int)fBegin + 2,
                        (unsigned int)fBegin + 1,
                        (unsigned int)fBegin + 0));
                    for (size_t c = 3; c < count; ++c)
                    {
                        m_triangles.push_back(tri((unsigned int)fBegin + c,
                            (unsigned int)fBegin + c - 1,
                            (unsigned int)fBegin + 0));
                    }
                }
            }
        }

        auto vertexStream = gcnew SlimDX::DataStream(m_triangles.size() * 3 * vertexSize, true, true);
        vertexStream->Position = 0;

        {
            const V3f* points = m_points->get();
            const N3f* norms = m_norms->get();
            const V2f* uvs = m_uvs->get();
            const int32_t* indices = m_indices->get();
            if (IsIndexedUV)
            {
                const auto uvInds = uvValue.getIndices()->get();
                if (hasRGB)
                {
                    const AbcGeom::IC3fGeomParam::Sample m_col = m_rgb.getIndexedValue(ss);
                    const C3f* cols = m_col.getVals()->get();
                    const auto colInds = m_col.getIndices()->get();
                    for (size_t j = 0; j < m_triangles.size(); ++j)
                    {
                        tri& t = m_triangles[j];

                        const V3f& v0 = points[indices[t[0]]];
                        const N3f& n0 = norms[t[0]];
                        const V2f& uv0 = uvs[uvInds[t[0]]];
                        const C3f& col0 = cols[colInds[t[0]]];
                        const Pos3Norm3Tex2Col3Vertex& p0 = { SlimDX::Vector3(v0.x, v0.y, v0.z) ,
                                                            SlimDX::Vector3(n0.x, n0.y, n0.z) ,
                                                            SlimDX::Vector2(uv0.x, uv0.y),
                                                            SlimDX::Vector3(col0.x, col0.y, col0.z) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[uvInds[t[1]]];
                        const C3f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col3Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
                                                            SlimDX::Vector3(n1.x, n1.y, n1.z) ,
                                                            SlimDX::Vector2(uv1.x, uv1.y),
                                                            SlimDX::Vector3(col1.x, col1.y, col1.z) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[uvInds[t[2]]];
                        const C3f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col3Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
                                                            SlimDX::Vector3(n2.x, n2.y, n2.z) ,
                                                            SlimDX::Vector2(uv2.x, uv2.y),
                                                            SlimDX::Vector3(col2.x, col2.y, col2.z) };

                        vertexStream->Write(p2);
                    }
                }
                else if (hasRGBA)
                {
                    const AbcGeom::IC4fGeomParam::Sample m_col = m_rgba.getIndexedValue(ss);
                    const C4f* cols = m_col.getVals()->get();
                    const auto colInds = m_col.getIndices()->get();
                    for (size_t j = 0; j < m_triangles.size(); ++j)
                    {
                        tri& t = m_triangles[j];

                        const V3f& v0 = points[indices[t[0]]];
                        const N3f& n0 = norms[t[0]];
                        const V2f& uv0 = uvs[uvInds[t[0]]];
                        const C4f& col0 = cols[colInds[t[0]]];
                        const Pos3Norm3Tex2Col4Vertex& p0 = { SlimDX::Vector3(v0.x, v0.y, v0.z) ,
                                                            SlimDX::Vector3(n0.x, n0.y, n0.z) ,
                                                            SlimDX::Vector2(uv0.x, uv0.y),
                                                            SlimDX::Vector4(col0.r, col0.g, col0.b, col0.a) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[uvInds[t[1]]];
                        const C4f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col4Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
                                                            SlimDX::Vector3(n1.x, n1.y, n1.z) ,
                                                            SlimDX::Vector2(uv1.x, uv1.y),
                                                            SlimDX::Vector4(col1.r, col1.g, col1.b, col1.a) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[uvInds[t[2]]];
                        const C4f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col4Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
                                                            SlimDX::Vector3(n2.x, n2.y, n2.z) ,
                                                            SlimDX::Vector2(uv2.x, uv2.y),
                                                            SlimDX::Vector4(col2.r, col2.g, col2.b, col2.a) };

                        vertexStream->Write(p2);
                    }
                }
                else
                {
                    for (size_t j = 0; j < m_triangles.size(); ++j)
                    {
                        tri& t = m_triangles[j];

                        const V3f& v0 = points[indices[t[0]]];
                        const N3f& n0 = norms[t[0]];
                        const V2f& uv0 = uvs[uvInds[t[0]]];
                        const Pos3Norm3Tex2Vertex& p0 = { SlimDX::Vector3(v0.x, v0.y, v0.z) ,
                                                            SlimDX::Vector3(n0.x, n0.y, n0.z) ,
                                                            SlimDX::Vector2(uv0.x, uv0.y) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[uvInds[t[1]]];
                        const Pos3Norm3Tex2Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
                                                            SlimDX::Vector3(n1.x, n1.y, n1.z) ,
                                                            SlimDX::Vector2(uv1.x, uv1.y) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[uvInds[t[2]]];
                        const Pos3Norm3Tex2Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
                                                            SlimDX::Vector3(n2.x, n2.y, n2.z) ,
                                                            SlimDX::Vector2(uv2.x, uv2.y) };

                        vertexStream->Write(p2);
                    }
                }
            }
            else
            {
                if (hasRGB)
                {
                    const AbcGeom::IC3fGeomParam::Sample m_col = m_rgb.getIndexedValue(ss);
                    const C3f* cols = m_col.getVals()->get();
                    const auto colInds = m_col.getIndices()->get();
                    for (size_t j = 0; j < m_triangles.size(); ++j)
                    {
                        tri& t = m_triangles[j];

                        const V3f& v0 = points[indices[t[0]]];
                        const N3f& n0 = norms[t[0]];
                        const V2f& uv0 = uvs[t[0]];
                        const C3f& col0 = cols[colInds[t[0]]];
                        const Pos3Norm3Tex2Col3Vertex& p0 = { SlimDX::Vector3(v0.x, v0.y, v0.z) ,
                                                            SlimDX::Vector3(n0.x, n0.y, n0.z) ,
                                                            SlimDX::Vector2(uv0.x, uv0.y),
                                                            SlimDX::Vector3(col0.x, col0.y, col0.z) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[t[1]];
                        const C3f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col3Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
                                                            SlimDX::Vector3(n1.x, n1.y, n1.z) ,
                                                            SlimDX::Vector2(uv1.x, uv1.y),
                                                            SlimDX::Vector3(col1.x, col1.y, col1.z) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[t[2]];
                        const C3f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col3Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
                                                            SlimDX::Vector3(n2.x, n2.y, n2.z) ,
                                                            SlimDX::Vector2(uv2.x, uv2.y),
                                                            SlimDX::Vector3(col2.x, col2.y, col2.z) };

                        vertexStream->Write(p2);
                    }
                }
                else if (hasRGBA)
                {
                    const AbcGeom::IC4fGeomParam::Sample m_col = m_rgba.getIndexedValue(ss);
                    const C4f* cols = m_col.getVals()->get();
                    const auto colInds = m_col.getIndices()->get();
                    for (size_t j = 0; j < m_triangles.size(); ++j)
                    {
                        tri& t = m_triangles[j];

                        const V3f& v0 = points[indices[t[0]]];
                        const N3f& n0 = norms[t[0]];
                        const V2f& uv0 = uvs[t[0]];
                        const C4f& col0 = cols[colInds[t[0]]];
                        const Pos3Norm3Tex2Col4Vertex& p0 = { SlimDX::Vector3(v0.x, v0.y, v0.z) ,
                                                            SlimDX::Vector3(n0.x, n0.y, n0.z) ,
                                                            SlimDX::Vector2(uv0.x, uv0.y),
                                                            SlimDX::Vector4(col0.r, col0.g, col0.b, col0.a) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[t[1]];
                        const C4f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col4Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
                                                            SlimDX::Vector3(n1.x, n1.y, n1.z) ,
                                                            SlimDX::Vector2(uv1.x, uv1.y),
                                                            SlimDX::Vector4(col1.r, col1.g, col1.b, col1.a) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[t[2]];
                        const C4f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col4Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
                                                            SlimDX::Vector3(n2.x, n2.y, n2.z) ,
                                                            SlimDX::Vector2(uv2.x, uv2.y),
                                                            SlimDX::Vector4(col2.r, col2.g, col2.b, col2.a) };

                        vertexStream->Write(p2);
                    }
                }
                else
                {
                    for (size_t j = 0; j < m_triangles.size(); ++j)
                    {
                        tri& t = m_triangles[j];

                        const V3f& v0 = points[indices[t[0]]];
                        const N3f& n0 = norms[t[0]];
                        const V2f& uv0 = uvs[t[0]];
                        const Pos3Norm3Tex2Vertex& p0 = { SlimDX::Vector3(v0.x, v0.y, v0.z) ,
                                                            SlimDX::Vector3(n0.x, n0.y, n0.z) ,
                                                            SlimDX::Vector2(uv0.x, uv0.y) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[t[1]];
                        const Pos3Norm3Tex2Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
                                                            SlimDX::Vector3(n1.x, n1.y, n1.z) ,
                                                            SlimDX::Vector2(uv1.x, uv1.y) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[t[2]];
                        const Pos3Norm3Tex2Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
                                                            SlimDX::Vector3(n2.x, n2.y, n2.z) ,
                                                            SlimDX::Vector2(uv2.x, uv2.y) };

                        vertexStream->Write(p2);
                    }
                }
            }
            vertexStream->Position = 0;
        }

        {
            auto vertexBuffer = gcnew DX11Buffer(context->Device, vertexStream,
                (int)vertexStream->Length,
                ResourceUsage::Default,
                BindFlags::VertexBuffer,
                CpuAccessFlags::None,
                ResourceOptionFlags::None,
                (int)vertexSize
            );

            geom->Topology = PrimitiveTopology::TriangleList;
            geom->HasBoundingBox = false;
            geom->VerticesCount = m_triangles.size() * 3;
            geom->InputLayout = Layout;
            geom->VertexSize = vertexSize;
            geom->VertexBuffer = vertexBuffer;
        }
    }
}
}
}