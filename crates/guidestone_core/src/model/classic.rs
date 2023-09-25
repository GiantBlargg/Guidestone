use std::{
	collections::HashMap,
	ffi::CString,
	io::{self},
	path::{Path, PathBuf},
};

use log::warn;

use crate::{
	fs::{ClassicFS, GetFromRead},
	math::{U8Vec4, UVec2, Vector},
};

use super::{Material, Model, Node, Texture, Vertex};

#[allow(dead_code, non_snake_case)]
mod geo {
	use crate::{
		fs::FromRead,
		math::{Mat4, U8Vec4, Vec2, Vec3},
	};

	#[derive(FromRead)]
	pub struct Header {
		pub identifier: [u8; 8],   // File identifier.
		pub version: u32,          // File version.
		pub pName: u32,            // Offset to a file name.
		pub fileSize: u32,         // File size (in bytes), not counting this header.
		pub localSize: u32,        // Object size (in bytes).
		pub nPublicMaterials: u32, // Number of public materials.
		pub nLocalMaterials: u32,  // Number of local materials.
		pub oPublicMaterial: u32,  // list of public materials
		pub oLocalMaterial: u32,   // list of local materials
		pub nPolygonObjects: u32,  // Number of polygon objects.
		pub reserved: [u8; 24],    // Reserved for future use.
	}

	#[derive(FromRead)]
	pub struct PolygonObject {
		pub pName: u32,          // Name for animation.
		pub flags: u8,           // General flags (see above)
		pub iObject: u8, // fixed up at load time so we know what object index we have when recursively processing
		pub nameCRC: u16, // 16-bit CRC of name (!!!!no room for 32 right now - make room next version)
		pub nVertices: u32, // Number of vertices in vertex list for this object.
		pub nFaceNormals: u32, // Number of face normals for this object.
		pub nVertexNormals: u32, // Number of vertex normals for this object.
		pub nPolygons: u32, // Number of polygons in this object.
		pub pVertexList: u32, // Offset to the vertex list in this object.
		pub pNormalList: u32, // Offset to the normal list in this object.
		pub pPolygonList: u32, // Offset to the polygon list in this object.
		pub pMother: u32, // link to parent object
		pub pDaughter: u32, // link to child object
		pub pSister: u32, // link to sibling object
		pub localMatrix: Mat4,
	}

	#[derive(FromRead)]
	pub struct PolyEntry {
		pub iFaceNormal: u32,  // Index into the face normal list.
		pub iVertex: [u16; 3], // Indicies to the vertices of the polygon.
		pub iMaterial: u16,    // Index into material list.
		pub uv: [Vec2; 3],
		pub flags: u16, // Flags for this polygon.
		pub reserved: [u8; 2],
	}

	#[derive(FromRead)]
	pub struct VertexEntry {
		pub pos: Vec3,
		pub iVertexNormal: u32, // Index into the point normal list.
	}

	#[derive(FromRead)]
	pub struct MaterialFlags(u16);

	bitflags::bitflags! {
		impl MaterialFlags: u16 {
			const Smoothing = 2;
			const DoubleSided = 8;
			const SelfIllum = 64;

			const _ = !0;
		}
	}

	#[derive(FromRead)]
	pub struct MaterialEntry {
		pub pName: u32,              // Offset to name of material (may be a CRC32).
		pub ambient: U8Vec4,         // Ambient color information.
		pub diffuse: U8Vec4,         // Diffuse color information.
		pub specular: U8Vec4,        // Specular color information.
		pub kAlpha: f32,             // Alpha blending information.
		pub texture: u32,            // Pointer to texture information (or CRC32).
		pub flags: MaterialFlags,    // Flags for this material.
		pub nFullAmbient: u8,        // Number of self-illuminating colors in texture.
		pub bTexturesRegistered: u8, // Set to TRUE when some textures have been registered.
		pub textureNameSave: u32,    // the name of the texture, after the texture has been registered
	}
}

#[allow(dead_code, non_snake_case)]
mod lif {
	use crate::fs::FromRead;

	#[derive(FromRead)]
	pub struct TextureFlags(u32);

	bitflags::bitflags! {
		impl TextureFlags: u32 {
			const Paletted = 0x02;   // texture uses a palette
			const Alpha = 0x08;      // alpha channel image
			const TeamColor0 = 0x10; // team color flags
			const TeamColor1 = 0x20;

			const _ = !0;
		}
	}

	#[derive(FromRead)]
	pub struct Header {
		pub ident: [u8; 8],      // compared to "Willy 7"
		pub version: u32,        // version number
		pub flags: TextureFlags, // to plug straight into texreg flags
		pub width: u32,
		pub height: u32,     // dimensions of image
		pub paletteCRC: u32, // a CRC of palettes for fast comparison
		pub imageCRC: u32,   // crc of the unquantized image
		pub data: u32,       // pointer to actual image
		pub palette: u32,    // pointer to palette for this image
		pub teamEffect0: u32,
		pub teamEffect1: u32, // pointers to palettes of team color effect
	}

	pub const PALETTE_SIZE: u32 = 256;
}

struct Surface {
	emissive: bool,
	double_sided: bool,
	node: u32,
	texture: u32,
	vertices: Vec<Vertex>,
}

impl Surface {
	fn can_merge(&self, other: &Surface) -> bool {
		self.emissive == other.emissive
			&& self.double_sided == other.double_sided
			&& self.node == other.node
			&& self.texture == other.texture
	}

	fn merge(&mut self, mut other: Surface) {
		assert!(self.can_merge(&other));
		self.vertices.append(&mut other.vertices);
	}
}

#[derive(Default)]
struct Patch {
	triangle: u32,
	vertex: u8,
	pos: Vector<Option<(f32, f32)>, 3>,
	normal: Vector<Option<(f32, f32)>, 3>,
	uv: Vector<Option<(f32, f32)>, 2>,
}

fn patch_vector<T: Copy + PartialEq, const N: usize>(
	target: &mut Vector<T, N>,
	patch: &Vector<Option<(T, T)>, N>,
) {
	for i in 0..N {
		if let Some((old, new)) = patch[i] {
			if target[i] != old {
				warn!(target:"model::classic::patch", "Unexpected old value! Patching anyway...");
			}
			target[i] = new;
		}
	}
}

struct PatchSet(HashMap<PathBuf, Vec<Patch>>);

fn patch(path: &Path, triangles: &mut [Surface]) {
	let patch_set = {
		let patch_set = [(
			"r1/resourcecollector/rl0/lod0/resourcecollector.peo",
			vec![
				Patch {
					triangle: 177,
					vertex: 2,
					uv: Vector::<_, 2>::new(Some((0.49725038, 0.25)), None),
					..Default::default()
				},
				Patch {
					triangle: 179,
					vertex: 2,
					uv: Vector::<_, 2>::new(Some((0.49725038, 0.25)), None),
					..Default::default()
				},
			],
		)];
		PatchSet(
			patch_set
				.into_iter()
				.map(|(path, patches)| (path.into(), patches))
				.collect(),
		)
	};

	if let Some(patches) = patch_set.0.get(path) {
		for patch in patches {
			let vertex = &mut triangles[patch.triangle as usize].vertices[patch.vertex as usize];
			patch_vector(&mut vertex.pos, &patch.pos);
			patch_vector(&mut vertex.normal, &patch.normal);
			patch_vector(&mut vertex.uv, &patch.uv);
		}
	}
}

pub fn load_model(fs: &ClassicFS, path: &Path) -> io::Result<Model> {
	let mut geo = fs.load(path)?;

	let header: geo::Header = geo.get()?;

	// Read this before we change the cursor
	let polygon_objects: Vec<geo::PolygonObject> = geo.get_vec(header.nPolygonObjects)?;

	let geo_materials: Vec<geo::MaterialEntry> = geo.get_vec_at(
		header.oLocalMaterial,
		header.nPublicMaterials + header.nLocalMaterials,
	)?;

	let mut texture_names = Vec::new();
	let mut texture_lookup = Vec::new();

	for mat in &geo_materials {
		if mat.texture != 0 {
			let tex_name: CString = geo.get_at(mat.texture)?;
			let tex_id = texture_names.iter().position(|t| *t == tex_name);
			if let Some(id) = tex_id {
				texture_lookup.push(id as u32);
			} else {
				texture_lookup.push(texture_names.len() as u32);
				texture_names.push(tex_name);
			}
		} else {
			texture_lookup.push(u32::MAX);
		}
	}

	let mut nodes = Vec::new();
	let mut triangles = Vec::new();

	for po in polygon_objects {
		let parent = if po.pMother != 0 {
			// (po.pMother - sizeof(Geo::Header)) / sizeof(Geo::PolygonObject)
			Some((po.pMother - 68) / 112)
		} else {
			None
		};
		let node = nodes.len() as u32;
		nodes.push(Node {
			parent,
			transform: po.localMatrix,
		});

		let poly_entries: Vec<geo::PolyEntry> =
			geo.get_vec_at(po.pPolygonList, po.nPolygons).unwrap();

		for pe in poly_entries {
			let mat = &geo_materials[pe.iMaterial as usize];
			let smooth = mat.flags.contains(geo::MaterialFlags::Smoothing);
			let double_sided = mat.flags.contains(geo::MaterialFlags::DoubleSided);
			let emissive = mat.flags.contains(geo::MaterialFlags::SelfIllum);

			let texture = texture_lookup[pe.iMaterial as usize];
			let vertices = (0..3)
				.map(|i| {
					let vertex: geo::VertexEntry =
						geo.get_at(po.pVertexList + u32::from(pe.iVertex[i]) * 16)?;

					let normal = {
						let normal_index = if smooth {
							vertex.iVertexNormal
						} else {
							pe.iFaceNormal
						};
						geo.get_at(po.pNormalList + normal_index * 16)
					}?;

					let uv = pe.uv[i];

					Ok(Vertex {
						pos: vertex.pos,
						normal,
						uv,
					})
				})
				.collect::<io::Result<_>>()?;

			triangles.push(Surface {
				emissive,
				double_sided,
				node,
				texture,
				vertices,
			});
		}
	}

	patch(path, &mut triangles);

	let textures = texture_names
		.into_iter()
		.map(|t| {
			let texture_path = path
				.parent()
				.unwrap()
				.join(t.to_str().unwrap())
				.with_extension("lif");

			let mut lif = fs.load(texture_path)?;

			let texture_header: lif::Header = lif.get()?;

			let size = UVec2::new(texture_header.width, texture_header.height);
			let has_alpha = texture_header.flags.contains(lif::TextureFlags::Paletted);

			let rgba = if texture_header.flags.contains(lif::TextureFlags::Paletted) {
				let indicies: Vec<u8> = lif.get_vec_at(texture_header.data, size.x * size.y)?;
				let palette: Vec<U8Vec4> =
					lif.get_vec_at(texture_header.palette, lif::PALETTE_SIZE)?;

				indicies.into_iter().map(|i| palette[i as usize]).collect()
			} else {
				todo!()
			};

			io::Result::Ok(Texture {
				size,
				has_alpha,
				rgba,
			})
		})
		.collect::<io::Result<_>>()?;

	triangles.sort_by(|a, b| {
		(a.node.cmp(&b.node))
			.then_with(|| a.texture.cmp(&b.texture))
			.then_with(|| a.emissive.cmp(&b.emissive))
			.then_with(|| a.double_sided.cmp(&b.double_sided))
	});

	let mut surfaces: Vec<Surface> = Vec::new();

	for t in triangles {
		match surfaces.last_mut() {
			Some(merge) if merge.can_merge(&t) => merge.merge(t),
			_ => surfaces.push(t),
		}
	}

	let mut vertices = Vec::new();
	let mut materials = Vec::new();
	let mut meshes = Vec::new();

	for mut s in surfaces {
		let mat = Material { texture: s.texture };
		let mat_id = materials.iter().position(|m| *m == mat).unwrap_or_else(|| {
			let id = materials.len();
			materials.push(mat);
			id
		}) as u32;
		meshes.push(super::Mesh {
			first_vertex: vertices.len() as u32,
			num_vertices: s.vertices.len() as u32,
			material: mat_id,
			node: s.node,
		});
		vertices.append(&mut s.vertices);
	}

	Ok(Model {
		vertices,
		textures,
		materials,
		nodes,
		meshes,
	})
}
