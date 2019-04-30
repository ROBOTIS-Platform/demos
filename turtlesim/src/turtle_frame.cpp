/*
 * Copyright (c) 2009, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "turtlesim/turtle_frame.h"

#include <QPointF>

// #include <ros/package.h>
#include <cstdlib>
#include <ctime>

#define DEFAULT_BG_R 0x45
#define DEFAULT_BG_G 0x56
#define DEFAULT_BG_B 0xff

namespace turtlesim
{

TurtleFrame::TurtleFrame(rclcpp::Node::SharedPtr &node_handle, QWidget* parent, Qt::WindowFlags f)
: QFrame(parent, f),
  path_image_(500, 500, QImage::Format_ARGB32),
  path_painter_(&path_image_),
  frame_count_(0),
  id_counter_(0),
  nh_(node_handle)
{
  setFixedSize(500, 500);
  setWindowTitle("TurtleSim");

  srand(time(NULL));

  update_timer_ = new QTimer(this);
  update_timer_->setInterval(16);
  update_timer_->start();

  connect(update_timer_, SIGNAL(timeout()), this, SLOT(onUpdate()));

  nh_->declare_parameter("background_r", DEFAULT_BG_R);
  nh_->declare_parameter("background_g", DEFAULT_BG_G);
  nh_->declare_parameter("background_b", DEFAULT_BG_B);

  QVector<QString> turtles;
  turtles.append("box-turtle.png");
  turtles.append("robot-turtle.png");
  turtles.append("sea-turtle.png");
  turtles.append("diamondback.png");
  turtles.append("electric.png");
  turtles.append("fuerte.png");
  turtles.append("groovy.png");
  turtles.append("hydro.svg");
  turtles.append("indigo.svg");
  turtles.append("jade.png");
  turtles.append("kinetic.png");
  turtles.append("lunar.png");
  turtles.append("melodic.png");

  QString images_path = (ament_index_cpp::get_package_share_directory("turtlesim") + "/images/").c_str();
    //(ros::package::getPath("turtlesim") + "/images/").c_str();
  for (int i = 0; i < turtles.size(); ++i)
  {
    QImage img;
    img.load(images_path + turtles[i]);
    turtle_images_.append(img);
  }

  meter_ = turtle_images_[0].height();

  clear();

  auto clear_call_back = 
    [this](const std::shared_ptr<rmw_request_id_t> /*request_header*/,
          const std::shared_ptr<std_srvs::srv::Empty::Request> /*request*/,
          std::shared_ptr<std_srvs::srv::Empty::Response> /*response*/) -> bool
    {
      RCLCPP_INFO(nh_->get_logger(), "Clearing turtlesim.");
      clear();

      return true;
    };

  auto reset_call_back = 
    [this](const std::shared_ptr<rmw_request_id_t> /*request_header*/,
          const std::shared_ptr<std_srvs::srv::Empty::Request> /*request*/,
          std::shared_ptr<std_srvs::srv::Empty::Response> /*response*/) -> bool
    {
      RCLCPP_INFO(nh_->get_logger(), "Resetting turtlesim.");
      turtles_.clear();
      id_counter_ = 0;
      spawnTurtle("", width_in_meters_ / 2.0, height_in_meters_ / 2.0, 0);
      clear();

      return true;
    };

  auto spawn_call_back = 
    [this](const std::shared_ptr<rmw_request_id_t> /*request_header*/,
          const std::shared_ptr<turtlesim::srv::Spawn::Request> request,
          std::shared_ptr<turtlesim::srv::Spawn::Response> response) -> bool
    {
      std::string name = spawnTurtle(request->name, request->x, request->y, request->theta);
      if (name.empty())
      {
        RCLCPP_ERROR(nh_->get_logger(), "A turtled named [%s] already exists", request->name.c_str());
        return false;
      }

      response->name = name;

      return true;
    };

  auto kill_call_back = 
    [this](const std::shared_ptr<rmw_request_id_t> /*request_header*/,
          const std::shared_ptr<turtlesim::srv::Kill::Request> request,
          std::shared_ptr<turtlesim::srv::Kill::Response> /*response*/) -> bool
    {
      M_Turtle::iterator it = turtles_.find(request->name);
      if (it == turtles_.end())
      {
        RCLCPP_ERROR(nh_->get_logger(), "Tried to kill turtle [%s], which does not exist", request->name.c_str());
        return false;
      }

      turtles_.erase(it);
      update();

      return true;
    };

  clear_srv_ = nh_->create_service<std_srvs::srv::Empty>("clear", clear_call_back);
  reset_srv_ = nh_->create_service<std_srvs::srv::Empty>("reset", reset_call_back);
  spawn_srv_ = nh_->create_service<turtlesim::srv::Spawn>("spawn", spawn_call_back);
  kill_srv_ = nh_->create_service<turtlesim::srv::Kill>("kill", kill_call_back);

  // RCLCPP_INFO(nh_->get_logger(), "Starting turtlesim with node name %s", nh_->get_node_names().at(0).c_str());

  width_in_meters_ = (width() - 1) / meter_;
  height_in_meters_ = (height() - 1) / meter_;
  spawnTurtle("", width_in_meters_ / 2.0, height_in_meters_ / 2.0, 0);

  // spawn all available turtle types
  if(false)
  {
    for(int index = 0; index < turtles.size(); ++index)
    {
      QString name = turtles[index];
      name = name.split(".").first();
      name.replace(QString("-"), QString(""));
      spawnTurtle(name.toStdString(), 1.0 + 1.5 * (index % 7), 1.0 + 1.5 * (index / 7), PI / 2.0, index);
    }
  }
}

TurtleFrame::~TurtleFrame()
{
  delete update_timer_;
}

// bool TurtleFrame::spawnCallback(turtlesim::Spawn::Request& req, turtlesim::Spawn::Response& res)
// {
  // std::string name = spawnTurtle(req.name, req.x, req.y, req.theta);
  // if (name.empty())
  // {
    // ROS_ERROR("A turtled named [%s] already exists", req.name.c_str());
    // return false;
  // }

  // res.name = name;

  // return true;
// }

// bool TurtleFrame::killCallback(turtlesim::Kill::Request& req, turtlesim::Kill::Response&)
// {
  // M_Turtle::iterator it = turtles_.find(req.name);
  // if (it == turtles_.end())
  // {
    // ROS_ERROR("Tried to kill turtle [%s], which does not exist", req.name.c_str());
    // return false;
  // }

  // turtles_.erase(it);
  // update();

  // return true;
// }

bool TurtleFrame::hasTurtle(const std::string& name)
{
  return turtles_.find(name) != turtles_.end();
}

std::string TurtleFrame::spawnTurtle(const std::string& name, float x, float y, float angle)
{
  return spawnTurtle(name, x, y, angle, rand() % turtle_images_.size());
}

std::string TurtleFrame::spawnTurtle(const std::string& name, float x, float y, float angle, size_t index)
{
  std::string real_name = name;
  if (real_name.empty())
  {
    do
    {
      std::stringstream ss;
      ss << "turtle" << ++id_counter_;
      real_name = ss.str();
    } while (hasTurtle(real_name));
  }
  else
  {
    if (hasTurtle(real_name))
    {
      return "";
    }
  }

  TurtlePtr t = std::make_shared<Turtle>(nh_, turtle_images_[index], QPointF(x, height_in_meters_ - y), angle);
  turtles_[real_name] = t;
  update();

  RCLCPP_INFO(nh_->get_logger(), "Spawning turtle [%s] at x=[%f], y=[%f], theta=[%f]", real_name.c_str(), x, y, angle);

  return real_name;
}

void TurtleFrame::clear()
{
  int r = DEFAULT_BG_R;
  int g = DEFAULT_BG_G;
  int b = DEFAULT_BG_B;

  nh_->declare_parameter("background_r", r);
  nh_->declare_parameter("background_g", g);
  nh_->declare_parameter("background_b", b);

  path_image_.fill(qRgb(r, g, b));
  update();
}

void TurtleFrame::onUpdate()
{
  // ros::spinOnce();

  updateTurtles();

  // if (!ros::ok())
  // {
    // close();
  // }
}

void TurtleFrame::paintEvent(QPaintEvent*)
{
  QPainter painter(this);

  painter.drawImage(QPoint(0, 0), path_image_);

  M_Turtle::iterator it = turtles_.begin();
  M_Turtle::iterator end = turtles_.end();
  for (; it != end; ++it)
  {
    it->second->paint(painter);
  }
}

void TurtleFrame::updateTurtles()
{
  if (last_turtle_update_ == rclcpp::Time(0))
  {
    last_turtle_update_ = nh_->now();
    return;
  }

  bool modified = false;
  M_Turtle::iterator it = turtles_.begin();
  M_Turtle::iterator end = turtles_.end();
  for (; it != end; ++it)
  {
    modified |= it->second->update(0.001 * update_timer_->interval(), path_painter_, path_image_, width_in_meters_, height_in_meters_);
  }
  if (modified)
  {
    update();
  }

  ++frame_count_;
}


// bool TurtleFrame::clearCallback(std_srvs::Empty::Request&, std_srvs::Empty::Response&)
// {
  // ROS_INFO("Clearing turtlesim.");
  // clear();
  // return true;
// }

// bool TurtleFrame::resetCallback(std_srvs::Empty::Request&, std_srvs::Empty::Response&)
// {
  // ROS_INFO("Resetting turtlesim.");
  // turtles_.clear();
  // id_counter_ = 0;
  // spawnTurtle("", width_in_meters_ / 2.0, height_in_meters_ / 2.0, 0);
  // clear();
  // return true;
// }

}