#pragma once
#include "GameObject.h"

namespace Thunder
{
	class ITexture : public GameResource
	{
	public:
		ITexture() = default;
		virtual ~ITexture() = default;
	};

	struct ImageData  : public RefCountedObject
	{
		int width;
		int height;
		int channels;
		std::vector<unsigned char> data;
    
		ImageData() : width(0), height(0), channels(0) {}
		ImageData(int w, int h, int c)
			: width(w), height(h), channels(c){ data.clear(); data.resize(w*h*c); }
		ImageData(int w, int h, int c, const std::vector<unsigned char>& d)
			: width(w), height(h), channels(c), data(d) {}
	};
	using TImageDataRef = TRefCountPtr<ImageData>;

	class Texture2D : public ITexture
	{
	public:
		Texture2D(unsigned char *data, int width, int height, int channels);
		
		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;
	private:
		TImageDataRef Data {};
	};
}
