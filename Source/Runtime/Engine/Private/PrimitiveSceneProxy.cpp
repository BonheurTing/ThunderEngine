#include "PrimitiveSceneProxy.h"
#include "Compomemt.h"

namespace Thunder
{
    PrimitiveSceneProxy::PrimitiveSceneProxy(const PrimitiveComponent* inComponent)
    {
        
    }

    StaticMeshSceneProxy::StaticMeshSceneProxy(StaticMeshComponent* inComponent)
        : PrimitiveSceneProxy(inComponent)
    {
        
    }

    void StaticMeshSceneProxy::GetDynamicMeshElements()
    {

        
    }
}
