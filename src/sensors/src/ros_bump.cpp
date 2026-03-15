#include <ros/ros.h>
#include <std_msgs/Int16MultiArray.h>
#include <geometry_msgs/Twist.h>

enum
{
    STATE_FORWARD = 0,
    STATE_BACKWARD = 1
};

int state = STATE_FORWARD;

// 是否收到过碰撞传感器消息
bool got_bump_msg = false;

// 当前是否有任一碰撞传感器触发
bool bump_triggered = false;

// 后退开始时间
ros::Time backward_start_time;

void bumpCallback(const std_msgs::Int16MultiArray::ConstPtr &msg)
{
    got_bump_msg = true;

    bool triggered = false;

    for (int i = 0; i < msg->data.size(); ++i)
    {
        if (msg->data[i] != 0)
        {
            triggered = true;
        }
    }

    bump_triggered = triggered;

    // 只要在前进状态下检测到碰撞，立刻切换到后退
    if (state == STATE_FORWARD && bump_triggered)
    {
        state = STATE_BACKWARD;
        backward_start_time = ros::Time::now();

        ROS_WARN("Bump triggered! Start backing up.");
    }
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "bump_react_node");
    ros::NodeHandle nh;

    ros::Subscriber bump_sub =
        nh.subscribe("/robot/bump_sensor", 1000, bumpCallback);

    ros::Publisher cmd_pub =
        nh.advertise<geometry_msgs::Twist>("/cmd_vel", 10);

    ros::Rate rate(50);  // 高频一点，反应更灵敏

    geometry_msgs::Twist cmd;

    // ===== 参数 =====
    const double forward_speed = 0.08;     // 慢速前进
    const double backward_speed = -0.12;   // 后退速度（负号）
    const double backward_distance = 0.2;  // 希望后退 0.2 m
    const double backward_time = backward_distance / 0.12; // 距离 / 速度 = 时间
    // ================

    while (ros::ok())
    {
        ros::spinOnce();

        // 如果还没收到碰撞传感器消息，先停车等待，避免盲目前进
        if (!got_bump_msg)
        {
            cmd.linear.x = 0.0;
            cmd.angular.z = 0.0;
            cmd_pub.publish(cmd);

            rate.sleep();
            continue;
        }

        if (state == STATE_FORWARD)
        {
            cmd.linear.x = forward_speed;
            cmd.angular.z = 0.0;
        }
        else if (state == STATE_BACKWARD)
        {
            double elapsed = (ros::Time::now() - backward_start_time).toSec();

            if (elapsed < backward_time)
            {
                cmd.linear.x = backward_speed;
                cmd.angular.z = 0.0;
            }
            else
            {
                cmd.linear.x = 0.0;
                cmd.angular.z = 0.0;

                state = STATE_FORWARD;
                bump_triggered = false;

                ROS_INFO("Backing finished. Move forward again.");
            }
        }

        cmd_pub.publish(cmd);

        ROS_INFO_THROTTLE(
            0.5,
            "state=%d bump=%d cmd_v=%.3f",
            state, bump_triggered, cmd.linear.x
        );

        rate.sleep();
    }

    return 0;
}