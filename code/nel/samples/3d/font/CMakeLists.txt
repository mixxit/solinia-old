FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(nl_sample_font ${SRC})

ADD_DEFINITIONS(-DFONT_DIR="\\"${NL_SHARE_PREFIX}/nl_sample_font/\\"")

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(nl_sample_font ${LIBXML2_LIBRARIES} ${PLATFORM_LINKFLAGS} nelmisc nel3d)
IF(WIN32)
  SET_TARGET_PROPERTIES(nl_sample_font PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:libcmt /SUBSYSTEM:WINDOWS"
    PROJECT_LABEL "Samples, 3D: Font")
ENDIF(WIN32)
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS nl_sample_font RUNTIME DESTINATION bin COMPONENT samples3d)
INSTALL(FILES beteckna.ttf DESTINATION share/nel/nl_sample_font COMPONENT samples3d)
