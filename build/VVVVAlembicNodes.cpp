#pragma once
#include "VVVVAlembicNodes.h"

#include <msclr\marshal_cppstd.h>
using namespace msclr::interop;

namespace VVVV
{
namespace Nodes
{

    void abcr::VVVVAlembicReader::Evaluate(int SpreadMax)
    {
        if (!FOutgeo[0])
            FOutgeo[0] = gcnew DX11Resource<IDX11Geometry^>();

        if (FFirst) FPath->Sync();

        String^ prevPath = FPath[0];
        FPath->Sync();
        if ( (FPath[0] != prevPath) || FReload[0] || (FFirst && FPath[0] != "") )
        {
            try
            {
                delete m_archive;
                m_archive = new IArchive(AbcCoreOgawa::ReadArchive(), marshal_as<string>(FPath[0]),
                                        Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
                FLogger->Log(LogType::Debug, "Success : Load Alembic Ogawa");
            }
            catch (Alembic::Util::Exception e)
            {
                FLogger->Log(LogType::Debug, "Failed : HDF5 or illigal");
                FLogger->Log(LogType::Debug, marshal_as<String^>(e.what()));
            }

            if (m_archive->valid())
            {
                IObject m_object = m_archive->getTop();
                size_t numChild = m_object.getNumChildren();
                for (size_t i = 0; i < numChild; ++i)
                {
                    const ObjectHeader& head = m_object.getChildHeader(i);
                    if (IXform::matches(head))
                    {
                        IXform xform(m_object, head.getName());
                        IObject child = xform.getChild(0);
                        string name = child.getName();
                        if (IPolyMesh::matches(xform.getChildHeader(0)))
                        {
                            m_mesh = new IPolyMesh(child);
                        }
                    }
                }
            }
        }
    }

    void abcr::VVVVAlembicReader::Update(DX11RenderContext^ context)
    {
        Device^ device = context->Device;
        DeviceContext^ deviceContext = context->CurrentDeviceContext;

        if ( (FTime->IsChanged || FReload[0] || FFirst) && m_mesh->valid() )
        {
            delete FOutgeo[0][context];
            auto geom = gcnew DX11VertexGeometry(context);

            ISpread<float>^ time = FTime;
            ISampleSelector ss(time[0], ISampleSelector::kNearIndex);

            IPolyMeshSchema mesh = m_mesh->getSchema();
            IPolyMeshSchema::Sample mesh_samp;
            try
            {
                mesh.get(mesh_samp, ss);
            }
            catch (Alembic::Util::Exception e)
            {
                FOutgeo[0][context] = (IDX11Geometry^)geom;
                FMessage[0] = "Not Support Static Geometry";
                FLogger->Log(LogType::Debug, "Not Support Static Geometry");
                FLogger->Log(LogType::Debug, marshal_as<String^>(e.what()));
                return;
            }

            //sample some property
            P3fArraySamplePtr m_points = mesh_samp.getPositions();
            Int32ArraySamplePtr m_indices = mesh_samp.getFaceIndices();
            Int32ArraySamplePtr m_faceCounts = mesh_samp.getFaceCounts();

            bool IsIndexedNormal = false;
            IN3fGeomParam N = mesh.getNormalsParam();
            if (N.isIndexed())
            {
                IsIndexedNormal = true;
                FMessage[0] = "Not Support : Indexed Normal";
                FLogger->Log(LogType::Debug, "Not Support : Indexed Normal");
                return;
            }
            N3fArraySamplePtr m_norms = N.getExpandedValue(ss).getVals();

            bool IsIndexedUV = false;
            IV2fGeomParam UV = mesh.getUVsParam();
            IV2fGeomParam::Sample uvValue;
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
                FOutgeo[0][context] = (IDX11Geometry^)geom;
                FMessage[0] = "Not Valid Geometry";
                FLogger->Log(LogType::Debug, "Not Valid Geometry");
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
                        FLogger->Log(LogType::Debug, "Mesh update failed on face");
                        break;
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

            if (vertexStream != nullptr) delete vertexStream;
            vertexStream = gcnew DataStream(m_triangles.size()*3*vertexSize, true, true);
            vertexStream->Position = 0;

            {
                const V3f* points = m_points->get();
                const N3f* norms = m_norms->get();
                const V2f* uvs = m_uvs->get();
                const int32_t* indices = m_indices->get();
                if (IsIndexedUV)
                {
                    const auto uvInds = uvValue.getIndices()->get();
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
                geom->InputLayout = Pos3Norm3Tex2Vertex::Layout;
                geom->VertexSize = vertexSize;
                geom->VertexBuffer = vertexBuffer;
            }

            FMessage[0] = "Name : " + marshal_as<String^>(m_mesh->getName());


            FOutgeo[0][context] = (IDX11Geometry^)geom;
        }

        if (FPath[0] == "")
        {
            FOutgeo->SliceCount = 0;
            FMessage->SliceCount = 0;
        }

        if (FFirst) FFirst = false;
    }

    void abcr::VVVVAlembicReader::Destroy(DX11RenderContext^ context, bool force)
    {
        delete FOutgeo;
        delete vertexStream;
        m_archive->reset();
        m_mesh->reset();
        delete m_archive;
        delete m_mesh;
        delete m_root;
        GC::SuppressFinalize(this);
    }

    void abcr::VVVVAlembicScene::Evaluate(int SpreadMax)
    {
        if (FFirst) FPath->Sync();

        String^ prevPath = FPath[0];
        FPath->Sync();

        //load file
        if ((FPath[0] != prevPath) || FReload[0] || (FFirst && FPath[0] != String::Empty)) 
        {
            this->RenderRequest(this, this->FHost);

            FOutScene->SliceCount = 1;
            if (FOutScene[0].m_Scene) delete FOutScene[0].m_Scene;

            AbcScene s;
            s.m_Scene = new abcrScene();
            FOutScene[0] = s;

            try
            {
                if (FOutScene[0].m_Scene->open(marshal_as<string>(FPath[0]), this->AssignedContext))
                {
                    FLogger->Log(LogType::Debug, "Success Open");
                    
                    FDulation->SliceCount = 1;
                    FDulation[0] = FOutScene[0].m_Scene->getMaxTime();

                    FOutScene[0].m_Scene->getFullNameMap(FNames);
                }
                else
                {
                    FLogger->Log(LogType::Debug, "Failed Open : Illigal Format");
                    FOutScene->SliceCount = 0;
                    FDulation->SliceCount = 0;
                    FNames->SliceCount = 0;
                }
            }
            catch(System::Exception^ e)
            {
                FLogger->Log(LogType::Debug, "Failed Open : " + e->Message);
            }

            FOutScene->Stream->IsChanged = true;
        }
        else if (SpreadMax == 0 || FPath->SliceCount == 0 || FPath[0] == String::Empty)
        {
            if (FOutScene[0].m_Scene) delete FOutScene[0].m_Scene;

            FDulation->SliceCount = 0;
            FNames->SliceCount = 0;

            return;
        }

        //update
        if (((FPath[0] != prevPath) || FTime->IsChanged || FReload[0] || FFirst) 
            && FOutScene->SliceCount > 0)
        {
            if (FOutScene[0].m_Scene)
            {
                try
                {
                    if(FOutScene[0].m_Scene->updateSample((chrono_t)(((ISpread<float>^)FTime)[0])))
                        FOutScene->Stream->IsChanged = true;
                }
                catch (Alembic::Util::Exception e)
                {
                    FLogger->Log(LogType::Debug, "Failed Update : " + marshal_as<String^>(e.what()));
                }
            }
        }

        FFirst = false;
    }

    void abcr::VVVVAlembicPoint::Evaluate(int SpreadMax)
    {
        if ((FInScene->Stream->IsChanged || FFirst) && FInScene[0].m_Scene)
        {
            if (FInScene[0].m_Scene->valid())
            {
                size_t cnt = 0;
                SpreadExtensions::RemoveRange(FOutPoints, 0, FOutPoints->SliceCount);
                SpreadExtensions::RemoveRange(FOutMat, 0, FOutMat->SliceCount);
                SpreadExtensions::RemoveRange(FNames, 0, FNames->SliceCount);
                for each (auto pts in FInScene[0].m_Scene->getGeomIterator())
                {
                    if (pts.m_ptr->isTypeOf(POINTS))
                    {
                        SpreadExtensions::Add(FOutMat, pts.m_ptr->getTransform());
                        SpreadExtensions::Add(FNames, pts.m_ptr->getName());

                        SpreadExtensions::Add(FOutPoints, static_cast<ISpread<Vector3D>^>(gcnew Spread<Vector3D>()));
                        pts.m_ptr->get(FOutPoints[cnt++]);

                    }
                }

                FOutPoints->SliceCount = cnt;
                FOutMat->SliceCount = cnt;
                FNames->SliceCount = cnt;

            }
        }

        if (FInScene->IsConnected) FFirst = false;
    }

    void abcr::VVVVAlembicCurve::Evaluate(int SpreadMax)
    {
        if ((FInScene->Stream->IsChanged || FFirst) && FInScene[0].m_Scene)
        {
            if (FInScene[0].m_Scene->valid())
            {
                size_t cnt = 0;
                SpreadExtensions::RemoveRange(FOutPoints, 0, FOutPoints->SliceCount);
                SpreadExtensions::RemoveRange(FOutMat, 0, FOutMat->SliceCount);
                SpreadExtensions::RemoveRange(FNames, 0, FNames->SliceCount);
                SpreadExtensions::RemoveRange(FOutCnt, 0, FOutCnt->SliceCount);
                SpreadExtensions::RemoveRange(FOutIndex, 0, FOutIndex->SliceCount);

                for each (auto curves in FInScene[0].m_Scene->getGeomIterator())
                {
                    if (curves.m_ptr->isTypeOf(CURVES))
                    {
                        SpreadExtensions::Add(FOutMat, curves.m_ptr->getTransform());
                        SpreadExtensions::Add(FNames, curves.m_ptr->getName());
                        SpreadExtensions::Add(FOutCnt, ((Curves*)curves.m_ptr)->getCurveCount());

                        curves.m_ptr->get(FOutPoints);
                        ((Curves*)curves.m_ptr)->getIndexSpread(FOutIndex);
                        cnt++;
                    }
                }
                FOutMat->SliceCount = cnt;
                FNames->SliceCount = cnt;

            }
        }

        if (FInScene->IsConnected) FFirst = false;
    }

    void abcr::VVVVAlembicPolyMesh::Evaluate(int SpreadMax)
    {
        if (!FOutGeo[0]) FOutGeo[0] = gcnew DX11Resource<DX11VertexGeometry^>();

        if ((FInScene->Stream->IsChanged || FFirst) && FInScene[0].m_Scene)
        {
            if (FInScene[0].m_Scene->valid())
            {
                if (FOutGeo->SliceCount == 0)
                {
                    FOutGeo = gcnew Spread<DX11Resource<DX11VertexGeometry^>^>();
                    FOutMat = gcnew Spread<Matrix4x4>();
                    FNames = gcnew Spread<String^>();
                }

                size_t cnt = 0;
                for each (auto geom in FInScene[0].m_Scene->getGeomIterator())
                {
                    if (geom.m_ptr->isTypeOf(POLYMESH))
                    {
                        FOutMat[cnt] = geom.m_ptr->getTransform();
                        FNames[cnt] = geom.m_ptr->getName();

                        if (!FOutGeo[cnt])
                            FOutGeo[cnt] = gcnew DX11Resource<DX11VertexGeometry^>();

                        geom.m_ptr->get(FOutGeo[cnt++]);

                    }
                }

                FOutGeo->SliceCount = cnt;
                FOutMat->SliceCount = cnt;
                FNames->SliceCount = cnt;

            }
        }

        if (FInScene->IsConnected) FFirst = false;
    }

    void abcr::VVVVAlembicPolyMesh::Update(DX11RenderContext^ context)
    {
    }

    void abcr::VVVVAlembicPolyMesh::Destroy(DX11RenderContext^ context, bool force)
    {
    }

    void abcr::VVVVAlembicCamera::Evaluate(int SpreadMax)
    {
        if ((FInScene->Stream->IsChanged || FFirst) && FInScene[0].m_Scene)
        {
            if (FInScene[0].m_Scene->valid())
            {


                size_t cnt = 0;
                SpreadExtensions::RemoveRange(FOutView, 0, FOutView->SliceCount);
                SpreadExtensions::RemoveRange(FOutProj, 0, FOutProj->SliceCount);
                SpreadExtensions::RemoveRange(FNames, 0, FNames->SliceCount);
                for each (auto cam in FInScene[0].m_Scene->getGeomIterator())
                {
                    if (cam.m_ptr->isTypeOf(CAMERA))
                    {
                        ViewProj VP;

                        SpreadExtensions::Add(FNames, cam.m_ptr->getName());

                        cam.m_ptr->get(VP);
                        SpreadExtensions::Add(FOutView, VP.View);
                        SpreadExtensions::Add(FOutProj, VP.Proj);

                        cnt++;
                    }
                }

                FOutView->SliceCount = cnt;
                FOutProj->SliceCount = cnt;
                FNames->SliceCount = cnt;
            }
        }

        if (FInScene->IsConnected) FFirst = false;
    }

}
}