use crate::{
	fs::ClassicFS,
	math::{Mat4, U8Vec4, UVec2, Vec2, Vec3},
};

mod classic;

#[repr(C)]
pub struct Vertex {
	pub pos: Vec3,
	pub normal: Vec3,
	pub uv: Vec2,
}

pub struct Texture {
	pub size: UVec2,
	pub has_alpha: bool,
	pub rgba: Vec<U8Vec4>,
}

#[derive(PartialEq, Eq)]
pub struct Material {
	pub texture: u32,
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

pub struct CachedModel {
	pub nodes: Vec<Node>,
	pub meshes: Vec<Mesh>,
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
	pub vertices: Vec<Vertex>,
	pub textures: Vec<Texture>,
	pub materials: Vec<Material>,
	pub models: Vec<CachedModel>,
}

impl ModelCache {
	pub fn push(&mut self, mut model: Model) -> u32 {
		for mat in &mut model.materials {
			if mat.texture >= model.textures.len() as u32 {
				mat.texture = 0;
			} else {
				mat.texture += self.textures.len() as u32;
			}
		}

		for mesh in &mut model.meshes {
			mesh.first_vertex += self.vertices.len() as u32;
			mesh.material += self.materials.len() as u32;
		}

		self.vertices.append(&mut model.vertices);
		self.textures.append(&mut model.textures);
		self.materials.append(&mut model.materials);

		self.models.push(CachedModel {
			nodes: model.nodes,
			meshes: model.meshes,
		});

		self.models.len() as u32 - 1
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
