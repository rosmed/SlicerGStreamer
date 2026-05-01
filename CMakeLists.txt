
cmake_minimum_required(VERSION 3.5)
project(slicer_gstreamer_module)

#-----------------------------------------------------------------------------
# Extension GStreamer dependencies
#-----------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

#-----------------------------------------------------------------------------
# Slicer
#-----------------------------------------------------------------------------
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# Module
#-----------------------------------------------------------------------------
set(MODULE_NAME GStreamer)
set(MODULE_TITLE "GStreamer")
string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

set(SCRIPTS_DIR "scripts")

include_directories(
  ${Slicer_Base_INCLUDE_DIRS}
  ${Slicer_Libs_INCLUDE_DIRS}
  ${slicer_gstreamer_module_SOURCE_DIR}
  ${slicer_gstreamer_module_BINARY_DIR}
  ${slicer_gstreamer_module_SOURCE_DIR}/Logic
  ${slicer_gstreamer_module_BINARY_DIR}/Logic
  ${slicer_gstreamer_module_SOURCE_DIR}/MRML
  ${slicer_gstreamer_module_BINARY_DIR}/MRML
  ${slicer_gstreamer_module_SOURCE_DIR}/Resources/UI
  ${GSTREAMER_INCLUDE_DIRS}
  ${GSTREAMER_BASE_INCLUDE_DIRS}
  ${GSTREAMER_VIDEO_INCLUDE_DIRS}
  ${GSTREAMER_APP_INCLUDE_DIRS}
)

#-----------------------------------------------------------------------------
add_subdirectory(MRML)
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")

set(MODULE_INCLUDE_DIRECTORIES
  ${Slicer_Base_INCLUDE_DIRS}
  ${Slicer_Libs_INCLUDE_DIRS}
  ${slicer_gstreamer_module_SOURCE_DIR}/Logic
  ${slicer_gstreamer_module_BINARY_DIR}/Logic
  ${slicer_gstreamer_module_SOURCE_DIR}/MRML
  ${slicer_gstreamer_module_BINARY_DIR}/MRML
)

set(MODULE_SRCS
  qSlicer${MODULE_NAME}Module.cxx
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.cxx
  qSlicer${MODULE_NAME}ModuleWidget.h
)

set(MODULE_UI
  qSlicer${MODULE_NAME}ModuleWidget.ui
)

set(MODULE_TARGET_LIBRARIES
  vtkSlicerGStreamerModuleLogic
  qSlicerBaseQTGUI
  qMRMLWidgets
  ${GSTREAMER_LIBRARIES}
  ${GSTREAMER_BASE_LIBRARIES}
  ${GSTREAMER_VIDEO_LIBRARIES}
  ${GSTREAMER_APP_LIBRARIES}
)

set(MODULE_RESOURCES
  Resources/qSlicer${MODULE_NAME}Module.qrc
)

SlicerMacroBuildModuleQtLibrary(
  NAME qSlicer${MODULE_NAME}Module
  EXPORT_DIRECTIVE ${MODULE_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  SRCS ${MODULE_SRCS} ${MODULE_UI}
  RESOURCES ${MODULE_RESOURCES}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
)

#-----------------------------------------------------------------------------
# Slicer Extension
#-----------------------------------------------------------------------------
# SlicerMacroBuildExtension(
#   NAME ${PROJECT_NAME}
#   DESCRIPTION "Streaming images to/from GStreamer"
#   CONTRIBUTORS "GitHub Copilot (Gemini 3 Flash (Preview))"
#   CATEGORY "GStreamer"
# )
