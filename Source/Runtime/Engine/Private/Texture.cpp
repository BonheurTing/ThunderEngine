#include "Texture.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Concurrent/TaskScheduler.h"
#include "External/stb_image_write.h"
#include "RenderModule.h"

namespace Thunder
{
	ITexture::~ITexture()
	{
		ReleaseResource();
	}

	void ITexture::OnResourceLoaded()
	{
		LOG("Texture loaded: %s", GetResourceName().c_str());
		UpdateResource();
	}

	RenderTexture* ITexture::GetResource()
	{
		// check thread
		return TextureResource.Get();
	}

	void ITexture::SetResource(RenderTexture* resource)
	{
		GRenderScheduler->PushTask([this, resource]()
		{
			this->TextureResource = resource;
		});
	}

	void ITexture::UpdateResource()
	{
		ReleaseResource();

		TextureResource = CreateResource_GameThread();

		InitResource();
	}

	void ITexture::ReleaseResource()
	{
		if (TextureResource)
		{
			TGuid guid = GetGUID();
			GRenderScheduler->PushTask([resource = this->TextureResource]()
			{
				resource->ReleaseResource();
			});
			RenderModule::UnregisterTexture_GameThread(guid);
		}
	}

	void ITexture::InitResource()
	{
		TGuid guid = GetGUID();
		RenderTexture* resource = TextureResource.Get();
		RenderModule::RegisterTexture_GameThread(guid, resource);

		GRenderScheduler->PushTask([resource]()
		{
			if (resource)
			{
				resource->InitResource();
			}
		});
	}

	
	RenderTexture* Texture2D::CreateResource_GameThread()
	{
		RHIFormat format;
		switch (Data->Channels)
		{
			case 1:
			format = RHIFormat::R8_UNORM;
			break;
			case 2:
			format = RHIFormat::R8G8_UNORM;
			break;
			case 4:
			format = RHIFormat::R8G8B8A8_UNORM;
			break;
			default:
			TAssertf(false, "Invalid format");
			return nullptr;
		}
		RenderTexture2D* Texture2DResource = new RenderTexture2D(Data->Width, Data->Height, format, Data->ImgData);
		return Texture2DResource;
	}

	// -----------------------------------GameObject----------------------------------------------------------

	void ImageData::SetData(const unsigned char* inData)
	{
		TAssert(Width > 0);
		if (!ImgData.IsValid())
		{
			ImgData = MakeRefCount<BinaryData>();
		}
		ImgData->Size = static_cast<size_t>(Width) * Height * 4;
		ImgData->Data = TMemory::Malloc(ImgData->Size);
		if (Channels == 4)
		{
			memcpy(ImgData->Data, inData, ImgData->Size);
		}
		else if (Channels == 3)
		{
			unsigned char * dstData = static_cast<unsigned char *>(ImgData->Data);
			for (int i = 0; i < Width * Height; i++)
			{
				dstData[i * 4 + 0] = inData[i * 3 + 0]; // R
				dstData[i * 4 + 1] = inData[i * 3 + 1]; // G
				dstData[i * 4 + 2] = inData[i * 3 + 2]; // B
				dstData[i * 4 + 3] = 255; // A
			}
			Channels = 4;
		}
		else if (Channels == 1) // Todo : real single channel texture
		{
			unsigned char * dstData = static_cast<unsigned char *>(ImgData->Data);
			for (int i = 0; i < Width * Height; i++)
			{
				dstData[i * 4 + 0] = inData[i]; // R
				dstData[i * 4 + 1] = inData[i]; // G
				dstData[i * 4 + 2] = inData[i]; // B
				dstData[i * 4 + 3] = 255; // A
			}
			Channels = 4;
		}
		else
		{
			TAssertf(false, "Invalid Data");
		}
	}

	void Texture2D::Serialize(MemoryWriter& archive)
	{
		GameResource::Serialize(archive);
		archive << Data->Width;
		archive << Data->Height;
		archive << Data->Channels;

		int compressed_size;
		unsigned char* compressed_data = nullptr;

		struct WriteContext {
			std::vector<unsigned char>* buffer;
		};
		WriteContext context;
		std::vector<unsigned char> tga_buffer;
		context.buffer = &tga_buffer;

		auto write_callback = [](void* context_ptr, void* data, int size) {
			WriteContext* ctx = static_cast<WriteContext*>(context_ptr);
			unsigned char* bytes = static_cast<unsigned char*>(data);
			ctx->buffer->insert(ctx->buffer->end(), bytes, bytes + size);
		};

		if (stbi_write_tga_to_func(write_callback, &context, Data->Width, Data->Height, Data->Channels, Data->ImgData->Data))
		{
			compressed_size = static_cast<int>(tga_buffer.size());
			archive << compressed_size;
			archive.WriteRaw(tga_buffer.data(), compressed_size);
		}
		else
		{
			compressed_size = 0;
			archive << compressed_size;
		}
	}

	void Texture2D::DeSerialize(MemoryReader& archive)
	{
		GameResource::DeSerialize(archive);
		archive >> Data->Width;
		archive >> Data->Height;
		archive >> Data->Channels;

		int compressed_size;
		archive >> compressed_size;
		
		if (compressed_size > 0)
		{
			std::vector<unsigned char> compressed_data(compressed_size);
			archive.ReadRaw(compressed_data.data(), compressed_size);

			int loaded_width, loaded_height, loaded_channels;
			unsigned char* raw_data = stbi_load_from_memory(
				compressed_data.data(),
				compressed_size,
				&loaded_width,
				&loaded_height,
				&loaded_channels,
				Data->Channels
			);
			
			if (raw_data)
			{
				if (loaded_width == Data->Width && loaded_height == Data->Height)
				{
					Data->SetData(raw_data);
				}
				
				stbi_image_free(raw_data);
			}
			else
			{
				Data->ImgData = {};
			}
		}
		else
		{
			Data->ImgData = {};
		}
	}
}
