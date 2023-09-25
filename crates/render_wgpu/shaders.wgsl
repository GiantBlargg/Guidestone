struct Camera {
	mat: mat4x4<f32>,
}

@group(0) @binding(0) 
var<uniform> camera: Camera;

@vertex 
fn vert(
	@location(0) pos: vec3<f32>, 
	@location(1) normal: vec3<f32>, 
	@location(2) uv: vec2<f32>
) -> VertexOutput {
	let view_pos = (camera.mat * vec4<f32>(pos.x, pos.y, pos.z, 1.0));
	return VertexOutput(pos, normal, uv, view_pos);
}

struct VertexOutput {
	@location(0) world_pos: vec3<f32>,
	@location(1) normal: vec3<f32>,
	@location(2) uv: vec2<f32>,
	@builtin(position) view_pos: vec4<f32>,
}

@fragment
fn frag(in: VertexOutput) -> @location(0) vec4<f32> {
	let normal = normalize(in.normal);
	// let temp = (vec3<f32>(1.0) + normal) * 0.5;
	// return vec4<f32>(temp.x, temp.y, temp.z, 1.0);
	return vec4<f32>(in.uv.x, in.uv.y, 0.0, 1.0);
}


