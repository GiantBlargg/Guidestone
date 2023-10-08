use crate::math::{Mat4, U8Vec4, UVec2, Vec2, Vec3};

#[repr(C)]
pub struct Vertex {
	pub pos: Vec3,
	pub normal: Vec3,
	pub uv: Vec2,
}

#[derive(PartialEq, Eq)]
pub struct Material<Texture> {
	pub texture: Texture,
}

pub struct Node {
	pub parent: Option<u32>,
	pub transform: Mat4,
}

pub struct Mesh {
	pub first_vertex: u32,
	pub num_vertices: u32,
	pub material: u32,
	pub node: u32,
}
pub struct Model {
	pub vertices: Vec<Vertex>,
	pub materials: Vec<Material<Option<String>>>,
	pub nodes: Vec<Node>,
	pub meshes: Vec<Mesh>,
}

pub struct Texture {
	pub size: UVec2,
	pub rgba: Vec<U8Vec4>,
}
