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

#ifndef __qSlicerPolyDataCompressedTransmissionModuleWidget_h
#define __qSlicerPolyDataCompressedTransmissionModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// CTK includes
#include <ctkVTKObject.h>

// VTK includes
#include <vtkSmartPointer.h>
#include "vtkRenderer.h"
#include <vtkSmartPointer.h>
#include "vtkImageData.h"
#include "vtkImageActor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkImageActor.h"
#include "vtkInformation.h"
#include <vtkRenderWindow.h>


#include "qSlicerPolyDataCompressedTransmissionModuleExport.h"

class qSlicerPolyDataCompressedTransmissionModuleWidgetPrivate;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLIGTLQueryNode;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkObject;


/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_POLYDATACOMPRESSEDTRANSMISSION_EXPORT qSlicerPolyDataCompressedTransmissionModuleWidget :
public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPolyDataCompressedTransmissionModuleWidget(QWidget *parent=0);
  virtual ~qSlicerPolyDataCompressedTransmissionModuleWidget();
  
  public slots:
  /// Set the MRML node of interest
  void setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode * connectorNode);
  
  void setMRMLScene(vtkMRMLScene* scene);
  
  /// Utility function that calls setMRMLIGTLConnectorNode(vtkMRMLIGTLConnectorNode*)
  /// It's useful to connect to vtkMRMLNode* signals when you are sure of
  /// the type
  void setMRMLIGTLConnectorNode(vtkMRMLNode* node);
  
  void importDataAndEvents();
  
  
  protected slots:
  /// Internal function to update the widgets based on the IGTLConnector node
  void onMRMLNodeModified();
  
  void startVideoTransmission(bool value);
  
  void startCurrentIGTLConnector(bool enabled);
  
  /// Internal function to update the IGTLConnector node based on the property widget
  void updateIGTLConnectorNode();
  
protected:
  QScopedPointer<qSlicerPolyDataCompressedTransmissionModuleWidgetPrivate> d_ptr;
  
private:
  Q_DECLARE_PRIVATE(qSlicerPolyDataCompressedTransmissionModuleWidget);
  Q_DISABLE_COPY(qSlicerPolyDataCompressedTransmissionModuleWidget);
  
};

#endif
