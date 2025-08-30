#include "Texture.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"		// 定义 stb_image 实现
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "External/stb_image_write.h" // 定义 stb_image_write 实现

namespace Thunder
{
	Texture2D::Texture2D(unsigned char *data, int width, int height, int channels, GameObject* inOuter)
		: ITexture(inOuter, ETempGameResourceReflective::Texture2D),
		Data(MakeRefCount<ImageData>(width, height, channels))
	{
		Data->data.assign(data, data + (width * height * channels));
	}

	void Texture2D::Serialize(MemoryWriter& archive)
	{
		GameResource::Serialize(archive);
		archive << Data->width;
		archive << Data->height;
		archive << Data->channels;

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
		if (stbi_write_tga_to_func(write_callback, &context, Data->width, Data->height, Data->channels, Data->data.data())) {
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
		archive >> Data->width;
		archive >> Data->height;
		archive >> Data->channels;

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
				Data->channels  // 强制使用存储的通道数
			);
			
			if (raw_data) {
				// 验证解压后的图像尺寸是否匹配
				if (loaded_width == Data->width && loaded_height == Data->height) {
					// 将解压后的数据复制到ImageData中
					int data_size = Data->width * Data->height * Data->channels;
					Data->data.assign(raw_data, raw_data + data_size);
				}
				
				// 释放stb_image分配的内存
				stbi_image_free(raw_data);
			} else {
				// 解压失败，创建空的数据缓冲区
				Data->data.clear();
				Data->data.resize(Data->width * Data->height * Data->channels, 0);
			}
		} else {
			// 没有压缩数据，创建空的数据缓冲区
			Data->data.clear();
			Data->data.resize(Data->width * Data->height * Data->channels, 0);
		}
	}
}
