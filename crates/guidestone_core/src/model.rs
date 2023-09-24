use crate::{
	fs::ClassicFS,
	math::{Mat4, U8Vec4, UVec2, Vec2, Vec3},
};

mod classic;

struct Vertex {
	pos: Vec3,
	normal: Vec3,
	uv: Vec2,
}

struct Texture {
	size: UVec2,
	has_alpha: bool,
	rgba: Vec<U8Vec4>,
}

#[derive(PartialEq, Eq)]
struct Material {
	texture: u64,
}

struct Node {
	parent: Option<u64>,
	transform: Mat4,
}

struct Mesh {
	first_vertex: u64,
	num_vertices: u64,
	material: u64,
	node: u64,
}

struct CachedModel {
	nodes: Vec<Node>,
	meshes: Vec<Mesh>,
}

pub struct Model {
	vertices: Vec<Vertex>,
	textures: Vec<Texture>,
	materials: Vec<Material>,
	nodes: Vec<Node>,
	meshes: Vec<Mesh>,
}

impl Model {
	pub fn load_classic_model(fs: &ClassicFS, path: &std::path::Path) -> std::io::Result<Self> {
		classic::load_model(fs, path)
	}
}

pub struct ModelCache {
	vertices: Vec<Vertex>,
	textures: Vec<Texture>,
	materials: Vec<Material>,
	models: Vec<CachedModel>,
}

impl ModelCache {
	pub fn push(&mut self, mut model: Model) -> u64 {
		for mat in &mut model.materials {
			if mat.texture >= model.textures.len() as u64 {
				mat.texture = 0;
			} else {
				mat.texture += self.textures.len() as u64;
			}
		}

		for mesh in &mut model.meshes {
			mesh.first_vertex += self.vertices.len() as u64;
			mesh.material += self.materials.len() as u64;
		}

		self.vertices.append(&mut model.vertices);
		self.textures.append(&mut model.textures);
		self.materials.append(&mut model.materials);

		self.models.push(CachedModel {
			nodes: model.nodes,
			meshes: model.meshes,
		});

		self.models.len() as u64 - 1
	}
}

impl Default for ModelCache {
	fn default() -> Self {
		Self {
			vertices: Vec::new(),
			textures: vec![Texture {
				size: UVec2::new(1, 1),
				has_alpha: false,
				rgba: vec![U8Vec4::new(255, 255, 255, 255)],
			}],
			materials: Vec::new(),
			models: Vec::new(),
		}
	}
}
