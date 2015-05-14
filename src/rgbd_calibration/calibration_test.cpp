/*
 *  Copyright (C) 2013 - Filippo Basso <bassofil@dei.unipd.it>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <rgbd_calibration/calibration_test.h>
#include <rgbd_calibration/checkerboard_views_extractor.h>

#include <calibration_common/base/pcl_conversion.h>

#include <pcl/filters/random_sample.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/passthrough.h>

#include <pcl/visualization/point_cloud_color_handlers.h>

#include <ceres/ceres.h>
#include <ceres/rotation.h>

namespace calibration
{

void CalibrationTest::publishData() const
{
  if (not publisher_)
    return;

  publisher_->publishTF(*depth_sensor_);
  publisher_->publishTF(*color_sensor_);

  for (size_t i = 0; i < data_vec_.size(); i += 1)
    publisher_->publish(*data_vec_[i]);

  for (size_t i = 0; i < und_data_vec_.size(); i += 1)
    publisher_->publish(*und_data_vec_[i]);

}

void CalibrationTest::addData(const cv::Mat & image,
                              const PCLCloud3::ConstPtr & cloud)
{
//  PCLCloud3::Ptr new_cloud = boost::make_shared<PCLCloud3>();
//  std::vector<int> remapping;
//  pcl::removeNaNFromPointCloud(*cloud, *new_cloud, remapping);

//  if (ratio_ > 1)
//  {
//    pcl::RandomSample<PCLPoint3> random_sample;
//    random_sample.setInputCloud(new_cloud);
//    random_sample.setSample(new_cloud->size() / ratio_);
//    random_sample.setSeed(rand());
//    random_sample.filter(*new_cloud);
//  }

  int index = data_vec_.size() + 1;

  cv::Mat rectified;
  color_sensor_->cameraModel()->rectifyImage(image, rectified);

  RGBDData::Ptr data(boost::make_shared<RGBDData>(index));
  data->setColorSensor(color_sensor_);
  data->setDepthSensor(depth_sensor_);
  data->setColorData(rectified);
  data->setDepthData(*cloud);

  PCLCloud3::Ptr part_und_cloud = boost::make_shared<PCLCloud3>(*cloud);
  local_matrix_->undistort(*part_und_cloud);

  RGBDData::Ptr part_und_data(boost::make_shared<RGBDData>(index));
  part_und_data->setColorSensor(color_sensor_);
  part_und_data->setDepthSensor(depth_sensor_);
  part_und_data->setColorData(rectified);
  part_und_data->setDepthData(*part_und_cloud);

  PCLCloud3::Ptr und_cloud = boost::make_shared<PCLCloud3>(*part_und_cloud);
  global_matrix_->undistort(*und_cloud);

  RGBDData::Ptr und_data(boost::make_shared<RGBDData>(index + 1000));
  und_data->setColorSensor(color_sensor_);
  und_data->setDepthSensor(depth_sensor_);
  und_data->setColorData(rectified);
  und_data->setDepthData(*und_cloud);

  data_vec_.push_back(data);
  part_und_data_vec_.push_back(part_und_data);
  und_data_vec_.push_back(und_data);

  part_data_map_[und_data] = part_und_data;
  data_map_[und_data] = data;


  //  float z = 0.0f;
  //  for (size_t i = 0; i < und_cloud->size(); ++i)
  //    z += und_cloud->points[i].z;
  //  z /= und_cloud->size();

  //  PCLCloud::Ptr tmp_cloud = boost::make_shared<PCLCloud>();
  //  pcl::PassThrough<pcl::PointXYZ> pass;
  //  pass.setInputCloud(new_cloud);
  //  pass.setFilterFieldName("y");
  //  pass.setFilterLimits(-3.0f, 1.0f);
  //  pass.filter(*tmp_cloud);
  //
  //  pcl::VoxelGrid<pcl::PointXYZ> voxel;
  //  voxel.setInputCloud(tmp_cloud);
  //  voxel.setLeafSize(0.05f, 10.0f, 0.05f);
  //  voxel.filter(*tmp_cloud);
  //
  //  std::fstream fs;
  //  fs.open("/tmp/points.txt", std::fstream::out | std::fstream::app);
  //  for (size_t i = 0; i < tmp_cloud->size(); ++i)
  //    fs << tmp_cloud->points[i].x << " " << tmp_cloud->points[i].z << " " << tmp_cloud->points[i].y << std::endl;
  //  fs << "------------------------------------------------------------------" << std::endl;
  //  fs.close();
  //
  //  pass.setInputCloud(und_cloud);
  //  pass.filter(*tmp_cloud);
  //  voxel.setInputCloud(tmp_cloud);
  //  voxel.filter(*tmp_cloud);
  //
  //  fs.open("/tmp/und_points.txt", std::fstream::out | std::fstream::app);
  //  for (size_t i = 0; i < tmp_cloud->size(); ++i)
  //    fs << tmp_cloud->points[i].x << " " << tmp_cloud->points[i].z << " " << tmp_cloud->points[i].y << std::endl;
  //  fs << "------------------------------------------------------------------" << std::endl;
  //  fs.close();

}

void CalibrationTest::visualizeClouds() const
{

  pcl::visualization::PCLVisualizer viz1("Test Set Visualization");

  PCLCloud3::Ptr fake_cloud = boost::make_shared<PCLCloud3>(640, 480);
  const PinholeCameraModel & camera_model = *color_sensor_->cameraModel();

  pcl::visualization::PointCloudColorHandlerGenericField<PCLPoint3> color_handler(fake_cloud, "z");
  //viz1.addPointCloud(fake_cloud, color_handler);

  ros::Rate rate = ros::Rate(100.0);
  for (float s = 1.0f; s <= 4.5f; s += 1.5f)
  {

    for (size_t i = 0; i < fake_cloud->width; ++i)
    {
      for (size_t j = 0; j < fake_cloud->height; ++j)
      {
        PCLPoint3 & fake_p = fake_cloud->at(i, j);
        fake_p.x = (i - camera_model.cx()) * s / camera_model.fx();
        fake_p.y = (j - camera_model.cy()) * s / camera_model.fy();
        fake_p.z = s;
      }
    }

    local_matrix_->undistort(*fake_cloud);
    //global_matrix_->undistort(*fake_cloud);

    std::stringstream ss;
    ss << "cloud_" << s;
    viz1.addPointCloud(fake_cloud, color_handler, ss.str());

    ss.str("");
    ss << s << " m";

    PCLPoint3 p = fake_cloud->points[280];
    p.y -= 0.05f;
    viz1.addText3D(ss.str(), p, 0.1, 1.0, 1.0, 1.0, ss.str());

    viz1.spinOnce(10, true);
    rate.sleep();
  }

  int ORIGINAL = 0, UNDISTORTED = 1, FINAL = 2;

  pcl::visualization::PCLVisualizer viz("Test Set Visualization");
  viz.createViewPort(0.0, 0.0, 0.33, 1.0, ORIGINAL);
  viz.createViewPort(0.33, 0.0, 0.67, 1.0, UNDISTORTED);
  viz.createViewPort(0.67, 0.0, 1.0, 1.0, FINAL);
  viz.addCoordinateSystem();

  std::vector<double> plane_distances(12);
  plane_distances[0] = 1.00134;
  plane_distances[1] = 1.20495;
  plane_distances[2] = 1.53675;
  plane_distances[3] = 1.86759;
  plane_distances[4] = 2.21074;
  plane_distances[5] = 2.54476;
  plane_distances[6] = 2.89025;
  plane_distances[7] = 3.23017;
  plane_distances[8] = 3.56103;
  plane_distances[9] = 3.89967;
  plane_distances[10] = 4.21813;
  plane_distances[11] = 4.57653;

  for (size_t index = 1; index < data_vec_.size(); index += 4)
  {
    std::stringstream ss;
    ss << "cloud_" << index;
    const RGBDData::ConstPtr & data = data_vec_[index];
    const RGBDData::ConstPtr & part_und_data = part_und_data_vec_[index];
    const RGBDData::ConstPtr & und_data = und_data_vec_[index];

    data->fuseData();
    part_und_data->fuseData();
    und_data->fuseData();
    viz.addPointCloud(data->fusedData(), ss.str(), ORIGINAL);
    viz.addPointCloud(part_und_data->fusedData(), ss.str() + "_und", UNDISTORTED);
    viz.addPointCloud(und_data->fusedData(), ss.str() + "_final", FINAL);

    pcl::ModelCoefficients coeffs;
    coeffs.values.resize(4);
    coeffs.values[0] = 0.00582574;
    coeffs.values[1] = 0.0834788;
    coeffs.values[2] = 0.996493;
    coeffs.values[3] = -plane_distances[index];

    ss.str("");
    ss << "plane_" << index;
    viz.addPlane(coeffs, ss.str());
    viz.setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_COLOR, 1.0, 0.1, 0.1, ss.str());
    viz.setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_OPACITY, 0.5, ss.str());

  }

  viz.setBackgroundColor(0.08, 0.08, 0.08, UNDISTORTED);

  viz.addText("TEST SET - VISUALIZATION", 20, 1010, 30, 1.0, 1.0, 1.0, "test_set_text", ORIGINAL);
  viz.addText("ORIGINAL CLOUDS", 20, 20, 30, 1.0, 1.0, 1.0, "original_text", ORIGINAL);
  viz.addText("UNDISTORTED CLOUDS", 20, 20, 30, 1.0, 1.0, 1.0, "undistorted_text", UNDISTORTED);
  viz.addText("FINAL CLOUDS", 20, 20, 30, 1.0, 1.0, 1.0, "final_text", FINAL);

  /*ros::Rate rate = ros::Rate(100.0);
  for (int i = 0; i < 500; ++i)
  {
    viz.spinOnce(10);
    rate.sleep();
  }

  for (size_t index = 1; index < data_vec_.size(); index += 3)
  {
    PCLCloud3::Ptr tmp_cloud = boost::make_shared<PCLCloud3>(*data_vec_[index]->depthData());

    for (float s = 0.0f; s <= 1.0f; s += 0.02f)
    {

      for (size_t i = 0; i < tmp_cloud->size(); ++i)
      {
        PCLPoint3 & tmp_p = tmp_cloud->points[i];
        const PCLPoint3 & p = data_vec_[index]->depthData()->points[i];
        const PCLPoint3 & und_p = und_data_vec_[index]->depthData()->points[i];

        tmp_p.x = und_p.x * s + p.x * (1.0f - s);
        tmp_p.y = und_p.y * s + p.y * (1.0f - s);
        tmp_p.z = und_p.z * s + p.z * (1.0f - s);
      }

      RGBDData::Ptr tmp_data(boost::make_shared<RGBDData>(index));
      tmp_data->setColorSensor(color_sensor_);
      tmp_data->setDepthSensor(depth_sensor_);
      tmp_data->setColorData(data_vec_[index]->colorData());
      tmp_data->setDepthData(*tmp_cloud);

      std::stringstream ss;
      ss << "Cloud_" << index << "_und";

      tmp_data->fuseData();
      viz.updatePointCloud(tmp_data->fusedData(), ss.str());
      viz.spinOnce(10, true);
      rate.sleep();
    }
  }*/
  /*PCLCloud3::Ptr tmp_cloud = boost::make_shared<PCLCloud3>(*cloud);

  for (float s = 0.0f; s <= 1.0f; s += 0.02f)
  {

    for (size_t i = 0; i < tmp_cloud->size(); ++i)
    {
      PCLPoint3 & tmp_p = tmp_cloud->points[i];
      const PCLPoint3 & p = cloud->points[i];
      const PCLPoint3 & und_p = und_cloud->points[i];

      tmp_p.x = und_p.x * s + p.x * (1.0f - s);
      tmp_p.y = und_p.y * s + p.y * (1.0f - s);
      tmp_p.z = und_p.z * s + p.z * (1.0f - s);
    }

    RGBDData::Ptr tmp_data(boost::make_shared<RGBDData>(index));
    tmp_data->setColorSensor(color_sensor_);
    tmp_data->setDepthSensor(depth_sensor_);
    tmp_data->setColorData(rectified);
    tmp_data->setDepthData(*tmp_cloud);

    tmp_data->fuseData();
    viz.updatePointCloud(tmp_data->fusedData(), "Cloud");
    viz.spinOnce(10, true);
    rate.sleep();

  }*/

  viz.spin();
}

void CalibrationTest::testPlanarityError() const
{
  std::vector<CheckerboardViews::Ptr> cb_views_vec;

  CheckerboardViewsExtraction cb_extractor;
  cb_extractor.setColorSensorPose(color_sensor_->pose());
  cb_extractor.setCheckerboardVector(cb_vec_);
  cb_extractor.setInputData(und_data_vec_);
  cb_extractor.extractAll(cb_views_vec);

  for (size_t i = 0; i < cb_views_vec.size(); ++i)
  {
    const CheckerboardViews & cb_views = *cb_views_vec[i];
    RGBDData::ConstPtr und_data = cb_views.data();
    RGBDData::ConstPtr data = data_map_.at(und_data);

    const Cloud3 & und_points = PCLConversion<Scalar>::toPointMatrix(*und_data->depthData()/*, *cb_views.planeInliers()*/);
    Plane und_plane = PlaneFit<Scalar>::fit(und_points);

    Scalar d_mean = 0;
    Scalar und_mean = 0;
    Scalar und_max = 0;
    Scalar und_var = 0;
    int count = 0;
    for (int p = 0; p < und_points.elements(); ++p)
    {
      if (not und_points[p].allFinite())
        continue;
      d_mean += und_points[p].z();
      Scalar d = und_plane.absDistance(und_points[p]);
      und_mean += d;
      und_var += d * d;
      if (d > und_max)
        und_max = d;
      ++count;
    }

    d_mean /= count;
    und_mean /= count;
    und_var /= count;
    und_var -= und_mean * und_mean;

    const Cloud3 points = PCLConversion<Scalar>::toPointMatrix(*data->depthData()/*, *cb_views.planeInliers()*/);
    Plane plane = PlaneFit<Scalar>::fit(points);

    Scalar mean = 0;
    Scalar max = 0;
    Scalar var = 0;
    for (int p = 0; p < points.elements(); ++p)
    {
      if (not points[p].allFinite())
        continue;
      Scalar d = plane.absDistance(points[p]);
      mean += d;
      var += d * d;
      if (d > max)
        max = d;
    }

    mean /= count;
    var /= count;
    var -= mean * mean;

    std::cout << d_mean << " " << count << " ** " << mean << " " << std::sqrt(var) << " " << max << " ** " << und_mean << " " << std::sqrt(und_var) << " " << und_max << std::endl;
  }

}

class NormalError
{
public:

  NormalError(const PinholeCameraModel::ConstPtr & camera_model,
              const Checkerboard::ConstPtr & checkerboard,
              const Cloud2 & image_corners)
    : camera_model_(camera_model),
      checkerboard_(checkerboard),
      image_corners_(image_corners)
  {
  }

  template <typename T>
  bool operator ()(const T * const rotation,
                   const T * const checkerboard_pose,
                   T * residuals) const
  {

    typename Types<T>::Pose checkerboard_pose_eigen = Types<T>::Pose::Identity();
    checkerboard_pose_eigen.linear().template topLeftCorner<2, 2>() = Eigen::Rotation2D<T>(checkerboard_pose[0]).matrix();
    checkerboard_pose_eigen.translation() = Eigen::Map<const Eigen::Matrix<T, 3, 1> >(&checkerboard_pose[1]);

    Eigen::Matrix<T, 3, 3> new_base;
    T unit_xyz[] = {0.0, 0.0, 1.0, 0.0, 0.0};
    ceres::QuaternionRotatePoint(rotation, &unit_xyz[2], new_base.col(0).data());
    ceres::QuaternionRotatePoint(rotation, &unit_xyz[1], new_base.col(1).data());
    ceres::QuaternionRotatePoint(rotation, &unit_xyz[0], new_base.col(2).data());

    checkerboard_pose_eigen = new_base * checkerboard_pose_eigen;

    typename Types<T>::Cloud3 cb_corners(checkerboard_->corners().size());
    cb_corners.container() = checkerboard_pose_eigen * checkerboard_->corners().container().cast<T>();
    typename Types<T>::Cloud2 reprojected_corners = camera_model_->project3dToPixel<T>(cb_corners);

    for (Size1 i = 0; i < cb_corners.elements(); ++i)
    {
      typename Types<T>::Vector2 diff = (reprojected_corners[i] - image_corners_[i].cast<T>());
      residuals[2 * i + 0] = diff.x();
      residuals[2 * i + 1] = diff.y();
    }

    return true;
  }

private:

  const PinholeCameraModel::ConstPtr & camera_model_;
  const Checkerboard::ConstPtr & checkerboard_;
  const Cloud2 & image_corners_;

};

void CalibrationTest::testCheckerboardError() const
{
  std::vector<CheckerboardViews::Ptr> cb_views_vec;

  CheckerboardViewsExtraction cb_extractor;
  cb_extractor.setColorSensorPose(color_sensor_->pose());
  cb_extractor.setCheckerboardVector(cb_vec_);
  cb_extractor.setInputData(und_data_vec_);
  cb_extractor.extractAll(cb_views_vec);

  ceres::Problem problem;
  Eigen::Matrix<Scalar, Eigen::Dynamic, 4, Eigen::DontAlign | Eigen::RowMajor> data(cb_views_vec.size(), 4);

  Eigen::Matrix3d rot_matrix;
  rot_matrix.col(2) = cb_views_vec[0]->colorCheckerboard()->plane().normal();
  rot_matrix.col(0) = (Vector3::UnitX() - Vector3::UnitX().dot(rot_matrix.col(2)) * rot_matrix.col(2)).normalized();
  rot_matrix.col(1) = rot_matrix.col(2).cross(rot_matrix.col(0));

  Quaternion q(rot_matrix);
  Eigen::Matrix<Scalar, 1, 4, Eigen::RowMajor | Eigen::DontAlign> rotation(q.w(), q.x(), q.y(), q.z());

  for (Size1 i = 0; i < cb_views_vec.size(); ++i)
  {
    const CheckerboardViews & cb_views = *cb_views_vec[i];
    data.row(i)[0] = std::acos((cb_views.colorCheckerboard()->corners()(1, 0) - cb_views.colorCheckerboard()->corners()(0, 0)).normalized().x());
    data.row(i).tail<3>() = rot_matrix.inverse() * cb_views.colorCheckerboard()->pose().translation();

    NormalError * error = new NormalError(color_sensor_->cameraModel(),
                                          cb_views.checkerboard(),
                                          cb_views.colorView()->points());

    typedef ceres::NumericDiffCostFunction<NormalError, ceres::CENTRAL, ceres::DYNAMIC, 4, 4> NormalCostFunction;

    ceres::CostFunction * cost_function = new NormalCostFunction(error,
                                                                 ceres::DO_NOT_TAKE_OWNERSHIP,
                                                                 0 + 2 * cb_views.checkerboard()->size());
    problem.AddResidualBlock(cost_function, NULL, rotation.data(), data.row(i).data());
  }

  problem.SetParameterization(rotation.data(), new ceres::QuaternionParameterization());

  ceres::Solver::Options options;
  options.linear_solver_type = ceres::DENSE_QR;
  options.max_num_iterations = 50;
  options.minimizer_progress_to_stdout = true;
  options.num_threads = 8;

  ceres::Solver::Summary summary;
  ceres::Solve(options, &problem, &summary);

  std::vector<Plane> plane_vec;
  for (size_t i = 0; i < cb_views_vec.size(); ++i)
  {
    const CheckerboardViews & cb_views = *cb_views_vec[i];

    Pose checkerboard_pose_eigen = Pose::Identity();
    checkerboard_pose_eigen.linear().template topLeftCorner<2, 2>() = Eigen::Rotation2Dd(data.row(i)[0]).matrix();
    checkerboard_pose_eigen.translation() = data.row(i).tail<3>();

    Quaternion q(rotation[0], rotation[1], rotation[2], rotation[3]);
    Transform rot_matrix = q.matrix() * Transform::Identity();

    Checkerboard cb(*cb_views.checkerboard());
    cb.transform(color_sensor_->pose() * rot_matrix * checkerboard_pose_eigen);

    std::cout << cb.plane().coeffs().transpose() << ": " << cb.plane().normal().dot(cb.corners().container().rowwise().mean()) << std::endl;

//    AngleAxis rotation;
//    rotation.angle() = data.row(i).head<3>().norm();
//    rotation.axis() = data.row(i).head<3>().normalized();
//    Translation3 translation(data.row(i).tail<3>());
//    cb.transform(translation * rotation);
    plane_vec.push_back(cb.plane());
  }

  for (size_t i = 0; i < cb_views_vec.size(); ++i)
  {
    const CheckerboardViews & cb_views = *cb_views_vec[i];
    RGBDData::ConstPtr und_data = cb_views.data();
    RGBDData::ConstPtr part_und_data = part_data_map_.at(und_data);
    RGBDData::ConstPtr data = data_map_.at(und_data);
    Plane plane = plane_vec[i]; //cb_views.colorCheckerboard()->plane();
    //plane.transform(color_sensor_->pose());

    const Cloud3 & und_points = PCLConversion<Scalar>::toPointMatrix(*und_data->depthData()/*, *cb_views.planeInliers()*/);

    Point3 und_d_mean(0.0, 0.0, 0.0);
    Scalar und_mean = 0;
    Scalar und_mean2 = 0;
    int count = 0;
    for (int p = 0; p < und_points.elements(); ++p)
    {
      if (not und_points[p].allFinite())
        continue;
      und_d_mean += und_points[p];
      Scalar d = plane.signedDistance(und_points[p]);
      und_mean += d;
      und_mean2 += d * d;
      ++count;
    }

    und_d_mean /= count;
    und_mean /= count;
    und_mean2 /= count;
    Scalar und_std_dev = std::sqrt(und_mean2 - und_mean * und_mean);

    const Cloud3 points = PCLConversion<Scalar>::toPointMatrix(*data->depthData()/*, *cb_views.planeInliers()*/);

    Point3 d_mean(0.0, 0.0, 0.0);
    Scalar mean = 0;
    Scalar mean2 = 0;
    for (Size1 p = 0; p < points.elements(); ++p)
    {
      if (not points[p].allFinite())
        continue;
      d_mean += points[p];
      Scalar d = plane.signedDistance(points[p]);
      mean += d;
      mean2 += d * d;
    }

    d_mean /= count;
    mean /= count;
    mean2 /= count;
    Scalar std_dev = std::sqrt(mean2 - mean * mean);

    const Cloud3 part_points = PCLConversion<Scalar>::toPointMatrix(*part_und_data->depthData()/*, *cb_views.planeInliers()*/);

    Scalar part_mean = 0;
    for (int p = 0; p < part_points.elements(); ++p)
    {
      if (not part_points[p].allFinite())
        continue;
      Scalar d = plane.signedDistance(part_points[p]);
      part_mean += d;
    }

    part_mean /= count;

    Plane und_fitted_plane = PlaneFit<Scalar>::fit(und_points);
    Scalar und_angle = std::acos(und_fitted_plane.normal().dot(plane.normal()));
    if (und_angle > M_PI_2)
      und_angle = M_PI - und_angle;

    Plane fitted_plane = PlaneFit<Scalar>::fit(points);
    Scalar angle = std::acos(fitted_plane.normal().dot(plane.normal()));
    if (angle > M_PI_2)
      angle = M_PI - angle;

    std::cout << plane.normal().dot(d_mean) << ": " << mean << ", " << std_dev << ", " << angle << "; " << plane.normal().dot(und_d_mean) << ": "<< und_mean << ", " << und_std_dev << ", " << und_angle << "; " << part_mean << std::endl;



  }

}

} /* namespace calibration */
