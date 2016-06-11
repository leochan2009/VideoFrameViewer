/*==========================================================================
 
 Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See Doc/copyright/copyright.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 Program:   3D Slicer
 Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLPolyData.cxx $
 Date:      $Date: 2010-12-07 21:39:19 -0500 (Tue, 07 Dec 2010) $
 Version:   $Revision: 15621 $
 
 ==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLPolyData.h"
#include "vtkMRMLIGTLQueryNode.h"
#include <sys/time.h>
#include <time.h>
#include "codec/api/svc/codec_api.h"
#include "codec/api/svc/codec_app_def.h"
#include "test/utils/BufferedData.h"
#include "test/utils/FileInputStream.h"

// OpenIGTLink includes
#include <igtl_util.h>
#include <igtlVideoMessage.h>


// Slicer includes
//#include <vtkSlicerColorLogic.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#include <omp.h>

#define NO_DELAY_DECODING

int64_t getTimePoly()
{
  struct timeval tv_date;
  gettimeofday(&tv_date, NULL);
  return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);
  
}


void ComposeByteSteamPoly(uint8_t** inputData, SBufferInfo bufInfo, uint8_t *outputByteStream)
{
  int iHeight = bufInfo.UsrData.sSystemBuffer.iHeight ;
  int iWidth = bufInfo.UsrData.sSystemBuffer.iWidth ;
  int iStride [2] = {bufInfo.UsrData.sSystemBuffer.iStride[0],bufInfo.UsrData.sSystemBuffer.iStride[1]};
#pragma omp parallel for default(none) shared(outputByteStream,inputData, iStride, iHeight, iWidth)
  for (int i = 0; i < iHeight; i++)
  {
    uint8_t* pPtr = inputData[0]+i*iStride[0];
    for (int j = 0; j < iWidth; j++)
    {
      outputByteStream[i*iWidth + j] = pPtr[j];
    }
  }
#pragma omp parallel for default(none) shared(outputByteStream,inputData, iStride, iHeight, iWidth)
  for (int i = 0; i < iHeight/2; i++)
  {
    uint8_t* pPtr = inputData[1]+i*iStride[1];
    for (int j = 0; j < iWidth/2; j++)
    {
      outputByteStream[i*iWidth/2 + j + iHeight*iWidth] = pPtr[j];
    }
  }
#pragma omp parallel for default(none) shared(outputByteStream, inputData, iStride, iHeight, iWidth)
  for (int i = 0; i < iHeight/2; i++)
  {
    uint8_t* pPtr = inputData[2]+i*iStride[1];
    for (int j = 0; j < iWidth/2; j++)
    {
      outputByteStream[i*iWidth/2 + j + iHeight*iWidth*5/4] = pPtr[j];
    }
  }
  
}

void vtkIGTLToMRMLPolyData::H264Decode (ISVCDecoder* pDecoder, unsigned char* kpH264BitStream, int32_t& iWidth, int32_t& iHeight, int32_t& iStreamSize, uint8_t* outputByteStream) {
  
  
  unsigned long long uiTimeStamp = 0;
  int64_t iStart = 0, iEnd = 0, iTotal = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  uint8_t* pBuf = NULL;
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  
  iStart = getTimePoly();
  pData[0] = NULL;
  pData[1] = NULL;
  pData[2] = NULL;
  
  SBufferInfo sDstBufInfo;
  
  int32_t iBufPos = 0;
  int32_t i = 0;
  int32_t iFrameCount = 0;
  int32_t iEndOfStreamFlag = 0;
  //for coverage test purpose
  int32_t iErrorConMethod = (int32_t) ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
  pDecoder->SetOption (DECODER_OPTION_ERROR_CON_IDC, &iErrorConMethod);
  //~end for
  double dElapsed = 0;
  
  if (pDecoder == NULL) return;
  
  if (iStreamSize <= 0) {
    fprintf (stderr, "Current Bit Stream File is too small, read error!!!!\n");
    goto label_exit;
  }
  pBuf = new uint8_t[iStreamSize + 4];
  if (pBuf == NULL) {
    fprintf (stderr, "new buffer failed!\n");
    goto label_exit;
  }
  memcpy (pBuf, kpH264BitStream, iStreamSize);
  memcpy (pBuf + iStreamSize, &uiStartCode[0], 4); //confirmed_safe_unsafe_usage
  
  while (true) {
    if (iBufPos >= iStreamSize) {
      iEndOfStreamFlag = true;
      if (iEndOfStreamFlag)
        pDecoder->SetOption (DECODER_OPTION_END_OF_STREAM, (void*)&iEndOfStreamFlag);
      break;
    }
    for (i = 0; i < iStreamSize; i++) {
      if ((pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 && pBuf[iBufPos + i + 3] == 1
           && i > 0) || (pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 1 && i > 0)) {
        break;
      }
    }
    iSliceSize = i;
    if (iSliceSize < 4) { //too small size, no effective data, ignore
      iBufPos += iSliceSize;
      continue;
    }
    
    iStart = getTimePoly();
    delete [] pData[0];
    pData[0] = NULL;
    delete [] pData[1];
    pData[1] = NULL;
    delete [] pData[2];
    pData[2] = NULL;
    uiTimeStamp ++;
    memset (&sDstBufInfo, 0, sizeof (SBufferInfo));
    sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;
#ifndef NO_DELAY_DECODING
    pDecoder->DecodeFrameNoDelay (pBuf + iBufPos, iSliceSize, pData, &sDstBufInfo);
#else
    pDecoder->DecodeFrame2 (pBuf + iBufPos, iSliceSize, pData, &sDstBufInfo);
#endif
    
#ifdef NO_DELAY_DECODING
    
    pData[0] = NULL;
    pData[1] = NULL;
    pData[2] = NULL;
    memset (&sDstBufInfo, 0, sizeof (SBufferInfo));
    sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;
    pDecoder->DecodeFrame2 (NULL, 0, pData, &sDstBufInfo);
    fprintf (stderr, "iStreamSize:\t%d \t slice size:\t%d\n", iStreamSize, iSliceSize);
    iEnd  = getTimePoly();
    iTotal = iEnd - iStart;
    if (sDstBufInfo.iBufferStatus == 1) {
      int64_t iStart2 = getTimePoly();
      ComposeByteSteamPoly(pData, sDstBufInfo, outputByteStream);
      
      fprintf (stderr, "compose time:\t%f\n", (getTimePoly()-iStart2)/1e6);
      ++ iFrameCount;
    }
#endif
    if (iFrameCount)
    {
      dElapsed = iTotal / 1e6;
      fprintf (stderr, "-------------------------------------------------------\n");
      fprintf (stderr, "iWidth:\t\t%d\nheight:\t\t%d\nFrames:\t\t%d\ndecode time:\t%f sec\nFPS:\t\t%f fps\n",
               iWidth, iHeight, iFrameCount, dElapsed, (iFrameCount * 1.0) / dElapsed);
      fprintf (stderr, "-------------------------------------------------------\n");
    }
    iBufPos += iSliceSize;
    ++ iSliceIndex;
  }
  // coverity scan uninitial
label_exit:
  if (pBuf) {
    delete[] pBuf;
    pBuf = NULL;
  }
}

int vtkIGTLToMRMLPolyData::SetupDecoder()
{
  WelsCreateDecoder (&decoder_);
  SDecodingParam decParam;
  memset (&decParam, 0, sizeof (SDecodingParam));
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  decoder_->Initialize (&decParam);
  return 1;
}


//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLPolyData);

//---------------------------------------------------------------------------
vtkIGTLToMRMLPolyData::vtkIGTLToMRMLPolyData()
{
  IGTLName = (char*)"PolyData";
  SetupDecoder();
  YUVFrame = NULL;
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLPolyData::~vtkIGTLToMRMLPolyData()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

vtkMRMLNode* vtkIGTLToMRMLPolyData::CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name,igtl::MessageBase::Pointer vtkNotUsed(message) )
{
  vtkSmartPointer<vtkMRMLVolumeNode> volumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  volumeNode->SetAndObserveImageData(image);
  volumeNode->SetName(name);
  
  scene->SaveStateForUndo();
  
  vtkDebugMacro("Setting scene info");
  volumeNode->SetScene(scene);
  volumeNode->SetDescription("Received by OpenIGTLink");
  
  ///double range[2];
  vtkDebugMacro("Set basic display info");
  //volumeNode->GetImageData()->GetScalarRange(range);
  //range[0] = 0.0;
  //range[1] = 256.0;
  //displayNode->SetLowerThreshold(range[0]);
  //displayNode->SetUpperThreshold(range[1]);
  //displayNode->SetWindow(range[1] - range[0]);
  //displayNode->SetLevel(0.5 * (range[1] + range[0]) );
  
  vtkDebugMacro("Name vol node "<<volumeNode->GetClassName());
  vtkMRMLNode* n = scene->AddNode(volumeNode);
  
  
  return n;
}

#include "igtlMessageDebugFunction.h"
//---------------------------------------------------------------------------
uint8_t * vtkIGTLToMRMLPolyData::IGTLToMRML(igtl::MessageBase::Pointer buffer )
{
  int64_t iStart = getTimePoly();
  igtl::VideoMessage::Pointer videoMsg;
  videoMsg = igtl::VideoMessage::New();
  videoMsg->AllocatePack(buffer->GetBodySizeToRead());
  memcpy(videoMsg->GetPackBodyPointer(),(unsigned char*) buffer->GetPackBodyPointer(),buffer->GetBodySizeToRead());
  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  videoMsg->Unpack();
  fprintf (stderr, "Message unpack time:\t%f\n", (getTimePoly()-iStart)/1e6);
  iStart = getTimePoly();
  if (igtl::MessageHeader::UNPACK_BODY)
  {
    //SFrameBSInfo * info  = (SFrameBSInfo *)videoMsg->GetPackFragmentPointer(2); //Here the m_Frame point is receive, the m_FrameHeader is at index 1, we need to check what information we need to put into the image header.
    
    SBufferInfo bufInfo;
    memset (&bufInfo, 0, sizeof (SBufferInfo));
    int32_t iWidth = videoMsg->GetWidth(), iHeight = videoMsg->GetHeight(), streamLength = videoMsg->GetPackBodySize()- IGTL_VIDEO_HEADER_SIZE;
    if (YUVFrame)
      delete [] YUVFrame;
      YUVFrame = NULL;
    YUVFrame = new uint8_t[iHeight*iWidth*3];
    uint8_t* YUV420Frame = new uint8_t[iHeight*iWidth*3/2];
    if (_useCompress)
    {
      H264Decode(this->decoder_, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, YUV420Frame);
    }
    else
    {
      memcpy(YUV420Frame, videoMsg->GetPackFragmentPointer(2), iWidth*iHeight*3/2);
    }
    fprintf (stderr, "decode total time:\t%f\n", (getTimePoly()-iStart)/1e6);
    delete [] YUV420Frame;
    YUV420Frame = NULL;
    return YUVFrame;
  }
  return NULL;
  
}
//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
{
  if (!mrmlNode)
  {
    return 0;
  }
  if (strcmp(mrmlNode->GetNodeTagName(), "IGTLQuery") == 0)   // If mrmlNode is query node
  {
    vtkMRMLIGTLQueryNode* qnode = vtkMRMLIGTLQueryNode::SafeDownCast(mrmlNode);
    if (qnode)
    {
      if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_START)
      {
        if (this->StartVideoMsg.IsNull())
        {
          this->StartVideoMsg = igtl::StartVideoDataMessage::New();
        }
        this->StartVideoMsg->SetDeviceName(qnode->GetIGTLDeviceName());
        this->StartVideoMsg->SetResolution(this->interval);
        this->StartVideoMsg->SetUseCompress(this->_useCompress);
        this->StartVideoMsg->Pack();
        *size = this->StartVideoMsg->GetPackSize();
        *igtlMsg = this->StartVideoMsg->GetPackPointer();
      }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_STOP)
      {
        if (this->StopVideoMsg.IsNull())
        {
          this->StopVideoMsg = igtl::StopVideoMessage::New();
        }
        this->StopVideoMsg->SetDeviceName(qnode->GetIGTLDeviceName());
        this->StopVideoMsg->Pack();
        *size = this->StopVideoMsg->GetPackSize();
        *igtlMsg = this->StopVideoMsg->GetPackPointer();
      }
      return 1;
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::IGTLToVTKScalarType(int igtlType)
{
  switch (igtlType)
  {
    case igtl::VideoMessage::TYPE_INT8: return VTK_CHAR;
    case igtl::VideoMessage::TYPE_UINT8: return VTK_UNSIGNED_CHAR;
    case igtl::VideoMessage::TYPE_INT16: return VTK_SHORT;
    case igtl::VideoMessage::TYPE_UINT16: return VTK_UNSIGNED_SHORT;
    case igtl::VideoMessage::TYPE_INT32: return VTK_UNSIGNED_LONG;
    case igtl::VideoMessage::TYPE_UINT32: return VTK_UNSIGNED_LONG;
    case igtl::VideoMessage::TYPE_FLOAT32: return VTK_FLOAT;
    case igtl::VideoMessage::TYPE_FLOAT64: return VTK_DOUBLE;
    default:
      vtkErrorMacro ("Invalid IGTL scalar Type: "<<igtlType);
      return VTK_VOID;
  }
}
