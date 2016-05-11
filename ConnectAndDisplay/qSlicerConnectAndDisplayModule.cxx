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
#include <QtPlugin>

// ConnectAndDisplay Logic includes
#include <vtkSlicerConnectAndDisplayLogic.h>

// ConnectAndDisplay includes
#include "qSlicerConnectAndDisplayModule.h"
#include "qSlicerConnectAndDisplayModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerConnectAndDisplayModule, qSlicerConnectAndDisplayModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerConnectAndDisplayModulePrivate
{
public:
  qSlicerConnectAndDisplayModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerConnectAndDisplayModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerConnectAndDisplayModulePrivate::qSlicerConnectAndDisplayModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerConnectAndDisplayModule methods

//-----------------------------------------------------------------------------
qSlicerConnectAndDisplayModule::qSlicerConnectAndDisplayModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerConnectAndDisplayModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerConnectAndDisplayModule::~qSlicerConnectAndDisplayModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerConnectAndDisplayModule::helpText() const
{
  return "This is a loadable module that can be bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerConnectAndDisplayModule::acknowledgementText() const
{
  return "This work was partially funded by NIH grant NXNNXXNNNNNN-NNXN";
}

//-----------------------------------------------------------------------------
QStringList qSlicerConnectAndDisplayModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (AnyWare Corp.)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerConnectAndDisplayModule::icon() const
{
  return QIcon(":/Icons/ConnectAndDisplay.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerConnectAndDisplayModule::categories() const
{
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerConnectAndDisplayModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerConnectAndDisplayModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerConnectAndDisplayModule
::createWidgetRepresentation()
{
  return new qSlicerConnectAndDisplayModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerConnectAndDisplayModule::createLogic()
{
  return vtkSlicerConnectAndDisplayLogic::New();
}
