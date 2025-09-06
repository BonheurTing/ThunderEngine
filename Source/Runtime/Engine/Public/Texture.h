#pragma once
#include "GameObject.h"
#include "RenderResource.h"

namespace Thunder
{
	class ITexture : public GameResource
	{
	public:
		ITexture(GameObject* inOuter = nullptr, ETempGameResourceReflective inType = ETempGameResourceReflective::Unknown)
			: GameResource(inOuter, inType) {}

		~ITexture() override;

		// game object
		void OnResourceLoaded() override;

		// render resource
		virtual class RenderTexture* CreateResource_GameThread() = 0;
		ENGINE_API RenderTexture* GetResource();
		ENGINE_API void SetResource(RenderTexture* Resource);
		virtual void UpdateResource();
		void ReleaseResource();
		void InitResource();
		
	private:
		RenderTextureRef TextureResource {};
	};

	struct ImageData  : public RefCountedObject
	{
		int Width;
		int Height;
		int Channels;
		BinaryDataRef ImgData {};
    
		ImageData() : Width(0), Height(0), Channels(0) {}
		ImageData(int w, int h, int c)
			: Width(w), Height(h), Channels(c){ ImgData->Size = static_cast<size_t>(w) * h * c; }
		ImageData(int w, int h, int c, const unsigned char* d)
			: Width(w), Height(h), Channels(c)
		{
			SetData(d);
		}
		~ImageData() override
		{
			if (ImgData->Size > 0)
			{
				TMemory::Free(ImgData->Data);
			}
		}
		void SetData(const unsigned char* inData);

	};
	using ImageDataRef = TRefCountPtr<ImageData>;

	class Texture2D : public ITexture
	{
	public:
		Texture2D(GameObject* inOuter = nullptr)
			: ITexture(inOuter, ETempGameResourceReflective::Texture2D), Data(MakeRefCount<ImageData>()) {}
		Texture2D(unsigned char *data, int width, int height, int channels, GameObject* inOuter = nullptr)
			: ITexture(inOuter, ETempGameResourceReflective::Texture2D),
			Data(MakeRefCount<ImageData>(width, height, channels, data)) {}
		
		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;

		RenderTexture* CreateResource_GameThread() override;
	private:
		ImageDataRef Data {};
	};
}
