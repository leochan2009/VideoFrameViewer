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

// PolyDataCompressedTransmission Logic includes
#include <vtkSlicerPolyDataCompressedTransmissionLogic.h>

// PolyDataCompressedTransmission includes
#include "qSlicerPolyDataCompressedTransmissionModule.h"
#include "qSlicerPolyDataCompressedTransmissionModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPolyDataCompressedTransmissionModule, qSlicerPolyDataCompressedTransmissionModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPolyDataCompressedTransmissionModulePrivate
{
public:
  qSlicerPolyDataCompressedTransmissionModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPolyDataCompressedTransmissionModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPolyDataCompressedTransmissionModulePrivate::qSlicerPolyDataCompressedTransmissionModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPolyDataCompressedTransmissionModule methods

//-----------------------------------------------------------------------------
qSlicerPolyDataCompressedTransmissionModule::qSlicerPolyDataCompressedTransmissionModule(QObject* _parent)
: Superclass(_parent)
, d_ptr(new qSlicerPolyDataCompressedTransmissionModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPolyDataCompressedTransmissionModule::~qSlicerPolyDataCompressedTransmissionModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPolyDataCompressedTransmissionModule::helpText() const
{
  return "This is a loadable module that can be bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerPolyDataCompressedTransmissionModule::acknowledgementText() const
{
  return "This work was partially funded by NIH grant NXNNXXNNNNNN-NNXN";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPolyDataCompressedTransmissionModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (AnyWare Corp.)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerPolyDataCompressedTransmissionModule::icon() const
{
  return QIcon(":/Icons/PolyDataCompressedTransmission.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPolyDataCompressedTransmissionModule::categories() const
{
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPolyDataCompressedTransmissionModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerPolyDataCompressedTransmissionModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPolyDataCompressedTransmissionModule
::createWidgetRepresentation()
{
  return new qSlicerPolyDataCompressedTransmissionModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPolyDataCompressedTransmissionModule::createLogic()
{
  return vtkSlicerPolyDataCompressedTransmissionLogic::New();
}
