# -*- cmake -*-
#
# Compilation options shared by all Second Life components.

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

include(CheckCCompilerFlag)
include(Variables)

# Portable compilation flags.
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -DLL_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE
    "-DLL_RELEASE=1 -DLL_RELEASE_FOR_DOWNLOAD=1 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "-DLL_RELEASE=1 -DNDEBUG -DLL_RELEASE_WITH_DEBUG_INFO=1")

# Configure crash reporting
option(RELEASE_CRASH_REPORTING "Enable use of crash reporting in release builds" OFF)
option(NON_RELEASE_CRASH_REPORTING "Enable use of crash reporting in developer builds" OFF)

if(RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLL_SEND_CRASH_REPORTS=1")
endif()

if(NON_RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DLL_SEND_CRASH_REPORTS=1")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLL_SEND_CRASH_REPORTS=1")
endif()

# Don't bother with a MinSizeRel build.
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release;Debug" CACHE STRING
    "Supported build types." FORCE)

# Platform-specific compilation flags.
if (WINDOWS)
  # Don't build DLLs.
  set(BUILD_SHARED_LIBS OFF)

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /MDd /MP -D_SCL_SECURE_NO_WARNINGS=1"
      CACHE STRING "C++ compiler debug options" FORCE)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od /Zi /MD /MP /Ob0 -D_SECURE_STL=0"
      CACHE STRING "C++ compiler release-with-debug options" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /Zi /Zo /MD /MP /Ob2 -D_SECURE_STL=0 -D_HAS_ITERATOR_DEBUGGING=0"
      CACHE STRING "C++ compiler release options" FORCE)

  if (WORD_SIZE EQUAL 32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  endif (WORD_SIZE EQUAL 32)

  if (USE_LTO AND NOT INCREMENTAL_LINK)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:REF /OPT:ICF /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF /OPT:ICF /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /LTCG")
  elseif (INCREMENTAL_LINK AND NOT USE_LTO)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS} /INCREMENTAL /VERBOSE:INCR")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL /VERBOSE:INCR")
  else ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:REF /INCREMENTAL:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF /INCREMENTAL:NO")
  endif (USE_LTO AND NOT INCREMENTAL_LINK)

  set(CMAKE_CXX_STANDARD_LIBRARIES "")
  set(CMAKE_C_STANDARD_LIBRARIES "")

  add_compile_options(
      /GS
      /TP
      /W3
      /c
      /Zc:wchar_t
      /Zc:forScope
      /nologo
      /Oy-
      /fp:fast
      /Zm140
      )

  add_definitions(
      /DLL_WINDOWS=1
      /DNOMINMAX
      /DUNICODE
      /D_UNICODE
      /D_CRT_SECURE_NO_WARNINGS
      /D_WINSOCK_DEPRECATED_NO_WARNINGS
      )

  if (USE_LTO AND NOT INCREMENTAL_LINK)
    add_compile_options(
        /GL
        /Gy
        /Gw
        )
  endif (USE_LTO AND NOT INCREMENTAL_LINK)

  if (USE_AVX)
    add_compile_options(/arch:AVX)
  elseif (USE_AVX2)
    add_compile_options(/arch:AVX2)
  elseif (WORD_SIZE EQUAL 32)
    add_compile_options(/arch:SSE2)
  endif (USE_AVX)

  if (NOT VS_DISABLE_FATAL_WARNINGS)
    add_compile_options(/WX)
  endif (NOT VS_DISABLE_FATAL_WARNINGS)

  # configure win32 API for windows vista+ compatibility
  set(WINVER "0x0600" CACHE STRING "Win32 API Target version (see http://msdn.microsoft.com/en-us/library/aa383745%28v=VS.85%29.aspx)")
  add_definitions("/DWINVER=${WINVER}" "/D_WIN32_WINNT=${WINVER}")
endif (WINDOWS)


if (LINUX)
  option(CONSERVE_MEMORY "Optimize for memory usage during link stage for memory-starved systems" OFF)
  set(CMAKE_SKIP_RPATH TRUE)

  add_compile_options(
    -fvisibility=hidden
    -fexceptions
    -fno-math-errno
    -fno-strict-aliasing
    -fsigned-char
    -std=gnu++11
    -g
    -pthread
    )

  add_definitions(
    -DLL_LINUX=1
    -DAPPID=secondlife
    -DLL_IGNORE_SIGCHLD
    -D_REENTRANT
    )

  # Explicitly disable new C++11 ABI for GCC5/libstd++
  # 1. https://llvm.org/bugs/show_bug.cgi?id=23529
  # 2. https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

  CHECK_C_COMPILER_FLAG(-Og HAS_DEBUG_OPTIMIZATION)
  CHECK_C_COMPILER_FLAG(-fstack-protector-strong HAS_STRONG_STACK_PROTECTOR)
  CHECK_C_COMPILER_FLAG(-fstack-protector HAS_STACK_PROTECTOR)

  if (USE_LTO)
    add_compile_options(-flto=8)
  endif (USE_LTO)

  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(HAS_STRONG_STACK_PROTECTOR)
      add_compile_options(-fstack-protector-strong)
    elseif(HAS_STACK_PROTECTOR)
      add_compile_options(-fstack-protector)
    endif(HAS_STRONG_STACK_PROTECTOR)
    add_definitions(-D_FORTIFY_SOURCE=2)
  endif (${CMAKE_BUILD_TYPE} STREQUAL "Release")

  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    if (WORD_SIZE EQUAL 32 AND NOT USESYSTEMLIBS)
      add_compile_options(
        -msse2
        -mfpmath=sse
        -march=pentium4)
    endif (WORD_SIZE EQUAL 32 AND NOT USESYSTEMLIBS)

    if (HAS_DEBUG_OPTIMIZATION)
      set(CMAKE_CXX_FLAGS_DEBUG "-Og ${CMAKE_CXX_FLAGS_DEBUG}")
    else (HAS_DEBUG_OPTIMIZATION)
      set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fno-inline ${CMAKE_CXX_FLAGS_DEBUG}")
    endif (HAS_DEBUG_OPTIMIZATION)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS_RELEASE}")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if (WORD_SIZE EQUAL 32 AND NOT USESYSTEMLIBS)
      add_definitions(-msse2 -march=pentium4)
    endif (WORD_SIZE EQUAL 32 AND NOT USESYSTEMLIBS)

    set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    if (WORD_SIZE EQUAL 32 AND NOT USESYSTEMLIBS)
      add_compile_options(-march=pentium4)
    endif (WORD_SIZE EQUAL 32 AND NOT USESYSTEMLIBS)

    set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 ${CMAKE_CXX_FLAGS_RELEASE}")
  endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

  if (NOT USESYSTEMLIBS AND CMAKE_SIZEOF_VOID_P EQUAL 4 OR CONSERVE_MEMORY)
    # linking can be very memory-hungry, especially the final viewer link
    set(CMAKE_CXX_LINK_FLAGS "-Wl,--no-keep-memory")
  endif (NOT USESYSTEMLIBS AND CMAKE_SIZEOF_VOID_P EQUAL 4 OR CONSERVE_MEMORY)
endif (LINUX)


if (DARWIN)
  add_definitions(-DLL_DARWIN=1)
  set(CMAKE_CXX_LINK_FLAGS "-Wl,-no_compact_unwind -Wl,-headerpad_max_install_names,-search_paths_first")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
  set(DARWIN_extra_cstar_flags "-g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DARWIN_extra_cstar_flags}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${DARWIN_extra_cstar_flags}")
  # NOTE: it's critical that the optimization flag is put in front.
  # NOTE: it's critical to have both CXX_FLAGS and C_FLAGS covered.
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O0 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O0 ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS_RELEASE}")
  set(CMAKE_C_FLAGS_RELEASE "-O3 ${CMAKE_C_FLAGS_RELEASE}")
endif (DARWIN)


if (LINUX OR DARWIN)
  if (NOT UNIX_DISABLE_FATAL_WARNINGS)
    set(UNIX_WARNINGS "-Werror")
  endif (NOT UNIX_DISABLE_FATAL_WARNINGS)

  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(UNIX_WARNINGS "-Wall -Wno-sign-compare ${UNIX_WARNINGS} ")
    set(UNIX_CXX_WARNINGS "${UNIX_WARNINGS} -Wno-reorder")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(UNIX_WARNINGS "-Wall -Wno-sign-compare ${UNIX_WARNINGS} ")
    set(UNIX_CXX_WARNINGS "${UNIX_WARNINGS} -Wno-reorder -Wno-unused-local-typedef -Wempty-body -Wunreachable-code -Wundefined-bool-conversion -Wenum-conversion -Wassign-enum -Wint-conversion -Wconstant-conversion -Wnewline-eof -Wno-protocol -Wno-deprecated-declarations")
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(UNIX_WARNINGS "-w2 -diag-disable remark -wd68 -wd597 -wd780 -wd858 ${UNIX_WARNINGS} ")
    set(UNIX_CXX_WARNINGS "${UNIX_WARNINGS}")
  endif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${UNIX_WARNINGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${UNIX_CXX_WARNINGS}")

  if (WORD_SIZE EQUAL 32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  elseif (WORD_SIZE EQUAL 64)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  endif (WORD_SIZE EQUAL 32)
endif (LINUX OR DARWIN)


if (USESYSTEMLIBS)
  add_definitions(-DLL_USESYSTEMLIBS=1)

  if (LINUX AND ${ARCH} STREQUAL "i686")
    add_compile_options(-march=pentiumpro)
  endif (LINUX AND ${ARCH} STREQUAL "i686")

else (USESYSTEMLIBS)
  set(${ARCH}_linux_INCLUDES
      ELFIO
      atk-1.0
      cairo
      gdk-pixbuf-2.0
      glib-2.0
      gstreamer-0.10
      gtk-2.0
      pango-1.0
      pixman-1
      )
endif (USESYSTEMLIBS)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
