#pragma once

#include <tuple>
#include <type_traits>

struct FuncUtils {
	template <typename T> struct return_type;
	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...)> {
		using type = R;
	};
	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) &> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) &&> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) const> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) const &> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) const &&> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) volatile> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) volatile &> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) volatile &&> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) const volatile> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) const volatile &> {
		using type = R;
	};

	template <typename R, typename C, typename... Args>
	struct return_type<R (C::*)(Args...) const volatile &&> {
		using type = R;
	};

	template <typename T> using return_type_t = typename return_type<T>::type;
#define CHECK_IF_EXISTS(x)                                                     \
	template <typename T> struct has_##x {                                     \
		typedef char one;                                                      \
		struct two {                                                           \
			char x[2];                                                         \
		};                                                                     \
		template <typename C> static one test(decltype(&C::x));                \
		template <typename C> static two test(...);                            \
		enum { value = sizeof(test<T>(0)) == sizeof(char) };                   \
	};                                                                         \
	template <typename T, typename R, typename... K>                           \
	static typename std::enable_if<                                            \
	    has_##x<T>::value, FuncUtils::return_type_t<decltype(&T::x)>>::type    \
	    CallIfExists_##x(T *obj, R def, const K &...args) {                    \
		(void)def;                                                             \
		return obj->x(args...);                                                \
	}                                                                          \
	template <typename T, typename R, typename... K>                           \
	static                                                                     \
	    typename std::enable_if<!has_##x<T>::value, R>::type CallIfExists_##x( \
	        T *obj, R def, const K &...args) {                                 \
		(void)obj;                                                             \
		static_cast<void>(std::make_tuple(args...));                           \
		return def;                                                            \
	};                                                                         \
	template <typename T, typename... K>                                       \
	static typename std::enable_if<has_##x<T>::value, void>::type              \
	    CallIfExists_##x(T *obj, const K &...args) {                           \
		obj->x(args...);                                                       \
	}                                                                          \
	template <typename T, typename... K>                                       \
	static typename std::enable_if<!has_##x<T>::value, void>::type             \
	    CallIfExists_##x(T *obj, const K &...args) {                           \
		static_cast<void>(std::make_tuple(args...));                           \
		(void)obj;                                                             \
	};                                                                         \
	template <typename T, typename R, typename... K>                           \
	static typename std::enable_if<                                            \
	    has_##x<T>::value, FuncUtils::return_type_t<decltype(&(T::x))>>::type  \
	    CallStaticIfExists_##x(R def, const K &...args) {                      \
		(void)def;                                                             \
		return T::x(args...);                                                  \
	}                                                                          \
	template <typename T, typename R, typename... K>                           \
	static typename std::enable_if<!has_##x<T>::value, R>::type                \
	    CallStaticIfExists_##x(R def, const K &...args) {                      \
		static_cast<void>(std::make_tuple(args...));                           \
		return def;                                                            \
	};                                                                         \
	template <typename T, typename... K>                                       \
	static typename std::enable_if<has_##x<T>::value>::type                    \
	    CallStaticIfExists_##x(const K &...args) {                             \
		T::x(args...);                                                         \
	}                                                                          \
	template <typename T, typename... K>                                       \
	static typename std::enable_if<!has_##x<T>::value>::type                   \
	    CallStaticIfExists_##x(const K &...args) {                             \
		static_cast<void>(std::make_tuple(args...));                           \
	};                                                                         \
	template <typename T, typename R>                                          \
	static typename std::enable_if<FuncUtils::has_##x<T>::value, R>::type      \
	    GetIfExists_##x() {                                                    \
		return &T::x;                                                          \
	}                                                                          \
	template <typename T, typename R>                                          \
	static typename std::enable_if<!FuncUtils::has_##x<T>::value, R>::type     \
	    GetIfExists_##x() {                                                    \
		return nullptr;                                                        \
	}
	CHECK_IF_EXISTS(mark)    // called while gc marking
	CHECK_IF_EXISTS(release) // called which gc release, to release any custom
	                         // allocated memory
	CHECK_IF_EXISTS(depend)  // called while DEBUG_GC is on, to mark any of the
	                         // members necessary for its gc representation
	CHECK_IF_EXISTS(gc_repr) // called while DEBUG_GC is on, to print a
	                         // representation of the object while release
	                         //
	                         //
	CHECK_IF_EXISTS(preInit) // called while init'ing a builtin module, before
	                         // calling 'init'. a module can allocate custom
	                         // structures and memory if it chooses to do so.
	CHECK_IF_EXISTS(init)    // called while init'ing a builtin module, a
	                         // BuiltinModule* is passed, all the members are
	                         // needed to be registered on this.
	CHECK_IF_EXISTS(destroy) // called while gc releasing a builtin module.
	                         // current instance of the module is passed
};
