// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.h"

#include <glm/ext.hpp>

export module RenderCore.Types.Transform;

export import <string>;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API Vector
    {
        float X {0.f};
        float Y {0.f};
        float Z {0.f};

        Vector() = default;

        explicit Vector(float const Value)
            : X(Value), Y(Value), Z(Value)
        {
        }

        Vector(float const X, float const Y, float const Z)
            : X(X), Y(Y), Z(Z)
        {
        }

        explicit Vector(glm::vec3 const& Value)
            : X(Value.x), Y(Value.y), Z(Value.z)
        {
        }

        explicit Vector(glm::vec4 const& Value)
            : X(Value.x), Y(Value.y), Z(Value.z)
        {
        }

        Vector& operator=(Vector const& Value) = default;

        Vector& operator=(glm::vec3 const& Value)
        {
            X = Value.x;
            Y = Value.y;
            Z = Value.z;
            return *this;
        }

        Vector& operator=(glm::vec4 const& Value)
        {
            X = Value.x;
            Y = Value.y;
            Z = Value.z;
            return *this;
        }

        Vector& operator=(float const Value)
        {
            X = Value;
            Y = Value;
            Z = Value;
            return *this;
        }

        Vector& operator+=(Vector const& Value)
        {
            X += Value.X;
            Y += Value.Y;
            Z += Value.Z;
            return *this;
        }

        Vector& operator+=(float const Value)
        {
            X += Value;
            Y += Value;
            Z += Value;
            return *this;
        }

        Vector& operator-=(Vector const& Value)
        {
            X -= Value.X;
            Y -= Value.Y;
            Z -= Value.Z;
            return *this;
        }

        Vector& operator-=(float const Value)
        {
            X -= Value;
            Y -= Value;
            Z -= Value;
            return *this;
        }

        Vector& operator*=(Vector const& Value)
        {
            X *= Value.X;
            Y *= Value.Y;
            Z *= Value.Z;
            return *this;
        }

        Vector& operator*=(float const Value)
        {
            X *= Value;
            Y *= Value;
            Z *= Value;
            return *this;
        }

        Vector& operator/=(Vector const& Value)
        {
            X /= Value.X;
            Y /= Value.Y;
            Z /= Value.Z;
            return *this;
        }

        Vector& operator/=(float const Value)
        {
            X /= Value;
            Y /= Value;
            Z /= Value;
            return *this;
        }

        Vector operator+(Vector const& Value) const
        {
            return {X + Value.X, Y + Value.Y, Z + Value.Z};
        }

        Vector operator+(float const Value) const
        {
            return {X + Value, Y + Value, Z + Value};
        }

        Vector operator-(Vector const& Value) const
        {
            return {X - Value.X, Y - Value.Y, Z - Value.Z};
        }

        Vector operator-(float const Value) const
        {
            return {X - Value, Y - Value, Z - Value};
        }

        Vector operator*(Vector const& Value) const
        {
            return {X * Value.X, Y * Value.Y, Z * Value.Z};
        }

        Vector operator*(float const Value) const
        {
            return {X * Value, Y * Value, Z * Value};
        }

        Vector operator/(Vector const& Value) const
        {
            return {X / Value.X, Y / Value.Y, Z / Value.Z};
        }

        Vector operator/(float const Value) const
        {
            return {X / Value, Y / Value, Z / Value};
        }

        [[nodiscard]] glm::vec3 ToGlmVec3() const
        {
            return {X, Y, Z};
        }

        [[nodiscard]] glm::vec4 ToGlmVec4() const
        {
            return {X, Y, Z, 1.F};
        }

        void Normalize()
        {
            float const Length {sqrt(X * X + Y * Y + Z * Z)};
            X /= Length;
            Y /= Length;
            Z /= Length;
        }

        [[nodiscard]] Vector NormalizeInline() const
        {
            float const Length {sqrt(X * X + Y * Y + Z * Z)};
            return {X / Length, Y / Length, Z / Length};
        }

        void Cross(Vector const& Value)
        {
            X = Y * Value.Z - Z * Value.Y;
            Y = Z * Value.X - X * Value.Z;
            Z = X * Value.Y - Y * Value.X;
        }

        [[nodiscard]] Vector CrossInline(Vector const& Value) const
        {
            return {
                    Y * Value.Z - Z * Value.Y,
                    Z * Value.X - X * Value.Z,
                    X * Value.Y - Y * Value.X};
        }

        [[nodiscard]] std::string ToString()
        {
            return std::to_string(X) + ", " + std::to_string(Y) + ", " + std::to_string(Z);
        }
    };

    export [[nodiscard]] Vector operator*(float const Value, Vector const& Vector)
    {
        return {Vector.X * Value, Vector.Y * Value, Vector.Z * Value};
    }

    export struct RENDERCOREMODULE_API Rotator
    {
        float Pitch {0.f};
        float Yaw {0.f};
        float Roll {0.f};

        Rotator() = default;

        explicit Rotator(float const Value)
            : Pitch(Value), Yaw(Value), Roll(Value)
        {
        }

        Rotator(float const Pitch, float const Yaw, float const Roll)
            : Pitch(Pitch), Yaw(Yaw), Roll(Roll)
        {
        }

        explicit Rotator(glm::vec3 const& Value)
            : Pitch(Value.y), Yaw(Value.z), Roll(Value.x)
        {
        }

        explicit Rotator(glm::vec4 const& Value)
            : Pitch(Value.y), Yaw(Value.z), Roll(Value.x)
        {
        }

        Rotator& operator=(Rotator const& Value) = default;

        Rotator& operator=(glm::vec3 const& Value)
        {
            Roll  = Value.x;
            Pitch = Value.y;
            Yaw   = Value.z;
            return *this;
        }

        Rotator& operator=(glm::vec4 const& Value)
        {
            Roll  = Value.x;
            Pitch = Value.y;
            Yaw   = Value.z;
            return *this;
        }

        Rotator& operator=(float const Value)
        {
            Pitch = Value;
            Yaw   = Value;
            Roll  = Value;
            return *this;
        }

        Rotator& operator=(glm::quat const& Value)
        {
            Pitch = glm::degrees(roll(Value));
            Yaw   = glm::degrees(pitch(Value));
            Roll  = glm::degrees(yaw(Value));
            return *this;
        }

        Rotator& operator+=(Rotator const& Value)
        {
            Pitch += Value.Pitch;
            Yaw += Value.Yaw;
            Roll += Value.Roll;
            return *this;
        }

        Rotator& operator+=(float const Value)
        {
            Pitch += Value;
            Yaw += Value;
            Roll += Value;
            return *this;
        }

        Rotator& operator-=(Rotator const& Value)
        {
            Pitch -= Value.Pitch;
            Yaw -= Value.Yaw;
            Roll -= Value.Roll;
            return *this;
        }

        Rotator& operator-=(float const Value)
        {
            Pitch -= Value;
            Yaw -= Value;
            Roll -= Value;
            return *this;
        }

        Rotator& operator*=(Rotator const& Value)
        {
            Pitch *= Value.Pitch;
            Yaw *= Value.Yaw;
            Roll *= Value.Roll;
            return *this;
        }

        Rotator& operator*=(float const Value)
        {
            Pitch *= Value;
            Yaw *= Value;
            Roll *= Value;
            return *this;
        }

        Rotator& operator/=(Rotator const& Value)
        {
            Pitch /= Value.Pitch;
            Yaw /= Value.Yaw;
            Roll /= Value.Roll;
            return *this;
        }

        Rotator& operator/=(float const Value)
        {
            Pitch /= Value;
            Yaw /= Value;
            Roll /= Value;
            return *this;
        }

        Rotator operator+(Rotator const& Value) const
        {
            return {Pitch + Value.Pitch, Yaw + Value.Yaw, Roll + Value.Roll};
        }

        Rotator operator+(float const Value) const
        {
            return {Pitch + Value, Yaw + Value, Roll + Value};
        }

        Rotator operator-(Rotator const& Value) const
        {
            return {Pitch - Value.Pitch, Yaw - Value.Yaw, Roll - Value.Roll};
        }

        Rotator operator-(float const Value) const
        {
            return {Pitch - Value, Yaw - Value, Roll - Value};
        }

        Rotator operator*(Rotator const& Value) const
        {
            return {Pitch * Value.Pitch, Yaw * Value.Yaw, Roll * Value.Roll};
        }

        Rotator operator*(float const Value) const
        {
            return {Pitch * Value, Yaw * Value, Roll * Value};
        }

        Rotator operator/(Rotator const& Value) const
        {
            return {Pitch / Value.Pitch, Yaw / Value.Yaw, Roll / Value.Roll};
        }

        Rotator operator/(float const Value) const
        {
            return {Pitch / Value, Yaw / Value, Roll / Value};
        }

        [[nodiscard]] glm::vec3 ToGlmVec3() const
        {
            return {Pitch, Yaw, Roll};
        }

        [[nodiscard]] glm::vec4 ToGlmVec4() const
        {
            return {Pitch, Yaw, Roll, 1.F};
        }

        [[nodiscard]] Vector GetFront() const
        {
            return {
                    cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
                    sin(glm::radians(Pitch)),
                    sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))};
        }

        [[nodiscard]] Vector GetRight() const
        {
            return {
                    cos(glm::radians(Yaw - 90.F)),
                    0.F,
                    sin(glm::radians(Yaw - 90.F))};
        }

        [[nodiscard]] Vector GetUp() const
        {
            return GetFront().CrossInline(GetRight());
        }

        [[nodiscard]] std::string ToString() const
        {
            return std::to_string(Pitch) + ", " + std::to_string(Yaw) + ", " + std::to_string(Roll);
        }
    };

    export [[nodiscard]] Rotator operator*(float const Value, Rotator const& Rotator)
    {
        return {Rotator.Pitch * Value, Rotator.Yaw * Value, Rotator.Roll * Value};
    }

    export struct RENDERCOREMODULE_API Transform
    {
        Vector Position {0.F};
        Vector Scale {1.F};
        Rotator Rotation {0.F};

        Transform() = default;

        Transform(Vector const& Position, Vector const& Scale, Rotator const& Rotation)
            : Position(Position), Scale(Scale), Rotation(Rotation)
        {
        }

        explicit Transform(glm::mat4 const& TransformMatrix)
            : Position(Vector(TransformMatrix[3])),
              Scale(Vector(glm::vec3(TransformMatrix[0][0], TransformMatrix[1][1], TransformMatrix[2][2]))),
              Rotation(Rotator(glm::eulerAngles(glm::quat(TransformMatrix))))
        {
        }

        Transform& operator=(Transform const& Value) = default;

        Transform& operator=(float const Value)
        {
            Position = Value;
            Scale    = Value;
            Rotation = Value;
            return *this;
        }

        Transform& operator+=(Transform const& Value)
        {
            Position += Value.Position;
            Scale += Value.Scale;
            Rotation += Value.Rotation;
            return *this;
        }

        Transform& operator+=(float const Value)
        {
            Position += Value;
            Scale += Value;
            Rotation += Value;
            return *this;
        }

        Transform& operator-=(Transform const& Value)
        {
            Position -= Value.Position;
            Scale -= Value.Scale;
            Rotation -= Value.Rotation;
            return *this;
        }

        Transform& operator-=(float const Value)
        {
            Position -= Value;
            Scale -= Value;
            Rotation -= Value;
            return *this;
        }

        Transform& operator*=(Transform const& Value)
        {
            Position *= Value.Position;
            Scale *= Value.Scale;
            Rotation *= Value.Rotation;
            return *this;
        }

        Transform& operator*=(float const Value)
        {
            Position *= Value;
            Scale *= Value;
            Rotation *= Value;
            return *this;
        }

        Transform& operator/=(Transform const& Value)
        {
            Position /= Value.Position;
            Scale /= Value.Scale;
            Rotation /= Value.Rotation;
            return *this;
        }

        Transform& operator/=(float const Value)
        {
            Position /= Value;
            Scale /= Value;
            Rotation /= Value;
            return *this;
        }

        Transform operator+(Transform const& Value) const
        {
            return {Position + Value.Position, Scale + Value.Scale, Rotation + Value.Rotation};
        }

        Transform operator+(float const Value) const
        {
            return {Position + Value, Scale + Value, Rotation + Value};
        }

        Transform operator-(Transform const& Value) const
        {
            return {Position - Value.Position, Scale - Value.Scale, Rotation - Value.Rotation};
        }

        Transform operator-(float const Value) const
        {
            return {Position - Value, Scale - Value, Rotation - Value};
        }

        Transform operator*(Transform const& Value) const
        {
            return {Position * Value.Position, Scale * Value.Scale, Rotation * Value.Rotation};
        }

        Transform operator*(float const Value) const
        {
            return {Position * Value, Scale * Value, Rotation * Value};
        }

        Transform operator/(Transform const& Value) const
        {
            return {Position / Value.Position, Scale / Value.Scale, Rotation / Value.Rotation};
        }

        Transform operator/(float const Value) const
        {
            return {Position / Value, Scale / Value, Rotation / Value};
        }

        [[nodiscard]] glm::mat4 ToGlmMat4() const
        {
            glm::mat4 TransformMatrix(1.F);
            TransformMatrix = glm::translate(TransformMatrix, Position.ToGlmVec3());
            TransformMatrix = glm::scale(TransformMatrix, Scale.ToGlmVec3());
            TransformMatrix = glm::rotate(TransformMatrix, glm::radians(Rotation.Pitch), glm::vec3(1.F, 0.F, 0.F));
            TransformMatrix = glm::rotate(TransformMatrix, glm::radians(Rotation.Yaw), glm::vec3(0.F, 1.F, 0.F));
            TransformMatrix = glm::rotate(TransformMatrix, glm::radians(Rotation.Roll), glm::vec3(0.F, 0.F, 1.F));
            return TransformMatrix;
        }

        [[nodiscard]] std::string ToString()
        {
            return Position.ToString() + ", " + Scale.ToString() + ", " + Rotation.ToString();
        }
    };
}// namespace RenderCore