// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/Logger.h>

#ifdef _DEBUG
#include <spirv-tools/libspirv.hpp>
#endif

module RenderCore.Runtime.ShaderCompiler;

using namespace RenderCore;

bool CompileInternal(ShaderType const            ShaderType,
                     strzilla::string_view const Source,
                     EShLanguage const           Language,
                     strzilla::string_view const EntryPoint,
                     std::int32_t const          Version,
                     std::vector<std::uint32_t> &OutSPIRVCode)
{
    glslang::InitializeProcess();

    glslang::TShader Shader(Language);

    char const *ShaderContent = std::data(Source);
    Shader.setStringsWithLengths(&ShaderContent, nullptr, 1);

    Shader.setEntryPoint(std::data(EntryPoint));
    Shader.setSourceEntryPoint(std::data(EntryPoint));
    Shader.setEnvInput(ShaderType == ShaderType::GLSL ? glslang::EShSourceGlsl : glslang::EShSourceHlsl, Language, glslang::EShClientVulkan, 1);
    Shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    Shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    TBuiltInResource const *Resources    = GetDefaultResources();
    constexpr auto          MessageFlags = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!Shader.parse(Resources, Version, ECoreProfile, false, true, MessageFlags))
    {
        glslang::FinalizeProcess();

        auto const InfoLog(strzilla::string { "Info Log: " } + Shader.getInfoLog());
        auto const DebugLog(strzilla::string { "Debug Log: " } + Shader.getInfoDebugLog());

        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: " << std::format("Failed to parse shader:\n{}\n{}",
                                                                            std::data(InfoLog),
                                                                            std::data(DebugLog));
        return false;
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(MessageFlags))
    {
        glslang::FinalizeProcess();

        auto const InfoLog(strzilla::string { "Info Log: " } + Shader.getInfoLog());
        auto const DebugLog(strzilla::string { "Debug Log: " } + Shader.getInfoDebugLog());

        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: " << std::format("Failed to parse shader:\n{}\n{}",
                                                                            std::data(InfoLog),
                                                                            std::data(DebugLog));
        return false;
    }

    #ifdef _DEBUG
    // Uncomment to print the shader code
    // BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Compiling shader:\n" << Source;
    #endif

    spv::SpvBuildLogger Logger;
    GlslangToSpv(*Program.getIntermediate(Language), OutSPIRVCode, &Logger);
    glslang::FinalizeProcess();

    if (strzilla::string const GeneratedLogs = Logger.getAllMessages();
        !std::empty(GeneratedLogs))
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Shader compilation result log:\n" << GeneratedLogs;
    }

    return !std::empty(OutSPIRVCode);
}

#ifdef _DEBUG
bool ValidateSPIRV(std::vector<std::uint32_t> const &SPIRVData)
{
    static spvtools::SpirvTools SPIRVToolsInstance(SPV_ENV_VULKAN_1_3);

    if (!SPIRVToolsInstance.IsValid())
    {
        return false;
    }

    static bool Initialized = false;

    if (!Initialized)
    {
        SPIRVToolsInstance.SetMessageConsumer([_func_internal_ = __func__](spv_message_level_t const              Level,
                                                                           [[maybe_unused]] char const *          Source,
                                                                           [[maybe_unused]] spv_position_t const &Position,
                                                                           char const *                           Message)
        {
            switch (Level)
            {
                case SPV_MSG_FATAL:
                case SPV_MSG_INTERNAL_ERROR:
                case SPV_MSG_ERROR: BOOST_LOG_TRIVIAL(error) << "[" << _func_internal_ << "]: " << std::format("Error: {}\n", Message);
                    break;

                case SPV_MSG_WARNING: BOOST_LOG_TRIVIAL(warning) << "[" << _func_internal_ << "]: " << std::format("Warning: {}\n", Message);
                    break;

                case SPV_MSG_INFO: BOOST_LOG_TRIVIAL(info) << "[" << _func_internal_ << "]: " << std::format("Info: {}\n", Message);
                    break;

                default:
                    break;
            }
        });

        Initialized = true;
    }

    // Uncomment to print SPIR-V disassembly
    // if (strzilla::string DisassemblySPIRVData; SPIRVToolsInstance.Disassemble(SPIRVData, &DisassemblySPIRVData))
    // {
    //     BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Generated SPIR-V shader disassembly:\n" << DisassemblySPIRVData;
    // }

    return SPIRVToolsInstance.Validate(SPIRVData);
}
#endif

bool RenderCore::Compile(strzilla::string_view const Source,
                         ShaderType const            ShaderType,
                         strzilla::string_view const EntryPoint,
                         std::int32_t const          Version,
                         EShLanguage const           Language,
                         std::vector<std::uint32_t> &OutSPIRVCode)
{
    std::filesystem::path const Path { std::data(Source) };
    std::stringstream           ShaderSource;
    std::ifstream               File { Path };

    if (!File.is_open())
    {
        return false;
    }

    ShaderSource << File.rdbuf();
    File.close();

    bool const Result = CompileInternal(ShaderType, ShaderSource.str(), Language, EntryPoint, Version, OutSPIRVCode);

    if (Result)
    {
        #ifdef _DEBUG
        if (!ValidateSPIRV(OutSPIRVCode))
        {
            return false;
        }
        #endif

        strzilla::string const SPIRVPath = std::format("{}_{}.spv", std::data(Source), static_cast<std::uint8_t>(Language));
        std::ofstream          SPIRVFile(SPIRVPath, std::ios::binary);
        if (!SPIRVFile.is_open())
        {
            return false;
        }

        SPIRVFile << strzilla::string(reinterpret_cast<char const *>(std::data(OutSPIRVCode)), std::size(OutSPIRVCode) * sizeof(std::uint32_t));
        SPIRVFile.close();
    }

    return Result;
}

bool RenderCore::Load(strzilla::string_view const Source, std::vector<std::uint32_t> &OutSPIRVCode)
{
    std::filesystem::path const Path { std::data(Source) };
    if (!exists(Path))
    {
        return false;
    }

    std::ifstream File(Path, std::ios::ate | std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }

    std::size_t const FileSize = File.tellg();
    if (FileSize == 0)
    {
        return false;
    }

    OutSPIRVCode.resize(FileSize / sizeof(std::uint32_t), std::uint32_t());

    File.seekg(0);
    std::istream const &ReadResult = File.read(reinterpret_cast<char *>(std::data(OutSPIRVCode)), static_cast<std::streamsize>(FileSize));
    File.close();

    return !ReadResult.fail();
}

bool RenderCore::CompileOrLoadIfExists(strzilla::string_view const Source,
                                       ShaderType const            ShaderType,
                                       strzilla::string_view const EntryPoint,
                                       std::int32_t const          Version,
                                       EShLanguage const           Language,
                                       std::vector<std::uint32_t> &OutSPIRVCode)
{
    if (strzilla::string const CompiledShaderPath = std::format("{}_{}.spv", std::data(Source), static_cast<std::uint8_t>(Language));
        std::filesystem::exists(std::data(CompiledShaderPath)))
    {
        return Load(CompiledShaderPath, OutSPIRVCode);
    }

    return Compile(Source, ShaderType, EntryPoint, Version, Language, OutSPIRVCode);
}

void RenderCore::CompileDefaultShaders()
{
    constexpr auto GlslVersion = 450;
    constexpr auto EntryPoint  = "main";

    auto const CompileAndStage = [EntryPoint, GlslVersion](strzilla::string_view const Shader, EShLanguage const Language)
    {
        if (auto &[StageInfo, ShaderCode] = g_StageInfos.emplace_back();
            CompileOrLoadIfExists(Shader, ShaderType::GLSL, EntryPoint, GlslVersion, Language, ShaderCode))
        {
            StageInfo = VkPipelineShaderStageCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = Language == EShLangVertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pName = EntryPoint
            };
        }
    };

    // constexpr auto TaskLang { EShLangTask };
    // constexpr auto TaskShader { DEFAULT_TASK_SHADER };
    // CompileAndStage(TaskShader, TaskLang);

    // constexpr auto MeshLang { EShLangMesh };
    // constexpr auto MeshShader { DEFAULT_MESH_SHADER };
    // CompileAndStage(MeshShader, MeshLang);

    constexpr auto VertexLang { EShLangVertex };
    constexpr auto VertexShader { DEFAULT_VERTEX_SHADER };
    CompileAndStage(VertexShader, VertexLang);

    constexpr auto FragmentLang { EShLangFragment };
    constexpr auto FragmentShader { DEFAULT_FRAGMENT_SHADER };
    CompileAndStage(FragmentShader, FragmentLang);
}
