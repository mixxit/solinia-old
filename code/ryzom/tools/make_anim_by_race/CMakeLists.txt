FILE(GLOB SRC *.cpp *.h)

ADD_EXECUTABLE(make_anim_by_race ${SRC})

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR} ${NEL_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(make_anim_by_race ${PLATFORM_LINKFLAGS} ${LIBXML2_LIBRARIES} ${NELMISC_LIBRARY})
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS make_anim_by_race RUNTIME DESTINATION bin COMPONENT tools)
