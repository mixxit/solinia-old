FILE(GLOB SRC *.cpp *.h ../s3tc_compressor_lib/*.h ../s3tc_compressor_lib/*.cpp)

ADD_EXECUTABLE(panoply_maker ${SRC})

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR} ${S3TC_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(panoply_maker ${LIBXML2_LIBRARIES} ${PLATFORM_LINKFLAGS} ${S3TC_LIBRARY} nelmisc nel3d)
IF(WIN32)
  SET_TARGET_PROPERTIES(panoply_maker PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:libcmt /NODEFAULTLIB:libc"
    PROJECT_LABEL "Tools, 3D: panoply_maker")
ENDIF(WIN32)
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS panoply_maker RUNTIME DESTINATION bin COMPONENT tools3d)
