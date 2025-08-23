#include "GameObject.h"

namespace Thunder
{
	void GameResource::Serialize(MemoryWriter& archive)
	{
		archive << Guid;

		const auto dependencyCount = static_cast<uint32>(Dependencies.size());
		archive << dependencyCount;
		for (const auto& dep : Dependencies)
		{
			archive << dep;
		}
	}

	void GameResource::DeSerialize(MemoryReader& archive)
	{
		archive >> Guid;
		
		uint32 dependencyCount;
		archive >> dependencyCount;
		
		Dependencies.clear();
		Dependencies.reserve(dependencyCount);
		
		for (uint32 i = 0; i < dependencyCount; ++i)
		{
			TGuid dep;
			archive >> dep;
			Dependencies.push_back(dep);
		}
	}
}
