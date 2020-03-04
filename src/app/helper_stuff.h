#pragma once

#include <srrg2_proslam_tracking/instances.h>
#include <srrg_config/configurable_manager.h>
#include <srrg_laser_slam_2d/instances.h>
#include <srrg_messages/instances.h>
#include <srrg_messages_ros/instances.h>
#include <srrg_pcl/instances.h>
#include <srrg_slam_interfaces/instances.h>
#include <srrg_solver/solver_core/instances.h>
#include <srrg_solver/solver_core/internals/linear_solvers/instances.h>
#include <srrg_solver/variables_and_factors/types_2d/instances.h>
#include <srrg_solver/variables_and_factors/types_3d/instances.h>
#include <srrg_solver/variables_and_factors/types_calib/instances.h>
#include <srrg_solver/variables_and_factors/types_projective/instances.h>
#include <srrg_system_utils/parse_command_line.h>

static void registerAllInstances() {
  srrg2_core::point_cloud_registerTypes();
  srrg2_core::messages_registerTypes();
  srrg2_core_ros::messages_ros_registerTypes();
  
  srrg2_solver::solver_registerTypes();
  srrg2_solver::linear_solver_registerTypes();
  srrg2_solver::registerTypes2D();
  srrg2_solver::registerTypes3D();
  srrg2_solver::calib_registerTypes();
  srrg2_solver::projective_registerTypes();
  
  srrg2_slam_interfaces::slam_interfaces_registerTypes();
  
  srrg2_laser_tracker_2d::laser_tracker_2d_registerTypes();
  srrg2_proslam::srrg2_proslam_tracking_registerTypes();
}
