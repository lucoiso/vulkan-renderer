// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>
#include <string>
#include <GLFW/glfw3.h>

export module RenderCore.Integrations.GLFWHandler;

import RenderCore.UserInterface.Window.Flags;

namespace RenderCore
{
    export class GLFWHandler final
    {
        GLFWwindow *m_Window {nullptr};

    public:
        [[nodiscard]] bool Initialize(std::uint16_t, std::uint16_t, std::string_view, InitializationFlags);
        void               Shutdown();

        [[nodiscard]] GLFWwindow *GetWindow() const;
        [[nodiscard]] bool        IsOpen() const;
    };
} // namespace RenderCore
