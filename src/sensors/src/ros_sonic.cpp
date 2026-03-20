#include <ros/ros.h>
#include <sensor_msgs/Range.h>
#include <geometry_msgs/Twist.h>

enum
{
    STATE_FORWARD = 0,
    STATE_BACKWARD = 1
};

int state = STATE_FORWARD;

bool got_front_range = false;
double front_range = 999.0;

ros::Time backward_start_time;

void rangeCallbackFront(const sensor_msgs::Range::ConstPtr &msg)
{
    got_front_range = true;
    front_range = msg->range;

    // 只要当前处于前进状态，且前方距离小于阈值，就立刻后退
    if (state == STATE_FORWARD && front_range < 0.4)
    {
        state = STATE_BACKWARD;
        backward_start_time = ros::Time::now();

        ROS_WARN("Front obstacle detected! range = %.3f m, start backing up.", front_range);
    }
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "front_tof_avoid");
    ros::NodeHandle nh;

    ros::Subscriber sub_front =
        nh.subscribe<sensor_msgs::Range>("/ul/sensor2", 10, rangeCallbackFront);

    ros::Publisher cmd_pub =
        nh.advertise<geometry_msgs::Twist>("/cmd_vel", 10);

    ros::Rate rate(50);   // 高频循环，反应更快
    geometry_msgs::Twist cmd;

    // ===== 参数 =====
    const double obstacle_threshold = 0.4;   // 触发后退的距离阈值
    const double forward_speed = 0.08;       // 缓慢前进
    const double backward_speed = -0.12;     // 开环后退速度
    const double backward_distance = 0.5;    // 希望后退 0.5m
    const double backward_time = backward_distance / 0.12;  // t = s / v
    // ================

    while (ros::ok())
    {
        ros::spinOnce();

        // 没收到前向 TOF 数据前，不动
        if (!got_front_range)
        {
            cmd.linear.x = 0.0;
            cmd.angular.z = 0.0;
            cmd_pub.publish(cmd);

            rate.sleep();
            continue;
        }

        if (state == STATE_FORWARD)
        {
            // 正常缓慢前进
            cmd.linear.x = forward_speed;
            cmd.angular.z = 0.0;

            // 再保险：主循环里也判断一次，避免只靠回调
            if (front_range < obstacle_threshold)
            {
                state = STATE_BACKWARD;
                backward_start_time = ros::Time::now();

                cmd.linear.x = 0.0;
                cmd.angular.z = 0.0;

                ROS_WARN("Front obstacle detected in main loop! range = %.3f m", front_range);
            }
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

                ROS_INFO("Backing finished. Move forward again.");
            }
        }

        cmd_pub.publish(cmd);

        ROS_INFO_THROTTLE(
            0.5,
            "state=%d front_range=%.3f cmd_v=%.3f",
            state, front_range, cmd.linear.x
        );

        rate.sleep();
    }

    return 0;
}