#include "garage_utils_msgs/action/garage_vehicle_avoidance.hpp"
#include "garage_utils_msgs/msg/state.hpp"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

#include "visualization_msgs/msg/marker_array.hpp"
#include <map>
#include "garage_utils_msgs/msg/polygons.hpp"

#include "capella_ros_msg/msg/car_detect_array.hpp"
#include <unistd.h>
#include <nlohmann/json.hpp>


// double rects_[][4][2] = {
//         {{1.0, 0.0}, {1.0, 20.0}, {-1.0, 20.0}, {-1.0, 0.0}},
//         {{1.5, 6.0}, {15.0, 6.0}, {15, 8.0}, {1.5, 8.0}},
//         {{1.5, 13.0}, {15.0, 13.0}, {15, 15.0}, {1.5, 15.0}},
//         {{-10.0, 20.5}, {10.0, 20.5}, {10.0, 22.5}, {-10, 22.5}},
//         {{10.5, 22.0}, {12.5, 22.0}, {12.5, 33.0}, {10.5, 33.0}},
//         {{-13.0, 10.0}, {-11.0, 30.0}, {-13.0, 30.0}, {-11.0, 10.0}}  // 打乱顺序
// };
// int number = 6;   // 无地图纯测试环境

bool test_cancel = false;

uint8_t state_current = garage_utils_msgs::msg::State::INIT;
uint8_t state_last    = garage_utils_msgs::msg::State::INIT;

std::map<uint8_t, std::string> state_map;

rclcpp_action::ClientGoalHandle<garage_utils_msgs::action::GarageVehicleAvoidance>::SharedPtr goal_handle_;

void goal_response_callback(const rclcpp_action::ClientGoalHandle<garage_utils_msgs::action::GarageVehicleAvoidance>::SharedPtr& goal_handle)
{
        std::cout << "goal response callback" << std::endl;
        goal_handle_ = goal_handle;
        if (!goal_handle_) 
        {
                std::cout << "Goal was rejected by server" << std::endl;
        } 
        else 
        {
                std::cout << "Goal accepted by server, waiting for result" << std::endl;
        }
}

void feedback_callback(rclcpp_action::ClientGoalHandle<garage_utils_msgs::action::GarageVehicleAvoidance>::SharedPtr goal_handle, const std::shared_ptr<const garage_utils_msgs::action::GarageVehicleAvoidance::Feedback> feedback)
{
        (void)goal_handle;

        // (void)feedback;
        // std::cout << "state " << (int)feedback->state << std::endl;
        state_current = feedback->state;
        if (state_current != state_last)
        {
                std::cout << "change state from " << state_map[state_last] << " to " << state_map[state_current] << std::endl;;
                state_last = state_current;
        }
        
}

void result_callback(const rclcpp_action::ClientGoalHandle<garage_utils_msgs::action::GarageVehicleAvoidance>::WrappedResult& result)
{
        switch (result.code) 
        {
                case rclcpp_action::ResultCode::SUCCEEDED:
                std::cout << "Goal was succeed." << std::endl;
                
                break;

                case rclcpp_action::ResultCode::ABORTED:
                std::cout << "Goal was aborted" << std::endl;
                return;

                case rclcpp_action::ResultCode::CANCELED:
                std::cout << "Goal was canceled" << std::endl;
                return;

                default:
                std::cout << "Unknown result code" << std::endl;
                return;
        }
        std::cout << "reason: " << result.result->reason << std::endl;
}

bool received = false;
capella_ros_msg::msg::CarDetectArray car_information_msg;
bool test_in_garage = true;

void car_information_callback(const capella_ros_msg::msg::CarDetectArray::SharedPtr msg)
{
        received = true;
        car_information_msg = *msg;
        RCLCPP_INFO(rclcpp::get_logger("test"), "received /car_information msg.");
}

std::vector<std::vector<double>> parse_json_array(const std::string& json_str) {
        auto json = nlohmann::json::parse(json_str);
        std::vector<std::vector<double>> result;

        for (const auto& inner_array : json) {
            std::vector<double> vec;
            for (auto val : inner_array) vec.push_back(val.get<double>());
            result.push_back(vec);
        }
        return result;
    }

int main(int argc, char** argv)
{
        rclcpp::init(argc, argv);

        // define node
        auto node = std::make_shared<rclcpp::Node>("test_garage_action_client");

        RCLCPP_INFO(node->get_logger(), "get params from params file");
        // get polygon coords from params file
        std::vector<std::vector<std::vector<double>>> rects2_;
        node->declare_parameter("polygons", std::vector<std::string>());
        auto param_polygons = node->get_parameter("polygons").as_string_array();
        
        int number = param_polygons.size();

        for(int i = 0; i < number; i++)
        {
                RCLCPP_INFO(node->get_logger(), "polygon: %s", param_polygons[i].c_str());
        }

        for (int i = 0; i < number; i++)
        {
                auto rect_coords = parse_json_array(param_polygons[i]);
                rects2_.push_back(rect_coords);
        }

        // get other params
        node->declare_parameter<bool>("test_cancel", false);
        node->declare_parameter<bool>("test_in_garage", false);
        node->declare_parameter<std::vector<double>>("car_coord", std::vector<double>());
        node->declare_parameter<std::vector<double>>("car_size", std::vector<double>());

        node->get_parameter<bool>("test_cancel", test_cancel);
        node->get_parameter<bool>("test_in_garage", test_in_garage);
        std::vector<double> car_size_, car_coord_;
        node->get_parameter<std::vector<double>>("car_size", car_size_);
        node->get_parameter<std::vector<double>>("car_coord", car_coord_);

        std::stringstream ss_car_size, ss_car_coord;
        for (size_t i = 0; i < car_size_.size(); i++)
        {
                ss_car_size << car_size_[i] << " ";
        }
        RCLCPP_INFO(node->get_logger(), "car_size: [ %s]", ss_car_size.str().c_str());


        for (size_t i = 0; i < car_coord_.size(); i++)
        {
                ss_car_coord << car_coord_[i] << " ";
        }
        RCLCPP_INFO(node->get_logger(), "car_coord: [ %s]", ss_car_coord.str().c_str());

        RCLCPP_INFO(node->get_logger(), "test_cancel: %s", test_cancel ? "true" : "false");
        RCLCPP_INFO(node->get_logger(), "test_in_garage: %s", test_in_garage ? "true" : "false");

        auto callback_group = node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        auto sub_options = rclcpp::SubscriptionOptions();
        sub_options.callback_group = callback_group;
        auto callback = std::bind(car_information_callback, std::placeholders::_1);
        auto sub = node->create_subscription<capella_ros_msg::msg::CarDetectArray>("/car_information", rclcpp::SensorDataQoS(), callback, sub_options);

        // publisher
        auto polygons_pub = node->create_publisher<garage_utils_msgs::msg::Polygons>("garage_polygons", rclcpp::QoS(1).reliable().transient_local());

        // generate map from uint8_t to string for states
        state_map[garage_utils_msgs::msg::State::INIT] = "INIT";
        state_map[garage_utils_msgs::msg::State::SEARCHING_PARKING_SPACE] = "SEARCHING_PARKING_SPACE";
        state_map[garage_utils_msgs::msg::State::GENERATE_PATH] = "GENERATE_PATH";
        state_map[garage_utils_msgs::msg::State::WELT] = "WELT";
        state_map[garage_utils_msgs::msg::State::NAVIGATE_TO_POSE] = "NAVIGATE_TO_POSE";
        state_map[garage_utils_msgs::msg::State::NAVIGATE_THROUGH_POSES] = "NAVIGATE_THROUGH_POSES";
        state_map[garage_utils_msgs::msg::State::WAITING] = "WAITING";

        // show rects in rviz2 by visualization_msgs
        auto rects_pub = node->create_publisher<visualization_msgs::msg::Marker>("rects_visualization", rclcpp::QoS(5).reliable().transient_local());
        visualization_msgs::msg::Marker msg;
        msg.header.frame_id = "map";
        msg.header.stamp = node->now();
        msg.ns = "test";
        msg.id = 0;
        msg.type = visualization_msgs::msg::Marker::LINE_LIST;
        msg.action = 0;
        msg.scale.x = 0.1;
        msg.color.r = 0.0;
        msg.color.g = 0.0;
        msg.color.b = 1.0;
        msg.color.a = 1.0;
        for (int i = 0; i < number; i++)
        {                
                for (int j = 0; j < 4; j++)
                {
                        geometry_msgs::msg::Point p_start;
                        p_start.x = rects2_[i][j][0];
                        p_start.y = rects2_[i][j][1];
                        p_start.z = 0.0;
                        msg.points.push_back(p_start);
                        geometry_msgs::msg::Point p_end;
                        p_end.x = rects2_[i][(j+1)%4][0];
                        p_end.y = rects2_[i][(j+1)%4][1];
                        p_end.z = 0.0;
                        msg.points.push_back(p_end);
                }
        }
        // for (int i = 0; i < 5; i++)
        // {
        //         RCLCPP_INFO(node->get_logger(), "publish rects visualization.");
                rects_pub->publish(msg);
        //         std::this_thread::sleep_for(std::chrono::seconds(1));
        // }
        
        

        geometry_msgs::msg::PoseStamped car_pose;
        car_pose.header.frame_id = "map";
        car_pose.pose.position.x = car_coord_[0];
        car_pose.pose.position.y = car_coord_[1];
        
        std::vector<geometry_msgs::msg::Polygon> polygons;
        garage_utils_msgs::msg::Polygons polygons_msg;
        for (int i = 0; i < number; i++)
        {
                geometry_msgs::msg::Polygon polygon;
                for (int j = 0; j < 4; j++)
                {
                        geometry_msgs::msg::Point32 point;
                        point.x = rects2_[i][j][0];
                        point.y = rects2_[i][j][1];
                        polygon.points.push_back(point);
                }
                polygons.push_back(polygon);
                polygons_msg.polygons.push_back(polygon);
        }

        // publish polygons msg
        RCLCPP_INFO(node->get_logger(), "publish garage_polygons topic one time ...");
        polygons_pub->publish(polygons_msg);
                
        RCLCPP_INFO(node->get_logger(), "create action client ...");
        auto client_ptr_ = rclcpp_action::create_client<garage_utils_msgs::action::GarageVehicleAvoidance>(
                node, "/garage_vehicle_avoidance");

        using namespace std::chrono_literals;
        while (!client_ptr_->wait_for_action_server(1s)) 
        {
                RCLCPP_WARN(node->get_logger(), "Action server not available, waiting");
                std::this_thread::sleep_for(std::chrono::duration<double>(1.0));                
        }

        auto goal_msg = garage_utils_msgs::action::GarageVehicleAvoidance::Goal();
        capella_ros_msg::msg::CarDetectSingle car_information;
        geometry_msgs::msg::Vector3 size;
        size.x = car_size_[0];
        size.y = car_size_[1];
        size.z = car_size_[2];
        car_information.pose = car_pose;
        car_information.size = size;

        auto send_goal_options = rclcpp_action::Client<garage_utils_msgs::action::GarageVehicleAvoidance>::SendGoalOptions();
        send_goal_options.goal_response_callback = goal_response_callback;
        send_goal_options.result_callback = result_callback;
        send_goal_options.feedback_callback = feedback_callback;
        goal_msg.polygons = polygons;

        if (!test_in_garage)
        {
                goal_msg.cars_information.results.push_back(car_information);
                
                RCLCPP_INFO(node->get_logger(), "Sending goal");
                auto future = client_ptr_->async_send_goal(goal_msg, send_goal_options);
        }
        else
        {
                RCLCPP_INFO(node->get_logger(), "test in garage");
                std::thread([&](){
                        try{
                        while(true)
                        {
                                if (!received)
                                {
                                        RCLCPP_INFO(node->get_logger(), "normal driving ...");
                                        sleep(1);
                                }
                                else
                                {
                                        RCLCPP_INFO(node->get_logger(), "reset sub for /car_information topic.");
                                        sub.reset();
                                        RCLCPP_INFO(node->get_logger(), "car_pose: (%f, %f)", 
                                                car_information_msg.results[0].pose.pose.position.x,
                                                car_information_msg.results[0].pose.pose.position.y);
                                        RCLCPP_INFO(node->get_logger(), "car_size: (%f, %f, %f)",
                                                car_information_msg.results[0].size.x,
                                                car_information_msg.results[0].size.y,
                                                car_information_msg.results[0].size.z);                                        
                                        RCLCPP_INFO(node->get_logger(), "car_speed=> linear_x: %f, angular_z: %f)", 
                                                car_information_msg.results[0].speed.twist.linear.x,
                                                car_information_msg.results[0].speed.twist.angular.z);
                                        RCLCPP_INFO(node->get_logger(), "Sending goal");
                                        goal_msg.cars_information = car_information_msg;
                                        auto future = client_ptr_->async_send_goal(goal_msg, send_goal_options);
                                        break;
                                }
                        }}
                        catch(...)
                        {
                                RCLCPP_INFO(node->get_logger(), "unknow exception.");
                        }
                }).detach();
                
        }       
        
        

        rclcpp::executors::MultiThreadedExecutor executor;
        executor.add_node(node);
        executor.spin();

        // test for cancel goal
        if (test_cancel)
        {
                std::thread([&](){
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                        if (goal_handle_)
                        {
                                RCLCPP_INFO(node->get_logger(), "cancel the goal.");
                                auto cancel_future = client_ptr_->async_cancel_goal(goal_handle_);
                                cancel_future.wait();
                                RCLCPP_INFO(node->get_logger(), "cancel goal completed.");
                        }
                        else
                        {
                                RCLCPP_INFO(node->get_logger(), "goal_handle is empty ptr, failed to cancel goal."); 
                        }   
                        }).detach();
        }

       
        
        
        // rclcpp::spin(node);
        rclcpp::shutdown();
}
