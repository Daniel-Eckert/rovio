# README #


### Get all git repositories & submodules s###
```
#!command

git clone git@bitbucket.org:bloesch/rovio.git
git submodule init
git submodule updates
```
### Install without opengl scene ###
Dependencies: ros, kindr, lightweight_filtering
```
#!command

catkin build rovio --cmake-args -DCMAKE_BUILD_TYPE=Release
```

### Install with opengl scene ###
Dependencies: ros, kindr, lightweight_filtering, opengl, glut, glew
```
#!command

catkin build rovio --cmake-args -DCMAKE_BUILD_TYPE=Release -DMAKE_SCENE=ON
```
