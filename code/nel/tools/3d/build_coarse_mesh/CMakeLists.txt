FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(build_coarse_mesh ${SRC})

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(build_coarse_mesh ${LIBXML2_LIBRARIES} ${PLATFORM_LINKFLAGS} nel3d)
IF(WIN32)
  SET_TARGET_PROPERTIES(build_coarse_mesh PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:libcmt"
    PROJECT_LABEL "Tools, 3D: build_coarse_mesh")
ENDIF(WIN32)
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS build_coarse_mesh RUNTIME DESTINATION bin COMPONENT tools3d)
