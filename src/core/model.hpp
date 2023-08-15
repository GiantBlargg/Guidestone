#pragma once

#include "fs.hpp"
#include "math.hpp"
#include "types.hpp"
#include <string>
#include <variant>
#include <vector>

class ModelCache {
  public:
	using index = u64;
	static constexpr index index_null = std::numeric_limits<index>::max();

	struct Vertex {
		vec3 pos;
		vec3 normal;
		vec2 uv;
	};
	std::vector<Vertex> vertices;

	struct Texture {
		uvec2 size;
		bool has_alpha = false;
		std::vector<u8vec4> rgba = {};
	};
	std::vector<Texture> textures = {{{1, 1}, false, {{255, 255, 255, 255}}}};

	struct Material {
		index texture = index_null;

		bool operator==(const Material&) const = default;
	};
	std::vector<Material> materials;

	struct Model {
		struct Node {
			index parent_node = index_null;
			mat4 transform = mat4::identity();
		};
		std::vector<Node> nodes;

		struct Mesh {
			index first_vertex;
			index num_vertices;
			index material;
			index node;
		};
		std::vector<Mesh> meshes;
	};
	std::vector<Model> models;

	u32 loadClassicModel(const FS::Path&);
};
