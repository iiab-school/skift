#pragma once

#include "clamp.h"
#include "iter.h"
#include "slice.h"
#include "std.h"

namespace Karm {

template <typename T, usize N>
struct Array {
    using Inner = T;

    T _buf[N] = {};

    constexpr T &operator[](usize i) {
        if (i >= N) {
            panic("index out of range");
        }
        return _buf[i];
    }

    constexpr T const &operator[](usize i) const {
        if (i >= N) {
            panic("index out of range");
        }
        return _buf[i];
    }

    constexpr usize len() const { return N; }

    constexpr T *buf() { return _buf; }

    constexpr T const *buf() const { return _buf; }

    constexpr bool operator==(Array const &other) const {
        for (usize i = 0; i < N; i++) {
            if (_buf[i] != other._buf[i]) {
                return false;
            }
        }
        return true;
    }

    Bytes bytes() const {
        return {reinterpret_cast<Byte const *>(buf()), len() * sizeof(T)};
    }

    MutBytes mutBytes() {
        return {reinterpret_cast<Byte *>(buf()), len() * sizeof(T)};
    }
};

template <class T, class... U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

} // namespace Karm
