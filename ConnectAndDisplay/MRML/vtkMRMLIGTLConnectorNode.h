/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLConnectorNode_h
#define __vtkMRMLIGTLConnectorNode_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkSlicerConnectAndDisplayModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlServerSocket.h>
#include <igtlClientSocket.h>
#include <igtlConditionVariable.h>

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkPolyData.h>

// STD includes
#include <string>
#include <map>
#include <vector>
#include <set>

class vtkMultiThreader;
class vtkMutexLock;
class vtkIGTLCircularBuffer;

namespace Connector{
  int64_t getTime();
}

class VTK_SLICER_CONNECTANDDISPLAY_MODULE_MRML_EXPORT vtkMRMLIGTLConnectorNode : public vtkMRMLNode
{

 public:

  //----------------------------------------------------------------
  // Constants Definitions
  //----------------------------------------------------------------

  // Events
  enum {
    ConnectedEvent        = 118944,
    DisconnectedEvent     = 118945,
    ActivatedEvent        = 118946,
    DeactivatedEvent      = 118947,
    ReceiveEvent          = 118948,
    NewDeviceEvent        = 118949,
    DeviceModifiedEvent   = 118950
  };

  enum {
    TYPE_NOT_DEFINED,
    TYPE_SERVER,
    TYPE_CLIENT,
    NUM_TYPE
  };

  static const char* ConnectorTypeStr[vtkMRMLIGTLConnectorNode::NUM_TYPE];

  enum {
    STATE_OFF,
    STATE_WAIT_CONNECTION,
    STATE_CONNECTED,
    NUM_STATE
  };

  static const char* ConnectorStateStr[vtkMRMLIGTLConnectorNode::NUM_STATE];
  
  enum {
    IO_UNSPECIFIED = 0x00,
    IO_INCOMING   = 0x01,
    IO_OUTGOING   = 0x02,
  };
  
  enum {
    PERSISTENT_OFF,
    PERSISTENT_ON,
  };

  typedef struct {
    std::string   name;
    std::string   type;
    int           io;
  } DeviceInfoType;

  typedef struct {
    int           lock;
    int           second;
    int           nanosecond;
  } NodeInfoType;

  typedef std::map<int, DeviceInfoType>   DeviceInfoMapType;   // Device list:  index is referred as
                                                               // a device id in the connector.
  typedef std::set<int>                   DeviceIDSetType;
  typedef std::list< vtkSmartPointer<vtkIGTLToMRMLBase> >   MessageConverterListType;
  typedef std::vector< vtkSmartPointer<vtkMRMLNode> >       MRMLNodeListType;
  typedef std::map<std::string, NodeInfoType>       NodeInfoMapType;
  typedef std::map<std::string, vtkSmartPointer <vtkIGTLToMRMLBase> > MessageConverterMapType;

 public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLIGTLConnectorNode *New();
  vtkTypeMacro(vtkMRMLIGTLConnectorNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName()
    {return "IGTLConnector";};

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData );

  //BTX
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference);

  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference);

  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference);
  //ETX

 protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLConnectorNode();
  ~vtkMRMLIGTLConnectorNode();
  vtkMRMLIGTLConnectorNode(const vtkMRMLIGTLConnectorNode&);
  void operator=(const vtkMRMLIGTLConnectorNode&);


 public:
  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------

  vtkGetMacro( ServerPort, int );
  vtkSetMacro( ServerPort, int );
  vtkGetMacro( Type, int );
  vtkSetMacro( Type, int );
  vtkGetMacro( State, int );
  vtkSetMacro( RestrictDeviceName, int );
  vtkGetMacro( RestrictDeviceName, int );
  vtkSetMacro( LogErrorIfServerConnectionFailed, bool);
  vtkGetMacro( LogErrorIfServerConnectionFailed, bool);

  // Controls if active connection will be resumed when
  // scene is loaded (cf: PERSISTENT_ON/_OFF)
  vtkSetMacro( Persistent, int );
  vtkGetMacro( Persistent, int );

  const char* GetServerHostname();
  void SetServerHostname(std::string str);

  int SetTypeServer(int port);
  int SetTypeClient(std::string hostname, int port);

  vtkGetMacro( CheckCRC, int );
  void SetCheckCRC(int c);


  //----------------------------------------------------------------
  // Thread Control
  //----------------------------------------------------------------

  int Start();
  int Stop();
  static void* ThreadFunction(void* ptr);

  //----------------------------------------------------------------
  // OpenIGTLink Message handlers
  //----------------------------------------------------------------
  int WaitForConnection();
  int ReceiveController();
  int SendData(int size, unsigned char* data);
  int Skip(int length, int skipFully=1);

  //----------------------------------------------------------------
  // Circular Buffer
  //----------------------------------------------------------------

  typedef std::vector<std::string> NameListType;
  unsigned int GetUpdatedBuffersList(NameListType& nameList); // TODO: this will be moved to private
  vtkIGTLCircularBuffer* GetCircularBuffer(std::string& key);     // TODO: Is it OK to use device name as a key?

  //----------------------------------------------------------------
  // Device Lists
  //----------------------------------------------------------------
  
  // Description:
  // Import received data from the circular buffer to the MRML scne.
  // This is currently called by vtkOpenIGTLinkIFLogic class.
  uint8_t* ImportDataFromCircularBuffer();

  // Description:
  // Import events from the event buffer to the MRML scene.
  // This is currently called by vtkOpenIGTLinkIFLogic class.
  void ImportEventsFromEventBuffer();

  // Description:
  // Push all outgoing messages to the network stream, if permitted.
  // This function is used, when the connection is established. To permit the OpenIGTLink IF
  // to push individual "outgoing" MRML nodes, set "OpenIGTLinkIF.pushOnConnection" attribute to 1. 
  void PushOutgoingMessages();

  // Description:
  // Register IGTL to MRML message converter.
  // This is used by vtkOpenIGTLinkIFLogic class.
  int RegisterMessageConverter(vtkIGTLToMRMLBase* converter);

  // Description:
  // Unregister IGTL to MRML message converter.
  // This is used by vtkOpenIGTLinkIFLogic class.
  void UnregisterMessageConverter(vtkIGTLToMRMLBase* converter);

  // Description:
  // Set and start observing MRML node for outgoing data.
  // If devType == NULL, a converter is chosen based only on MRML Tag.
  int RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType=NULL);

  // Description:
  // Stop observing and remove MRML node for outgoing data.
  void UnregisterOutgoingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Register MRML node for incoming data.
  // Returns a pointer to the node information in IncomingMRMLNodeInfoList
  NodeInfoType* RegisterIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Unregister MRML node for incoming data.
  void UnregisterIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfOutgoingMRMLNodes();

  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetOutgoingMRMLNode(unsigned int i);

  // Description:
  // Get MRML to IGTL converter assigned to the specified MRML node ID
  vtkIGTLToMRMLBase* GetConverterByNodeID(const char* id);

  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfIncomingMRMLNodes();

  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetIncomingMRMLNode(unsigned int i);

  // Description:
  // A function to explicitly push node to OpenIGTLink. The function is called either by 
  // external nodes or MRML event hander in the connector node. 
  int PushNode(vtkMRMLNode* node, int event=-1);

  // Description:
  // Push query int the query list.
  void PushQuery(vtkMRMLIGTLQueryNode* query);

  // Description:
  // Removes query from the query list.
  void CancelQuery(vtkMRMLIGTLQueryNode* node);

  //----------------------------------------------------------------
  // For OpenIGTLink time stamp access
  //----------------------------------------------------------------

  // Description:
  // Turn lock flag on to stop updating MRML node. Call this function before
  // reading the content of the MRML node and the corresponding time stamp.
  void LockIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Turn lock flag off to start updating MRML node. Make sure to call this function
  // after reading the content / time stamp.
  void UnlockIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Get OpenIGTLink's time stamp information. Returns 0, if it fails to obtain time stamp.
  int GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond);

  
  void setInterval (int timeInterva){interval = timeInterva;};
  void setUseCompress (int useComp){useCompress = useComp;};
  void setRequireConversion (int requireConver){requireConversion = requireConver;};
  
  uint8_t* RGBFrame;
  uint8_t* DepthFrame;
  uint8_t* DepthIndex;
  
  vtkSmartPointer<vtkPolyData> PolyData;

 protected:

  vtkIGTLToMRMLBase* GetConverterByMRMLTag(const char* tag);
  vtkIGTLToMRMLBase* GetConverterByIGTLDeviceType(const char* type);

  // Description:
  // Inserts the eventId to the EventQueue, and the event will be invoked from the main thread
  void RequestInvokeEvent(unsigned long eventId);

  // Description:
  // Reeust to push all outgoing messages to the network stream, if permitted.
  // This function is used, when the connection is established. To permit the OpenIGTLink IF
  // to push individual "outgoing" MRML nodes, set "OpenIGTLinkIF.pushOnConnection" attribute to 1. 
  // The request will be processed in PushOutgonigMessages().
  void RequestPushOutgoingMessages();
  
protected:

  //----------------------------------------------------------------
  // Reference role strings
  //----------------------------------------------------------------
  char* IncomingNodeReferenceRole;
  char* IncomingNodeReferenceMRMLAttributeName;

  char* OutgoingNodeReferenceRole;
  char* OutgoingNodeReferenceMRMLAttributeName;

  vtkSetStringMacro(IncomingNodeReferenceRole);
  vtkGetStringMacro(IncomingNodeReferenceRole);
  vtkSetStringMacro(IncomingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(IncomingNodeReferenceMRMLAttributeName);

  vtkSetStringMacro(OutgoingNodeReferenceRole);
  vtkGetStringMacro(OutgoingNodeReferenceRole);
  vtkSetStringMacro(OutgoingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(OutgoingNodeReferenceMRMLAttributeName);

  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------
  std::string Name;
  int Type;
  int State;
  int Persistent;
  bool LogErrorIfServerConnectionFailed;

  //----------------------------------------------------------------
  // Thread and Socket
  //----------------------------------------------------------------

  vtkMultiThreader* Thread;
  vtkMutexLock*     Mutex;
  igtl::ServerSocket::Pointer  ServerSocket;
  igtl::ClientSocket::Pointer  Socket;
  int               ThreadID;
  int               ServerPort;
  int               ServerStopFlag;

  std::string       ServerHostname;

  //----------------------------------------------------------------
  // Data
  //----------------------------------------------------------------


  typedef std::map<std::string, vtkIGTLCircularBuffer*> CircularBufferMap;
  CircularBufferMap Buffer;

  vtkMutexLock* CircularBufferMutex;
  int           RestrictDeviceName;  // Flag to restrict incoming and outgoing data by device names

  // Event queueing mechanism is needed to send all event notifications from the main thread.
  // Events can be pushed to the end of the EventQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  std::list<unsigned long> EventQueue;
  vtkMutexLock* EventQueueMutex;

  // Query queueing mechanism is needed to send all queries from the connector thread.
  // Queries can be pushed to the end of the QueryQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  // Use a weak pointer to make sure we don't try to access the query node after it is deleted from the scene.
  std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> > QueryWaitingQueue;
  vtkMutexLock* QueryQueueMutex;

  // Flag for the push outoing message request
  // If the flag is ON, the external timer will update the outgoing nodes with 
  // "OpenIGTLinkIF.pushOnConnection" attribute to push the nodes to the network.
  int PushOutgoingMessageFlag;
  vtkMutexLock* PushOutgoingMessageMutex;

  // -- Device Name (same as MRML node) and data type (data type string defined in OpenIGTLink)
  DeviceIDSetType   IncomingDeviceIDSet;
  DeviceIDSetType   OutgoingDeviceIDSet;
  DeviceIDSetType   UnspecifiedDeviceIDSet;

  // Message converter (IGTLToMRML)
  MessageConverterListType MessageConverterList;
  MessageConverterMapType  IGTLNameToConverterMap;
  MessageConverterMapType  MRMLIDToConverterMap;

  // List of nodes that this connector node is observing.
  //MRMLNodeListType         OutgoingMRMLNodeList;      // TODO: -> ReferenceList
  //NodeInfoListType         IncomingMRMLNodeInfoList;  // TODO: -> ReferenceList with attributes (.lock, .second, .nanosecond)
  NodeInfoMapType          IncomingMRMLNodeInfoMap;

  int CheckCRC;
  
  int interval;
  
  bool useCompress;
  
  bool requireConversion;
  
  bool conversionFinish;
  igtl::ConditionVariable::Pointer conditionVar;
  igtl::SimpleMutexLock *localMutex;

};

#endif

