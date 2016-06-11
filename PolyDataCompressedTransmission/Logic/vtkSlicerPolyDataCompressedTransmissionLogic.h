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

// .NAME vtkSlicerPolyDataCompressedTransmissionLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPolyDataCompressedTransmissionLogic_h
#define __vtkSlicerPolyDataCompressedTransmissionLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes
#include "vtkIGTLToMRMLPolyData.h"
#include "vtkIGTLToMRMLBase.h"

// STD includes
#include <cstdlib>

// VTK includes
#include <vtkNew.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkVector.h>

#include "vtkSlicerPolyDataCompressedTransmissionModuleLogicExport.h"

class vtkMRMLIGTLConnectorNode;
/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_POLYDATACOMPRESSEDTRANSMISSION_MODULE_LOGIC_EXPORT vtkSlicerPolyDataCompressedTransmissionLogic :
  public vtkSlicerModuleLogic
{
public:
  
  enum {  // Events
    StatusUpdateEvent       = 50001,
    //SliceUpdateEvent        = 50002,
  };
  
  typedef struct {
    std::string name;
    std::string type;
    int io;
    std::string nodeID;
  } IGTLMrmlNodeInfoType;
  
  typedef std::vector<IGTLMrmlNodeInfoType>         IGTLMrmlNodeListType;
  typedef std::vector<vtkIGTLToMRMLBase*>           MessageConverterListType;
  
  // Work phase keywords used in NaviTrack (defined in BRPTPRInterface.h)
  
public:
  
  static vtkSlicerPolyDataCompressedTransmissionLogic *New();
  vtkTypeMacro(vtkSlicerPolyDataCompressedTransmissionLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream&, vtkIndent);
  
  /// The selected transform node is observed for TransformModified events and the transform
  /// data is copied to the slice nodes depending on the current mode
  
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);
  
  virtual void RegisterNodes();
  
  //----------------------------------------------------------------
  // Events
  //----------------------------------------------------------------
  
  virtual void OnMRMLSceneEndImport();
  
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* /*node*/);
  
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* /*node*/);
  
  virtual void OnMRMLNodeModified(vtkMRMLNode* /*node*/){}
  
  //----------------------------------------------------------------
  // Connector and converter Management
  //----------------------------------------------------------------
  
  // Access connectors
  vtkMRMLIGTLConnectorNode* GetConnector(const char* conID);
  
  // Call timer-driven routines for each connector
  vtkSmartPointer<vtkPolyData>  CallConnectorTimerHander();
  
  // Device Name management
  int  SetRestrictDeviceName(int f);
  
  int  RegisterMessageConverter(vtkIGTLToMRMLBase* converter);
  int  UnregisterMessageConverter(vtkIGTLToMRMLBase* converter);
  
  unsigned int       GetNumberOfConverters();
  vtkIGTLToMRMLBase* GetConverter(unsigned int i);
  vtkIGTLToMRMLBase* GetConverterByMRMLTag(const char* mrmlTag);
  vtkIGTLToMRMLBase* GetConverterByDeviceType(const char* deviceType);
  
  //----------------------------------------------------------------
  // MRML Management
  //----------------------------------------------------------------
  
  virtual void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void * callData);
  //virtual void ProcessLogicEvents(vtkObject * caller, unsigned long event, void * callData);
  
  void ProcCommand(const char* nodeName, int size, unsigned char* data);
  
  void GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list);
  void GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list, const char* mrmlTagName);
  //void GetDeviceTypes(std::vector<char*> &list);
  
protected:
  
  //----------------------------------------------------------------
  // Constructor, destructor etc.
  //----------------------------------------------------------------
  
  vtkSlicerPolyDataCompressedTransmissionLogic();
  virtual ~vtkSlicerPolyDataCompressedTransmissionLogic();
  
  static void DataCallback(vtkObject*, unsigned long, void *, void *);
  
  void AddMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode);
  void RemoveMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode);
  
  void RegisterMessageConverters(vtkMRMLIGTLConnectorNode * connectorNode);
  
  void UpdateAll();
  void UpdateSliceDisplay();
  void prepareLookupTables();
  vtkSmartPointer<vtkPoints> ConvertDepthToPoints(unsigned char* buf, int depth_width_, int depth_height_);
  vtkCallbackCommand *DataCallbackCommand;
  
private:
  
  int Initialized;
  
  vtkSmartPointer<vtkPolyData> polyData;
  //----------------------------------------------------------------
  // Connector Management
  //----------------------------------------------------------------
  
  //ConnectorMapType              ConnectorMap;
  MessageConverterListType      MessageConverterList;
  
  //int LastConnectorID;
  int RestrictDeviceName;
  
  //----------------------------------------------------------------
  // IGTL-MRML converters
  //----------------------------------------------------------------
  vtkIGTLToMRMLPolyData * PolyConverter;
  
  unsigned char * lu;
  vtkSmartPointer<vtkUnsignedCharArray> colors;
  
private:
  
  vtkSlicerPolyDataCompressedTransmissionLogic(const vtkSlicerPolyDataCompressedTransmissionLogic&); // Not implemented
  void operator=(const vtkSlicerPolyDataCompressedTransmissionLogic&);               // Not implemented
};

#endif
