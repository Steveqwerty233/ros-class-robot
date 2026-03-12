#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <tf/tf.h>
#include <math.h>

double x = 0;
double y = 0;
double yaw = 0;

double start_x = 0;
double start_y = 0;

double target_yaw = 0;
double forward_yaw_ref = 0;   // 当前直线段参考朝向

bool got_odom = false;
bool start_recorded = false;

int edge_count = 0;

enum
{
    STATE_FORWARD = 0,
    STATE_TURN = 1,
    STATE_PAUSE = 2
};

int state = STATE_FORWARD;

ros::Time pause_time;

double normalize_angle(double angle)
{
    while(angle > M_PI) angle -= 2*M_PI;
    while(angle < -M_PI) angle += 2*M_PI;
    return angle;
}

void odom_callback(const nav_msgs::OdometryConstPtr &msg)
{
    x = msg->pose.pose.position.x;
    y = msg->pose.pose.position.y;

    tf::Quaternion q(
        msg->pose.pose.orientation.x,
        msg->pose.pose.orientation.y,
        msg->pose.pose.orientation.z,
        msg->pose.pose.orientation.w);

    tf::Matrix3x3 m(q);

    double roll, pitch;
    m.getRPY(roll, pitch, yaw);

    if(!got_odom)
    {
        target_yaw = yaw;
        forward_yaw_ref = yaw;
        got_odom = true;
    }
}

double get_distance()
{
    return sqrt(pow(x - start_x, 2) + pow(y - start_y, 2));
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "square_move");

    ros::NodeHandle nh;

    ros::Subscriber odom_sub =
        nh.subscribe("/odom", 10, odom_callback);

    ros::Publisher cmd_pub =
        nh.advertise<geometry_msgs::Twist>("/cmd_vel", 10);

    ros::Rate rate(20);

    geometry_msgs::Twist cmd;

    // ===== 参数 =====
    const double side_length = 1.0;

    const double forward_speed_fast = 0.25;
    const double forward_speed_mid  = 0.15;
    const double forward_speed_slow = 0.08;

    const double forward_kp = 1.5;     // 直线航向修正
    const double max_forward_w = 0.20; // 直线阶段最大修正角速度

    const double turn_kp = 2.0;
    const double max_turn_w = 0.6;
    const double min_turn_w = 0.1;
    const double yaw_tolerance = 0.08; // 约 4.6 度

    const double pause_duration = 0.5;
    // ================

    while(ros::ok())
    {
        if(!got_odom)
        {
            ros::spinOnce();
            rate.sleep();
            continue;
        }

        if(edge_count >= 4)
        {
            cmd.linear.x = 0;
            cmd.angular.z = 0;
            cmd_pub.publish(cmd);

            ROS_INFO("Square finished");
            break;
        }

        // ==========================
        // STATE_FORWARD
        // ==========================
        if(state == STATE_FORWARD)
        {
            if(!start_recorded)
            {
                start_x = x;
                start_y = y;
                forward_yaw_ref = yaw;   // 每条边开始时记录当前朝向
                start_recorded = true;

                ROS_INFO("Start edge %d", edge_count + 1);
            }

            double dist = get_distance();
            double remain = side_length - dist;

            if(dist < side_length)
            {
                // 接近目标时减速，减少超调
                if(remain > 0.30)
                    cmd.linear.x = forward_speed_fast;
                else if(remain > 0.10)
                    cmd.linear.x = forward_speed_mid;
                else
                    cmd.linear.x = forward_speed_slow;

                // 直线阶段加入轻微航向修正
                double yaw_err = normalize_angle(forward_yaw_ref - yaw);
                cmd.angular.z = forward_kp * yaw_err;

                if(cmd.angular.z >  max_forward_w) cmd.angular.z =  max_forward_w;
                if(cmd.angular.z < -max_forward_w) cmd.angular.z = -max_forward_w;
            }
            else
            {
                cmd.linear.x = 0;
                cmd.angular.z = 0;

                target_yaw += M_PI / 2.0;
                target_yaw = normalize_angle(target_yaw);

                state = STATE_TURN;
            }
        }

        // ==========================
        // STATE_TURN
        // ==========================
        else if(state == STATE_TURN)
        {
            double err = normalize_angle(target_yaw - yaw);

            if(fabs(err) > yaw_tolerance)
            {
                cmd.linear.x = 0;

                cmd.angular.z = turn_kp * err;

                if(cmd.angular.z >  max_turn_w) cmd.angular.z =  max_turn_w;
                if(cmd.angular.z < -max_turn_w) cmd.angular.z = -max_turn_w;

                if(fabs(cmd.angular.z) < min_turn_w)
                    cmd.angular.z = (err > 0) ? min_turn_w : -min_turn_w;
            }
            else
            {
                cmd.linear.x = 0;
                cmd.angular.z = 0;

                edge_count++;
                pause_time = ros::Time::now();
                state = STATE_PAUSE;
                start_recorded = false;

                ROS_INFO("Turn finished edge_count=%d", edge_count);
            }
        }

        // ==========================
        // STATE_PAUSE
        // ==========================
        else if(state == STATE_PAUSE)
        {
            cmd.linear.x = 0;
            cmd.angular.z = 0;

            if((ros::Time::now() - pause_time).toSec() > pause_duration)
            {
                state = STATE_FORWARD;
            }
        }

        cmd_pub.publish(cmd);

        ROS_INFO_THROTTLE(
            0.5,
            "state:%d edge:%d dist:%.3f yaw:%.3f fwd_ref:%.3f target:%.3f cmd_v:%.3f cmd_w:%.3f",
            state, edge_count, get_distance(), yaw, forward_yaw_ref, target_yaw,
            cmd.linear.x, cmd.angular.z
        );

        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}
