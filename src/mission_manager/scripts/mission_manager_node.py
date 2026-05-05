#!/usr/bin/env python3
import math
import rospy
import actionlib

from std_msgs.msg import String, Bool
from geometry_msgs.msg import Quaternion, Twist
from move_base_msgs.msg import MoveBaseAction, MoveBaseGoal
from actionlib_msgs.msg import GoalStatus


class MissionManager:
    def __init__(self):
        rospy.init_node("mission_manager_node")

        self.move_base_action = rospy.get_param("~move_base_action", "/move_base")
        self.goal_timeout = rospy.get_param("~goal_timeout", 45.0)
        self.retry_count = rospy.get_param("~retry_count", 1)
        self.detect_duration = rospy.get_param("~detect_duration", 3.0)

        self.detection_topic = rospy.get_param("~detection_topic", "/target_found")
        self.talk_topic = rospy.get_param("~talk_topic", "/talk")

        self.waypoints = rospy.get_param("~waypoints")

        self.last_detect_time = None
        self.detect_true_count = 0
        self.detect_total_count = 0

        self.talk_pub = rospy.Publisher(self.talk_topic, String, queue_size=10)
        self.cmd_vel_pub = rospy.Publisher("/cmd_vel", Twist, queue_size=10)

        rospy.Subscriber(self.detection_topic, Bool, self.detection_callback)

        self.client = actionlib.SimpleActionClient(self.move_base_action, MoveBaseAction)

        rospy.loginfo("Waiting for move_base action server...")
        self.client.wait_for_server()
        rospy.loginfo("move_base connected.")

        rospy.sleep(0.5)

    def detection_callback(self, msg):
        self.detect_total_count += 1
        if msg.data:
            self.detect_true_count += 1
            self.last_detect_time = rospy.Time.now()

    def yaw_to_quaternion(self, yaw):
        q = Quaternion()
        q.z = math.sin(yaw / 2.0)
        q.w = math.cos(yaw / 2.0)
        return q

    def make_goal(self, x, y, yaw):
        goal = MoveBaseGoal()
        goal.target_pose.header.frame_id = "map"
        goal.target_pose.header.stamp = rospy.Time.now()

        goal.target_pose.pose.position.x = x
        goal.target_pose.pose.position.y = y
        goal.target_pose.pose.position.z = 0.0
        goal.target_pose.pose.orientation = self.yaw_to_quaternion(yaw)

        return goal

    def stop_robot(self, duration=0.3):
        twist = Twist()
        rate = rospy.Rate(20)
        end_time = rospy.Time.now() + rospy.Duration(duration)

        while rospy.Time.now() < end_time and not rospy.is_shutdown():
            self.cmd_vel_pub.publish(twist)
            rate.sleep()

    def speak(self, text):
        msg = String()
        msg.data = text
        self.talk_pub.publish(msg)
        rospy.loginfo("Speak: %s", text)
        rospy.sleep(1.0)

    def goto_waypoint(self, name):
        if name not in self.waypoints:
            rospy.logerr("Waypoint [%s] not found in yaml.", name)
            return False

        x, y, yaw = self.waypoints[name]

        for attempt in range(self.retry_count + 1):
            rospy.loginfo("Going to [%s], attempt %d/%d: x=%.2f y=%.2f yaw=%.2f",
                          name, attempt + 1, self.retry_count + 1, x, y, yaw)

            goal = self.make_goal(x, y, yaw)
            self.client.send_goal(goal)

            finished = self.client.wait_for_result(rospy.Duration(self.goal_timeout))

            if not finished:
                rospy.logwarn("[%s] timeout. Cancel goal.", name)
                self.client.cancel_goal()
                self.stop_robot(0.5)
                rospy.sleep(1.0)
                continue

            state = self.client.get_state()

            if state == GoalStatus.SUCCEEDED:
                rospy.loginfo("[%s] reached.", name)
                self.stop_robot(0.5)
                return True
            else:
                rospy.logwarn("[%s] failed, state=%d.", name, state)
                self.client.cancel_goal()
                self.stop_robot(0.5)
                rospy.sleep(1.0)

        rospy.logwarn("[%s] finally failed.", name)
        return False

    def detect_target(self, room_name):
        rospy.loginfo("Start detection in %s.", room_name)

        self.stop_robot(0.8)

        self.detect_true_count = 0
        self.detect_total_count = 0
        self.last_detect_time = None

        start_time = rospy.Time.now()
        rate = rospy.Rate(10)

        while not rospy.is_shutdown():
            elapsed = (rospy.Time.now() - start_time).to_sec()
            if elapsed >= self.detect_duration:
                break
            rate.sleep()

        rospy.loginfo("Detection result: true=%d total=%d",
                      self.detect_true_count, self.detect_total_count)

        # 判定策略：识别到至少 2 帧，就认为找到目标
        found = self.detect_true_count >= 2

        if found:
            self.speak("已找到目标")
        else:
            self.speak("未找到目标")

        return found

    def run(self):
        self.speak("开始巡逻")

        # 1. 房间1
        ok_room1 = self.goto_waypoint("room1")
        if not ok_room1 and "room1_backup" in self.waypoints:
            self.goto_waypoint("room1_backup")

        self.detect_target("room1")

        # 2. 房间2
        ok_room2 = self.goto_waypoint("room2")
        if not ok_room2 and "room2_backup" in self.waypoints:
            self.goto_waypoint("room2_backup")

        self.detect_target("room2")

        # 3. 返程通过障碍中间区域
        self.goto_waypoint("return_mid")

        # 4. 回终点
        self.goto_waypoint("home")

        self.speak("巡逻完成")
        rospy.loginfo("Mission finished.")


if __name__ == "__main__":
    try:
        node = MissionManager()
        node.run()
    except rospy.ROSInterruptException:
        pass
