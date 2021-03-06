﻿#include "GameObjectPool.h"
#include "AppFrame.h"
#include "CollisionDetect.h"
#include "Utility.h"
#include "GameObjectPropertyHash.h"
#include "Renderer.h"
#include "../Math/XMath.h"
#include "UtilLua.h"
#include "XProfiler.h"
#include "LuaWrapper.h"
#include "ComponentDataAni.h"
#include "XThreadPool.hpp"
#include "scripting/lua-bindings/manual/tolua_fix.h"
#include "scripting/lua-bindings/manual/LuaBasicConversions.h"

#define METATABLE_OBJ "mt"
#define USE_CLASSID
#define OPT_RENDER_LIST
#define MT_UpdateTransformMat
#define MT_UpdateTransform
#define MT_CollisionCheck
#define MT_BoundCheck
#define USE_ComponentManager

#define error_prop(L, p) luaL_error(L, "invalid argument for property '%s'", p)
#define error_obj(L) luaL_error(L, "invalid luastg game object")
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define GETOBJTABLE \
	do { \
		lua_pushlightuserdata(L, (void*)&LAPP); \
		lua_gettable(L, LUA_REGISTRYINDEX); \
	} while (false)

#define LIST_INSERT_BEFORE(target, p, field) \
	do { \
		p->p##field##Prev = (target)->p##field##Prev; \
		p->p##field##Next = (target); \
		p->p##field##Prev->p##field##Next = p; \
		p->p##field##Next->p##field##Prev = p; \
	} while(false)

#define LIST_INSERT_AFTER(target, p, field) \
	do { \
		p->p##field##Prev = (target); \
		p->p##field##Next = (target)->p##field##Next; \
		p->p##field##Prev->p##field##Next = p; \
		p->p##field##Next->p##field##Prev = p; \
	} while(false)

#define LIST_REMOVE(p, field) \
	do { \
		p->p##field##Prev->p##field##Next = p->p##field##Next; \
		p->p##field##Next->p##field##Prev = p->p##field##Prev; \
	} while(false)

using namespace std;
using namespace cocos2d;
using namespace lstg;
using namespace xmath;
using namespace intersect;
using namespace collision;

static bool ObjectListSortFunc(GameObject* p1, GameObject* p2)noexcept
{
	// by uid
	return p1->uid < p2->uid;
}

static bool RenderListSortFunc(GameObject* p1, GameObject* p2)noexcept
{
	// lower layer first, then uid
	return (p1->layer < p2->layer) || ((p1->layer == p2->layer) && (p1->uid < p2->uid));
}

bool lstg::CollisionCheck(GameObject* p1, GameObject* p2)noexcept
{
	// skip
	if (!p1->colli || !p2->colli)
		return false;
	return p1->cm->collisionCheck(p2->cm);
}

void CreateObjectTable(lua_State* L)
{
	// create a global table to hold game object
	lua_pushlightuserdata(L, (void*)&LAPP);  // p
	lua_createtable(L, LGOBJ_MAXCNT, 0);  // p t

	// get lstg.GetAttr and lstg.SetAttr, create metatable
	lua_newtable(L);  // ... t
	lua_getglobal(L, "lstg");  // ... t t
	lua_pushstring(L, "GetAttr");  // ... t t s
	lua_gettable(L, -2);  // ... t t f(GetAttr)
	lua_pushstring(L, "SetAttr");  // ... t t f(GetAttr) s
	lua_gettable(L, -3);  // ... t t f(GetAttr) f(SetAttr)
	CCASSERT(lua_iscfunction(L, -1) && lua_iscfunction(L, -2), "GameObjectPool: Wrong lua stack.");
	lua_setfield(L, -4, "__newindex");  // ... t t f(GetAttr)
	lua_setfield(L, -3, "__index");  // ... t t
	lua_pop(L, 1);  // ... t(mt)

	// save to register[app]['mt']
	lua_setfield(L, -2, METATABLE_OBJ);  // p t
	lua_settable(L, LUA_REGISTRYINDEX);
}

////////////////////////////////////////////////////////////////////////////////
/// GameObject
////////////////////////////////////////////////////////////////////////////////

void GameObject::_Reset()
{
	status = STATUS_FREE;
	id = (size_t)-1;
	uid = 0;

	layer = 0.;
	colli = bound = true;
	hide = false;
	timer = 0;

	ReleaseResource();
	res = nullptr;
	cm = nullptr;

	pObjectPrev = pObjectNext = nullptr;
	pRenderPrev = pRenderNext = nullptr;
	pCollisionPrev = pCollisionNext = nullptr;

	//classID = 0;
	cls = nullptr;
}

void GameObject::ReleaseResource()
{
	if (res)
	{
		res->release();
		res = nullptr;
	}
	//if(block_frame)
	//{
	//	block_frame->release();
	//	block_frame = nullptr;
	//}
	if (cm)
	{
		// just release things related to game resource
		cm->resetForGameObject();
	}
}

bool GameObject::ChangeResource(Resource* r)
{
	if (!r || r == res)return true;

	//const auto factor = LRES.GetGlobalImageScaleFactor();
	switch (r->getType()) {
		case ResourceType::Sprite:
			{
				ReleaseResource();
				const auto sprite = dynamic_cast<ResSprite*>(r);
				res = sprite;
				res->retain();
				cm->applyResSprite(sprite);
				return true;
			}
		case ResourceType::Animation:
			{
				ReleaseResource();
				const auto animation = dynamic_cast<ResAnimation*>(r);
				res = animation;
				res->retain();
				const auto data = ComponentDataAni::create(animation);
				cm->setDataAni(data);
				cm->applyResAnimation(animation);
				return true;
			}
		case ResourceType::Particle:
			{
				ReleaseResource();
				const auto particle = dynamic_cast<ResParticle*>(r);
				res = particle;
				const auto data = ComponentDataParticle::create(particle);
				if (!data||!data->pool)
				{
					res = nullptr;
					LERROR("failed to create particle pool");
					return false;
				}
				cm->setDataParticle(data);
				cm->applyResParticle(particle);
				auto ps = data->pool;
				const auto tr = cm->getDataTrasform();
				ps->setActive(false);
				ps->setCenter(Vec2(tr->x, tr->y));
				ps->setRotation(tr->rot);
				ps->setActive(true);
				res->retain();
				return true;
			}
		case ResourceType::Font:
			{
				ReleaseResource();
				const auto font = dynamic_cast<ResFont*>(r);
				res = font;
				res->retain();
				const auto data = ComponentDataLabel::create(font);
				cm->setDataLabel(data);
				cm->applyResFont(font);
				return true;
			}
		case ResourceType::Texture:
			{
				ReleaseResource();
				const auto tex = dynamic_cast<ResTexture*>(r);
				res = tex;
				res->retain();
				const auto data = ComponentDataVertex::create(tex);
				cm->setDataVertex(data);
				return true;
			}
		default: ;
	}

	return false;
}

void GameObject::UpdateXY()
{
#ifdef USE_ComponentManager
#else
	dx = x - lastx;
	dy = y - lasty;
	lastx = x;
	lasty = y;
	if (navi && (dx != 0 || dy != 0))
		rot = atan2(dy, dx);
#endif // USE_ComponentManager
}

void GameObject::OnDoFrame()
{
#ifdef USE_ComponentManager
#else
	// update by derivatives
	vx += ax;
	vy += ay;
	x += vx;
	y += vy;
	rot += omiga;
#endif // USE_ComponentManager
	// update particle
	GameObjectPool::UpdateParticle(this);
}

void GameObject::CopyAttrFrom(GameObject* other)
{
	//TODO: ComponentManager
	colli = other->colli;
	layer = other->layer;
	timer = other->timer;
	bound = other->bound;
	hide = other->hide;
	status = other->status;
	cls = other->cls;
}

void GameObject::removeFromList()
{
	// remove from obj list
	pObjectPrev->pObjectNext = pObjectNext;
	pObjectNext->pObjectPrev = pObjectPrev;
	// remove from render list
	pRenderPrev->pRenderNext = pRenderNext;
	pRenderNext->pRenderPrev = pRenderPrev;
	// remove from collision list
	pCollisionPrev->pCollisionNext = pCollisionNext;
	pCollisionNext->pCollisionPrev = pCollisionPrev;
}

////////////////////////////////////////////////////////////////////////////////
/// GameObjectPool
////////////////////////////////////////////////////////////////////////////////

GameObjectPool::GameObjectPool(lua_State* pL)
	: L(pL)
{
	maxObjectCount = LGOBJ_MAXCNT;
	maxGroupCount = LGOBJ_GROUPCNT;
	// init data
	const auto uid_max = numeric_limits<uint64_t>::max();
	const auto uid_min = numeric_limits<uint64_t>::lowest();
	memset(&m_pObjectListHeader, 0, sizeof(GameObject));
	memset(&m_pRenderListHeader, 0, sizeof(GameObject));
	memset(m_pCollisionListHeader, 0, sizeof(m_pCollisionListHeader));
	memset(&m_pObjectListTail, 0, sizeof(GameObject));
	memset(&m_pRenderListTail, 0, sizeof(GameObject));
	memset(m_pCollisionListTail, 0, sizeof(m_pCollisionListTail));
	m_pObjectListHeader.pObjectNext = &m_pObjectListTail;
	m_pObjectListHeader.uid = uid_min;
	m_pObjectListTail.pObjectPrev = &m_pObjectListHeader;
	m_pObjectListTail.uid = uid_max;
	m_pRenderListHeader.pRenderNext = &m_pRenderListTail;
	m_pRenderListHeader.uid = uid_min;
	m_pRenderListHeader.layer = numeric_limits<lua_Number>::lowest();
	m_pRenderListTail.pRenderPrev = &m_pRenderListHeader;
	m_pRenderListTail.uid = uid_max;
	m_pRenderListTail.layer = numeric_limits<lua_Number>::max();
	for (size_t i = 0; i < maxGroupCount; ++i)
	{
		m_pCollisionListHeader[i].pCollisionNext = &m_pCollisionListTail[i];
		m_pCollisionListTail[i].pCollisionPrev = &m_pCollisionListHeader[i];
	}

	CreateObjectTable(L);
}

GameObjectPool::~GameObjectPool()
{
	ResetPool();
}

void GameObjectPool::ResetLua(lua_State* pL)
{
	L = pL;
	GameClass::clear(pL);
	lightSources.clear();
}

GameObject* GameObjectPool::freeObjectInternal(GameObject* p) noexcept
{
	const auto pRet = p->pObjectNext;
	p->removeFromList();
	// delete from obj table, assert there is ot on stack
	lua_pushnil(L);  // ot nil
	lua_rawseti(L, -2, p->id + 1);  // ot
	// release resource
	p->ReleaseResource();
	p->cm->releaseComponentsForGameObject();
	if(p->cm->getLightSource())
		lightSources.erase(p);
	p->status = STATUS_FREE;
#ifdef OPT_RENDER_LIST
	if (p == renderMid)
	{
		if (p->pRenderNext&&p->pRenderNext->pRenderNext)
			renderMid = p->pRenderNext;
		else
			renderMid = nullptr;
	}
#endif
	// delete from pool
	_objectPool.Free(p->id);
	return pRet;
}

bool GameObjectPool::isDefaultFrame(GameObject* p)
{
	return p->cls->isDefaultFrame;
}

bool GameObjectPool::isDefaultRender(GameObject* p)
{
	return p->cls->isDefaultRender;
}

bool GameObjectPool::isExtProperty(GameObject* p)
{
	return p->cls->extProperty;
}

bool GameObjectPool::isExtProperty3D(GameObject* p)
{
	return p->cls->is3D;
}

GameObject* GameObjectPool::freeObject(GameObject* p)noexcept
{
	const auto pRet = p->pObjectNext;
	p->removeFromList();
	// delete from obj table
	GETOBJTABLE;  // ot
	lua_pushnil(L);  // ot nil
	lua_rawseti(L, -2, p->id + 1);  // ot
	lua_pop(L, 1);
	// release resource
	p->ReleaseResource();
	p->cm->releaseComponentsForGameObject();
	if (p->cm->getLightSource())
		lightSources.erase(p);
	p->status = STATUS_FREE;
#ifdef OPT_RENDER_LIST
	if (p == renderMid)
	{
		if (p->pRenderNext&&p->pRenderNext->pRenderNext)
			renderMid = p->pRenderNext;
		else
			renderMid = nullptr;
	}
#endif
	// delete from pool
	_objectPool.Free(p->id);
	return pRet;
}

void GameObjectPool::DoFrame()noexcept
{
	inDoFrame = true;
	GETOBJTABLE;  // ot

	uint32_t last_classID = 0;

	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		currentObj = p;
#ifdef USE_CLASSID
		if(p->cls->isDefaultFrame)
		{
			// pass
		}
		else if (p->cls->frameBlock)
		{
			p->cls->frameBlock->exe(p);
		}
		else
		{
			const auto cid = p->cls->id;
			CCASSERT(cid != 0, "internal error");
			if (cid != last_classID)
			{
				lua_settop(L, 1);  // ot
				p->cls->pushLuaFrameFunc(L);  // ot f(frame)
				lua_pushvalue(L, -1);  // ot f(frame) f(frame)
				lua_rawgeti(L, 1, p->id + 1);  // ot f(frame) f(frame) obj
				// lua_call is the most slow function here
				lua_call(L, 1, 0);  // ot f(frame)
				last_classID = cid;
			}
			else
			{
				lua_pushvalue(L, -1);  // ot f(frame) f(frame)
				lua_rawgeti(L, 1, p->id + 1);  // ot f(frame) f(frame) obj
				lua_call(L, 1, 0);  // ot f(frame)
			}
		}
#else
		lua_settop(L, 1);  // ot
		// id -> lua table -> class -> framefunc
		lua_rawgeti(L, -1, p->id + 1);  // ot obj
		lua_rawgeti(L, -1, 1);  // ot obj cls
		lua_rawgeti(L, -1, LGOBJ_CC_FRAME);  // ot obj cls f(frame)
		lua_pushvalue(L, -3);  // ot obj cls f(frame) obj
		lua_call(L, 1, 0);  // ot obj cls
		lua_pop(L, 2);  // ot			
#endif
		p = p->pObjectNext;
	}
	currentObj = nullptr;

	//XProfiler::getInstance()->tic("trans_par");

#ifdef MT_UpdateTransform
	static vector<GameObject*> to_frame;
	to_frame.clear();
	to_frame.swap(to_frame);
	to_frame.reserve(GetObjectCount());
#endif // MT_UpdateTransform

	p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
#ifdef MT_UpdateTransform
		to_frame.push_back(p);
#else
		p->cm->updateTransform();
		p->cm->updateParticle();
#endif // MT_UpdateTransform
		p = p->pObjectNext;
	}
#ifdef MT_UpdateTransformMat
	static atomic<int> _trans_par_flagn;
	_trans_par_flagn = 0;
	const auto nThr = LTHP.size() + 1;
	const auto dat = to_frame.data();
	deployThreadTask(to_frame.size(), nThr, [=](int start, int end)
	{
		for (auto j = start; j < end; j++)
		{
			dat[j]->cm->updateTransform();
			dat[j]->cm->updateParticle();
		}
		++_trans_par_flagn;
	});
	while (_trans_par_flagn < nThr) {}
#endif
	//XProfiler::getInstance()->toc("trans_par");

	lua_pop(L, 1);
	inDoFrame = false;
}

void GameObjectPool::DoRender()noexcept
{
	inDoRender = true;
	//XProfiler::getInstance()->tic("transform");
#ifdef MT_UpdateTransformMat
	static vector<GameObject*> to_render;
	to_render.clear();
	to_render.swap(to_render);
	to_render.reserve(GetObjectCount());
#endif
	GameObject* p = m_pRenderListHeader.pRenderNext;
	while (p && p != &m_pRenderListTail)
	{
		if (!p->hide/*&&p->isDefaultRender*/) // TODO: will waste, should make a flag for p
		{
#ifdef MT_UpdateTransformMat
			to_render.push_back(p);
#else
			p->cm->updateTransformMat();
#endif
		}
		p = p->pRenderNext;
	}
#ifdef MT_UpdateTransformMat
	static atomic<int> _trans_mat_flagn;
	_trans_mat_flagn = 0;
	const auto nThr = LTHP.size() + 1;
	const auto dat = to_render.data();
	deployThreadTask(to_render.size(), nThr, [=](int start, int end)
	{
		for (auto j = start; j < end; j++)
			dat[j]->cm->updateTransformMat();
		++_trans_mat_flagn;
	});
	while (_trans_mat_flagn < nThr) {}
#endif
	//XProfiler::getInstance()->toc("transform");

	// collect light sources
	vector<BaseLight*> lights;
	for (auto& o : lightSources)
	{
		if (o->cm->getLightSource())
			lights.push_back(o->cm->getLightSource());
	}

	GETOBJTABLE;  // ot

	/*GameObject* */p = m_pRenderListHeader.pRenderNext;
	CCASSERT(p != nullptr, "DoRender: GameObject is null.");
	uint32_t last_classID = 0;
	while (p && p != &m_pRenderListTail)
	{
		if (!p->hide)
		{
			currentObj = p;
#ifdef USE_CLASSID
			if (p->cls->isDefaultRender)
			{
				p->cm->applyLights(lights);// only enable light here
				DoDefaultRender(p);
			}
			else
			{
				const auto cid = p->cls->id;
				CCASSERT(cid > 0, "internal error");
				if (cid != last_classID)
				{
					lua_settop(L, 1);  // ot
					p->cls->pushLuaRenderFunc(L);  // ot f(render)
					if (lua_isnil(L, -1))
					{
						DoDefaultRender(p);
					}
					else
					{
						lua_pushvalue(L, -1);  // ot f(render) f(render)
						lua_rawgeti(L, 1, p->id + 1);  // ot f(render) f(render) obj
						lua_call(L, 1, 0);  // ot f(render)
					}
					last_classID = cid;
				}
				else
				{
					// ot f(render)
					if (lua_isnil(L, -1))
					{
						DoDefaultRender(p);
					}
					else
					{
						lua_pushvalue(L, -1);  // ot f(render) f(render)
						lua_rawgeti(L, 1, p->id + 1);  // ot f(render) f(render) obj
						lua_call(L, 1, 0);  // ot f(render)
					}
				}
			}
#else
			// rawgeti*3
			lua_rawgeti(L, -1, p->id + 1);  // ot obj
			lua_rawgeti(L, -1, 1);  // ot obj cls
			lua_rawgeti(L, -1, LGOBJ_CC_RENDER);  // ot obj cls f(render)
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 3);  // ot
				DoDefaultRender(p);
			}
			else
			{
				lua_pushvalue(L, -3);  // ot obj cls f(render) obj
				lua_call(L, 1, 0);  // ot obj cls
				lua_pop(L, 2);  // ot
			}
#endif
		}
		p = p->pRenderNext;
	}
	currentObj = nullptr;
	lua_pop(L, 1);
	inDoRender = false;
}

void GameObjectPool::BoundCheck()noexcept
{
#ifdef MT_BoundCheck
	vector<GameObject*> to_bc;
	to_bc.reserve(GetObjectCount());
	GameObject* obj = m_pObjectListHeader.pObjectNext;
	while (obj && obj != &m_pObjectListTail)
	{
		to_bc.push_back(obj);
		obj = obj->pObjectNext;
	}

	const auto nThr = LTHP.size() + 1;
	auto pros = new promise<vector<GameObject*>>[nThr];
	const auto dat = to_bc.data();
	deployThreadTask(to_bc.size(), nThr, [=](int start, int end, int i)
	{
		vector<GameObject*> ret;
		for (auto j = start; j < end; j++)
		{
			if (obj->bound && !dat[j]->cm->boundCheck(_boundLeft, _boundRight, _boundBottom, _boundTop))
				ret.push_back(dat[j]);
		}
		pros[i].set_value(ret);
	});
	vector<GameObject*> result;
	for (int i = 0; i < nThr; ++i)
	{
		auto ret = pros[i].get_future().get();
		result.insert(result.end(), ret.begin(), ret.end());
	}
	delete[] pros;

	GETOBJTABLE;  // ot
	for (auto& p : result)
	{
		// set to STATUS_DEL when out of bound
		p->status = STATUS_DEL;
		// id -> lua table -> class -> delfunc
		lua_rawgeti(L, -1, p->id + 1);  // ot obj
		lua_rawgeti(L, -1, 1);  // ot obj cls
		lua_rawgeti(L, -1, LGOBJ_CC_DEL);  // ot obj cls f(del)
		lua_pushvalue(L, -3);  // ot obj cls f(del) obj
		lua_call(L, 1, 0);  // ot obj cls
		lua_pop(L, 2);  // ot
	}
	lua_pop(L, 1);
#else
	GETOBJTABLE;  // ot
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		if (p->bound && !p->cm->boundCheck(_boundLeft, _boundRight, _boundBottom, _boundTop))
		{
			// set to STATUS_DEL when out of bound
			p->status = STATUS_DEL;
			// id -> lua table -> class -> delfunc
			lua_rawgeti(L, -1, p->id + 1);  // ot obj
			lua_rawgeti(L, -1, 1);  // ot obj cls
			lua_rawgeti(L, -1, LGOBJ_CC_DEL);  // ot obj cls f(del)
			lua_pushvalue(L, -3);  // ot obj cls f(del) obj
			lua_call(L, 1, 0);  // ot obj cls
			lua_pop(L, 2);  // ot
		}
		p = p->pObjectNext;
	}
	lua_pop(L, 1);
#endif
}

void GameObjectPool::CollisionCheck(size_t groupA, size_t groupB)noexcept
{
	if (groupA >= maxGroupCount || groupB >= maxGroupCount)
		luaL_error(L, "Invalid collision group.");

	GETOBJTABLE;  // ot

	GameObject* pA = m_pCollisionListHeader[groupA].pCollisionNext;
	const auto pATail = &m_pCollisionListTail[groupA];
	const auto pBHeader = m_pCollisionListHeader[groupB].pCollisionNext;
	const auto pBTail = &m_pCollisionListTail[groupB];

	if (groupA != groupB)
	{
#ifdef MT_CollisionCheck
		static vector<GameObject*> to_colli_lhs;
		static vector<GameObject*> to_colli_rhs;
		const auto nobj = GetObjectCount();
		to_colli_lhs.clear();
		to_colli_lhs.reserve(nobj);
		to_colli_rhs.clear();
		to_colli_rhs.reserve(nobj);
		array<vector<GameObject*>, 4> ret_colli_lhs;
		array<vector<GameObject*>, 4> ret_colli_rhs;
		auto ret_colli_l_p = ret_colli_lhs.data();
		auto ret_colli_r_p = ret_colli_rhs.data();

		while (pA && pA != pATail)
		{
			auto pB = pBHeader;
			while (pB && pB != pBTail)
			{
				if (pA->colli&&pB->colli)
				{
					to_colli_lhs.push_back(pA);
					to_colli_rhs.push_back(pB);
				}
				pB = pB->pCollisionNext;
			}
			pA = pA->pCollisionNext;
		}
		const auto size = to_colli_lhs.size();
		auto dat_l = to_colli_lhs.data();
		auto dat_r = to_colli_rhs.data();
		static atomic<int> _colli_flagn;
		_colli_flagn = 0;
		const auto nThr = LTHP.size() + 1;
		// not good to reserve nobj
		for (auto i = 0; i < nThr; ++i)
		{
			ret_colli_l_p[i].reserve(nobj);
			ret_colli_r_p[i].reserve(nobj);
		}
		deployThreadTask(size, nThr, [=](int start, int end, int i)
		{
			for (auto j = start; j < end; j++)
			{
				if (::CollisionCheck(dat_l[j], dat_r[j]))
				{
					ret_colli_l_p[i].push_back(dat_l[j]);
					ret_colli_r_p[i].push_back(dat_r[j]);
				}
			}
			++_colli_flagn;
		});
		while (_colli_flagn < nThr) {}

		for (auto i = 0; i < 4; ++i)
		{
			const auto sz = ret_colli_lhs[i].size();
			for (auto j = 0; j < sz; ++j)
			{
				// id -> lua table -> class -> collifunc
				lua_rawgeti(L, -1, ret_colli_lhs[i][j]->id + 1);  // ot obj
				lua_rawgeti(L, -1, 1);  // ot obj cls
				lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot obj cls f(colli)
				lua_pushvalue(L, -3);  // ot obj cls f(colli) obj
				lua_rawgeti(L, -5, ret_colli_rhs[i][j]->id + 1);  // ot obj cls f(colli) obj obj
				lua_call(L, 2, 0);  // ot obj cls
				lua_pop(L, 2);  // ot
			}
		}
#else
		while (pA && pA != pATail)
		{
			auto pB = pBHeader;
			while (pB && pB != pBTail)
			{
				if (::CollisionCheck(pA, pB))
				{
					// id -> lua table -> class -> collifunc
					lua_rawgeti(L, -1, pA->id + 1);  // ot obj
					lua_rawgeti(L, -1, 1);  // ot obj cls
					lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot obj cls f(colli)
					lua_pushvalue(L, -3);  // ot obj cls f(colli) obj
					lua_rawgeti(L, -5, pB->id + 1);  // ot obj cls f(colli) obj obj
					lua_call(L, 2, 0);  // ot obj cls
					lua_pop(L, 2);  // ot
				}
				pB = pB->pCollisionNext;
			}
			pA = pA->pCollisionNext;
		}
#endif
		lua_pop(L, 1);
	}
	else
	{
		while (pA && pA != pATail)
		{
			auto pB = pBHeader;
			while (pB && pB != pBTail)
			{
				if (::CollisionCheck(pA, pB) && pA != pB)
				{
					// id -> lua table -> class -> collifunc
					lua_rawgeti(L, -1, pA->id + 1);  // ot obj
					lua_rawgeti(L, -1, 1);  // ot obj cls
					lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot obj cls f(colli)
					lua_pushvalue(L, -3);  // ot obj cls f(colli) obj
					lua_rawgeti(L, -5, pB->id + 1);  // ot obj cls f(colli) obj obj
					lua_call(L, 2, 0);  // ot obj cls
					lua_pop(L, 2);  // ot
				}
				pB = pB->pCollisionNext;
			}
			pA = pA->pCollisionNext;
		}
		lua_pop(L, 1);
	}
}

void GameObjectPool::CollisionCheck(GameObject* objectA, size_t groupB) noexcept
{
	if (!objectA)return;
	const auto groupA = objectA->cm->getDataColli()->group;
	if (groupA >= maxGroupCount || groupB >= maxGroupCount)
		luaL_error(L, "Invalid collision group.");
	const auto pA = objectA;
	GameObject* pBHeader = m_pCollisionListHeader[groupB].pCollisionNext;
	const auto pBTail = &m_pCollisionListTail[groupB];
	auto pB = pBHeader;
	GETOBJTABLE;  // ot
	// id -> lua table -> class -> collifunc
	lua_rawgeti(L, -1, pA->id + 1);  // ot objA
	lua_rawgeti(L, -1, 1);  // ot objA cls
	lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot objA cls f(colli)
	lua_remove(L, -2);  // ot objA f(colli)
	while (pB && pB != pBTail)
	{
		if (::CollisionCheck(pA, pB) && pA != pB)
		{
			lua_pushvalue(L, -1);  // ot objA f(colli) f(colli)
			lua_pushvalue(L, -3);  // ot objA f(colli) f(colli) objA
			lua_rawgeti(L, -5, pB->id + 1);  // ot objA f(colli) f(colli) objA objB
			lua_call(L, 2, 0);  // ot objA f(colli)
		}
		pB = pB->pCollisionNext;
	}
	lua_pop(L, 3);
}

void GameObjectPool::CollisionCheck(size_t groupA, GameObject* objectB) noexcept
{
	if (!objectB)return;
	const auto groupB = objectB->cm->getDataColli()->group;
	if (groupA >= maxGroupCount || groupB >= maxGroupCount)
		luaL_error(L, "Invalid collision group.");
	GameObject* pA = m_pCollisionListHeader[groupA].pCollisionNext;
	const auto pATail = &m_pCollisionListTail[groupA];
	const auto pB = objectB;
	GETOBJTABLE;  // ot
	lua_rawgeti(L, -1, pB->id + 1);  // ot objB
	while (pA && pA != pATail)
	{
		if (::CollisionCheck(pA, pB) && pA != pB)
		{
			// id -> lua table -> class -> collifunc
			lua_rawgeti(L, -2, pA->id + 1);  // ot objB objA
			lua_rawgeti(L, -1, 1);  // ot objB objA cls
			lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot objB objA cls f(colli)
			lua_pushvalue(L, -3);  // ot objB objA cls f(colli) objA
			lua_pushvalue(L, -5);  // ot objB objA cls f(colli) objA objB
			lua_call(L, 2, 0);  // ot objB objA cls
			lua_pop(L, 2);  // ot objB
		}
		pA = pA->pCollisionNext;
	}
	lua_pop(L, 1);
}

void GameObjectPool::CollisionCheck(GameObject* objectA, GameObject* objectB) noexcept
{
	if (!objectA || !objectB) return;
	if (::CollisionCheck(objectA, objectB) && objectA != objectB)
	{
		GETOBJTABLE;  // ot
		lua_rawgeti(L, -1, objectA->id + 1);  // ot objA
		lua_rawgeti(L, -1, 1);  // ot objA cls
		lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot objA cls f(colli)
		lua_pushvalue(L, -3);  // ot objA cls f(colli) objA
		lua_rawgeti(L, -5, objectB->id + 1);  // ot objA cls f(colli) objA objB
		lua_call(L, 2, 0);  // ot objA cls
		lua_pop(L, 3);
	}
}

void GameObjectPool::CollisionCheck3D(size_t groupA, size_t groupB) noexcept
{
	if (groupA >= maxGroupCount || groupB >= maxGroupCount)
		luaL_error(L, "Invalid collision group.");
	GETOBJTABLE;  // ot
	GameObject* pA = m_pCollisionListHeader[groupA].pCollisionNext;
	const auto pATail = &m_pCollisionListTail[groupA];
	const auto pBHeader = m_pCollisionListHeader[groupB].pCollisionNext;
	const auto pBTail = &m_pCollisionListTail[groupB];
	if (groupA != groupB)
	{
		while (pA && pA != pATail)
		{
			auto pB = pBHeader;
			while (pB && pB != pBTail)
			{
				if (pA->colli && pB->colli && pA->cm->collisionCheck3D(pB->cm))
				{
					// id -> lua table -> class -> collifunc
					lua_rawgeti(L, -1, pA->id + 1);  // ot obj
					lua_rawgeti(L, -1, 1);  // ot obj cls
					lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot obj cls f(colli)
					lua_pushvalue(L, -3);  // ot obj cls f(colli) obj
					lua_rawgeti(L, -5, pB->id + 1);  // ot obj cls f(colli) obj obj
					lua_call(L, 2, 0);  // ot obj cls
					lua_pop(L, 2);  // ot
				}
				pB = pB->pCollisionNext;
			}
			pA = pA->pCollisionNext;
		}
		lua_pop(L, 1);
	}
	else
	{
		while (pA && pA != pATail)
		{
			auto pB = pBHeader;
			while (pB && pB != pBTail)
			{
				if (pA->colli && pB->colli && pA->cm->collisionCheck3D(pB->cm) && pA != pB)
				{
					// id -> lua table -> class -> collifunc
					lua_rawgeti(L, -1, pA->id + 1);  // ot obj
					lua_rawgeti(L, -1, 1);  // ot obj cls
					lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot obj cls f(colli)
					lua_pushvalue(L, -3);  // ot obj cls f(colli) obj
					lua_rawgeti(L, -5, pB->id + 1);  // ot obj cls f(colli) obj obj
					lua_call(L, 2, 0);  // ot obj cls
					lua_pop(L, 2);  // ot
				}
				pB = pB->pCollisionNext;
			}
			pA = pA->pCollisionNext;
		}
		lua_pop(L, 1);
	}
}

void GameObjectPool::UpdateXY()noexcept
{
	auto p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		p->UpdateXY();
		p = p->pObjectNext;
	}
}

void GameObjectPool::AfterFrame()noexcept
{
	GameObject* p = m_pObjectListHeader.pObjectNext;
	GETOBJTABLE;
	while (p && p != &m_pObjectListTail)
	{
		p->timer++;
		p->cm->updateAni();
		p->ani_timer++;
		if (p->status != STATUS_DEFAULT)
			//p = freeObject(p);
			p = freeObjectInternal(p);
		else
			p = p->pObjectNext;
	}
	lua_pop(L, 1);

#ifdef OPT_RENDER_LIST
	if (GetObjectCount() > 32)
	{
		renderMid = m_pRenderListHeader.pRenderNext;
		for (int i = 0; i < GetObjectCount() / 2; ++i)
			renderMid = renderMid->pRenderNext;
	}
	else
	{
		renderMid = nullptr;
	}
#endif
}

int GameObjectPool::RawNew(lua_State* L) noexcept
{
	if (!checkClass(L, 1))
		return luaL_error(L, "invalid argument #1, luastg object class required for 'RawNew'.");

	const auto p = allocateObject();
	if (!p)
		return luaL_error(L, "can't alloc object, object pool may be full.");

	const auto cid = getClassID(L, 1);
	p->cls = GameClass::getByID(cid);
	if (!p->cls)
		return error_obj(L);
	p->cm->getDataTrasform()->is3D = isExtProperty3D(p);
	// obj[1]=class
	// obj[2]=id
	// setmetatable(obj, ot["mt"])
	// ot[id+1]=obj
	GETOBJTABLE;  // cls ... ot
	lua_createtable(L, 2, 0);  // cls ... ot obj
	lua_pushvalue(L, 1);  // cls ... ot obj cls
	lua_rawseti(L, -2, 1);  // cls ... ot obj  // set class
	lua_pushinteger(L, lua_Integer(p->id));  // cls ... ot obj id
	lua_rawseti(L, -2, 2);  // cls ... ot obj  // set id
	lua_getfield(L, -2, METATABLE_OBJ);  // cls ... ot obj mt
	lua_setmetatable(L, -2);  // cls ... ot obj  // set metatable
	lua_pushvalue(L, -1);  // cls ... ot obj obj
	lua_rawseti(L, -3, p->id + 1);  // cls ... ot obj  // store in ot

	lua_insert(L, 1);  // obj cls ... ot
	lua_settop(L, 1);  // obj

	const auto tr = p->cm->getDataTrasform();
	tr->lastx = tr->x;
	tr->lasty = tr->y;
	return 1;
}

int GameObjectPool::New(lua_State* L)noexcept
{
	if (!checkClass(L, 1))
		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");

	auto p = allocateObject();
	if (!p)
		return luaL_error(L, "can't alloc object, object pool may be full.");

	const auto cid = getClassID(L, 1);
	p->cls = GameClass::getByID(cid);
	if (!p->cls)
		return error_obj(L);
	p->cm->getDataTrasform()->is3D = isExtProperty3D(p);

	// obj[1]=class
	// obj[2]=id
	// setmetatable(obj, ot["mt"])
	// ot[id+1]=obj
	// class.init(obj, ...)

	GETOBJTABLE;  // cls ... ot
	lua_createtable(L, 2, 0);  // cls ... ot obj
	lua_pushvalue(L, 1);  // cls ... ot obj cls
	lua_rawseti(L, -2, 1);  // cls ... ot obj  // set class
	lua_pushinteger(L, lua_Integer(p->id));  // cls ... ot obj id
	lua_rawseti(L, -2, 2);  // cls ... ot obj  // set id
	lua_getfield(L, -2, METATABLE_OBJ);  // cls ... ot obj mt
	lua_setmetatable(L, -2);  // cls ... ot obj  // set metatable
	lua_pushvalue(L, -1);  // cls ... ot obj obj
	lua_rawseti(L, -3, p->id + 1);  // cls ... ot obj  // store in ot
	lua_insert(L, 1);  // obj cls ... ot
	lua_pop(L, 1);  // obj cls ...
	lua_rawgeti(L, 2, LGOBJ_CC_INIT);  // obj cls ... f(init)
	lua_insert(L, 3);  // obj cls f(init) ...
	lua_pushvalue(L, 1);  // obj cls f(init) ... obj
	lua_insert(L, 4);  // obj cls f(init) obj ...
	lua_call(L, lua_gettop(L) - 3, 0);  // obj cls  // init callback

	//note: f(init) may change class
	lua_pop(L, 1);

	const auto tr = p->cm->getDataTrasform();
	tr->lastx = tr->x;
	tr->lasty = tr->y;
	return 1;
}
//TODO:
#ifdef USE_ComponentManager 
int GameObjectPool::Clone(lua_State* L, int idx) noexcept { return 0; }
#else
int GameObjectPool::Clone(lua_State* L, int idx) noexcept
{
	auto p = checkObject(L, idx);
	if (!p)
		return luaL_error(L, "not a valid luastg object for 'Clone'.");
	const auto pNew = allocateObject();
	if (!pNew)
		return luaL_error(L, "can't alloc object, object pool may be full.");
	pNew->CopyAttrFrom(p);
	pNew->layer++;
	pNew->group++;
	setObjectLayer(pNew, p->layer);
	setObjectGroup(pNew, p->group);
	if (pNew->res)
	{
		if (pNew->res->getType() == ResourceType::Particle)
		{
			pNew->ps = dynamic_cast<ResParticle*>(pNew->res)->AllocInstance();
			pNew->ps->setInactive();
			pNew->ps->setCenter(Vec2(float(pNew->x), float(pNew->y)));
			pNew->ps->setRotation(float(pNew->rot));
			pNew->ps->setActive();
		}
		pNew->res->retain();
	}

	// obj[1]=class
	// obj[2]=id
	// setmetatable(obj, ot["mt"])
	// ot[id+1]=obj

	GETOBJTABLE;  // ... ot
	lua_createtable(L, 2, 0);  // ... ot obj
	if (idx < 0)idx = idx - 2;
	lua_rawgeti(L, idx, 1);  // ... ot obj class
	lua_rawseti(L, -2, 1);  // ... ot obj

	lua_pushinteger(L, lua_Integer(pNew->id));  // ... ot obj id
	lua_rawseti(L, -2, 2);  // ... ot obj

	lua_getfield(L, -2, METATABLE_OBJ);  // ... ot obj mt
	lua_setmetatable(L, -2);  // ... ot obj

	lua_pushvalue(L, -1);  // ... ot obj obj
	lua_rawseti(L, -3, pNew->id + 1);  // ... ot obj

	lua_remove(L, -2);  // ... obj
	return 1;
}
#endif // USE_ComponentManager

void GameObjectPool::callBack(lua_State* L, GameObject* p, int callBack, bool hasParam)
{
	if (!p) return;
	// note: prepare stack
	if (hasParam)
	{
		// obj ...
		lua_rawgeti(L, 1, 1);  // obj ... class
		lua_rawgeti(L, -1, callBack);  // obj ... class f
		lua_insert(L, 1);  // f obj ... class
		lua_pop(L, 1);  // f obj ...
		lua_call(L, lua_gettop(L) - 1, 0); //
	}
	else
	{
		// ... obj
		lua_rawgeti(L, -1, 1);  // ... obj class
		lua_rawgeti(L, -1, callBack);  // ... obj class f
		lua_insert(L, -3);  // ... f obj class
		lua_pop(L, 1);     // ... f obj
		lua_call(L, 1, 0); // ...
	}
}

int GameObjectPool::del_or_kill(lua_State* L, GAMEOBJECTSTATUS status, int callBackIdx)
{
	const auto type = lua_type(L, 1);
	if (type == LUA_TTABLE)
	{
		lua_rawgeti(L, 1, 2);
		const auto type2 = lua_type(L, -1);
		if (type2 == LUA_TNUMBER) // F(obj)
		{
			// obj ... id
			auto p = _objectPool.Data(size_t(luaL_checknumber(L, -1)));
			lua_pop(L, 1);  // obj ...
			if (!p)
				return error_obj(L);
			if (p->status == STATUS_DEFAULT)
			{
				p->status = status;
				callBack(L, p, callBackIdx, true);
			}
			return 0;
		}
		else // F({obj})
		{
			lua_settop(L, 1);
			// t
			const auto num = lua_objlen(L, 1);
			for (auto i = 1u; i <= num; ++i)
			{
				lua_rawgeti(L, 1, i); // t obj
				lua_rawgeti(L, 1, 2); // t obj id
				auto p = _objectPool.Data(size_t(luaL_checknumber(L, -1)));
				lua_pop(L, 1);  // t obj
				if (!p)
					return error_obj(L);
				if (p->status == STATUS_DEFAULT)
				{
					p->status = status;
					callBack(L, p, callBackIdx, false); // t
				}
			}
			return 0;
		}
	}
	else if (type == LUA_TNUMBER) // F(group)
	{
		const auto group = lua_tointeger(L, 1);
		if (group < 0 || group >= maxGroupCount)
			return luaL_error(L, "Invalid collision group");
		auto p = m_pCollisionListHeader[group].pCollisionNext;
		const auto tail = &m_pCollisionListTail[group];
		// note: no extra param
		lua_settop(L, 0);
		GETOBJTABLE; // ot
		while (p && p != tail)
		{
			if (p->status == STATUS_DEFAULT)
			{
				p->status = status;
				lua_rawgeti(L, -1, p->id + 1); // ot obj
				callBack(L, p, callBackIdx, false); // ot
			}
			p = p->pCollisionNext;
		}
		return 0;
	}
	return luaL_error(L, "invalid argument #1");
}

int GameObjectPool::Del(lua_State* L)noexcept
{
	return del_or_kill(L, STATUS_DEL, LGOBJ_CC_DEL);
}

int GameObjectPool::Kill(lua_State* L)noexcept
{
	return del_or_kill(L, STATUS_KILL, LGOBJ_CC_KILL);
}

int GameObjectPool::IsValid(lua_State* L)noexcept
{
	lua_pushboolean(L, checkObject(L, -1) ? 1 : 0);
	return 1;
}

bool GameObjectPool::Angle(size_t idA, size_t idB, double& out)noexcept
{
	const auto pA = _objectPool.Data(idA);
	const auto pB = _objectPool.Data(idB);
	if (!pA || !pB)
		return false;
	const auto trA = pA->cm->getDataTrasform();
	const auto trB = pB->cm->getDataTrasform();
	out = LRAD2DEGREE * atan2(trB->y - trA->y, trB->x - trA->x);
	return true;
}

bool GameObjectPool::Dist(size_t idA, size_t idB, double& out)noexcept
{
	const auto pA = _objectPool.Data(idA);
	const auto pB = _objectPool.Data(idB);
	if (!pA || !pB)
		return false;
	const auto trA = pA->cm->getDataTrasform();
	const auto trB = pB->cm->getDataTrasform();
	const auto dx = trB->x - trA->x;
	const auto dy = trB->y - trA->y;
	out = sqrt(dx * dx + dy * dy);
	return true;
}

bool GameObjectPool::GetV(size_t id, double& v, double& a)noexcept
{
	const auto p = _objectPool.Data(id);
	if (!p)
		return false;
	const auto tr = p->cm->getDataTrasform();
	v = sqrt(tr->vx * tr->vx + tr->vy * tr->vy);
	a = atan2(tr->vy, tr->vx) * LRAD2DEGREE;
	return true;
}

bool GameObjectPool::SetV(size_t id, double v, double a, bool updateRot)noexcept
{
	const auto p = _objectPool.Data(id);
	if (!p)
		return false;
	const auto rad = a * LDEGREE2RAD;
	const auto tr = p->cm->getDataTrasform();
	tr->vx = v * cos(rad);
	tr->vy = v * sin(rad);
	if (updateRot)
	{
		tr->rot = rad;
		p->cm->setTransformDirty(true);
	}
	return true;
}

bool GameObjectPool::GetLastXY(size_t id, double& x, double& y) noexcept
{
	const auto p = _objectPool.Data(id);
	if (!p)
		return false;
	const auto tr = p->cm->getDataTrasform();
	x = tr->lastx;
	y = tr->lasty;
	return true;
}

bool GameObjectPool::SetLastXY(size_t id, double x, double y) noexcept
{
	auto p = _objectPool.Data(id);
	if (!p)
		return false;
	const auto tr = p->cm->getDataTrasform();
	tr->lastx = x;
	tr->lasty = y;
	return true;
}

bool GameObjectPool::SetImgState(size_t id, BlendMode* m, const Color4B& c)noexcept
{
	GameObject* p = _objectPool.Data(id);
	if (!p)
		return false;
	if (p->res)
	{
		switch (p->res->getType())
		{
		case ResourceType::Sprite:
			{
				auto s = static_cast<ResSprite*>(p->res);
				s->setBlendMode(m);
				s->setColor(c);
			}
			break;				
		case ResourceType::Animation:
			{
				auto ani = static_cast<ResAnimation*>(p->res);
				ani->setBlendMode(m);
				ani->setColor(c);
			}
			break;
		default:
			break;
		}
	}
	return true;
}

bool GameObjectPool::BoxCheck(size_t id,
	double left, double right, double bottom, double top, bool& ret)noexcept
{
	const auto p = _objectPool.Data(id);
	if (!p)
		return false;
	ret = p->cm->boundCheck(left, right, bottom, top);
	return true;
}

void GameObjectPool::ResetPool()noexcept
{
	GETOBJTABLE;
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p != &m_pObjectListTail)
		//p = freeObject(p);
		p = freeObjectInternal(p);
	lua_pop(L, 1);
	renderMid = nullptr;
}

void GameObjectPool::UpdateParticle(GameObject* p) noexcept
{
	if (!p) return;
	p->cm->updateParticle();
}

bool GameObjectPool::UpdateParticle(size_t id) noexcept
{
	auto p = _objectPool.Data(id);
	if (!p)return false;
	UpdateParticle(p);
	return true;
}

bool GameObjectPool::DoDefaultRender(size_t id)noexcept
{
	return DoDefaultRender(_objectPool.Data(id));
}

using QUEUE_GROUP = RenderQueue::QUEUE_GROUP;
bool GameObjectPool::DoDefaultRender(GameObject* p) noexcept
{
	if (!p)
		return false;
	if (p->res)
	{
		//const auto factor = LRES.GetGlobalImageScaleFactor();
		if (p->cm->isTransformDirty())
			p->cm->updateTransformMat();
		switch (p->res->getType())
		{
		case ResourceType::Sprite:
			p->cm->renderSprite((ResSprite*)p->res);
			break;
		case ResourceType::Animation:
			p->cm->renderAnimation((ResAnimation*)p->res);
			break;
		case ResourceType::Particle:
			p->cm->renderParticle();
			break;
		case ResourceType::Font:
			p->cm->renderLabel();
			break;
		case ResourceType::Texture:
			p->cm->renderTexture((ResTexture*)p->res);
			break;
		default:
			break;
		}
	}
	if (p->cm->getBindNode())
	{
		LRR.flushTriangles();
		const auto node = p->cm->getBindNode();
		const auto sp3d = dynamic_cast<Sprite3D*>(node);
		const auto zorder = node->getGlobalZOrder();
		if (zorder != 0.f || sp3d)
		{
			const auto vp_store = LMP.applyArray<GLint, 4>();
			const auto proj_store = LMP.getMat4();
			auto vp = LRR.getCurrentViewport();
			const auto mt = LRR.getCurrentProjection();
			const auto f1 = [=]() {
				glGetIntegerv(GL_VIEWPORT, vp_store->data());
				*proj_store = Director::getInstance()->getProjectionMatrix(0);

				glViewport(vp._left, vp._bottom, vp._width, vp._height);
				Director::getInstance()->loadProjectionMatrix(mt, 0);
			};
			const auto f2 = [=]() {
				glViewport(vp_store->at(0), vp_store->at(1), vp_store->at(2), vp_store->at(3));
				Director::getInstance()->loadProjectionMatrix(*proj_store, 0);
			};
			if (zorder != 0.f)
			{
				LRR.pushCustomCommend(QUEUE_GROUP::GLOBALZ_NEG, zorder, f1);
				node->visit();
				LRR.pushCustomCommend(QUEUE_GROUP::GLOBALZ_NEG, zorder, f2);
			}
			else
			{
				LRR.pushCustomCommend(QUEUE_GROUP::OPAQUE_3D, zorder, f1);
				LRR.pushCustomCommend(QUEUE_GROUP::TRANSPARENT_3D, zorder, f1);
				node->visit();
				LRR.pushCustomCommend(QUEUE_GROUP::OPAQUE_3D, zorder, f2);
				LRR.pushCustomCommend(QUEUE_GROUP::TRANSPARENT_3D, zorder, f2);
			}
		}
		else
		{
			node->visit();
		}
	}
	return true;
}

int GameObjectPool::NextObject(int groupId, int id)noexcept
{
	if (id < 0)
		return -1;

	GameObject* p = _objectPool.Data(static_cast<size_t>(id));
	if (!p)
		return -1;

	// traverse all if not a valid group
	if (groupId < 0 || groupId >= maxGroupCount)
	{
		p = p->pObjectNext;
		if (p == &m_pObjectListTail)
			return -1;
		else
			return static_cast<int>(p->id);
	}
	else
	{
		if (p->cm->getDataColli()->group != groupId)
			return -1;
		p = p->pCollisionNext;
		if (p == &m_pCollisionListTail[groupId])
			return -1;
		else
			return static_cast<int>(p->id);
	}
}

int GameObjectPool::NextObject(lua_State* L)noexcept
{
	int g = luaL_checkinteger(L, 1);  // i(groupId)
	const int id = luaL_checkinteger(L, 2);  // id
	if (id < 0)
		return 0;

	lua_pushinteger(L, NextObject(g, id));  // ??? i(next)
	GETOBJTABLE;  // ??? i(next) ot
	lua_rawgeti(L, -1, id + 1);  // ??? i(next) ot obj
	lua_remove(L, -2);  // ??? i(next) obj
	return 2;
}

int GameObjectPool::FirstObject(int groupId)noexcept
{
	GameObject* p;

	// traverse all if not a valid group
	if (groupId < 0 || groupId >= maxGroupCount)
	{
		p = m_pObjectListHeader.pObjectNext;
		if (p == &m_pObjectListTail)
			return -1;
		else
			return static_cast<int>(p->id);
	}
	else
	{
		p = m_pCollisionListHeader[groupId].pCollisionNext;
		if (p == &m_pCollisionListTail[groupId])
			return -1;
		else
			return static_cast<int>(p->id);
	}
}

int GameObjectPool::GetAttr(lua_State* L)noexcept
{
	lua_rawgeti(L, 1, 2);  // obj s(key) ??? i(id)
	const auto id = size_t(lua_tonumber(L, -1));
	lua_pop(L, 1);  // obj s(key)
	GameObject* p = _objectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for '__index' meta operation.");

	// property
	size_t strlen;
	const char* key = luaL_checklstring(L, 2, &strlen);

	// x, y
	if (key[1] == '\0')
	{
		const auto k0 = key[0];
		if (k0 == 'x')
		{
			lua_pushnumber(L, p->cm->getDataTrasform()->x);
			return 1;
		}
		if (k0 == 'y')
		{
			lua_pushnumber(L, p->cm->getDataTrasform()->y);
			return 1;
		}
	}
	// others
	const auto cm = p->cm;
	switch (GameObjectPropertyHash(key))
	{
	case GameObjectProperty::ROT:
		lua_pushnumber(L, cm->getDataTrasform()->rot * LRAD2DEGREE);
		break;
	case GameObjectProperty::OMEGA:
		lua_pushnumber(L, cm->getDataTrasform()->omega * LRAD2DEGREE);
		break;
	case GameObjectProperty::TIMER:
		lua_pushinteger(L, p->timer);
		break;
	case GameObjectProperty::DX:
		lua_pushnumber(L, cm->getDataTrasform()->dx);
		break;
	case GameObjectProperty::DY:
		lua_pushnumber(L, cm->getDataTrasform()->dy);
		break;
	case GameObjectProperty::VX:
		lua_pushnumber(L, cm->getDataTrasform()->vx);
		break;
	case GameObjectProperty::VY:
		lua_pushnumber(L, cm->getDataTrasform()->vy);
		break;
	case GameObjectProperty::AX:
		lua_pushnumber(L, cm->getDataTrasform()->ax);
		break;
	case GameObjectProperty::AY:
		lua_pushnumber(L, cm->getDataTrasform()->ay);
		break;
	case GameObjectProperty::LAYER:
		lua_pushnumber(L, p->layer);
		break;
	case GameObjectProperty::GROUP:
		lua_pushinteger(L, cm->getDataColli()->group);
		break;
	case GameObjectProperty::HIDE:
		lua_pushboolean(L, p->hide);
		break;
	case GameObjectProperty::BOUND:
		lua_pushboolean(L, p->bound);
		break;
	case GameObjectProperty::NAVI:
		lua_pushboolean(L, cm->getDataTrasform()->navi);
		break;
	case GameObjectProperty::COLLI:
		lua_pushboolean(L, p->colli);
		break;
	case GameObjectProperty::STATUS:
		switch (p->status)
		{
		case STATUS_DEFAULT:
			lua_pushstring(L, "normal");
			break;
		case STATUS_KILL:
			lua_pushstring(L, "kill");
			break;
		case STATUS_DEL:
			lua_pushstring(L, "del");
			break;
		default:
			CCASSERT(false, "GetAttr: Wrong status.");
			break;
		}
		break;
	case GameObjectProperty::HSCALE:
		lua_pushnumber(L, cm->getDataTrasform()->hscale);
		break;
	case GameObjectProperty::VSCALE:
		lua_pushnumber(L, cm->getDataTrasform()->vscale);
		break;
	case GameObjectProperty::CLASS:
		lua_rawgeti(L, 1, 1);
		break;
	case GameObjectProperty::A:
		lua_pushnumber(L, cm->getDataColli()->a / L_IMG_FACTOR);
		break;
	case GameObjectProperty::B:
		lua_pushnumber(L, cm->getDataColli()->b / L_IMG_FACTOR);
		break;
	case GameObjectProperty::RECT:
		lua::ColliderType_to_luaval(L, cm->getDataColli()->type);
		break;
	case GameObjectProperty::IMG:
		if (p->res)
			lua_pushstring(L, p->res->getName().c_str());
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::ANI:
		if(cm->getDataAni())
			lua_pushinteger(L, cm->getDataAni()->timer);
		else
			lua_pushinteger(L, p->ani_timer);
		break;
	case GameObjectProperty::RES:
		if (isExtProperty(p))
		{
			if (p->res)
				p->cm->pushLua(L, p->res);
			else
				lua_pushnil(L);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::BLEND:
		if (isExtProperty(p))
		{
			const auto dat = p->cm->getDataBlend();
			if (dat)
				//lua::BlendMode_to_luaval(L, dat->blendMode);
				lua_pushstring(L, dat->blendMode->getName().c_str());
			else
				lua_pushnil(L);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::COLOR:
		if (isExtProperty(p))
		{
			const auto dat = p->cm->getDataBlend();
			if (dat)
				lua::_color4b_to_luaval(L, dat->blendColor);
			else
				lua_pushnil(L);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::_A:
		if (isExtProperty(p))
		{
			const auto blend = p->cm->getOrCreateBlend();
			lua_pushinteger(L, blend->blendColor.a);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::_R:
		if (isExtProperty(p))
		{
			const auto blend = p->cm->getOrCreateBlend();
			lua_pushinteger(L, blend->blendColor.r);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::_G:
		if (isExtProperty(p))
		{
			const auto blend = p->cm->getOrCreateBlend();
			lua_pushinteger(L, blend->blendColor.g);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::_B:
		if (isExtProperty(p))
		{
			const auto blend = p->cm->getOrCreateBlend();
			lua_pushinteger(L, blend->blendColor.b);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::LIGHT:
		if (isExtProperty(p))
		{
			// 1.light source 2.light flag 3.nil
			const auto lightSource = p->cm->getLightSource();
			if (lightSource)
			{
				lua::BaseLight_to_luaval(L, lightSource);
				break;
			}
			const auto data = p->cm->getDataLight();
			if (data)
			{
				lua_pushnumber(L, data->lightFlag);
				break;
			}
			lua_pushnil(L);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::SHADER:
		if (isExtProperty(p))
		{
			const auto state = p->cm->getProgramStateCopy();
			object_to_luaval(L, "cc.GLProgramState", state);
		}
		else
			lua_pushnil(L);
		break;

#define GET_3D_VALUE(_P) if (isExtProperty3D(p))\
		lua_pushnumber(L, cm->getDataTrasform()->_P);\
		else lua_pushnil(L); break;

	case GameObjectProperty::Z:
		GET_3D_VALUE(z);
	case GameObjectProperty::DZ:
		GET_3D_VALUE(dz);
	case GameObjectProperty::VZ:
		GET_3D_VALUE(vz);
	case GameObjectProperty::AZ:
		GET_3D_VALUE(az);
	case GameObjectProperty::ZSCALE:
		GET_3D_VALUE(zscale);
	case GameObjectProperty::QUAT:
		if (isExtProperty3D(p))
		{
			Quaternion q;
			Quaternion::createFromAxisAngle(
				cm->getDataTrasform()->rotAxis, cm->getDataTrasform()->rot, &q);
			quaternion_to_luaval(L, q);
		}
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::X:
	case GameObjectProperty::Y:
	default:
		lua_pushnil(L);
		break;
	}
	return 1;
}

int GameObjectPool::SetAttr(lua_State* L)noexcept
{
	lua_rawgeti(L, 1, 2);  // obj s(key) any(v) i(id)
	const auto id = static_cast<size_t>(lua_tonumber(L, -1));
	lua_pop(L, 1);  // obj s(key) any(v)

	GameObject* p = _objectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for '__newindex' meta operation.");

	// property
	size_t strlen;
	const char* key = luaL_checklstring(L, 2, &strlen);

	// x, y
	if (key[0] == 'x' && key[1] == '\0')
	{
		p->cm->getDataTrasform()->x = luaL_checknumber(L, 3);
		p->cm->setTransformDirty(true);
		return 0;
	}
	else if (key[0] == 'y' && key[1] == '\0')
	{
		p->cm->getDataTrasform()->y = luaL_checknumber(L, 3);
		p->cm->setTransformDirty(true);
		return 0;
	}	
	// others
	const auto cm = p->cm;
	switch (GameObjectPropertyHash(key))
	{
	case GameObjectProperty::DX:
		return luaL_error(L, "property 'dx' is readonly");
	case GameObjectProperty::DY:
		return luaL_error(L, "property 'dy' is readonly");
	case GameObjectProperty::ROT:
		cm->getDataTrasform()->rot = luaL_checknumber(L, 3) * LDEGREE2RAD;
		p->cm->setTransformDirty(true);
		break;
	case GameObjectProperty::OMEGA:
		cm->getDataTrasform()->omega = luaL_checknumber(L, 3) * LDEGREE2RAD;
		break;
	case GameObjectProperty::TIMER:
		p->timer = luaL_checkinteger(L, 3);
		break;
	case GameObjectProperty::VX:
		cm->getDataTrasform()->vx = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::VY:
		cm->getDataTrasform()->vy = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::AX:
		cm->getDataTrasform()->ax = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::AY:
		cm->getDataTrasform()->ay = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::LAYER:
		setObjectLayer(p, luaL_checknumber(L, 3));
		break;
	case GameObjectProperty::GROUP:
		setObjectGroup(p, luaL_checkinteger(L, 3));
		break;
	case GameObjectProperty::HIDE:
		p->hide = lua_toboolean(L, 3) != 0;
		break;
	case GameObjectProperty::BOUND:
		p->bound = lua_toboolean(L, 3) != 0;
		break;
	case GameObjectProperty::NAVI:
		cm->getDataTrasform()->navi = lua_toboolean(L, 3) != 0;
		break;
	case GameObjectProperty::COLLI:
		p->colli = lua_toboolean(L, 3) != 0;
		break;
	case GameObjectProperty::STATUS:
		do {
			const char* val = luaL_checkstring(L, 3);
			if (strcmp(val, "normal") == 0)
				p->status = STATUS_DEFAULT;
			else if (strcmp(val, "del") == 0)
				p->status = STATUS_DEL;
			else if (strcmp(val, "kill") == 0)
				p->status = STATUS_KILL;
			else
				return error_prop(L, "status");
		} while (false);
		break;
	case GameObjectProperty::HSCALE:
		cm->getDataTrasform()->hscale = luaL_checknumber(L, 3);
		p->cm->setTransformDirty(true);
		break;
	case GameObjectProperty::VSCALE:
		cm->getDataTrasform()->vscale = luaL_checknumber(L, 3);
		p->cm->setTransformDirty(true);
		break;
	case GameObjectProperty::CLASS:
		{
			// obj s(key) cls
			lua_rawgeti(L, -1, 7);// obj s(key) cls cid
			const auto cid = luaL_checkinteger(L, -1);
			const auto cls = GameClass::getByID(cid);
			if (!cls)
				return error_prop(L, "class");
			p->cls = cls;
			lua_pop(L, 1);
			lua_rawseti(L, 1, 1);
			p->cm->getDataTrasform()->is3D = isExtProperty3D(p);
		}
		break;
	case GameObjectProperty::A:
	{
		const auto v = luaL_checknumber(L, 3);
		if (v < 0.0)
			return luaL_error(L, "invalid negative value for property 'a': %f", v);
		cm->getDataColli()->a = v * L_IMG_FACTOR;
		cm->updateColli();
	}
		break;
	case GameObjectProperty::B:
	{
		const auto v = luaL_checknumber(L, 3);
		if (v < 0.0)
			return luaL_error(L, "invalid negative value for property 'b': %f", v);
		cm->getDataColli()->b = v * L_IMG_FACTOR;
		cm->updateColli();
	}
		break;
	case GameObjectProperty::RECT:
		if (!lua::luaval_to_ColliderType(L, 3, &cm->getDataColli()->type))
			return error_prop(L, "rect");
		cm->updateColli();
		break;
	case GameObjectProperty::IMG:
		if (!setObjectResource(p, L, 3))
			return luaL_error(L, "can't set resource");
		break;
	case GameObjectProperty::ANI:
		if (cm->getDataAni())
			cm->getDataAni()->timer = luaL_checknumber(L, 3);
		else
			p->ani_timer = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::RES:
		if (isExtProperty(p))
		{
			// readonly
		}
		else
			lua_rawset(L, 1);
		break;
	case GameObjectProperty::BLEND:
		if (isExtProperty(p))
		{
			BlendMode* v = nullptr;
			lua::luaval_to_BlendMode(L, 3, &v);
			if(!v)
				return error_prop(L, "blend");
			const auto blend = p->cm->getOrCreateBlend();
			blend->blendMode = v;
		}
		else
			lua_rawset(L, 1);
		break;
	case GameObjectProperty::COLOR:
		if (isExtProperty(p))
		{
			Color4B c;
			if (lua_type(L, 3) == LUA_TNUMBER)
			{
				const uint32_t val = luaL_checkinteger(L, 3);
				c.a = val >> 24;
				c.r = val >> 16;
				c.g = val >> 8;
				c.b = val;
			}
			else if(!lua::_luaval_to_color4b(L, 3, &c))
			{
				return error_prop(L, "color");
			}
			const auto blend = p->cm->getOrCreateBlend();
			blend->blendColor = c;
			blend->useColor = true;
		}
		else
			lua_rawset(L, 1);
		break;

#define  SET_COLOR_VALUE(_P) if (isExtProperty(p)){\
			const auto blend = p->cm->getOrCreateBlend();\
			blend->blendColor._P = luaL_checkinteger(L, 3);\
			blend->useColor = true;\
		}else lua_rawset(L, 1); break;

	case GameObjectProperty::_A:
		SET_COLOR_VALUE(a);
	case GameObjectProperty::_R:
		SET_COLOR_VALUE(r);
	case GameObjectProperty::_G:
		SET_COLOR_VALUE(g);
	case GameObjectProperty::_B:
		SET_COLOR_VALUE(b);
	case GameObjectProperty::LIGHT:
		if (isExtProperty(p))
		{
			const auto type = lua_type(L, 3);
			if (type == LUA_TNUMBER)
			{
				const auto flag = luaL_checkinteger(L, 3);
				if (flag == 0)
				{
					p->cm->setDataLight(nullptr);
				}
				else
				{
					const auto data = p->cm->getOrCreateLight();
					data->lightFlag = luaL_checknumber(L, 3);
				}
			}
			else if (type == LUA_TUSERDATA)
			{
				const auto source = lua::tousertype<BaseLight>(L, 3, "cc.BaseLight");
				if (source)
				{
					p->cm->setLightSource(source);
					lightSources.insert(p);
				}
			}
			else if (type == LUA_TNIL)
			{
				p->cm->setLightSource(nullptr);
				lightSources.erase(p);
			}
		}
		else
			lua_rawset(L, 1);
		break;
	case GameObjectProperty::SHADER:
		if (isExtProperty(p))
		{
			// 1.string 2.ResFX 3.GLProgramState 4.nil
			const auto type = lua_type(L, 3);
			GLProgramState* state = nullptr;
			if (type == LUA_TSTRING)
			{
				state = GLProgramState::getOrCreateWithGLProgramName(lua_tostring(L, 3));
			}
			else if (type == LUA_TUSERDATA)
			{
				do
				{
					const auto fx = lua::tousertype<ResFX>(L, 3, "lstg.ResFX");
					if (fx)
						state = fx->getProgramState();
					if (state) break;
					state = lua::tousertype<GLProgramState>(L, 3, "cc.GLProgramState");
				}
				while (false);
			}
			if (type == LUA_TNIL)
				p->cm->setProgramStateCopy(nullptr);
			else if (state)
				p->cm->setProgramStateCopy(state->clone());
		}
		else
			lua_rawset(L, 1);
		break;

#define SET_3D_VALUE(_P) if (isExtProperty3D(p))\
		cm->getDataTrasform()->_P = luaL_checknumber(L, 3);\
		else lua_rawset(L, 1); break;

	case GameObjectProperty::Z:
		if (isExtProperty3D(p))
		{
			cm->getDataTrasform()->z = luaL_checknumber(L, 3);
			p->cm->setTransformDirty(true);
		}
		else lua_rawset(L, 1); break;
	case GameObjectProperty::DZ:
		if (isExtProperty3D(p))
			return luaL_error(L, "property 'dz' is readonly");
		else
			lua_rawset(L, 1);
		break;
	case GameObjectProperty::VZ:
		SET_3D_VALUE(vz);
	case GameObjectProperty::AZ:
		SET_3D_VALUE(az);
	case GameObjectProperty::ZSCALE:
		if (isExtProperty3D(p))
		{
			cm->getDataTrasform()->zscale = luaL_checknumber(L, 3);
			p->cm->setTransformDirty(true);
		}
		else lua_rawset(L, 1); break;
	case GameObjectProperty::QUAT:
		if (isExtProperty3D(p))
		{
			const auto type = lua_type(L, 3);
			if (type == LUA_TTABLE)
			{
				lua_pushstring(L, "w");
				lua_rawget(L, 3);
				if (lua_type(L, -1) == LUA_TNUMBER)
				{
					Quaternion q;
					luaval_to_quaternion(L, 3, &q);
					cm->getDataTrasform()->rot =
						q.toAxisAngle(&(cm->getDataTrasform()->rotAxis));
				}
				else
				{
					luaval_to_vec3(L, 3, &(cm->getDataTrasform()->rotAxis));
					cm->getDataTrasform()->rotAxis.normalize();
				}
				p->cm->setTransformDirty(true);
			}
			else
				return error_prop(L, "quat");
		}
		else
			lua_rawset(L, 1);
		break;
	case GameObjectProperty::X:
	case GameObjectProperty::Y:
		break;
	default:
		lua_rawset(L, 1);
		break;
	}
	return 0;
}

int GameObjectPool::BindNode(lua_State* L) noexcept
{
	const auto p = checkObjectFast(L, 1);
	if (!p)
		return error_obj(L);
	p->cm->setBindNode(lua::tousertype<Node>(L, 2, "cc.Node"));
	return 0;
}

int GameObjectPool::GetObjectTable(lua_State* L)noexcept
{
	GETOBJTABLE;
	return 1;
}

int GameObjectPool::GetParticlePool(lua_State* L) noexcept
{
	const auto p = checkObjectFast(L, 1);
	if (!p)
		return 0;
	const auto data = p->cm->getDataParticle();
	if (data && data->pool)
	{
		object_to_luaval(L, "lstg.ResParticle::ParticlePool", data->pool);
		return 1;
	}
	return 0;
}

void GameObjectPool::DrawGroupCollider(int groupId, const Color4B& fillColor, const Vec2& offset)
{
	GameObject* p = m_pCollisionListHeader[groupId].pCollisionNext;
	const auto pTail = &m_pCollisionListTail[groupId];
	while (p && p != pTail)
	{
		if (p->colli)
		{
			p->cm->drawCollider(fillColor);
		}
		p = p->pCollisionNext;
	}
}

bool GameObjectPool::checkClass(lua_State* L, int idx)
{
	if (!lua_istable(L, idx))
		return false;
	lua_getfield(L, idx, "is_class");  // ... b
	if (!lua_toboolean(L, -1))
		return false;
	lua_pop(L, 1);  // ...
	return true;
}

GameObject* GameObjectPool::allocateObject()
{
	// allocate
	size_t id = 0;
	if (!_objectPool.Alloc(id))
		return nullptr;

	// initial setting
	auto p = _objectPool.Data(id);
	assert_ptr(p);
	p->_Reset();
	p->status = STATUS_DEFAULT;
	p->id = id;
	p->uid = _iUid++;

	p->cm = &cmPool[p->id];
	p->cm->getOrCreateTrasform2D();
	p->cm->getOrCreateColli();
	p->cm->reset();

	// insert to list
	insertBeforeObjectList(p);// insert to tail
	insertBeforeRenderList(p);
	insertBeforeCollisionList(p, p->cm->getDataColli()->group);
	sortRenderList(p);
	sortCollisionList(p);// for compatibility
	return p;
}

void GameObjectPool::pushObject(GameObject* obj)
{
	if (L)
		pushObject(L, obj);
}

void GameObjectPool::pushObject(lua_State* L, GameObject* obj)
{
	if (obj&&obj->status != STATUS_FREE)
	{
		GETOBJTABLE;
		lua_rawgeti(L, -1, obj->id + 1);
		lua_remove(L, -2);
	}
	else
		lua_pushnil(L);
}

void GameObjectPool::pushGroup(lua_State* L, size_t group)
{
	if (group >= maxGroupCount)
	{
		//lua_createtable(L, 0, 0);
		lua_pushnil(L);
	}
	else
	{
		auto p = m_pCollisionListHeader[group].pCollisionNext;
		const auto tail = &m_pCollisionListTail[group];
		GETOBJTABLE;
		lua_createtable(L, GetObjectCount() / 2, 0); // ... ot t
		size_t idx = 1;
		while (p && p != tail)
		{
			lua_rawgeti(L, -1, p->id + 1); // ... ot t obj
			lua_rawseti(L, -2, idx); // ... ot t
			++idx;
			p = p->pCollisionNext;
		}
	}
}

int GameObjectPool::RegistClass(lua_State* L)
{
	const auto cls = GameClass::registerClass(L);
	if (!cls)
		return luaL_error(L, "failed when register class");
	return 0;
}

uint32_t GameObjectPool::getClassID(lua_State* L, int idx)
{
	lua_rawgeti(L, idx, 7);  // cls ... cid
	const uint32_t cid = lua_tointeger(L, -1);
	lua_pop(L, 1);  // cls ...
	return cid;
}

GameObject* GameObjectPool::checkObject(lua_State* L, int idx)
{
	if (!lua_istable(L, idx))
		return nullptr;
	lua_rawgeti(L, idx, 2);  // ... id
	if (!lua_isnumber(L, -1))
		return nullptr;
	// check in pool
	const auto id = size_t(lua_tonumber(L, -1));
	lua_pop(L, 1);  // ...
	const auto p = _objectPool.Data(id);
	if (!p)
		return nullptr;

	GETOBJTABLE;  // ... ot
	lua_rawgeti(L, -1, lua_Integer(id + 1));  // ... ot obj
	if (idx < 0)
		idx = idx - 2;
	const bool eq = lua_rawequal(L, -1, idx) != 0;
	lua_pop(L, 2);  // ...
	return eq ? p : nullptr;
}

GameObject* GameObjectPool::checkObjectFast(lua_State* L, int idx)
{
	if (!lua_istable(L, idx))
		return nullptr;
	lua_rawgeti(L, idx, 2);  // ... id
	if (!lua_isnumber(L, -1))
		return nullptr;
	// check in pool
	const auto id = size_t(lua_tonumber(L, -1));
	lua_pop(L, 1);  // ...
	return _objectPool.Data(id);
}

void GameObjectPool::setObjectLayer(GameObject* p, lua_Number layer)
{
	if (layer == p->layer)
		return;
	p->layer = layer;
	sortRenderList(p); // 刷新p的渲染层级
	CCASSERT(m_pRenderListHeader.pRenderNext, "");
	CCASSERT(m_pRenderListTail.pRenderPrev, "");
}

void GameObjectPool::setObjectGroup(GameObject* p, lua_Integer group)
{
	const auto old = p->cm->getDataColli()->group;
	if (group == old)
		return;
	// remove from old
	if (0 <= old && old < maxGroupCount)
	{
		LIST_REMOVE(p, Collision);
	}
	p->cm->getDataColli()->group = group;
	// insert to new
	if (0 <= group && group < maxGroupCount)
	{
		insertBeforeCollisionList(p, group);
		sortCollisionList(p);
		CCASSERT(m_pCollisionListHeader[group].pCollisionNext, "");
		CCASSERT(m_pCollisionListTail[group].pCollisionPrev, "");
	}
}

bool GameObjectPool::setObjectResource(GameObject* p, lua_State* L, int idx)
{
	const auto type = lua_type(L, idx);
	Resource* res = nullptr;
	if (type == LUA_TSTRING)
	{
		const auto name = lua_tostring(L, idx);
		//res = LRES.FindSprite(name);
		//if (!res)res = LRES.FindAnimation(name);
		//if (!res)res = LRES.FindParticle(name);
		//if (!res)res = LRES.FindFont(name);
		lua_getglobal(L, "FindResForObject");
		if (!lua_isfunction(L, -1))
		{
			lua_pop(L, 1);
			return false;
		}
		else
		{
			lua_pushstring(L, name);
			lua_pcall(L, 1, 1, 0);
			if(lua_isnil(L,-1))
			{
				lua_pop(L, 1);
				return false;
			}
			res = lua::tousertype<Resource>(L, -1, "lstg.Resource");
			lua_pop(L, 1);
			if (!res)
				return false;
		}
	}
	else if (type == LUA_TUSERDATA)
	{
		res = Resource::fromLua(L, idx, ResourceType::Sprite);
		if (!res)res = Resource::fromLua(L, idx, ResourceType::Animation);
		if (!res)res = Resource::fromLua(L, idx, ResourceType::Particle);
		if (!res)res = Resource::fromLua(L, idx, ResourceType::Font);
		if (!res)res = Resource::fromLua(L, idx, ResourceType::Texture);
		if (!res)
			return false;
	}
	else if (type == LUA_TNIL)
	{
		p->ReleaseResource();
		return true;
	}

	if (res)
	{
		p->ChangeResource(res);
		return true;
	}
	return false;
}

size_t GameObjectPool::updateIDForLua()
{
	size_t idx = 0;
	auto p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		idForLua[idx] = p->id + 1;
		idx++;
		p = p->pObjectNext;
	}
	return idx;
}

void GameObjectPool::pushIDForLua(lua_State* L)
{
	lua::cptr_to_luaval(L, idForLua.data());
}

void GameObjectPool::insertBeforeObjectList(GameObject* p)
{
	// insert to tail
	p->pObjectPrev = (&m_pObjectListTail)->pObjectPrev;
	p->pObjectNext = (&m_pObjectListTail);
	p->pObjectPrev->pObjectNext = p;
	p->pObjectNext->pObjectPrev = p;
}

void GameObjectPool::insertBeforeRenderList(GameObject* p)
{
	p->pRenderPrev = (&m_pRenderListTail)->pRenderPrev;
	p->pRenderNext = (&m_pRenderListTail);
	p->pRenderPrev->pRenderNext = p;
	p->pRenderNext->pRenderPrev = p;
}

void GameObjectPool::insertBeforeCollisionList(GameObject* p, uint32_t group)
{
	p->pCollisionPrev = (&m_pCollisionListTail[group])->pCollisionPrev;
	p->pCollisionNext = (&m_pCollisionListTail[group]);
	p->pCollisionPrev->pCollisionNext = p;
	p->pCollisionNext->pCollisionPrev = p;
}

void GameObjectPool::sortRenderList(GameObject* p)
{
	if (p->pRenderNext->pRenderNext && RenderListSortFunc(p->pRenderNext, p))
	{
		GameObject* pInsertBefore = p->pRenderNext->pRenderNext;
#ifdef OPT_RENDER_LIST
		if (renderMid&&RenderListSortFunc(renderMid, p) && RenderListSortFunc(pInsertBefore, renderMid))
			pInsertBefore = renderMid->pRenderNext;
#endif
		while (pInsertBefore->pRenderNext && RenderListSortFunc(pInsertBefore, p))
			pInsertBefore = pInsertBefore->pRenderNext;
		//LIST_REMOVE(p, Render);
		p->pRenderPrev->pRenderNext = p->pRenderNext;
		p->pRenderNext->pRenderPrev = p->pRenderPrev;
		//LIST_INSERT_BEFORE(pInsertBefore, p, Render);
		p->pRenderPrev = (pInsertBefore)->pRenderPrev;
		p->pRenderNext = (pInsertBefore);
		p->pRenderPrev->pRenderNext = p;
		p->pRenderNext->pRenderPrev = p;
	}
	else if (p->pRenderPrev->pRenderPrev && RenderListSortFunc(p, p->pRenderPrev))
	{
		GameObject* pInsertAfter = p->pRenderPrev->pRenderPrev;
#ifdef OPT_RENDER_LIST
		if (renderMid&&RenderListSortFunc(p, renderMid)&&RenderListSortFunc(renderMid, pInsertAfter))
			pInsertAfter = renderMid->pRenderPrev;
#endif
		while (pInsertAfter->pRenderPrev && RenderListSortFunc(p, pInsertAfter))
			pInsertAfter = pInsertAfter->pRenderPrev;
		//LIST_REMOVE(p, Render);
		p->pRenderPrev->pRenderNext = p->pRenderNext;
		p->pRenderNext->pRenderPrev = p->pRenderPrev;
		//LIST_INSERT_AFTER(pInsertAfter, p, Render);
		p->pRenderPrev = (pInsertAfter);
		p->pRenderNext = (pInsertAfter)->pRenderNext;
		p->pRenderPrev->pRenderNext = p;
		p->pRenderNext->pRenderPrev = p;
	}
}

void GameObjectPool::sortCollisionList(GameObject* p)
{
	// for compatibility
	if (p->pCollisionNext->pCollisionNext && ObjectListSortFunc(p->pCollisionNext, p))
	{
		GameObject* pInsertBefore = p->pCollisionNext->pCollisionNext;
		while (pInsertBefore->pCollisionNext && ObjectListSortFunc(pInsertBefore, p))
			pInsertBefore = pInsertBefore->pCollisionNext;
		//LIST_REMOVE(p, Collision);
		p->pCollisionPrev->pCollisionNext = p->pCollisionNext;
		p->pCollisionNext->pCollisionPrev = p->pCollisionPrev;
		//LIST_INSERT_BEFORE(pInsertBefore, p, Collision);
		p->pCollisionPrev = (pInsertBefore)->pCollisionPrev;
		p->pCollisionNext = (pInsertBefore);
		p->pCollisionPrev->pCollisionNext = p;
		p->pCollisionNext->pCollisionPrev = p;
	}
	else if (p->pCollisionPrev->pCollisionPrev && ObjectListSortFunc(p, p->pCollisionPrev))
	{
		GameObject* pInsertAfter = p->pCollisionPrev->pCollisionPrev;
		while (pInsertAfter->pCollisionPrev && ObjectListSortFunc(p, pInsertAfter))
			pInsertAfter = pInsertAfter->pCollisionPrev;
		//LIST_REMOVE(p, Collision);
		p->pCollisionPrev->pCollisionNext = p->pCollisionNext;
		p->pCollisionNext->pCollisionPrev = p->pCollisionPrev;
		//LIST_INSERT_AFTER(pInsertAfter, p, Collision);
		p->pCollisionPrev = (pInsertAfter);
		p->pCollisionNext = (pInsertAfter)->pCollisionNext;
		p->pCollisionPrev->pCollisionNext = p;
		p->pCollisionNext->pCollisionPrev = p;
	}
}
