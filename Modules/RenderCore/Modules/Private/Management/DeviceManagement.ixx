// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Management.DeviceManagement;

export import <list>;
export import <vector>;
export import <string>;
export import <string_view>;

export import RenderCore.Types.DeviceProperties;

namespace RenderCore
{
    export void PickPhysicalDevice();
    export void CreateLogicalDevice();
    export void ReleaseDeviceResources();

    export [[nodiscard]] std::list<VkPhysicalDevice> GetAvailablePhysicalDevices();
    export [[nodiscard]] std::list<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions();
    export [[nodiscard]] std::list<VkLayerProperties> GetAvailablePhysicalDeviceLayers();
    export [[nodiscard]] std::list<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(std::string_view);
    export [[nodiscard]] std::list<std::string> GetAvailablePhysicalDeviceExtensionsNames();
    export [[nodiscard]] std::list<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view);
    export [[nodiscard]] std::list<std::string> GetAvailablePhysicalDeviceLayersNames();
    export [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities();
    export [[nodiscard]] std::list<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats();
    export [[nodiscard]] std::list<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes();
    export [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment();

    export bool UpdateDeviceProperties(GLFWwindow* Window);
    export [[nodiscard]] DeviceProperties& GetDeviceProperties();

    export [[nodiscard]] VkDevice& GetLogicalDevice();
    export [[nodiscard]] VkPhysicalDevice& GetPhysicalDevice();

    export [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetGraphicsQueue();
    export [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetPresentationQueue();
    export [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetTransferQueue();

    export [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    export [[nodiscard]] std::uint32_t GetMinImageCount();
}// namespace RenderCore