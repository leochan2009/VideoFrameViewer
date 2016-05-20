/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __SlicerVideoWidgetWidget_h
#define __SlicerVideoWidgetWidget_h

#include <QGLWidget>
#include<QPainter>
#include <QBrush>
#include<sys/time.h>


class SlicerVideoWidget : public QWidget {
public:
  SlicerVideoWidget(QWidget *parent) : QWidget(parent) {
    RGBFrame = NULL;
  }
  void setRGBFrame(uint8_t * inRGBFrame)
  {
    RGBFrame = inRGBFrame;
  }
  
  void draw_frame(void *data) {
    // render data into screen_image_
  }
  
  void start_frame() {
    // do any pre-rendering prep work that needs to be done before
    // each frame
  }
  
  void end_frame() {
    update(); // force a paint event
  }
  
  int64_t getTheTime()
  {
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);
    
  }
  
  void paintEvent(QPaintEvent *) {
    if (this->RGBFrame)
    {
      QPainter p(this);
      int p_width = 1280, p_height = 720;
      //int sizeInMB = (p_width*p_height) >> 20;  //RGBFrame size should be acquired from somewhere
      int64_t iStart = getTheTime();
      screen_image_ = QImage(this->RGBFrame, p_width,p_height,p_width*3, QImage::Format_RGB888);
      QPixmap pixMap = QPixmap();
      pixMap.convertFromImage(screen_image_);
      int64_t iMiddle = getTheTime();
      p.drawPixmap(this->rect(), pixMap, pixMap.rect());
      int64_t iEnd = getTheTime();
      std::cerr<<"Diplay time draw: "<<(iEnd-iMiddle)/1e6<<std::endl;
      std::cerr<<"Diplay time Total: "<<(iEnd-iStart)/1e6<<std::endl;
      
    }
  }
  uint8_t * RGBFrame;
  QImage screen_image_;
};


class SlicerVideoGLWidget : public QGLWidget {
 
protected:
  void initializeGL(){;}

  
public:
  SlicerVideoGLWidget(QWidget *parent) : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
    setAutoFillBackground(false);
    setMinimumSize(20, 20);
    //setFixedSize(1280, 720);
    RGBFrame = NULL;
  }
  void setRGBFrame(uint8_t * inRGBFrame)
  {
    RGBFrame = inRGBFrame;
  }
  void draw_frame(void *data) {
    // render data into screen_image_
  }
  
  void start_frame() {
    // do any pre-rendering prep work that needs to be done before
    // each frame
  }
  
  void end_frame() {
    update(); // force a paint event
  }
  
  int64_t getTheTime()
  {
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);
    
  }
  
  void paintEvent(QPaintEvent *) {
    if (this->RGBFrame)
    {
      QPainter p(this);
      int p_width = 1280, p_height = 720;
      int64_t iStart = getTheTime();
      screen_image_ = QImage(this->RGBFrame, p_width,p_height,p_width*3, QImage::Format_RGB888);
      p.fillRect(this->rect(), QBrush(QColor(64, 32, 64)));
      p.save();
      p.drawImage(this->rect(),screen_image_,screen_image_.rect()); // drawPixMap under OpenGL environment has bug https://bugreports.qt.io/browse/QTBUG-13814
      p.restore();
      int64_t iEnd = getTheTime();
      std::cerr<<"Diplay time Total: "<<(iEnd-iStart)/1e6<<std::endl;
      
    }
  }
  uint8_t * RGBFrame;
  QImage screen_image_;
};

#endif
