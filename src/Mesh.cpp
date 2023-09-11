#include "Mesh.h"
#include "vulkan/vulkan.hpp"
#include "Utils.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

VertexInputDescription get_vertex_description() {
    VertexInputDescription description;

    // We will have 1 vertex buffer binding, with a per vertex rate (rather than instance)
    vk::VertexInputBindingDescription mainBindingDescription = {};
    mainBindingDescription.binding = 0;
    mainBindingDescription.inputRate = vk::VertexInputRate::eVertex;
    mainBindingDescription.stride = sizeof(Vertex);

    description.bindings.push_back(mainBindingDescription);

    // We have 3 attributes: position, color, and normal stored at locations 0, 1, and 2
    vk::VertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = vk::Format::eR32G32B32Sfloat;
    positionAttribute.offset = offsetof(Vertex, position);

    vk::VertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = vk::Format::eR32G32B32Sfloat;
    normalAttribute.offset = offsetof(Vertex, normal);

    vk::VertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 2;
    colorAttribute.format = vk::Format::eR32G32B32Sfloat;
    colorAttribute.offset = offsetof(Vertex, color);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);
    return description;
}

void load_mesh_from_obj(Mesh& mesh, const char* fileName) {
    // Attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib;
    // Shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes;
    // Materials contains the information about the material of each shape, but we won't use it.
    std::vector<tinyobj::material_t> materials;

    // Error and warning output from the load function
	std::string warn;
	std::string err;

    // Load the OBJ file
	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName, nullptr);
    // Make sure to output the warnings to the console, in case there are issues with the file
	if (!warn.empty()) {
		MRWARN(warn);
	}
    // If we have any error, print it to the console, and break the mesh loading.
    // This happens if the file can't be found or is malformed
	if (!err.empty()) {
		MRCERR("Failed to load mesh: " <<  fileName);
	}

    // Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            //hardcode loading to triangles
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                //vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                //vertex normal
            	tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                //copy it into our vertex
				Vertex new_vert;
				new_vert.position.x = vx;
				new_vert.position.y = vy;
				new_vert.position.z = vz;

				new_vert.normal.x = nx;
				new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                //we are setting the vertex color as the vertex normal. This is just for display purposes
                new_vert.color = new_vert.normal;


				mesh.vertices.push_back(new_vert);
			}
			index_offset += fv;
		}
	}
}

void upload_mesh(Mesh& mesh, VmaAllocator allocator, DeletionQueue& deletionQueue) {
    upload_buffer(mesh.vertexBuffer, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, allocator, deletionQueue);

    if (mesh.indices.size() > 0) {
        upload_buffer(mesh.indexBuffer, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, allocator, deletionQueue);
    }
}