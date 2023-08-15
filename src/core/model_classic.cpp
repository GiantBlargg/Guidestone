#include "log.hpp"
#include "model.hpp"

#include "fs.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <set>

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

struct Surface {
	bool emissive;
	bool double_sided;
	ModelCache::index node;
	size_t texture;
	std::vector<ModelCache::Vertex> vertices;

	bool cam_merge(const Surface& s) {
		return emissive == s.emissive && double_sided == s.double_sided && node == s.node && texture == s.texture;
	};
	void merge(const Surface& s) {
		assert(cam_merge(s));
		vertices.insert(vertices.end(), s.vertices.begin(), s.vertices.end());
	}
};

void patch(const FS::Path& path, std::vector<Surface>& triangles) {
	if (path == "r1/resourcecollector/rl0/lod0/resourcecollector.peo") {
		auto& uv1 = triangles[177].vertices[2].uv.x;
		auto& uv2 = triangles[179].vertices[2].uv.x;
		if (uv1 == uv2 && uv1 == 0.497250378f) {
			uv1 = uv2 = 0.25f;
		} else {
			Log::warn("Failed to apply patch");
		}
	}
}

u32 ModelCache::loadClassicModel(const FS::Path& path) {

	models.emplace_back();
	Model& model = models.back();

	FS::Reader geo = FS::loadClassicFile(path);

	auto header = geo.get<Classic::Geo::Header>();

	// Read this before we change the cursor
	auto polygon_objects = geo.getVector<Classic::Geo::PolygonObject>(header.nPolygonObjects);

	geo.cursor = header.oLocalMaterial;
	auto geo_materials = geo.getVector<Classic::Geo::MaterialEntry>(header.nPublicMaterials + header.nLocalMaterials);

	std::set<std::string> texture_offsets_set;
	for (auto& mat : geo_materials) {
		if (mat.texture) {
			texture_offsets_set.insert(geo.get<std::string>(mat.texture));
		}
	}
	std::vector<std::string> texture_offsets(texture_offsets_set.cbegin(), texture_offsets_set.cend());

	std::vector<size_t> texture_lookup;

	for (auto& mat : geo_materials) {
		materials.emplace_back();
		if (mat.texture) {
			texture_lookup.push_back(
				std::ranges::find(texture_offsets, geo.get<std::string>(mat.texture)) - texture_offsets.begin());
		} else {
			Log::error("Non textured surfaces not yet supported.");
			texture_lookup.push_back(std::numeric_limits<size_t>::max());
		}
	}

	std::vector<Surface> triangles;

	for (auto& po : polygon_objects) {
		model.nodes.push_back({.transform = po.localMatrix});
		Model::Node& node = model.nodes.back();
		if (po.pMother) {
			node.parent_node = (po.pMother - sizeof(Classic::Geo::Header)) / sizeof(Classic::Geo::PolygonObject);
		}

		auto poly_entries = geo.getVector<Classic::Geo::PolyEntry>(po.nPolygons, po.pPolygonList);

		for (auto& pe : poly_entries) {
			Classic::Geo::VertexEntry v;
			Classic::Geo::VertexEntry n;
			u32 iNormal = pe.iFaceNormal;
			bool smooth = geo_materials[pe.iMaterial].flags & Classic::Geo::MaterialEntry::Flags::Smoothing;

			triangles.push_back({
				.emissive = static_cast<bool>(
					geo_materials[pe.iMaterial].flags & Classic::Geo::MaterialEntry::Flags::SelfIllum),
				.double_sided = static_cast<bool>(
					geo_materials[pe.iMaterial].flags & Classic::Geo::MaterialEntry::Flags::DoubleSided),
				.node = model.nodes.size() - 1,
				.texture = texture_lookup[pe.iMaterial],
			});

			for (int i = 0; i < 3; i++) {
				geo.cursor = po.pVertexList + pe.iVertex[i] * sizeof(Classic::Geo::VertexEntry);
				v = geo.get<Classic::Geo::VertexEntry>();
				if (smooth) {
					iNormal = v.iVertexNormal;
				}
				geo.cursor = po.pNormalList + iNormal * sizeof(Classic::Geo::VertexEntry);
				n = geo.get<Classic::Geo::VertexEntry>();
				triangles.back().vertices.push_back({.pos = v.pos, .normal = n.pos, .uv = pe.uv[i]});
			}
		}
	}

	patch(path, triangles);

	std::vector<Texture> local_textures;

	for (auto& t : texture_offsets) {
		std::string texture_name = t + ".lif";
		FS::Path texture_path = (path.parent_path() / texture_name);

		FS::Reader lif = FS::loadClassicFile(texture_path);
		auto texture_header = lif.get<Classic::Lif::Header>();

		Texture tex{
			.size = {texture_header.width, texture_header.height},
			.has_alpha = !!(texture_header.flags & Classic::Lif::Header::Flags::Alpha)};
		tex.rgba.reserve(tex.size.x * tex.size.y);

		if (texture_header.flags & Classic::Lif::Header::Flags::Paletted) {
			auto indicies = lif.getVector<u8>(tex.size.x * tex.size.y, texture_header.data);
			auto palette = lif.getVector<u8vec4>(Classic::Lif::PaletteSize, texture_header.palette);
			for (auto i : indicies) {
				tex.rgba.push_back(palette[i]);
			}
		} else {
			Log::error("Non paletted images not yet supported.");
		}

		local_textures.push_back(tex);
	}

	std::ranges::stable_sort(triangles, [](const Surface& a, const Surface& b) {
		if (a.node != b.node)
			return a.node < b.node;
		if (a.texture != b.texture)
			return a.texture < b.texture;
		if (a.emissive != b.emissive)
			return a.emissive < b.emissive;
		// if (a.double_sided != b.double_sided)
		return a.double_sided < b.double_sided;
	});

	std::vector<Surface> surfaces;
	for (auto& t : triangles) {
		if (surfaces.empty() || !surfaces.back().cam_merge(t)) {
			surfaces.push_back(t);
		} else {
			surfaces.back().merge(t);
		}
	}

	for (auto& s : surfaces) {
		Material mat{.texture = s.texture + textures.size()};
		index mat_index = materials.size();
		for (index i = 0; i < materials.size(); i++) {
			auto& mat_ = materials[i];
			if (mat == mat_) {
				mat_index = i;
			}
		}
		if (mat_index == materials.size()) {
			materials.push_back(mat);
		}

		model.meshes.push_back(Model::Mesh{vertices.size(), s.vertices.size(), mat_index, s.node});
		vertices.insert(vertices.end(), s.vertices.begin(), s.vertices.end());
	}

	textures.insert(textures.end(), local_textures.begin(), local_textures.end());

	return models.size() - 1;
}
