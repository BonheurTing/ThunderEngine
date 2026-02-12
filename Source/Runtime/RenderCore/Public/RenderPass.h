#pragma once
#include "RHIDefinition.h"
#include "Templates/RefCounting.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
    struct RenderPassKey
    {
        union
        {
            uint64 Hash[2] = { 0, 0 };

            struct
            {
                uint8 RenderTargetFormats[8];
                uint8 DepthStencilFormat;
                uint8 RenderTargetCount;

                uint8 Unused[6];
            };
        };
    };

    inline bool operator<(const RenderPassKey& a, const RenderPassKey& b) noexcept
    {
        return (a.Hash[0] == b.Hash[0]) ? (a.Hash[1] < b.Hash[1]) : (a.Hash[0] < b.Hash[0]);
    }

    inline bool operator==(const RenderPassKey& a, const RenderPassKey& b) noexcept
    {
        return (a.Hash[0] == b.Hash[0]) && (a.Hash[1] == b.Hash[1]);
    }

    class RenderPass : public RefCountedObject
    {
    public:
        RenderPass(RenderPassKey const& key) : Key(key) {}
        ~RenderPass() override = default;

        FORCEINLINE RenderPassKey const& GetKey() const { return Key; }
        FORCEINLINE uint32 GetRenderTargetCount() const { return Key.RenderTargetCount; }
        FORCEINLINE RHIFormat GetDepthStencilFormat() const { return static_cast<RHIFormat>(Key.DepthStencilFormat); }
        FORCEINLINE RHIFormat GetRenderTargetFormat(uint32 index) const
        {
            TAssertf(index < Key.RenderTargetCount, "Invalid render target index : %d.", index);
            return index < Key.RenderTargetCount ? static_cast<RHIFormat>(Key.RenderTargetFormats[index]) : RHIFormat::UNKNOWN;
        }

    protected:
        RenderPassKey Key;
        uint16 GlobalIndex = 0;
    };
    using RenderPassRef = TRefCountPtr<RenderPass>;
}

template<>
struct std::hash<Thunder::RenderPassKey>
{
    size_t operator()(const Thunder::RenderPassKey& value) const noexcept
    {
        size_t h1 = static_cast<size_t>(value.Hash[0]);
        size_t h2 = static_cast<size_t>(value.Hash[1]);
        return h1 ^ ((h2 << 21) | (h2 >> 43)) ^ (h2 >> 13);
    }
};

