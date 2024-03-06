#include "Importer/Importer.h"

#include "Core/LogSystem.h"
#include "Scene/Components.h"

#include <assimp/Importer.hpp>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void ConvertAssimpScene(const aiScene* assimp_scene, brr::Scene& scene, brr::Entity parent = {})
{
	struct NodeInfo
	{
		aiNode* assimp_node;
		brr::Transform3DComponent* parent_transform;
	};
	std::list<NodeInfo> nodes;

	// Insert root node (as a child of the passed parent Entity, if that's the case)
	{
		brr::Transform3DComponent* parent_transf{ nullptr };
		if (parent)
		{
			parent_transf = &parent.GetComponent<brr::Transform3DComponent>();
		}
		nodes.push_back({assimp_scene->mRootNode, parent_transf });
	}

	while (!nodes.empty())
	{
		NodeInfo node_info = nodes.front();
		nodes.pop_front();

		brr::Entity entity = scene.Add3DEntity({});

		brr::Transform3DComponent& node_transform = entity.GetComponent<brr::Transform3DComponent>();

		if (node_info.parent_transform)
		{
			node_transform.SetParent(node_info.parent_transform);
		}

		aiNode* node = node_info.assimp_node;
		{
			if (node->mNumMeshes > 0)
			{
				brr::Mesh3DComponent& mesh_component = entity.AddComponent<brr::Mesh3DComponent>();
				entity.AddComponent<brr::Mesh3DRendererComponent>();
			
				for (size_t i = 0; i < node->mNumMeshes; i++)
				{
					uint32_t mesh_index = node->mMeshes[i];
					aiMesh* mesh = assimp_scene->mMeshes[mesh_index];

					std::vector<brr::Vertex3>  vertices(mesh->mNumVertices);
					for (uint32_t vertex_idx = 0; vertex_idx < mesh->mNumVertices; vertex_idx++)
					{
						const aiVector3D& aiVertex = mesh->mVertices[vertex_idx];
						brr::Vertex3& vertex = vertices[vertex_idx];
					    vertex.pos = {aiVertex.x, aiVertex.y, aiVertex.z};
						if (mesh->HasNormals())
						{
							const aiVector3D& aiNormal = mesh->mNormals[vertex_idx];
							vertex.normal = {aiNormal.x, aiNormal.y, aiNormal.z};
						}

						if (mesh->HasTangentsAndBitangents())
						{
						    const aiVector3D& aiTangent = mesh->mTangents[vertex_idx];
							vertex.tangent = {aiTangent.x, aiTangent.y, aiTangent.z};
						}

						if (mesh->HasTextureCoords(0))
						{
							const aiVector3D& aiUV = mesh->mTextureCoords[0][vertex_idx];
						    vertex.u = aiUV.x;
						    vertex.v = aiUV.y;
						}
					}

					std::vector<uint32_t> indices (mesh->mNumFaces * 3);
					for (uint32_t face_idx = 0, index_idx = 0; face_idx < mesh->mNumFaces; face_idx++, index_idx += 3)
					{
						aiFace face = mesh->mFaces[face_idx];

						indices[index_idx]	 = face.mIndices[0];
						indices[index_idx+1] = face.mIndices[1];
						indices[index_idx+2] = face.mIndices[2];
					}

					mesh_component.AddSurface(vertices, indices);
				}
			}
		}

		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			nodes.push_back({node->mChildren[i], &node_transform });
		}

	}
}

namespace brr
{
	void SceneImporter::LoadFileIntoScene(std::string path, Scene* scene, Entity parent)
	{
		
		Assimp::Importer assimp_importer;
		const aiScene* assimp_scene = assimp_importer.ReadFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

		if (!assimp_scene)
		{
			BRR_LogError("Assimp could not load Scene. Assimp Error Code:\n{}", assimp_importer.GetErrorString());
		}

		ConvertAssimpScene(assimp_scene, *scene, parent);
	}
}
