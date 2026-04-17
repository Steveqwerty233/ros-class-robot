#include "tf2_ros/transform_listener.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "geometry_msgs/TransformStamped.h"
#include "geometry_msgs/PointStamped.h"
#include "upros_message/ArmPosition.h"
#include "upros_message/TagCommand.h"
#include "std_srvs/Empty.h"
#include <ros/ros.h>
#include <iostream>

int target_tag = 0;

void sleep(double second)
{
    ros::Duration(second).sleep();
}

void cmd_callback(const upros_message::TagCommand::ConstPtr &msg)
{
    ROS_INFO("cmd_callback: 收到语音指令回调");
    ROS_INFO("   msg->target = %d", msg->target);
    ROS_INFO("   msg->intent = %s", msg->intent.c_str());
    
    target_tag = msg->target;
    std::cout << "Get Tag :" << target_tag << std::endl;
    
    ROS_INFO("cmd_callback: target_tag 已更新为 %d", target_tag);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "voice_grab");
    ros::NodeHandle nh;

    ROS_INFO("========== voice_grab 节点启动 ==========");
    ROS_INFO("订阅话题: /voice_control");
    
    ros::Subscriber cmd_sub = nh.subscribe("/voice_control", 10, cmd_callback);
    ros::Rate rate(10);

    ROS_INFO("等待语音指令设置 target_tag...");
    ROS_INFO("当前 target_tag = %d", target_tag);

    int loop_count = 0;
    while (target_tag == 0 && ros::ok())
    {
        rate.sleep();
        ros::spinOnce();
        
        loop_count++;
        if (loop_count % 100 == 0)  // 每100个循环打印一次，避免日志过多
        {
            ROS_INFO("等待中... 当前 target_tag = %d, 已等待 %d 个循环", target_tag, loop_count);
        }
    }

    if (target_tag == 0)
    {
        ROS_WARN("节点被关闭，未收到有效的 target_tag");
        return -1;
    }

    ROS_INFO("收到有效的 target_tag = %d", target_tag);

    std::string tag_link;
    if (target_tag == 1)
    {
        tag_link = "tag_1";
        ROS_INFO("目标 Tag 设置为: %s (ID: 1)", tag_link.c_str());
    }
    else if (target_tag == 2)
    {
        tag_link = "tag_2";
        ROS_INFO("目标 Tag 设置为: %s (ID: 2)", tag_link.c_str());
    }
    else
    {
        ROS_ERROR("无效的 target_tag = %d, 只支持 1 或 2", target_tag);
        return -1;
    }

    ROS_INFO("初始化 TF2 监听器...");
    tf2_ros::Buffer buffer;
    tf2_ros::TransformListener listener(buffer);
    ROS_INFO("tf coordinate transforming....");

    ROS_INFO("尝试获取 TF 变换: from 'arm_base_link' to '%s'", tag_link.c_str());
    
    // 获取 tag 到机械臂基坐标的坐标变换
    geometry_msgs::TransformStamped tfs_1;
    try
    {
        tfs_1 = buffer.lookupTransform("arm_base_link", tag_link, ros::Time(0), ros::Duration(5.0));
        ROS_INFO("TF 变换获取成功！");
        ROS_INFO("   translation: x=%.3f, y=%.3f, z=%.3f", 
                 tfs_1.transform.translation.x,
                 tfs_1.transform.translation.y,
                 tfs_1.transform.translation.z);
        ROS_INFO("   rotation: x=%.3f, y=%.3f, z=%.3f, w=%.3f",
                 tfs_1.transform.rotation.x,
                 tfs_1.transform.rotation.y,
                 tfs_1.transform.rotation.z,
                 tfs_1.transform.rotation.w);
    }
    catch (tf2::TransformException &ex)
    {
        ROS_ERROR("TF 变换获取失败: %s", ex.what());
        return -1;
    }

    // 单位转换，ros 坐标系到逆运算坐标系
    int x = -int(tfs_1.transform.translation.y * 1000);
    int y = int(tfs_1.transform.translation.x * 1000) + 30;
    int z = int(tfs_1.transform.translation.z * 1000 + 40);

    ROS_INFO("转换后的坐标（单位：mm）:");
    ROS_INFO("   X = %d (原 y: %.3f)", x, tfs_1.transform.translation.y);
    ROS_INFO("   Y = %d (原 x: %.3f + 30)", y, tfs_1.transform.translation.x);
    ROS_INFO("   Z = %d (原 z: %.3f + 40)", z, tfs_1.transform.translation.z);

    // 初始化服务客户端
    ROS_INFO("初始化服务客户端...");
    ros::ServiceClient arm_move_open_client =
        nh.serviceClient<upros_message::ArmPosition>("/upros_arm_control/arm_pos_service_open");
    ros::ServiceClient arm_move_close_client =
        nh.serviceClient<upros_message::ArmPosition>("/upros_arm_control/arm_pos_service_close");
    ros::ServiceClient arm_zero_client =
        nh.serviceClient<std_srvs::Empty>("/upros_arm_control/zero_service");
    ros::ServiceClient arm_grab_client =
        nh.serviceClient<std_srvs::Empty>("/upros_arm_control/grab_service");

    // 检查服务是否可用
    ROS_INFO("检查服务是否可用...");
    if (arm_move_open_client.waitForExistence(ros::Duration(3.0)))
        ROS_INFO("  arm_pos_service_open 服务可用");
    else
        ROS_WARN("  arm_pos_service_open 服务不可用");
    
    if (arm_grab_client.waitForExistence(ros::Duration(3.0)))
        ROS_INFO("  grab_service 服务可用");
    else
        ROS_WARN("  grab_service 服务不可用");
    
    if (arm_zero_client.waitForExistence(ros::Duration(3.0)))
        ROS_INFO("  zero_service 服务可用");
    else
        ROS_WARN("  zero_service 服务不可用");

    ROS_INFO("Target X: %d, Target Y: %d, Target Z: %d", x, y, z);

    // 第一步：移动到抓取位置
    ROS_INFO("========== 步骤 1/3: 移动到抓取位置 ==========");
    upros_message::ArmPosition move_srv;
    move_srv.request.x = x;
    move_srv.request.y = y;
    move_srv.request.z = z;
    ROS_INFO("发送移动请求: x=%d, y=%d, z=%d", x, y, z);
    
    if (arm_move_open_client.call(move_srv))
    {
        ROS_INFO("移动服务调用成功");
    }
    else
    {
        ROS_ERROR("移动服务调用失败");
    }
    
    ROS_INFO("等待 5 秒，让机械臂移动到目标位置...");
    sleep(5.0);

    // 第二步：闭合夹爪
    ROS_INFO("========== 步骤 2/3: 闭合夹爪 ==========");
    std_srvs::Empty empty_srv;
    if (arm_grab_client.call(empty_srv))
    {
        ROS_INFO("夹爪闭合服务调用成功");
    }
    else
    {
        ROS_ERROR("夹爪闭合服务调用失败");
    }
    
    ROS_INFO("等待 5 秒，让夹爪完全闭合...");
    sleep(5.0);

    // 第三步：返回零位
    ROS_INFO("========== 步骤 3/3: 返回零位 ==========");
    if (arm_zero_client.call(empty_srv))
    {
        ROS_INFO("归零服务调用成功");
    }
    else
    {
        ROS_ERROR("归零服务调用失败");
    }
    
    ROS_INFO("等待 5 秒，让机械臂返回零位...");
    sleep(5.0);

    ROS_INFO("========== voice_grab 节点执行完成 ==========");
    ros::spin();

    return 0;
}
