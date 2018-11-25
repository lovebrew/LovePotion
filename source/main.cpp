extern "C" 
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
    #include <compat-5.3.h>
    #include <luaobj.h>
}

#include <string>
#include <3ds.h>

#include "socket/luasocket.h"

#include "common/console.h"
#include "objects/gamepad/wrap_gamepad.h"
#include "modules/love.h"
#include "modules/timer.h"
#include "common/util.h"

#include "boot_lua.h"

bool ERROR = false;
bool LOVE_QUIT = false;

int main(int argc, char **argv)
{
    lua_State * L = luaL_newstate();

    luaL_openlibs(L);

    love_preload(L, LuaSocket::Initialize, "socket");

    Love::InitModules();

    // require "love"
    luaL_requiref(L, "love", Love::Initialize, 1);
    
    Love::InitConstants(L);
    
    gamepadNew(L);

    luaL_dobuffer(L, (char *)boot_lua, boot_lua_size, "boot");

    while (aptMainLoop())
    {
        if (Love::IsRunning())
            luaL_dostring(L, "xpcall(love.run, love.errhand)");
        else
            break;
    }

    Love::Exit(L);

    return 0;
}
