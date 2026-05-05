#!/usr/bin/env python3
import rospy
import cv2
import apriltag

from sensor_msgs.msg import Image
from std_msgs.msg import Bool
from cv_bridge import CvBridge, CvBridgeError


class AprilTagDetectorNode:
    def __init__(self):
        rospy.init_node("apriltag_detector_node", anonymous=True)

        self.image_topic = rospy.get_param("~image_topic", "/camera/color/image_raw")
        self.result_image_topic = rospy.get_param("~result_image_topic", "/image_result")
        self.target_found_topic = rospy.get_param("~target_found_topic", "/target_found")
        self.target_tag_id = rospy.get_param("~target_tag_id", 1)

        self.bridge = CvBridge()

        self.tag_detector = apriltag.Detector(
            apriltag.DetectorOptions(families="tag36h11")
        )

        self.image_sub = rospy.Subscriber(
            self.image_topic,
            Image,
            self.image_callback,
            queue_size=1
        )

        self.image_pub = rospy.Publisher(
            self.result_image_topic,
            Image,
            queue_size=10
        )

        self.target_pub = rospy.Publisher(
            self.target_found_topic,
            Bool,
            queue_size=10
        )

        rospy.loginfo("AprilTag detector started.")
        rospy.loginfo("Image topic: %s", self.image_topic)
        rospy.loginfo("Target tag id: %d", self.target_tag_id)
        rospy.loginfo("Publish target found topic: %s", self.target_found_topic)

    def image_callback(self, msg):
        found_target = False

        try:
            cv_image = self.bridge.imgmsg_to_cv2(msg, "bgr8")
            frame = cv_image.copy()
            gray_frame = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)

            tags = self.tag_detector.detect(gray_frame)

            for tag in tags:
                corners = tag.corners.astype(int)

                # 画出所有识别到的 tag 边框，方便调试
                for i in range(4):
                    p1 = tuple(corners[i])
                    p2 = tuple(corners[(i + 1) % 4])
                    cv2.line(frame, p1, p2, (0, 255, 0), 2)

                center_x = int(tag.center[0])
                center_y = int(tag.center[1])

                cv2.circle(frame, (center_x, center_y), 5, (0, 0, 255), -1)
                cv2.putText(
                    frame,
                    "ID: {}".format(tag.tag_id),
                    (center_x + 10, center_y),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (0, 0, 255),
                    2
                )

                if tag.tag_id == self.target_tag_id:
                    found_target = True

            self.target_pub.publish(Bool(data=found_target))

            ros_image = self.bridge.cv2_to_imgmsg(frame, "bgr8")
            self.image_pub.publish(ros_image)

        except CvBridgeError as e:
            rospy.logerr("CvBridge error: %s", e)
        except Exception as e:
            rospy.logerr("AprilTag detector error: %s", e)

    def spin(self):
        rospy.spin()


if __name__ == "__main__":
    try:
        node = AprilTagDetectorNode()
        node.spin()
    except rospy.ROSInterruptException:
        pass
