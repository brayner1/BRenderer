#ifndef BRR_IMPORTER_H
#define BRR_IMPORTER_H
#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace brr
{
	class SceneImporter
	{
	public:

		static void LoadFileIntoScene(std::string path, Scene* scene, Entity parent = {});
	};
}

#endif