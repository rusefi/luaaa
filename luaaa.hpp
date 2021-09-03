
/*
 Copyright (c) 2019 gengyong
 https://github.com/gengyong/luaaa
 licensed under MIT License.
*/

#ifndef HEADER_LUAAA_HPP
#define HEADER_LUAAA_HPP

#define LUAAA_NS luaaa

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM <= 501
inline void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
	luaL_checkstack(L, nup + 1, "too many upvalues");
	for (; l->name != NULL; l++) {
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)
			lua_pushvalue(L, -(nup + 1));
		lua_pushcclosure(L, l->func, nup);
		lua_settable(L, -(nup + 3));
	}
	lua_pop(L, nup);
}
#endif

#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM > 501 && !defined(LUA_COMPAT_MODULE)
#	define USE_NEW_MODULE_REGISTRY 1
#else
#	define USE_NEW_MODULE_REGISTRY 0
#endif


namespace LUAAA_NS
{
	template <typename>	struct LuaClass;

	//========================================================
	// Lua stack operator
	//========================================================

	template<typename T> struct LuaStack
	{
		inline static T& get(lua_State * state, int idx)
		{
			luaL_argcheck(state, LuaClass<T>::klassName != nullptr, 1, "class not exported");
			T ** t = (T**)luaL_checkudata(state, idx, LuaClass<T>::klassName);
			luaL_argcheck(state, t != NULL, 1, "invalid user data");
			luaL_argcheck(state, *t != NULL, 1, "invalid user data");
			return (**t);
		}

		inline static void put(lua_State * L, T * t)
		{
			lua_pushlightuserdata(L, t);
		}
	};


	template <typename T> struct LuaStack<const T> : public LuaStack<T> {};
	template <typename T> struct LuaStack<T&> : public LuaStack<T> {};
	template <typename T> struct LuaStack<const T&> : public LuaStack<T> {};

	template <typename T> struct LuaStack<volatile T&> : public LuaStack<T> 
	{
		inline static void put(lua_State * L, volatile T & t)
		{
			LuaStack<T>::put(L, const_cast<const T &>(t));
		}
	};

	template <typename T> struct LuaStack<const volatile T&> : public LuaStack<T>
	{
		inline static void put(lua_State * L, const volatile T & t)
		{
			LuaStack<T>::put(L, const_cast<const T &>(t));
		}
	};

	template <typename T> struct LuaStack<T&&> : public LuaStack<T>
	{
		inline static void put(lua_State * L, T && t)
		{
			LuaStack<T>::put(L, std::forward<T>(t));
		}
	};

	template <typename T> struct LuaStack<T*>
	{
		inline static T * get(lua_State * state, int idx)
		{
			if (!LuaClass<T*>::klassName.empty())
			{
				T ** t = (T**)luaL_checkudata(state, -1, LuaClass<T*>::klassName);
				luaL_argcheck(state, t != NULL, 1, "invalid user data");
				return *t;
			}
			else if (!LuaClass<T>::klassName.empty())
			{
				T ** t = (T**)luaL_checkudata(state, -1, LuaClass<T>::klassName);
				luaL_argcheck(state, t != NULL, 1, "invalid user data");
				luaL_argcheck(state, *t != NULL, 1, "invalid user data");
				return *t;
			}
			else if (lua_islightuserdata(state, idx))
			{
				T * t = (T*)lua_touserdata(state, idx);
				return t;
			}
			return nullptr;
		}

		inline static void put(lua_State * L, T * t)
		{
			lua_pushlightuserdata(L, t);
		}
	};

	template<>
	struct LuaStack<float>
	{
		inline static float get(lua_State * L, int idx)
		{
			if (lua_isnumber(L, idx) || lua_isstring(L, idx))
			{
				return float(lua_tonumber(L, idx));
			}
			else
			{
				luaL_checktype(L, idx, LUA_TNUMBER);
			}
			return 0;
		}

		inline static void put(lua_State * L, const float & t)
		{
			lua_pushnumber(L, t);
		}
	};

	template<>
	struct LuaStack<double>
	{
		inline static double get(lua_State * L, int idx)
		{
			if (lua_isnumber(L, idx) || lua_isstring(L, idx))
			{
				return double(lua_tonumber(L, idx));
			}
			else
			{
				luaL_checktype(L, idx, LUA_TNUMBER);
			}
			return 0;
		}

		inline static void put(lua_State * L, const double & t)
		{
			lua_pushnumber(L, t);
		}
	};

	template<>
	struct LuaStack<int>
	{
		inline static int get(lua_State * L, int idx)
		{
			if (lua_isnumber(L, idx) || lua_isstring(L, idx))
			{
				return int(lua_tointeger(L, idx));
			}
			else
			{
				luaL_checktype(L, idx, LUA_TNUMBER);
			}
			return 0;
		}

		inline static void put(lua_State * L, const int & t)
		{
			lua_pushinteger(L, t);
		}
	};

	template<>
	struct LuaStack<bool>
	{
		inline static bool get(lua_State * L, int idx)
		{
			luaL_checktype(L, idx, LUA_TBOOLEAN);
			return lua_toboolean(L, idx) != 0;
		}

		inline static void put(lua_State * L, const bool & t)
		{
			lua_pushboolean(L, t);
		}
	};

	template<>
	struct LuaStack<const char *>
	{
		inline static const char * get(lua_State * L, int idx)
		{
			thread_local static char sss[256];

			switch (lua_type(L, idx))
			{
			case LUA_TBOOLEAN:
				return (lua_toboolean(L, idx) ? "true" : "false");
			case LUA_TNUMBER:
				snprintf((sss), sizeof(sss), LUA_NUMBER_FMT, (lua_tonumber(L, idx)));
				return sss;
			case LUA_TSTRING:
				return lua_tostring(L, idx);
			default:
				luaL_checktype(L, idx, LUA_TSTRING);
				break;
			}
			return "";
		}

		inline static void put(lua_State * L, const char * s)
		{
			lua_pushstring(L, s);
		}
	};

	template<>
	struct LuaStack<char *>
	{
		inline static char * get(lua_State * L, int idx)
		{
			return const_cast<char*>(LuaStack<const char *>::get(L, idx));
		}

		inline static void put(lua_State * L, const char * s)
		{
			LuaStack<const char *>::put(L, s);
		}
	};

	// push ret data to stack
	template <typename T>
	inline void LuaStackReturn(lua_State * L, T t)
	{
		lua_settop(L, 0);
		LuaStack<T>::put(L, t);
	}

#define IMPLEMENT_CALLBACK_INVOKER(CALLCONV) \
	template<typename RET, typename ...ARGS> \
	struct LuaStack<RET(CALLCONV*)(ARGS...)> \
	{ \
		typedef RET(CALLCONV*FTYPE)(ARGS...); \
		inline static FTYPE get(lua_State * L, int idx) \
		{ \
			static lua_State * cacheLuaState = nullptr; \
			static int cacheLuaFuncId = 0; \
			struct HelperClass \
			{ \
				static RET CALLCONV f_callback(ARGS... args) \
				{ \
					lua_rawgeti(cacheLuaState, LUA_REGISTRYINDEX, cacheLuaFuncId); \
					if (lua_isfunction(cacheLuaState, -1)) \
					{ \
						int initParams[] = { (LuaStack<ARGS>::put(cacheLuaState, args), 0)..., 0 }; \
						if (lua_pcall(cacheLuaState, sizeof...(ARGS), 1, 0) != 0) \
						{ \
							lua_error(cacheLuaState); \
						} \
						luaL_unref(cacheLuaState, LUA_REGISTRYINDEX, cacheLuaFuncId); \
					} \
					else \
					{ \
						lua_pushnil(cacheLuaState); \
					} \
					return LuaStack<RET>::get(cacheLuaState, lua_gettop(cacheLuaState)); \
				} \
			}; \
			if (lua_isfunction(L, idx)) \
			{ \
				cacheLuaState = L; \
				lua_pushvalue(L, idx); \
				cacheLuaFuncId = luaL_ref(L, LUA_REGISTRYINDEX); \
				return HelperClass::f_callback; \
			} \
			return nullptr; \
		} \
		inline void put(lua_State * L, FTYPE f) \
		{ \
			lua_pushcfunction(L, NonMemberFunctionCaller(f)); \
		} \
	}; \
	template<typename ...ARGS> \
	struct LuaStack<void(CALLCONV*)(ARGS...)> \
	{ \
		typedef void(CALLCONV*FTYPE)(ARGS...); \
		inline static FTYPE get(lua_State * L, int idx) \
		{ \
			static lua_State * cacheLuaState = nullptr; \
			static int cacheLuaFuncId = 0; \
			struct HelperClass \
			{ \
				static void CALLCONV f_callback(ARGS... args) \
				{ \
					lua_rawgeti(cacheLuaState, LUA_REGISTRYINDEX, cacheLuaFuncId); \
					if (lua_isfunction(cacheLuaState, -1)) \
					{ \
						int initParams[] = { (LuaStack<ARGS>::put(cacheLuaState, args), 0)..., 0 }; \
						if (lua_pcall(cacheLuaState, sizeof...(ARGS), 1, 0) != 0) \
						{ \
							lua_error(cacheLuaState); \
						} \
						luaL_unref(cacheLuaState, LUA_REGISTRYINDEX, cacheLuaFuncId); \
					} \
					return; \
				} \
			}; \
			if (lua_isfunction(L, idx)) \
			{ \
				cacheLuaState = L; \
				lua_pushvalue(L, idx); \
				cacheLuaFuncId = luaL_ref(L, LUA_REGISTRYINDEX); \
				return HelperClass::f_callback; \
			} \
			return nullptr; \
		} \
		inline void put(lua_State * L, FTYPE f) \
		{ \
			lua_pushcfunction(L, NonMemberFunctionCaller(f)); \
		} \
	};


	//========================================================
	// non-member function caller & static member function caller
	//========================================================

#define IMPLEMENT_FUNCTION_CALLER(CALLERNAME, CALLCONV, SKIPPARAM) \
	template<typename TRET, typename ...ARGS> \
	lua_CFunction CALLERNAME(TRET(CALLCONV*func)(ARGS...)) \
	{ \
		typedef decltype(func) FTYPE; (void)(func); \
		struct HelperClass \
		{ \
			static int Invoke(lua_State* state) \
			{ \
				void * calleePtr = lua_touserdata(state, lua_upvalueindex(1)); \
				luaL_argcheck(state, calleePtr, 1, "cpp closure function not found."); \
				if (calleePtr) \
				{ \
					volatile int idx = sizeof...(ARGS) + SKIPPARAM; (void)(idx); \
					LuaStackReturn<TRET>(state, (*(FTYPE*)(calleePtr))((LuaStack<ARGS>::get(state, idx--))...)); \
					return 1; \
				} \
				return 0; \
			} \
		}; \
		return HelperClass::Invoke; \
	} \
	template<typename ...ARGS> \
	lua_CFunction CALLERNAME(void(CALLCONV*func)(ARGS...)) \
	{ \
		typedef decltype(func) FTYPE; (void)(func); \
		struct HelperClass \
		{ \
			static int Invoke(lua_State* state) \
			{ \
				void * calleePtr = lua_touserdata(state, lua_upvalueindex(1)); \
				luaL_argcheck(state, calleePtr, 1, "cpp closure function not found."); \
				if (calleePtr) \
				{ \
					volatile int idx = sizeof...(ARGS) + SKIPPARAM; (void)(idx); \
					(*(FTYPE*)(calleePtr))((LuaStack<ARGS>::get(state, idx--))...); \
				} \
				return 0; \
			} \
		}; \
		return HelperClass::Invoke; \
	}
	
#if defined(_MSC_VER)	
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, __cdecl, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, __cdecl, 1);
	IMPLEMENT_CALLBACK_INVOKER(__cdecl);

#	ifdef _M_CEE
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, __clrcall, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, __clrcall, 1);
	IMPLEMENT_CALLBACK_INVOKER(__clrcall);
#	endif

#	if defined(_M_IX86) && !defined(_M_CEE)
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, __fastcall, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, __fastcall, 1);
	IMPLEMENT_CALLBACK_INVOKER(__fastcall);
#	endif

#	ifdef _M_IX86
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, __stdcall, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, __stdcall, 1);
	IMPLEMENT_CALLBACK_INVOKER(__stdcall);
#	endif

#	if ((defined(_M_IX86) && _M_IX86_FP >= 2) || defined(_M_X64)) && !defined(_M_CEE)
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, __vectorcall, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, __vectorcall, 1);
	IMPLEMENT_CALLBACK_INVOKER(__vectorcall);
#	endif

#elif defined(__GNUC__)
#	define _NOTHING
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, _NOTHING, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, _NOTHING, 1);
	IMPLEMENT_CALLBACK_INVOKER(_NOTHING);
#	undef _NOTHING	
#else
#	define _NOTHING	
	IMPLEMENT_FUNCTION_CALLER(NonMemberFunctionCaller, _NOTHING, 0);
	IMPLEMENT_FUNCTION_CALLER(MemberFunctionCaller, _NOTHING, 1);
	IMPLEMENT_CALLBACK_INVOKER(_NOTHING);
#	undef _NOTHING		
#endif	

	//========================================================
	// member function invoker
	//========================================================
	template<typename TCLASS, typename TRET, typename ...ARGS>
	lua_CFunction MemberFunctionCaller(TRET(TCLASS::*func)(ARGS...))
	{
		typedef decltype(func) FTYPE; (void)(func);
		struct HelperClass
		{
			static int Invoke(lua_State* state)
			{
				void * calleePtr = lua_touserdata(state, lua_upvalueindex(1));
				luaL_argcheck(state, calleePtr, 1, "cpp closure function not found.");
				if (calleePtr)
				{
					volatile int idx = sizeof...(ARGS) + 1; (void)(idx);
					LuaStackReturn<TRET>(state, (LuaStack<TCLASS>::get(state, 1).**(FTYPE*)(calleePtr))(LuaStack<ARGS>::get(state, idx--)...));
					return 1;
				}
				return 0;
			}
		};
		return HelperClass::Invoke;
	}

	template<typename TCLASS, typename TRET, typename ...ARGS>
	lua_CFunction MemberFunctionCaller(TRET(TCLASS::*func)(ARGS...)const)
	{
		typedef decltype(func) FTYPE; (void)(func);
		struct HelperClass
		{
			static int Invoke(lua_State* state)
			{
				void * calleePtr = lua_touserdata(state, lua_upvalueindex(1));
				luaL_argcheck(state, calleePtr, 1, "cpp closure function not found.");
				if (calleePtr)
				{
					volatile int idx = sizeof...(ARGS) + 1; (void)(idx);
					LuaStackReturn<TRET>(state, (LuaStack<TCLASS>::get(state, 1).**(FTYPE*)(calleePtr))(LuaStack<ARGS>::get(state, idx--)...));
					return 1;
				}
				return 0;
			}
		};
		return HelperClass::Invoke;
	}

	template<typename TCLASS, typename ...ARGS>
	lua_CFunction MemberFunctionCaller(void(TCLASS::*func)(ARGS...))
	{
		typedef decltype(func) FTYPE; (void)(func);
		struct HelperClass
		{
			static int Invoke(lua_State* state)
			{
				void * calleePtr = lua_touserdata(state, lua_upvalueindex(1));
				luaL_argcheck(state, calleePtr, 1, "cpp closure function not found.");
				if (calleePtr)
				{
					volatile int idx = sizeof...(ARGS) + 1; (void)(idx);
					(LuaStack<TCLASS>::get(state, 1).**(FTYPE*)(calleePtr))(LuaStack<ARGS>::get(state, idx--)...);
				}
				return 0;
			}
		};
		return HelperClass::Invoke;
	}

	template<typename TCLASS, typename ...ARGS>
	lua_CFunction MemberFunctionCaller(void(TCLASS::*func)(ARGS...)const)
	{
		typedef decltype(func) FTYPE; (void)(func);
		struct HelperClass
		{
			static int Invoke(lua_State* state)
			{
				void * calleePtr = lua_touserdata(state, lua_upvalueindex(1));
				luaL_argcheck(state, calleePtr, 1, "cpp closure function not found.");
				if (calleePtr)
				{
					volatile int idx = sizeof...(ARGS) + 1; (void)(idx);
					(LuaStack<TCLASS>::get(state, 1).**(FTYPE*)(calleePtr))(LuaStack<ARGS>::get(state, idx--)...);
				}
				return 0;
			}
		};
		return HelperClass::Invoke;
	}
	
	//========================================================
	// constructor invoker
	//========================================================
	template<typename TCLASS, typename ...ARGS>
	struct ConstructorCaller
	{
		static TCLASS * Invoke(lua_State * state)
		{
			(void)(state);
			volatile int idx = sizeof...(ARGS); (void)(idx);

			auto ptr = lua_newuserdata(state, sizeof(TCLASS));

			// Placement-new the object in to the Lua-allocated space
			return new (ptr) TCLASS(LuaStack<ARGS>::get(state, idx--)...);
		}
	};
}

namespace LUAAA_NS
{
	//========================================================
	// export class
	//========================================================
	template <typename TCLASS>
	struct LuaClass
	{
	public:
		LuaClass(lua_State * state, const char* name, const luaL_Reg * functions = nullptr)
			: m_state(state)
		{
			luaL_argcheck(state, (klassName == nullptr || klassName == name), 1, "bind conflict");

			klassName = name;

			if (state)
			{
				struct HelperClass {
					static int f__gc(lua_State* state) {
						TCLASS ** objPtr = (TCLASS**)luaL_checkudata(state, -1, LuaClass<TCLASS>::klassName);
						if (objPtr)
						{
							(*objPtr)->~TCLASS();
						}
						return 0;
					}
				};

				luaL_Reg destructor[] = {
					{ "__gc", HelperClass::f__gc },
					{ nullptr, nullptr }
				};

				luaL_newmetatable(state, klassName);
				lua_pushvalue(state, -1);
				lua_setfield(state, -2, "__index");
				luaL_setfuncs(state, destructor, 0);
				if (functions)
				{
					luaL_setfuncs(state, functions, 0);
				}
				lua_pop(state, 1);

				ctor<>();
			}
		}


		template<typename ...ARGS>
		inline LuaClass<TCLASS>& ctor(const char * name = "new")
		{
			struct HelperClass {
				static int f_new(lua_State* state) {
					auto obj = ConstructorCaller<TCLASS, ARGS...>::Invoke(state);
					if (obj)
					{
						TCLASS ** objPtr = (TCLASS**)lua_newuserdata(state, sizeof(TCLASS*));
						if (objPtr)
						{
							*objPtr = obj;
							luaL_getmetatable(state, LuaClass<TCLASS>::klassName);
							lua_setmetatable(state, -2);
							return 1;
						}
						else
						{
							obj->~TCLASS();
						}
					}
					lua_pushnil(state);
					return 1;
				}

			};

			luaL_Reg constructor[] = { { name, HelperClass::f_new },{ nullptr, nullptr } };

#if USE_NEW_MODULE_REGISTRY
			lua_newtable(m_state);
			luaL_setfuncs(m_state, constructor, 0);
			lua_setglobal(m_state, klassName);
#else
			luaL_register(m_state, klassName, constructor);	//luaL_openlib(state, klassName, constructor, 0);
			lua_pop(m_state, 1);
#endif
			return (*this);
		}

		template<typename F>
		inline LuaClass<TCLASS>& fun(const char * name, F f)
		{
			luaL_getmetatable(m_state, klassName);
			lua_pushstring(m_state, name);

			F * funPtr = (F*)lua_newuserdata(m_state, sizeof(F));
			luaL_argcheck(m_state, funPtr != nullptr, 1, "failed to alloc mem to store function");
			*funPtr = f;
			lua_pushcclosure(m_state, MemberFunctionCaller(f), 1);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		inline LuaClass<TCLASS>& fun(const char * name, lua_CFunction f)
		{
			luaL_getmetatable(m_state, klassName);
			lua_pushstring(m_state, name);
			lua_pushcclosure(m_state, f, 0);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		template <typename V>
		inline LuaClass<TCLASS>& def(const char * name, const V& val)
		{
			luaL_getmetatable(m_state, klassName);
			lua_pushstring(m_state, name);
			LuaStack<V>::put(m_state, val);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		// disable cast from "const char [#]" to "char (*)[#]"
		inline LuaClass<TCLASS>& def(const char * name, const char * str)
		{
			luaL_getmetatable(m_state, klassName);
			lua_pushstring(m_state, name);
			LuaStack<decltype(str)>::put(m_state, str);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

	private:
		lua_State *	m_state;

	private:
		template<typename> friend struct LuaStack;
		static const char* klassName;
	};

	template <typename TCLASS> const char* LuaClass<TCLASS>::klassName = nullptr;


	// -----------------------------------
	// export module
	// -----------------------------------
	struct LuaModule
	{
	public:
		LuaModule(lua_State * state, const char* name = "_G")
			: m_state(state)
			, m_moduleName(name)
		{
		}

	public:
		template<typename F>
		inline LuaModule& fun(const char * name, F f)
		{
			luaL_Reg regtab[] = { { name, NonMemberFunctionCaller(f) },{ nullptr, nullptr } };

#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName);
			if (lua_isnil(m_state, -1)) 
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}

			F * funPtr = (F*)lua_newuserdata(m_state, sizeof(F));
			luaL_argcheck(m_state, funPtr != nullptr, 1, "failed to alloc mem to store function");
			*funPtr = f;

			luaL_setfuncs(m_state, regtab, 1);
			lua_setglobal(m_state, m_moduleName);
#else
			F * funPtr = (F*)lua_newuserdata(m_state, sizeof(F));
			luaL_argcheck(m_state, funPtr != nullptr, 1, "failed to alloc mem to store function");
			*funPtr = f;

			luaL_openlib(m_state, m_moduleName, regtab, 1);
#endif

			return (*this);
		}

		inline LuaModule& fun(const char * name, lua_CFunction f)
		{
			luaL_Reg regtab[] = { { name, f },{ nullptr, nullptr } };
#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName);
			if (lua_isnil(m_state, -1))
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}
			luaL_setfuncs(m_state, regtab, 0);
			lua_setglobal(m_state, m_moduleName);
#else
			luaL_openlib(m_state, m_moduleName, regtab, 0);
#endif
			return (*this);
		}

		template <typename V>
		inline LuaModule& def(const char * name, const V& val)
		{
#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName);
			if (lua_isnil(m_state, -1))
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}
			LuaStack<V>::put(m_state, val);
			lua_setfield(m_state, -2, name);
			lua_setglobal(m_state, m_moduleName);
#else
			luaL_Reg regtab = { nullptr, nullptr };
			luaL_openlib(m_state, m_moduleName, &regtab, 0);
			LuaStack<V>::put(m_state, val);
			lua_setfield(m_state, -2, name);
#endif
			return (*this);
		}

		// disable the cast from "const char [#]" to "char (*)[#]"
		inline LuaModule& def(const char * name, const char * str)
		{
			
#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName);
			if (lua_isnil(m_state, -1))
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}
			LuaStack<decltype(str)>::put(m_state, str);
			lua_setfield(m_state, -2, name);
			lua_setglobal(m_state, m_moduleName);
#else
			luaL_Reg regtab = { nullptr, nullptr };
			luaL_openlib(m_state, m_moduleName, &regtab, 0);
			LuaStack<decltype(str)>::put(m_state, str);
			lua_setfield(m_state, -2, name);
#endif
			return (*this);
		}

	private:
		lua_State *	m_state;
		const char* m_moduleName;
	};

}

#endif