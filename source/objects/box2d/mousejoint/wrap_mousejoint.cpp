#include "mousejoint/wrap_mousejoint.h"
#include "joint/wrap_joint.h"

using namespace love;

MouseJoint* Wrap_MouseJoint::CheckMouseJoint(lua_State* L, int index)
{
    MouseJoint* joint = Luax::CheckType<MouseJoint>(L, index);

    if (!joint->IsValid())
        luaL_error(L, "Attempt to use destroyed joint!");

    return joint;
}

int Wrap_MouseJoint::SetTarget(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);

    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);

    self->SetTarget(x, y);

    return 0;
}

int Wrap_MouseJoint::GetTarget(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);
    lua_remove(L, 1);

    return self->GetTarget(L);
}

int Wrap_MouseJoint::SetMaxForce(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);
    float maxForce   = luaL_checknumber(L, 2);

    self->SetMaxForce(maxForce);

    return 0;
}

int Wrap_MouseJoint::GetMaxForce(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);

    lua_pushnumber(L, self->GetMaxForce());

    return 1;
}

int Wrap_MouseJoint::SetFrequency(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);
    float frequency  = luaL_checknumber(L, 2);

    Luax::CatchException(L, [&]() { self->SetFrequency(frequency); });

    return 0;
}

int Wrap_MouseJoint::GetFrequency(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);

    lua_pushnumber(L, self->GetFrequency());

    return 1;
}

int Wrap_MouseJoint::SetDampingRatio(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);
    float ratio      = luaL_checknumber(L, 2);

    self->SetDampingRatio(ratio);

    return 0;
}

int Wrap_MouseJoint::GetDampingRatio(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);

    lua_pushnumber(L, self->GetDampingRatio());

    return 1;
}

int Wrap_MouseJoint::SetStiffness(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);
    float stiffness  = luaL_checknumber(L, 2);

    Luax::CatchException(L, [&]() { self->SetStiffness(stiffness); });

    return 0;
}

int Wrap_MouseJoint::GetStiffness(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);

    lua_pushnumber(L, self->GetStiffness());

    return 1;
}

int Wrap_MouseJoint::SetDamping(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);
    float damping    = luaL_checknumber(L, 2);

    self->SetDamping(damping);

    return 0;
}

int Wrap_MouseJoint::GetStiffness(lua_State* L)
{
    MouseJoint* self = Wrap_MouseJoint::CheckMouseJoint(L, 1);

    lua_pushnumber(L, self->GetStiffness());

    return 1;
}

int Wrap_MouseJoint::Register(lua_State* L)
{
    luaL_Reg funcs[] = { { "setTarget", SetTarget },
                         { "getTarget", GetTarget },
                         { "setMaxForce", SetMaxForce },
                         { "getMaxForce", GetMaxForce },
                         { "setFrequency", SetFrequency },
                         { "getFrequency", GetFrequency },
                         { "setDampingRatio", SetDampingRatio },
                         { "getDampingRatio", GetDampingRatio },
                         { "setStiffness", SetStiffness },
                         { "getStiffness", GetStiffness },
                         { "setDamping", SetDamping },
                         { "getDamping", GetDamping },
                         { 0, 0 } };

    Luax::RegisterType(L, &MouseJoint::type, Wrap_Joint::functions, funcs, nullptr);
}
