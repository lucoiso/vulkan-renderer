// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.hpp>
#include <volk.h>

module RenderCore.Management.ShaderManagement;

import <filesystem>;
import <fstream>;
import <ranges>;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import Timer.ExecutionCounter;

using namespace RenderCore;

constexpr char const* g_EntryPoint   = "main";
constexpr std::int32_t g_GlslVersion = 450;

std::unordered_map<VkShaderModule, VkPipelineShaderStageCreateInfo> g_StageInfos {};

bool Compile(std::string_view const& Source, EShLanguage const Language, std::vector<std::uint32_t>& OutSPIRVCode)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    char const* ShaderContent = std::data(Source);
    Shader.setStringsWithLengths(&ShaderContent, nullptr, 1);

    Shader.setEntryPoint(g_EntryPoint);
    Shader.setSourceEntryPoint(g_EntryPoint);
    Shader.setEnvInput(glslang::EShSourceGlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    TBuiltInResource const* Resources = GetDefaultResources();
    constexpr auto MessageFlags       = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!Shader.parse(Resources, g_GlslVersion, ECoreProfile, false, true, MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Shader.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Shader.getInfoDebugLog()));
        std::string const ErrMessage = std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        throw std::runtime_error(ErrMessage);
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();
        std::string const InfoLog("Info Log: " + std::string(Program.getInfoLog()));
        std::string const DebugLog("Debug Log: " + std::string(Program.getInfoDebugLog()));
        std::string const ErrMessage = std::format("Failed to parse shader:\n{}\n{}", InfoLog, DebugLog);
        throw std::runtime_error(ErrMessage);
    }

    if constexpr (g_EnableCustomDebug)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compiling shader:\n"
                                 << Source;
    }

    spv::SpvBuildLogger Logger;
    GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    if (std::string const GeneratedLogs = Logger.getAllMessages(); !std::empty(GeneratedLogs))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shader compilation result log:\n"
                                 << GeneratedLogs;
    }

    return !std::empty(OutSPIRVCode);
}

void StageInfo(VkShaderModule const& Module, EShLanguage const Language)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Module == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid shader module");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Staging shader info...";

    VkPipelineShaderStageCreateInfo StageInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = Module,
            .pName  = g_EntryPoint};

    switch (Language)
    {
        case EShLangVertex:
            StageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;

        case EShLangFragment:
            StageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;

        case EShLangCompute:
            StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            break;

        case EShLangGeometry:
            StageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            break;

        case EShLangTessControl:
            StageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            break;

        case EShLangTessEvaluation:
            StageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            break;

        case EShLangRayGen:
            StageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            break;

        case EShLangIntersect:
            StageInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            break;

        case EShLangAnyHit:
            StageInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            break;

        case EShLangClosestHit:
            StageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            break;

        case EShLangMiss:
            StageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            break;

        case EShLangCallable:
            StageInfo.stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            break;

        default:
            throw std::runtime_error("Unsupported shader language");
    }

    g_StageInfos.emplace(Module, StageInfo);
}

bool ValidateSPIRV(const std::vector<std::uint32_t>& SPIRVData)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    static spvtools::SpirvTools SPIRVToolsInstance(SPV_ENV_VULKAN_1_3);
    if (!SPIRVToolsInstance.IsValid())
    {
        throw std::runtime_error("Failed to initialize SPIRV-Tools");
    }

    if constexpr (g_EnableCustomDebug)
    {
        if (std::string DisassemblySPIRVData;
            SPIRVToolsInstance.Disassemble(SPIRVData, &DisassemblySPIRVData))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Generated SPIR-V shader disassembly:\n"
                                     << DisassemblySPIRVData;
        }
    }

    return SPIRVToolsInstance.Validate(SPIRVData);
}

bool RenderCore::Compile(std::string_view const& Source, std::vector<std::uint32_t>& OutSPIRVCode)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    EShLanguage Language = EShLangVertex;
    std::filesystem::path const Path(Source);

    if (std::filesystem::path const Extension = Path.extension();
        Extension == ".frag")
    {
        Language = EShLangFragment;
    }
    else if (Extension == ".comp")
    {
        Language = EShLangCompute;
    }
    else if (Extension == ".geom")
    {
        Language = EShLangGeometry;
    }
    else if (Extension == ".tesc")
    {
        Language = EShLangTessControl;
    }
    else if (Extension == ".tese")
    {
        Language = EShLangTessEvaluation;
    }
    else if (Extension == ".rgen")
    {
        Language = EShLangRayGen;
    }
    else if (Extension == ".rint")
    {
        Language = EShLangIntersect;
    }
    else if (Extension == ".rahit")
    {
        Language = EShLangAnyHit;
    }
    else if (Extension == ".rchit")
    {
        Language = EShLangClosestHit;
    }
    else if (Extension == ".rmiss")
    {
        Language = EShLangMiss;
    }
    else if (Extension == ".rcall")
    {
        Language = EShLangCallable;
    }
    else if (Extension != ".vert")
    {
        throw std::runtime_error("Unknown shader extension: " + Extension.string());
    }

    std::stringstream ShaderSource;
    std::ifstream File(Path);
    if (!File.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + Path.string());
    }

    ShaderSource << File.rdbuf();
    File.close();

    bool const Result = Compile(ShaderSource.str(), Language, OutSPIRVCode);
    if (Result)
    {
        if (!ValidateSPIRV(OutSPIRVCode))
        {
            throw std::runtime_error("Failed to validate SPIR-V code");
        }

        std::string const SPIRVPath = std::format("{}.spv", Source);
        std::ofstream SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            throw std::runtime_error("Failed to open SPIRV file: " + SPIRVPath);
        }

        SPIRVFile << std::string(reinterpret_cast<char const*>(std::data(OutSPIRVCode)), std::size(OutSPIRVCode) * sizeof(std::uint32_t));
        SPIRVFile.close();

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shader compiled, generated SPIR-V shader file: " << SPIRVPath;
    }

    return Result;
}

bool RenderCore::Load(std::string_view const& Source, std::vector<std::uint32_t>& OutSPIRVCode)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::filesystem::path const Path(Source);
    if (!exists(Path))
    {
        std::string const ErrMessage = "Shader file does not exist" + std::string(Source);
        throw std::runtime_error(ErrMessage);
    }

    std::ifstream File(Path, std::ios::ate | std::ios::binary);
    if (!File.is_open())
    {
        std::string const ErrMessage = "Failed to open shader file" + std::string(Source);
        throw std::runtime_error(ErrMessage);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading shader: " << Source;

    std::size_t const FileSize = File.tellg();
    if (FileSize == 0)
    {
        throw std::runtime_error("Shader file is empty");
    }

    OutSPIRVCode.resize(FileSize / sizeof(std::uint32_t), std::uint32_t());

    File.seekg(0);
    std::istream const& ReadResult = File.read(reinterpret_cast<char*>(std::data(OutSPIRVCode)), static_cast<std::streamsize>(FileSize)); /* Flawfinder: ignore */
    File.close();

    return !ReadResult.fail();
}

bool RenderCore::CompileOrLoadIfExists(std::string_view const& Source, std::vector<uint32_t>& OutSPIRVCode)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (std::string const CompiledShaderPath = std::format("{}.spv", Source);
        std::filesystem::exists(CompiledShaderPath))
    {
        return Load(CompiledShaderPath, OutSPIRVCode);
    }
    return Compile(Source, OutSPIRVCode);
}

VkShaderModule RenderCore::CreateModule(VkDevice const& Device, std::vector<std::uint32_t> const& SPIRVCode, EShLanguage const Language)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid vulkan logical device");
    }

    if (std::empty(SPIRVCode))
    {
        throw std::runtime_error("Invalid SPIRV code");
    }

    if (!ValidateSPIRV(SPIRVCode))
    {
        throw std::runtime_error("Failed to validate SPIR-V code");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating shader module...";

    VkShaderModuleCreateInfo const CreateInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = std::size(SPIRVCode) * sizeof(std::uint32_t),
            .pCode    = std::data(SPIRVCode)};

    VkShaderModule Output = nullptr;
    CheckVulkanResult(vkCreateShaderModule(Device, &CreateInfo, nullptr, &Output));

    StageInfo(Output, Language);

    return Output;
}

VkPipelineShaderStageCreateInfo RenderCore::GetStageInfo(VkShaderModule const& Module)
{
    return g_StageInfos.at(Module);
}

std::vector<VkShaderModule> RenderCore::GetShaderModules()
{
    std::vector<VkShaderModule> Output;
    for (auto const& ShaderModule: g_StageInfos | std::views::keys)
    {
        Output.push_back(ShaderModule);
    }

    return Output;
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::GetStageInfos()
{
    std::vector<VkPipelineShaderStageCreateInfo> Output;
    for (auto const& StageInfo: g_StageInfos | std::views::values)
    {
        Output.push_back(StageInfo);
    }

    return Output;
}

void RenderCore::ReleaseShaderResources(VkDevice const& LogicalDevice)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan shader resources";


    for (auto const& ShaderModule: g_StageInfos | std::views::keys)
    {
        if (ShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(LogicalDevice, ShaderModule, nullptr);
        }
    }
    g_StageInfos.clear();
}

void RenderCore::FreeStagedModules(VkDevice const& LogicalDevice, std::vector<VkPipelineShaderStageCreateInfo> const& StagedModules)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Freeing staged shader modules";


    for (VkPipelineShaderStageCreateInfo const& StageInfoIter: StagedModules)
    {
        if (StageInfoIter.module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(LogicalDevice, StageInfoIter.module, nullptr);
        }

        if (g_StageInfos.contains(StageInfoIter.module))
        {
            g_StageInfos.erase(StageInfoIter.module);
        }
    }
}

std::vector<VkPipelineShaderStageCreateInfo> RenderCore::CompileDefaultShaders(VkDevice const& LogicalDevice)
{
    constexpr std::array FragmentShaders {
            DEFAULT_SHADER_FRAG};
    constexpr std::array VertexShaders {
            DEFAULT_SHADER_VERT};

    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    for (char const* const& FragmentShaderIter: FragmentShaders)
    {
        if (std::vector<std::uint32_t> FragmentShaderCode;
            CompileOrLoadIfExists(FragmentShaderIter, FragmentShaderCode))
        {
            auto const FragmentModule = CreateModule(LogicalDevice, FragmentShaderCode, EShLangFragment);
            ShaderStages.push_back(GetStageInfo(FragmentModule));
        }
    }

    for (char const* const& VertexShaderIter: VertexShaders)
    {
        if (std::vector<std::uint32_t> VertexShaderCode;
            CompileOrLoadIfExists(VertexShaderIter, VertexShaderCode))
        {
            auto const VertexModule = CreateModule(LogicalDevice, VertexShaderCode, EShLangVertex);
            ShaderStages.push_back(GetStageInfo(VertexModule));
        }
    }

    return ShaderStages;
}