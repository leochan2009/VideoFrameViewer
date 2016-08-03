
#-----------------------------------------------------------------------------
set(MODULE_NAME PolyDataCompressedTransmission)
set(MODULE_TITLE ${MODULE_NAME})

string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
find_package(PCL 1.7 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})
add_subdirectory(Logic)
#-----------------------------------------------------------------------------
set(MODULE_EXPORT_DIRECTIVE "Q_SLICER_QTMODULES_${MODULE_NAME_UPPER}_EXPORT")
#add_directory(${CMAKE_CURRENT_SOURCE_DIR}/../ConnectAndDisplay/MRML)
# Current_{source,binary} and Slicer_{Libs,Base} already included
set(MODULE_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/../ConnectAndDisplay/MRML
  ${CMAKE_CURRENT_BINARY_DIR}/../ConnectAndDisplay/MRML
  ${CMAKE_CURRENT_SOURCE_DIR}/Widgets
  ${CMAKE_CURRENT_BINARY_DIR}/Widgets
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

set(MODULE_SRCS
  qSlicer${MODULE_NAME}Module.cxx
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.cxx
  qSlicer${MODULE_NAME}ModuleWidget.h
  #Tracking.cpp
  )

set(MODULE_MOC_SRCS
  qSlicer${MODULE_NAME}Module.h
  qSlicer${MODULE_NAME}ModuleWidget.h
  )

set(MODULE_UI_SRCS
  Resources/UI/qSlicer${MODULE_NAME}ModuleWidget.ui
  )
LINK_DIRECTORIES("/usr/local/lib")
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../ConnectAndDisplay/MRML)
set(MODULE_TARGET_LIBRARIES
  vtkSlicerConnectAndDisplayModuleMRML
  vtkSlicer${MODULE_NAME}ModuleLogic
  ${PCL_LIBRARIES}
  )

set(MODULE_RESOURCES
  Resources/qSlicer${MODULE_NAME}Module.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildQtModule(
  NAME ${MODULE_NAME}
  TITLE ${MODULE_TITLE}
  EXPORT_DIRECTIVE ${MODULE_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  SRCS ${MODULE_SRCS}
  MOC_SRCS ${MODULE_MOC_SRCS}
  UI_SRCS ${MODULE_UI_SRCS}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  RESOURCES ${MODULE_RESOURCES}
  WITH_GENERIC_TESTS
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
