#include <ros/ros.h>
#include <sensor_msgs/Imu.h>
#include <geometry_msgs/Twist.h>
#include <tf/tf.h>
#include <math.h>

double current_yaw = 0.0;
double start_yaw = 0.0;
double target_yaw = 0.0;

bool got_imu = false;
bool target_set = false;
bool finished = false;

double normalize_angle(double angle)
{
    while (angle > M_PI) angle -= 2.0 * M_PI;
    while (angle < -M_PI) angle += 2.0 * M_PI;
    return angle;
}

void imu_callback(const sensor_msgs::Imu::ConstPtr &imu_msg)
{
    // 从 IMU 四元数中提取 yaw
    tf::Quaternion q(
        imu_msg->orientation.x,
        imu_msg->orientation.y,
        imu_msg->orientation.z,
        imu_msg->orientation.w
    );

    tf::Matrix3x3 m(q);

    double roll, pitch;
    m.getRPY(roll, pitch, current_yaw);

    if (!got_imu)
    {
        start_yaw = current_yaw;
        got_imu = true;
    }
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "imu_turn_180");
    ros::NodeHandle nh;

    ros::Subscriber imu_sub = nh.subscribe("/imu/data", 10, imu_callback);
    ros::Publisher cmd_pub = nh.advertise<geometry_msgs::Twist>("/cmd_vel", 10);

    ros::Rate rate(20);
    geometry_msgs::Twist cmd;

    // ===== 参数 =====
    const double turn_kp = 2.5;          // 比例控制系数
    const double max_turn_w = 0.8;       // 最大角速度
    const double min_turn_w = 0.05;      // 最小角速度，防止快到目标时转不动
    const double yaw_tolerance = 0;   // 停止阈值
    // ================

    while (ros::ok())
    {
        ros::spinOnce();

        if (!got_imu)
        {
            rate.sleep();
            continue;
        }

        // 首次初始化目标角
        if (!target_set)
        {
            target_yaw = normalize_angle(start_yaw + M_PI);  // 原地旋转 180 度
            target_set = true;

            ROS_INFO("Start yaw = %.3f rad, target yaw = %.3f rad", start_yaw, target_yaw);
        }

        if (!finished)
        {
            double err = normalize_angle(target_yaw - current_yaw);

            if (fabs(err) > yaw_tolerance)
            {
                cmd.linear.x = 0.0;
                cmd.angular.z = turn_kp * err;

                // 限幅
                if (cmd.angular.z > max_turn_w)  cmd.angular.z = max_turn_w;
                if (cmd.angular.z < -max_turn_w) cmd.angular.z = -max_turn_w;

                // 防止角速度过小带不动底盘
                if (fabs(cmd.angular.z) < min_turn_w)
                {
                    cmd.angular.z = (err > 0) ? min_turn_w : -min_turn_w;
                }
            }
            else
            {
                cmd.linear.x = 0.0;
                cmd.angular.z = 0.0;
                finished = true;

                ROS_INFO("Turn 180 finished. Final yaw = %.3f rad", current_yaw);
            }
        }
        else
        {
            cmd.linear.x = 0.0;
            cmd.angular.z = 0.0;
        }

        cmd_pub.publish(cmd);

        ROS_INFO_THROTTLE(
            0.5,
            "yaw=%.3f target=%.3f err=%.3f cmd_w=%.3f finished=%d",
            current_yaw, target_yaw,
            normalize_angle(target_yaw - current_yaw),
            cmd.angular.z, finished
        );

        rate.sleep();
    }

    return 0;
}