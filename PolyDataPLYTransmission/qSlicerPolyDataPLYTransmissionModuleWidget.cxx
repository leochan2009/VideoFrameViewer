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
#include "qSlicerPolyDataPLYTransmissionModuleWidget.h"
#include "ui_qSlicerPolyDataPLYTransmissionModuleWidget.h"

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qSlicerWidget.h"

// qMRMLWidget includes
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLSliderWidget.h"

// Logic include
#include "vtkSlicerPolyDataPLYTransmissionLogic.h"

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkIGTLToMRMLPolyData.h"
#include "vtkMRMLIGTLQueryNode.h"
#include <qMRMLNodeFactory.h>

//VTK include
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPolyDataPLYTransmissionModuleWidgetPrivate: public Ui_qSlicerPolyDataPLYTransmissionModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerPolyDataPLYTransmissionModuleWidget);
protected:
  qSlicerPolyDataPLYTransmissionModuleWidget* const q_ptr;
public:
  qSlicerPolyDataPLYTransmissionModuleWidgetPrivate(qSlicerPolyDataPLYTransmissionModuleWidget& object);
  
  vtkMRMLIGTLConnectorNode * IGTLConnectorNode;
  vtkMRMLIGTLQueryNode * IGTLDataQueryNode;
  vtkIGTLToMRMLPolyData* converter;
  QTimer ImportDataAndEventsTimer;
  vtkSlicerPolyDataPLYTransmissionLogic * logic();
  
  vtkRenderer* activeRenderer;
  vtkRenderer*   PolyDataRenderer;
  vtkSmartPointer<vtkActor> PolyDataActor;
  vtkSmartPointer<vtkPolyData> polydata;
  vtkSmartPointer<vtkPolyDataMapper> mapper;
};

//-----------------------------------------------------------------------------
// qSlicerPolyDataPLYTransmissionModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPolyDataPLYTransmissionModuleWidgetPrivate::qSlicerPolyDataPLYTransmissionModuleWidgetPrivate(qSlicerPolyDataPLYTransmissionModuleWidget& object):q_ptr(&object)
{
  this->IGTLConnectorNode = NULL;
  this->IGTLDataQueryNode = NULL;
}


//-----------------------------------------------------------------------------
vtkSlicerPolyDataPLYTransmissionLogic * qSlicerPolyDataPLYTransmissionModuleWidgetPrivate::logic()
{
  Q_Q(qSlicerPolyDataPLYTransmissionModuleWidget);
  return vtkSlicerPolyDataPLYTransmissionLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerPolyDataPLYTransmissionModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPolyDataPLYTransmissionModuleWidget::qSlicerPolyDataPLYTransmissionModuleWidget(QWidget* _parent)
: Superclass( _parent )
, d_ptr( new qSlicerPolyDataPLYTransmissionModuleWidgetPrivate(*this) )
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
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
  vtkRenderer* activeRenderer = app->layoutManager()->activeThreeDRenderer();
  d->PolyDataRenderer = activeRenderer;
  vtkRenderWindow* activeRenderWindow = activeRenderer->GetRenderWindow();
  d->polydata = vtkSmartPointer<vtkPolyData>::New();
  if (activeRenderer)
  {
    d->mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    d->mapper->SetInputData(d->polydata);
    d->PolyDataActor = vtkSmartPointer<vtkActor>::New();
    d->PolyDataActor->SetMapper(d->mapper);
    d->PolyDataRenderer->AddActor(d->PolyDataActor);
    activeRenderWindow->AddRenderer(d->PolyDataRenderer);
    activeRenderWindow->Render();
    activeRenderWindow->GetInteractor()->Start();
  }
}

//-----------------------------------------------------------------------------
qSlicerPolyDataPLYTransmissionModuleWidget::~qSlicerPolyDataPLYTransmissionModuleWidget()
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  d->IGTLConnectorNode->UnregisterMessageConverter(d->converter);
  d->converter->Delete();
}

void qSlicerPolyDataPLYTransmissionModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  this->Superclass::setMRMLScene(scene);
  if (this->mrmlScene())
  {
    d->IGTLConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(this->mrmlScene()->CreateNodeByClass("vtkMRMLIGTLConnectorNode"));
    d->IGTLDataQueryNode = vtkMRMLIGTLQueryNode::SafeDownCast(this->mrmlScene()->CreateNodeByClass("vtkMRMLIGTLQueryNode"));
    this->mrmlScene()->AddNode(d->IGTLConnectorNode); //node added cause the IGTLConnectorNode be initialized
    this->mrmlScene()->AddNode(d->IGTLDataQueryNode);
    d->converter = vtkIGTLToMRMLPolyData::New();
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
void qSlicerPolyDataPLYTransmissionModuleWidget::setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode * connectorNode)
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
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
void qSlicerPolyDataPLYTransmissionModuleWidget::setMRMLIGTLConnectorNode(vtkMRMLNode* node)
{
  this->setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode::SafeDownCast(node));
}

//------------------------------------------------------------------------------
void qSlicerPolyDataPLYTransmissionModuleWidget::onMRMLNodeModified()
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  if (!d->IGTLConnectorNode)
  {
    return;
  }
  
  d->ConnectorPortEdit->setText(QString("%1").arg(d->IGTLConnectorNode->GetServerPort()));
  
  bool deactivated = d->IGTLConnectorNode->GetState() == vtkMRMLIGTLConnectorNode::STATE_OFF;
  d->ConnectorStateCheckBox->setChecked(!deactivated);
}

//------------------------------------------------------------------------------
void qSlicerPolyDataPLYTransmissionModuleWidget::startCurrentIGTLConnector(bool value)
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  
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
void qSlicerPolyDataPLYTransmissionModuleWidget::startVideoTransmission(bool value)
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  Q_ASSERT(d->IGTLConnectorNode);
  
  if(d->StartVideoCheckBox->checkState() == Qt::CheckState::Checked)
  {
    if (d->FrameFrequency->text().toInt()>0.0000001 && d->FrameFrequency->text().toInt()<1000000)
    {
      int interval = (int) (1000.0 / d->FrameFrequency->text().toInt());
      d->IGTLDataQueryNode->SetIGTLName("POLYDATA");
      d->IGTLDataQueryNode->SetQueryType(vtkMRMLIGTLQueryNode::TYPE_GET);
      d->IGTLConnectorNode->setInterval(interval);
      d->IGTLDataQueryNode->SetQueryStatus(d->IGTLDataQueryNode->STATUS_PREPARED);
      d->IGTLConnectorNode->PushQuery(d->IGTLDataQueryNode);
    }
  }
  else
  {
    d->IGTLDataQueryNode->SetIGTLName("POLYDATA");
    d->IGTLDataQueryNode->SetQueryType(d->IGTLDataQueryNode->TYPE_STOP);
    d->IGTLDataQueryNode->SetQueryStatus(d->IGTLDataQueryNode->STATUS_PREPARED);
    d->IGTLConnectorNode->PushQuery(d->IGTLDataQueryNode);
  }
}

//------------------------------------------------------------------------------
void qSlicerPolyDataPLYTransmissionModuleWidget::updateIGTLConnectorNode()
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  if (d->IGTLConnectorNode != NULL)
  {
    d->IGTLConnectorNode->DisableModifiedEventOn();
    d->IGTLConnectorNode->SetServerHostname(d->ConnectorHostAddressEdit->text().toStdString());
    d->IGTLConnectorNode->SetServerPort(d->ConnectorPortEdit->text().toInt());
    d->IGTLConnectorNode->DisableModifiedEventOff();
    d->IGTLConnectorNode->InvokePendingModifiedEvent();
  }
}

void qSlicerPolyDataPLYTransmissionModuleWidget::importDataAndEvents()
{
  Q_D(qSlicerPolyDataPLYTransmissionModuleWidget);
  if (d->StartVideoCheckBox->checkState() == Qt::CheckState::Checked)
  {
    vtkMRMLAbstractLogic* l = this->logic();
    vtkSlicerPolyDataPLYTransmissionLogic * igtlLogic = vtkSlicerPolyDataPLYTransmissionLogic::SafeDownCast(l);
    if (igtlLogic)
    {
      int64_t startTime = Connector::getTime();
      d->polydata = igtlLogic->CallConnectorTimerHander();
      //-------------------
      // Convert the image in p_PixmapConversionBuffer to a QPixmap
      if (d->polydata)
      {
        int64_t renderingTime = Connector::getTime();
        d->mapper->SetInputData(d->polydata);
        d->PolyDataRenderer->GetRenderWindow()->Render();
        d->graphicsView->update();
        std::cerr<<"Rendering Time: "<<(Connector::getTime()-renderingTime)/1e6 << std::endl;
      }
    }
  }
}

