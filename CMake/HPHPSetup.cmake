OPTION(INFINITE_LOOP_DETECTION "Enable Infinite Loop Detection" ON)
OPTION(INFINITE_RECURSION_DETECTION "Enable Infinite Recursion Detection" ON)
OPTION(REQUEST_TIMEOUT_DETECTION "Enable Timeout Detection" ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

include(HPHPFunctions)
include(HPHPFindLibs)

add_definitions(-D_GNU_SOURCE -D_REENTRANT=1 -D_PTHREADS=1)

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
	add_definitions(-DRELEASE=1)
endif()

# eable the OSS options if we have any
add_definitions(-DHPHP_OSS=1)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
  add_definitions(-DFREEBSD)
endif()

set(CMAKE_C_FLAGS "-w -fPIC")
set(CMAKE_CXX_FLAGS "-fPIC -fno-omit-frame-pointer -ftemplate-depth-60 -Wall -Woverloaded-virtual -Wno-deprecated -Wno-parentheses -Wno-strict-aliasing -Wno-write-strings -Wno-invalid-offsetof")

include_directories(${HPHP_HOME}/src)
include_directories(${HPHP_HOME}/src/lib/system/gen)
