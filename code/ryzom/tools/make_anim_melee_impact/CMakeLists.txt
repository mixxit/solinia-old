FILE(GLOB SRC *.cpp *.h)

ADD_EXECUTABLE(make_anim_melee_impact ${SRC})

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR} ${NEL_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(make_anim_melee_impact ${PLATFORM_LINKFLAGS} ${LIBXML2_LIBRARIES} ${NELMISC_LIBRARY})
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS make_anim_melee_impact RUNTIME DESTINATION bin COMPONENT tools)
