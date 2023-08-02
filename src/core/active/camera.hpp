#pragma once

#include "input.hpp"
#include "math.hpp"
#include "types.hpp"
#include <algorithm>

struct Camera {
	f32 angle = 0;
	f32 declination = 0;
	f32 distance;
	vec3 eye;
	vec3 target;
	vec3 old_target;
	vec3 up = {0, 0, 1};
	f32 fov = degToRad(75.0);
	f32 near_clip = 15.0;
	f32 min_distance = 50;
	f32 max_distance = 14500;

  private:
	void angle_verify() {
		constexpr f32 tau = 2 * std::numbers::pi;
		if (angle < 0)
			angle += tau;
		else if (angle > tau)
			angle -= tau;
	}

	void declination_verify() { declination = std::clamp(declination, degToRad(-85.0f), degToRad(85.0f)); }

  public:
	Camera(f32 dist) : distance(dist) { set_eye_position(); }

	void rot_angle(f32 ang) {
		angle += ang;
		angle_verify();
	}
	void rot_declination(f32 dec) {
		declination += dec;
		declination_verify();
	}

	void zoom(f32 zoom_factor) {
		distance *= zoom_factor;
		distance = std::clamp(distance, min_distance, max_distance);
	}

	void set_eye_position() {
		angle_verify();
		declination_verify();

		eye = target + (mat3::rotateZ(angle) * mat3::rotateY(declination) * vec3{distance, 0, 0});
	}

	void control(const Input::State& input_state) {
		zoom(1.0f - 0.1 * input_state.scroll);
		constexpr f32 rot = degToRad(0.1);
		rot_declination(rot * input_state.mouse_relative.y);
		rot_angle(rot * input_state.mouse_relative.x);
	}
};

class CameraSystem {
  public:
	Camera main_camera = {6000};

	const Camera& getActiveCamera() { return main_camera; }
};
