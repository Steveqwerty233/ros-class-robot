# ros-class-robot

=================== hello ros
source ~/lzr_ros_class_ws/devel/setup.bash
rsync -avzP bcsh@192.168.1.140:/home/bcsh/lzr_ros_class_ws/src/camera_and_arm/ /home/steve/lzr_ros_class_ws/src

roscore

rosrun hello_ros talker_cpp
rosrun hello_ros listener_cpp

rosrun hello_ros talker.py
rosrun hello_ros listener.py

rosrun hello_ros msg_publisher_node
rosrun hello_ros msg_subscriber_node

roslaunch hello_ros bringup_topic.launch

rosrun hello_ros ros_server_node
rosrun hello_ros ros_client_node

rosrun hello_ros ros_server.py
rosrun hello_ros ros_client.py

rosrun hello_ros ros_action_server
rosrun hello_ros ros_action_client

rosrun hello_ros ros_action_server.py
rosrun hello_ros ros_action_server.py

rosrun hello_ros ros_param
rosrun hello_ros ros_param.py

roslaunch hello_ros parameter.launch

rosrun hello_ros dynamic_reconfigure_node

rosrun turtlesim turtlesim_node
rosrun rqt_reconfigure rqt_reconfigure
rosrun hello_ros ros_dynamic_speed_node


=================== 1x1 square

roslaunch upros_bringup bringup_w2a.launch

rosrun square_move square_move

rosrun hello_ros ros_dynamic_speed_node

=================== sensors

rosrun sensors bump_react_node

rosrun sensors front_tof_avoid

rosrun sensors imu_turn_180

=================== urdf and gazebo

roslaunch lzr_robot_description display.launch // rviz

roslaunch lzr_robot_description gazebo.launch //gazebo

rosrun teleop_twist_keyboard teleop_twist_keyboard.py

roslaunch lzr_robot_description arm_only_gazebo.launch // robot arm

rosrun lzr_robot_description arm_demo.py

=================== camera and arm
source ~/lzr_ros_class_ws/devel/setup.bash
roslaunch upros_bringup bringup_w2a.launch

rosrun rqt_image_view rqt_image_view

rosrun camera_and_arm get_ros_image.py

rosrun upros_cv color_choose.py

rosrun camera_and_arm follow_line.py
rostopic pub -1 /enable_move std_msgs/Int16 "data: 1"

rosrun camera_and_arm gesture_movement.py

rosrun camera_and_arm apriltag_follow.py

roslaunch upros_arm recognize_apriltag.launch
rosrun camera_and_arm tag_grab_node


