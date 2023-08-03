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

	struct Model {
		struct Node {
			std::vector<index> children_nodes;
			mat4 transform;
		};
		std::vector<Node> nodes;
		std::vector<index> root_nodes;

		struct Mesh {
			index first_vertex;
			index num_vertices;
			index material;
			index node;
		};
		std::vector<Mesh> meshes;

		struct Material {};
		std::vector<Material> materials;
	};
	std::vector<Model> models;

	u32 loadClassicModel(const FS::Path&);
};
