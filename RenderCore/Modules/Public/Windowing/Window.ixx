// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.h"
#include <cstdint>
#include <string>

export module RenderCore.Window;

import RenderCore.Renderer;
import RenderCore.Window.Flags;
import RenderCore.Window.GLFWHandler;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window
    {
        GLFWHandler m_GLFWHandler {};
        Renderer m_Renderer {};

        std::string m_Title{};
        std::uint16_t m_Width{};
        std::uint16_t m_Height{};
        InitializationFlags m_Flags{};

    public:
        Window() = default;

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        bool Initialize(std::uint16_t, std::uint16_t, std::string, InitializationFlags Flags = InitializationFlags::NONE);
        void Shutdown();

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

        [[nodiscard]] Renderer& GetRenderer();

        virtual void PollEvents();

    protected:
        virtual void CreateOverlay();

    private:
        void RequestRender();
    };
}// namespace RenderCore