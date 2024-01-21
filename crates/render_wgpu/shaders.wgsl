struct Camera {
	mat: mat4x4<f32>,
}

struct Object {
	transform: mat4x4<f32>
}

@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var default_sampler: sampler;
@group(1) @binding(0) var base_texture: texture_2d<f32>;
@group(2) @binding(0) var<uniform> object: Object;

struct VertexOutput {
	@location(0) world_pos: vec3<f32>,
	@location(1) normal: vec3<f32>,
	@location(2) uv: vec2<f32>,
	@builtin(position) clip_pos: vec4<f32>,
}

@vertex 
fn vert(
	@location(0) pos: vec3<f32>, 
	@location(1) normal: vec3<f32>, 
	@location(2) uv: vec2<f32>
) -> VertexOutput {
	let clip_pos = (camera.mat * object.transform * vec4<f32>(pos, 1.0));
	return VertexOutput(pos, normal, uv, clip_pos);
}

@fragment
fn frag(in: VertexOutput) -> @location(0) vec4<f32> {
	let normal = normalize(in.normal);
	// let temp = (vec3<f32>(1.0) + normal) * 0.5;
	// return vec4<f32>(temp.x, temp.y, temp.z, 1.0);
	// return vec4<f32>(in.uv.x, in.uv.y, 0.0, 1.0);
	let base_colour = textureSample(base_texture, default_sampler, in.uv);
	return base_colour;
}


