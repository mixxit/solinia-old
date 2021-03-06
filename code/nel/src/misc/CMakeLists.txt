FILE(GLOB SRC *.cpp *.h config_file/*.cpp config_file/*.h)

IF(NOT WIN32)
  ADD_LIBRARY(nelmisc SHARED ${SRC})
ELSE(NOT WIN32)
  ADD_LIBRARY(nelmisc STATIC ${SRC})
ENDIF(NOT WIN32)

IF(WITH_GTK)
  IF(GTK2_FOUND)
    INCLUDE_DIRECTORIES(${GTK2_INCLUDE_DIRS})
    ADD_DEFINITIONS(-DNL_USE_GTK)
    TARGET_LINK_LIBRARIES(nelmisc ${GTK2_LIBRARIES})
  ENDIF(GTK2_FOUND)
ENDIF(WITH_GTK)

IF(JPEG_FOUND)
  INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
  ADD_DEFINITIONS(-DUSE_JPEG)
  TARGET_LINK_LIBRARIES(nelmisc ${JPEG_LIBRARY})
ENDIF(JPEG_FOUND)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR} ${PNG_INCLUDE_DIR} config_file)
TARGET_LINK_LIBRARIES(nelmisc ${LIBXML2_LIBRARIES} ${PNG_LIBRARIES} ${WINSOCK2_LIB})
SET_TARGET_PROPERTIES(nelmisc PROPERTIES 
  VERSION ${NL_VERSION}
  PROJECT_LABEL "Library: NeL Misc")

IF(WIN32)
  FOREACH(FILENAME ${SRC})
    IF("${FILENAME}" MATCHES "stdmisc.cpp")
      SET_SOURCE_FILES_PROPERTIES(stdmisc.cpp PROPERTIES COMPILE_FLAGS "/Ycstdmisc.h")
    ELSEIF("${FILENAME}" MATCHES "config_file")
#      I don't really want to do anything here but you can uncomment this if you need to test.
#      MESSAGE("Note: Not adding pre-compiled headers to: ${FILENAME}.")
#
    ELSE("${FILENAME}" MATCHES "stdmisc.cpp")
      SET_SOURCE_FILES_PROPERTIES(${FILENAME} PROPERTIES COMPILE_FLAGS "/Yustdmisc.h")
    ENDIF("${FILENAME}" MATCHES "stdmisc.cpp")
  ENDFOREACH (FILENAME)

  SET_TARGET_PROPERTIES(nelmisc PROPERTIES 
    LINK_FLAGS "/NODEFAULTLIB:libcmt"
    DEBUG_POSTFIX "_d"
    RELEASE_POSTFIX "_r")
  INCLUDE_DIRECTORIES(${DXSDK_INCLUDE_DIR})
ENDIF(WIN32)

ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

INSTALL(TARGETS nelmisc LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)
