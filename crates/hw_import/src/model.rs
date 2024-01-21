use std::{
	collections::HashMap,
	io::{self, Read, Seek, SeekFrom},
	num::NonZeroU32,
};

use binrw::{binread, file_ptr::NonZeroFilePtr32, BinRead, FilePtr32, NullString};
use bitflags::bitflags;
use glam::{vec2, Mat4, Vec2, Vec3};

use common::model::{Material, Mesh, Model, Node, Texture, Vertex};

use crate::big::HwFs;

#[derive(BinRead)]
enum GeoID {
	#[br(magic = b"RMF97ba\0")]
	RMF97ba,
	#[br(magic = b"RMF99ba\0")]
	RMF99ba,
}

#[allow(dead_code)]
#[binread]
#[br(little)]
struct Geo {
	identifier: GeoID, // File identifier.
	#[br(temp, assert(version == 0x402))]
	version: u32, // File version.

	#[br(try, parse_with = NonZeroFilePtr32::parse, pad_after = 8)]
	name: Option<NullString>, // Offset to a file name.
	// fileSize: u32, // File size (in bytes), not counting this header.
	// localSize: u32, // Object size (in bytes).
	#[br(temp)]
	nPublicMaterials: u32, // Number of public materials.
	#[br(temp)]
	nLocalMaterials: u32, // Number of local materials.
	#[br(temp)]
	oPublicMaterial: u32, // list of public materials
	#[br(temp, assert(oPublicMaterial == oLocalMaterial))]
	oLocalMaterial: u32, // list of local materials
	#[br(
		count = nPublicMaterials + nLocalMaterials,
		seek_before = SeekFrom::Start(oLocalMaterial.into()),
		restore_position
	)]
	materials: Vec<MaterialEntry>,

	#[br(temp)]
	nPolygonObjects: u32, // Number of polygon objects.
	#[br(pad_before = 24, count = nPolygonObjects)]
	polygon_objects: Vec<PolygonObject>,
}

#[binread]
struct MaterialFlags(u16);

bitflags! {
	impl MaterialFlags: u16 {
		const Smoothing = 2;
		const DoubleSided = 8;
		const SelfIllum = 64;

		const _ = !0;
	}
}

#[binread]
#[allow(dead_code)]
struct MaterialEntry {
	#[br(try, parse_with = NonZeroFilePtr32::parse)]
	name: Option<NullString>, // Offset to name of material (may be a CRC32).
	ambient: [u8; 4],  // Ambient color information.
	diffuse: [u8; 4],  // Diffuse color information.
	specular: [u8; 4], // Specular color information.
	alpha: f32,        // Alpha blending information.
	#[br(try, parse_with = NonZeroFilePtr32::parse)]
	texture: Option<NullString>, // Pointer to texture information (or CRC32).
	#[br(pad_after = 6)]
	flags: MaterialFlags, // Flags for this material.
	                   // nFullAmbient: u8,  // Number of self-illuminating colors in texture.
	                   // bTexturesRegistered: u8, // Set to TRUE when some textures have been registered.
	                   // textureNameSave: u32, // the name of the texture, after the texture has been registered
}

#[binread]
struct VertexEntry {
	pos: [f32; 3],
	normal_index: u32,
}

#[binread]
pub struct PolyEntry {
	face_normal_index: u32, // Index into the face normal list.
	vertex_index: [u16; 3], // Indicies to the vertices of the polygon.
	material_index: u16,    // Index into material list.
	#[br(pad_after = 4)]
	uv: [[f32; 2]; 3],
	// flags: u16, // Flags for this polygon.
	// reserved: [u8; 2],
}

#[allow(dead_code)]
#[binread]
struct PolygonObject {
	#[br(try, parse_with = NonZeroFilePtr32::parse, pad_after = 4)]
	name: Option<NullString>, // Name for animation.
	// flags: u8,    // General flags (see above)
	// iObject: u8, // fixed up at load time so we know what object index we have when recursively processing
	// nameCRC: u16, // 16-bit CRC of name (!!!!no room for 32 right now - make room next version)
	#[br(temp)]
	nVertices: u32, // Number of vertices in vertex list for this object.
	#[br(temp)]
	nFaceNormals: u32, // Number of face normals for this object.
	#[br(temp)]
	nVertexNormals: u32, // Number of vertex normals for this object.
	#[br(temp)]
	nPolygons: u32, // Number of polygons in this object.
	#[br(temp)]
	pVertexList: u32, // Offset to the vertex list in this object.
	#[br(temp)]
	pNormalList: u32, // Offset to the normal list in this object.
	#[br(temp)]
	pPolygonList: u32, // Offset to the polygon list in this object.

	#[br(count = nVertices, seek_before = SeekFrom::Start(pVertexList.into()), restore_position)]
	vertices: Vec<VertexEntry>,
	#[br(count = nFaceNormals + nVertexNormals, seek_before = SeekFrom::Start(pNormalList.into()), restore_position)]
	normals: Vec<VertexEntry>,
	#[br(count = nPolygons, seek_before = SeekFrom::Start(pPolygonList.into()), restore_position)]
	polygons: Vec<PolyEntry>,

	#[br(try, map = |x: Option<NonZeroU32>| Some((x?.get() - 68) / 112))]
	mother: Option<u32>, // link to parent object
	#[br(try, map = |x: Option<NonZeroU32>| Some((x?.get() - 68) / 112))]
	daughter: Option<u32>, // link to child object
	#[br(try, map = |x: Option<NonZeroU32>| Some((x?.get() - 68) / 112))]
	sister: Option<u32>, // link to sibling object
	#[br(map = |m:[f32;16]|{Mat4::from_cols_array(&m)})]
	local_matrix: Mat4,
}

#[binread]
#[br(little, magic = b"Event13\0")]
struct LL {
	#[br(temp, assert(version == 0x104))]
	version: u32, //version number
	#[br(temp, pad_after = 12)]
	nElements: u32, //number of textures listed
	// stringLength: u32,  //length of all strings
	// sharingLength: u32, //length of all offsets
	// totalLength: u32,   //total length of file, this header not included
	#[br(count = nElements, args{inner: 28 + (32 * nElements)})]
	lifs: Vec<LLElement>,
}

#[binread]
#[br(import_raw(strings_offset: u32))]
struct LLElement {
	#[br(args{offset: strings_offset.into()}, parse_with = FilePtr32::parse)]
	name: NullString, //name of texture, an offset from start of string block
	size: [u32; 2], //size of texture
	#[br(pad_after = 12)]
	flags: TextureFlags, //flags for things like alpha and luminance map
	// imageCRC: u32,      //crc of the unquantized image
	// nShared: i32,       //number of images which share this one
	// sharedTo: u32,      //list of indices of images which share this one
	#[br(map(|x| if x == u32::MAX {None} else {Some(x)}))]
	shared_from: Option<u32>, //image this one is shared from, or -1 if none
}

#[binread]
#[br(little, magic = b"Willy 7\0")]
struct Lif {
	#[br(temp, assert(version == 0x104))]
	version: u32,
	flags: TextureFlags, // to plug straight into texreg flags
	size: [u32; 2],      // dimensions of image
	// paletteCRC: u32,      // a CRC of palettes for fast comparison
	// imageCRC: u32,        // crc of the unquantized image
	#[br(pad_before = 8)]
	data: u32, // pointer to actual image
	palette: u32, // pointer to palette for this image
	#[allow(dead_code)]
	team_effect: [u32; 2], // pointers to palettes of team color effect
}

#[derive(BinRead, PartialEq, Eq, Clone, Copy)]
struct TextureFlags(u32);

bitflags! {
	impl TextureFlags: u32 {
		const Paletted = 0x02;   // texture uses a palette
		const Alpha = 0x08;      // alpha channel image
		const TeamColor0 = 0x10; // team color flags
		const TeamColor1 = 0x20;

		const _ = !0;
	}
}

#[allow(dead_code)]
struct TextureMeta {
	name: String,
	size: [u32; 2],
	palette: bool,
	alpha: bool,
	team_colour: bool,
}

impl TextureMeta {
	fn load_default(fs: &mut HwFs) -> HashMap<String, Self> {
		let mut ll_file = fs.open("textures.ll").unwrap();
		let ll = LL::read(&mut ll_file).unwrap();
		TextureMeta::new(&ll)
	}

	fn new(ll: &LL) -> HashMap<String, Self> {
		(ll.lifs.iter())
			.filter(|l| l.name.len() > 0)
			.map(|l| {
				let local_name = l.name.to_string();
				let shared_from = l.shared_from.map(|s| &ll.lifs[s as usize]);
				if let Some(s) = shared_from {
					assert!(l.size == s.size);
					assert!(s.flags.contains(l.flags));
				}
				let true_name = shared_from
					.map(|s| s.name.to_string())
					.unwrap_or_else(|| local_name.clone());
				(
					local_name.to_ascii_lowercase(),
					TextureMeta {
						name: true_name,
						size: l.size,
						palette: l.flags.contains(TextureFlags::Paletted),
						alpha: l.flags.contains(TextureFlags::Alpha),
						team_colour: (l.flags)
							.intersects(TextureFlags::TeamColor0 | TextureFlags::TeamColor1),
					},
				)
			})
			.collect()
	}
}

struct Surface {
	node: u32,
	texture: Option<String>,
	self_illum: bool,
	double_sided: bool,
	alpha: bool,
	team_colour: bool,
	vertices: Vec<Vertex>,
}
impl Surface {
	fn can_merge(&self, other: &Surface) -> bool {
		self.node == other.node
			&& self.texture == other.texture
			&& self.self_illum == other.self_illum
			&& self.double_sided == other.double_sided
			&& self.alpha == other.alpha
			&& self.team_colour == other.team_colour
	}

	fn merge(&mut self, mut other: Surface) {
		assert!(self.can_merge(&other));
		self.vertices.append(&mut other.vertices);
	}
}

pub struct ImportModels {
	texture_list: HashMap<String, TextureMeta>,
}
impl ImportModels {
	pub fn new(fs: &mut HwFs) -> Self {
		let texture_list = TextureMeta::load_default(fs);
		Self { texture_list }
	}

	pub fn load_model(&mut self, fs: &mut HwFs, path: &str) -> io::Result<Model> {
		let geo = Geo::read(&mut fs.open(path).unwrap()).unwrap();
		let folder = {
			let mut path_vec: Vec<_> = path.split(['\\', '/']).collect();
			path_vec.pop();
			path_vec.join("\\")
		};

		let mut nodes = Vec::new();
		let mut triangles = Vec::new();

		for po in geo.polygon_objects {
			let node = nodes.len() as u32;
			nodes.push(Node {
				parent: po.mother,
				transform: po.local_matrix,
			});

			for pe in po.polygons {
				let mat = &geo.materials[pe.material_index as usize];
				let smooth = mat.flags.contains(MaterialFlags::Smoothing);
				let double_sided = mat.flags.contains(MaterialFlags::DoubleSided);
				let self_illum = mat.flags.contains(MaterialFlags::SelfIllum);

				let texture_meta = (mat.texture.as_ref()).map(|tex| {
					let texture = format!("{}\\{}", folder, tex).to_lowercase();
					self.texture_list.get(&texture).unwrap()
				});
				let texture = texture_meta.map(|t| t.name.clone());
				let alpha = texture_meta.map(|m| m.alpha).unwrap_or(false);
				let team_colour = texture_meta.map(|m| m.team_colour).unwrap_or(false);

				let vertices = (0..3)
					.map(|i| {
						let vertex: &VertexEntry = &po.vertices[usize::from(pe.vertex_index[i])];
						let normal = {
							let normal_index = if smooth {
								vertex.normal_index
							} else {
								pe.face_normal_index
							};
							po.normals[normal_index as usize].pos
						};

						let uv = pe.uv[i];

						Vertex {
							pos: vertex.pos.into(),
							normal: normal.into(),
							uv: uv.into(),
						}
					})
					.collect();

				triangles.push(Surface {
					node,
					texture,
					self_illum,
					double_sided,
					alpha,
					team_colour,
					vertices,
				})
			}
		}

		patch(path, &mut triangles);

		let surfaces = if true {
			triangles.sort_by(|a, b| {
				(a.node.cmp(&b.node))
					.then_with(|| a.texture.cmp(&b.texture))
					.then_with(|| a.self_illum.cmp(&b.self_illum))
					.then_with(|| a.double_sided.cmp(&b.double_sided))
					.then_with(|| a.alpha.cmp(&b.alpha))
					.then_with(|| a.team_colour.cmp(&b.team_colour))
			});

			let mut surfaces: Vec<Surface> = Vec::new();

			for t in triangles {
				match surfaces.last_mut() {
					Some(merge) if merge.can_merge(&t) => merge.merge(t),
					_ => surfaces.push(t),
				}
			}

			surfaces
		} else {
			triangles
		};

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
			meshes.push(Mesh {
				first_vertex: vertices.len() as u32,
				num_vertices: s.vertices.len() as u32,
				material: mat_id,
				node: s.node,
			});
			vertices.append(&mut s.vertices);
		}

		Ok(Model {
			vertices,
			materials,
			nodes,
			meshes,
		})
	}

	pub fn load_texture(&mut self, fs: &mut HwFs, path: &str) -> io::Result<Texture> {
		let mut lif_file = fs.open(&format!("{}.lif", path)).unwrap();
		let lif_header = Lif::read(&mut lif_file).unwrap();

		lif_file
			.seek(SeekFrom::Start(lif_header.data.into()))
			.unwrap();

		let pixel_count = (lif_header.size[0] * lif_header.size[1]) as usize;

		let data = if lif_header.flags.contains(TextureFlags::Paletted) {
			let mut data = vec![0u8; pixel_count];
			lif_file.read_exact(&mut data).unwrap();

			lif_file
				.seek(SeekFrom::Start(lif_header.palette.into()))
				.unwrap();

			let palette: [[u8; 4]; 256] = BinRead::read(&mut lif_file).unwrap();

			data.into_iter()
				.map(|i| palette[usize::from(i)].into())
				.collect()
		} else {
			Vec::<[u8; 4]>::read_args(
				&mut lif_file,
				binrw::VecArgs {
					count: pixel_count,
					inner: (),
				},
			)
			.unwrap()
			.into_iter()
			.map(Into::into)
			.collect()
		};
		Ok(Texture {
			size: lif_header.size.into(),
			rgba: data,
		})
	}
}

#[derive(Default)]
struct Patch {
	triangle: u32,
	vertex: u8,
	pos: Option<Vec3>,
	old_pos: Option<Vec3>,
	normal: Option<Vec3>,
	old_normal: Option<Vec3>,
	uv: Option<Vec2>,
	old_uv: Option<Vec2>,
	// pos: Vector<Option<(f32, f32)>, 3>,
	// normal: Vector<Option<(f32, f32)>, 3>,
	// uv: Vector<Option<(f32, f32)>, 2>,
}

// fn patch_vector<T: Copy + PartialEq, const N: usize>(
// 	target: &mut Vector<T, N>,
// 	patch: &Vector<Option<(T, T)>, N>,
// ) {
// 	for i in 0..N {
// 		if let Some((old, new)) = patch[i] {
// 			if target[i] != old {
// 				log::warn!(target:"model::classic::patch", "Unexpected old value! Patching anyway...");
// 			}
// 			target[i] = new;
// 		}
// 	}
// }

struct PatchSet(HashMap<String, Vec<Patch>>);

fn patch(path: &str, triangles: &mut [Surface]) {
	let patch_set = {
		let patch_set = [
			(
				"r1/resourcecollector/rl0/lod0/resourcecollector.peo",
				vec![
					Patch {
						triangle: 177,
						vertex: 2,
						uv: Some(vec2(0.25, 0.999999761)),
						old_uv: Some(vec2(0.49725038, 0.999999761)),
						// uv: Vector::<_, 2>::new(Some((0.49725038, 0.25)), None),
						..Default::default()
					},
					Patch {
						triangle: 179,
						vertex: 2,
						uv: Some(vec2(0.25, 0.999999761)),
						old_uv: Some(vec2(0.49725038, 0.999999761)),
						// uv: Vector::<_, 2>::new(Some((0.49725038, 0.25)), None),
						..Default::default()
					},
				],
			),
			(
				"r1/mothership/rl0/lod0/mothership.peo",
				vec![
					Patch {
						triangle: 415,
						vertex: 0,
						uv: Some(vec2(0.755954623, 0.8771702)),
						old_uv: Some(vec2(0.755954623, 0.9865452)),
						// uv: Vector::<_, 2>::new(None, Some((0.9865452, 0.8771702))),
						..Default::default()
					},
					Patch {
						triangle: 415,
						vertex: 2,
						uv: Some(vec2(1.0, 0.875)),
						old_uv: Some(vec2(1.0, 0.9962938)),
						// uv: Vector::<_, 2>::new(None, Some((0.9962938, 0.875))),
						..Default::default()
					},
					Patch {
						triangle: 419,
						vertex: 2,
						uv: Some(vec2(1.0, 1.0)),
						old_uv: Some(vec2(1.0, 1.0037062)),
						// uv: Vector::<_, 2>::new(None, Some((1.0037062, 1.0))),
						..Default::default()
					},
				],
			),
		];
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
			if let Some(old_pos) = patch.old_pos {
				if vertex.pos != old_pos {
					log::warn!(target:"model::classic::patch", "Unexpected old value! Patching anyway...");
				}
			}
			if let Some(old_normal) = patch.old_normal {
				if vertex.normal != old_normal {
					log::warn!(target:"model::classic::patch", "Unexpected old value! Patching anyway...");
				}
			}
			if let Some(old_uv) = patch.old_uv {
				if vertex.uv != old_uv {
					log::warn!(target:"model::classic::patch", "Unexpected old value! Patching anyway...");
				}
			}

			if let Some(pos) = patch.pos {
				vertex.pos = pos;
			}
			if let Some(normal) = patch.normal {
				vertex.normal = normal;
			}
			if let Some(uv) = patch.uv {
				vertex.uv = uv;
			}

			// patch_vector(&mut vertex.pos, &patch.pos);
			// patch_vector(&mut vertex.normal, &patch.normal);
			// patch_vector(&mut vertex.uv, &patch.uv);
		}
	}
}
