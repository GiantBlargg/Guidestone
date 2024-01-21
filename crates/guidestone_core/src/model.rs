use std::collections::{HashMap, HashSet};

use glam::UVec2;

use hw_import::HWImporter;

pub use common::model::*;

pub struct CachedModel {
	pub nodes: Vec<Node>,
	pub meshes: Vec<Mesh>,
}

pub struct ModelCache {
	pub vertices: Vec<Vertex>,
	pub textures: Vec<Texture>,
	pub materials: Vec<Material<u32>>,
	pub models: Vec<CachedModel>,
}

impl ModelCache {
	pub fn load(hw_import: &mut HWImporter, models: Vec<String>) -> Self {
		let models: Vec<_> = models
			.into_iter()
			.map(|path| hw_import.load_model(&path).unwrap())
			.collect();

		let textures = models
			.iter()
			.flat_map(|model| {
				model
					.materials
					.iter()
					.filter_map(|mat| mat.texture.to_owned())
			})
			.collect::<HashSet<_>>()
			.into_iter()
			.map(|tex_path| {
				let texture = hw_import.load_texture(&tex_path).unwrap();
				(tex_path, texture)
			})
			.collect();

		Self::new(models, textures)
	}

	fn new(models: Vec<Model>, textures: HashMap<String, Texture>) -> Self {
		let (mut textures, texture_index): (Vec<_>, HashMap<_, _>) = textures
			.into_iter()
			.enumerate()
			.map(|(i, (name, tex))| (tex, (name, i as u32)))
			.unzip();

		let default_texture = textures.len() as u32;
		textures.push(Texture {
			size: UVec2::new(1, 1),
			rgba: vec![[255, 255, 255, 255]],
		});

		let mut vertices = Vec::new();
		let mut materials: Vec<Material<Option<String>>> = Vec::new();
		let mut cached_models = Vec::new();

		for mut model in models {
			let mats: Vec<u32> = model
				.materials
				.into_iter()
				.map(|mat| {
					(match materials.iter().position(|m| *m == mat) {
						Some(n) => n,
						None => {
							let n = materials.len();
							materials.push(mat);
							n
						}
					}) as u32
				})
				.collect();

			for mesh in &mut model.meshes {
				mesh.first_vertex += vertices.len() as u32;
				mesh.material = mats[mesh.material as usize];
			}

			vertices.append(&mut model.vertices);

			cached_models.push(CachedModel {
				nodes: model.nodes,
				meshes: model.meshes,
			})
		}

		let materials = materials
			.into_iter()
			.map(|mat| Material::<u32> {
				texture: mat
					.texture
					.as_ref()
					.map(|t| texture_index[t])
					.unwrap_or(default_texture),
			})
			.collect();

		Self {
			vertices,
			textures,
			materials,
			models: cached_models,
		}
	}
}
