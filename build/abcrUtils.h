#pragma once

#include <Alembic\Abc\All.h>

namespace abcrUtils
{

    inline SlimDX::Vector2 toSlimDX(const Alembic::Abc::V2f& v)
    {
        return SlimDX::Vector2(v.x, v.y);
    }

    inline SlimDX::Vector3 toSlimDX(const Alembic::Abc::V3f& v)
    {
        return SlimDX::Vector3(-v.x, v.y, v.z);
    }

    inline SlimDX::Vector3 toSlimDX(const Alembic::Abc::V3d& v)
    {
        return SlimDX::Vector3(-v.x, v.y, v.z);
    }

    inline SlimDX::Vector3 toSlimDX(const Alembic::Abc::C3f& v)
    {
        return SlimDX::Vector3(v.x, v.y, v.z);
    }

    inline SlimDX::Vector4 toSlimDX(const Alembic::Abc::C4f& v)
    {
        return SlimDX::Vector4(v.r, v.g, v.b, v.a);
    }

    inline SlimDX::Vector3 toSlimDX(const VVVV::Utils::VMath::Vector3D& v)
    {
        return SlimDX::Vector3(v.x, v.y, v.z);
    }

    inline SlimDX::Quaternion toSlimDX(const Imath::Quatd& q)
    {
        return SlimDX::Quaternion(-q.v[0], q.v[1], q.v[2], -q.r);
    }

    inline SlimDX::Matrix toSlimDX(const VVVV::Utils::VMath::Matrix4x4& m)
    {
        SlimDX::Matrix M;

        M.M11 = (float)m.m11;
        M.M12 = (float)m.m12;
        M.M13 = (float)m.m13;
        M.M14 = (float)m.m14;
        M.M21 = (float)m.m21;
        M.M22 = (float)m.m22;
        M.M23 = (float)m.m23;
        M.M24 = (float)m.m24;
        M.M31 = (float)m.m31;
        M.M32 = (float)m.m32;
        M.M33 = (float)m.m33;
        M.M34 = (float)m.m34;
        M.M41 = (float)m.m41;
        M.M42 = (float)m.m42;
        M.M43 = (float)m.m43;
        M.M44 = (float)m.m44;

        return M;
    }

    inline VVVV::Utils::VMath::Vector3D toVVVV(const Alembic::Abc::V3f& v)
    {
        return VVVV::Utils::VMath::Vector3D(-v.x, v.y, v.z);
    }

    inline VVVV::Utils::VMath::Vector3D toVVVV(const SlimDX::Vector3& v)
    {
        return VVVV::Utils::VMath::Vector3D(v.X, v.X, v.Z);
    }

    inline VVVV::Utils::VMath::Matrix4x4 toVVVV(const SlimDX::Matrix& m)
    {
        return VVVV::Utils::VMath::Matrix4x4(m.M11, m.M12, m.M13, m.M14,
            m.M21, m.M22, m.M23, m.M24,
            m.M31, m.M32, m.M33, m.M34,
            m.M41, m.M42, m.M43, m.M44);
    }

    inline SlimDX::Matrix toLH(const SlimDX::Matrix& m)
    {
        SlimDX::Matrix M;

        M.M11 = (float)m.M11;
        M.M12 = (float)m.M12;
        M.M13 = (float)m.M13;
        M.M14 = (float)m.M14;
        M.M21 = (float)m.M21;
        M.M22 = (float)m.M22;
        M.M23 = (float)m.M23;
        M.M24 = (float)m.M24;
        M.M31 = -(float)m.M31;
        M.M32 = -(float)m.M32;
        M.M33 = -(float)m.M33;
        M.M34 = (float)m.M34;
        M.M41 = (float)m.M41;
        M.M42 = (float)m.M42;
        M.M43 = (float)m.M43;
        M.M44 = (float)m.M44;

        return M;
    }

    inline VVVV::Utils::VMath::Matrix4x4 toVVVV(const Imath::M44f& m)
    {
        Imath::M44d mat(m);

        Imath::V3d transform;
        Imath::V3d scale;
        Imath::V3d shear;

        Imath::extractAndRemoveScalingAndShear(mat, scale, shear, false);

        transform.x = mat[3][0];
        transform.y = mat[3][1];
        transform.z = mat[3][2];
        transform *= scale;

        Imath::Quatd r = Imath::extractQuat(mat);
        auto rotation = toSlimDX(r);

        SlimDX::Matrix M = SlimDX::Matrix::Identity;

        M *= SlimDX::Matrix::RotationQuaternion(rotation);
        M *= SlimDX::Matrix::Translation(toSlimDX(transform));

        return toVVVV(M);
    }

}
