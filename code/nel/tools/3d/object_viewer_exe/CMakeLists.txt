FILE(GLOB SRC *.cpp *.h)

ENABLE_LANGUAGE(RC)
ADD_DEFINITIONS(-D_AFXDLL)
SET(CMAKE_MFC_FLAG 2)
ADD_EXECUTABLE(object_viewer WIN32 ${SRC} object_viewer_exe.rc)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(	object_viewer 
						${LIBXML2_LIBRARIES}
						${PLATFORM_LINKFLAGS} 
						nelmisc 
						nel3d
						nelsound
						nelsnd_lowlevel
						object_viewer_dll)
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

SET_TARGET_PROPERTIES(object_viewer PROPERTIES COMPILE_FLAGS "/Yustd_afx.h")
SET_SOURCE_FILES_PROPERTIES(std_afx.cpp PROPERTIES COMPILE_FLAGS "/Ycstd_afx.h")
SET_TARGET_PROPERTIES(object_viewer PROPERTIES 
  LINK_FLAGS "/NODEFAULTLIB:libcmt"
  PROJECT_LABEL "Tools, 3D: Object Viewer")

INSTALL(TARGETS object_viewer RUNTIME DESTINATION bin COMPONENT tools3d)
IF(WITH_MAXPLUGIN)
  INSTALL(TARGETS object_viewer RUNTIME DESTINATION maxplugin COMPONENT tools3d)
ENDIF(WITH_MAXPLUGIN)