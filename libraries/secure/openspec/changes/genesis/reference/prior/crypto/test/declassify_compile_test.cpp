// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Every header that marks a verdict public for the constant-time checks
// (WEBCPP_CRYPTO_DETAIL_DECLASSIFY) compiles with the marks on, as the constant-time lane's
// memcheck programs build them (WEBCPP_CRYPTO_DETAIL_CT_VALGRIND), on every lane: Valgrind's
// header is a stand-in that reads the address and the size (support/valgrind_stub). Outside that
// lane the macro expands to nothing, so a mark that names a local no longer there compiles
// everywhere else; here it stops the build.

#include <webcpp/crypto/backends/bearssl.hpp>
#include <webcpp/crypto/backends/sodium.hpp>
#include <webcpp/crypto/curves/p256/formats.hpp>
#include <webcpp/crypto/curves/secp256k1.hpp>
#include <webcpp/crypto/curves/secp256k1/formats.hpp>
#include <webcpp/crypto/curves/secp256k1/silent_payments.hpp>
#include <webcpp/crypto/curves/x25519.hpp>
#include <webcpp/crypto/detail/declassify.hpp>
#include <webcpp/crypto/detail/p_curves.hpp>

#ifndef WEBCPP_CRYPTO_DETAIL_CT_VALGRIND
#error "the test compiles the marks of the memcheck build: define WEBCPP_CRYPTO_DETAIL_CT_VALGRIND"
#endif

int main() {
    return 0;
}
