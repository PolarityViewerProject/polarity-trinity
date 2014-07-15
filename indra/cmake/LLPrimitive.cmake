# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)

use_prebuilt_binary(colladadom)
use_prebuilt_binary(pcre)
use_prebuilt_binary(libxml2)

set(LLPRIMITIVE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llprimitive
    )
if (WINDOWS)
  if (MSVC12)
    set(LLPRIMITIVE_LIBRARIES 
        debug llprimitive
        optimized llprimitive
        debug libcollada14dom23-sd
        optimized libcollada14dom23-s
        libxml2_a
        debug pcrecppd
        optimized pcrecpp
        debug pcred
        optimized pcre
        ${BOOST_SYSTEM_LIBRARIES}
        )
  else (MSVC12)
    set(LLPRIMITIVE_LIBRARIES
        debug llprimitive
        optimized llprimitive
        debug libcollada14dom22-d
        optimized libcollada14dom22
        ${BOOST_SYSTEM_LIBRARIES}
        )
  endif (MSVC12)
elseif (LINUX)
    set(LLPRIMITIVE_LIBRARIES
        llprimitive
        collada14dom
        minizip
        xml2
        pcrecpp
        pcre
        )
else (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES
        llprimitive
        collada14dom
        minizip
        xml2
        pcrecpp
        pcre
        )
endif (WINDOWS)

