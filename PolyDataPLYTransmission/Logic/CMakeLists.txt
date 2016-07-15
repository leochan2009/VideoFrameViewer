project(vtkSlicer${MODULE_NAME}ModuleLogic)

set(KIT ${PROJECT_NAME})
message(“output”)
message(${MODULE_NAME})
set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_LOGIC_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../ConnectAndDisplay/MRML
  ${CMAKE_CURRENT_BINARY_DIR}/../../ConnectAndDisplay/MRML
  )

set(${KIT}_SRCS
  vtkSlicer${MODULE_NAME}Logic.cxx
  vtkSlicer${MODULE_NAME}Logic.h
  )
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/../ConnectAndDisplay/MRML)
LINK_DIRECTORIES("/usr/local/lib") #MRML is linked with iomp5 library, here it is only to find the iomp library for MRML
set(${KIT}_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  vtkSlicerConnectAndDisplayModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleLogic(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
