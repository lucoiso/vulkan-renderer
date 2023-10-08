// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#pragma once

#include <RenderCoreModule.h>

export module RenderCore.Window;

import <thread>;
import <queue>;
import <string_view>;
import <stdexcept>;
import <unordered_map>;
import <functional>;

namespace RenderCore
{
    enum class ApplicationEventFlags : std::uint8_t
    {
        DRAW_FRAME,
        LOAD_SCENE,
        UNLOAD_SCENE,
        MAX
    };

    export class RENDERCOREMODULE_API Window
    {
        std::uint32_t m_DrawTimerID {0U};
        std::queue<std::uint8_t> m_EventIDQueue;
        std::thread::id m_MainThreadID;

    public:
        Window();

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        bool Initialize(std::uint16_t Width, std::uint16_t Height, std::string_view Title);
        void Shutdown();

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

        void PollEvents();

        virtual void CreateOverlay();

    private:
        void RegisterTimers();
    };
}// namespace RenderCore