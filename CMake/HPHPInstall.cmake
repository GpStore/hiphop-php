# install hphp and hphpi
install(PROGRAMS src/hphp/hphp src/hphpi/hphpi
	DESTINATION bin)

# copy CMake and doc
install(DIRECTORY CMake doc 
	DESTINATION share/hphp)

# install everything under bin except for bin/run.sh
install(DIRECTORY bin 
	DESTINATION share/hphp
	PATTERN "run.sh" EXCLUDE)

# bin/run.sh is installed executable
install(PROGRAMS bin/run.sh
	DESTINATION share/hphp/bin)

# install src/ffi
install(DIRECTORY src/ffi
	DESTINATION share/hphp/src)

# install headers from src/runtime src/system src/util src/third_party
set(install_header_dirs runtime system util third_party)
foreach(dir ${install_header_dirs})
	file(GLOB_RECURSE header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src/" 
          PATTERN "src/${dir}/*.h" PATTERN "src/${dir}/*.hpp" PATTERN "src/${dir}/*.hh" PATTERN "src/${dir}/*.inc")
	foreach(item ${header_files})
		GET_FILENAME_COMPONENT(filepath ${item} PATH)
		install(FILES src/${item} DESTINATION share/hphp/src/${filepath})
	endforeach()
endforeach()

# install license and readme files
install(FILES LICENSE.PHP LICENSE.ZEND README.md 
	DESTINATION share/hphp)

# enable CPack
include(CPack)

