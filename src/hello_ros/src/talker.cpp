#include <ros/ros.h>
#include <std_msgs/String.h>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "hello_cpp_publisher");

    ros::NodeHandle nh;

    ros::Publisher pub =
        nh.advertise<std_msgs::String>("hello_topic", 10);

    ros::Rate loop_rate(1);

    while (ros::ok())
    {
        std_msgs::String msg;
        msg.data = "Hello ROS from C++";

        ROS_INFO("%s", msg.data.c_str());

        pub.publish(msg);

        ros::spinOnce();

        loop_rate.sleep();
    }

    return 0;
}
