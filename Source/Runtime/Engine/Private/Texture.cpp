#include "Texture.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"		// 定义 stb_image 实现
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Concurrent/TaskScheduler.h"
#include "External/stb_image_write.h" // 定义 stb_image_write 实现

namespace Thunder
{
	ITexture::~ITexture()
	{
		ReleaseResource();
	}

	void ITexture::OnResourceLoaded()
	{
		GameResource::OnResourceLoaded();
		UpdateResource();
	}

	RenderTexture* ITexture::GetResource()
	{
		// check thread
		return TextureResource.Get();
	}

	void ITexture::SetResource(RenderTexture* Resource)
	{
		GRenderScheduler->PushTask([this, Resource]()
		{
			this->TextureResource = Resource;
		});
	}

	void ITexture::UpdateResource()
	{
		ReleaseResource();

		RenderTexture* NewResource = CreateResource_GameThread(); //纯虚函数
		SetResource(NewResource);
		
		InitResource();
	}

	void ITexture::ReleaseResource()
	{
		if (TextureResource)
		{
			GRenderScheduler->PushTask([Resource = this->TextureResource]()
			{
				Resource->ReleaseResource();
			});
		}
	}

	void ITexture::InitResource()
	{
		GRenderScheduler->PushTask([this]()
		{
			if (this->TextureResource)
			{
				this->TextureResource->InitResource();
			}
		});
	}

	
	RenderTexture* Texture2D::CreateResource_GameThread()
	{
		RHIFormat format;
		switch (Data->Channels)
		{
			case 1:
			format = RHIFormat::R8_UINT;
			break;
			case 2:
			format = RHIFormat::R8G8_UINT;
			break;
			case 4:
			format = RHIFormat::R8G8B8A8_UINT;
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
			for (int i = 0; i < Width * Height; i++) {
				dstData[i * 4 + 0] = inData[i * 3 + 0]; // R
				dstData[i * 4 + 1] = inData[i * 3 + 1]; // G
				dstData[i * 4 + 2] = inData[i * 3 + 2]; // B
				dstData[i * 4 + 3] = 255; // A - 填充为不透明
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

		// 使用stb_image_write将原始图像数据压缩为TGA格式
		int compressed_size;
		unsigned char* compressed_data = nullptr;
		
		// 定义回调函数将TGA数据写入内存
		struct WriteContext {
			std::vector<unsigned char>* buffer;
		};
		WriteContext context;
		std::vector<unsigned char> tga_buffer;
		context.buffer = &tga_buffer;
		
		// 使用stb_image_write的回调函数
		auto write_callback = [](void* context_ptr, void* data, int size) {
			WriteContext* ctx = static_cast<WriteContext*>(context_ptr);
			unsigned char* bytes = static_cast<unsigned char*>(data);
			ctx->buffer->insert(ctx->buffer->end(), bytes, bytes + size);
		};
		
		// 将原始数据压缩为TGA格式
		if (stbi_write_tga_to_func(write_callback, &context, Data->Width, Data->Height, Data->Channels, Data->ImgData->Data)) {
			// 序列化压缩后的数据大小和数据
			compressed_size = static_cast<int>(tga_buffer.size());
			archive << compressed_size;
			archive.WriteRaw(tga_buffer.data(), compressed_size);
		} else {
			// 压缩失败，写入0表示无数据
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

		// 读取压缩数据的大小
		int compressed_size;
		archive >> compressed_size;
		
		if (compressed_size > 0) {
			// 读取压缩的TGA数据
			std::vector<unsigned char> compressed_data(compressed_size);
			archive.ReadRaw(compressed_data.data(), compressed_size);
			
			// 使用stb_image从TGA数据解压缩为原始图像数据
			int loaded_width, loaded_height, loaded_channels;
			unsigned char* raw_data = stbi_load_from_memory(
				compressed_data.data(),
				compressed_size,
				&loaded_width,
				&loaded_height,
				&loaded_channels,
				Data->Channels  // 强制使用存储的通道数
			);
			
			if (raw_data) {
				// 验证解压后的图像尺寸是否匹配
				if (loaded_width == Data->Width && loaded_height == Data->Height) {
					Data->SetData(raw_data);
				}
				
				// 释放stb_image分配的内存
				stbi_image_free(raw_data);
			} else {
				// 解压失败，创建空的数据缓冲区
				Data->ImgData = {};
			}
		} else {
			// 没有压缩数据，创建空的数据缓冲区
			Data->ImgData = {};
		}
	}
}
