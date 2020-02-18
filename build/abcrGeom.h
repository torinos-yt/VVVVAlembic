#pragma once
#include <Alembic\Abc\All.h>
#include <Alembic\AbcGeom\IPolyMesh.h>
#include <Alembic\AbcGeom\IPoints.h>
#include <Alembic\AbcGeom\ICurves.h>
#include <Alembic\AbcGeom\IXForm.h>

using namespace Alembic;
using namespace Alembic::Abc;

namespace VVVV
{
namespace Nodes
{
namespace abcr
{

    class XForm;
    class Points;
    class Curves;
    class PolyMesh;

    ref class VVVVAlembicReader;

    enum Type
    {
        POINTS = 0,
        CURVES,
        POLYMESH,
        CAMERA,
        XFORM,
        UNKNOWN
    };

    template <typename T>
    inline Type type2enum() { return UNKNOWN; }

    template <>
    inline Type type2enum<XForm>() { return XFORM; }

    template <>
    inline Type type2enum<Points>() { return POINTS; }

    template <>
    inline Type type2enum<Curves>() { return CURVES; }

    template <>
    inline Type type2enum<PolyMesh>() { return POLYMESH; }

    class abcrGeom
    {
        friend ref class VVVVAlembicReader;

    public:

        abcrGeom();
        abcrGeom(IObject obj);
        virtual ~abcrGeom();

        virtual bool valid() const { return m_obj; };

        size_t getIndex() const { return index; };

        string getName() const;

        IObject getIObject() const { return m_obj; };

        virtual const char* getTypeName() const { return ""; };

        inline bool isTypeOf(Type t) const { return type == t; };

        template<typename T>
        inline bool isTypeOf() const { return type == type2enum<T>(); };

        template <typename T>
        inline bool get(T& out)
        {
            FLogger->Log(LogType::Debug, "invalid type");
            return false;
        }

        abcrGeom* newChild(const IObject& obj);

    protected:

        Type type;

        size_t index;

        IObject m_obj;
        std::vector<std::unique_ptr<abcrGeom>> m_children;

        void updateSample(const ISampleSelector& ss);

    private:

        static void setUpNodeRecursive(abcrGeom* obj);
    };

    class XForm : public abcrGeom
    {
        public:

            XForm(AbcGeom::IXform xform);
    };

    class Points : public abcrGeom
    {
    public:

        Points(AbcGeom::IPoints points);
    };

    class Curves : public abcrGeom
    {
    public:

        Curves(AbcGeom::ICurves curves);
    };

    class PolyMesh : public abcrGeom
    {
    public:

        PolyMesh(AbcGeom::IPolyMesh pmesh);
    };

}
}
}
