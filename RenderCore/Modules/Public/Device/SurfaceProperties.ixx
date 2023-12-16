// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export struct SurfaceProperties
    {
        VkSurfaceFormatKHR Format;
        VkFormat DepthFormat;
        VkPresentModeKHR Mode;
        VkExtent2D Extent;

        [[nodiscard]] bool IsValid() const
        {
            return Extent.height != 0U && Extent.width != 0U;
        }

        bool operator!=(SurfaceProperties const& Other) const
        {
            return Format.format != Other.Format.format
                   || Format.colorSpace != Other.Format.colorSpace
                   || DepthFormat != Other.DepthFormat
                   || Mode != Other.Mode
                   || Extent.height != Other.Extent.height
                   || Extent.width != Other.Extent.width;
        }
    };
}// namespace RenderCore