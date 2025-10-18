#pragma optimize("", off)
#include "GameObject.h"

namespace Thunder
{
	void GameResource::Serialize(MemoryWriter& archive)
	{
		const uint32 dependencyCount = static_cast<uint32>(Dependencies.size());
		archive << dependencyCount;
		for (const TGuid& dep : Dependencies)
		{
			archive << dep;
		}
	}

	void GameResource::DeSerialize(MemoryReader& archive)
	{
		uint32 dependencyCount;
		archive >> dependencyCount;
		
		Dependencies.clear();
		Dependencies.resize(dependencyCount);
		
		for (uint32 i = 0; i < dependencyCount; ++i)
		{
			TGuid dep;
			archive >> dep;
			Dependencies.push_back(dep);
		}
	}
}
