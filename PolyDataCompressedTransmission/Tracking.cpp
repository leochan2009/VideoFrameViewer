#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/console/parse.h>
#include <pcl/common/time.h>
#include <pcl/common/centroid.h>

#include <pcl/visualization/cloud_viewer.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/io/pcd_io.h>

#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/approximate_voxel_grid.h>

#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>

#include <pcl/search/pcl_search.h>
#include <pcl/common/transforms.h>

#include <boost/format.hpp>

#include <pcl/tracking/tracking.h>
#include <pcl/tracking/particle_filter.h>
#include <pcl/tracking/kld_adaptive_particle_filter_omp.h>
#include <pcl/tracking/particle_filter_omp.h>
#include <pcl/tracking/coherence.h>
#include <pcl/tracking/distance_coherence.h>
#include <pcl/tracking/hsv_color_coherence.h>
#include <pcl/tracking/approx_nearest_pair_point_cloud_coherence.h>
#include <pcl/tracking/nearest_pair_point_cloud_coherence.h>

//VTK include
#include "vtkPointSource.h"
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkVertexGlyphFilter.h>

using namespace pcl::tracking;

typedef pcl::PointXYZRGB RefPointType;
typedef ParticleXYZRPY ParticleT;
typedef pcl::PointCloud<pcl::PointXYZRGB> Cloud;
typedef Cloud::Ptr CloudPtr;
typedef Cloud::ConstPtr CloudConstPtr;
typedef ParticleFilterTracker<RefPointType, ParticleT> ParticleFilter;

CloudPtr cloud_pass_;
CloudPtr cloud_pass_downsampled_;
CloudPtr target_cloud;
vtkSmartPointer<vtkUnsignedCharArray> colorsProcessed;
vtkSmartPointer<vtkVertexGlyphFilter> vertexFilter;
vtkSmartPointer<vtkPoints>  cloudProcessed;


boost::mutex mtx_;
boost::shared_ptr<ParticleFilter> tracker_;
bool new_cloud_;
double downsampling_grid_size_;
int counter;


//Filter along a specified dimension
void filterPassThrough (const CloudConstPtr &cloud, Cloud &result)
{
  pcl::PassThrough<pcl::PointXYZRGB> pass;
  pass.setFilterFieldName ("z");
  pass.setFilterLimits (1000.0, 2000.0);
  pass.setKeepOrganized (false);
  pass.setInputCloud (cloud);
  pass.filter (result);
}


void gridSampleApprox (const CloudConstPtr &cloud, Cloud &result, double leaf_size)
{
  pcl::ApproximateVoxelGrid<pcl::PointXYZRGB> grid;
  grid.setLeafSize (static_cast<float> (leaf_size), static_cast<float> (leaf_size), static_cast<float> (leaf_size));
  grid.setInputCloud (cloud);
  grid.filter (result);
}


//Draw model reference point cloud
void
drawResult ()
{
  ParticleXYZRPY result = tracker_->getResult ();
  Eigen::Affine3f transformation = tracker_->toEigenMatrix (result);
  
  //move close to camera a little for better visualization
  //transformation.translation () += Eigen::Vector3f (0.0f, 0.0f, -0.005f);
  CloudPtr result_cloud (new Cloud ());
  pcl::transformPointCloud<RefPointType> (*(tracker_->getReferenceCloud ()), *result_cloud, transformation);
  for(int index = 0; index<result_cloud->size();index++)
  {
    cloudProcessed->InsertNextPoint(result_cloud->at(index).data[0],result_cloud->at(index).data[1],result_cloud->at(index).data[2]);
    unsigned char color[3] = {0,0,255};
    colorsProcessed->InsertNextTupleValue(color);
  }
}

void trackingInitialization()
{
  //read pcd file
  vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
  cloudProcessed = vtkSmartPointer<vtkPoints>::New();
  colorsProcessed = vtkSmartPointer<vtkUnsignedCharArray>::New();
  target_cloud.reset(new Cloud());
  const char * targetPCDFile =  "/Users/longquanchen/TestCat.pcd";
  if(pcl::io::loadPCDFile (targetPCDFile, *target_cloud) == -1){
    std::cerr << "pcd file not found" << std::endl;
  }

  counter = 0;
  
  //Set parameters
  new_cloud_  = false;
  downsampling_grid_size_ =  0.002;
  
  std::vector<double> default_step_covariance = std::vector<double> (6, 0.015 * 0.015);
  default_step_covariance[3] *= 40.0;
  default_step_covariance[4] *= 40.0;
  default_step_covariance[5] *= 40.0;
  
  std::vector<double> initial_noise_covariance = std::vector<double> (6, 0.00001);
  std::vector<double> default_initial_mean = std::vector<double> (6, 0.0);
  
  boost::shared_ptr<KLDAdaptiveParticleFilterOMPTracker<RefPointType, ParticleT> > tracker
  (new KLDAdaptiveParticleFilterOMPTracker<RefPointType, ParticleT> (8));
  
  ParticleT bin_size;
  bin_size.x = 0.1f;
  bin_size.y = 0.1f;
  bin_size.z = 0.1f;
  bin_size.roll = 0.1f;
  bin_size.pitch = 0.1f;
  bin_size.yaw = 0.1f;
  
  
  //Set all parameters for  KLDAdaptiveParticleFilterOMPTracker
  tracker->setMaximumParticleNum (1000);
  tracker->setDelta (0.99);
  tracker->setEpsilon (0.2);
  tracker->setBinSize (bin_size);
  
  //Set all parameters for  ParticleFilter
  tracker_ = tracker;
  tracker_->setTrans (Eigen::Affine3f::Identity ());
  tracker_->setStepNoiseCovariance (default_step_covariance);
  tracker_->setInitialNoiseCovariance (initial_noise_covariance);
  tracker_->setInitialNoiseMean (default_initial_mean);
  tracker_->setIterationNum (1);
  tracker_->setParticleNum (600);
  tracker_->setResampleLikelihoodThr(0.00);
  tracker_->setUseNormal (false);
  
  
  //Setup coherence object for tracking
  ApproxNearestPairPointCloudCoherence<RefPointType>::Ptr coherence = ApproxNearestPairPointCloudCoherence<RefPointType>::Ptr
  (new ApproxNearestPairPointCloudCoherence<RefPointType> ());
  
  boost::shared_ptr<DistanceCoherence<RefPointType> > distance_coherence
  = boost::shared_ptr<DistanceCoherence<RefPointType> > (new DistanceCoherence<RefPointType> ());
  coherence->addPointCoherence (distance_coherence);
  
  boost::shared_ptr<pcl::search::Octree<RefPointType> > search (new pcl::search::Octree<RefPointType> (0.01));
  coherence->setSearchMethod (search);
  coherence->setMaximumDistance (0.01);
  
  tracker_->setCloudCoherence (coherence);
  
  //prepare the model of tracker's target
  Eigen::Vector4f c;
  Eigen::Affine3f trans = Eigen::Affine3f::Identity ();
  CloudPtr transed_ref (new Cloud);
  CloudPtr transed_ref_downsampled (new Cloud);
  
  pcl::compute3DCentroid<RefPointType> (*target_cloud, c);
  trans.translation ().matrix () = Eigen::Vector3f (c[0], c[1], c[2]);
  pcl::transformPointCloud<RefPointType> (*target_cloud, *transed_ref, trans.inverse());
  gridSampleApprox (transed_ref, *transed_ref_downsampled, downsampling_grid_size_);
  
  //set reference model and trans
  tracker_->setReferenceCloud (transed_ref_downsampled);
  tracker_->setTrans (trans);
  
}

int TrackObject (vtkSmartPointer<vtkPolyData> polydata/*const CloudConstPtr &cloud*/)
{
  boost::mutex::scoped_lock lock (mtx_);
  cloud_pass_.reset (new Cloud);
  cloud_pass_downsampled_.reset (new Cloud);
  CloudPtr cloud;
  vtkSmartPointer<vtkPoints> pts = polydata->GetPoints();
  vtkSmartPointer<vtkDataArray> colorData = polydata->GetPointData()->GetScalars();
  cloud.reset(new Cloud);
  for (int i = 0; i<pts->GetNumberOfPoints(); i++)
  {
    pcl::_PointXYZRGB temp;
    temp.data[0] = pts->GetPoint(i)[0],temp.data[1] = pts->GetPoint(i)[1],temp.data[2] = pts->GetPoint(i)[2];
    temp.r = colorData->GetTuple(i)[0]; temp.g = colorData->GetTuple(i)[1]; temp.b = colorData->GetTuple(i)[2];
    cloud->push_back(temp);
  }
  filterPassThrough (cloud, *cloud_pass_);
  gridSampleApprox (cloud_pass_, *cloud_pass_downsampled_, downsampling_grid_size_);
  
  //Track the object
  tracker_->setInputCloud (cloud_pass_downsampled_);
  tracker_->compute ();
  new_cloud_ = true;
  cloudProcessed->Reset();
  colorsProcessed->Reset();
  colorsProcessed->SetNumberOfComponents(3);
  colorsProcessed->SetName("Colors");
  for(int index = 0; index<cloud_pass_downsampled_->size();index++)
  {
    cloudProcessed->InsertNextPoint(cloud_pass_downsampled_->at(index).data[0],cloud_pass_downsampled_->at(index).data[1],cloud_pass_downsampled_->at(index).data[2]);
    unsigned char color[3] = {cloud_pass_downsampled_->at(index).r,cloud_pass_downsampled_->at(index).g,cloud_pass_downsampled_->at(index).b};
    colorsProcessed->InsertNextTupleValue(color);
  }
  drawResult();
  polydata->SetPoints(cloudProcessed);
  polydata->GetPointData()->SetScalars(colorsProcessed);
  vertexFilter->SetInputData(polydata);
  vertexFilter->Update();
  polydata->ShallowCopy(vertexFilter->GetOutput());
  return 1;
}


