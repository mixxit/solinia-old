FILE(GLOB SRC *.cpp *.h)

ADD_EXECUTABLE(build_clod_bank ${SRC})

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(build_clod_bank ${LIBXML2_LIBRARIES} ${PLATFORM_LINKFLAGS} nel3d)
IF(WIN32)
  SET_TARGET_PROPERTIES(build_clod_bank PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:libcmt"
    PROJECT_LABEL "Tools, 3D: build_clod_bank")
ENDIF(WIN32)
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS build_clod_bank RUNTIME DESTINATION bin COMPONENT tools3d)
