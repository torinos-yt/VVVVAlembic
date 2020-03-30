#pragma once
#include "abcrGeom.h"

namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    abcrGeom::abcrGeom() : type(UNKNOWN), 
                        m_minTime(std::numeric_limits<float>::infinity()), m_maxTime(0) {}

    abcrGeom::abcrGeom(IObject obj, DX11RenderContext^ context)
        : m_obj(obj), m_context(context), type(UNKNOWN), constant(false), 
        m_minTime(std::numeric_limits<float>::infinity()), m_maxTime(0)
    {
        this->setUpNodeRecursive(m_obj, context);
    }

    abcrGeom::~abcrGeom()
    {
        m_children.clear();

        if (m_obj) m_obj.reset();
    }

    void abcrGeom::setUpNodeRecursive(IObject obj, DX11RenderContext^ context)
    {
        size_t nChildren = obj.getNumChildren();

        for (size_t i = 0; i < nChildren; ++i)
        {
            const ObjectHeader& head = obj.getChildHeader(i);

            shared_ptr<abcrGeom> geom;

            if (AbcGeom::IXform::matches(head))
            {
                AbcGeom::IXform xform(obj.getChild(i));
                geom.reset( new abcr::XForm(xform, context));
            }
            else if (AbcGeom::IPoints::matches(head))
            {
                AbcGeom::IPoints points(obj.getChild(i));
                geom.reset( new abcr::Points(points, context));
            }
            else if (AbcGeom::ICurves::matches(head))
            {
                AbcGeom::ICurves curves(obj.getChild(i));
                geom.reset( new abcr::Curves(curves, context));
            }
            else if (AbcGeom::IPolyMesh::matches(head))
            {
                AbcGeom::IPolyMesh pmesh(obj.getChild(i));
                geom.reset( new abcr::PolyMesh(pmesh, context));
            }
            else if (AbcGeom::ICamera::matches(head))
            {
                AbcGeom::ICamera camera(obj.getChild(i));
                geom.reset( new abcr::Camera(camera, context));
            }
            else
            {
                geom.reset( new abcrGeom(obj.getChild(i), context));
            }

            if (geom && geom->valid())
            {
                geom->index = m_children.size();
                this->m_children.emplace_back(geom);
                this->m_minTime = std::min(this->m_minTime, geom->m_minTime);
                this->m_maxTime = std::max(this->m_maxTime, geom->m_maxTime);
            }

        }
    }

    void abcrGeom::setUpDocRecursive(shared_ptr<abcrGeom>& obj, Dictionary<String^, abcrPtr>^% nameMap, Dictionary<String^, abcrPtr>^% fullnameMap)
    {
        if (!obj->isTypeOf(UNKNOWN))
        {
            nameMap[obj->getName()] = abcrPtr(obj.get());
            fullnameMap[obj->getFullName()] = abcrPtr(obj.get());
        }

        for (size_t i = 0; i < obj->m_children.size(); i++)
            setUpDocRecursive(obj->m_children[i], nameMap, fullnameMap);
    }


    template<typename T>
    void abcrGeom::setMinMaxTime(T& obj)
    {
        TimeSamplingPtr tptr = obj.getSchema().getTimeSampling();
        if (!obj.getSchema().isConstant())
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

    XForm::XForm(AbcGeom::IXform xform, DX11RenderContext^ context)
        : abcrGeom(xform, context), m_xform(xform)
    {
        type = XFORM;
        setMinMaxTime(m_xform);

        if (m_xform.getSchema().isConstant())
        {
            this->set(m_minTime, this->transform);
            this->constant = true;
        }
    }

    void XForm::set(chrono_t time, Imath::M44f& transform)
    {
        if (!this->constant)
        {
            ISampleSelector ss(time, ISampleSelector::kNearIndex);

            const Imath::M44d& m = m_xform.getSchema().getValue(ss).getMatrix();
            const double* src = m.getValue();
            float* dst = this->mat.getValue();

            for (size_t i = 0; i < 16; ++i) dst[i] = src[i];
        }

        transform =  this->mat * transform;
    }

    Points::Points(AbcGeom::IPoints points, DX11RenderContext^ context)
        : abcrGeom(points, context), m_points(points)
    {
        type = POINTS;
        setMinMaxTime(m_points);

        if (m_points.getSchema().isConstant())
        {
            this->set(this->m_minTime, this->transform);
            this->constant = true;
        }
    }

    void Points::set(chrono_t time, Imath::M44f& transform)
    {
        if (this->constant) return;

        AbcGeom::IPointsSchema ptSchema = m_points.getSchema();
        AbcGeom::IPointsSchema::Sample pts_sample;

        ISampleSelector ss(time, ISampleSelector::kNearIndex);

        ptSchema.get(pts_sample, ss);

        P3fArraySamplePtr m_positions = pts_sample.getPositions();

        size_t nPts = m_positions->size();

        if (static_cast<ISpread<Vector3D>^>(this->points) == nullptr)
        {
            points = gcnew Spread<Vector3D>(nPts);
        }
        else
        {
            this->points->SliceCount = nPts;
        }

        const V3f* src = m_positions->get();
        
        for (size_t i = 0; i < nPts; i++)
        {
            const V3f& v = src[i];
            static_cast<ISpread<Vector3D>^>(this->points)[i] = abcrUtils::toVVVV(v);
        }
    }

    Curves::Curves(AbcGeom::ICurves curves, DX11RenderContext^ context)
        : abcrGeom(curves, context), m_curves(curves)
    {
        type = CURVES;
        setMinMaxTime(m_curves);

        if (m_curves.getSchema().isConstant())
        {
            this->set(m_minTime, this->transform);
            this->constant = true;
        }
    }

    void Curves::set(chrono_t time, Imath::M44f& transform)
    {
        if (this->constant) return;

        AbcGeom::ICurvesSchema curvSchema = m_curves.getSchema();
        AbcGeom::ICurvesSchema::Sample curve_sample;

        ISampleSelector ss(time, ISampleSelector::kNearIndex);

        curvSchema.get(curve_sample, ss);

        P3fArraySamplePtr m_positions = curve_sample.getPositions();

        size_t nCurves = curve_sample.getNumCurves();
        const V3f* src = m_positions->get();

        
        if (static_cast<ISpread<ISpread<Vector3D>^>^>(this->curves) == nullptr)
        {
            curves = gcnew Spread<ISpread<Vector3D>^>(nCurves);
        }
        else
        {
            this->curves->SliceCount = nCurves;
        }

        const Alembic::Util::int32_t* nVertices = curve_sample.getCurvesNumVertices()->get();

        for (int i = 0; i < nCurves; ++i)
        {
            const int num = nVertices[i];
            ISpread<Vector3D>^ cp = gcnew Spread<Vector3D>(num);

            for (int j = 0; j < num; ++j)
            {
                const V3f& v = *src;
                cp[j] = abcrUtils::toVVVV(v);
                src++;
            }

            static_cast<ISpread<ISpread<Vector3D>^>^>(this->curves)[i] = cp;
        }
    }

    PolyMesh::PolyMesh(AbcGeom::IPolyMesh pmesh, DX11RenderContext^ context)
        : abcrGeom(pmesh, context), m_polymesh(pmesh), hasRGB(false), hasRGBA(false)
    {
        type = POLYMESH;
        setMinMaxTime(m_polymesh);

        this->vertexSize = Pos3Norm3Tex2Vertex::VertexSize;
        this->Layout = Pos3Norm3Tex2Vertex::Layout;

        auto topo = m_polymesh.getSchema().getTopologyVariance();
        IsChangeTopo = topo == 2;

        auto geomParam = m_polymesh.getSchema().getArbGeomParams();

        if (geomParam.valid())
        {
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
                else if (AbcGeom::IC4fGeomParam::matches(head))
                {
                    hasRGBA = true;
                    vertexSize = Pos3Norm3Tex2Col4Vertex::VertexSize;
                    Layout = Pos3Norm3Tex2Col4Vertex::Layout;
                    m_rgba = AbcGeom::IC4fGeomParam(geomParam, head.getName());
                }
            }
        }

        this->geom = gcnew DX11VertexGeometry(this->m_context);

        if (m_polymesh.getSchema().isConstant()) 
        {
            this->set(m_minTime, this->transform);
            this->constant = true;
        }

    }

    void PolyMesh::set(chrono_t time, Imath::M44f& transform)
    {
        if (this->constant) return;

        if (static_cast<DX11VertexGeometry^>(this->geom) == nullptr)
        {
            this->geom = gcnew DX11VertexGeometry(this->m_context);
        }

        AbcGeom::IPolyMeshSchema mesh = m_polymesh.getSchema();
        AbcGeom::IPolyMeshSchema::Sample mesh_samp;

        ISampleSelector ss(time, ISampleSelector::kNearIndex);
        
        mesh.get(mesh_samp, ss);

        //sample some property
        P3fArraySamplePtr m_points = mesh_samp.getPositions();
        Int32ArraySamplePtr m_indices = mesh_samp.getFaceIndices();
        Int32ArraySamplePtr m_faceCounts = mesh_samp.getFaceCounts();

        AbcGeom::IN3fGeomParam N = mesh.getNormalsParam();
        N3fArraySamplePtr m_norms = N.getExpandedValue(ss).getVals();
        auto scope = N.getScope();
        bool isIndexedNorm = scope == AbcGeom::kVertexScope || scope == AbcGeom::kVaryingScope;

        AbcGeom::IV2fGeomParam UV = mesh.getUVsParam();
        AbcGeom::IV2fGeomParam::Sample uvValue;
        V2fArraySamplePtr m_uvs;
        uvValue = UV.getExpandedValue(ss);
        m_uvs = uvValue.getVals();
        scope = UV.getScope();
        bool isIndexedUV = scope == AbcGeom::kVertexScope || scope == AbcGeom::kVaryingScope;

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
        std::vector<int32_t> inds;

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
                    m_triangles.push_back(tri((unsigned int)fBegin + 0,
                        (unsigned int)fBegin + 1,
                        (unsigned int)fBegin + 2));
                    for (size_t c = 3; c < count; ++c)
                    {
                        m_triangles.push_back(tri((unsigned int)fBegin + 0,
                            (unsigned int)fBegin + c - 1,
                            (unsigned int)fBegin + c));
                    }
                }
            }
        }
        void *buffer;
        auto vertexStream = gcnew SlimDX::DataStream(m_triangles.size() * 3 * vertexSize, true, true);
        vertexStream->Position = 0;

        {
            const V3f* points = m_points->get();
            const N3f* norms = m_norms->get();
            const V2f* uvs = m_uvs->get();
            const int32_t* indices = m_indices->get();
            const auto uvInds = uvValue.getIndices()->get();
            
            if (hasRGB)
            {
                const AbcGeom::IC3fGeomParam::Sample m_col = m_rgb.getExpandedValue(ss);
                const C3f* cols = m_col.getVals()->get();
                auto scope = m_col.getScope();
                bool isIndexedColor = scope == AbcGeom::kVertexScope || scope == AbcGeom::kVaryingScope;
                for (size_t j = 0; j < m_triangles.size(); ++j)
                {
                    tri& t = m_triangles[j];

                    const V3f& v0 = points[indices[t[0]]];
                    const N3f& n0 = isIndexedNorm ? norms[indices[t[0]]] : norms[t[0]];
                    const V2f& uv0 = isIndexedUV ? uvs[indices[t[0]]] : uvs[t[0]];
                    const C3f& col0 = isIndexedColor ? cols[indices[t[0]]] : cols[t[0]];
                    const Pos3Norm3Tex2Col3Vertex& p0 = { abcrUtils::toSlimDX(v0) ,
                                                          abcrUtils::toSlimDX(n0) ,
                                                          abcrUtils::toSlimDX(uv0),
                                                          abcrUtils::toSlimDX(col0)};

                    vertexStream->Write(p0);

                    const V3f& v1 = points[indices[t[1]]];
                    const N3f& n1 = isIndexedNorm ? norms[indices[t[1]]] : norms[t[1]];
                    const V2f& uv1 = isIndexedUV ? uvs[indices[t[1]]] : uvs[t[1]];
                    const C3f& col1 = isIndexedColor ? cols[indices[t[1]]] : cols[t[1]];
                    const Pos3Norm3Tex2Col3Vertex& p1 = { abcrUtils::toSlimDX(v1) ,
                                                          abcrUtils::toSlimDX(n1) ,
                                                          abcrUtils::toSlimDX(uv1),
                                                          abcrUtils::toSlimDX(col1) };

                    vertexStream->Write(p1);

                    const V3f& v2 = points[indices[t[2]]];
                    const N3f& n2 = isIndexedNorm ? norms[indices[t[2]]] : norms[t[2]];
                    const V2f& uv2 = isIndexedUV ? uvs[indices[t[2]]] : uvs[t[2]];
                    const C3f& col2 = isIndexedColor ? cols[indices[t[2]]] : cols[t[2]];
                    const Pos3Norm3Tex2Col3Vertex& p2 = { abcrUtils::toSlimDX(v2) ,
                                                          abcrUtils::toSlimDX(n2) ,
                                                          abcrUtils::toSlimDX(uv2),
                                                          abcrUtils::toSlimDX(col2) };

                    vertexStream->Write(p2);
                }
            }
            else if (hasRGBA)
            {
                const AbcGeom::IC4fGeomParam::Sample m_col = m_rgba.getExpandedValue(ss);
                const C4f* cols = m_col.getVals()->get();
                auto scope = m_col.getScope();
                bool isIndexedColor = scope == AbcGeom::kVertexScope || scope == AbcGeom::kVaryingScope;
                for (size_t j = 0; j < m_triangles.size(); ++j)
                {
                    tri& t = m_triangles[j];

                    const V3f& v0 = points[indices[t[0]]];
                    const N3f& n0 = isIndexedNorm ? norms[indices[t[0]]] : norms[t[0]];
                    const V2f& uv0 = isIndexedUV ? uvs[indices[t[0]]] : uvs[t[0]];
                    const C4f& col0 = isIndexedColor ? cols[indices[t[0]]] : cols[t[0]];
                    const Pos3Norm3Tex2Col4Vertex& p0 = { abcrUtils::toSlimDX(v0) ,
                                                          abcrUtils::toSlimDX(n0) ,
                                                          abcrUtils::toSlimDX(uv0),
                                                          abcrUtils::toSlimDX(col0) };

                    vertexStream->Write(p0);

                    const V3f& v1 = points[indices[t[1]]];
                    const N3f& n1 = isIndexedNorm ? norms[indices[t[1]]] : norms[t[1]];
                    const V2f& uv1 = isIndexedUV ? uvs[indices[t[1]]] : uvs[t[1]];
                    const C4f& col1 = isIndexedColor ? cols[indices[t[1]]] : cols[t[1]];
                    const Pos3Norm3Tex2Col4Vertex& p1 = { abcrUtils::toSlimDX(v1) ,
                                                          abcrUtils::toSlimDX(n1) ,
                                                          abcrUtils::toSlimDX(uv1),
                                                          abcrUtils::toSlimDX(col1) };

                    vertexStream->Write(p1);

                    const V3f& v2 = points[indices[t[2]]];
                    const N3f& n2 = isIndexedNorm ? norms[indices[t[2]]] : norms[t[2]];
                    const V2f& uv2 = isIndexedUV ? uvs[indices[t[2]]] : uvs[t[2]];
                    const C4f& col2 = isIndexedColor ? cols[indices[t[2]]] : cols[t[2]];
                    const Pos3Norm3Tex2Col4Vertex& p2 = { abcrUtils::toSlimDX(v2) ,
                                                          abcrUtils::toSlimDX(n2) ,
                                                          abcrUtils::toSlimDX(uv2),
                                                          abcrUtils::toSlimDX(col2) };

                    vertexStream->Write(p2);
                }
            }
            else
            {
                for (size_t j = 0; j < m_triangles.size(); ++j)
                {
                    tri& t = m_triangles[j];

                    const V3f& v0 = points[indices[t[0]]];
                    const N3f& n0 = isIndexedNorm ? norms[indices[t[0]]] : norms[t[0]];
                    const V2f& uv0 = isIndexedUV ? uvs[indices[t[0]]] : uvs[t[0]];
                    const Pos3Norm3Tex2Vertex& p0 = { abcrUtils::toSlimDX(v0) ,
                                                      abcrUtils::toSlimDX(n0) ,
                                                      abcrUtils::toSlimDX(uv0) };

                    vertexStream->Write(p0);

                    const V3f& v1 = points[indices[t[1]]];
                    const N3f& n1 = isIndexedNorm ? norms[indices[t[1]]] : norms[t[1]];
                    const V2f& uv1 = isIndexedUV ? uvs[indices[t[1]]] : uvs[t[2]];
                    const Pos3Norm3Tex2Vertex& p1 = { abcrUtils::toSlimDX(v1) ,
                                                      abcrUtils::toSlimDX(n1) ,
                                                      abcrUtils::toSlimDX(uv1) };

                    vertexStream->Write(p1);

                    const V3f& v2 = points[indices[t[2]]];
                    const N3f& n2 = isIndexedNorm ? norms[indices[t[2]]] : norms[t[2]];
                    const V2f& uv2 = isIndexedUV ? uvs[indices[t[2]]] : uvs[t[2]];
                    const Pos3Norm3Tex2Vertex& p2 = { abcrUtils::toSlimDX(v2) ,
                                                      abcrUtils::toSlimDX(n2) ,
                                                      abcrUtils::toSlimDX(uv2) };

                    vertexStream->Write(p2);
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

            delete this->geom;

            this->geom->Topology = PrimitiveTopology::TriangleList;
            this->geom->HasBoundingBox = false;
            this->geom->VerticesCount = m_triangles.size() * 3;
            this->geom->InputLayout = Layout;
            this->geom->VertexSize = vertexSize;
            this->geom->VertexBuffer = vertexBuffer;
        }

        delete vertexStream;

    }

    Camera::Camera(AbcGeom::ICamera camera, DX11RenderContext^ context)
        : abcrGeom(camera, context), m_camera(camera)
    {
        type = CAMERA;
        setMinMaxTime(m_camera);

        if (camera.getSchema().isConstant())
        {
            this->set(m_minTime, this->transform);
            this->constant = true;
        }
    }

    void Camera::set(chrono_t time, Imath::M44f& transform)
    {
        if (!this->constant)
        {
            ISampleSelector ss(time, ISampleSelector::kNearIndex);

            AbcGeom::CameraSample cam_samp;
            AbcGeom::ICameraSchema camSchema = m_camera.getSchema();

            camSchema.get(cam_samp);

            float Aperture = cam_samp.getVerticalAperture();
            float Near = std::max(cam_samp.getNearClippingPlane(), .001);
            float Far = std::min(cam_samp.getFarClippingPlane(), 100000.0);
            float ForcalLength = cam_samp.getFocalLength();
            float FoV = 2.0 * (atan(Aperture * 10.0 / (2.0 * ForcalLength))) * VMath::RadToDeg;

            this->VP.Proj = VMath::PerspectiveLH(FoV * VMath::DegToCyc, Near, Far, 1.0);
        }

        this->VP.View = VMath::Inverse(VMath::RotateY(VMath::Pi) * abcrUtils::toVVVV(transform));
    }
    
}
}
}