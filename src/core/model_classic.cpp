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

template <typename I, typename N> inline auto posmod(I i, N n) { return (i % n + n) % n; }
inline auto posmod(f32 i, f32 n) { return fmod((fmod(i, n) + n), n); }
inline auto posmod(f64 i, f64 n) { return fmod((fmod(i, n) + n), n); }

template <typename N> bool is_po2(N n) { return (n & (n - 1)) == 0; }

// Some textures have been packed together, however this doesn't play well with texture filtering
// Try to separate them back into the individual textures
void split_textures(std::vector<ModelCache::Texture>& textures, std::vector<Surface>& surfaces) {

	// Repeated copies are not considered to be the same area
	// TODO: Fix this

	struct Region {
		size_t texture;
		ivec2 min, max;
		uvec2 size;
		vec2 offset, scale;
	};

	std::vector<uvec2> texture_size;

	texture_size.reserve(textures.size());
	for (auto& t : textures) {
		texture_size.push_back(t.size);
	}

	std::vector<Region> regions;
	regions.reserve(surfaces.size());

	constexpr size_t merged_null = std::numeric_limits<size_t>::max();
	std::vector<size_t> merged(surfaces.size(), merged_null);

	std::vector<size_t> map;
	map.reserve(surfaces.size());

	for (auto& s : surfaces) {
		map.push_back(regions.size());

		vec2 min = s.vertices[0].uv, max = s.vertices[0].uv;

		for (auto& v : s.vertices) {
			min.x = std::min(min.x, v.uv.x);
			min.y = std::min(min.y, v.uv.y);
			max.x = std::max(max.x, v.uv.x);
			max.y = std::max(max.y, v.uv.y);
		}

		uvec2 tex_size = texture_size[s.texture];
		regions.push_back({
			.texture = s.texture,
			.min =
				{
					static_cast<int>(std::floor(min.x * tex_size.x)),
					static_cast<int>(std::floor(min.y * tex_size.y)),
				},
			.max =
				{
					static_cast<int>(std::ceil(max.x * tex_size.x)),
					static_cast<int>(std::ceil(max.y * tex_size.y)),
				},
		});
	}

	std::ranges::sort(
		map,
		[](const Region& a, const Region& b) {
			if (a.texture != b.texture)
				return a.texture < b.texture;

			if (a.min.x != b.min.x)
				return a.min.x < b.min.x;
			if (a.min.y != b.min.y)
				return a.min.y < b.min.y;

			if (a.max.x != b.max.x)
				return a.min.x < b.min.x;

			return a.max.y < b.max.y;
		},
		[&regions](size_t i) { return regions[i]; });

	// Merge overlapping regions
	bool done = false;
	while (!done) {
		done = true;

		for (size_t i = 0; i < map.size(); i++) {
			Region& root = regions[map[i]];
			if (merged[map[i]] != merged_null) // This region has already been merged
				continue;

			for (size_t j = i + 1; j < map.size(); j++) {
				Region& merge = regions[map[j]];

				// Since the map is sorted we can back out early
				if (root.texture < merge.texture)
					break;
				if (root.max.x <= merge.min.x)
					break;

				if (merged[map[j]] != merged_null) // This region has already been merged
					continue;

				if (root.max.y <= merge.min.y)
					continue;
				if (root.min.y >= merge.max.y)
					continue;

				// Found a region to merge
				merged[map[j]] = map[i];
				// min.x shouldn't need to change
				assert(root.min.x <= merge.min.x);
				root.min.y = std::min(root.min.y, merge.min.y);
				root.max = {std::max(root.max.x, merge.max.x), std::max(root.max.y, merge.max.y)};
				done = false;
			}
		}
	}

	{
		std::vector<Region> new_regions;

		for (size_t i : map) {
			Region& region = regions[i];
			size_t& m = merged[i];
			if (m == merged_null) {
				m = new_regions.size();
				new_regions.push_back(region);
			} else {
				m = merged[m];
			}
		}

		regions = std::move(new_regions);
	}

	for (Region& region : regions) {
		region.size = static_cast<uvec2>(region.max - region.min);
		uvec2 tex_size = texture_size[region.texture];

		if (!is_po2(region.size.x) || !is_po2(region.size.y)) {
			Log::warn("Region is not a power of 2!");
		}

		region.offset = {
			static_cast<f32>(region.min.x) / static_cast<f32>(tex_size.x),
			static_cast<f32>(region.min.y) / static_cast<f32>(tex_size.y),
		};
		region.scale = {
			static_cast<f32>(tex_size.x) / static_cast<f32>(region.size.x),
			static_cast<f32>(tex_size.y) / static_cast<f32>(region.size.y),
		};
	}

	// Calculated regions, now split them up

	for (size_t i = 0; i < surfaces.size(); i++) {
		Surface& surface = surfaces[i];
		size_t region_index = merged[i];
		Region& region = regions[region_index];

		surface.texture = region_index;

		for (auto& v : surface.vertices) {
			v.uv = (v.uv - region.offset) * region.scale;
		}
	}
	{
		std::vector<ModelCache::Texture> new_textures;
		new_textures.reserve((regions.size()));

		for (const auto& region : regions) {
			const ModelCache::Texture& source_texture = textures[region.texture];
			if (source_texture.size == region.size) {
				new_textures.push_back(source_texture);
			} else {
				ModelCache::Texture tex{.size = region.size, .has_alpha = textures[region.texture].has_alpha};
				tex.rgba.reserve(region.size.x * region.size.y);
				for (u32 y = 0; y < region.size.y; y++) {
					for (u32 x = 0; x < region.size.x; x++) {
						size_t source_pixel_addr =
							(posmod(y + region.min.y, source_texture.size.y) * source_texture.size.x) +
							posmod(x + region.min.x, source_texture.size.y);

						tex.rgba.push_back(source_texture.rgba[source_pixel_addr]);
					}
				}
				new_textures.push_back(tex);
			}
		}
		textures = std::move(new_textures);
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

	std::vector<std::string> texture_names;
	std::vector<size_t> texture_lookup;

	for (auto& mat : geo_materials) {
		if (mat.texture) {
			std::string tex_name = geo.get<std::string>(mat.texture);
			auto it = std::ranges::find(texture_names, tex_name);
			if (it == texture_names.end()) {
				texture_lookup.push_back(texture_names.size());
				texture_names.push_back(tex_name);
			} else {
				texture_lookup.push_back(it - texture_names.begin());
			}
		} else {
			// Default texture
			texture_lookup.push_back(-textures.size());
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
				vec2 uv = pe.uv[i];
				if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
					Log::warn(
						"Texture \"" + texture_names[triangles.back().texture] + "\"(" +
						std::to_string(triangles.back().texture) + ") will be read out of range");
				}
				triangles.back().vertices.push_back({.pos = v.pos, .normal = n.pos, .uv = uv});
			}
		}
	}

	patch(path, triangles);

	std::vector<Texture> local_textures;

	for (auto& t : texture_names) {
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

	// split_textures(local_textures, triangles);

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
