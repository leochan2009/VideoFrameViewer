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

#ifndef __qSlicerConnectAndDisplayModuleWidget_h
#define __qSlicerConnectAndDisplayModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// CTK includes
#include <ctkVTKObject.h>

#include "qSlicerConnectAndDisplayModuleExport.h"

class qSlicerConnectAndDisplayModuleWidgetPrivate;
class vtkMRMLIGTLConnectorNode;
class vtkMRMLIGTLQueryNode;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkObject;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_CONNECTANDDISPLAY_EXPORT qSlicerConnectAndDisplayModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT
public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerConnectAndDisplayModuleWidget(QWidget *parent=0);
  virtual ~qSlicerConnectAndDisplayModuleWidget();

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
  QScopedPointer<qSlicerConnectAndDisplayModuleWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerConnectAndDisplayModuleWidget);
  Q_DISABLE_COPY(qSlicerConnectAndDisplayModuleWidget);
};

#endif
