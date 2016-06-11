/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLPolyData.h $
  Date:      $Date: 2010-11-23 00:58:13 -0500 (Tue, 23 Nov 2010) $
  Version:   $Revision: 15552 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLPolyData_h
#define __vtkIGTLToMRMLPolyData_h

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

class VTK_SLICER_CONNECTANDDISPLAY_MODULE_MRML_EXPORT vtkIGTLToMRMLPolyData : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLPolyData *New();
  vtkTypeMacro(vtkIGTLToMRMLPolyData,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual const char*  GetIGTLName() { return IGTLName; };
  virtual void SetIGTLName(char* name) { IGTLName = name; };
  virtual const char*  GetMRMLName() { return "Frame"; };
  //virtual vtkIntArray* GetNodeEvents();
  virtual vtkMRMLNode* CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name, igtl::MessageBase::Pointer vtkNotUsed(message));
  virtual uint8_t * IGTLToMRML(igtl::MessageBase::Pointer buffer);
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg);
  
  virtual void setInterval(int timeInterval) {interval = timeInterval;};
  virtual void setUseCompress(bool useCompress){_useCompress = useCompress;};

 protected:
  vtkIGTLToMRMLPolyData();
  ~vtkIGTLToMRMLPolyData();

  int IGTLToVTKScalarType(int igtlType);
  int SetupDecoder();
  void H264Decode (ISVCDecoder* pDecoder, unsigned char* kpH264BitStream, int32_t& iWidth, int32_t& iHeight, int32_t& iStreamSize, uint8_t* outputByteStream);
  uint8_t *YUVFrame;
 protected:
  igtl::VideoMessage::Pointer OutVideoMsg;
  ISVCDecoder* decoder_;
  char*  IGTLName;
  int interval;
  bool _useCompress;
  igtl::StartVideoDataMessage::Pointer StartVideoMsg;
  igtl::StopVideoMessage::Pointer StopVideoMsg;
  uint8_t* pData[3];

};


#endif //__vtkIGTLToMRMLPolyData_h
