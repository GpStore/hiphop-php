set(CXX_SOURCES)
auto_sources(files "*.cpp" "")
list(APPEND CXX_SOURCES ${files})

add_executable(hphp ${CXX_SOURCES})
find_library(DL_LIBRARY dl)

IF(DL_LIBRARY)
    target_link_libraries(hphp ${DL_LIBRARY})
ENDIF()

target_link_libraries(hphp hphp_analysis hphp_runtime_static)

