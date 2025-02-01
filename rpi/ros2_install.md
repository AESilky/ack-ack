sudo apt install -y git colcon python3-rosdep2 vcstool wget python3-flake8-docstrings python3-pip python3-pytest-cov python3-flake8-blind-except python3-flake8-builtins python3-flake8-class-newline python3-flake8-comprehensions python3-flake8-deprecated python3-flake8-import-order python3-flake8-quotes python3-pytest-repeat python3-pytest-rerunfailures python3-vcstools libx11-dev libxrandr-dev libasio-dev libtinyxml2-dev

mkdir -p ~/ros2_jazzy/src

cd ~/ros2_jazzy

vcs import --input https://raw.githubusercontent.com/ros2/ros2/jazzy/ros2.repos src

sudo rm /etc/ros/rosdep/sources.list.d/20-default.list

sudo apt upgrade

sudo rosdep init

rosdep update

rosdep install --from-paths src --ignore-src --rosdistro jazzy -y --skip-keys "fastcdr rti-connext-dds-6.0.1 urdfdom_headers python3-vcstool"

colcon build --symlink-install


sudo setcap -r /usr/bin/python3.11

