#include "Importer/Importer.h"

#include "Core/LogSystem.h"
#include "Scene/Components.h"

#include <assimp/Importer.hpp>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void ConvertAssimpScene(const aiScene* assimp_scene, brr::Scene& scene, brr::Entity parent = {})
{
	//struct NodeInfo
	//{
	//	aiNode* assimp_node;
	//	brr::Transform3DComponent* parent_transform;
	//};
	//std::list<NodeInfo> nodes;

	//// Insert root node (as a child of the passed parent Entity, if that's the case)
	//{
	//	brr::Transform3DComponent* parent_transf{ nullptr };
	//	if (parent)
	//	{
	//		parent_transf = &parent.GetComponent<brr::Transform3DComponent>();
	//	}
	//	nodes.push_back({assimp_scene->mRootNode, parent_transf });
	//}

	//while (!nodes.empty())
	//{
	//	NodeInfo node_info = nodes.front();
	//	nodes.pop_front();

	//	brr::Entity entity = scene.Add3DEntity({});

	//	brr::Transform3DComponent& node_transform = entity.GetComponent<brr::Transform3DComponent>();

	//	if (node_info.parent_transform)
	//	{
	//		node_transform.SetParent(node_info.parent_transform);
	//	}

	//	aiNode* node = node_info.assimp_node;
	//	{
	//		if (node->mNumMeshes > 0)
	//		{
	//			//brr::Entity mesh_entity = scene.Add3DEntity();
	//			brr::Mesh3DComponent& mesh_component = entity.AddComponent<brr::Mesh3DComponent>();

	//			mesh_component.m_surfaces.resize(node->mNumMeshes);
	//		
	//			for (size_t i = 0; i < node->mNumMeshes; i++)
	//			{
	//				brr::Transform3DComponent& mesh_transform = entity.GetComponent<brr::Transform3DComponent>();
	//				mesh_transform.SetParent(&node_transform);

	//				uint32_t mesh_index = node->mMeshes[i];
	//				aiMesh* mesh = assimp_scene->mMeshes[mesh_index];

	//			
	//				mesh_component.m_surfaces[i].m_vertices.resize(mesh->mNumVertices);
	//				memcpy(mesh_component.m_surfaces[i].m_vertices.data(), &mesh->mVertices, mesh->mNumVertices * sizeof(glm::vec3));

	//				mesh_component.m_surfaces[i].m_indices.resize(mesh->mNumFaces * 3);

	//				for (uint32_t i = 0, j = 0; i < mesh->mNumFaces; i++, j += 3)
	//				{
	//					aiFace face = mesh->mFaces[i];

	//					mesh_component.m_surfaces[i].m_indices[ j ] = face.mIndices[0];
	//					mesh_component.m_surfaces[i].m_indices[j+1] = face.mIndices[1];
	//					mesh_component.m_surfaces[i].m_indices[j+2] = face.mIndices[2];
	//				}
	//			}
	//		}
	//	}

	//	for (size_t i = 0; i < node->mNumChildren; i++)
	//	{
	//		nodes.push_back({node->mChildren[i], &node_transform });
	//	}

	//}
}

namespace brr
{
	void SceneImporter::LoadFileIntoScene(std::string path, Scene* scene, Entity parent)
	{
		
		Assimp::Importer assimp_importer;
		const aiScene* assimp_scene = assimp_importer.ReadFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

		if (!assimp_scene)
		{
			BRR_LogError("Assimp could not load Scene. Assimp Error:\n{}", assimp_importer.GetErrorString());
		}

		ConvertAssimpScene(assimp_scene, *scene, parent);
	}
}
