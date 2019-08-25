#pragma once

#include <cstdint>
#include <stdio.h>

#define QNAN_COUNTER_READ_CRUMB(TAG, RANK, ACC) \
	counter_crumb(TAG(), constant_index<RANK>(), constant_index<ACC>())
#define QNAN_COUNTER_READ(TAG)                       \
	QNAN_COUNTER_READ_CRUMB(                         \
	    TAG, 1,                                      \
	    QNAN_COUNTER_READ_CRUMB(                     \
	        TAG, 2,                                  \
	        QNAN_COUNTER_READ_CRUMB(                 \
	            TAG, 4,                              \
	            QNAN_COUNTER_READ_CRUMB(             \
	                TAG, 8,                          \
	                QNAN_COUNTER_READ_CRUMB(         \
	                    TAG, 16,                     \
	                    QNAN_COUNTER_READ_CRUMB(     \
	                        TAG, 32,                 \
	                        QNAN_COUNTER_READ_CRUMB( \
	                            TAG, 64,             \
	                            QNAN_COUNTER_READ_CRUMB(TAG, 128, 0))))))))

#define QNAN_COUNTER_INC(TAG)                                           \
	constant_index<QNAN_COUNTER_READ(TAG) + 1> constexpr counter_crumb( \
	    TAG,                                                            \
	    constant_index<(QNAN_COUNTER_READ(TAG) + 1) &                   \
	                   ~QNAN_COUNTER_READ(TAG)>,                        \
	    constant_index<(QNAN_COUNTER_READ(TAG) + 1) &                   \
	                   QNAN_COUNTER_READ(TAG)>) {                       \
		return {};                                                      \
	}

//#define QNAN_COUNTER_LINK_NAMESPACE(NS) using NS::counter_crumb;

#include <utility>

template <std::size_t n>
struct constant_index : std::integral_constant<std::size_t, n> {};

template <typename id, std::size_t rank, std::size_t acc>
constexpr constant_index<acc> counter_crumb(id, constant_index<rank>,
                                            constant_index<acc>) {
	return {};
} // found by ADL via constant_index

struct QNAN_COUNTER {};

//  0111 1111 1111 1[---] 0000 0000 0000 [data]
const uint64_t VAL_QNAN      = 0x7ff8000000000000;
const uint64_t VAL_MASK      = 0x0000ffffffffffff;
const uint64_t VAL_TYPE_MASK = 0x0007000000000000;
const uint64_t VAL_TAG_MASK  = 0xffff000000000000;

#define VAL(v) (((v)&VAL_MASK))
#define VAL_TAG(v) (((v)&VAL_TAG_MASK))
#define VAL_TYPE(v) (((v)&VAL_TYPE_MASK) >> 48)

constexpr uint64_t encodeType(int num) {
	uint64_t encoding = num;
	encoding <<= 48;
	return VAL_QNAN | encoding;
}

QNAN_COUNTER_INC(QNAN_COUNTER);
const uint64_t QNAN_NIL = encodeType(QNAN_COUNTER_READ(QNAN_COUNTER));

#define TYPE(type, name)            \
	QNAN_COUNTER_INC(QNAN_COUNTER); \
	const uint64_t QNAN_##name = encodeType(QNAN_COUNTER_READ(QNAN_COUNTER));
#include "valuetypes.h"
#undef TYPE
