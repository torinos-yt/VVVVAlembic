#pragma once
#include <Alembic\Abc\All.h>
#include <Alembic\AbcGeom\IPolyMesh.h>

namespace VVVV
{
    namespace abcr
    {
        public class abcrGeom
        {
            public :
                
                abcrGeom();
                abcrGeom(Alembic::Abc::IObject obj);
                virtual ~abcrGeom();

                virtual bool valid() { return m_obj; };


            protected :

                Alembic::Abc::IObject m_obj;

        };
    }
}
