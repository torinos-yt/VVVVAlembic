#include "stdafx.h"

#include "VVVVAlembic.h"

#include <msclr\marshal_cppstd.h>
using namespace msclr::interop;

namespace VVVV
{
	namespace Nodes
	{
		void VVVVAlembicReader::Evaluate(int SpreadMax)
		{
			if (!FOutgeo[0])
				FOutgeo[0] = gcnew DX11Resource<DX11IndexedGeometry^>();

			if ((FPath->IsChanged || FReload[0]))
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
			}
		}

		void VVVVAlembicReader::Update(DX11RenderContext^ context)
		{
			Device^ device = context->Device;
			DeviceContext^ deviceContext = context->CurrentDeviceContext;

			if ((m_archive->valid() && FTime->IsChanged) || FReload[0] 
				 || (FFirst && m_archive->valid()) )
			{
				delete FOutgeo[0][context];
				auto geom = gcnew DX11IndexedGeometry(context);

				IObject m_object = m_archive->getTop();
				size_t numChild = m_object.getNumChildren();

				for (size_t i = 0; i < numChild; ++i)
				{
					const ObjectHeader &head = m_object.getChildHeader(i);
					if (IXform::matches(head))
					{
						IXform xform(m_object, head.getName());
						IObject child = xform.getChild(0);
						string name = child.getName();
						if (IPolyMesh::matches(xform.getChildHeader(0)))
						{
							ISpread<float>^ time = FTime;
							ISampleSelector ss(time[0], ISampleSelector::kNearIndex);

							m_mesh = new IPolyMesh(child);
							IPolyMeshSchema mesh = m_mesh->getSchema();
							MeshTopologyVariance top = mesh.getTopologyVariance();

							IPolyMeshSchema::Sample mesh_samp;
							mesh.get(mesh_samp, ss);

							//sample some property
							P3fArraySamplePtr psp = mesh_samp.getPositions();
							Int32ArraySamplePtr isp = mesh_samp.getFaceIndices();
							Int32ArraySamplePtr csp = mesh_samp.getFaceCounts();

							IN3fGeomParam N = mesh.getNormalsParam();
							N3fArraySamplePtr nsp = N.getExpandedValue(ss).getVals();

							IV2fGeomParam UV = mesh.getUVsParam();
							auto uvValue = UV.getIndexedValue(ss);
							V2fArraySamplePtr usp = uvValue.getVals();

							size_t nPts = psp->size();
							size_t nInds = isp->size();
							size_t nMesh = csp->size();

							if (nPts < 1 || nInds < 1 || nMesh < 1) return;

							#pragma region unfold index for quad & n-gon to triArray
							using tri = Imath::Vec3<unsigned int>;
							using triArray = std::vector<tri>;

							triArray m_triangles;

							size_t fBegin = 0;
							size_t fEnd = 0;
							for (size_t face = 0; face < nMesh; ++face)
							{
								fBegin = fEnd;
								size_t count = (*csp)[face];
								fEnd = fBegin + count;

								if (fEnd > nInds || fEnd < fBegin)
								{
									FLogger->Log(LogType::Debug, "Mesh update failed on face");
									break;
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
							#pragma endregion

							if (vertexStream != nullptr) delete vertexStream;
							vertexStream = gcnew DataStream(m_triangles.size() * 3 * vertexSize, true, true);

							if (indexStream != nullptr) delete indexStream;
							indexStream = gcnew DataStream(m_triangles.size() * 3 * 4, true, true);

							vertexStream->Position = 0;
							indexStream->Position = 0;

							const V3f* points = psp->get();
							const N3f* norms = nsp->get();
							const V2f* uvs = usp->get();
							const int32_t* indices = isp->get();
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
								indexStream->Write(indices[t[0]]);

								const V3f& v1 = points[indices[t[1]]];
								const N3f& n1 = norms[t[1]];
								const V2f& uv1 = uvs[uvInds[t[1]]];
								const Pos3Norm3Tex2Vertex& p1 = { SlimDX::Vector3(v1.x, v1.y, v1.z) ,
																  SlimDX::Vector3(n1.x, n1.y, n1.z) ,
																  SlimDX::Vector2(uv1.x, uv1.y) };

								vertexStream->Write(p1);
								indexStream->Write(indices[t[1]]);

								const V3f& v2 = points[indices[t[2]]];
								const N3f& n2 = norms[t[2]];
								const V2f& uv2 = uvs[uvInds[t[2]]];
								const Pos3Norm3Tex2Vertex& p2 = { SlimDX::Vector3(v2.x, v2.y, v2.z) ,
																  SlimDX::Vector3(n2.x, n2.y, n2.z) ,
																  SlimDX::Vector2(uv2.x, uv2.y) };

								vertexStream->Write(p2);
								indexStream->Write(indices[t[2]]);
							}
							vertexStream->Position = 0;
							indexStream->Position = 0;

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
							geom->IndexBuffer = gcnew DX11IndexBuffer(context, indexStream, false, false);
							geom->VertexBuffer = vertexBuffer;

							delete m_mesh;
						}
					}
				}

				if(FFirst) FFirst = false;

				FOutgeo[0][context] = geom;
			}

			if (!FPath->IsConnected) FOutgeo->SliceCount = 0;
		}

		void VVVVAlembicReader::Destroy(DX11RenderContext^ context, bool force)
		{

		}

	}
}