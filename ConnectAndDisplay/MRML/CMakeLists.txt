project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT "${PROJECT_NAME}")

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_CONNECTANDDISPLAY_MODULE_MRML_EXPORT")
FIND_LIBRARY(OpenIGTLinkLib OpenIGTLink /Users/longquanchen/Desktop/Github/OpenIGTLink-xcodeBuild/bin/Debug)
FIND_LIBRARY(OpenH264Lib openh264 /Users/longquanchen/Desktop/Github/OpenIGTLink-xcodeBuild/Testing/OpenH264)
set(H264Directory /Users/longquanchen/Desktop/Github/OpenIGTLink-xcodeBuild/Testing/OpenH264)
set(${KIT}_INCLUDE_DIRECTORIES
  ${Slicer_Base_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../Logic
  ${CMAKE_CURRENT_BINARY_DIR}/../Logic
  ${H264Directory}
  )

set(${KIT}_SRCS
  vtkIGTLCircularBuffer.cxx
  vtkIGTLToMRMLBase.cxx
  vtkMRMLIGTLConnectorNode.cxx
  vtkMRMLIGTLStatusNode.cxx
  yuvfile.cpp
  yuvsource.cpp
  )

if(OpenIGTLink_PROTOCOL_VERSION GREATER 1)
  list(APPEND ${KIT}_SRCS
    vtkMRMLIGTLQueryNode.cxx
    vtkIGTLToMRMLVideo.cxx
    )
endif()
message(${OpenH264Lib})
set(${KIT}_TARGET_LIBRARIES
  ${OpenIGTLinkLib}
  ${MRML_LIBRARIES}
  SlicerBaseLogic
  ${OpenH264Lib}
  vtkSlicerAnnotationsModuleMRML
  vtkSlicerMarkupsModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
