#pragma once

#include "math.hpp"
#include "types.hpp"

struct Camera {
	vec3 eye;
	vec3 target;
	vec3 up = {0, 0, 1};
	f32 fov = degToRad(75.0);
	f32 near_clip = 15.0;
};

class CameraSystem {
	Camera main_camera;

  public:
	const Camera& getActiveCamera() { return main_camera; }
};
