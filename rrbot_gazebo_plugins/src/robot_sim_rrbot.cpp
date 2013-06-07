/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2013, Open Source Robotics Foundation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Open Source Robotics Foundation
 *     nor the names of its contributors may be
 *     used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Jonathan Bohren, Dave Coleman
   Desc:   Simple manipulator effort controller
*/

#include <pluginlib/class_list_macros.h>

#include <hardware_interface/joint_command_interface.h>
#include <hardware_interface/robot_hw.h>

#include <ros_control_gazebo/robot_sim.h>

#include <angles/angles.h>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>

namespace rrbot_gazebo {

  class RobotSimRRBot : public ros_control_gazebo::RobotSim
  {
  public:
    RobotSimRRBot() :
      n_dof_(2),
      joint_name_(n_dof_),
      joint_position_(n_dof_),
      joint_velocity_(n_dof_),
      joint_effort_(n_dof_),
      joint_effort_command_(n_dof_),
      joint_velocity_command_(n_dof_)
    {
      // Move this out
      joint_name_[0] = "joint1";
      joint_name_[1] = "joint2";

      for(unsigned int j=0; j < n_dof_; j++) 
      {
        joint_position_[j] = 1.0;
        joint_velocity_[j] = 0.0;
        joint_effort_[j] = 1.0;  // N/m for continuous joints
        joint_effort_command_[j] = 0.0;
        joint_velocity_command_[j] = 0.0;

        // Register joints
        js_interface_.registerJoint(joint_name_[j], &joint_position_[j], &joint_velocity_[j], &joint_effort_[j]);
        ej_interface_.registerJoint(js_interface_.getJointStateHandle(joint_name_[j]), &joint_effort_command_[j]);
        vj_interface_.registerJoint(js_interface_.getJointStateHandle(joint_name_[j]), &joint_velocity_command_[j]);
      }

      // Register interfaces
      registerInterface(&js_interface_);
      registerInterface(&ej_interface_);
      registerInterface(&vj_interface_);
    }

    bool initSim(ros::NodeHandle nh, gazebo::physics::ModelPtr model)
    {
      // Get the gazebo joints that correspond to the robot joints
      for(unsigned int j=0; j < n_dof_; j++) 
      {
        ROS_INFO_STREAM("Getting pointer to gazebo joint: "<<joint_name_[j]);
        gazebo::physics::JointPtr joint = model->GetJoint(joint_name_[j]);
        if (joint) 
        {
          sim_joints_.push_back(joint);
        } 
        else 
        {
          ROS_ERROR_STREAM("This robot has a joint named \""<<joint_name_[j]<<"\" which is not in the gazebo model.");
          return false;
        }
      }

      return true;
    }

    void readSim(ros::Time time, ros::Duration period)
    {
      for(unsigned int j=0; j < n_dof_; j++) 
      {
        // Gazebo has an interesting API...
        joint_position_[j] += angles::shortest_angular_distance
          (joint_position_[j], sim_joints_[j]->GetAngle(0).Radian());
        joint_velocity_[j] = sim_joints_[j]->GetVelocity(0);
        joint_effort_[j] = sim_joints_[j]->GetForce((unsigned int)(0));
      }
    }

    void writeSim(ros::Time time, ros::Duration period) 
    {
      for(unsigned int j=0; j < n_dof_; j++) 
      {
        // Gazebo has an interesting API...
        sim_joints_[j]->SetForce(0,joint_effort_command_[j]);
      }
    }

  private:
    unsigned int n_dof_;

    hardware_interface::JointStateInterface    js_interface_;
    hardware_interface::EffortJointInterface   ej_interface_;
    hardware_interface::VelocityJointInterface vj_interface_;

    std::vector<std::string> joint_name_;
    std::vector<double> joint_position_;
    std::vector<double> joint_velocity_;
    std::vector<double> joint_effort_;
    std::vector<double> joint_effort_command_;
    std::vector<double> joint_velocity_command_;

    std::vector<gazebo::physics::JointPtr> sim_joints_;
  };

}

PLUGINLIB_EXPORT_CLASS(rrbot_gazebo::RobotSimRRBot, ros_control_gazebo::RobotSim)
