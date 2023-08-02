#include "model.hpp"

#include "fs.hpp"

namespace Classic {

struct GeoHeader {
	u8 identifier[8];     // File identifier.
	u32 version;          // File version.
	u32 pName;            // Offset to a file name.
	u32 fileSize;         // File size (in bytes), not counting this header.
	u32 localSize;        // Object size (in bytes).
	u32 nPublicMaterials; // Number of public materials.
	u32 nLocalMaterials;  // Number of local materials.
	u32 oPublicMaterial;  // list of public materials
	u32 oLocalMaterial;   // list of local materials
	u32 nPolygonObjects;  // Number of polygon objects.
	u8 reserved[24];      // Reserved for future use.
};
template <typename R> R& operator>>(R& r, GeoHeader& g) {
	return r >> g.identifier >> g.version >> g.pName >> g.fileSize >> g.localSize >> g.nPublicMaterials >>
		g.nLocalMaterials >> g.oPublicMaterial >> g.oLocalMaterial >> g.nPolygonObjects >> g.reserved;
}

struct PolygonObject {
	u32 pName;          // Name for animation.
	u8 flags;           // General flags (see above)
	u8 iObject;         // fixed up at load time so we know what object index we have when recursively processing
	u16 nameCRC;        // 16-bit CRC of name (!!!!no room for 32 right now - make room next version)
	i32 nVertices;      // Number of vertices in vertex list for this object.
	i32 nFaceNormals;   // Number of face normals for this object.
	i32 nVertexNormals; // Number of vertex normals for this object.
	i32 nPolygons;      // Number of polygons in this object.
	u32 pVertexList;    // Offset to the vertex list in this object.
	u32 pNormalList;    // Offset to the normal list in this object.
	u32 pPolygonList;   // Offset to the polygon list in this object.
	u32 pMother;        // link to parent object
	u32 pDaughter;      // link to child object
	u32 pSister;        // link to sibling object
	mat4 localMatrix;
};
template <typename R> R& operator>>(R& r, PolygonObject& p) {
	return r >> p.pName >> p.flags >> p.iObject >> p.nameCRC >> p.nVertices >> p.nFaceNormals >> p.nVertexNormals >>
		p.nPolygons >> p.pVertexList >> p.pNormalList >> p.pPolygonList >> p.pMother >> p.pDaughter >> p.pSister >>
		p.localMatrix;
}

struct PolyEntry {
	i32 iFaceNormal; // Index into the face normal list.
	u16 iV0;         // Index to V0 of the polygon.
	u16 iV1;         // Index to V1 of the polygon.
	u16 iV2;         // Index to V2 of the polygon.
	u16 iMaterial;   // Index into material list.
	f32 s0;
	f32 t0;
	f32 s1;
	f32 t1;
	f32 s2;
	f32 t2;
	u16 flags; // Flags for this polygon.
	u8 reserved[2];
};
template <typename R> R& operator>>(R& r, PolyEntry& p) {
	return r >> p.iFaceNormal >> p.iV0 >> p.iV1 >> p.iV2 >> p.iMaterial >> p.s0 >> p.t0 >> p.s1 >> p.t1 >> p.s2 >>
		p.t2 >> p.flags >> p.reserved;
}

struct VertexEntry {
	f32 x;             // X component of this vertex.
	f32 y;             // Y component of this vertex.
	f32 z;             // Z component of this vertex.
	i32 iVertexNormal; // Index into the point normal list.
};
template <typename R> R& operator>>(R& r, VertexEntry& v) { return r >> v.x >> v.y >> v.z >> v.iVertexNormal; }

struct MaterialEntry {
	u32 pName;       // Offset to name of material (may be a CRC32).
	u8vec4 ambient;  // Ambient color information.
	u8vec4 diffuse;  // Diffuse color information.
	u8vec4 specular; // Specular color information.
	f32 kAlpha;      // Alpha blending information.
	u32 texture;     // Pointer to texture information (or CRC32).
	enum Flags : u16 {
		Smoothing = 2,
		DoubleSided = 8,
		SelfIllum = 64,
	};
	u16 flags;              // Flags for this material.
	u8 nFullAmbient;        // Number of self-illuminating colors in texture.
	u8 bTexturesRegistered; // Set to TRUE when some textures have been registered.
	u32 textureNameSave;    // the name of the texture, after the texture has been registered
};
template <typename R> R& operator>>(R& r, MaterialEntry& m) {
	return r >> m.pName >> m.ambient >> m.diffuse >> m.specular >> m.kAlpha >> m.texture >> m.flags >> m.nFullAmbient >>
		m.bTexturesRegistered >> m.textureNameSave;
}

} // namespace Classic

u32 ModelCache::loadClassicModel(FS::Path path) {
	FS::Reader data = FS::loadClassicFile(path);

	auto header = data.get<Classic::GeoHeader>();

	auto polygon_objects = data.getVector<Classic::PolygonObject>(header.nPolygonObjects);

	data.cursor = header.oLocalMaterial;
	auto materials = data.getVector<Classic::MaterialEntry>(header.nPublicMaterials + header.nLocalMaterials);

	for (auto& po : polygon_objects) {
		data.cursor = po.pPolygonList;
		auto poly_entries = data.getVector<Classic::PolyEntry>(po.nPolygons);

		for (auto& pe : poly_entries) {
			Classic::VertexEntry v;
			Classic::VertexEntry n;
			u32 iNormal = pe.iFaceNormal;
			bool smooth = materials[pe.iMaterial].flags & Classic::MaterialEntry::Flags::Smoothing;

#define LOAD_VERTEX(i)                                                                                                 \
	data.cursor = po.pVertexList + pe.iV##i * sizeof(Classic::VertexEntry);                                            \
	v = data.get<Classic::VertexEntry>();                                                                              \
	if (smooth) {                                                                                                      \
		iNormal = v.iVertexNormal;                                                                                     \
	}                                                                                                                  \
	data.cursor = po.pNormalList + iNormal * sizeof(Classic::VertexEntry);                                             \
	n = data.get<Classic::VertexEntry>();                                                                              \
	vertices.push_back(Vertex{.pos = {v.x, v.y, v.z}, .normal = {n.x, n.y, n.z}, .uv = {pe.s##i, pe.t##i}});

			LOAD_VERTEX(0)
			LOAD_VERTEX(1)
			LOAD_VERTEX(2)

#undef LOAD_VERTEX
		}
	}

	return models.size() - 1;
}
