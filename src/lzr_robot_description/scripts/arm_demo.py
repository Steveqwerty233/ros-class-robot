#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import rospy
from std_msgs.msg import Float64


def publish_positions(pub1, pub2, pub3, pub4, pub5, q1, q2, q3, q4, q5, repeat=50):
    msg1 = Float64(data=q1)
    msg2 = Float64(data=q2)
    msg3 = Float64(data=q3)
    msg4 = Float64(data=q4)
    msg5 = Float64(data=q5)

    rate = rospy.Rate(20)
    for _ in range(repeat):
        pub1.publish(msg1)
        pub2.publish(msg2)
        pub3.publish(msg3)
        pub4.publish(msg4)
        pub5.publish(msg5)
        rate.sleep()


def main():
    rospy.init_node("arm_demo_position_controller")

    pub1 = rospy.Publisher("/joint_1_position_controller/command", Float64, queue_size=10)
    pub2 = rospy.Publisher("/joint_2_position_controller/command", Float64, queue_size=10)
    pub3 = rospy.Publisher("/joint_3_position_controller/command", Float64, queue_size=10)
    pub4 = rospy.Publisher("/joint_4_position_controller/command", Float64, queue_size=10)
    pub5 = rospy.Publisher("/joint_5_position_controller/command", Float64, queue_size=10)

    rospy.sleep(1.0)

    rospy.loginfo("Move to pose 1")
    publish_positions(pub1, pub2, pub3, pub4, pub5,
                      0.0, -0.5, 0.8, -0.3, 0.0)

    rospy.sleep(1.0)

    rospy.loginfo("Move to pose 2")
    publish_positions(pub1, pub2, pub3, pub4, pub5,
                      0.6, -0.8, 1.0, -0.5, 0.4)

    rospy.sleep(1.0)

    rospy.loginfo("Move to pose 3")
    publish_positions(pub1, pub2, pub3, pub4, pub5,
                      -0.6, -0.3, 0.6, 0.2, -0.4)

    rospy.loginfo("Demo finished")


if __name__ == "__main__":
    main()
