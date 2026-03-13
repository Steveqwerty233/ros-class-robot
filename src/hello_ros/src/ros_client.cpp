#include "ros/ros.h"
#include "hello_ros/MyServiceMsg.h"

int main(int argc, char **argv)
{
    ros::init(argc, argv, "my_service_client");
    ros::NodeHandle nh;

    // 创建一个 ROS 客户端，请求 my_service 服务
    ros::ServiceClient client =
        nh.serviceClient<hello_ros::MyServiceMsg>("my_service");

    // 创建一个请求消息
    hello_ros::MyServiceMsg srv;
    srv.request.input = 42;

    if (client.call(srv))
    {
        ROS_INFO("Service response: %ld", (long int)srv.response.output);
    }
    else
    {
        ROS_ERROR("Failed to call service my_service");
        return 1;
    }

    return 0;
}