#include "objects/data/compresseddata/wrap_compresseddata.hpp"

using namespace love;

int Wrap_CompressedData::Clone(lua_State* L)
{
    CompressedData* compressedData = Wrap_CompressedData::CheckCompressedData(L, 1);
    CompressedData* clone          = nullptr;

    luax::CatchException(L, [&]() { clone = compressedData->Clone(); });

    luax::PushType(L, clone);
    clone->Release();

    return 1;
}

int Wrap_CompressedData::GetFormat(lua_State* L)
{
    CompressedData* compressedData = Wrap_CompressedData::CheckCompressedData(L, 1);
    const char* formatName         = nullptr;

    if (auto found = Compressor::formats.ReverseFind(compressedData->GetFormat()))
        lua_pushstring(L, *found);
    else
        return luax::EnumError(L, "compressed data format", Compressor::formats.GetNames(),
                               formatName);

    return 1;
}

CompressedData* Wrap_CompressedData::CheckCompressedData(lua_State* L, int index)
{
    return luax::CheckType<CompressedData>(L, index);
}

// clang-format off
static constexpr luaL_Reg functions[] =
{
    { "clone",     Wrap_CompressedData::Clone     },
    { "getFormat", Wrap_CompressedData::GetFormat }
};
// clang-format on

int Wrap_CompressedData::Register(lua_State* L)
{
    return luax::RegisterType(L, &CompressedData::type, Wrap_Data::functions, functions);
}
