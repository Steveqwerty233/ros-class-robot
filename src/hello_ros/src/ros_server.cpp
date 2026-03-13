#include "ros/ros.h"
#include "hello_ros/MyServiceMsg.h"

bool myServiceCallback(hello_ros::MyServiceMsgRequest &req,
                       hello_ros::MyServiceMsgResponse &res)
{
    // 处理服务请求
    res.output = req.input * 2;
    ROS_INFO("Request: input = %d, output = %d", req.input, res.output);
    return true;
}

int main(int argc, char **argv)
{
    // 初始化 ROS 节点
    ros::init(argc, argv, "my_service_node");

    // 创建节点句柄
    ros::NodeHandle nh;

    // 创建服务
    ros::ServiceServer service = nh.advertiseService("my_service", myServiceCallback);

    ROS_INFO("Ready to receive service requests.");

    // 循环处理 ROS 回调函数
    ros::spin();

    return 0;
}