#include <sys/time.h>
#include <time.h>
#include <QString>
#include <QRegExp>
#include "codec/api/svc/codec_api.h"
#include "codec/api/svc/codec_app_def.h"


#define NO_DELAY_DECODING

void Write2File (FILE* pFp, unsigned char* pData[3], int iStride[2], int iWidth, int iHeight) {
  int   i;
  unsigned char*  pPtr = NULL;
  
  pPtr = pData[0];
  for (i = 0; i < iHeight; i++) {
    fwrite (pPtr, 1, iWidth, pFp);
    pPtr += iStride[0];
  }
  
  iHeight = iHeight / 2;
  iWidth = iWidth / 2;
  pPtr = pData[1];
  for (i = 0; i < iHeight; i++) {
    fwrite (pPtr, 1, iWidth, pFp);
    pPtr += iStride[1];
  }
  
  pPtr = pData[2];
  for (i = 0; i < iHeight; i++) {
    fwrite (pPtr, 1, iWidth, pFp);
    pPtr += iStride[1];
  }
}


int Process (void* pDst[3], SBufferInfo* pInfo, FILE* pFp) {
  
  int iRet = 0;
  
    if (pFp && pDst[0] && pDst[1] && pDst[2] && pInfo) {
      int iStride[2];
      int iWidth = pInfo->UsrData.sSystemBuffer.iWidth;
      int iHeight = pInfo->UsrData.sSystemBuffer.iHeight;
      iStride[0] = pInfo->UsrData.sSystemBuffer.iStride[0];
      iStride[1] = pInfo->UsrData.sSystemBuffer.iStride[1];
      
      Write2File (pFp, (unsigned char**)pDst, iStride, iWidth, iHeight);
    }
  return iRet;
}

int64_t getCurrentTime()
{
  struct timeval tv_date;
  gettimeofday(&tv_date, NULL);
  return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);

}

void H264DecodeInstance (ISVCDecoder* pDecoder, unsigned char* kpH264BitStream, const char* kpOuputFileName,
                         int32_t& iWidth, int32_t& iHeight, int32_t& iStreamSize, const char* pOptionFileName) {

  
  unsigned long long uiTimeStamp = 0;
  int64_t iStart = 0, iEnd = 0, iTotal = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  uint8_t* pBuf = NULL;
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  
  uint8_t* pData[3] = {NULL};
  uint8_t* pDst[3] = {NULL};
  SBufferInfo sDstBufInfo;
  
  int32_t iBufPos = 0;
  int32_t i = 0;
  int32_t iLastWidth = 0, iLastHeight = 0;
  int32_t iFrameCount = 0;
  int32_t iEndOfStreamFlag = 0;
  //for coverage test purpose
  int32_t iErrorConMethod = (int32_t) ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
  pDecoder->SetOption (DECODER_OPTION_ERROR_CON_IDC, &iErrorConMethod);
  //~end for
  double dElapsed = 0;
  
  if (pDecoder == NULL) return;
  
  
  FILE* pYuvFile    = NULL;
  FILE* pOptionFile = NULL;
  // Lenght input mode support
  if (kpOuputFileName) {
    pYuvFile = fopen (kpOuputFileName, "ab");
    if (pYuvFile == NULL) {
      fprintf (stderr, "Can not open yuv file to output result of decoding..\n");
      // any options
      //return; // can let decoder work in quiet mode, no writing any output
    } else
      fprintf (stderr, "Sequence output file name: %s..\n", kpOuputFileName);
  } else {
    fprintf (stderr, "Can not find any output file to write..\n");
    // any options
  }
  
  if (pOptionFileName) {
    pOptionFile = fopen (pOptionFileName, "wb");
    if (pOptionFile == NULL) {
      fprintf (stderr, "Can not open optional file for write..\n");
    } else
      fprintf (stderr, "Extra optional file: %s..\n", pOptionFileName);
  }
  
  printf ("------------------------------------------------------\n");
  
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
    
    //for coverage test purpose
    int32_t iEndOfStreamFlag;
    pDecoder->GetOption (DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);
    int32_t iCurIdrPicId;
    pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &iCurIdrPicId);
    int32_t iFrameNum;
    pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &iFrameNum);
    int32_t bCurAuContainLtrMarkSeFlag;
    pDecoder->GetOption (DECODER_OPTION_LTR_MARKING_FLAG, &bCurAuContainLtrMarkSeFlag);
    int32_t iFrameNumOfAuMarkedLtr;
    pDecoder->GetOption (DECODER_OPTION_LTR_MARKED_FRAME_NUM, &iFrameNumOfAuMarkedLtr);
    int32_t iFeedbackVclNalInAu;
    pDecoder->GetOption (DECODER_OPTION_VCL_NAL, &iFeedbackVclNalInAu);
    int32_t iFeedbackTidInAu;
    pDecoder->GetOption (DECODER_OPTION_TEMPORAL_ID, &iFeedbackTidInAu);
    //~end for
    
    iStart = getCurrentTime();
    pData[0] = NULL;
    pData[1] = NULL;
    pData[2] = NULL;
    uiTimeStamp ++;
    memset (&sDstBufInfo, 0, sizeof (SBufferInfo));
    sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;
    sDstBufInfo.UsrData.sSystemBuffer.iWidth =
#ifndef NO_DELAY_DECODING
    pDecoder->DecodeFrameNoDelay (pBuf + iBufPos, iSliceSize, pData, &sDstBufInfo);
#else
    pDecoder->DecodeFrame2 (pBuf + iBufPos, iSliceSize, pData, &sDstBufInfo);
#endif
    
    if (sDstBufInfo.iBufferStatus == 1) {
      pDst[0] = pData[0];
      pDst[1] = pData[1];
      pDst[2] = pData[2];
    }
    iEnd    = getCurrentTime();
    iTotal  = iEnd - iStart;
    if (sDstBufInfo.iBufferStatus == 1) {
      Process((void**)pDst, &sDstBufInfo, pYuvFile);
      iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
      iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
      
      if (pOptionFile != NULL) {
        if (iWidth != iLastWidth && iHeight != iLastHeight) {
          fwrite (&iFrameCount, sizeof (iFrameCount), 1, pOptionFile);
          fwrite (&iWidth , sizeof (iWidth) , 1, pOptionFile);
          fwrite (&iHeight, sizeof (iHeight), 1, pOptionFile);
          iLastWidth  = iWidth;
          iLastHeight = iHeight;
        }
      }
      ++ iFrameCount;
    }
    
#ifdef NO_DELAY_DECODING
    iStart = getCurrentTime();
    pData[0] = NULL;
    pData[1] = NULL;
    pData[2] = NULL;
    memset (&sDstBufInfo, 0, sizeof (SBufferInfo));
    sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;
    pDecoder->DecodeFrame2 (NULL, 0, pData, &sDstBufInfo);
    if (sDstBufInfo.iBufferStatus == 1) {
      pDst[0] = pData[0];
      pDst[1] = pData[1];
      pDst[2] = pData[2];
    }
    iEnd    = getCurrentTime();
    iTotal = iEnd - iStart;
    if (sDstBufInfo.iBufferStatus == 1) {
      Process ((void**)pDst, &sDstBufInfo, pYuvFile);
      iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
      iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
      
      if (pOptionFile != NULL) {
        if (iWidth != iLastWidth && iHeight != iLastHeight) {
          fwrite (&iFrameCount, sizeof (iFrameCount), 1, pOptionFile);
          fwrite (&iWidth , sizeof (iWidth) , 1, pOptionFile);
          fwrite (&iHeight, sizeof (iHeight), 1, pOptionFile);
          iLastWidth  = iWidth;
          iLastHeight = iHeight;
        }
      }
      ++ iFrameCount;
    }
#endif
    dElapsed = iTotal / 1e6;
    fprintf (stderr, "-------------------------------------------------------\n");
    fprintf (stderr, "iWidth:\t\t%d\nheight:\t\t%d\nFrames:\t\t%d\ndecode time:\t%f sec\nFPS:\t\t%f fps\n",
             iWidth, iHeight, iFrameCount, dElapsed, (iFrameCount * 1.0) / dElapsed);
    fprintf (stderr, "-------------------------------------------------------\n");
    iBufPos += iSliceSize;
    ++ iSliceIndex;
  }
  // coverity scan uninitial
label_exit:
  if (pBuf) {
    delete[] pBuf;
    pBuf = NULL;
  }
  if (pYuvFile) {
    fclose (pYuvFile);
    pYuvFile = NULL;
  }
  if (pOptionFile) {
    fclose (pOptionFile);
    pOptionFile = NULL;
  }
}

void formatFromFilename(QString name, int &width, int &height, int &frameRate, int &bitDepth, int &subFormat)
{
  // preset return values first
  width = -1;
  height = -1;
  frameRate = -1;
  bitDepth = -1;
  subFormat = -1;
  
  if (name.isEmpty())
    return;
  
  // parse filename and extract width, height and framerate
  // default format is: sequenceName_widthxheight_framerate.yuv
  QRegExp rxExtendedFormat("([0-9]+)x([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)");
  QRegExp rxExtended("([0-9]+)x([0-9]+)_([0-9]+)_([0-9]+)");
  QRegExp rxDefault("([0-9]+)x([0-9]+)_([0-9]+)");
  QRegExp rxSizeOnly("([0-9]+)x([0-9]+)");
  
  if (rxExtendedFormat.indexIn(name) > -1)
  {
    QString widthString = rxExtendedFormat.cap(1);
    width = widthString.toInt();
    
    QString heightString = rxExtendedFormat.cap(2);
    height = heightString.toInt();
    
    QString rateString = rxExtendedFormat.cap(3);
    frameRate = rateString.toDouble();
    
    QString bitDepthString = rxExtendedFormat.cap(4);
    bitDepth = bitDepthString.toInt();
    
    QString subSampling = rxExtendedFormat.cap(5);
    subFormat = subSampling.toInt();
    
  }
  else if (rxExtended.indexIn(name) > -1)
  {
    QString widthString = rxExtended.cap(1);
    width = widthString.toInt();
    
    QString heightString = rxExtended.cap(2);
    height = heightString.toInt();
    
    QString rateString = rxExtended.cap(3);
    frameRate = rateString.toDouble();
    
    QString bitDepthString = rxExtended.cap(4);
    bitDepth = bitDepthString.toInt();
  }
  else if (rxDefault.indexIn(name) > -1) {
    QString widthString = rxDefault.cap(1);
    width = widthString.toInt();
    
    QString heightString = rxDefault.cap(2);
    height = heightString.toInt();
    
    QString rateString = rxDefault.cap(3);
    frameRate = rateString.toDouble();
    
    bitDepth = 8; // assume 8 bit
  }
  else if (rxSizeOnly.indexIn(name) > -1) {
    QString widthString = rxSizeOnly.cap(1);
    width = widthString.toInt();
    
    QString heightString = rxSizeOnly.cap(2);
    height = heightString.toInt();
    
    bitDepth = 8; // assume 8 bit
  }
  else
  {
    // try to find resolution indicators (e.g. 'cif', 'hd') in file name
    if (name.contains("_cif", Qt::CaseInsensitive))
    {
      width = 352;
      height = 288;
    }
    else if (name.contains("_qcif", Qt::CaseInsensitive))
    {
      width = 176;
      height = 144;
    }
    else if (name.contains("_4cif", Qt::CaseInsensitive))
    {
      width = 704;
      height = 576;
    }
  }
}
