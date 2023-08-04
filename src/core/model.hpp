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

	struct Vertex {
		vec3 pos;
		vec3 normal;
		vec2 uv;
	};
	std::vector<Vertex> vertices;

	struct Texture {
		u32 width, height;
		// Only fill one
		std::vector<u8vec3> rgb = {};
		std::vector<u8vec4> rgba = {};
	};
	std::vector<Texture> textures;

	struct Material {
		index texture = std::numeric_limits<index>::max();
	};
	std::vector<Material> materials;

	struct Model {
		struct Node {
			index parent_node = std::numeric_limits<index>::max();
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
