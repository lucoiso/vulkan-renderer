// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <benchmark/benchmark.h>

export module Benchmark.RenderCore;

import RenderCore.Tests.SharedUtils;

import RenderCore.Window;
import RenderCore.EngineCore;

static void InitializeWindow(benchmark::State& State)
{
    for ([[maybe_unused]] auto const _: State)
    {
        if (RenderCore::Window Window; Window.Initialize(600U, 600U, "Vulkan Renderer", true).Get())
        {
            Window.Shutdown().Get();
            benchmark::DoNotOptimize(Window);
        }
    }
}

BENCHMARK(InitializeWindow);

static void LoadAndUnloadScene(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    std::string ObjectPath {"Resources/Assets/Box/glTF/Box.gltf"};
    benchmark::DoNotOptimize(ObjectPath);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto LoadedIDs = RenderCore::EngineCore::Get().LoadScene(ObjectPath);
        benchmark::DoNotOptimize(LoadedIDs);

        RenderCore::EngineCore::Get().UnloadAllScenes();
    }
}

BENCHMARK(LoadAndUnloadScene);