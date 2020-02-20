#pragma once
#include "abcrGeom.h"

namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    abcrGeom::abcrGeom() : type(UNKNOWN) {}

    abcrGeom::abcrGeom(IObject obj, DX11RenderContext^ context) : m_obj(obj), m_context(context), type(UNKNOWN) {}

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
            auto* child = geom->newChild(obj.getChild(i), geom->m_context);
            setUpNodeRecursive(child);
        }
    }

    abcrGeom* abcrGeom::newChild(const IObject& obj, DX11RenderContext^ context)
    {
        abcrGeom* geom = nullptr;
        if (obj.valid())
        {
            const auto& meta = obj.getMetaData();

            if (AbcGeom::IXformSchema::matches(meta))
            {
                AbcGeom::IXform xform(obj, obj.getName());
                geom = new abcr::XForm(xform, context);
            }
            else if (AbcGeom::IPointsSchema::matches(meta))
            {
                AbcGeom::IPoints points(obj, obj.getName());
                geom = new abcr::Points(points, context);
            }
            else if (AbcGeom::ICurvesSchema::matches(meta))
            {
                AbcGeom::ICurves curves(obj, obj.getName());
                geom = new abcr::Curves(curves, context);
            }
            else if (AbcGeom::IPolyMeshSchema::matches(meta))
            {
                AbcGeom::IPolyMesh pmesh(obj, obj.getName());
                geom = new abcr::PolyMesh(pmesh, context);
            }
            else
            {
                geom = new abcrGeom();
            }

            if (geom) m_children.emplace_back(geom);
            
        }
        return geom;
    }

    template<typename T>
    void abcrGeom::setMinMaxTime(T& obj)
    {
        TimeSamplingPtr tptr = obj.getSchema().getTimeSampling();
        constant = obj.getSchema().isConstant();
        if (!constant)
        {
            size_t nSamples = obj.getSchema().getNumSamples();
            if (nSamples > 0)
            {
                m_minTime = tptr->getSampleTime(0);
                m_maxTime = tptr->getSampleTime(nSamples - 1);
            }
        }

        
    }

    void abcrGeom::updateTimeSample(chrono_t time, Imath::M44f& transform)
    {
        set(time, transform);
        this->transform = transform;

        for (size_t i = 0; i < m_children.size(); ++i)
        {
            Imath::M44f m = transform;
            m_children[i]->updateTimeSample(time, m);
        }
    }

    XForm::XForm(AbcGeom::IXform xform, DX11RenderContext^ context) : abcrGeom(xform, context), m_xform(xform)
    {
        type = XFORM;
        setMinMaxTime(m_xform);
    }

    Points::Points(AbcGeom::IPoints points, DX11RenderContext^ context) : abcrGeom(points, context), m_points(points)
    {
        type = POINTS;
        setMinMaxTime(m_points);
    }

    void Points::set(chrono_t time, Imath::M44f& transform)
    {
        if (static_cast<ISpread<Vector3D>^>(this->points) == nullptr)
        {
            points = gcnew Spread<Vector3D>();
        }

        AbcGeom::IPointsSchema ptSchema = m_points.getSchema();
        AbcGeom::IPointsSchema::Sample pts_sample;

        ISampleSelector ss(time, ISampleSelector::kNearIndex);

        AbcGeom::IPointsSchema ptSchema = m_points.getSchema();
        AbcGeom::IPointsSchema::Sample pts_sample;

        ptSchema.get(pts_sample, ss);

        P3fArraySamplePtr m_positions = pts_sample.getPositions();

        size_t nPts = m_positions->size();
        const V3f* src = m_positions->get();

        this->points->SliceCount = nPts;
        
        for (size_t i = 0; i < nPts; i++)
        {
            const V3f& v = src[i];
            static_cast<ISpread<Vector3D>^>(this->points)[0] = Vector3D(v.x, v.y, v.z);
        }
    }

    Curves::Curves(AbcGeom::ICurves curves, DX11RenderContext^ context) : abcrGeom(curves, context), m_curves(curves)
    {
        type = CURVES;
        setMinMaxTime(m_curves);
    }

    PolyMesh::PolyMesh(AbcGeom::IPolyMesh pmesh, DX11RenderContext^ context) : abcrGeom(pmesh, context), m_polymesh(pmesh)
    {
        type = POLYMESH;
        hasRGB = false;
        hasRGBA = false;
        setMinMaxTime(m_polymesh);

        auto geomParam = pmesh.getSchema().getArbGeomParams();
        size_t nParam = geomParam.getNumProperties();
        for (size_t i = 0; i < nParam; ++i)
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

        if (pmesh.getSchema().isConstant()) 
        {
            this->set(m_minTime, this->transform);
        }
    }

    void PolyMesh::set(chrono_t time, Imath::M44f& transform)
    {
        if (static_cast<DX11VertexGeometry^>(this->geom) == nullptr)
        {
            this->geom = gcnew DX11VertexGeometry();
        }

        AbcGeom::IPolyMeshSchema mesh = m_polymesh.getSchema();
        AbcGeom::IPolyMeshSchema::Sample mesh_samp;

        if (mesh.isConstant()) return;

        ISampleSelector ss(time, ISampleSelector::kNearIndex);
        
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
                        const Pos3Norm3Tex2Col3Vertex& p0 = { toSlimDX(v0) ,
                                                              toSlimDX(n0) ,
                                                              toSlimDX(uv0),
                                                              toSlimDX(col0) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[uvInds[t[1]]];
                        const C3f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col3Vertex& p1 = { toSlimDX(v1) ,
                                                              toSlimDX(n1) ,
                                                              toSlimDX(uv1),
                                                              toSlimDX(col1) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[uvInds[t[2]]];
                        const C3f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col3Vertex& p2 = { toSlimDX(v2) ,
                                                              toSlimDX(n2) ,
                                                              toSlimDX(uv2),
                                                              toSlimDX(col2) };

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
                        const Pos3Norm3Tex2Col4Vertex& p0 = { toSlimDX(v0) ,
                                                              toSlimDX(n0) ,
                                                              toSlimDX(uv0),
                                                              toSlimDX(col0) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[uvInds[t[1]]];
                        const C4f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col4Vertex& p1 = { toSlimDX(v1) ,
                                                              toSlimDX(n1) ,
                                                              toSlimDX(uv1),
                                                              toSlimDX(col1) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[uvInds[t[2]]];
                        const C4f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col4Vertex& p2 = { toSlimDX(v2) ,
                                                              toSlimDX(n2) ,
                                                              toSlimDX(uv2),
                                                              toSlimDX(col2) };

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
                        const Pos3Norm3Tex2Vertex& p0 = { toSlimDX(v0) ,
                                                          toSlimDX(n0) ,
                                                          toSlimDX(uv0) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[uvInds[t[1]]];
                        const Pos3Norm3Tex2Vertex& p1 = { toSlimDX(v1) ,
                                                          toSlimDX(n1) ,
                                                          toSlimDX(uv1) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[uvInds[t[2]]];
                        const Pos3Norm3Tex2Vertex& p2 = { toSlimDX(v2) ,
                                                          toSlimDX(n2) ,
                                                          toSlimDX(uv2) };

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
                        const Pos3Norm3Tex2Col3Vertex& p0 = { toSlimDX(v0) ,
                                                              toSlimDX(n0) ,
                                                              toSlimDX(uv0),
                                                              toSlimDX(col0)};

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[t[1]];
                        const C3f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col3Vertex& p1 = { toSlimDX(v1) ,
                                                              toSlimDX(n1) ,
                                                              toSlimDX(uv1),
                                                              toSlimDX(col1) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[t[2]];
                        const C3f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col3Vertex& p2 = { toSlimDX(v2) ,
                                                              toSlimDX(n2) ,
                                                              toSlimDX(uv2),
                                                              toSlimDX(col2) };

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
                        const Pos3Norm3Tex2Col4Vertex& p0 = { toSlimDX(v0) ,
                                                              toSlimDX(n0) ,
                                                              toSlimDX(uv0),
                                                              toSlimDX(col0) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[t[1]];
                        const C4f& col1 = cols[colInds[t[1]]];
                        const Pos3Norm3Tex2Col4Vertex& p1 = { toSlimDX(v1) ,
                                                              toSlimDX(n1) ,
                                                              toSlimDX(uv1),
                                                              toSlimDX(col1) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[t[2]];
                        const C4f& col2 = cols[colInds[t[2]]];
                        const Pos3Norm3Tex2Col4Vertex& p2 = { toSlimDX(v2) ,
                                                              toSlimDX(n2) ,
                                                              toSlimDX(uv2),
                                                              toSlimDX(col2) };

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
                        const Pos3Norm3Tex2Vertex& p0 = { toSlimDX(v0) ,
                                                          toSlimDX(n0) ,
                                                          toSlimDX(uv0) };

                        vertexStream->Write(p0);

                        const V3f& v1 = points[indices[t[1]]];
                        const N3f& n1 = norms[t[1]];
                        const V2f& uv1 = uvs[t[1]];
                        const Pos3Norm3Tex2Vertex& p1 = { toSlimDX(v1) ,
                                                          toSlimDX(n1) ,
                                                          toSlimDX(uv1) };

                        vertexStream->Write(p1);

                        const V3f& v2 = points[indices[t[2]]];
                        const N3f& n2 = norms[t[2]];
                        const V2f& uv2 = uvs[t[2]];
                        const Pos3Norm3Tex2Vertex& p2 = { toSlimDX(v2) ,
                                                          toSlimDX(n2) ,
                                                          toSlimDX(uv2) };

                        vertexStream->Write(p2);
                    }
                }
            }
            vertexStream->Position = 0;
        }

        {
            auto vertexBuffer = gcnew DX11Buffer(m_context->Device, vertexStream,
                (int)vertexStream->Length,
                ResourceUsage::Default,
                BindFlags::VertexBuffer,
                CpuAccessFlags::None,
                ResourceOptionFlags::None,
                (int)vertexSize
            );

            this->geom->Topology = PrimitiveTopology::TriangleList;
            this->geom->HasBoundingBox = false;
            this->geom->VerticesCount = m_triangles.size() * 3;
            this->geom->InputLayout = Layout;
            this->geom->VertexSize = vertexSize;
            this->geom->VertexBuffer = vertexBuffer;
        }

        delete vertexStream;
    }

    Camera::Camera(AbcGeom::ICamera camera, DX11RenderContext^ context) : abcrGeom(camera, context), m_camera(camera)
    {
        type = CAMERA;
        setMinMaxTime(m_camera);
    }

    void Camera::set(chrono_t time, Imath::M44f& transform)
    {
        ISampleSelector ss(time, ISampleSelector::kNearIndex);
        
        AbcGeom::CameraSample cam_samp;
        AbcGeom::ICameraSchema camSchema = m_camera.getSchema();

        camSchema.get(cam_samp);

        float Aperture = cam_samp.getVerticalAperture();
        float Near = cam_samp.getNearClippingPlane();
        float Far = cam_samp.getFarClippingPlane();
        float ForcalLength = cam_samp.getFocalLength();

        float FoV = 2.0 * ( atan(Aperture * 10.0 / (2.0 * ForcalLength)) ) * VMath::RadToDeg;

        this->proj = VMath::PerspectiveLH(FoV, Near, Far, 1.0);
        this->view = VMath::Inverse(toVVVV(this->transform));
    }
    
}
}
}