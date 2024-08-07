// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <algorithm>
#include <execution>
#include <source_location>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <Volk/volk.h>

export module RenderCore.Utils.Helpers;

import RenderCore.Runtime.Instance;

export namespace RenderCore
{
    void EmitFatalError(std::string_view, std::source_location const &Location = std::source_location::current());

    void CheckVulkanResult(VkResult InputOperation, std::source_location const &Location = std::source_location::current());

    template <typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, VkResult> || std::is_same_v<std::invoke_result_t<T>, VkResult &>)
    constexpr void CheckVulkanResult(T &&InputOperation, std::source_location const &Location = std::source_location::current())
    {
        CheckVulkanResult(InputOperation(), Location);
    }

    constexpr bool DepthHasStencil(VkFormat const &Format)
    {
        return Format >= VK_FORMAT_D16_UNORM_S8_UINT && Format <= VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    template <typename T>
    constexpr T LoadVulkanProcedure(std::string_view const ProcedureName)
    {
        return reinterpret_cast<T>(vkGetInstanceProcAddr(GetInstance(), ProcedureName.data()));
    }

    [[nodiscard]] VkExtent2D GetWindowExtent(GLFWwindow *, VkSurfaceCapabilitiesKHR const &);

    [[nodiscard]] std::vector<std::string> GetGLFWExtensions();

    [[nodiscard]] std::vector<VkLayerProperties> GetAvailableInstanceLayers();

    [[nodiscard]] std::vector<std::string> GetAvailableInstanceLayersNames();

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

    [[nodiscard]] std::vector<std::string> GetAvailableInstanceExtensionsNames();

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceLayerExtensions(std::string_view);

    [[nodiscard]] std::vector<std::string> GetAvailableInstanceLayerExtensionsNames(std::string_view);

    [[nodiscard]] VkVertexInputBindingDescription GetBindingDescriptors(std::uint32_t);

    [[nodiscard]] std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(std::uint32_t,
                                                                                          std::vector<VkVertexInputAttributeDescription> const &);

    template <typename ItemType, typename ContainerType>
    constexpr bool Contains(ContainerType const &Container, ItemType const &Item)
    {
        return std::ranges::find(Container, Item) != std::cend(Container);
    }

    template <typename Out, typename Opt, typename Avail>
    constexpr void GetAvailableResources(std::string_view const Identifier, Out &Resource, Opt const &Optional, Avail const &Available)
    {
        std::for_each(std::execution::unseq,
                      std::cbegin(Resource),
                      std::cend(Resource),
                      [&Available, &Identifier](auto const &ResIter)
                      {
                          if (std::ranges::find(Available, ResIter) == std::cend(Available))
                          {
                              auto ErrorMessage = "Required " + std::string { Identifier } + " not available: " + std::string { ResIter };
                              EmitFatalError(std::move(ErrorMessage));
                          }
                      });

        std::for_each(std::execution::unseq,
                      std::cbegin(Optional),
                      std::cend(Optional),
                      [&Available, &Resource](auto const &OptIter)
                      {
                          if (std::ranges::find(Available, OptIter) != std::cend(Available))
                          {
                              Resource.emplace_back(OptIter);
                          }
                      });
    }
} // namespace RenderCore
