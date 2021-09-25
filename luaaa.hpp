
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


#include <string>
#include <typeinfo>

namespace LUAAA_NS
{
    //========================================================
    // Lua Class
    //========================================================

	template <typename>	struct LuaClass;

	//========================================================
	// Lua stack operator
	//========================================================

	template<typename T> struct LuaStack
	{
		inline static T& get(lua_State * state, int idx)
		{
			luaL_argcheck(state, !LuaClass<T>::klassName.empty(), 1, (std::string("cpp class `") /*+ typeid(T).name()*/ + "` not export").c_str());
			T ** t = (T**)luaL_checkudata(state, idx, LuaClass<T>::klassName.c_str());
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
				T ** t = (T**)luaL_checkudata(state, idx, LuaClass<T*>::klassName.c_str());
				luaL_argcheck(state, t != NULL, 1, "invalid user data");
				return *t;
			}
			else if (!LuaClass<T>::klassName.empty())
			{
				T ** t = (T**)luaL_checkudata(state, idx, LuaClass<T>::klassName.c_str());
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
			thread_local static char sss[64];

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


	template<>
	struct LuaStack<std::string>
	{
		inline static std::string get(lua_State * L, int idx)
		{
			return LuaStack<const char *>::get(L, idx);
		}

		inline static void put(lua_State * L, const std::string& s)
		{
			LuaStack<const char *>::put(L, s.c_str());
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
			return new TCLASS((LuaStack<ARGS>::get(state, idx--))...);
		}
	};
}

#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace LUAAA_NS
{
	// array
	template<typename K, size_t N>
	struct LuaStack<std::array<K, N>>
	{
		typedef std::array<K, N> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;

			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				int index = 0;
				lua_pushnil(L);
				while (0 != lua_next(L, idx) && index < N)
				{
					result[index++] = LuaStack<typename Container::value_type>::get(L, idx + 2);
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// vector
	template<typename K, typename ...ARGS>
	struct LuaStack<std::vector<K, ARGS...>>
	{
		typedef std::vector<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.push_back(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// deque
	template<typename K, typename ...ARGS>
	struct LuaStack<std::deque<K, ARGS...>>
	{
		typedef std::deque<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.push_back(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// list
	template<typename K, typename ...ARGS>
	struct LuaStack<std::list<K, ARGS...>>
	{
		typedef std::list<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.push_back(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// forward_list
	template<typename K, typename ...ARGS>
	struct LuaStack<std::forward_list<K, ARGS...>>
	{
		typedef std::forward_list<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.push_back(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// set
	template<typename K, typename ...ARGS>
	struct LuaStack<std::set<K, ARGS...>>
	{
		typedef std::set<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.insert(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// multiset
	template<typename K, typename ...ARGS>
	struct LuaStack<std::multiset<K, ARGS...>>
	{
		typedef std::multiset<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.insert(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// unordered_set
	template<typename K, typename ...ARGS>
	struct LuaStack<std::unordered_set<K, ARGS...>>
	{
		typedef std::unordered_set<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.insert(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}

		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// unordered_multiset
	template<typename K, typename ...ARGS>
	struct LuaStack<std::unordered_multiset<K, ARGS...>>
	{
		typedef std::unordered_multiset<K, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result.insert(LuaStack<typename Container::value_type>::get(L, idx + 2));
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}

		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			int index = 1;
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::value_type>::put(L, *it);
				lua_rawseti(L, -2, index++);
			}
		}
	};

	// map
	template<typename K, typename V, typename ...ARGS>
	struct LuaStack<std::map<K, V, ARGS...>>
	{
		typedef std::map<K, V, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result[LuaStack<typename Container::key_type>::get(L, idx + 1)] = LuaStack<typename Container::mapped_type>::get(L, idx + 2);
					lua_pop(L, 1);	
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::key_type>::put(L, it->first);
				LuaStack<typename Container::mapped_type>::put(L, it->second);
				lua_rawset(L, -3);
			}
		}
	};

	// multimap
	template<typename K, typename V, typename ...ARGS>
	struct LuaStack<std::multimap<K, V, ARGS...>>
	{
		typedef std::multimap<K, V, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result[LuaStack<typename Container::key_type>::get(L, idx + 1)] = LuaStack<typename Container::mapped_type>::get(L, idx + 2);
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::key_type>::put(L, it->first);
				LuaStack<typename Container::mapped_type>::put(L, it->second);
				lua_rawset(L, -3);
			}
		}
	};

	// unordered_map
	template<typename K, typename V, typename ...ARGS>
	struct LuaStack<std::unordered_map<K, V, ARGS...>>
	{
		typedef std::unordered_map<K, V, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result[LuaStack<typename Container::key_type>::get(L, idx + 1)] = LuaStack<typename Container::mapped_type>::get(L, idx + 2);
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::key_type>::put(L, it->first);
				LuaStack<typename Container::mapped_type>::put(L, it->second);
				lua_rawset(L, -3);
			}
		}
	};

	// unordered_multimap
	template<typename K, typename V, typename ...ARGS>
	struct LuaStack<std::unordered_multimap<K, V, ARGS...>>
	{
		typedef std::unordered_multimap<K, V, ARGS...> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				lua_pushnil(L);
				while (0 != lua_next(L, idx))
				{
					result[LuaStack<typename Container::key_type>::get(L, idx + 1)] = LuaStack<typename Container::mapped_type>::get(L, idx + 2);
					lua_pop(L, 1);
				}
				lua_pop(L, 0);
			}
			return result;
		}
		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				LuaStack<typename Container::key_type>::put(L, it->first);
				LuaStack<typename Container::mapped_type>::put(L, it->second);
				lua_rawset(L, -3);
			}
		}
	};


	// std::pair
	template<typename U, typename V>
	struct LuaStack<std::pair<U, V>>
	{
		typedef std::pair<U, V> Container;
		inline static Container get(lua_State * L, int idx)
		{
			Container result;
			luaL_argcheck(L, lua_istable(L, idx), 1, "required table not found on stack.");
			if (lua_istable(L, idx))
			{
				result.first = LuaStack<typename Container::first_type>::get(L, idx + 1);
				result.second = LuaStack<typename Container::second_type>::get(L, idx + 2);
			}
			return result;
		}

		inline static void put(lua_State * L, const Container& s)
		{
			lua_newtable(L);
			LuaStack<typename Container::first_type>::put(L, s.first);
			lua_rawseti(L, -2, 1);
			LuaStack<typename Container::second_type>::put(L, s.second);
			lua_rawseti(L, -2, 2);			
		}
	};

	//========================================================
	// utilities
	//========================================================
    template<typename TCLASS, bool = std::is_destructible<TCLASS>::value>
    struct DestructorHelperClass {
        static void f_delete(TCLASS * obj) {
            delete obj;
        }
    };

    template<typename TCLASS>
    struct DestructorHelperClass<TCLASS, false> {
        static void f_delete(TCLASS * obj) {
            return;
        }
    };

    //========================================================
    // export class
    //========================================================
	template <typename TCLASS>
	struct LuaClass
	{
        template<typename TCLASS, bool> friend struct DestructorHelperClass;
	public:
		LuaClass(lua_State * state, const std::string& name, const luaL_Reg * functions = nullptr)
			: m_state(state)
		{
			luaL_argcheck(state, (klassName.empty() || klassName == name), 1, (std::string("C++ class `") /*+ typeid(TCLASS).name()*/ + "` bind to conflict lua name `" + name + "`, origin name: " + klassName).c_str());

			klassName = name;

			if (state)
			{
				luaL_newmetatable(state, klassName.c_str());
				lua_pushvalue(state, -1);
				lua_setfield(state, -2, "__index");
				if (functions)
				{
					luaL_setfuncs(state, functions, 0);
				}
				lua_pop(state, 1);

				//ctor<>();
			}
		}


		template<typename ...ARGS>
		inline LuaClass<TCLASS>& ctor(const char * name = "new")
		{
			struct HelperClass {
                static int f__gc(lua_State* state) {
                    TCLASS ** objPtr = (TCLASS**)luaL_checkudata(state, -1, LuaClass<TCLASS>::klassName.c_str());
                    if (objPtr)
                    {
                        //std::cout << "ctor(const char * name = 'new'):f__gc" << std::endl;
                        delete *objPtr;
                    }
                    return 0;
                }

				static int f_new(lua_State* state) {
					auto obj = ConstructorCaller<TCLASS, ARGS...>::Invoke(state);
					if (obj)
					{   
						TCLASS ** objPtr = (TCLASS**)lua_newuserdata(state, sizeof(TCLASS*));
						if (objPtr)
						{
							*objPtr = obj;

                            luaL_Reg destructor[] = {
                                { "__gc", HelperClass::f__gc },
                                { nullptr, nullptr }
                            };

							luaL_getmetatable(state, LuaClass<TCLASS>::klassName.c_str());
                            luaL_setfuncs(state, destructor, 0);
                            lua_setmetatable(state, -2);

							return 1;
						}
						else
						{
                            DestructorHelperClass<TCLASS>::f_delete(obj);
						}
					}
					lua_pushnil(state);			
					return 1;
				}
			};

			luaL_Reg constructor[] = { { name, HelperClass::f_new },{ nullptr, nullptr } };
#if USE_NEW_MODULE_REGISTRY
            //lua_newtable(state);
            //luaL_setfuncs(state, regtable, nup);
            //lua_setglobal(state, klassName);

            lua_getglobal(m_state, klassName.c_str());
            if (lua_isnil(m_state, -1))
            {
                lua_pop(m_state, 1);
                lua_newtable(m_state);
            }
            luaL_setfuncs(m_state, constructor, 0);
            lua_setglobal(m_state, klassName.c_str());
#else
            luaL_openlib(m_state, klassName.c_str(), constructor, 0);
#endif

			return (*this);
		}

        template<typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const char * name, TCLASS*(*spawner)(ARGS...)) {
            typedef decltype(spawner) SPAWNERFTYPE;
            struct HelperClass {
                static int f__gc(lua_State* state) {
                    TCLASS ** objPtr = (TCLASS**)luaL_checkudata(state, -1, LuaClass<TCLASS>::klassName.c_str());
                    if (objPtr)
                    {
                        //std::cout << "ctor(const char * name, F f):f__gc" << std::endl;
                        delete *objPtr;
                    }
                    return 0;
                }

                static int f_new(lua_State* state) {
                    void * spawner = lua_touserdata(state, lua_upvalueindex(1));
                    luaL_argcheck(state, spawner, 1, "cpp closure spawner not found.");
                    if (spawner) {
                        volatile int idx = sizeof...(ARGS); (void)(idx);
                        auto obj = (*(SPAWNERFTYPE*)(spawner))(LuaStack<ARGS>::get(state, idx--)...);
                        if (obj)
                        {
                            TCLASS ** objPtr = (TCLASS**)lua_newuserdata(state, sizeof(TCLASS*));
                            if (objPtr)
                            {
                                *objPtr = obj;

                                luaL_Reg destructor[] = {
                                    { "__gc", HelperClass::f__gc },
                                    { nullptr, nullptr }
                                };

                                luaL_getmetatable(state, LuaClass<TCLASS>::klassName.c_str());
                                luaL_setfuncs(state, destructor, 0);
                                lua_setmetatable(state, -2);
                                
                                return 1;
                            }
                            else
                            {
                                DestructorHelperClass<TCLASS>::f_delete(obj);
                            }
                        }
                    }
                    lua_pushnil(state);
                    return 1;
                }

            };

            luaL_Reg constructor[] = { { name, HelperClass::f_new },{ nullptr, nullptr } };
#if USE_NEW_MODULE_REGISTRY
            lua_getglobal(m_state, klassName.c_str());
            if (lua_isnil(m_state, -1))
            {
                lua_pop(m_state, 1);
                lua_newtable(m_state);
            }
#endif

            SPAWNERFTYPE * spawnerPtr = (SPAWNERFTYPE*)lua_newuserdata(m_state, sizeof(SPAWNERFTYPE));
            luaL_argcheck(m_state, spawnerPtr != nullptr, 1, (std::string("faild to alloc mem to store spawner for ctor `") + name + "`").c_str());
            *spawnerPtr = spawner;

#if USE_NEW_MODULE_REGISTRY
            luaL_setfuncs(m_state, constructor, 1);
            lua_setglobal(m_state, klassName.c_str());
#else
            luaL_openlib(m_state, klassName.c_str(), constructor, 1);
#endif

            return (*this);
        }

        template<typename TRET, typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const char * name, TCLASS*(*spawner)(ARGS...), TRET(*deleter)(TCLASS*)){
            typedef decltype(spawner) SPAWNERFTYPE;
            typedef decltype(deleter) DELETERFTYPE;

            struct HelperClass {
                static int f__gc(lua_State* state) {
                    void * deleter = lua_touserdata(state, lua_upvalueindex(1));
                    luaL_argcheck(state, deleter, 1, "cpp closure deleter not found.");
                    if (deleter) {
                        TCLASS ** objPtr = (TCLASS**)luaL_checkudata(state, -1, LuaClass<TCLASS>::klassName.c_str());
                        if (objPtr)
                        {
                            //std::cout << "ctor(const char * name, F f, D d):f__gc" << std::endl;
                            (*(DELETERFTYPE*)(deleter))(*objPtr);
                        }
                    }
                    
                    return 0;
                }

                static int f_new(lua_State* state) {
                    void * spawner = lua_touserdata(state, lua_upvalueindex(1));
                    luaL_argcheck(state, spawner, 1, "cpp closure spawner not found.");

                    void * deleter = lua_touserdata(state, lua_upvalueindex(2));
                    luaL_argcheck(state, deleter, 1, "cpp closure deleter not found.");

                    if (spawner) {
                        volatile int idx = sizeof...(ARGS); (void)(idx);
                        auto obj = (*(SPAWNERFTYPE*)(spawner))(LuaStack<ARGS>::get(state, idx--)...);
                        if (obj)
                        {
                            TCLASS ** objPtr = (TCLASS**)lua_newuserdata(state, sizeof(TCLASS*));
                            if (objPtr)
                            {
                                *objPtr = obj;

                                luaL_Reg destructor[] = {
                                    { "__gc", HelperClass::f__gc },
                                    { nullptr, nullptr }
                                };

                               
                                luaL_getmetatable(state, LuaClass<TCLASS>::klassName.c_str());

                                DELETERFTYPE * deleterPtr = (DELETERFTYPE*)lua_newuserdata(state, sizeof(DELETERFTYPE));
                                luaL_argcheck(state, deleterPtr != nullptr, 1, (std::string("faild to alloc mem to store deleter for ctor meta table `") + LuaClass<TCLASS>::klassName + "`").c_str());
                                *deleterPtr = *(DELETERFTYPE*)(deleter);

                                luaL_setfuncs(state, destructor, 1);

                                lua_setmetatable(state, -2);

                                return 1;
                            }
                            else
                            {
                                DestructorHelperClass<TCLASS>::f_delete(obj);
                            }
                        }
                    }
                    lua_pushnil(state);
                    return 1;
                }

            };

            luaL_Reg constructor[] = { { name, HelperClass::f_new },{ nullptr, nullptr } };

#if USE_NEW_MODULE_REGISTRY
            lua_getglobal(m_state, klassName.c_str());
            if (lua_isnil(m_state, -1))
            {
                lua_pop(m_state, 1);
                lua_newtable(m_state);
            }
#endif

            SPAWNERFTYPE * spawnerPtr = (SPAWNERFTYPE*)lua_newuserdata(m_state, sizeof(SPAWNERFTYPE));
            luaL_argcheck(m_state, spawnerPtr != nullptr, 1, (std::string("faild to alloc mem to store spawner for ctor `") + name + "`").c_str());
            *spawnerPtr = spawner;

            DELETERFTYPE * deleterPtr = (DELETERFTYPE*)lua_newuserdata(m_state, sizeof(DELETERFTYPE));
            luaL_argcheck(m_state, deleterPtr != nullptr, 1, (std::string("faild to alloc mem to store deleter for ctor `") + name + "`").c_str());
            *deleterPtr = deleter;

#if USE_NEW_MODULE_REGISTRY
            luaL_setfuncs(m_state, constructor, 2);
            lua_setglobal(m_state, klassName.c_str());
#else
            luaL_openlib(m_state, klassName.c_str(), constructor, 2);
#endif

            return (*this);
        }

        template<typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const char * name, TCLASS*(*spawner)(ARGS...), nullptr_t) {
            typedef decltype(spawner) SPAWNERFTYPE;

            struct HelperClass {
                static int f__nogc(lua_State* state) {
                    //std::cout << "ctor(const char * name, TCLASS*(*spawner)(ARGS...), nullptr_t):f__nogc" << std::endl;
                    return 0;
                }

                static int f_new(lua_State* state) {
                    void * spawner = lua_touserdata(state, lua_upvalueindex(1));
                    luaL_argcheck(state, spawner, 1, "cpp closure spawner not found.");
                    if (spawner) {
                        volatile int idx = sizeof...(ARGS)+1; (void)(idx);
                        auto obj = (*(SPAWNERFTYPE*)(spawner))(LuaStack<ARGS>::get(state, idx--)...);
                        if (obj)
                        {
                            TCLASS ** objPtr = (TCLASS**)lua_newuserdata(state, sizeof(TCLASS*));
                            if (objPtr)
                            {
                                *objPtr = obj;

                                luaL_Reg destructor[] = {
                                    { "__gc", HelperClass::f__nogc },
                                    { nullptr, nullptr }
                                };

                                luaL_getmetatable(state, LuaClass<TCLASS>::klassName.c_str());
                                luaL_setfuncs(state, destructor, 0);
                                lua_setmetatable(state, -2);
                                return 1;
                            }
                            else
                            {
                                DestructorHelperClass<TCLASS>::f_delete(obj);
                            }
                        }
                        
                    }
                    lua_pushnil(state);
                    return 1;
                }

            };

            luaL_Reg constructor[] = { { name, HelperClass::f_new },{ nullptr, nullptr } };

#if USE_NEW_MODULE_REGISTRY
            lua_getglobal(m_state, klassName.c_str());
            if (lua_isnil(m_state, -1))
            {
                lua_pop(m_state, 1);
                lua_newtable(m_state);
            }
#endif
            SPAWNERFTYPE * spawnerPtr = (SPAWNERFTYPE*)lua_newuserdata(m_state, sizeof(SPAWNERFTYPE));
            luaL_argcheck(m_state, spawnerPtr != nullptr, 1, (std::string("faild to alloc mem to store spawner for ctor `") + name + "`").c_str());
            *spawnerPtr = spawner;

#if USE_NEW_MODULE_REGISTRY
            luaL_setfuncs(m_state, constructor, 1);
            lua_setglobal(m_state, klassName.c_str());
#else
            luaL_openlib(m_state, klassName.c_str(), constructor, 1);
#endif

            return (*this);
        }

       

        template<typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const std::string& name)
        {
            return ctor<ARGS...>(name.c_str());
        }

        template<typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const std::string& name, TCLASS*(*spawner)(ARGS...)) {
            return ctor<F>(name.c_str(), spawner);
        }

        template<typename TRET, typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const std::string& name, TCLASS*(*spawner)(ARGS...), TRET(*deleter)(TCLASS*)) {
            return ctor<F, D>(name.c_str(), spawner, deleter);
        }

        template<typename ...ARGS>
        inline LuaClass<TCLASS>& ctor(const std::string& name, TCLASS*(*spawner)(ARGS...), nullptr_t) {
            return ctor<F, D>(name.c_str(), spawner, nullptr);
        }

		template<typename F>
		inline LuaClass<TCLASS>& fun(const char * name, F f)
		{
			luaL_getmetatable(m_state, klassName.c_str());
			lua_pushstring(m_state, name);

			F * funPtr = (F*)lua_newuserdata(m_state, sizeof(F));
			luaL_argcheck(m_state, funPtr != nullptr, 1, (std::string("faild to alloc mem to store function `") + name + "`").c_str());
			*funPtr = f;
			lua_pushcclosure(m_state, MemberFunctionCaller(f), 1);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		inline LuaClass<TCLASS>& fun(const char * name, lua_CFunction f)
		{
			luaL_getmetatable(m_state, klassName.c_str());
			lua_pushstring(m_state, name);
			lua_pushcclosure(m_state, f, 0);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		template <typename F>
		inline LuaClass<TCLASS>& fun(const std::string& name, F f)
		{
			return fun(name.c_str(), f);
		}

		template <typename V>
		inline LuaClass<TCLASS>& def(const char * name, const V& val)
		{
			luaL_getmetatable(m_state, klassName.c_str());
			lua_pushstring(m_state, name);
			LuaStack<V>::put(m_state, val);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		// disable cast from "const char [#]" to "char (*)[#]"
		inline LuaClass<TCLASS>& def(const char * name, const char * str)
		{
			luaL_getmetatable(m_state, klassName.c_str());
			lua_pushstring(m_state, name);
			LuaStack<decltype(str)>::put(m_state, str);
			lua_settable(m_state, -3);
			lua_pop(m_state, 1);
			return (*this);
		}

		template <typename V>
		inline LuaClass<TCLASS>& def(const std::string& name, const V& val)
		{
			return def(name.c_str(), val);
		}

	private:
		lua_State *	m_state;

	private:
		template<typename> friend struct LuaStack;
		static std::string klassName;
	};

	template <typename TCLASS> std::string LuaClass<TCLASS>::klassName;


	// -----------------------------------
	// export module
	// -----------------------------------
	struct LuaModule
	{
	public:
		LuaModule(lua_State * state, const std::string& name = "_G")
			: m_state(state)
			, m_moduleName(name.empty() ? "_G" : name)
		{
		}

	public:
		template<typename F>
		inline LuaModule& fun(const char * name, F f)
		{
			luaL_Reg regtab[] = { { name, NonMemberFunctionCaller(f) },{ nullptr, nullptr } };

#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName.c_str());
			if (lua_isnil(m_state, -1)) 
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}

			F * funPtr = (F*)lua_newuserdata(m_state, sizeof(F));
			luaL_argcheck(m_state, funPtr != nullptr, 1, (std::string("faild to alloc mem to store function `") + name + "`").c_str());
			*funPtr = f;

			luaL_setfuncs(m_state, regtab, 1);
			lua_setglobal(m_state, m_moduleName.c_str());
#else
			F * funPtr = (F*)lua_newuserdata(m_state, sizeof(F));
			luaL_argcheck(m_state, funPtr != nullptr, 1, (std::string("faild to alloc mem to store function `") + name + "`").c_str());
			*funPtr = f;

			luaL_openlib(m_state, m_moduleName.c_str(), regtab, 1);
#endif

			return (*this);
		}

		inline LuaModule& fun(const char * name, lua_CFunction f)
		{
			luaL_Reg regtab[] = { { name, f },{ nullptr, nullptr } };
#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName.c_str());
			if (lua_isnil(m_state, -1))
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}
			luaL_setfuncs(m_state, regtab, 0);
			lua_setglobal(m_state, m_moduleName.c_str());
#else
			luaL_openlib(m_state, m_moduleName.c_str(), regtab, 0);
#endif
			return (*this);
		}

		template <typename V>
		inline LuaModule& def(const char * name, const V& val)
		{
#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName.c_str());
			if (lua_isnil(m_state, -1))
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}
			LuaStack<V>::put(m_state, val);
			lua_setfield(m_state, -2, name);
			lua_setglobal(m_state, m_moduleName.c_str());
#else
			luaL_Reg regtab = { nullptr, nullptr };
			luaL_openlib(m_state, m_moduleName.c_str(), &regtab, 0);
			LuaStack<V>::put(m_state, val);
			lua_setfield(m_state, -2, name);
#endif
			return (*this);
		}

		// disable the cast from "const char [#]" to "char (*)[#]"
		inline LuaModule& def(const char * name, const char * str)
		{
			
#if USE_NEW_MODULE_REGISTRY
			lua_getglobal(m_state, m_moduleName.c_str());
			if (lua_isnil(m_state, -1))
			{
				lua_pop(m_state, 1);
				lua_newtable(m_state);
			}
			LuaStack<decltype(str)>::put(m_state, str);
			lua_setfield(m_state, -2, name);
			lua_setglobal(m_state, m_moduleName.c_str());
#else
			luaL_Reg regtab = { nullptr, nullptr };
			luaL_openlib(m_state, m_moduleName.c_str(), &regtab, 0);
			LuaStack<decltype(str)>::put(m_state, str);
			lua_setfield(m_state, -2, name);
#endif
			return (*this);
		}

		template <typename V>
		inline LuaModule& def(const std::string& name, const V& val)
		{
			return def(name.c_str(), val);
		}

	private:
		lua_State *	m_state;
		std::string m_moduleName;
	};

}

#endif