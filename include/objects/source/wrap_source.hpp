#pragma once

#include <common/luax.hpp>
#include <objects/source_ext.hpp>

namespace Wrap_Source
{
    love::Source<love::Console::Which>* CheckSource(lua_State* L, int index);

    int Clone(lua_State* L);

    int Play(lua_State* L);

    int Stop(lua_State* L);

    int Pause(lua_State* L);

    int SetVolume(lua_State* L);

    int GetVolume(lua_State* L);

    int SetPitch(lua_State* L);

    int GetPitch(lua_State* L);

    int Seek(lua_State* L);

    int Tell(lua_State* L);

    int GetDuration(lua_State* L);

    int SetLooping(lua_State* L);

    int IsLooping(lua_State* L);

    int IsPlaying(lua_State* L);

    int SetVolumeLimits(lua_State* L);

    int GetVolumeLimits(lua_State* L);

    int GetChannelCount(lua_State* L);

    int GetFreeBufferCount(lua_State* L);

    int Queue(lua_State* L);

    int GetType(lua_State* L);

    int Register(lua_State* L);
} // namespace Wrap_Source
