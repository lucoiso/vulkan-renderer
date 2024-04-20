// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <ranges>
#include <stb_image_write.h>
#include <glm/matrix.hpp>
#include <Volk/volk.h>

#ifndef VMA_IMPLEMENTATION
#include <boost/log/trivial.hpp>
#define VMA_LEAK_LOG_FORMAT(format, ...)                                       \
        do                                                                     \
        {                                                                      \
            std::size_t _BuffSize = snprintf(nullptr, 0, format, __VA_ARGS__); \
            std::string _Message(_BuffSize + 1, '\0');                         \
            snprintf(&_Message[0], _Message.size(), format, __VA_ARGS__);      \
            _Message.pop_back();                                               \
            BOOST_LOG_TRIVIAL(debug) << _Message;                              \
        }                                                                      \
        while (false)
#define VMA_IMPLEMENTATION
#endif
#include <vma/vk_mem_alloc.h>

module RenderCore.Runtime.Memory;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Scene;
import RenderCore.Types.Object;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

VmaPool      g_StagingBufferPool { VK_NULL_HANDLE };
VmaPool      g_BufferPool { VK_NULL_HANDLE };
VmaPool      g_ImagePool { VK_NULL_HANDLE };
VmaAllocator g_Allocator { VK_NULL_HANDLE };

std::atomic<std::uint64_t>                          g_BufferAllocationIDCounter { 0U };
std::unordered_map<std::uint32_t, BufferAllocation> g_AllocatedBuffers {};
std::unordered_map<std::uint32_t, std::uint32_t>    g_BufferAllocationCounter {};

std::atomic<std::uint64_t>                         g_ImageAllocationIDCounter { 0U };
std::unordered_map<std::uint32_t, ImageAllocation> g_AllocatedImages {};
std::unordered_map<std::uint32_t, std::uint32_t>   g_ImageAllocationCounter {};

void RenderCore::CreateMemoryAllocator(VkPhysicalDevice const &PhysicalDevice)
{
    VmaVulkanFunctions const VulkanFunctions { .vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr };

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
            .physicalDevice = PhysicalDevice,
            .device = volkGetLoadedDevice(),
            .preferredLargeHeapBlockSize = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = &VulkanFunctions,
            .instance = volkGetLoadedInstance(),
            .vulkanApiVersion = VK_API_VERSION_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr
    };

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &g_Allocator));

    {
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .usage = g_ModelMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0x100, .usage = g_ModelMemoryFlags };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo { .memoryTypeIndex = MemoryType, .flags = 0U, .minBlockCount = 0U, .priority = 0.F };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_StagingBufferPool));
    }

    {
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .usage = g_ModelMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0x100, .usage = g_ModelMemoryFlags };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
                .memoryTypeIndex = MemoryType,
                .flags = 0U,
                .blockSize = g_BufferMemoryAllocationSize,
                .minBlockCount = g_MinMemoryBlock,
                .priority = 1.F,
                .minAllocationAlignment = GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_BufferPool));
    }

    {
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .usage = g_TextureMemoryUsage };

        constexpr VkImageCreateInfo ImageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .extent = { .width = 4U, .height = 4U, .depth = 1U },
                .mipLevels = 1U,
                .arrayLayers = 1U,
                .samples = g_MSAASamples,
                .tiling = g_ImageTiling,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForImageInfo(g_Allocator, &ImageViewCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
                .memoryTypeIndex = MemoryType,
                .flags = 0U,
                .blockSize = g_ImageMemoryAllocationSize,
                .minBlockCount = g_MinMemoryBlock,
                .priority = 1.F
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_ImagePool));
    }
}

void RenderCore::ReleaseMemoryResources()
{
    for (auto &Buffer : g_AllocatedBuffers | std::views::values)
    {
        Buffer.DestroyResources(g_Allocator);
    }
    g_AllocatedBuffers.clear();

    for (auto &ImageIter : g_AllocatedImages | std::views::values)
    {
        ImageIter.DestroyResources(g_Allocator);
    }
    g_AllocatedImages.clear();

    vmaDestroyPool(g_Allocator, g_StagingBufferPool);
    g_StagingBufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_BufferPool);
    g_BufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_ImagePool);
    g_ImagePool = VK_NULL_HANDLE;

    vmaDestroyAllocator(g_Allocator);
    g_Allocator = VK_NULL_HANDLE;
}

VmaAllocator const &RenderCore::GetAllocator()
{
    return g_Allocator;
}

VmaAllocationInfo RenderCore::CreateBuffer(VkDeviceSize const &     Size,
                                           VkBufferUsageFlags const Usage,
                                           std::string_view const   Identifier,
                                           VkBuffer &               Buffer,
                                           VmaAllocation &          Allocation)
{
    VmaAllocationCreateInfo const AllocationCreateInfo {
            .usage = g_ModelMemoryUsage,
            .pool = Size > g_BufferMemoryAllocationSize ? g_StagingBufferPool : g_BufferPool
    };

    VkBufferCreateInfo const BufferCreateInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = Size, .usage = Usage };

    VmaAllocationInfo MemoryAllocationInfo;
    CheckVulkanResult(vmaCreateBuffer(GetAllocator(), &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    vmaSetAllocationName(GetAllocator(), Allocation, std::data(std::format("Buffer: {}", Identifier)));

    return MemoryAllocationInfo;
}

void RenderCore::CopyBuffer(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkBuffer const &Destination, VkDeviceSize const &Size)
{
    VkBufferCopy const BufferCopy { .size = Size, };
    vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
}

void RenderCore::CreateUniformBuffers(BufferAllocation &BufferAllocation, VkDeviceSize const BufferSize, std::string_view const Identifier)
{
    constexpr VkBufferUsageFlags DestinationUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    CreateBuffer(BufferSize, DestinationUsageFlags, Identifier, BufferAllocation.Buffer, BufferAllocation.Allocation);
    vmaMapMemory(GetAllocator(), BufferAllocation.Allocation, &BufferAllocation.MappedData);
}

void RenderCore::CreateImage(VkFormat const &        ImageFormat,
                             VkExtent2D const &      Extent,
                             VkImageTiling const &   Tiling,
                             VkImageUsageFlags const ImageUsage,
                             VmaMemoryUsage const    MemoryUsage,
                             std::string_view const  Identifier,
                             VkImage &               Image,
                             VmaAllocation &         Allocation)
{
    VkImageCreateInfo const ImageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ImageFormat,
            .extent = { .width = Extent.width, .height = Extent.height, .depth = 1U },
            .mipLevels = 1U,
            .arrayLayers = 1U,
            .samples = g_MSAASamples,
            .tiling = Tiling,
            .usage = ImageUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo const ImageCreateInfo { .usage = MemoryUsage, .pool = g_ImagePool, .priority = 1.F };

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(GetAllocator(), &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));

    vmaSetAllocationName(GetAllocator(), Allocation, std::data(std::format("Image: {}", Identifier)));
}

void RenderCore::CreateTextureSampler(VkPhysicalDevice const &Device, VkSampler &Sampler)
{
    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(Device, &SurfaceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.F,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = SurfaceProperties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.F,
            .maxLod = FLT_MAX,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
    };

    CheckVulkanResult(vkCreateSampler(volkGetLoadedDevice(), &SamplerCreateInfo, nullptr, &Sampler));
}

void RenderCore::CreateImageView(VkImage const &Image, VkFormat const &Format, VkImageAspectFlags const &AspectFlags, VkImageView &ImageView)
{
    VkImageViewCreateInfo const ImageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = Image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = Format,
            .subresourceRange = { .aspectMask = AspectFlags, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U }
    };

    CheckVulkanResult(vkCreateImageView(volkGetLoadedDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void RenderCore::CreateTextureImageView(ImageAllocation &Allocation, VkFormat const ImageFormat)
{
    CreateImageView(Allocation.Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void RenderCore::CopyBufferToImage(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkImage const &Destination, VkExtent2D const &Extent)
{
    VkBufferImageCopy const BufferImageCopy {
            .bufferOffset = 0U,
            .bufferRowLength = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U },
            .imageOffset = { .x = 0U, .y = 0U, .z = 0U },
            .imageExtent = { .width = Extent.width, .height = Extent.height, .depth = 1U }
    };

    vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
}

std::tuple<std::uint32_t, VkBuffer, VmaAllocation> RenderCore::AllocateTexture(VkCommandBuffer &    CommandBuffer,
                                                                               unsigned char const *Data,
                                                                               std::uint32_t const  Width,
                                                                               std::uint32_t const  Height,
                                                                               VkFormat const       ImageFormat,
                                                                               VkDeviceSize const   AllocationSize)
{
    if (std::empty(g_AllocatedImages))
    {
        g_ImageAllocationIDCounter.fetch_sub(g_ImageAllocationIDCounter.load());
    }

    std::pair<VkBuffer, VmaAllocation> Output;
    VmaAllocationInfo                  StagingInfo = CreateBuffer(AllocationSize, g_ModelMemoryUsage, "STAGING_TEXTURE", Output.first, Output.second);

    CheckVulkanResult(vmaMapMemory(GetAllocator(), Output.second, &StagingInfo.pMappedData));
    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    ImageAllocation NewAllocation {
        .Extent = { .width = Width, .height = Height },
        .Format = ImageFormat
    };

    CreateImage(ImageFormat,
                NewAllocation.Extent,
                g_ImageTiling,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                g_TextureMemoryUsage,
                "TEXTURE",
                NewAllocation.Image,
                NewAllocation.Allocation);

    MoveImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffer,
        NewAllocation.Image,
        NewAllocation.Format);

    CopyBufferToImage(CommandBuffer, Output.first, NewAllocation.Image, NewAllocation.Extent);

    MoveImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffer,
        NewAllocation.Image,
        NewAllocation.Format);

    CreateImageView(NewAllocation.Image, NewAllocation.Format, VK_IMAGE_ASPECT_COLOR_BIT, NewAllocation.View);
    vmaUnmapMemory(GetAllocator(), Output.second);

    std::uint32_t const BufferID = g_ImageAllocationIDCounter.fetch_add(1U);
    g_AllocatedImages.emplace(BufferID, std::move(NewAllocation));

    if (g_ImageAllocationCounter.contains(BufferID))
    {
        g_ImageAllocationCounter.at(BufferID) += 1U;
    }
    else
    {
        g_ImageAllocationCounter.emplace(BufferID, 1U);
    }

    return { BufferID, Output.first, Output.second };
}

void RenderCore::AllocateModelsBuffers(std::vector<std::shared_ptr<Object>> const &Objects)
{
    if (std::empty(g_AllocatedBuffers))
    {
        g_BufferAllocationIDCounter.fetch_sub(g_BufferAllocationIDCounter.load());
    }

    std::vector<Vertex>        Vertices;
    std::vector<std::uint32_t> Indices;

    for (auto const &ObjectIter : Objects)
    {
        auto const &Mesh = ObjectIter->GetMesh();

        Mesh->SetVertexOffset(std::size(Vertices) * sizeof(Vertex));
        Mesh->SetIndexOffset(std::size(Indices) * sizeof(std::uint32_t));

        Vertices.insert(std::end(Vertices), std::begin(Mesh->GetVertices()), std::end(Mesh->GetVertices()));
        Indices.insert(std::end(Indices), std::begin(Mesh->GetIndices()), std::end(Mesh->GetIndices()));
    }

    VkDeviceSize const VertexBufferSize = std::size(Vertices) * sizeof(Vertex);
    VkDeviceSize const IndexBufferSize  = std::size(Indices) * sizeof(std::uint32_t);
    VkDeviceSize       UniformOffset    = VertexBufferSize + IndexBufferSize;

    if (VkDeviceSize const MinAlignment = GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
        MinAlignment > 0U)
    {
        UniformOffset = UniformOffset + MinAlignment - 1U & ~(MinAlignment - 1U);
    }

    std::uint32_t const BufferID      = g_BufferAllocationIDCounter.fetch_add(1U);
    BufferAllocation    NewAllocation {};

    VkDeviceSize const BufferSize = UniformOffset + sizeof(ModelUniformData) * std::size(Objects);

    CreateBuffer(BufferSize, g_ModelMemoryFlags, "MODEL_UNIFIED_BUFFER", NewAllocation.Buffer, NewAllocation.Allocation);

    CheckVulkanResult(vmaMapMemory(GetAllocator(), NewAllocation.Allocation, &NewAllocation.MappedData));

    std::memcpy(NewAllocation.MappedData, std::data(Vertices), VertexBufferSize);
    std::memcpy(static_cast<char *>(NewAllocation.MappedData) + VertexBufferSize, std::data(Indices), IndexBufferSize);

    CheckVulkanResult(vmaFlushAllocation(GetAllocator(), NewAllocation.Allocation, VertexBufferSize, IndexBufferSize));
    g_AllocatedBuffers.emplace(BufferID, std::move(NewAllocation));

    for (auto const &ObjectIter : Objects)
    {
        auto const &Mesh = ObjectIter->GetMesh();

        Mesh->SetIndexOffset(Mesh->GetIndexOffset() + VertexBufferSize);

        ObjectIter->SetBufferIndex(BufferID);
        ObjectIter->SetUniformOffset(UniformOffset + sizeof(ModelUniformData) * std::distance(std::data(Objects), &ObjectIter));
        ObjectIter->SetupUniformDescriptor();
    }

    if (g_BufferAllocationCounter.contains(BufferID))
    {
        g_BufferAllocationCounter.at(BufferID) += static_cast<std::uint32_t>(std::size(Objects));
    }
    else
    {
        g_BufferAllocationCounter.emplace(BufferID, static_cast<std::uint32_t>(std::size(Objects)));
    }
}

VkBuffer const &RenderCore::GetAllocationBuffer(std::uint32_t const Index)
{
    return g_AllocatedBuffers.at(Index).Buffer;
}

void *RenderCore::GetAllocationMappedData(std::uint32_t const Index)
{
    return g_AllocatedBuffers.at(Index).MappedData;
}

VkDescriptorBufferInfo RenderCore::GetAllocationBufferDescriptor(std::uint32_t const Index, std::uint32_t const Offset, std::uint32_t const Range)
{
    return VkDescriptorBufferInfo { .buffer = GetAllocationBuffer(Index), .offset = Offset, .range = Range };
}

VkDescriptorImageInfo RenderCore::GetAllocationImageDescriptor(std::uint32_t const Index)
{
    return VkDescriptorImageInfo {
            .sampler = GetSampler(),
            .imageView = g_AllocatedImages.at(Index).View,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
    };
}

void RenderCore::SaveImageToFile(VkImage const &Image, std::string_view const Path, VkExtent2D const &Extent)
{
    VkBuffer            Buffer;
    VmaAllocation       Allocation;
    VkSubresourceLayout Layout;

    constexpr std::uint8_t Components { 4U };

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffer { VK_NULL_HANDLE };
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, FamilyIndex);
    {
        VkImageSubresource SubResource { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .arrayLayer = 0 };

        vkGetImageSubresourceLayout(volkGetLoadedDevice(), Image, &SubResource, &Layout);

        VkBufferCreateInfo BufferInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = Layout.size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo AllocationInfo { .usage = VMA_MEMORY_USAGE_CPU_ONLY };

        vmaCreateBuffer(g_Allocator, &BufferInfo, &AllocationInfo, &Buffer, &Allocation, nullptr);

        VkBufferImageCopy Region {
                .bufferOffset = 0U,
                .bufferRowLength = 0U,
                .bufferImageHeight = 0U,
                .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U },
                .imageOffset = { 0U, 0U, 0U },
                .imageExtent = { .width = Extent.width, .height = Extent.height, .depth = 1U }
        };

        VkImageMemoryBarrier2 PreCopyBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0U,
                        .levelCount = 1U,
                        .baseArrayLayer = 0U,
                        .layerCount = 1U
                }
        };

        VkImageMemoryBarrier2 PostCopyBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR,
                .dstAccessMask = VK_ACCESS_2_NONE_KHR,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0U,
                        .levelCount = 1U,
                        .baseArrayLayer = 0U,
                        .layerCount = 1U
                }
        };

        VkDependencyInfo DependencyInfo {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                .imageMemoryBarrierCount = 1U,
                .pImageMemoryBarriers = &PreCopyBarrier
        };

        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
        vkCmdCopyImageToBuffer(CommandBuffer.back(), Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Buffer, 1U, &Region);

        DependencyInfo.pImageMemoryBarriers = &PostCopyBarrier;
        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);

    void *ImageData;
    vmaMapMemory(g_Allocator, Allocation, &ImageData);

    auto ImagePixels = static_cast<unsigned char *>(ImageData);

    stbi_write_png(std::data(Path), Extent.width, Extent.height, Components, ImagePixels, Extent.width * Components);

    vmaUnmapMemory(g_Allocator, Allocation);
    vmaDestroyBuffer(g_Allocator, Buffer, Allocation);
}

void ObjectDeleter::operator()(Object const *const Object) const
{
    std::uint32_t const BufferIndex = Object->GetBufferIndex();
    g_BufferAllocationCounter.at(BufferIndex) -= 1U;

    if (g_BufferAllocationCounter.at(BufferIndex) == 0U)
    {
        g_AllocatedBuffers.at(BufferIndex).DestroyResources(g_Allocator);
        g_AllocatedBuffers.erase(BufferIndex);
        g_BufferAllocationCounter.erase(BufferIndex);
    }
}

void TextureDeleter::operator()(Texture const *const Texture) const
{
    std::uint32_t const BufferIndex = Texture->GetBufferIndex();
    g_ImageAllocationCounter.at(BufferIndex) -= 1U;

    if (g_ImageAllocationCounter.at(BufferIndex) == 0U)
    {
        g_AllocatedImages.at(BufferIndex).DestroyResources(g_Allocator);
        g_AllocatedImages.erase(BufferIndex);
        g_ImageAllocationCounter.erase(BufferIndex);
    }
}

void MeshDeleter::operator()(Mesh const *const Mesh) const
{
    std::uint32_t const BufferIndex = Mesh->GetBufferIndex();
    g_BufferAllocationCounter.at(BufferIndex) -= 1U;

    if (g_BufferAllocationCounter.at(BufferIndex) == 0U)
    {
        g_AllocatedBuffers.at(BufferIndex).DestroyResources(g_Allocator);
        g_AllocatedBuffers.erase(BufferIndex);
        g_BufferAllocationCounter.erase(BufferIndex);
    }
}