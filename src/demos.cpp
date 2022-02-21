#include "Object4.hpp"

/**
 * Builds a 4D complex which is basically a hypercube sliced in the X, Z and W
 * directions, ie 8 hypercubes.
 */
void complexDemo(Object4 &scene)
{
    // The scene already has a room in it
    // 21+X
    {
        Object4 &room2 = scene.addChild(scene[0]);
        room2.removeChild(5);
        room2.scale(Empty::math::vec4(-1, 1, 1, 1));
        room2.pos(0) = 1;
    }
    // 31+Z
    {
        Object4 &room3 = scene.addChild(scene[0]);
        room3.removeChild(6);
        room3.scale(Empty::math::vec4(1, 1, -1, 1));
        room3.pos(2) = 1;
    }
    // 41+W
    {
        Object4 &room4 = scene.addChild(scene[0]);
        room4.removeChild(7);
        room4.scale(Empty::math::vec4(1, 1, 1, -1));
        room4.pos(3) = 1;
    }
    // 52+Z
    {
        Object4 &room5 = scene.addChild(scene[0]);
        room5.removeChild(5, 6);
        room5.scale(Empty::math::vec4(-1, 1, -1, 1));
        room5.pos(0) = room5.pos(2) = 1;
    }
    // 62+W
    {
        Object4 &room6 = scene.addChild(scene[0]);
        room6.removeChild(5, 7);
        room6.scale(Empty::math::vec4(-1, 1, 1, -1));
        room6.pos(0) = room6.pos(3) = 1;
    }
    // 73+W
    {
        Object4 &room7 = scene.addChild(scene[0]);
        room7.removeChild(6, 7);
        room7.scale(Empty::math::vec4(1, 1, -1, -1));
        room7.pos(2) = room7.pos(3) = 1;
    }
    // 85+W
    {
        Object4 &room8 = scene.addChild(scene[0]);
        room8.removeChild(5, 6, 7);
        room8.scale(Empty::math::vec4(-1, 1, -1, -1));
        room8.pos = Empty::math::vec4(1, 0, 1, 1);
    }
}
