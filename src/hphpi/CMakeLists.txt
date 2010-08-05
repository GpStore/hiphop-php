add_custom_target(hphpi DEPENDS hphp)

IF(EXISTS "${HPHP_HOME}/src/hphpi/gen/CMakeCache.txt")
  file(REMOVE "${HPHP_HOME}/src/hphpi/gen/CMakeCache.txt")
ENDIF()

add_custom_command(
        TARGET hphpi
        COMMAND hphp ARGS -t cpp -f exe --input-dir . -i hphpi.php -o gen -c hphpi_build.hdf --log=1
        COMMAND mv ARGS gen/program hphpi
        WORKING_DIRECTORY ${HPHP_HOME}/src/hphpi
        COMMENT "Building hphpi"
)

