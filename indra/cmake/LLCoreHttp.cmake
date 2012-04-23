# -*- cmake -*-

include(CARes)
include(CURL)
include(OpenSSL)

set(LLCOREHTTP_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llcorehttp
    ${CARES_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    )

set(LLCOREHTTP_LIBRARIES llcorehttp)
