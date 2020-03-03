#pragma once
#include <gcroot.h>
#include <numeric>

#include <Alembic\Abc\All.h>
#include <Alembic\AbcGeom\IPolyMesh.h>
#include <Alembic\AbcGeom\IPoints.h>
#include <Alembic\AbcGeom\ICurves.h>
#include <Alembic\AbcGeom\IXForm.h>
#include <Alembic\AbcGeom\ICamera.h>

#include "abcrUtils.h"
#include "abcrlayout.h"

#include <msclr\marshal_cppstd.h>
using namespace msclr::interop;

using namespace Alembic;
using namespace Alembic::Abc;

using namespace VVVV::PluginInterfaces::V1;
using namespace VVVV::PluginInterfaces::V2;

using namespace VVVV::Utils::VMath;

using namespace VVVV::DX11;

using namespace SlimDX;
using namespace SlimDX::Direct3D11;

using namespace System;
using namespace System::Collections::Generic;
using namespace std;

using DX11Buffer = SlimDX::Direct3D11::Buffer;

using namespace FeralTic::DX11;
using namespace FeralTic::DX11::Resources;
using namespace FeralTic::DX11::Geometry;

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
    class Camera;

    class abcrScene;

    value struct abcrPtr;

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

    template <>
    inline Type type2enum<Camera>() { return CAMERA; }

    class abcrGeom
    {
        friend class abcrScene;

    public:

        abcrGeom();
        abcrGeom(IObject obj, DX11RenderContext^ context);
        virtual ~abcrGeom();

        virtual bool valid() const { return m_obj; };

        size_t getIndex() const { return index; };

        String^ getName() const { return marshal_as<String^>( m_obj.getName() ); };

        String^ getFullName() const { return marshal_as<String^>( m_obj.getFullName() ); };

        IObject getIObject() const { return m_obj; };

        Matrix4x4 getTransform() const { return abcrUtils::toVVVV(transform); };

        virtual const char* getTypeName() const { return ""; };

        inline bool isTypeOf(Type t) const { return type == t; };

        template<typename T>
        inline bool isTypeOf() const { return type == type2enum<T>(); };

        template <typename T>
        inline bool get(T% out)
        {
            return false;
        }

        void setUpNodeRecursive(IObject obj, DX11RenderContext^ context);
        static void setUpDocRecursive(shared_ptr<abcrGeom>& obj, Dictionary<String^, abcrPtr>^% nameMap, Dictionary<String^, abcrPtr>^% fullnameMap);

    protected:

        Type type;

        size_t index;

        chrono_t m_minTime;
        chrono_t m_maxTime;

        Imath::M44f transform;

        bool constant;

        IObject m_obj;
        vector<shared_ptr<abcrGeom>> m_children;
        gcroot<DX11RenderContext^> m_context;

        virtual void updateTimeSample(chrono_t time, Imath::M44f& transform);
        virtual void set(chrono_t time, Imath::M44f& transform) {};

        template<typename T>
        void setMinMaxTime(T& obj);
    };

    class XForm : public abcrGeom
    {
        public:

            Imath::M44f mat;

            XForm(AbcGeom::IXform xform, DX11RenderContext^ context);
            ~XForm()
            {
                if (m_xform) m_xform.reset();
            }

            void set(chrono_t time, Imath::M44f& transform) override;

        private:

            AbcGeom::IXform m_xform;

    };

    class Points : public abcrGeom
    {
    public:

        gcroot<ISpread<Vector3D>^> points;

        Points(AbcGeom::IPoints points, DX11RenderContext^ context);
        ~Points() 
        {
            if (m_points) m_points.reset();
            if (static_cast<ISpread<Vector3D>^>(this->points) == nullptr)
                delete this->points;
        }

        void set(chrono_t time, Imath::M44f& transform) override;

    private:

        AbcGeom::IPoints m_points;
    };

    class Curves : public abcrGeom
    {
    public:

        gcroot<ISpread<ISpread<Vector3D>^>^> curves;

        Curves(AbcGeom::ICurves curves, DX11RenderContext^ context);
        ~Curves()
        {
            if (m_curves) m_curves.reset();
            if (static_cast<ISpread<ISpread<Vector3D>^>^>(this->curves) != nullptr)
                delete this->curves;
        }

        void set(chrono_t time, Imath::M44f& transform) override;

    private:

        AbcGeom::ICurves m_curves;
    };

    class PolyMesh : public abcrGeom
    {
    public:

        gcroot<DX11VertexGeometry^> geom;

        PolyMesh(AbcGeom::IPolyMesh pmesh, DX11RenderContext^ context);
        ~PolyMesh()
        {
            if (m_polymesh) m_polymesh.reset();
            if (static_cast<DX11VertexGeometry^>(this->geom) != nullptr)
                delete this->geom;
        }

        const char* getTypeNmae() const { return "PolyMesh"; }

        void set(chrono_t time, Imath::M44f& transform) override;

    private:

        bool hasRGB;
        bool hasRGBA;

        AbcGeom::IPolyMesh m_polymesh;
        AbcGeom::IC3fGeomParam m_rgb;
        AbcGeom::IC4fGeomParam m_rgba;
        
        size_t vertexSize;
        gcroot<cli::array<InputElement>^> Layout;
    };

    class Camera : public abcrGeom
    {
    public:
        Matrix4x4 view;
        Matrix4x4 proj;

        Camera(AbcGeom::ICamera camera, DX11RenderContext^ context);
        ~Camera()
        {
            if (m_camera) m_camera.reset();
        }

    protected:

        void set(chrono_t time, Imath::M44f& transform) override;

    private:

        AbcGeom::ICamera m_camera;
        AbcGeom::CameraSample m_sample;

    };

    value struct abcrPtr
    {
        abcrGeom* m_ptr;

        abcrPtr(abcrGeom* ptr) : m_ptr(ptr) {}
    };

    template <>
    inline bool abcrGeom::get(DX11Resource<DX11VertexGeometry^>^% o)
    {
        if (type != POLYMESH) return false;

        o[this->m_context] = static_cast<DX11VertexGeometry^>( ((PolyMesh*)this)->geom->ShallowCopy() );
        return true;
    }

}
}
}
