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
// Slicer includes
#include <vtkObjectFactory.h>
#include <vtkSlicerColorLogic.h>


// PolyDataPLYTransmission Logic includes
#include "vtkSlicerPolyDataPLYTransmissionLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLIGTLConnectorNode.h"

// STD includes
#include <cassert>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPolyDataPLYTransmissionLogic);

vtkSlicerPolyDataPLYTransmissionLogic::vtkSlicerPolyDataPLYTransmissionLogic()
{
  
  // Timer Handling
  this->DataCallbackCommand = vtkCallbackCommand::New();
  this->DataCallbackCommand->SetClientData( reinterpret_cast<void *> (this) );
  this->DataCallbackCommand->SetCallback(vtkSlicerPolyDataPLYTransmissionLogic::DataCallback);
  
  this->Initialized   = 0;
  this->RestrictDeviceName = 0;
  
  this->MessageConverterList.clear();
  this->PolyData = vtkSmartPointer<vtkPolyData>::New();
  // register default data types
  this->PolyConverter = vtkIGTLToMRMLPolyData::New();
  
  RegisterMessageConverter(this->PolyConverter);
}

//---------------------------------------------------------------------------
vtkSlicerPolyDataPLYTransmissionLogic::~vtkSlicerPolyDataPLYTransmissionLogic()
{
  if (this->PolyConverter)
  {
    UnregisterMessageConverter(this->PolyConverter);
    this->PolyConverter->Delete();
  }
  
  if (this->DataCallbackCommand)
  {
    this->DataCallbackCommand->Delete();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  os << indent << "vtkSlicerPolyDataPLYTransmissionLogic:             " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> sceneEvents;
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, sceneEvents.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::RegisterNodes()
{
  vtkMRMLScene * scene = this->GetMRMLScene();
  if(!scene)
  {
    return;
  }
  
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLQueryNode>().GetPointer(), "vtkMRMLIGTLQueryNode");
  scene->RegisterNodeClass(vtkNew<vtkMRMLIGTLConnectorNode>().GetPointer(), "vtkMRMLIGTLConnectorNode");
  
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::DataCallback(vtkObject *vtkNotUsed(caller),
                                               unsigned long vtkNotUsed(eid), void *clientData, void *vtkNotUsed(callData))
{
  vtkSlicerPolyDataPLYTransmissionLogic *self = reinterpret_cast<vtkSlicerPolyDataPLYTransmissionLogic *>(clientData);
  vtkDebugWithObjectMacro(self, "In vtkSlicerPolyDataPLYTransmissionLogic DataCallback");
  self->UpdateAll();
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::UpdateAll()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::AddMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
  {
    return;
  }
  // Make sure we don't add duplicate observation
  vtkUnObserveMRMLNodeMacro(connectorNode);
  // Start observing the connector node
  vtkNew<vtkIntArray> connectorNodeEvents;
  connectorNodeEvents->InsertNextValue(vtkMRMLIGTLConnectorNode::DeviceModifiedEvent);
  vtkObserveMRMLNodeEventsMacro(connectorNode, connectorNodeEvents.GetPointer());
}

//----------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::RemoveMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
  {
    return;
  }
  vtkUnObserveMRMLNodeMacro(connectorNode);
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::RegisterMessageConverters(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
  {
    return;
  }
  for (unsigned short i = 0; i < this->GetNumberOfConverters(); i ++)
  {
    connectorNode->RegisterMessageConverter(this->GetConverter(i));
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::OnMRMLSceneEndImport()
{
  // Scene loading/import is finished, so now start the command processing thread
  // of all the active persistent connector nodes
  
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
  for (std::vector< vtkMRMLNode* >::iterator iter = nodes.begin(); iter != nodes.end(); ++iter)
  {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector == NULL)
    {
      continue;
    }
    if (connector->GetPersistent() == vtkMRMLIGTLConnectorNode::PERSISTENT_ON)
    {
      this->Modified();
      if (connector->GetState()!=vtkMRMLIGTLConnectorNode::STATE_OFF)
      {
        connector->Start();
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  //vtkDebugMacro("vtkSlicerPolyDataPLYTransmissionLogic::OnMRMLSceneNodeAdded");
  
  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
  {
    // TODO Remove this line when the corresponding UI option will be added
    connectorNode->SetRestrictDeviceName(0);
    
    this->AddMRMLConnectorNodeObserver(connectorNode);
    this->RegisterMessageConverters(connectorNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
  {
    this->RemoveMRMLConnectorNodeObserver(connectorNode);
  }
}

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkSlicerPolyDataPLYTransmissionLogic::GetConnector(const char* conID)
{
  vtkMRMLNode* node = this->GetMRMLScene()->GetNodeByID(conID);
  if (node)
  {
    vtkMRMLIGTLConnectorNode* conNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
    return conNode;
  }
  else
  {
    return NULL;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::CallConnectorTimerHander()
{
  //ConnectorMapType::iterator cmiter;
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
  
  std::vector<vtkMRMLNode*>::iterator iter;
  //for (cmiter = this->ConnectorMap.begin(); cmiter != this->ConnectorMap.end(); cmiter ++)
  for (iter = nodes.begin(); iter != nodes.end(); iter ++)
  {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector == NULL)
    {
      continue;
    }
    connector->ImportDataFromCircularBuffer();
    connector->ImportEventsFromEventBuffer();
    connector->PushOutgoingMessages();
    PolyData = connector->PolyData;
  }
}

//---------------------------------------------------------------------------
int vtkSlicerPolyDataPLYTransmissionLogic::SetRestrictDeviceName(int f)
{
  if (f != 0) f = 1; // make sure that f is either 0 or 1.
  this->RestrictDeviceName = f;
  
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
  
  std::vector<vtkMRMLNode*>::iterator iter;
  
  for (iter = nodes.begin(); iter != nodes.end(); iter ++)
  {
    vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
    if (connector)
    {
      connector->SetRestrictDeviceName(f);
    }
  }
  return this->RestrictDeviceName;
}

//---------------------------------------------------------------------------
int vtkSlicerPolyDataPLYTransmissionLogic::RegisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  if (converter == NULL)
  {
    return 0;
  }
  
  // Search the list and check if the same converter has already been registered.
  int found = 0;
  
  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
  {
    if ((converter->GetIGTLName() && strcmp(converter->GetIGTLName(), (*iter)->GetIGTLName()) == 0) &&
        (converter->GetMRMLName() && strcmp(converter->GetMRMLName(), (*iter)->GetMRMLName()) == 0))
    {
      found = 1;
    }
  }
  if (found)
  {
    return 0;
  }
  
  if (converter->GetIGTLName() && converter->GetMRMLName())
    // TODO: is this correct? Shouldn't it be "&&"
  {
    this->MessageConverterList.push_back(converter);
  }
  else
  {
    return 0;
  }
  
  // Add the converter to the existing connectors
  if (this->GetMRMLScene())
  {
    std::vector<vtkMRMLNode*> nodes;
    this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
    
    std::vector<vtkMRMLNode*>::iterator niter;
    for (niter = nodes.begin(); niter != nodes.end(); niter ++)
    {
      vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*niter);
      if (connector)
      {
        connector->RegisterMessageConverter(converter);
      }
    }
  }
  
  return 1;
}


//---------------------------------------------------------------------------
int vtkSlicerPolyDataPLYTransmissionLogic::UnregisterMessageConverter(vtkIGTLToMRMLBase* converter)
{
  if (converter == NULL)
  {
    return 0;
  }
  
  // Look up the message converter list
  MessageConverterListType::iterator iter;
  iter = this->MessageConverterList.begin();
  while ((*iter) != converter) iter ++;
  
  if (iter != this->MessageConverterList.end())
    // if the converter is on the list
  {
    this->MessageConverterList.erase(iter);
    // Remove the converter from the existing connectors
    std::vector<vtkMRMLNode*> nodes;
    if (this->GetMRMLScene())
    {
      this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
      
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
      {
        vtkMRMLIGTLConnectorNode* connector = vtkMRMLIGTLConnectorNode::SafeDownCast(*iter);
        if (connector)
        {
          connector->UnregisterMessageConverter(converter);
        }
      }
    }
    return 1;
  }
  else
  {
    return 0;
  }
}

//---------------------------------------------------------------------------
unsigned int vtkSlicerPolyDataPLYTransmissionLogic::GetNumberOfConverters()
{
  return this->MessageConverterList.size();
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerPolyDataPLYTransmissionLogic::GetConverter(unsigned int i)
{
  
  if (i < this->MessageConverterList.size())
  {
    return this->MessageConverterList[i];
  }
  else
  {
    return NULL;
  }
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerPolyDataPLYTransmissionLogic::GetConverterByMRMLTag(const char* mrmlTag)
{
  //Currently, this function cannot find multiple converters
  // that use the same mrmlType (e.g. vtkIGTLToMRMLLinearTransform
  // and vtkIGTLToMRMLPosition). A converter that is found first
  // will be returned.
  
  vtkIGTLToMRMLBase* converter = NULL;
  
  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
  {
    if (strcmp((*iter)->GetMRMLName(), mrmlTag) == 0)
    {
      converter = *iter;
      break;
    }
  }
  
  return converter;
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerPolyDataPLYTransmissionLogic::GetConverterByDeviceType(const char* deviceType)
{
  vtkIGTLToMRMLBase* converter = NULL;
  
  MessageConverterListType::iterator iter;
  for (iter = this->MessageConverterList.begin();
       iter != this->MessageConverterList.end();
       iter ++)
  {
    if ((*iter)->GetConverterType() == vtkIGTLToMRMLBase::TYPE_NORMAL)
    {
      if (strcmp((*iter)->GetIGTLName(), deviceType) == 0)
      {
        converter = *iter;
        break;
      }
    }
    else
    {
      int n = (*iter)->GetNumberOfIGTLNames();
      for (int i = 0; i < n; i ++)
      {
        if (strcmp((*iter)->GetIGTLName(i), deviceType) == 0)
        {
          converter = *iter;
          break;
        }
      }
    }
  }
  
  return converter;
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::ProcessMRMLNodesEvents(vtkObject * caller, unsigned long event, void * callData)
{
  
  if (caller != NULL)
  {
    vtkSlicerModuleLogic::ProcessMRMLNodesEvents(caller, event, callData);
    
    vtkMRMLIGTLConnectorNode* cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(caller);
    if (cnode && event == vtkMRMLIGTLConnectorNode::DeviceModifiedEvent)
    {
      // Check visibility
      int nnodes;
      
      // Incoming nodes
      nnodes = cnode->GetNumberOfIncomingMRMLNodes();
      for (int i = 0; i < nnodes; i ++)
      {
        vtkMRMLNode* inode = cnode->GetIncomingMRMLNode(i);
        if (inode)
        {
          vtkIGTLToMRMLBase* converter = GetConverterByMRMLTag(inode->GetNodeTagName());
          if (converter)
          {
            const char * attr = inode->GetAttribute("IGTLVisible");
            if (attr && strcmp(attr, "true") == 0)
            {
              converter->SetVisibility(1, this->GetMRMLScene(), inode);
            }
            else
            {
              converter->SetVisibility(0, this->GetMRMLScene(), inode);
            }
          }
        }
      }
      
      // Outgoing nodes
      nnodes = cnode->GetNumberOfOutgoingMRMLNodes();
      for (int i = 0; i < nnodes; i ++)
      {
        vtkMRMLNode* inode = cnode->GetOutgoingMRMLNode(i);
        if (inode)
        {
          vtkIGTLToMRMLBase* converter = GetConverterByMRMLTag(inode->GetNodeTagName());
          if (converter)
          {
            const char * attr = inode->GetAttribute("IGTLVisible");
            if (attr && strcmp(attr, "true") == 0)
            {
              converter->SetVisibility(1, this->GetMRMLScene(), inode);
            }
            else
            {
              converter->SetVisibility(0, this->GetMRMLScene(), inode);
            }
          }
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::ProcCommand(const char* vtkNotUsed(nodeName), int vtkNotUsed(size), unsigned char* vtkNotUsed(data))
{
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list)
{
  list.clear();
  
  MessageConverterListType::iterator mcliter;
  for (mcliter = this->MessageConverterList.begin();
       mcliter != this->MessageConverterList.end();
       mcliter ++)
  {
    if ((*mcliter)->GetMRMLName())
    {
      std::string className = this->GetMRMLScene()->GetClassNameByTag((*mcliter)->GetMRMLName());
      std::string deviceTypeName;
      if ((*mcliter)->GetIGTLName() != NULL)
      {
        deviceTypeName = (*mcliter)->GetIGTLName();
      }
      else
      {
        deviceTypeName = (*mcliter)->GetMRMLName();
      }
      std::vector<vtkMRMLNode*> nodes;
      this->GetApplicationLogic()->GetMRMLScene()->GetNodesByClass(className.c_str(), nodes);
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
      {
        IGTLMrmlNodeInfoType nodeInfo;
        nodeInfo.name   = (*iter)->GetName();
        nodeInfo.type   = deviceTypeName.c_str();
        nodeInfo.io     = vtkMRMLIGTLConnectorNode::IO_UNSPECIFIED;
        nodeInfo.nodeID = (*iter)->GetID();
        list.push_back(nodeInfo);
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataPLYTransmissionLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list, const char* mrmlTagName)
{
  list.clear();
  
  MessageConverterListType::iterator mcliter;
  for (mcliter = this->MessageConverterList.begin();
       mcliter != this->MessageConverterList.end();
       mcliter ++)
  {
    if ((*mcliter)->GetMRMLName() && strcmp(mrmlTagName, (*mcliter)->GetMRMLName()) == 0)
    {
      const char* className = this->GetMRMLScene()->GetClassNameByTag(mrmlTagName);
      const char* deviceTypeName = (*mcliter)->GetIGTLName();
      std::vector<vtkMRMLNode*> nodes;
      this->GetMRMLScene()->GetNodesByClass(className, nodes);
      std::vector<vtkMRMLNode*>::iterator iter;
      for (iter = nodes.begin(); iter != nodes.end(); iter ++)
      {
        IGTLMrmlNodeInfoType nodeInfo;
        nodeInfo.name = (*iter)->GetName();
        nodeInfo.type = deviceTypeName;
        nodeInfo.io   = vtkMRMLIGTLConnectorNode::IO_UNSPECIFIED;
        nodeInfo.nodeID = (*iter)->GetID();
        list.push_back(nodeInfo);
      }
    }
  }
}