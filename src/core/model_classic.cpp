#include "log.hpp"
#include "model.hpp"

#include "fs.hpp"
#include <array>
#include <fstream>

namespace Classic {

using color = u8vec4;

namespace Geo {
struct Header {
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
template <typename R> R& operator>>(R& r, Header& g) {
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
	i32 iFaceNormal;            // Index into the face normal list.
	std::array<u16, 3> iVertex; // Indicies to the vertices of the polygon.
	u16 iMaterial;              // Index into material list.
	std::array<vec2, 3> uv;
	u16 flags; // Flags for this polygon.
	u8 reserved[2];
};
template <typename R> R& operator>>(R& r, PolyEntry& p) {
	return r >> p.iFaceNormal >> p.iVertex >> p.iMaterial >> p.uv >> p.flags >> p.reserved;
}

struct VertexEntry {
	fvec3 pos;
	i32 iVertexNormal; // Index into the point normal list.
};
template <typename R> R& operator>>(R& r, VertexEntry& v) { return r >> v.pos >> v.iVertexNormal; }

struct MaterialEntry {
	u32 pName;      // Offset to name of material (may be a CRC32).
	color ambient;  // Ambient color information.
	color diffuse;  // Diffuse color information.
	color specular; // Specular color information.
	f32 kAlpha;     // Alpha blending information.
	u32 texture;    // Pointer to texture information (or CRC32).
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

} // namespace Geo

namespace Lif {

struct Header {
	u8 ident[8]; // compared to "Willy 7"
	u32 version; // version number
	enum Flags : u32 {
		Paletted = 0x02,   // texture uses a palette
		Alpha = 0x08,      // alpha channel image
		TeamColor0 = 0x10, // team color flags
		TeamColor1 = 0x20,
	};
	u32 flags;                    // to plug straight into texreg flags
	u32 width, height;            // dimensions of image
	u32 paletteCRC;               // a CRC of palettes for fast comparison
	u32 imageCRC;                 // crc of the unquantized image
	u32 data;                     // pointer to actual image
	u32 palette;                  // pointer to palette for this image
	u32 teamEffect0, teamEffect1; // pointers to palettes of team color effect
};
template <typename R> R& operator>>(R& r, Header& h) {
	return r >> h.ident >> h.version >> h.flags >> h.width >> h.height >> h.paletteCRC >> h.imageCRC >> h.data >>
		h.palette >> h.teamEffect0 >> h.teamEffect1;
}

const size_t PaletteSize = 256;

} // namespace Lif

} // namespace Classic

u32 ModelCache::loadClassicModel(const FS::Path& path) {
	FS::Reader geo = FS::loadClassicFile(path);

	auto header = geo.get<Classic::Geo::Header>();

	auto polygon_objects = geo.getVector<Classic::Geo::PolygonObject>(header.nPolygonObjects);

	geo.cursor = header.oLocalMaterial;
	auto materials = geo.getVector<Classic::Geo::MaterialEntry>(header.nPublicMaterials + header.nLocalMaterials);

	for (auto& po : polygon_objects) {
		auto poly_entries = geo.getVector<Classic::Geo::PolyEntry>(po.nPolygons, po.pPolygonList);

		for (auto& pe : poly_entries) {
			Classic::Geo::VertexEntry v;
			Classic::Geo::VertexEntry n;
			u32 iNormal = pe.iFaceNormal;
			bool smooth = materials[pe.iMaterial].flags & Classic::Geo::MaterialEntry::Flags::Smoothing;

			for (int i = 0; i < 3; i++) {
				geo.cursor = po.pVertexList + pe.iVertex[i] * sizeof(Classic::Geo::VertexEntry);
				v = geo.get<Classic::Geo::VertexEntry>();
				if (smooth) {
					iNormal = v.iVertexNormal;
				}
				geo.cursor = po.pNormalList + iNormal * sizeof(Classic::Geo::VertexEntry);
				n = geo.get<Classic::Geo::VertexEntry>();
				vertices.push_back(Vertex{.pos = v.pos, .normal = n.pos, .uv = pe.uv[i]});
			}
		}
	}

	for (auto& mat : materials) {
		if (mat.texture) {
			std::string texture_name = geo.get<std::string>(mat.texture) + ".lif";
			FS::Path texture_path = (path.parent_path() / texture_name);

			FS::Reader lif = FS::loadClassicFile(texture_path);
			auto texture_header = lif.get<Classic::Lif::Header>();

			Texture tex{texture_header.width, texture_header.height};
			tex.rgba.reserve(tex.width * tex.height);

			if (texture_header.flags & Classic::Lif::Header::Flags::Paletted) {
				auto indicies = lif.getVector<u8>(tex.width * tex.height, texture_header.data);
				auto palette = lif.getVector<u8vec4>(Classic::Lif::PaletteSize, texture_header.palette);
				for (auto i : indicies) {
					tex.rgba.push_back(palette[i]);
				}
			} else {
				Log::error("Non paletted images not yet supported.");
			}

			if (!(texture_header.flags & Classic::Lif::Header::Flags::Alpha)) { // Opaque image, discard alpha
				tex.rgb.reserve(tex.width * tex.height);
				for (auto c : tex.rgba) {
					tex.rgb.push_back({c.x, c.y, c.z});
				}
				tex.rgba.clear();
				tex.rgba.shrink_to_fit();
			}
		}
	}

	return models.size() - 1;
}
