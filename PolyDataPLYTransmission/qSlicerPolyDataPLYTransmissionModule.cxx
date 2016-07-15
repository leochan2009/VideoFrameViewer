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

// PolyDataPLYTransmission Logic includes
#include <vtkSlicerPolyDataPLYTransmissionLogic.h>

// PolyDataPLYTransmission includes
#include "qSlicerPolyDataPLYTransmissionModule.h"
#include "qSlicerPolyDataPLYTransmissionModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPolyDataPLYTransmissionModule, qSlicerPolyDataPLYTransmissionModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPolyDataPLYTransmissionModulePrivate
{
public:
  qSlicerPolyDataPLYTransmissionModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPolyDataPLYTransmissionModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPolyDataPLYTransmissionModulePrivate::qSlicerPolyDataPLYTransmissionModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPolyDataPLYTransmissionModule methods

//-----------------------------------------------------------------------------
qSlicerPolyDataPLYTransmissionModule::qSlicerPolyDataPLYTransmissionModule(QObject* _parent)
: Superclass(_parent)
, d_ptr(new qSlicerPolyDataPLYTransmissionModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPolyDataPLYTransmissionModule::~qSlicerPolyDataPLYTransmissionModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPolyDataPLYTransmissionModule::helpText() const
{
  return "This is a loadable module that can be bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerPolyDataPLYTransmissionModule::acknowledgementText() const
{
  return "This work was partially funded by NIH grant NXNNXXNNNNNN-NNXN";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPolyDataPLYTransmissionModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (AnyWare Corp.)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerPolyDataPLYTransmissionModule::icon() const
{
  return QIcon(":/Icons/PolyDataPLYTransmission.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPolyDataPLYTransmissionModule::categories() const
{
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPolyDataPLYTransmissionModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerPolyDataPLYTransmissionModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPolyDataPLYTransmissionModule
::createWidgetRepresentation()
{
  return new qSlicerPolyDataPLYTransmissionModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPolyDataPLYTransmissionModule::createLogic()
{
  return vtkSlicerPolyDataPLYTransmissionLogic::New();
}
