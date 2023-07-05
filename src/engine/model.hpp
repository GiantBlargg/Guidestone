#pragma once

#include "fs.hpp"
#include "math.hpp"
#include "types.hpp"
#include <string>
#include <vector>

class ModelCache {
  public:
	using index = u64;

	struct Vertex {
		Vector3 pos;
		Vector3 normal;
		Vector2 uv;
	};
	std::vector<Vertex> vertices;

	struct Model {
		struct Node {
			std::vector<index> children_nodes;
			Matrix4 transform;
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

	u32 loadClassicModel(FS::Path);
};
