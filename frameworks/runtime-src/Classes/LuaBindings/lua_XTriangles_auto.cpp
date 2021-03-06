#include "lua_XTriangles_auto.hpp"
#include "../Classes/XTriangles.h"
#include "scripting/lua-bindings/manual/tolua_fix.h"
#include "scripting/lua-bindings/manual/LuaBasicConversions.h"
#include "UtilLua.h"
#define printf(...)
#define color4b_to_luaval lstg::lua::_color4b_to_luaval
#define luaval_to_color4b lstg::lua::_luaval_to_color4b
using namespace lstg::lua;

int lua_x_XTriangles_XTriangles_setVertex(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_setVertex'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 7) 
    {
        unsigned int arg0;
        double arg1;
        double arg2;
        double arg3;
        double arg4;
        double arg5;
        cocos2d::Color4B arg6;

        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:setVertex");

        ok &= luaval_to_number(tolua_S, 3,&arg1, "lstg.XTriangles:setVertex");

        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:setVertex");

        ok &= luaval_to_number(tolua_S, 5,&arg3, "lstg.XTriangles:setVertex");

        ok &= luaval_to_number(tolua_S, 6,&arg4, "lstg.XTriangles:setVertex");

        ok &= luaval_to_number(tolua_S, 7,&arg5, "lstg.XTriangles:setVertex");

        ok &=luaval_to_color4b(tolua_S, 8, &arg6, "lstg.XTriangles:setVertex");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_setVertex'", nullptr);
            return 0;
        }
        cobj->setVertex(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
        lua_settop(tolua_S, 1);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:setVertex",argc, 7);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_setVertex'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_setIndex(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_setIndex'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 2) 
    {
        unsigned int arg0;
        unsigned short arg1;

        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:setIndex");

        ok &= luaval_to_ushort(tolua_S, 3, &arg1, "lstg.XTriangles:setIndex");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_setIndex'", nullptr);
            return 0;
        }
        cobj->setIndex(arg0, arg1);
        lua_settop(tolua_S, 1);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:setIndex",argc, 2);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_setIndex'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_clone(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_clone'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 0) 
    {
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_clone'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = cobj->clone();
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:clone",argc, 0);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_clone'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_setVertexCoords(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_setVertexCoords'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 3) 
    {
        unsigned int arg0;
        double arg1;
        double arg2;

        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:setVertexCoords");

        ok &= luaval_to_number(tolua_S, 3,&arg1, "lstg.XTriangles:setVertexCoords");

        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:setVertexCoords");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_setVertexCoords'", nullptr);
            return 0;
        }
        cobj->setVertexCoords(arg0, arg1, arg2);
        lua_settop(tolua_S, 1);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:setVertexCoords",argc, 3);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_setVertexCoords'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_getIndexCount(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_getIndexCount'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 0) 
    {
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_getIndexCount'", nullptr);
            return 0;
        }
        unsigned int ret = cobj->getIndexCount();
        tolua_pushnumber(tolua_S,(lua_Number)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:getIndexCount",argc, 0);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_getIndexCount'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_setVertexPosition(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_setVertexPosition'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 4) 
    {
        unsigned int arg0;
        double arg1;
        double arg2;
        double arg3;

        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:setVertexPosition");

        ok &= luaval_to_number(tolua_S, 3,&arg1, "lstg.XTriangles:setVertexPosition");

        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:setVertexPosition");

        ok &= luaval_to_number(tolua_S, 5,&arg3, "lstg.XTriangles:setVertexPosition");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_setVertexPosition'", nullptr);
            return 0;
        }
        cobj->setVertexPosition(arg0, arg1, arg2, arg3);
        lua_settop(tolua_S, 1);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:setVertexPosition",argc, 4);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_setVertexPosition'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_getVertexCount(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_getVertexCount'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 0) 
    {
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_getVertexCount'", nullptr);
            return 0;
        }
        unsigned int ret = cobj->getVertexCount();
        tolua_pushnumber(tolua_S,(lua_Number)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:getVertexCount",argc, 0);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_getVertexCount'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_setAllVertexColor(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_setAllVertexColor'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 1) 
    {
        cocos2d::Color4B arg0;

        ok &=luaval_to_color4b(tolua_S, 2, &arg0, "lstg.XTriangles:setAllVertexColor");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_setAllVertexColor'", nullptr);
            return 0;
        }
        cobj->setAllVertexColor(arg0);
        lua_settop(tolua_S, 1);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:setAllVertexColor",argc, 1);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_setAllVertexColor'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_setVertexColor(lua_State* tolua_S)
{
    int argc = 0;
    lstg::XTriangles* cobj = nullptr;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif


#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertype(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    cobj = (lstg::XTriangles*)tolua_tousertype(tolua_S,1,0);

#if COCOS2D_DEBUG >= 1
    if (!cobj) 
    {
        tolua_error(tolua_S,"invalid 'cobj' in function 'lua_x_XTriangles_XTriangles_setVertexColor'", nullptr);
        return 0;
    }
#endif

    argc = lua_gettop(tolua_S)-1;
    if (argc == 2) 
    {
        unsigned int arg0;
        cocos2d::Color4B arg1;

        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:setVertexColor");

        ok &=luaval_to_color4b(tolua_S, 3, &arg1, "lstg.XTriangles:setVertexColor");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_setVertexColor'", nullptr);
            return 0;
        }
        cobj->setVertexColor(arg0, arg1);
        lua_settop(tolua_S, 1);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d \n", "lstg.XTriangles:setVertexColor",argc, 2);
    return 0;

#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_setVertexColor'.",&tolua_err);
#endif

    return 0;
}
int lua_x_XTriangles_XTriangles_create(lua_State* tolua_S)
{
    int argc = 0;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif

#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertable(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    argc = lua_gettop(tolua_S) - 1;

    if (argc == 2)
    {
        unsigned int arg0;
        unsigned int arg1;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:create");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:create");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_create'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::create(arg0, arg1);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d\n ", "lstg.XTriangles:create",argc, 2);
    return 0;
#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_create'.",&tolua_err);
#endif
    return 0;
}
int lua_x_XTriangles_XTriangles_createQuad(lua_State* tolua_S)
{
    int argc = 0;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif

#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertable(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    argc = lua_gettop(tolua_S) - 1;

    if (argc == 1)
    {
        cocos2d::Vec2 arg0;
        ok &= luaval_to_vec2(tolua_S, 2, &arg0, "lstg.XTriangles:createQuad");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createQuad'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createQuad(arg0);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    if (argc == 2)
    {
        cocos2d::Vec2 arg0;
        cocos2d::Rect arg1;
        ok &= luaval_to_vec2(tolua_S, 2, &arg0, "lstg.XTriangles:createQuad");
        ok &= luaval_to_rect(tolua_S, 3, &arg1, "lstg.XTriangles:createQuad");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createQuad'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createQuad(arg0, arg1);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d\n ", "lstg.XTriangles:createQuad",argc, 1);
    return 0;
#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_createQuad'.",&tolua_err);
#endif
    return 0;
}
int lua_x_XTriangles_XTriangles_createCircle(lua_State* tolua_S)
{
    int argc = 0;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif

#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertable(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    argc = lua_gettop(tolua_S) - 1;

    if (argc == 3)
    {
        unsigned int arg0;
        unsigned int arg1;
        double arg2;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:createCircle");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:createCircle");
        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:createCircle");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createCircle'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createCircle(arg0, arg1, arg2);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    if (argc == 4)
    {
        unsigned int arg0;
        unsigned int arg1;
        double arg2;
        cocos2d::Rect arg3;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:createCircle");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:createCircle");
        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:createCircle");
        ok &= luaval_to_rect(tolua_S, 5, &arg3, "lstg.XTriangles:createCircle");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createCircle'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createCircle(arg0, arg1, arg2, arg3);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d\n ", "lstg.XTriangles:createCircle",argc, 3);
    return 0;
#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_createCircle'.",&tolua_err);
#endif
    return 0;
}
int lua_x_XTriangles_XTriangles_createSector(lua_State* tolua_S)
{
    int argc = 0;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif

#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertable(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    argc = lua_gettop(tolua_S) - 1;

    if (argc == 5)
    {
        unsigned int arg0;
        unsigned int arg1;
        double arg2;
        double arg3;
        double arg4;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:createSector");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:createSector");
        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:createSector");
        ok &= luaval_to_number(tolua_S, 5,&arg3, "lstg.XTriangles:createSector");
        ok &= luaval_to_number(tolua_S, 6,&arg4, "lstg.XTriangles:createSector");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createSector'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createSector(arg0, arg1, arg2, arg3, arg4);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    if (argc == 6)
    {
        unsigned int arg0;
        unsigned int arg1;
        double arg2;
        double arg3;
        double arg4;
        cocos2d::Rect arg5;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:createSector");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:createSector");
        ok &= luaval_to_number(tolua_S, 4,&arg2, "lstg.XTriangles:createSector");
        ok &= luaval_to_number(tolua_S, 5,&arg3, "lstg.XTriangles:createSector");
        ok &= luaval_to_number(tolua_S, 6,&arg4, "lstg.XTriangles:createSector");
        ok &= luaval_to_rect(tolua_S, 7, &arg5, "lstg.XTriangles:createSector");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createSector'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createSector(arg0, arg1, arg2, arg3, arg4, arg5);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d\n ", "lstg.XTriangles:createSector",argc, 5);
    return 0;
#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_createSector'.",&tolua_err);
#endif
    return 0;
}
int lua_x_XTriangles_XTriangles_createGrid(lua_State* tolua_S)
{
    int argc = 0;
    bool ok  = true;

#if COCOS2D_DEBUG >= 1
    tolua_Error tolua_err;
#endif

#if COCOS2D_DEBUG >= 1
    if (!tolua_isusertable(tolua_S,1,"lstg.XTriangles",0,&tolua_err)) goto tolua_lerror;
#endif

    argc = lua_gettop(tolua_S) - 1;

    if (argc == 3)
    {
        unsigned int arg0;
        unsigned int arg1;
        cocos2d::Vec2 arg2;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:createGrid");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:createGrid");
        ok &= luaval_to_vec2(tolua_S, 4, &arg2, "lstg.XTriangles:createGrid");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createGrid'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createGrid(arg0, arg1, arg2);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    if (argc == 4)
    {
        unsigned int arg0;
        unsigned int arg1;
        cocos2d::Vec2 arg2;
        cocos2d::Rect arg3;
        ok &= luaval_to_uint32(tolua_S, 2,&arg0, "lstg.XTriangles:createGrid");
        ok &= luaval_to_uint32(tolua_S, 3,&arg1, "lstg.XTriangles:createGrid");
        ok &= luaval_to_vec2(tolua_S, 4, &arg2, "lstg.XTriangles:createGrid");
        ok &= luaval_to_rect(tolua_S, 5, &arg3, "lstg.XTriangles:createGrid");
        if(!ok)
        {
            tolua_error(tolua_S,"invalid arguments in function 'lua_x_XTriangles_XTriangles_createGrid'", nullptr);
            return 0;
        }
        lstg::XTriangles* ret = lstg::XTriangles::createGrid(arg0, arg1, arg2, arg3);
        object_to_luaval<lstg::XTriangles>(tolua_S, "lstg.XTriangles",(lstg::XTriangles*)ret);
        return 1;
    }
    luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d\n ", "lstg.XTriangles:createGrid",argc, 3);
    return 0;
#if COCOS2D_DEBUG >= 1
    tolua_lerror:
    tolua_error(tolua_S,"#ferror in function 'lua_x_XTriangles_XTriangles_createGrid'.",&tolua_err);
#endif
    return 0;
}
static int lua_x_XTriangles_XTriangles_finalize(lua_State* tolua_S)
{
    printf("luabindings: finalizing LUA object (XTriangles)");
    return 0;
}

int lua_register_x_XTriangles_XTriangles(lua_State* tolua_S)
{
    tolua_usertype(tolua_S,"lstg.XTriangles");
    tolua_cclass(tolua_S,"XTriangles","lstg.XTriangles","cc.Ref",nullptr);

    tolua_beginmodule(tolua_S,"XTriangles");
        tolua_function(tolua_S,"setVertex",lua_x_XTriangles_XTriangles_setVertex);
        tolua_function(tolua_S,"setIndex",lua_x_XTriangles_XTriangles_setIndex);
        tolua_function(tolua_S,"clone",lua_x_XTriangles_XTriangles_clone);
        tolua_function(tolua_S,"setVertexCoords",lua_x_XTriangles_XTriangles_setVertexCoords);
        tolua_function(tolua_S,"getIndexCount",lua_x_XTriangles_XTriangles_getIndexCount);
        tolua_function(tolua_S,"setVertexPosition",lua_x_XTriangles_XTriangles_setVertexPosition);
        tolua_function(tolua_S,"getVertexCount",lua_x_XTriangles_XTriangles_getVertexCount);
        tolua_function(tolua_S,"setAllVertexColor",lua_x_XTriangles_XTriangles_setAllVertexColor);
        tolua_function(tolua_S,"setVertexColor",lua_x_XTriangles_XTriangles_setVertexColor);
        tolua_function(tolua_S,"create", lua_x_XTriangles_XTriangles_create);
        tolua_function(tolua_S,"createQuad", lua_x_XTriangles_XTriangles_createQuad);
        tolua_function(tolua_S,"createCircle", lua_x_XTriangles_XTriangles_createCircle);
        tolua_function(tolua_S,"createSector", lua_x_XTriangles_XTriangles_createSector);
        tolua_function(tolua_S,"createGrid", lua_x_XTriangles_XTriangles_createGrid);
    tolua_endmodule(tolua_S);
    std::string typeName = typeid(lstg::XTriangles).name();
    g_luaType[typeName] = "lstg.XTriangles";
    g_typeCast["XTriangles"] = "lstg.XTriangles";
    return 1;
}
TOLUA_API int register_all_x_XTriangles(lua_State* tolua_S)
{
	tolua_open(tolua_S);
	
	tolua_module(tolua_S,"lstg",0);
	tolua_beginmodule(tolua_S,"lstg");

	lua_register_x_XTriangles_XTriangles(tolua_S);

	tolua_endmodule(tolua_S);
	return 1;
}

