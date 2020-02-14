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
						const ObjectHeader &head = m_object.getChildHeader(i);
						if (IXform::matches(head))
						{
							IXform xform(m_object, head.getName());
							IObject child = xform.getChild(0);
							string name = child.getName();
							if (IPolyMesh::matches(xform.getChildHeader(0)))
							{
								IPolyMesh pmesh(child);
								IPolyMeshSchema mesh = pmesh.getSchema();
								MeshTopologyVariance top = mesh.getTopologyVariance();
								FLogger->Log(LogType::Debug, marshal_as<String^>(ToporogyArray[top]));

								IPolyMeshSchema::Sample mesh_samp;
								mesh.get(mesh_samp);

								P3fArraySamplePtr psp = mesh_samp.getPositions();

								FLogger->Log(LogType::Debug, psp->size().ToString());
							}
						}
					}
				}

				FLogger->Log(LogType::Debug, "Alembic : ver. " + marshal_as<String^>(Alembic::Abc::GetLibraryVersionShort()));


			}


		}

		void VVVVAlembicReader::Update(DX11RenderContext^ context)
		{

		}

		void VVVVAlembicReader::Destroy(DX11RenderContext^ context, bool force)
		{

		}

	}
}