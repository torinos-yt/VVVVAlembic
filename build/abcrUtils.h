#pragma once

#include <Alembic\Abc\All.h>

namespace abcrUtils
{

    inline SlimDX::Vector2 toSlimDX(const Alembic::AbcGeom::V2f& v)
    {
        return SlimDX::Vector2(v.x, v.y);
    }

    inline SlimDX::Vector3 toSlimDX(const Alembic::AbcGeom::V3f& v)
    {
        return SlimDX::Vector3(v.x, v.y, v.z);
    }

    inline SlimDX::Vector3 toSlimDX(const Alembic::AbcGeom::C3f& v)
    {
        return SlimDX::Vector3(v.x, v.y, v.z);
    }

    inline SlimDX::Vector4 toSlimDX(const Alembic::AbcGeom::C4f& v)
    {
        return SlimDX::Vector4(v.r, v.g, v.b, v.a);
    }

    inline SlimDX::Vector3 toSlimDX(const VVVV::Utils::VMath::Vector3D& v)
    {
        return SlimDX::Vector3(v.x, v.y, v.z);
    }

    inline VVVV::Utils::VMath::Vector3D toVVVV(const Alembic::AbcGeom::V3f& v)
    {
        return VVVV::Utils::VMath::Vector3D(v.x, v.y, v.z);
    }

    inline VVVV::Utils::VMath::Vector3D toVVVV(const SlimDX::Vector3& v)
    {
        return VVVV::Utils::VMath::Vector3D(v.X, v.X, v.Z);
    }

    inline VVVV::Utils::VMath::Matrix4x4 toVVVV(const Imath::M44f& m)
    {
        return VVVV::Utils::VMath::Matrix4x4(m[0][0], m[1][0], m[2][0], m[3][0],
            m[0][1], m[1][1], m[2][1], m[3][1],
            m[0][2], m[1][2], m[2][2], m[3][2],
            m[0][3], m[1][3], m[2][3], m[3][3]);
    }

}
