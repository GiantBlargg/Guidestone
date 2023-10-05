use std::{
	collections::{BTreeMap, HashMap},
	io::{self, SeekFrom},
	num::NonZeroU32,
};

use binrw::{binread, file_ptr::NonZeroFilePtr32, BinRead, FilePtr32, NullString};
use bitflags::bitflags;

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

#[allow(dead_code)]
#[binread]
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

#[allow(dead_code)]
#[binread]
struct VertexEntry {
	pos: [f32; 3],
	normal_index: u32,
}

#[allow(dead_code)]
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
	local_matrix: [f32; 16],
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
	textureName: NullString, //name of texture, an offset from start of string block
	size: [u32; 2],      //size of texture
	flags: TextureFlags, //flags for things like alpha and luminance map
	// imageCRC: u32,       //crc of the unquantized image
	// nShared: i32,        //number of images which share this one
	// sharedTo: u32,       //list of indices of images which share this one
	#[br(pad_before = 12, map(|x| if x == u32::MAX {None} else {Some(x)}))]
	sharedFrom: Option<u32>, //image this one is shared from, or -1 if none
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

struct TextureMeta {
	name: String,
	size: [u32; 2],
	pallete: bool,
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
			.filter(|l| l.textureName.len() > 0)
			.map(|l| {
				let local_name = l.textureName.to_string();
				let shared_from = l.sharedFrom.map(|s| &ll.lifs[s as usize]);
				if let Some(s) = shared_from {
					assert!(l.size == s.size);
					assert!(s.flags.contains(l.flags));
				}
				let true_name = shared_from
					.map(|s| s.textureName.to_string())
					.unwrap_or_else(|| local_name.clone());
				(
					local_name.to_ascii_lowercase(),
					TextureMeta {
						name: true_name,
						size: l.size,
						pallete: l.flags.contains(TextureFlags::Paletted),
						alpha: l.flags.contains(TextureFlags::Alpha),
						team_colour: (l.flags)
							.intersects(TextureFlags::TeamColor0 | TextureFlags::TeamColor1),
					},
				)
			})
			.collect()
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
}
