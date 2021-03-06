/*==========================================================================
 
 Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See Doc/copyright/copyright.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 Program:   3D Slicer
 Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLDepthVideo.cxx $
 Date:      $Date: 2010-12-07 21:39:19 -0500 (Tue, 07 Dec 2010) $
 Version:   $Revision: 15621 $
 
 ==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLDepthVideo.h"
#include "vtkMRMLIGTLQueryNode.h"
#include <sys/time.h>
#include <time.h>
#include "codec/api/svc/codec_api.h"
#include "codec/api/svc/codec_app_def.h"
#include "test/utils/BufferedData.h"
#include "test/utils/FileInputStream.h"
extern "C" {
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/opt.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/frame.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavcodec/avcodec.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/channel_layout.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/common.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/imgutils.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/mathematics.h"
  #include "/Users/longquanchen/Downloads/libav-10.7/libavutil/samplefmt.h"
}
#define 	AV_INPUT_BUFFER_PADDING_SIZE   8
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

#include "vtkSlicerConnectAndDisplayLogic.h"
#include <omp.h>

#define NO_DELAY_DECODING
namespace vtkIGTLToMRMLDepthVideoFunction{
  int64_t getTime()
  {
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);
    
  }
  void ComposeByteSteam(uint8_t** inputData, SBufferInfo bufInfo, uint8_t *outputByteStream,  int iWidth, int iHeight)
  {
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
}

using namespace vtkIGTLToMRMLDepthVideoFunction;

void vtkIGTLToMRMLDepthVideo::AVDecode(AVCodecContext *c, unsigned char* kpH264BitStream, int32_t& iWidth, int32_t& iHeight, int32_t& iStreamSize, uint8_t* outputByteStream)
{
  AVFrame *frame;
  frame = av_frame_alloc();
  AVPacket avpkt;
  av_init_packet(&avpkt);
  uint8_t inbuf[iStreamSize + AV_INPUT_BUFFER_PADDING_SIZE];
  memcpy(inbuf, kpH264BitStream, iStreamSize);
  memset(inbuf + iStreamSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
  avpkt.size = iStreamSize;
  avpkt.data = inbuf;
  int got_frame;
  int len = avcodec_decode_video2(c, frame, &got_frame, &avpkt);
  if (len)
  {
    memcpy(outputByteStream, frame->data[0], iWidth*iHeight*3/2);
  }
  /* Some codecs, such as MPEG, transmit the I- and P-frame with a
   latency of one frame. You must do the following to have a
   chance to get the last frame of the video. */
  av_frame_free(&frame);

}

void vtkIGTLToMRMLDepthVideo::H264Decode (ISVCDecoder* pDecoder, unsigned char* kpH264BitStream, int32_t& iWidth, int32_t& iHeight, int32_t& iStreamSize, uint8_t* outputByteStream) {
  
  unsigned long long uiTimeStamp = 0;
  int64_t iStart = 0, iEnd = 0, iTotal = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  uint8_t* pBuf = NULL;
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  
  iStart = getTime();
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
    
    iStart = getTime();
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
    iEnd  = getTime();
    iTotal = iEnd - iStart;
    if (sDstBufInfo.iBufferStatus == 1) {
      int64_t iStart2 = getTime();
      ComposeByteSteam(pData, sDstBufInfo, outputByteStream, iWidth,iHeight);
      
      fprintf (stderr, "compose time:\t%f\n", (getTime()-iStart2)/1e6);
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

int vtkIGTLToMRMLDepthVideo::SetupDecoder()
{
 
  SDecodingParam decParam;
  memset (&decParam, 0, sizeof (SDecodingParam));
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  WelsCreateDecoder (&decoderDepth_);
  decoderDepth_->Initialize (&decParam);
  WelsCreateDecoder (&decoderDepthIndex_);
  decoderDepthIndex_->Initialize (&decParam);
  WelsCreateDecoder (&decoderDepthFrame_);
  decoderDepthFrame_->Initialize (&decParam);
  WelsCreateDecoder (&decoderColor_);
  decoderColor_->Initialize (&decParam);
  
  //avcodec_register_all();
  AVCodec *codec;
  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  AVDecoderDepthIndex = avcodec_alloc_context3(codec);
  avcodec_open2(AVDecoderDepthIndex, codec, NULL);
  AVDecoderDepthFrame = avcodec_alloc_context3(codec);
  avcodec_open2(AVDecoderDepthFrame, codec, NULL);
  AVDecoderDepthColor = avcodec_alloc_context3(codec);
  avcodec_open2(AVDecoderDepthColor, codec, NULL);
  
  return 1;
}


//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLDepthVideo);

//---------------------------------------------------------------------------
vtkIGTLToMRMLDepthVideo::vtkIGTLToMRMLDepthVideo()
{
  IGTLName = (char*)"Video";
  SetupDecoder();
  RGBFrame = NULL;
  DepthFrame = NULL;
  RequireYUVToRBGConversion = true;
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLDepthVideo::~vtkIGTLToMRMLDepthVideo()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLDepthVideo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

vtkMRMLNode* vtkIGTLToMRMLDepthVideo::CreateNewNodeWithMessage(vtkMRMLScene* scene, const char* name,igtl::MessageBase::Pointer vtkNotUsed(message) )
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
  
  vtkDebugMacro("Name vol node "<<volumeNode->GetClassName());
  vtkMRMLNode* n = scene->AddNode(volumeNode);
  
  
  return n;
}

//#include "igtlMessageDebugFunction.h"
//---------------------------------------------------------------------------
uint8_t * vtkIGTLToMRMLDepthVideo::IGTLToMRML(igtl::MessageBase::Pointer buffer )
{
  int64_t iStart = getTime();
  igtl::VideoMessage::Pointer videoMsg;
  videoMsg = igtl::VideoMessage::New();
  videoMsg->AllocatePack(buffer->GetBodySizeToRead());
  if(strcmp(buffer->GetDeviceName(), "DepthFrame") == 0 || strcmp(buffer->GetDeviceName(), "DepthIndex") == 0)
  {
    memcpy(videoMsg->GetPackBodyPointer(),(unsigned char*) buffer->GetPackBodyPointer(),buffer->GetBodySizeToRead());
    // Deserialize the transform data
    // If you want to skip CRC check, call Unpack() without argument.
    videoMsg->Unpack();
    fprintf (stderr, "Message unpack time:\t%f\n", (getTime()-iStart)/1e6);
    iStart = getTime();
    if (igtl::MessageHeader::UNPACK_BODY)
    {
      //SFrameBSInfo * info  = (SFrameBSInfo *)videoMsg->GetPackFragmentPointer(2); //Here the m_Frame point is receive, the m_FrameHeader is at index 1, we need to check what information we need to put into the image header.
      
      SBufferInfo bufInfo;
      memset (&bufInfo, 0, sizeof (SBufferInfo));
      int32_t iWidth = videoMsg->GetWidth(), iHeight = videoMsg->GetHeight(), streamLength = videoMsg->GetPackBodySize()- IGTL_VIDEO_HEADER_SIZE;
      
      
      int DemuxMethod = 2;
      if (DemuxMethod == 1)
      {
        if (DepthIndex)
          delete [] DepthIndex;
        DepthIndex = NULL;
        if (DepthFrame)
          delete [] DepthFrame;
        DepthFrame = NULL;
        DepthIndex = new uint8_t[1]; // just to make the work flow continue;
        uint8_t* YUV420Frame = new uint8_t[iHeight*iWidth*3/2];
        H264Decode(this->decoderDepth_, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, YUV420Frame);
        DepthFrame = new uint8_t[iHeight*iWidth*3/2];
        memcpy(DepthFrame, YUV420Frame, iWidth*iHeight*3/2);
        delete [] YUV420Frame;
        YUV420Frame = NULL;
        fprintf (stderr, "decode depth frame total time:\t%f\n", (getTime()-iStart)/1e6);
        return DepthFrame;
      }
      else if (DemuxMethod == 2)
      {
        if(strcmp(buffer->GetDeviceName(), "DepthFrame") == 0)
        {
          if (DepthFrame)
            delete [] DepthFrame;
          DepthFrame = NULL;
          DepthFrame = new uint8_t[iHeight*iWidth];
          memcpy(DepthFrame, videoMsg->GetPackFragmentPointer(2), iWidth*iHeight);
          std::string outname = "DepthFrame.264";
          FILE* pf = fopen(outname.c_str(), "a");
          if (pf == NULL){
            fprintf(stderr, "Error open file %s\nPress ENTER to exit\n");
            getchar();
            exit(-1);
          }
          int temp;
          for(unsigned int r=0; r<iHeight; r++)
          {
            for(unsigned int c=0; c<iWidth; c++)
            {
              
              fputc( DepthFrame[r* iWidth+ c], pf);
            }
          }
          for (int r=0; r<iHeight*iWidth/2; r++){
            temp = fputc(0, pf);
          }
          fclose(pf);
          return DepthFrame;
        }
        else if(strcmp(buffer->GetDeviceName(), "DepthIndex") == 0)
        {
          if (DepthIndex)
            delete [] DepthIndex;
          DepthIndex = NULL;
          DepthIndex = new uint8_t[iHeight*iWidth];
          memcpy(DepthIndex, videoMsg->GetPackFragmentPointer(2), iWidth*iHeight);
          std::string outname = "Frame2.YUV";
          FILE* pf2 = fopen(outname.c_str(), "a");
          if (pf2 == NULL){
            fprintf(stderr, "Error open file %s\nPress ENTER to exit\n");
            getchar();
            exit(-1);
          }
          for(unsigned int r=0; r<iHeight; r++)
          {
            for(unsigned int c=0; c<iWidth; c++)
            {
              
              fputc( DepthIndex[r* iWidth+ c], pf2);
            }
          }
          int temp;
          for (int r=0; r<iHeight*iWidth/2; r++){
            temp = fputc(0, pf2);
          }
          fclose(pf2);
          return DepthIndex;
        }
      }
      else if (DemuxMethod == 3)
      {
        if(strcmp(buffer->GetDeviceName(), "DepthFrame") == 0)
        {
          if (DepthFrame)
            delete [] DepthFrame;
          DepthFrame = NULL;
          DepthFrame = new uint8_t[iHeight*iWidth*3/2];
          AVDecode(this->AVDecoderDepthIndex, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, DepthFrame);
          //H264Decode(this->decoderDepthIndex_, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, DepthFrame);
          std::string outname = "DepthFrame.264";
          FILE* pf = fopen(outname.c_str(), "a");
          int temp;
          for (int r=0; r<streamLength; r++){
            temp = fputc(*(videoMsg->GetPackFragmentPointer(2)+r), pf);
          }
          fclose(pf);
          return DepthFrame;
        }
        else if(strcmp(buffer->GetDeviceName(), "DepthIndex") == 0)
        {
          if (DepthIndex)
            delete [] DepthIndex;
          DepthIndex = NULL;
          DepthIndex = new uint8_t[iHeight*iWidth*3/2];
          AVDecode(this->AVDecoderDepthFrame, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, DepthIndex);
          //H264Decode(this->decoderDepthFrame_, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, DepthIndex);
          std::string outname = "DepthIndex.264";
          FILE* pf = fopen(outname.c_str(), "a");
          int temp;
          for (int r=0; r<streamLength; r++){
            temp = fputc(*(videoMsg->GetPackFragmentPointer(2)+r), pf);
          }
          fclose(pf);
          return DepthIndex;
        }
      }
    }
  }
  else if(strcmp(buffer->GetDeviceName(), "ColorFrame") == 0)
  {
    memcpy(videoMsg->GetPackBodyPointer(),(unsigned char*) buffer->GetPackBodyPointer(),buffer->GetBodySizeToRead());
    // Deserialize the transform data
    // If you want to skip CRC check, call Unpack() without argument.
    videoMsg->Unpack();
    fprintf (stderr, "Message unpack time:\t%f\n", (getTime()-iStart)/1e6);
    iStart = getTime();
    if (igtl::MessageHeader::UNPACK_BODY)
    {
      //SFrameBSInfo * info  = (SFrameBSInfo *)videoMsg->GetPackFragmentPointer(2); //Here the m_Frame point is receive, the m_FrameHeader is at index 1, we need to check what information we need to put into the image header.
      
      SBufferInfo bufInfo;
      memset (&bufInfo, 0, sizeof (SBufferInfo));
      int32_t iWidth = videoMsg->GetWidth(), iHeight = videoMsg->GetHeight(), streamLength = videoMsg->GetPackBodySize()- IGTL_VIDEO_HEADER_SIZE;
      if (RGBFrame)
        delete [] RGBFrame;
      RGBFrame = NULL;
      uint8_t* YUV420Frame = new uint8_t[iHeight*iWidth*3/2];
      RGBFrame = new uint8_t[iHeight*iWidth*3];
      bool UseCompressForRGB = false;
      if (UseCompressForRGB)
      {
        AVDecode(this->AVDecoderDepthColor, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, YUV420Frame);
        //H264Decode(this->decoderColor_, videoMsg->GetPackFragmentPointer(2), iWidth, iHeight, streamLength, YUV420Frame);
        std::string outname = "ColorFrame.264";
        FILE* pf = fopen(outname.c_str(), "a");
        int temp;
        for (int r=0; r<streamLength; r++){
          temp = fputc(*(videoMsg->GetPackFragmentPointer(2)+r), pf);
        }
        fclose(pf);
        bool bConverion = YUV420ToRGBConversion(RGBFrame, YUV420Frame, iHeight, iWidth);
      }
      else
      {
        memcpy(RGBFrame, videoMsg->GetPackFragmentPointer(2), iWidth*iHeight*3);
      }
      fprintf (stderr, "decode color frame total time:\t%f\n", (getTime()-iStart)/1e6);
      delete [] YUV420Frame;
      YUV420Frame = NULL;
      return RGBFrame;
    }
  }
  return NULL;
  
}

bool vtkIGTLToMRMLDepthVideo::RGBUpsampling(uint8_t *RGBFrame, uint8_t * YUV420Frame, int iHeight, int iWidth)
{
  size_t i = 0;
  for (int x = 0; x < iWidth*iHeight/2; x++) {
    RGBFrame[3*(i+1)] = RGBFrame[3*i]  = YUV420Frame[3*x];
    RGBFrame[3*(i+1)+1] = RGBFrame[3*i+1] = YUV420Frame[3*x + 1];
    RGBFrame[3*(i+1)+2] = RGBFrame[3*i+2] = YUV420Frame[3*x + 2];
    i = i + 2;
  }
}

int vtkIGTLToMRMLDepthVideo::YUV420ToRGBConversion(uint8_t *RGBFrame, uint8_t * YUV420Frame, int iHeight, int iWidth)
{
  int componentLength = iHeight*iWidth;
  const uint8_t *srcY = YUV420Frame;
  const uint8_t *srcU = YUV420Frame + componentLength;
  const uint8_t *srcV = srcY + componentLength + componentLength/4;
  uint8_t * YUV444 = new uint8_t[componentLength * 3];
  uint8_t *dstY = YUV444;
  uint8_t *dstU = dstY + componentLength;
  uint8_t *dstV = dstU + componentLength;
  
  memcpy(dstY, srcY, componentLength);
  const int halfHeight = iHeight/2;
  const int halfWidth = iWidth/2;

#pragma omp parallel for default(none) shared(dstV,dstU,srcV,srcU,iWidth)
  for (int y = 0; y < halfHeight; y++) {
    for (int x = 0; x < halfWidth; x++) {
      dstU[2 * x + 2 * y*iWidth] = dstU[2 * x + 1 + 2 * y*iWidth] = srcU[x + y*iWidth/2];
      dstV[2 * x + 2 * y*iWidth] = dstV[2 * x + 1 + 2 * y*iWidth] = srcV[x + y*iWidth/2];
    }
    memcpy(&dstU[(2 * y + 1)*iWidth], &dstU[(2 * y)*iWidth], iWidth);
    memcpy(&dstV[(2 * y + 1)*iWidth], &dstV[(2 * y)*iWidth], iWidth);
  }
  
  const int yOffset = 16;
  const int cZero = 128;
  int yMult, rvMult, guMult, gvMult, buMult;
  yMult =   76309;
  rvMult = 117489;
  guMult = -13975;
  gvMult = -34925;
  buMult = 138438;
  
  static unsigned char clp_buf[384+256+384];
  static unsigned char *clip_buf = clp_buf+384;
  
  // initialize clipping table
  memset(clp_buf, 0, 384);
  
  for (int i = 0; i < 256; i++) {
    clp_buf[384+i] = i;
  }
  memset(clp_buf+384+256, 255, 384);
  
  
#pragma omp parallel for default(none) shared(dstY,dstU,dstV,RGBFrame,yMult,rvMult,guMult,gvMult,buMult,clip_buf,componentLength)// num_threads(2)
  for (int i = 0; i < componentLength; ++i) {
    const int Y_tmp = ((int)dstY[i] - yOffset) * yMult;
    const int U_tmp = (int)dstU[i] - cZero;
    const int V_tmp = (int)dstV[i] - cZero;
    
    const int R_tmp = (Y_tmp                  + V_tmp * rvMult ) >> 16;//32 to 16 bit conversion by left shifting
    const int G_tmp = (Y_tmp + U_tmp * guMult + V_tmp * gvMult ) >> 16;
    const int B_tmp = (Y_tmp + U_tmp * buMult                  ) >> 16;
    
    RGBFrame[3*i]   = clip_buf[R_tmp];
    RGBFrame[3*i+1] = clip_buf[G_tmp];
    RGBFrame[3*i+2] = clip_buf[B_tmp];
  }

  delete [] YUV444;
  YUV444 = NULL;
  dstY = NULL;
  dstU = NULL;
  dstV = NULL;
  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLDepthVideo::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
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
        this->StartVideoMsg->SetResolution(this->Interval);
        this->StartVideoMsg->SetUseCompress(this->UseCompress);
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
int vtkIGTLToMRMLDepthVideo::IGTLToVTKScalarType(int igtlType)
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
