#pragma once

#include <common/matrix.tcc>
#include <common/object.hpp>
#include <common/vector.hpp>

#include <utilities/bidirectionalmap/bidirectionalmap.hpp>

namespace love
{
    class Transform : public Object
    {
      public:
        enum MatrixLayout
        {
            MATRIX_ROW_MAJOR,
            MATRIX_COLUMN_MAJOR
        };

        static Type type;

        Transform();

        Transform(const Matrix4& matrix);

        Transform(float x, float y, float a, float sx, float sy, float ox, float oy, float kx,
                  float ky);

        ~Transform()
        {}

        Transform* Clone();

        Transform* Inverse();

        void Apply(Transform* other);

        void Translate(float x, float y);

        void Rotate(float angle);

        void Scale(float x, float y);

        void Shear(float x, float y);

        void Reset();

        void SetTransformation(float x, float y, float a, float sx, float sy, float ox, float oy,
                               float kx, float ky);

        Vector2 TransformPoint(Vector2 point) const;

        Vector2 InverseTransformPoint(Vector2 point);

        Matrix4& GetMatrix();

        void SetMatrix(const Matrix4& matrix);

        // clang-format off
        static constexpr BidirectionalMap matrixLayouts = {
            "row",    MATRIX_ROW_MAJOR,
            "column", MATRIX_COLUMN_MAJOR
        };
        // clang-format on

      private:
        inline const Matrix4& GetInverseMatrix()
        {
            if (this->inverseDirty)
            {
                this->inverseDirty  = false;
                this->inverseMatrix = this->matrix.Inverse();
            }

            return this->inverseMatrix;
        }

        Matrix4 matrix;
        bool inverseDirty;
        Matrix4 inverseMatrix;
    };
} // namespace love
