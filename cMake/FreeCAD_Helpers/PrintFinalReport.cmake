macro(PrintFinalReport)
    # -------------------------------- The final report ----------------------------------

    message(STATUS "\n==============\n"
                   "Summary report\n"
                   "==============\n")
    if (DEFINED CMAKE_BUILD_TYPE)
        message(STATUS "Build type:          ${CMAKE_BUILD_TYPE}")
    endif()

    # Qt5 needs/sets PYTHON_CONFIG_SUFFIX regarding Shiboken
    message(STATUS "Python:              [${PYTHON_EXECUTABLE}] [${PYTHON_CONFIG_SUFFIX}]")

    if(DEFINED PCL_FOUND)
        message(STATUS "PCL:                 ${PCL_VERSION}")
    else(DEFINED PCL_FOUND)
        message(STATUS "PCL:                 not enabled")
    endif(DEFINED PCL_FOUND)

    if(DEFINED pybind11_FOUND)
        message(STATUS "pybind11:            ${pybind11_VERSION}")
    else(DEFINED pybind11_FOUND)
        message(STATUS "pybind11:            not enabled")
    endif(DEFINED pybind11_FOUND)

    message(STATUS "Boost:               ${Boost_VERSION}")

    message(STATUS "XercesC:             [${XercesC_LIBRARIES}] [${XercesC_INCLUDE_DIRS}]")

    message(STATUS "ZLIB:                ${ZLIB_VERSION_STRING}")

    message(STATUS "PyCXX:               [${PYCXX_INCLUDE_DIR}]")

    message(STATUS "OCC:                 ${OCC_VERSION_STRING} [${OCC_LIBRARIES}]")

    if(BUILD_SMESH)
        if(FREECAD_USE_EXTERNAL_SMESH)
            message(STATUS "SMESH:               ${SMESH_VERSION_MAJOR}.${SMESH_VERSION_MINOR}.${SMESH_VERSION_PATCH}.${SMESH_VERSION_TWEAK}")
        else(FREECAD_USE_EXTERNAL_SMESH)
            message(STATUS "SMESH:               build internal")
            message(STATUS " MEDFile:            [${MEDFILE_LIBRARIES}] [${MEDFILE_INCLUDE_DIRS}]")
            message(STATUS " HDF5:               ${HDF5_VERSION}")
            message(STATUS " VTK:                ${VTK_VERSION}")
        endif(FREECAD_USE_EXTERNAL_SMESH)
    else(BUILD_SMESH)
        message(STATUS "SMESH:               do not build")
    endif(BUILD_SMESH)

    if(DEFINED NETGEN_FOUND)
        message(STATUS "NETGEN:              ${NETGEN_VERSION} [${NETGEN_DEFINITIONS}] [${NETGEN_CXX_FLAGS}] [${NGLIB_INCLUDE_DIR}] [${NGLIB_LIBRARIES}] [${NETGEN_INCLUDE_DIRS}]")
    else(DEFINED NETGEN_FOUND)
        message(STATUS "NETGEN:              not enabled")
    endif(DEFINED NETGEN_FOUND)

    #message(STATUS "OpenCV:              ${OpenCV_VERSION}")

    if(DEFINED SWIG_FOUND)
        message(STATUS "SWIG:                ${SWIG_VERSION}")
    else(DEFINED SWIG_FOUND)
        message(STATUS "SWIG:                not found")
    endif(DEFINED SWIG_FOUND)

    if(DEFINED EIGEN3_FOUND)
        message(STATUS "Eigen3               ${EIGEN3_VERSION}")
    else(DEFINED EIGEN3_FOUND)
        message(STATUS "Eigen3:              not found")
    endif(DEFINED EIGEN3_FOUND)

    if(NOT BUILD_QT5)
        message(STATUS "Qt4:                 ${Qt4_VERSION}")
        if(QT_QTWEBKIT_FOUND)
            message(STATUS "QtWebKit:            found")
        else(QT_QTWEBKIT_FOUND)
            message(STATUS "QtWebKit:            not found")
        endif(QT_QTWEBKIT_FOUND)
         message(STATUS "Shiboken:            ${Shiboken_VERSION} [${SHIBOKEN_INCLUDE_DIR}]")
        message(STATUS "PySide:              ${PySide_VERSION} [${PYSIDE_INCLUDE_DIR}]")
        message(STATUS "PySideTools:         [${PYSIDEUIC4BINARY}] [${PYSIDERCC4BINARY}]")
    else(NOT BUILD_QT5)
        message(STATUS "Qt5Core:             ${Qt5Core_VERSION}")
        message(STATUS "Qt5Network:          ${Qt5Network_VERSION}")
        message(STATUS "Qt5Xml:              ${Qt5Xml_VERSION}")
        message(STATUS "Qt5XmlPatterns:      ${Qt5XmlPatterns_VERSION}")
        if (BUILD_GUI)
            message(STATUS "Qt5Widgets:          ${Qt5Widgets_VERSION}")
            message(STATUS "Qt5PrintSupport:     ${Qt5PrintSupport_VERSION}")
            message(STATUS "Qt5OpenGL:           ${Qt5OpenGL_VERSION}")
            message(STATUS "Qt5Svg:              ${Qt5Svg_VERSION}")
            message(STATUS "Qt5UiTools:          ${Qt5UiTools_VERSION}")
            message(STATUS "Qt5Concurrent:       ${Qt5Concurrent_VERSION}")
            if(BUILD_WEB)
                if (Qt5WebKitWidgets_FOUND)
                    message(STATUS "Qt5WebKitWidgets:    ${Qt5WebKitWidgets_VERSION}")
                endif()
                if (Qt5WebEngineWidgets_FOUND)
                    message(STATUS "Qt5WebEngineWidgets: ${Qt5WebEngineWidgets_VERSION}")
                endif()
            else(BUILD_WEB)
                message(STATUS "Qt5WebKitWidgets:    not needed (BUILD_WEB)")
                message(STATUS "Qt5WebEngineWidgets:    not needed (BUILD_WEB)")
            endif(BUILD_WEB)
            if(${Qt5WinExtras_FOUND})
                message(STATUS "Qt5WinExtras:        ${Qt5WinExtras_VERSION}")
            endif()

        else(BUILD_GUI)
            message(STATUS "Qt5Widgets:          not needed")
            message(STATUS "Qt5PrintSupport:     not needed")
            message(STATUS "Qt5OpenGL:           not needed")
            message(STATUS "Qt5Svg:              not needed")
            message(STATUS "Qt5UiTools:          not needed")
            message(STATUS "Qt5Concurrent:       not needed")
            message(STATUS "Qt5WebKitWidgets:    not needed")
        endif(BUILD_GUI)

        if(DEFINED MACPORTS_PREFIX)
            if(DEFINED Shiboken_FOUND)
                message(STATUS "Shiboken:            ${Shiboken_VERSION} [${SHIBOKEN_INCLUDE_DIR}]")
            else(DEFINED Shiboken_FOUND)
                message(STATUS "Shiboken:            not found (only searched if MACPORTS_PREFIX is defined)")
            endif(DEFINED Shiboken_FOUND)
            if(DEFINED PySide_FOUND)
                message(STATUS "PySide:              ${PySide_VERSION} [${PYSIDE_INCLUDE_DIR}]")
                if(NOT PYSIDE_INCLUDE_DIR)
                    message(STATUS " IncludeDir:         Unable to find, python version mismatch?")
                endif(NOT PYSIDE_INCLUDE_DIR)
            else(DEFINED PySide_FOUND)
                message(STATUS "PySide:              not found (only searched if MACPORTS_PREFIX is defined)")
            endif(DEFINED PySide_FOUND)
        endif(DEFINED MACPORTS_PREFIX)

        if(DEFINED Shiboken2_FOUND)
            message(STATUS "Shiboken2:           ${Shiboken2_VERSION} [${Shiboken2_DIR}] [${SHIBOKEN_INCLUDE_DIR}]")
        else(DEFINED Shiboken2_FOUND)
            message(STATUS "Shiboken2:           not found")
        endif(DEFINED Shiboken2_FOUND)
        if(DEFINED PySide2_FOUND)
            message(STATUS "PySide2:             ${PySide2_VERSION} [${PYSIDE_INCLUDE_DIR}]")
            if(NOT PYSIDE_INCLUDE_DIR)
                message(STATUS " IncludeDir:         Unable to find, python version mismatch?")
            endif(NOT PYSIDE_INCLUDE_DIR)
        else(DEFINED PySide2_FOUND)
            message(STATUS "PySide2:             not found")
        endif(DEFINED PySide2_FOUND)
        if(DEFINED PYSIDE2_TOOLS_FOUND)
            message(STATUS "PySide2Tools:        [${PYSIDE2UICBINARY}] [${PYSIDE2RCCBINARY}]")
        else(DEFINED PYSIDE2_TOOLS_FOUND)
            message(STATUS "PySide2Tools:        not found")
        endif(DEFINED PYSIDE2_TOOLS_FOUND)
    endif(NOT BUILD_QT5)

    if(FREECAD_USE_FREETYPE)
        if(DEFINED FREETYPE_FOUND)
            message(STATUS "Freetype:            ${FREETYPE_VERSION_STRING}")
        else(DEFINED FREETYPE_FOUND)
            message(STATUS "Freetype:            not found")
        endif(DEFINED FREETYPE_FOUND)
    else(FREECAD_USE_FREETYPE)
        message(STATUS "Freetype:            disabled")
    endif(FREECAD_USE_FREETYPE)

    message(STATUS "OpenGLU:             ${OPENGL_glu_LIBRARY} [${OPENGL_glu_LIBRARY}][${OPENGL_INCLUDE_DIR}]")

    message(STATUS "Coin3D:              [${COIN3D_LIBRARIES}] [${COIN3D_INCLUDE_DIRS}]")


    if (WIN32)
    #message(STATUS "SPNAV:               not available yet for your OS") # FREECAD_USE_3DCONNEXION instead...
    else(WIN32)
        if(DEFINED SPNAV_FOUND)
            message(STATUS "SPNAV:               [${SPNAV_LIBRARY}] [${SPNAV_INCLUDE_DIR}]")
        else(DEFINED SPNAV_FOUND)
            message(STATUS "SPNAV:               not found")
        endif(DEFINED SPNAV_FOUND)
    endif(WIN32)

    if(MATPLOTLIB_FOUND)
        message(STATUS "Matplotlib:          ${MATPLOTLIB_VERSION}")
    else(MATPLOTLIB_FOUND)
        message(STATUS "Matplotlib:          not found")
    endif(MATPLOTLIB_FOUND)

    if(BUILD_VR)
        if(DEFINED RIFT_FOUND)
            message(STATUS "Rift:                ${Rift_VERSION}")
        else(DEFINED RIFT_FOUND)
            message(STATUS "Rift:                not found")
        endif(DEFINED RIFT_FOUND)
    else(BUILD_VR)
        message(STATUS "Rift:                not enabled (BUILD_VR)")
    endif(BUILD_VR)

    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen:             ${DOXYGEN_VERSION}")
        message(STATUS " Language:           ${DOXYGEN_LANGUAGE}")
        if(COIN3D_DOC_FOUND)
            message(STATUS " Coin3D_DOC:         found [${COIN3D_DOC_PATH}]")
        else(COIN3D_DOC_FOUND)
            message(STATUS " Coin3D_DOC:         not found")
        endif(COIN3D_DOC_FOUND)
    else(DOXYGEN_FOUND)
        message(STATUS "Doxygen:             not found")
    endif(DOXYGEN_FOUND)

    if(MSVC)
        # Copy libpack dependency directories to build folder for user as part of overall build process
        if(FREECAD_LIBPACK_USE AND LIBPACK_FOUND)
            if(FREECAD_COPY_DEPEND_DIRS_TO_BUILD)
                message(STATUS "=======================================\n"
                    "Copying libpack dependency directories to build directory for Windows MSVC build.\n")
                file(COPY ${FREECAD_LIBPACK_DIR}/bin/assistant.exe DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/bin/QtWebEngineProcess.exe DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/bin/QtWebEngineProcessd.exe DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/bin/qt.conf DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/plugins/platforms DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/plugins/imageformats DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/plugins/iconengines DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/plugins/sqldrivers DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/plugins/styles DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/plugins/printsupport DESTINATION ${CMAKE_BINARY_DIR}/bin)
                file(COPY ${FREECAD_LIBPACK_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
                file(COPY ${FREECAD_LIBPACK_DIR}/bin/Lib/site-packages/PySide2/translations/qtwebengine_locales
                    DESTINATION ${CMAKE_BINARY_DIR}/bin)
                message(STATUS "... end copying.\n=======================================\n")
            endif(FREECAD_COPY_DEPEND_DIRS_TO_BUILD)

            if(COPY_LIBPACK_BIN_TO_BUILD)
                if(FREECAD_COPY_LIBPACK_BIN_TO_BUILD)
                    message("=======================================\n"
                            "Copying libpack 'bin' directory to build directory.\n")
                    file(COPY ${FREECAD_LIBPACK_DIR}/bin DESTINATION ${CMAKE_BINARY_DIR})
                    message("... done copying libpack 'bin' directory.\n=======================================\n")
                endif()
            endif()

            if(FREECAD_INSTALL_DEPEND_DIRS)
                # Test install command for installing/copying directories
                message(STATUS "=======================================")
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/plugins/platforms DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/plugins/imageformats DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/plugins/iconengines DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/plugins/sqldrivers DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/plugins/styles DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/plugins/printsupport DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/bin/Lib/site-packages/PySide2/translations/qtwebengine_locales DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/resources DESTINATION ${CMAKE_INSTALL_PREFIX})
                install(DIRECTORY ${FREECAD_LIBPACK_DIR}/bin DESTINATION ${CMAKE_INSTALL_PREFIX})
                message(STATUS "Created install commands for INSTALL target.\n")
            endif(FREECAD_INSTALL_DEPEND_DIRS)
        endif(FREECAD_LIBPACK_USE AND LIBPACK_FOUND)
    endif()

    # Print message to start build process
    message("=================================================\n"
            "Now run 'cmake --build ${CMAKE_BINARY_DIR}' to build ${PROJECT_NAME}\n"
            "=================================================\n")
endmacro(PrintFinalReport)
