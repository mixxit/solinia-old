FILE(GLOB SRC *.cpp *.h)

ADD_EXECUTABLE(make_alias_file ${SRC})

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR} ${NEL_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(make_alias_file ${PLATFORM_LINKFLAGS} ${LIBXML2_LIBRARIES} ${NELMISC_LIBRARY} ${NELGEORGES_LIBRARY})
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS make_alias_file RUNTIME DESTINATION bin COMPONENT tools)
