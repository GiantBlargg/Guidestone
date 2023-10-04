use std::{
	io::{self, SeekFrom},
	num::NonZeroU32,
};

use binrw::{binread, file_ptr::NonZeroFilePtr32, BinRead, NullString};
use bitflags::bitflags;

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
	#[br(temp, assert(version == 0x402u32))]
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

pub fn make_model<R: io::Read + io::Seek>(mut read: R) {
	let geo = Geo::read(&mut read);
}
