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

// Qt includes
#include <QDebug>
// Qt includes
#include <QButtonGroup>
#include <QTimer>
#include <QPointer>

// SlicerQt includes
#include "qSlicerConnectAndDisplayModuleWidget.h"
#include "ui_qSlicerConnectAndDisplayModuleWidget.h"

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qSlicerWidget.h"

// qMRMLWidget includes
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLSliderWidget.h"

// Logic include
#include "vtkSlicerConnectAndDisplayLogic.h"

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkIGTLToMRMLVideo.h"
#include "vtkMRMLIGTLQueryNode.h"
#include <qMRMLNodeFactory.h>
#include <vtkNew.h>
//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerConnectAndDisplayModuleWidgetPrivate: public Ui_qSlicerConnectAndDisplayModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerConnectAndDisplayModuleWidget);
protected:
  qSlicerConnectAndDisplayModuleWidget* const q_ptr;
public:
  qSlicerConnectAndDisplayModuleWidgetPrivate(qSlicerConnectAndDisplayModuleWidget& object);
  
  vtkMRMLIGTLConnectorNode * IGTLConnectorNode;
  vtkMRMLIGTLQueryNode * IGTLDataQueryNode;
  vtkIGTLToMRMLVideo* converter;
  QTimer ImportDataAndEventsTimer;
  vtkSlicerConnectAndDisplayLogic * logic();
  uint8_t * RGBFrame;
  SlicerVideoGLWidget *openGL;
  
  vtkRenderer* activeRenderer;
  vtkRenderer*   BackgroundRenderer;
  vtkImageActor* BackgroundActor;
  vtkImageData* imageData ;


};

//-----------------------------------------------------------------------------
// qSlicerConnectAndDisplayModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerConnectAndDisplayModuleWidgetPrivate::qSlicerConnectAndDisplayModuleWidgetPrivate(qSlicerConnectAndDisplayModuleWidget& object):q_ptr(&object)
{
  this->IGTLConnectorNode = NULL;
  this->IGTLDataQueryNode = NULL;
  RGBFrame = new uint8_t[1280*720*3];
  openGL = new SlicerVideoGLWidget(q_ptr);
  openGL->setGeometry(QRect(130, 180, 1280, 720));
  //uint8_t *RGBFrame = new uint8_t[1];
  //openGL->setRGBFrame(RGBFrame);
}


//-----------------------------------------------------------------------------
vtkSlicerConnectAndDisplayLogic * qSlicerConnectAndDisplayModuleWidgetPrivate::logic()
{
  Q_Q(qSlicerConnectAndDisplayModuleWidget);
  return vtkSlicerConnectAndDisplayLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerConnectAndDisplayModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerConnectAndDisplayModuleWidget::qSlicerConnectAndDisplayModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerConnectAndDisplayModuleWidgetPrivate(*this) )
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
  QObject::connect(&d->ImportDataAndEventsTimer, SIGNAL(timeout()),
          this, SLOT(importDataAndEvents()));
  QObject::connect(d->ConnectorPortEdit, SIGNAL(editingFinished()),
                   this, SLOT(updateIGTLConnectorNode()));
  QObject::connect(d->ConnectorHostAddressEdit, SIGNAL(editingFinished()),
                   this, SLOT(updateIGTLConnectorNode()));
  QObject::connect(d->ConnectorStateCheckBox, SIGNAL(toggled(bool)),
                   this, SLOT(startCurrentIGTLConnector(bool)));
  QObject::connect(d->StartVideoCheckBox, SIGNAL(toggled(bool)),
                   this, SLOT(startVideoTransmission(bool)));
  
  qSlicerApplication *  app = qSlicerApplication::application();
  qMRMLThreeDView* threeDView = app->layoutManager()->threeDWidget(0)->threeDView();
  vtkRenderer* activeRenderer = app->layoutManager()->activeThreeDRenderer();
  vtkRenderWindow* activeRenderWindow = activeRenderer->GetRenderWindow();
  d->imageData = vtkImageData::New();
  d->imageData->SetDimensions(1280, 720, 1);
  d->imageData->SetExtent(0, 1279,0, 719, 0, 0);
  d->imageData->SetSpacing(1.0, 1.0, 1.0);
  d->imageData->SetOrigin(0.0, 0.0, 0.0);
  vtkInformation* meta_data = vtkInformation::New();
  d->imageData->SetNumberOfScalarComponents(3, meta_data);
  //d->imageData->SetScalarTypeToUnsignedChar();
  d->imageData->AllocateScalars(VTK_UNSIGNED_CHAR,3);
  activeRenderer->SetLayer(1);
  if (activeRenderer)
  {
    
    d->BackgroundActor = vtkImageActor::New();
    d->BackgroundActor->SetInputData(d->imageData);
    
    d->BackgroundRenderer = vtkRenderer::New();
    d->BackgroundRenderer->InteractiveOff();
    d->BackgroundRenderer->SetLayer(0);
    d->BackgroundRenderer->AddActor(d->BackgroundActor);
    
    d->BackgroundRenderer->GradientBackgroundOff();
    d->BackgroundRenderer->SetBackground(0,0,0);
    
    d->BackgroundActor->Modified();
    
    activeRenderWindow->AddRenderer(d->BackgroundRenderer);
    //BackgroundRenderer->GetActiveCamera();
    activeRenderWindow->Render();
    
  }
}

//-----------------------------------------------------------------------------
qSlicerConnectAndDisplayModuleWidget::~qSlicerConnectAndDisplayModuleWidget()
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  d->IGTLConnectorNode->UnregisterMessageConverter(d->converter);
  d->converter->Delete();
}

void qSlicerConnectAndDisplayModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  this->Superclass::setMRMLScene(scene);
  if (d->NodeSelector)
  {
    d->NodeSelector->setMRMLScene(scene);
  }
  if (this->mrmlScene())
  {
    d->IGTLConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(this->mrmlScene()->CreateNodeByClass("vtkMRMLIGTLConnectorNode"));
    d->IGTLDataQueryNode = vtkMRMLIGTLQueryNode::SafeDownCast(this->mrmlScene()->CreateNodeByClass("vtkMRMLIGTLQueryNode"));
    this->mrmlScene()->AddNode(d->IGTLConnectorNode); //node added cause the IGTLConnectorNode be initialized
    this->mrmlScene()->AddNode(d->IGTLDataQueryNode);
    d->converter = vtkIGTLToMRMLVideo::New();
    d->IGTLConnectorNode->RegisterMessageConverter(d->converter);
    if (d->IGTLConnectorNode)
    {
      // If the timer is not active
      if (!d->ImportDataAndEventsTimer.isActive())
      {
        d->ImportDataAndEventsTimer.start(5);
      }
    }
  }
}

//------------------------------------------------------------------------------
void qSlicerConnectAndDisplayModuleWidget::setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode * connectorNode)
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  qvtkReconnect(d->IGTLConnectorNode, connectorNode, vtkCommand::ModifiedEvent,
                this, SLOT(onMRMLNodeModified()));
  foreach(int evendId, QList<int>()
          << vtkMRMLIGTLConnectorNode::ActivatedEvent
          << vtkMRMLIGTLConnectorNode::ConnectedEvent
          << vtkMRMLIGTLConnectorNode::DisconnectedEvent
          << vtkMRMLIGTLConnectorNode::DeactivatedEvent)
  {
    qvtkReconnect(d->IGTLConnectorNode, connectorNode, evendId,
                  this, SLOT(onMRMLNodeModified()));
  }
  
  this->onMRMLNodeModified();
  this->setEnabled(connectorNode != 0);
}
//------------------------------------------------------------------------------
void qSlicerConnectAndDisplayModuleWidget::setMRMLIGTLConnectorNode(vtkMRMLNode* node)
{
  this->setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode::SafeDownCast(node));
}

//------------------------------------------------------------------------------
void qSlicerConnectAndDisplayModuleWidget::onMRMLNodeModified()
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  if (!d->IGTLConnectorNode)
  {
    return;
  }
  d->NodeSelector->setCurrentNode(d->IGTLConnectorNode);
  
  d->ConnectorPortEdit->setText(QString("%1").arg(d->IGTLConnectorNode->GetServerPort()));
  
  bool deactivated = d->IGTLConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::STATE_OFF;
  d->ConnectorStateCheckBox->setChecked(!deactivated);
}

//------------------------------------------------------------------------------
void qSlicerConnectAndDisplayModuleWidget::startCurrentIGTLConnector(bool value)
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  
  Q_ASSERT(d->IGTLConnectorNode);
  if (value)
  {
    d->IGTLConnectorNode->SetTypeClient(d->ConnectorHostAddressEdit->text().toStdString(), d->ConnectorPortEdit->text().toInt());
    
    bool success = false;
    int attemptTimes = 0;
    while(attemptTimes<10 && (not success) )
    {
      success = d->IGTLConnectorNode->Start();
      if (success)
        break;
      int milliseconds = 300;
      struct timespec req;
      req.tv_sec  = milliseconds / 1000;
      req.tv_nsec = (milliseconds % 1000) * 1000000;
      while ((nanosleep(&req, &req) == -1) && (errno == EINTR))
      {
        continue;
      }
      attemptTimes++;
    }
    if(not success)
    {
      d->ConnectorStateCheckBox->setCheckState(Qt::CheckState::Unchecked);
    }
  }
  else
  {
    d->IGTLConnectorNode->Stop();
  }
}

//------------------------------------------------------------------------------
void qSlicerConnectAndDisplayModuleWidget::startVideoTransmission(bool value)
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  Q_ASSERT(d->IGTLConnectorNode);
  
  if(d->StartVideoCheckBox->checkState() == Qt::CheckState::Checked)
  {
    if (d->FrameFrequency->text().toInt()>0.0000001 && d->FrameFrequency->text().toInt()<1000000)
    {
      d->IGTLDataQueryNode->SetIGTLName("Video");
      int interval = (int) (1000.0 / d->FrameFrequency->text().toInt());
      d->IGTLDataQueryNode->SetQueryType(d->IGTLDataQueryNode->TYPE_START);
      d->IGTLDataQueryNode->SetQueryStatus(d->IGTLDataQueryNode->STATUS_PREPARED);
      d->IGTLConnectorNode->setInterval(interval);
      d->IGTLConnectorNode->setUseCompress(d->UseCompressCheckBox->isChecked());
      d->IGTLConnectorNode->PushQuery(d->IGTLDataQueryNode);
    }
  }
  else
  {
    d->IGTLDataQueryNode->SetIGTLName("Video");
    d->IGTLDataQueryNode->SetQueryType(d->IGTLDataQueryNode->TYPE_STOP);
    d->IGTLDataQueryNode->SetQueryStatus(d->IGTLDataQueryNode->STATUS_PREPARED);
    d->IGTLConnectorNode->PushQuery(d->IGTLDataQueryNode);
  }
}

//------------------------------------------------------------------------------
void qSlicerConnectAndDisplayModuleWidget::updateIGTLConnectorNode()
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  if (d->IGTLConnectorNode != NULL)
  {
    d->IGTLConnectorNode->DisableModifiedEventOn();
    d->IGTLConnectorNode->SetServerHostname(d->ConnectorHostAddressEdit->text().toStdString());
    d->IGTLConnectorNode->SetServerPort(d->ConnectorPortEdit->text().toInt());
    d->IGTLConnectorNode->DisableModifiedEventOff();
    d->IGTLConnectorNode->InvokePendingModifiedEvent();
  }
}


void qSlicerConnectAndDisplayModuleWidget::importDataAndEvents()
{
  Q_D(qSlicerConnectAndDisplayModuleWidget);
  vtkMRMLAbstractLogic* l = this->logic();
  vtkSlicerConnectAndDisplayLogic * igtlLogic = vtkSlicerConnectAndDisplayLogic::SafeDownCast(l);
  if (igtlLogic)
  {
    int64_t startTime = Connector::getTime();
    uint8_t *frame = igtlLogic->CallConnectorTimerHander();
    //-------------------
    // Convert the image in p_PixmapConversionBuffer to a QPixmap
    if (frame && d->StartVideoCheckBox->checkState() == Qt::CheckState::Checked)
    {
      memcpy(d->RGBFrame, frame, 1280*720*3);
      d->graphicsView->setRGBFrame(d->RGBFrame);
      d->graphicsView->update();
      d->openGL->setRGBFrame(d->RGBFrame);
      d->openGL->update();
      std::cerr<<"CallConnectorTimerHander Time: "<<(Connector::getTime()-startTime)/1e6 << std::endl;
      memcpy((void*)d->imageData->GetScalarPointer(), (void*)d->RGBFrame, 1280*720*3);
      d->imageData->Modified();
      d->BackgroundRenderer->GetRenderWindow()->Render();
    }
    else{
      d->graphicsView->setRGBFrame(NULL);
    }
  }
}

