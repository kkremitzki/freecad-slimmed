# Try to find nglib/netgen
# 
# Optional input NETGENDATA is path to the netgen libsrc source tree - this is
# required due to some headers not being installed by netgen.
#
# Once done this will define
#
# NGLIB_INCLUDE_DIR   - where the nglib include directory can be found
# NGLIB_LIBRARIES     - Link this to use nglib
# NETGEN_INCLUDE_DIRS - where the netgen include directories can be found
# NETGEN_DEFINITIONS  - C++ compiler defines required to use netgen/nglib
#
# See also: http://git.salome-platform.org/gitweb/?p=NETGENPLUGIN_SRC.git;a=summary

SET(NETGEN_DEFINITIONS -DNO_PARALLEL_THREADS -DOCCGEOMETRY)

IF(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
    # We haven't supported Netgen prior to 5.3.1 on MacOS, and the current
    # plan is for the next Netgen version to be 6.1 (currently unreleased).
    IF(DEFINED HOMEBREW_PREFIX)
        EXEC_PROGRAM(brew ARGS --prefix nglib OUTPUT_VARIABLE NGLIB_PREFIX)
    ELSE(DEFINED HOMEBREW_PREFIX)
        SET(NGLIB_PREFIX ${MACPORTS_PREFIX})
    ENDIF(DEFINED HOMEBREW_PREFIX)

    FIND_PATH(NGLIB_INCLUDE_DIR nglib.h ${NGLIB_PREFIX}/include)

    FIND_LIBRARY(NGLIB_LIBNGLIB nglib ${NGLIB_PREFIX}/lib)
    FIND_LIBRARY(NGLIB_LIBMESH mesh ${NGLIB_PREFIX}/lib)
    FIND_LIBRARY(NGLIB_LIBOCC occ ${NGLIB_PREFIX}/lib)
    FIND_LIBRARY(NGLIB_LIBINTERFACE interface ${NGLIB_PREFIX}/lib)
    SET(NGLIB_LIBRARIES ${NGLIB_LIBNGLIB} ${NGLIB_LIBMESH}
                        ${NGLIB_LIBOCC} ${NGLIB_LIBINTERFACE})

    IF(NOT NETGENDATA)
        SET(NETGENDATA ${NGLIB_PREFIX}/include/netgen)
    ENDIF(NOT NETGENDATA)

ELSEIF(WIN32)
    FIND_PATH(NGLIB_INCLUDE_DIR NAMES nglib.h PATHS ${NETGEN_INCLUDEDIR})
    SET(NETGEN_LIBS nglib mesh occ interface)
    SET(NGLIB_LIBRARIES "")
    FOREACH(it ${NETGEN_LIBS})
        FIND_LIBRARY(NGLIB ${it} PATHS ${NETGEN_LIBDIR})
        FIND_LIBRARY(NGLIBD ${it}d PATHS ${NETGEN_LIBDIR})
        IF(NGLIBD AND NGLIB)
            SET(NG_LIB optimized ${NGLIB}
                       debug ${NGLIBD})
            SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NG_LIB})
            UNSET(NGLIB CACHE)
            UNSET(NGLIBD CACHE)
        ELSEIF(NGLIB)
            SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB})
            UNSET(NGLIB CACHE)
        ENDIF()
    ENDFOREACH()

    IF(NOT NETGENDATA)
        SET(NETGENDATA netgen)
    ENDIF(NOT NETGENDATA)

ELSE(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
    IF(NETGEN_ROOT)
        SET(NETGEN_INCLUDEDIR ${NETGEN_ROOT}/include)
        SET(NETGEN_LIBDIR ${NETGEN_ROOT}/lib)
        # allow to customize if NETGEN_ROOT is used
        IF(NOT NETGENDATA)
            SET(NETGENDATA ${NETGEN_ROOT}/libsrc)
        ENDIF(NOT NETGENDATA)
    ENDIF()

    FIND_PATH(NGLIB_INCLUDE_DIR NAMES nglib.h PATHS ${NETGEN_INCLUDEDIR} /usr/include)
    FIND_LIBRARY(NGLIB_LIBNGLIB nglib PATHS ${NETGEN_LIBDIR} /usr/lib /usr/local/lib)
    SET(NGLIB_LIBRARIES ${NGLIB_LIBNGLIB})
    FIND_LIBRARY(NGLIB_LIBMESH mesh PATHS ${NETGEN_LIBDIR} /usr/lib /usr/local/lib)
    IF(NGLIB_LIBMESH)
        SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBMESH})
    ENDIF()
    FIND_LIBRARY(NGLIB_LIBOCC occ PATHS ${NETGEN_LIBDIR} /usr/lib /usr/local/lib)
    IF(NGLIB_LIBOCC)
        SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBOCC})
    ENDIF()
    FIND_LIBRARY(NGLIB_LIBINTERFACE interface PATHS ${NETGEN_LIBDIR} /usr/lib /usr/local/lib)
    IF(NGLIB_LIBINTERFACE)
        SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBINTERFACE})
    ENDIF()

    IF(NOT NETGENDATA)
        SET(NETGENDATA /usr/share/netgen/libsrc)
    ENDIF(NOT NETGENDATA)

ENDIF(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)

FIND_PATH(NETGEN_DIR_include NAMES mydefs.hpp     PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/include)
FIND_PATH(NETGEN_DIR_csg     NAMES csg.hpp        PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/csg)
FIND_PATH(NETGEN_DIR_gen     NAMES array.hpp      PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/general)
FIND_PATH(NETGEN_DIR_geom2d  NAMES geom2dmesh.hpp PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/geom2d)
FIND_PATH(NETGEN_DIR_gprim   NAMES gprim.hpp      PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/gprim)
FIND_PATH(NETGEN_DIR_la      NAMES linalg.hpp     PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/linalg)
FIND_PATH(NETGEN_DIR_mesh    NAMES meshing.hpp    PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/meshing)
FIND_PATH(NETGEN_DIR_occ     NAMES occgeom.hpp    PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/occ)
FIND_PATH(NETGEN_DIR_stlgeom NAMES stlgeom.hpp    PATHS ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR} ${NETGENDATA}/stlgeom)

IF(NOT NGLIB_INCLUDE_DIR AND NOT NETGEN_DIR_include)
    MESSAGE(STATUS "Cannot find NETGEN header files.")
ELSE()
    file(STRINGS ${NETGEN_DIR_include}/mydefs.hpp NETGEN_VERSION
        REGEX "#define PACKAGE_VERSION.*"
    )
    string(REGEX MATCH "[0-9]+" NETGEN_VERSION ${NETGEN_VERSION})
#    EXECUTE_PROCESS(COMMAND grep -e ".*define.*PACKAGE_VERSION" ${NETGEN_DIR_include}/mydefs.hpp
#                    COMMAND sed -r "s:.*PACKAGE_VERSION \"([0-9]*).*:\\1:g"
#                    OUTPUT_VARIABLE NETGEN_VERSION
#                    OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()

IF(NOT NGLIB_LIBRARIES)
    MESSAGE(STATUS "Cannot find NETGEN library.")
ENDIF()

IF(NGLIB_INCLUDE_DIR AND NGLIB_LIBRARIES)
    SET(NETGEN_FOUND TRUE)
    SET(NETGEN_INCLUDE_DIRS ${NETGEN_DIR_include} ${NGLIB_INCLUDE_DIR}
      ${NETGEN_DIR_csg} ${NETGEN_DIR_gen} ${NETGEN_DIR_geom2d} 
      ${NETGEN_DIR_gprim} ${NETGEN_DIR_la} ${NETGEN_DIR_mesh}
      ${NETGEN_DIR_occ} ${NETGEN_DIR_stlgeom})
    LIST(REMOVE_DUPLICATES NETGEN_INCLUDE_DIRS)
    MESSAGE(STATUS "Found NETGEN version ${NETGEN_VERSION}")
    LIST(APPEND NETGEN_DEFINITIONS -DNETGEN_VERSION=${NETGEN_VERSION})
ELSE()
    SET(NETGEN_FOUND FALSE)
ENDIF()
