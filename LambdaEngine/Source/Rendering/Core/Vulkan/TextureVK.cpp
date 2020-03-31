#include "Log/Log.h"

#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	TextureVK::TextureVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_Desc()
	{
	}

	TextureVK::~TextureVK()
	{
		if (m_Image != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_pDevice->Device, m_Image, nullptr);
			m_Image = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_pDevice->Device, m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}
	}

	bool TextureVK::Init(const TextureDesc& desc)
	{
		VkImageCreateInfo info = {};
		info.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext					= nullptr;
		info.flags					= 0;
		info.format					= ConvertFormat(desc.Format);
		info.extent.width			= desc.Width;
		info.extent.height			= desc.Height;
		info.extent.depth			= desc.Depth;
		info.arrayLayers			= desc.ArrayCount;
		info.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		info.mipLevels				= desc.Miplevels;
		info.pQueueFamilyIndices	= nullptr;
		info.queueFamilyIndexCount	= 0;
		info.samples				= ConvertSamples(desc.SampleCount);
		info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		info.tiling					= VK_IMAGE_TILING_OPTIMAL;
		
		if (desc.Flags & ETextureFlags::TEXTURE_FLAG_RENDER_TARGET)
		{
			info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (desc.Flags & ETextureFlags::TEXTURE_FLAG_SHADER_RESOURCE)
		{
			info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (desc.Flags & ETextureFlags::TEXTURE_FLAG_DEPTH_STENCIL)
		{
			info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if (desc.Flags & ETextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS)
		{
			info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (desc.Flags & ETextureFlags::TEXTURE_FLAG_COPY_DST)
		{
			info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (desc.Flags & ETextureFlags::TEXTURE_FLAG_COPY_SRC)
		{
			info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (desc.Type == ETextureType::TEXTURE_1D)
		{
			info.imageType = VK_IMAGE_TYPE_1D;
		}
		else if (desc.Type == ETextureType::TEXTURE_2D)
		{
			info.imageType = VK_IMAGE_TYPE_2D;
		}
		else if (desc.Type == ETextureType::TEXTURE_3D)
		{
			info.imageType = VK_IMAGE_TYPE_3D;
		}

		VkResult result = vkCreateImage(m_pDevice->Device, &info, nullptr, &m_Image);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("TextureVK: Failed to create texture");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("TextureVK: Created texture w=%d, h=%d, d=%d", desc.Width, desc.Height, desc.Depth);

			m_Desc = desc;
			SetName(m_Desc.pName);
		}

		//TODO: Allocate with DeviceAllocator
		VkMemoryRequirements memoryRequirements = { };
		vkGetImageMemoryRequirements(m_pDevice->Device, m_Image, &memoryRequirements);

		VkMemoryPropertyFlags memoryProperties = 0;
		if (m_Desc.MemoryType == EMemoryType::CPU_MEMORY)
		{
			memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else if (m_Desc.MemoryType == EMemoryType::GPU_MEMORY)
		{
			memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		int32 memoryTypeIndex = FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, memoryProperties);

		VkMemoryAllocateInfo allocateInfo = { };
		allocateInfo.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext				= nullptr;
		allocateInfo.memoryTypeIndex	= memoryTypeIndex;
		allocateInfo.allocationSize		= memoryRequirements.size;

		result = vkAllocateMemory(m_pDevice->Device, &allocateInfo, nullptr, &m_Memory);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("TextureVK: Failed to allocate memory");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("TextureVK: Allocated %d bytes", memoryRequirements.size);
		}

		result = vkBindImageMemory(m_pDevice->Device, m_Image, m_Memory, 0);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("TextureVK: Failed to bind memory");
			return false;
		}

		return true;
	}
	
	void TextureVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Image, VK_OBJECT_TYPE_IMAGE);
	}
}