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


// PolyDataCompressedTransmission Logic includes
#include "vtkSlicerPolyDataCompressedTransmissionLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkMRMLIGTLConnectorNode.h"

// STD includes
#include <cassert>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPolyDataCompressedTransmissionLogic);


unsigned char red[3] = {255, 0, 0};
// creates the structures for rapidly getting the values to be associated to each depth value, and populates them.
void vtkSlicerPolyDataCompressedTransmissionLogic::prepareLookupTables()
{
  int lowBoundary = 600, highBoundary = 855;
  unsigned int span = (highBoundary - lowBoundary);
  lu = std::vector<int>(256,0);
  
  double scale_factor = ((double)span)/(double)255;
  
  for(unsigned int j=0; j<256; j++){
    lu[j] = (unsigned int)((double)j*scale_factor)+lowBoundary;
  }
}
vtkSmartPointer<vtkPolyData> vtkSlicerPolyDataCompressedTransmissionLogic::ConvertDepthToPoints(unsigned char* buf, unsigned char* bufColor, int depth_width_, int depth_height_) // now it is fixed to 512, 424
{
  ;//(depth_width_*depth_height_,vtkVector<float, 3>());
  
  bool isDepthOnly = false;
  cloud->Reset();
  colors->Reset();
  colors->SetNumberOfComponents(3);
  colors->SetName("Colors");
  //I inserted 525 as Julius Kammerl said as focal length
  register float constant = 0;
  
  if (isDepthOnly)
  {
    constant = 3.501e-3f;
  }
  else
  {
    constant = 1.83e-3f;
  }
  
  register int centerX = (depth_width_ >> 1);
  int centerY = (depth_height_ >> 1);
  
  //I also ignore invalid values completely
  float bad_point = std::numeric_limits<float>::quiet_NaN ();
  bool _useDemux = false;
  if(_useDemux)
  {
    std::vector<uint8_t> pBuf(depth_width_*depth_height_*4, 0);
    for (int i = 0; i< depth_width_*depth_height_*4 ; i++)
    {
      pBuf[i] = *(buf+i);
    }
    register int depth_idx = 0;
    std::vector<int> strides(4,0);
    int pointNum = 0;
    for (int v = -centerY; v < centerY; ++v)
    {
      for (register int u = -centerX; u < centerX; ++u, ++depth_idx)
      {
        strides[0] = depth_width_*2 * (v+centerY) + u + centerX;
        strides[1] = depth_width_*2 * (v+centerY) + depth_width_ + u + centerX;
        strides[2] = depth_width_*2 * (v+centerY) + depth_height_*depth_width_*2 + u + centerX;
        strides[3] = depth_width_*2 * (v+centerY) + depth_height_*depth_width_*2 + depth_width_ + u + centerX;
        for (int k = 0 ; k < 4; k++)
        {
          vtkVector<float, 3> pt;
          //This part is used for invalid measurements, I removed it
          int pixelValue = pBuf[strides[k]];
          if (pixelValue == 0 )
          {
            // not valid
            pt[0] = pt[1] = pt[2] = bad_point;
            continue;
          }
          pt[2] = pixelValue+k*255 + 500;
          pt[0] = static_cast<float> (u) * pt[2] * constant;
          pt[1] = static_cast<float> (v) * pt[2] * constant;
          cloud->InsertNextPoint(pt[0],pt[1],pt[2]);
          pointNum ++;
        }
      }
    }
    pBuf.clear();
  }
  else
  {
    std::vector<uint8_t> pBuf(depth_width_*depth_height_, 0);
    for (int i = 0; i< depth_width_*depth_height_; i++)
    {
      pBuf[i] = *(buf+i);
    }
    register int depth_idx = 0;
    int pointNum = 0;
    for (int v = -centerY; v < centerY; ++v)
    {
      for (register int u = -centerX; u < centerX; ++u, ++depth_idx)
      {
        vtkVector<float, 3> pt;
        //This part is used for invalid measurements, I removed it
        int pixelValue = pBuf[depth_idx];
        if (bufColor[3*depth_idx] == 0 && bufColor[3*depth_idx+1] == 0 && bufColor[3*depth_idx+2]==0 )
        {
          // not valid
          pt[0] = pt[1] = pt[2] = bad_point;
          continue;
        }
        pt[2] = pixelValue + 500;
        pt[0] = static_cast<float> (u) * pt[2] * constant;
        pt[1] = static_cast<float> (v) * pt[2] * constant;
        cloud->InsertNextPoint(pt[0],pt[1],pt[2]);
        unsigned char color[3] = {bufColor[3*depth_idx],bufColor[3*depth_idx+1],bufColor[3*depth_idx+2]};
        colors->InsertNextTupleValue(color);
        //delete[] color;
        pointNum ++;
      }
    }
    pBuf.clear();
  }
  int pointNum = cloud->GetNumberOfPoints();
  if (pointNum>0)
  {
    polyData->SetPoints(cloud);
    polyData->GetPointData()->SetScalars(colors);
    vertexFilter->SetInputData(polyData);
    vertexFilter->Update();
    polyData->ShallowCopy(vertexFilter->GetOutput());
  }
  return polyData;;
}

vtkSlicerPolyDataCompressedTransmissionLogic::vtkSlicerPolyDataCompressedTransmissionLogic()
{
  
  // Timer Handling
  this->DataCallbackCommand = vtkCallbackCommand::New();
  this->DataCallbackCommand->SetClientData( reinterpret_cast<void *> (this) );
  this->DataCallbackCommand->SetCallback(vtkSlicerPolyDataCompressedTransmissionLogic::DataCallback);
  
  this->Initialized   = 0;
  this->RestrictDeviceName = 0;
  
  this->MessageConverterList.clear();
  this->polyData = vtkSmartPointer<vtkPolyData>::New();
  // register default data types
  this->PolyConverter = vtkIGTLToMRMLDepthVideo::New();
  
  colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
  vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
  cloud = vtkSmartPointer<vtkPoints>::New();
  RegisterMessageConverter(this->PolyConverter);
  prepareLookupTables();
  
  //this->LocatorTransformNode = NULL;
}

//---------------------------------------------------------------------------
vtkSlicerPolyDataCompressedTransmissionLogic::~vtkSlicerPolyDataCompressedTransmissionLogic()
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
void vtkSlicerPolyDataCompressedTransmissionLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  os << indent << "vtkSlicerPolyDataCompressedTransmissionLogic:             " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataCompressedTransmissionLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> sceneEvents;
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  sceneEvents->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, sceneEvents.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataCompressedTransmissionLogic::RegisterNodes()
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
void vtkSlicerPolyDataCompressedTransmissionLogic::DataCallback(vtkObject *vtkNotUsed(caller),
                                               unsigned long vtkNotUsed(eid), void *clientData, void *vtkNotUsed(callData))
{
  vtkSlicerPolyDataCompressedTransmissionLogic *self = reinterpret_cast<vtkSlicerPolyDataCompressedTransmissionLogic *>(clientData);
  vtkDebugWithObjectMacro(self, "In vtkSlicerPolyDataCompressedTransmissionLogic DataCallback");
  self->UpdateAll();
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataCompressedTransmissionLogic::UpdateAll()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPolyDataCompressedTransmissionLogic::AddMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
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
void vtkSlicerPolyDataCompressedTransmissionLogic::RemoveMRMLConnectorNodeObserver(vtkMRMLIGTLConnectorNode * connectorNode)
{
  if (!connectorNode)
  {
    return;
  }
  vtkUnObserveMRMLNodeMacro(connectorNode);
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataCompressedTransmissionLogic::RegisterMessageConverters(vtkMRMLIGTLConnectorNode * connectorNode)
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
void vtkSlicerPolyDataCompressedTransmissionLogic::OnMRMLSceneEndImport()
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
void vtkSlicerPolyDataCompressedTransmissionLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  //vtkDebugMacro("vtkSlicerPolyDataCompressedTransmissionLogic::OnMRMLSceneNodeAdded");
  
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
void vtkSlicerPolyDataCompressedTransmissionLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  vtkMRMLIGTLConnectorNode * connectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (connectorNode)
  {
    this->RemoveMRMLConnectorNodeObserver(connectorNode);
  }
}

//---------------------------------------------------------------------------
vtkMRMLIGTLConnectorNode* vtkSlicerPolyDataCompressedTransmissionLogic::GetConnector(const char* conID)
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
vtkSmartPointer<vtkPolyData> vtkSlicerPolyDataCompressedTransmissionLogic::CallConnectorTimerHander()
{
  //ConnectorMapType::iterator cmiter;
  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLIGTLConnectorNode", nodes);
  
  std::vector<vtkMRMLNode*>::iterator iter;
  RGBFrame = NULL;
  DepthFrame = NULL;
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
    RGBFrame = connector->RGBFrame;
    DepthFrame = connector->DepthFrame;
  }
  if (DepthFrame && RGBFrame)
  {
    int64_t conversionTime = Connector::getTime();
    return ConvertDepthToPoints((unsigned char*)DepthFrame, RGBFrame, 512, 424);
    std::cerr<<"Depth Image conversion Time: "<<(Connector::getTime()-conversionTime)/1e6 << std::endl;
    
  }
  return NULL;
}


//---------------------------------------------------------------------------
int vtkSlicerPolyDataCompressedTransmissionLogic::SetRestrictDeviceName(int f)
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
int vtkSlicerPolyDataCompressedTransmissionLogic::RegisterMessageConverter(vtkIGTLToMRMLBase* converter)
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
int vtkSlicerPolyDataCompressedTransmissionLogic::UnregisterMessageConverter(vtkIGTLToMRMLBase* converter)
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
unsigned int vtkSlicerPolyDataCompressedTransmissionLogic::GetNumberOfConverters()
{
  return this->MessageConverterList.size();
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLBase* vtkSlicerPolyDataCompressedTransmissionLogic::GetConverter(unsigned int i)
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
vtkIGTLToMRMLBase* vtkSlicerPolyDataCompressedTransmissionLogic::GetConverterByMRMLTag(const char* mrmlTag)
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
vtkIGTLToMRMLBase* vtkSlicerPolyDataCompressedTransmissionLogic::GetConverterByDeviceType(const char* deviceType)
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
void vtkSlicerPolyDataCompressedTransmissionLogic::ProcessMRMLNodesEvents(vtkObject * caller, unsigned long event, void * callData)
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
void vtkSlicerPolyDataCompressedTransmissionLogic::ProcCommand(const char* vtkNotUsed(nodeName), int vtkNotUsed(size), unsigned char* vtkNotUsed(data))
{
}

//---------------------------------------------------------------------------
void vtkSlicerPolyDataCompressedTransmissionLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list)
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
void vtkSlicerPolyDataCompressedTransmissionLogic::GetDeviceNamesFromMrml(IGTLMrmlNodeListType &list, const char* mrmlTagName)
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