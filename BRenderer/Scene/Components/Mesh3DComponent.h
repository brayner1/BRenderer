#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H

#include <Core/Ref.h>
#include <Scene/Components/EntityComponent.h>
#include <Visualization/Resources/Mesh3D.h>

namespace brr
{
	class Mesh3DComponent : public EntityComponent
	{
    public:

		Ref<vis::Mesh3D> GetMesh() { return m_mesh; }

		void SetMesh(const Ref<vis::Mesh3D>& mesh);

	public:

		void RegisterGraphics();
		void UnregisterGraphics();

    private:

		Ref<vis::Mesh3D> m_mesh;
	};
}

#endif