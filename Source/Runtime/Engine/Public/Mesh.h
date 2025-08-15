#pragma once

namespace ThunderEngine
{
	class ENGINE_API IMesh
	{
	public:
		IMesh() {}
		virtual ~IMesh() {}
	};

	class ENGINE_API StaticMesh : public IMesh
	{
	public:
		StaticMesh() {}
		virtual ~StaticMesh() {}
	};
}