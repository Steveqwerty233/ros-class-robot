#!/usr/bin/env python3
import rospy
from std_msgs.msg import String

def talker():

    pub = rospy.Publisher('hello_topic', String, queue_size=10)

    rospy.init_node('hello_publisher')

    rate = rospy.Rate(1)

    while not rospy.is_shutdown():

        msg = String()
        msg.data = "Hello ROS"

        rospy.loginfo(msg.data)

        pub.publish(msg)

        rate.sleep()

if __name__ == '__main__':
    talker()
