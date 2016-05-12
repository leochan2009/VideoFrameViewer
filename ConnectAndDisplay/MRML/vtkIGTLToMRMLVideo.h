/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLVideo.h $
  Date:      $Date: 2010-11-23 00:58:13 -0500 (Tue, 23 Nov 2010) $
  Version:   $Revision: 15552 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLVideo_h
#define __vtkIGTLToMRMLVideo_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerConnectAndDisplayModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlVideoMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class ISVCDecoder;

class VTK_SLICER_CONNECTANDDISPLAY_MODULE_MRML_EXPORT vtkIGTLToMRMLVideo : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLVideo *New();
  vtkTypeMacro(vtkIGTLToMRMLVideo,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual const char*  GetIGTLName() { return IGTLName; };
  virtual void SetIGTLName(char* name) { IGTLName = name; };
  virtual const char*  GetMRMLName() { return "Frame"; };
  //virtual vtkIntArray* GetNodeEvents();
  virtual vtkMRMLNode* CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name, igtl::MessageBase::Pointer vtkNotUsed(message));
  virtual uint8_t * IGTLToMRML(igtl::MessageBase::Pointer buffer);
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg);

  int YUV420ToRGBConversion(uint8_t *RGBFrame, uint8_t * YUV420Frame, int iHeight, int iWidth);
  
  virtual void setInterval(int timeInterval) {interval = timeInterval;};

 protected:
  vtkIGTLToMRMLVideo();
  ~vtkIGTLToMRMLVideo();

  int IGTLToVTKScalarType(int igtlType);
  int SetupDecoder();
  uint8_t *RGBFrame;
 protected:
  igtl::VideoMessage::Pointer OutVideoMsg;
  ISVCDecoder* decoder_;
  char*  IGTLName;
  int interval;
  igtl::StartVideoDataMessage::Pointer StartVideoMsg;
  igtl::StopVideoMessage::Pointer StopVideoMsg;

};


#endif //__vtkIGTLToMRMLVideo_h
