#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H
#include "Geometry/Geometry.h"

namespace brr
{

	struct Mesh3DComponent
	{
		struct SurfaceData
		{
			std::vector<Vertex3_PosColor> vertices{};
			std::vector<uint32_t> indices{};
		};

		std::vector<SurfaceData> surfaces{};
	};

}

#endif