# srrg2_config_visualizer

This package contains the tool for configuration visualization.
It allows you to load/save a configuration, visualize it and change
the parameters of each node.

## Prerequisites

This package requires:
* [srrg2_core](https://github.com/srrg-sapienza/srrg2_core)
* [srrg2_executor](https://github.com/srrg-sapienza/srrg2_executor) - only for running the app

External requirements:
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page)
* [GLUT](http://freeglut.sourceforge.net/docs/install.php)
* [glfw3](https://github.com/glfw/glfw.git)

### How to use
This package loads all the required `srrg2` libraries at runtime.
Therefore, you need to run the [srrg2_executor](https://github.com/srrg-sapienza/srrg2_executor)
app called `auto_dl_finder` to collect them into a file.

Then you can run the `app_node_editor` with `-h` to know the app parameters.
