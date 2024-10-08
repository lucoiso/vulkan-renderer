// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Allocation;

import RenderCore.Runtime.Device;

using namespace RenderCore;

void ImageAllocation::DestroyResources(VmaAllocator const &Allocator)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(Allocator, Image, Allocation);
        Image      = VK_NULL_HANDLE;
        Allocation = VK_NULL_HANDLE;
    }

    if (View != VK_NULL_HANDLE)
    {
        vkDestroyImageView(LogicalDevice, View, nullptr);
        View = VK_NULL_HANDLE;

        if (Image != VK_NULL_HANDLE)
        {
            Image = VK_NULL_HANDLE;
        }
    }
}

void BufferAllocation::DestroyResources(VmaAllocator const &Allocator)
{
    if (Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        if (MappedData)
        {
            VmaAllocationInfo AllocationInfo;
            vmaGetAllocationInfo(Allocator, Allocation, &AllocationInfo);

            if (AllocationInfo.pMappedData != nullptr)
            {
                vmaUnmapMemory(Allocator, Allocation);
            }

            MappedData = nullptr;
        }

        vmaDestroyBuffer(Allocator, Buffer, Allocation);
        Allocation = VK_NULL_HANDLE;
        Buffer     = VK_NULL_HANDLE;
        Size       = 0U;
    }
}

VkDeviceSize BufferAllocation::GetAllocationSize(VmaAllocator const &Allocator) const
{
    if (Buffer == VK_NULL_HANDLE || Allocation == VK_NULL_HANDLE)
    {
        return 0U;
    }

    VmaAllocationInfo AllocationInfo;
    vmaGetAllocationInfo(Allocator, Allocation, &AllocationInfo);

    return AllocationInfo.size;
}

void DescriptorData::DestroyResources(VmaAllocator const &Allocator, bool const IncludeStatic)
{
    if (IncludeStatic)
    {
        if (SetLayout != VK_NULL_HANDLE)
        {
            VkDevice const &LogicalDevice = GetLogicalDevice();
            vkDestroyDescriptorSetLayout(LogicalDevice, SetLayout, nullptr);
            SetLayout = VK_NULL_HANDLE;
        }

        BufferDeviceAddress.deviceAddress = 0U;
        LayoutOffset                      = 0U;
        LayoutSize                        = 0U;
    }

    Buffer.DestroyResources(Allocator);
}

void DescriptorData::SetDescriptorLayoutSize(VkDeviceSize const &MinAlignment)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    vkGetDescriptorSetLayoutSizeEXT(LogicalDevice, SetLayout, &LayoutSize);
    LayoutSize = LayoutSize + MinAlignment - 1 & ~(MinAlignment - 1);

    vkGetDescriptorSetLayoutBindingOffsetEXT(LogicalDevice, SetLayout, 0U, &LayoutOffset);
}
