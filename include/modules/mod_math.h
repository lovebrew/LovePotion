#pragma once

namespace Math
{
    //Löve2D Functions

    int GammaToLinear(lua_State * L);

    int GetRandomSeed(lua_State * L);

    int GetRandomState(lua_State * L);

    int IsConvex(lua_State * L);

    int LinearToGamma(lua_State * L);

    int Noise(lua_State * L);

    int Random(lua_State * L);

    int RandomNormal(lua_State * L);

    int SetRandomSeed(lua_State * L);

    int SetRandomState(lua_State * L);

    int Triangulate(lua_State * L);

    //End Löve2D Functions

    //Helper functions

    float convertGammaToLinear(float c);

    float convertLinearToGamma(float c);

    int getGammaArgs(lua_State * L, float color[4]);

    //End Helper functions

    int Register(lua_State * L);
}