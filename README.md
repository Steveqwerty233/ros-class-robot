# ros-class-robot

=================== hello ros
source ~/lzr_ros_class_ws/devel/setup.bash

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
