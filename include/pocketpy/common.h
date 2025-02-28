#pragma once

#include <cmath>
#include <cstring>

#include <sstream>
#include <regex>
#include <stdexcept>
#include <vector>
#include <string>
#include <chrono>
#include <string_view>
#include <iomanip>
#include <memory>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <initializer_list>
#include <variant>
#include <type_traits>
#include <random>
#include <bitset>

#define PK_VERSION				"1.2.7"

#include "config.h"
#include "export.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

/*******************************************************************************/
#if PK_ENABLE_STD_FUNCTION
#include <functional>
#endif
/*******************************************************************************/

#if PK_ENABLE_THREAD
#define PK_THREAD_LOCAL thread_local
#include <mutex>

struct GIL {
	inline static std::mutex _mutex;
    explicit GIL() { _mutex.lock(); }
    ~GIL() { _mutex.unlock(); }
};
#define PK_GLOBAL_SCOPE_LOCK() GIL _lock;

#else
#define PK_THREAD_LOCAL
#define PK_GLOBAL_SCOPE_LOCK()
#endif

/*******************************************************************************/

#define PK_UNUSED(x) (void)(x)

#define PK_LOCAL_STATIC static

namespace pkpy{

namespace std = ::std;

template <size_t T>
struct NumberTraits;

template <>
struct NumberTraits<4> {
	using int_t = int32_t;
	using float_t = float;

	static constexpr int_t kMaxSmallInt = (1 << 28) - 1;
	static constexpr int_t kMinSmallInt = - (1 << 28);
};

template <>
struct NumberTraits<8> {
	using int_t = int64_t;
	using float_t = double;

	static constexpr int_t kMaxSmallInt = (1ll << 60) - 1;
	static constexpr int_t kMinSmallInt = - (1ll << 60);
};

using Number = NumberTraits<sizeof(void*)>;
using i64 = int64_t;		// always 64-bit
using f64 = Number::float_t;

template<size_t T>
union BitsCvtImpl;

template<>
union BitsCvtImpl<4>{
	NumberTraits<4>::int_t _int;
	NumberTraits<4>::float_t _float;

	// 1 + 8 + 23
	int sign() const { return _int >> 31; }
	unsigned int exp() const { return (_int >> 23) & 0b1111'1111; }
	uint64_t mantissa() const { return _int & 0x7fffff; }

	void set_exp(int exp) { _int = (_int & 0x807f'ffff) | (exp << 23); }
	void set_sign(int sign) { _int = (_int & 0x7fff'ffff) | (sign << 31); }
	void zero_mantissa() { _int &= 0xff80'0000; }

	static constexpr int C0 = 127;	// 2^7 - 1
	static constexpr int C1 = -62;	// 2 - 2^6
	static constexpr int C2 = 63;	// 2^6 - 1
	static constexpr NumberTraits<4>::int_t C3 = 0b1011'1111'1111'1111'1111'1111'1111'1111;
	static constexpr int C4 = 0b11111111;

	BitsCvtImpl(NumberTraits<4>::float_t val): _float(val) {}
	BitsCvtImpl(NumberTraits<4>::int_t val): _int(val) {}

	void print(){
		std::string s = std::bitset<32>(_int).to_string();
		std::cout << s.substr(0, 1) << '|';
		std::cout << s.substr(1, 8) << '|';
		std::cout << s.substr(9) << std::endl;
	}
};

template<>
union BitsCvtImpl<8>{
	NumberTraits<8>::int_t _int;
	NumberTraits<8>::float_t _float;

	// 1 + 11 + 52
	int sign() const { return _int >> 63; }
	unsigned int exp() const { return (_int >> 52) & 0b0111'1111'1111; }
	uint64_t mantissa() const { return _int & 0xfffffffffffff; }

	void set_exp(uint64_t exp) { _int = (_int & 0x800f'ffff'ffff'ffff) | (exp << 52); }
	void set_sign(uint64_t sign) { _int = (_int & 0x7fff'ffff'ffff'ffff) | (sign << 63); }
	void zero_mantissa() { _int &= 0xfff0'0000'0000'0000; }

	static constexpr int C0 = 1023;	// 2^10 - 1
	static constexpr int C1 = -510;	// 2 - 2^9
	static constexpr int C2 = 511;	// 2^9 - 1
	static constexpr NumberTraits<8>::int_t C3 = 0b1011'1111'1111'1111'1111'1111'1111'1111'1111'1111'1111'1111'1111'1111'1111'1111;
	static constexpr int C4 = 0b11111111111;

	BitsCvtImpl(NumberTraits<8>::float_t val): _float(val) {}
	BitsCvtImpl(NumberTraits<8>::int_t val): _int(val) {}

	void print(){
		std::string s = std::bitset<64>(_int).to_string();
		std::cout << s.substr(0, 1) << '|';
		std::cout << s.substr(1, 11) << '|';
		std::cout << s.substr(12) << std::endl;
	}
};

using BitsCvt = BitsCvtImpl<sizeof(void*)>;

static_assert(sizeof(i64) == 8);
static_assert(sizeof(Number::float_t) == sizeof(void*));
static_assert(sizeof(Number::int_t) == sizeof(void*));
static_assert(sizeof(BitsCvt) == sizeof(void*));
static_assert(std::numeric_limits<f64>::is_iec559);

struct Dummy { };
struct DummyInstance { };
struct DummyModule { };
struct NoReturn { };
struct Discarded { };

struct Type {
	int index;
	Type(): index(-1) {}
	Type(int index): index(index) {}
	bool operator==(Type other) const { return this->index == other.index; }
	bool operator!=(Type other) const { return this->index != other.index; }
	operator int() const { return this->index; }
};

#define PK_LAMBDA(x) ([](VM* vm, ArgsView args) { return x; })
#define PK_VAR_LAMBDA(x) ([](VM* vm, ArgsView args) { return VAR(x); })
#define PK_ACTION(x) ([](VM* vm, ArgsView args) { x; return vm->None; })

#ifdef POCKETPY_H
#define FATAL_ERROR() throw std::runtime_error( "L" + std::to_string(__LINE__) + " FATAL_ERROR()!");
#else
#define FATAL_ERROR() throw std::runtime_error( __FILE__ + std::string(":") + std::to_string(__LINE__) + " FATAL_ERROR()!");
#endif

#define PK_ASSERT(x) if(!(x)) FATAL_ERROR();

struct PyObject;
#define PK_BITS(p) (reinterpret_cast<Number::int_t>(p))

inline PyObject* tag_float(f64 val){
	BitsCvt decomposed(val);
	// std::cout << "tagging: " << val << std::endl;
	int sign = decomposed.sign();
	int exp_7b = decomposed.exp() - BitsCvt::C0;
	if(exp_7b < BitsCvt::C1){
		exp_7b = BitsCvt::C1 - 1;	// -63 + 63 = 0
		decomposed.zero_mantissa();
	}else if(exp_7b > BitsCvt::C2){
		exp_7b = BitsCvt::C2 + 1;	// 64 + 63 = 127
		if(!std::isnan(val)) decomposed.zero_mantissa();
	}
	decomposed.set_exp(exp_7b + BitsCvt::C2);
	decomposed._int = (decomposed._int << 1) | 0b01;
	decomposed.set_sign(sign);
	return reinterpret_cast<PyObject*>(decomposed._int);
}

inline f64 untag_float(PyObject* val){
	BitsCvt decomposed(reinterpret_cast<Number::int_t>(val));
	// std::cout << "untagging: " << val << std::endl;
	decomposed._int = (decomposed._int >> 1) & BitsCvt::C3;
	unsigned int exp_7b = decomposed.exp();
	if(exp_7b == 0) return 0.0f;
	if(exp_7b == BitsCvt::C0){
		decomposed.set_exp(BitsCvt::C4);
		return decomposed._float;
	}
	decomposed.set_exp(exp_7b - BitsCvt::C2 + BitsCvt::C0);
	return decomposed._float;
}

// is_pod<> for c++17 and c++20
template<typename T>
struct is_pod {
	static constexpr bool value = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
};

#define PK_ALWAYS_PASS_BY_POINTER(T) \
	T(const T&) = delete; \
	T& operator=(const T&) = delete; \
	T(T&&) = delete; \
	T& operator=(T&&) = delete;

} // namespace pkpy
