#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include <tf2/LinearMath/Quaternion.h>
#include <geometry_msgs/Quaternion.h>
#include <upros_message/TagCommand.h>
#include <map>
#include <string>

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

class VoiceNavNode
{
public:
    VoiceNavNode() : ac_("move_base", true)
    {
        ROS_INFO("Waiting for move_base action server...");
        ac_.waitForServer();
        ROS_INFO("Connected to move_base.");

        initGoals();

        voice_sub_ = nh_.subscribe("/voice_control", 10, &VoiceNavNode::voiceCallback, this);

        ROS_INFO("Voice navigation node is ready. Waiting for /voice_control commands...");
    }

private:
    ros::NodeHandle nh_;
    ros::Subscriber voice_sub_;
    MoveBaseClient ac_;
    std::map<int, move_base_msgs::MoveBaseGoal> goals_;

    void initGoals()
    {
        tf2::Quaternion quaternion;

        // ===== 1号点 =====
        move_base_msgs::MoveBaseGoal goal1;
        quaternion.setRPY(0, 0, 0);
        goal1.target_pose.header.frame_id = "map";
        goal1.target_pose.pose.position.x = 1.90;
        goal1.target_pose.pose.position.y = -1.79;
        goal1.target_pose.pose.position.z = 0.0;
        goal1.target_pose.pose.orientation.x = 0.0;
        goal1.target_pose.pose.orientation.y = 0.0;
        goal1.target_pose.pose.orientation.z = quaternion.z();
        goal1.target_pose.pose.orientation.w = quaternion.w();
        goals_[1] = goal1;

        // ===== 2号点 =====
        move_base_msgs::MoveBaseGoal goal2;
        quaternion.setRPY(0, 0, -1.5707);
        goal2.target_pose.header.frame_id = "map";
        goal2.target_pose.pose.position.x = 1.90;
        goal2.target_pose.pose.position.y = -2.99;
        goal2.target_pose.pose.position.z = 0.0;
        goal2.target_pose.pose.orientation.x = 0.0;
        goal2.target_pose.pose.orientation.y = 0.0;
        goal2.target_pose.pose.orientation.z = quaternion.z();
        goal2.target_pose.pose.orientation.w = quaternion.w();
        goals_[2] = goal2;

        // ===== Home 点（这里设为 3号点）=====
        move_base_msgs::MoveBaseGoal goal3;
        goal3.target_pose.header.frame_id = "map";
        goal3.target_pose.pose.position.x = 0.0;
        goal3.target_pose.pose.position.y = 0.0;
        goal3.target_pose.pose.position.z = 0.0;
        goal3.target_pose.pose.orientation.x = 0.0;
        goal3.target_pose.pose.orientation.y = 0.0;
        goal3.target_pose.pose.orientation.z = 0.0;
        goal3.target_pose.pose.orientation.w = 1.0;
        goals_[3] = goal3;
    }

    void voiceCallback(const upros_message::TagCommand::ConstPtr& msg)
    {
        ROS_INFO("Received voice command: intent=%s, target=%d",
                 msg->intent.c_str(), msg->target);

        // 只处理导航指令，其它如 pick / release 不在本节点处理
        if (msg->intent != "go_to")
        {
            ROS_WARN("Ignore non-navigation command: %s", msg->intent.c_str());
            return;
        }

        if (goals_.find(msg->target) == goals_.end())
        {
            ROS_WARN("Unknown target id: %d", msg->target);
            return;
        }

        sendGoal(msg->target);
    }

    void sendGoal(int target_id)
    {
        move_base_msgs::MoveBaseGoal goal = goals_[target_id];
        goal.target_pose.header.stamp = ros::Time::now();

        ROS_INFO("Send Goal %d !!!", target_id);

        ac_.sendGoal(goal);
        ac_.waitForResult();

        if (ac_.getState() == actionlib::SimpleClientGoalState::SUCCEEDED)
        {
            ROS_INFO("Goal %d reached successfully!", target_id);
        }
        else
        {
            ROS_WARN("Goal %d failed. State: %s",
                     target_id, ac_.getState().toString().c_str());
        }
    }
};

int main(int argc, char **argv)
{
    ros::init(argc, argv, "voice_nav_node");

    VoiceNavNode node;
    ros::spin();

    return 0;
}
