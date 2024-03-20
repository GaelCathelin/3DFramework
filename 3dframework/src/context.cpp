#include <context.h>
#include <framebuffer.h>
#include <nvrhi/utils.h>
#ifndef NDEBUG
#include <nvrhi/validation.h>
#endif
#include <nvrhi/vulkan.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap/VkBootstrap.h>
#include <vulkan/vulkan.hpp>
#include "private_log.h"
#include "private_impl.h"
#include "sdlwindow.h"

#define FRAME_BUFFERING 3

static struct {
    SDL_Window *window;
    vkb::Instance instance;
    vkb::PhysicalDevice physicalDevice;
    vkb::Device device;
    vk::Device vkDevice;
    vk::Queue vkGraphicsQueue;
    nvrhi::vulkan::DeviceHandle nvrhiVkDevice;
    nvrhi::DeviceHandle nvrhiDevice;
    vkb::Swapchain swapchain;
    vk::SwapchainKHR vkSwapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<Texture> swapchainTextures;
    Texture depthTexture;
    std::vector<Framebuffer> framebuffers;
    nvrhi::CommandListHandle commandList;
    std::vector<vk::Semaphore> acquireSemaphores, presentSemaphores;
    std::vector<nvrhi::EventQueryHandle> throttleCommands;
    uint currentFrame, imageIndex, width, height;
    ColorSpace colorSpace;
    nvrhi::TimerQueryHandle timerQuery;
    bool raytracing;
} context = {};

#ifndef NDEBUG
static VkBool32 loggerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *data, void*) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: logError(data->pMessage); putchar('\n'); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? logPerfWarning(data->pMessage) : logWarning(data->pMessage); putchar('\n'); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: FALLTHROUGH;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: logInfo(data->pMessage); putchar('\n'); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
    }
    return false;
}

static class MessageCallback : public nvrhi::IMessageCallback {
public:
    void message(nvrhi::MessageSeverity severity, const char *messageText) override {
        switch (severity) {
            case nvrhi::MessageSeverity::Fatal  : FALLTHROUGH;
            case nvrhi::MessageSeverity::Error  : logError  (messageText); putchar('\n'); break;
            case nvrhi::MessageSeverity::Warning: logWarning(messageText); putchar('\n'); break;
            case nvrhi::MessageSeverity::Info   : logInfo   (messageText); putchar('\n'); break;
        }
    }
} g_MyMessageCallback;
#endif

static vkb::Instance createInstance(const char** instanceExtensions, const size_t nbInstanceExtensions) {
    vkb::InstanceBuilder instance_builder;

    for (uint32_t i = 0; i < nbInstanceExtensions; i++)
        instance_builder.enable_extension(instanceExtensions[i]);

    vkb::Result<vkb::Instance> instance_builder_return = instance_builder
    #ifndef NDEBUG
        .request_validation_layers()
        .set_debug_callback(loggerCallback)
    #endif
        .require_api_version(1, 3, 249)
        .set_minimum_instance_version(1, 3, 249)
        .build();

    if (!instance_builder_return) {
        logError("Failed to create a Vulkan instance. Error: %s", instance_builder_return.error().message().c_str());
        exit(-1);
    }

    return instance_builder_return.value();
}

static vkb::PhysicalDevice selectPhysicalDevice(const vkb::Instance &vkb_instance, SDL_Window *window, const char **deviceExtensions, const size_t nbDeviceExtensions) {
    VkSurfaceKHR surface_handle;
    SDL_Vulkan_CreateSurface(window, vkb_instance.instance, &surface_handle);

    vkb::PhysicalDeviceSelector physical_device_selector(vkb_instance, surface_handle);
    for (size_t i = 0; i < nbDeviceExtensions; i++)
        physical_device_selector.add_required_extension(deviceExtensions[i]);
    vkb::Result<std::vector<vkb::PhysicalDevice>> physical_device_selector_return = physical_device_selector.select_devices();

    if (!physical_device_selector_return) {
        logError("Failed to select a physical device. Error: %s", physical_device_selector_return.error().message().c_str());
        exit(-1);
    }

    vkb::PhysicalDevice vkb_physical_device = {};
    for (const vkb::PhysicalDevice &pd : physical_device_selector_return.value()) {
//        logInfo(pd.name.c_str());
//        if (!strncmp(pd.name.c_str(), "AMD", 3)) {
        if (!strncmp(pd.name.c_str(), "NVIDIA", 6)) {
            vkb_physical_device = pd;
            break;
        }
    }
    if (vkb_physical_device.physical_device == VK_NULL_HANDLE)
        vkb_physical_device = physical_device_selector_return.value().front();

    return vkb_physical_device;
}

static vkb::Device createDevice(const vkb::PhysicalDevice &vkb_physical_device, const bool gcn, const bool rdna, const bool maxwell, const bool nvPascal, const bool raytracing, const bool turing) {
    std::vector<vkb::CustomQueueDescription> queue_descriptions;
    std::vector<VkQueueFamilyProperties> queue_families = vkb_physical_device.get_queue_families();
    for (size_t i = 0; i < queue_families.size(); i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_descriptions.push_back(vkb::CustomQueueDescription(i, std::vector<float>(1, 1.0f)));
            break;
        }
    }

    vkb::DeviceBuilder device_builder(vkb_physical_device);

    VkPhysicalDeviceFeatures2 vulkan10Features {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    device_builder.add_pNext(&vulkan10Features);
    vulkan10Features.features.robustBufferAccess = false;
    vulkan10Features.features.fullDrawIndexUint32 = true;
    vulkan10Features.features.imageCubeArray = true;
    vulkan10Features.features.independentBlend = true;
    vulkan10Features.features.geometryShader = true;
    vulkan10Features.features.tessellationShader = true;
    vulkan10Features.features.sampleRateShading = true;
    vulkan10Features.features.dualSrcBlend = true;
    vulkan10Features.features.logicOp = true;
    vulkan10Features.features.multiDrawIndirect = true;
    vulkan10Features.features.drawIndirectFirstInstance = true;
    vulkan10Features.features.depthClamp = true;
    vulkan10Features.features.depthBiasClamp = true;
    vulkan10Features.features.fillModeNonSolid = true;
    vulkan10Features.features.depthBounds = true;
    vulkan10Features.features.wideLines = true;
    vulkan10Features.features.largePoints = true;
    vulkan10Features.features.alphaToOne = false;
    vulkan10Features.features.multiViewport = true;
    vulkan10Features.features.samplerAnisotropy = true;
    vulkan10Features.features.textureCompressionETC2 = false;
    vulkan10Features.features.textureCompressionASTC_LDR = false;
    vulkan10Features.features.textureCompressionBC = true;
    vulkan10Features.features.occlusionQueryPrecise = true;
    vulkan10Features.features.pipelineStatisticsQuery = true;
    vulkan10Features.features.vertexPipelineStoresAndAtomics = true;
    vulkan10Features.features.fragmentStoresAndAtomics = true;
    vulkan10Features.features.shaderTessellationAndGeometryPointSize = true;
    vulkan10Features.features.shaderImageGatherExtended = true;
    vulkan10Features.features.shaderStorageImageExtendedFormats = true;
    vulkan10Features.features.shaderStorageImageMultisample = true;
    vulkan10Features.features.shaderStorageImageReadWithoutFormat = true;
    vulkan10Features.features.shaderStorageImageWriteWithoutFormat = true;
    vulkan10Features.features.shaderUniformBufferArrayDynamicIndexing = true;
    vulkan10Features.features.shaderSampledImageArrayDynamicIndexing = true;
    vulkan10Features.features.shaderStorageBufferArrayDynamicIndexing = true;
    vulkan10Features.features.shaderStorageImageArrayDynamicIndexing = true;
    vulkan10Features.features.shaderClipDistance = true;
    vulkan10Features.features.shaderCullDistance = true;
    vulkan10Features.features.shaderFloat64 = true;
    vulkan10Features.features.shaderInt64 = true;
    vulkan10Features.features.shaderInt16 = false;
    vulkan10Features.features.shaderResourceResidency = true;
    vulkan10Features.features.shaderResourceMinLod = true;
    vulkan10Features.features.sparseBinding = true;
    vulkan10Features.features.sparseResidencyBuffer = true;
    vulkan10Features.features.sparseResidencyImage2D = true;
    vulkan10Features.features.sparseResidencyImage3D = true;
    vulkan10Features.features.sparseResidency2Samples = false;
    vulkan10Features.features.sparseResidency4Samples = false;
    vulkan10Features.features.sparseResidency8Samples = false;
    vulkan10Features.features.sparseResidency16Samples = false;
    vulkan10Features.features.sparseResidencyAliased = true;
    vulkan10Features.features.variableMultisampleRate = true;
    vulkan10Features.features.inheritedQueries = true;

    VkPhysicalDeviceVulkan11Features vulkan11Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    device_builder.add_pNext(&vulkan11Features);
    vulkan11Features.storageBuffer16BitAccess = true;
    vulkan11Features.uniformAndStorageBuffer16BitAccess = true;

    VkPhysicalDeviceVulkan12Features vulkan12Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    device_builder.add_pNext(&vulkan12Features);
    vulkan12Features.bufferDeviceAddress = true;
    vulkan12Features.descriptorBindingPartiallyBound = true;
    vulkan12Features.drawIndirectCount = true;
    vulkan12Features.imagelessFramebuffer = true;
    vulkan12Features.samplerFilterMinmax = true;
    vulkan12Features.samplerMirrorClampToEdge = true;
    vulkan12Features.scalarBlockLayout = true;
    vulkan12Features.shaderBufferInt64Atomics = true;
    vulkan12Features.shaderInt8 = true;
    vulkan12Features.shaderOutputLayer = true;
    vulkan12Features.shaderOutputViewportIndex = true;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = true;
    vulkan12Features.shaderStorageBufferArrayNonUniformIndexing = true;
    vulkan12Features.shaderStorageImageArrayNonUniformIndexing = true;
    vulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing = true;
    vulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing = true;
    vulkan12Features.shaderSubgroupExtendedTypes = true;
    vulkan12Features.shaderUniformBufferArrayNonUniformIndexing = true;
    vulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing = true;
    vulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing = true;
    vulkan12Features.storageBuffer8BitAccess = true;
    vulkan12Features.subgroupBroadcastDynamicId = true;
    vulkan12Features.timelineSemaphore = true;
    vulkan12Features.uniformAndStorageBuffer8BitAccess = true;
    vulkan12Features.uniformBufferStandardLayout = true;
#if 1
    VkPhysicalDeviceVulkan13Features vulkan13Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    device_builder.add_pNext(&vulkan13Features);
    vulkan13Features.maintenance4 = true;
    vulkan13Features.shaderIntegerDotProduct = true;
    vulkan13Features.synchronization2 = true;
#endif
    VkPhysicalDeviceLineRasterizationFeaturesEXT featuresFatLines = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT};
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT featuresShaderAtomicFloat = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR featuresAS = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    VkPhysicalDeviceRayQueryFeaturesKHR featuresRayQuery = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR featuresPositionFetch = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR};
    VkPhysicalDeviceMeshShaderFeaturesEXT featuresMeshShader = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR featuresVRS = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
    VkPhysicalDeviceShaderSMBuiltinsFeaturesNV featuresSMBuiltins = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV};
    VkPhysicalDeviceComputeShaderDerivativesFeaturesNV featuresComputeDerivatives = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV};

    if (gcn) {
        device_builder.add_pNext(&featuresShaderAtomicFloat);
        featuresShaderAtomicFloat.shaderBufferFloat32Atomics = true;
        featuresShaderAtomicFloat.shaderImageFloat32Atomics = true;
        featuresShaderAtomicFloat.shaderSharedFloat32Atomics = true;
        featuresShaderAtomicFloat.sparseImageFloat32Atomics = true;

        if (rdna) {
            vulkan10Features.features.shaderInt16 = true;
            vulkan12Features.shaderFloat16 = true;
        }
    }

    if (maxwell) {
        device_builder.add_pNext(&featuresShaderAtomicFloat);
        featuresShaderAtomicFloat.shaderBufferFloat32AtomicAdd = true;
        featuresShaderAtomicFloat.shaderImageFloat32AtomicAdd = true;
        featuresShaderAtomicFloat.shaderSharedFloat32AtomicAdd = true;
        featuresShaderAtomicFloat.sparseImageFloat32AtomicAdd = true;
        featuresShaderAtomicFloat.shaderBufferFloat32Atomics = true;
        featuresShaderAtomicFloat.shaderImageFloat32Atomics = true;
        featuresShaderAtomicFloat.shaderSharedFloat32Atomics = true;
        featuresShaderAtomicFloat.sparseImageFloat32Atomics = true;

        device_builder.add_pNext(&featuresFatLines);
        featuresFatLines.smoothLines = true;
        featuresFatLines.rectangularLines = true;

        device_builder.add_pNext(&featuresSMBuiltins);
        featuresSMBuiltins.shaderSMBuiltins = true;

        if (nvPascal) {
            vulkan10Features.features.shaderInt16 = true;
        }
    }

    if (raytracing) {
        vulkan10Features.features.shaderInt16 = true;
        vulkan12Features.shaderFloat16 = true;
        vulkan12Features.shaderInputAttachmentArrayDynamicIndexing = true;
        vulkan12Features.shaderInputAttachmentArrayNonUniformIndexing = true;

        device_builder.add_pNext(&featuresAS);
        featuresAS.accelerationStructure = true;
        featuresAS.accelerationStructureCaptureReplay = true;
        featuresAS.descriptorBindingAccelerationStructureUpdateAfterBind = true;

        device_builder.add_pNext(&featuresRayQuery);
        featuresRayQuery.rayQuery = true;

        device_builder.add_pNext(&featuresPositionFetch);
        featuresPositionFetch.rayTracingPositionFetch = true;

        device_builder.add_pNext(&featuresMeshShader);
        featuresMeshShader.meshShader = true;
        featuresMeshShader.primitiveFragmentShadingRateMeshShader = true;
        featuresMeshShader.primitiveFragmentShadingRateMeshShader = true;

        if (turing) {
            vulkan12Features.bufferDeviceAddressCaptureReplay = true;

            featuresShaderAtomicFloat.shaderBufferFloat64AtomicAdd = true;
            featuresShaderAtomicFloat.shaderBufferFloat64Atomics = true;
            featuresShaderAtomicFloat.shaderSharedFloat64AtomicAdd = true;
            featuresShaderAtomicFloat.shaderSharedFloat64Atomics = true;
            featuresMeshShader.meshShaderQueries = true;
            featuresMeshShader.taskShader = true;

            device_builder.add_pNext(&featuresComputeDerivatives);
            featuresComputeDerivatives.computeDerivativeGroupLinear = true;
            featuresComputeDerivatives.computeDerivativeGroupQuads = true;

            device_builder.add_pNext(&featuresVRS);
            featuresVRS.attachmentFragmentShadingRate = true;
            featuresVRS.pipelineFragmentShadingRate = true;
            featuresVRS.primitiveFragmentShadingRate = true;
        }
    }

    vkb::Result<vkb::Device> device_return = device_builder.custom_queue_setup(queue_descriptions).build();

    if (!device_return) {
        logError("Failed to create a device. Error: %s", device_return.error().message().c_str());
        exit(-1);
    }

    return device_return.value();
}

static vkb::Swapchain createSwapchain(const vkb::Device &vkb_device, const uint32_t frameBuffering, const bool srgb, const bool hdr10, const bool hdr16, const bool vsync) {
    vkb::SwapchainBuilder swapchain_builder(vkb_device);

    if (hdr16) {
        swapchain_builder
            .set_desired_format({VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT})
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
    } else if (hdr10) {
        swapchain_builder
            .set_desired_format({VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT})
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
    } else if (srgb) {
        swapchain_builder
            .set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    } else {
        swapchain_builder
            .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
    }

    vkb::Result<vkb::Swapchain> swapchain_builder_return = swapchain_builder
        .set_desired_present_mode(vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
        .set_required_min_image_count(frameBuffering)
        .build();

    if (!swapchain_builder_return) {
        logError("Failed to create a swap chain. Error: %s", swapchain_builder_return.error().message().c_str());
        exit(-1);
    }

    return swapchain_builder_return.value();
}

extern "C" {

void initContext(const char *title, const uint width, const uint height, const ulong flags) {
    uint nbDeviceExtensions = 0;
    const char *deviceExtensions[16];
    deviceExtensions[nbDeviceExtensions++] = "VK_KHR_swapchain";

    const bool nvTuring = (flags & NV_TURING_FLAG) != 0, nvPascal = nvTuring || (flags & NV_PASCAL_FLAG) != 0, nvMaxwell = nvPascal || (flags & NV_MAXWELL_FLAG) != 0;
    const bool rdna = (flags & RDNA_FLAG) != 0, gcn = rdna || (flags & GCN_FLAG) != 0;
    context.raytracing = nvTuring || (flags & RAYTRACING_FLAG) != 0;

    if (gcn) {
        deviceExtensions[nbDeviceExtensions++] = "VK_EXT_shader_atomic_float";
        deviceExtensions[nbDeviceExtensions++] = "VK_EXT_sample_locations";
    }

    if (nvMaxwell) {
        deviceExtensions[nbDeviceExtensions++] = "VK_NV_geometry_shader_passthrough";
        deviceExtensions[nbDeviceExtensions++] = "VK_NV_shader_sm_builtins";
        deviceExtensions[nbDeviceExtensions++] = "VK_EXT_shader_atomic_float";
        deviceExtensions[nbDeviceExtensions++] = "VK_EXT_conservative_rasterization";
        deviceExtensions[nbDeviceExtensions++] = "VK_EXT_sample_locations";
    }

    if (context.raytracing) {
        deviceExtensions[nbDeviceExtensions++] = "VK_KHR_deferred_host_operations";
        deviceExtensions[nbDeviceExtensions++] = "VK_KHR_acceleration_structure";
        deviceExtensions[nbDeviceExtensions++] = "VK_KHR_ray_query";
        deviceExtensions[nbDeviceExtensions++] = "VK_KHR_ray_tracing_position_fetch";
        deviceExtensions[nbDeviceExtensions++] = "VK_EXT_mesh_shader";
        if (!nvMaxwell) {
            deviceExtensions[nbDeviceExtensions++] = "VK_EXT_conservative_rasterization";
            deviceExtensions[nbDeviceExtensions++] = "VK_EXT_sample_locations";
        }
        if (nvTuring) {
            deviceExtensions[nbDeviceExtensions++] = "VK_KHR_fragment_shading_rate";
            deviceExtensions[nbDeviceExtensions++] = "VK_NV_compute_shader_derivatives";
        }
    }

    SDL_Init(SDL_INIT_VIDEO);
    context.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_VULKAN | (uint)flags);
    uint nbInstanceExtensions = 0;
    const char *instanceExtensions[16];
    SDL_Vulkan_GetInstanceExtensions(context.window, &nbInstanceExtensions, NULL);
    SDL_Vulkan_GetInstanceExtensions(context.window, &nbInstanceExtensions, instanceExtensions);
#ifndef NDEBUG
    instanceExtensions[nbInstanceExtensions++] = "VK_EXT_debug_utils";
#endif
    instanceExtensions[nbInstanceExtensions++] = "VK_KHR_get_surface_capabilities2";
    instanceExtensions[nbInstanceExtensions++] = "VK_EXT_swapchain_colorspace";

    context.instance = createInstance(instanceExtensions, nbInstanceExtensions);
    context.physicalDevice = selectPhysicalDevice(context.instance, context.window, deviceExtensions, nbDeviceExtensions);
    logInfo("Running on a %s", context.physicalDevice.name.c_str());
    context.device = createDevice(context.physicalDevice, gcn, rdna, nvMaxwell, nvPascal, context.raytracing, nvTuring);
    context.vkDevice = context.device.device;
    context.vkGraphicsQueue = context.device.get_queue(vkb::QueueType::graphics).value();

    // NVRHI device
    nvrhi::vulkan::DeviceDesc deviceDesc = {};
#ifndef NDEBUG
    deviceDesc.errorCB = &g_MyMessageCallback;
#endif
    deviceDesc.instance = context.instance.instance;
    deviceDesc.physicalDevice = context.physicalDevice.physical_device;
    deviceDesc.device = context.device.device;
    deviceDesc.graphicsQueue = context.vkGraphicsQueue;
    deviceDesc.graphicsQueueIndex = context.device.get_queue_index(vkb::QueueType::graphics).value();
    deviceDesc.instanceExtensions = instanceExtensions;
    deviceDesc.numInstanceExtensions = nbInstanceExtensions;
    deviceDesc.deviceExtensions = deviceExtensions;
    deviceDesc.numDeviceExtensions = nbDeviceExtensions;
    deviceDesc.bufferDeviceAddressSupported = true;
    context.nvrhiVkDevice = nvrhi::vulkan::createDevice(deviceDesc);
    context.nvrhiDevice = context.nvrhiVkDevice;
#ifndef NDEBUG
    context.nvrhiDevice = nvrhi::validation::createValidationLayer(context.nvrhiVkDevice);
#endif

    const bool srgb = (flags & SRGB_FLAG) != 0, hdr10 = (flags & HDR10_FLAG) != 0, hdr16 = (flags & HDR16_FLAG) != 0;
    context.colorSpace = (srgb || hdr16) ? ColorSpace_Linear : hdr10 ? ColorSpace_HDR10 : ColorSpace_sRGB;
    context.swapchain = createSwapchain(context.device, FRAME_BUFFERING, srgb, hdr10, hdr16, (flags & VSYNC_FLAG) != 0);
    context.vkSwapchain = context.swapchain.swapchain;
    context.width = context.swapchain.extent.width; context.height = context.swapchain.extent.height;

    if (flags & DEPTH_MASK)
        context.depthTexture = createTexture2D(context.width, context.height, (flags & DEPTH16_FLAG) ? D16 : D32, flags & MSAA_MASK);

    context.swapchainImages = context.swapchain.get_images().value();
    nvrhi::TextureDesc swapchainDesc = nvrhi::TextureDesc()
        .setDimension(nvrhi::TextureDimension::Texture2D)
        .setFormat(hdr16 ? nvrhi::Format::RGBA16_FLOAT : hdr10 ? nvrhi::Format::R10G10B10A2_UNORM : srgb ? nvrhi::Format::SBGRA8_UNORM : nvrhi::Format::BGRA8_UNORM)
        .setWidth(context.width).setHeight(context.height)
        .setIsRenderTarget(true).setIsUAV(true)
        .setInitialState(nvrhi::ResourceStates::Present).setKeepInitialState(true);
    for (const VkImage &image : context.swapchainImages) {
        TextureImpl *tex = new TextureImpl();
        tex->texture = context.nvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, image, swapchainDesc);
        context.swapchainTextures.push_back(Texture{tex});
        context.framebuffers.push_back(createFramebuffer(&context.swapchainTextures.back(), 1, (flags & MSAA_MASK) ? NullTexture : context.depthTexture));
    }

    if (hdr10 || hdr16) {
        int x, y;
        SDL_GetWindowPosition(context.window, &x, &y);
        SDL_SetWindowPosition(context.window, x + 1, y);
        SDL_SetWindowPosition(context.window, x, y);
    }

    context.commandList = context.nvrhiDevice->createCommandList();
    for (size_t i = 0; i < FRAME_BUFFERING; i++) {
        context.acquireSemaphores.push_back(context.vkDevice.createSemaphore(vk::SemaphoreCreateInfo()));
        context.presentSemaphores.push_back(context.vkDevice.createSemaphore(vk::SemaphoreCreateInfo()));
        context.throttleCommands.push_back(context.nvrhiDevice->createEventQuery());
    }

    context.timerQuery = context.nvrhiDevice->createTimerQuery();

    context.currentFrame = 0;
}

void deleteContext() {
    context.nvrhiDevice->waitForIdle();
    context.timerQuery.Reset();
    for (size_t i = 0; i < FRAME_BUFFERING; i++) {
        context.throttleCommands[i].Reset();
        context.vkDevice.destroySemaphore(context.presentSemaphores[i]);
        context.vkDevice.destroySemaphore(context.acquireSemaphores[i]);
        deleteFramebuffer(&context.framebuffers[i]);
        deleteTexture(&context.swapchainTextures[i]);
    }
    deleteTexture(&context.depthTexture);
    context.commandList.Reset();
    vkb::destroy_swapchain(context.swapchain);
    context.nvrhiDevice.Reset();
    context.nvrhiVkDevice.Reset();
    vkb::destroy_device(context.device);
    vkb::destroy_surface(context.instance, context.physicalDevice.surface);
    SDL_DestroyWindow(context.window);
    vkb::destroy_instance(context.instance);
}

void beginFrame() {
    context.nvrhiDevice->resetEventQuery(context.throttleCommands[context.currentFrame]);
    context.nvrhiDevice->setEventQuery(context.throttleCommands[context.currentFrame], nvrhi::CommandQueue::Graphics);
    VERIFY(context.vkDevice.acquireNextImageKHR(context.vkSwapchain, ~0ull, context.acquireSemaphores[context.currentFrame], vk::Fence(), &context.imageIndex) == vk::Result::eSuccess);
    context.nvrhiVkDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, context.acquireSemaphores[context.currentFrame], 0);
    context.nvrhiVkDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, context.presentSemaphores[context.currentFrame], 0);
    context.commandList->open();
}

void endFrame() {
    context.commandList->close();
    context.nvrhiDevice->executeCommandList(context.commandList);
    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR({1}, {&context.presentSemaphores[context.currentFrame]}, {1}, {&context.vkSwapchain}, {&context.imageIndex});
    VERIFY(context.vkGraphicsQueue.presentKHR(&presentInfo) == vk::Result::eSuccess);
    context.nvrhiDevice->waitEventQuery(context.throttleCommands[context.currentFrame]);
    context.nvrhiDevice->runGarbageCollection();
    context.currentFrame = (context.currentFrame + 1) % FRAME_BUFFERING;
}

void waitGPUIdle() {context.nvrhiDevice->waitForIdle();}
void flush() {
    context.commandList->close();
    context.nvrhiDevice->executeCommandList(context.commandList);
    context.commandList->open();
}
void flushAndGarbageCollect() {
    context.commandList->close();
    context.nvrhiDevice->executeCommandList(context.commandList);
    context.commandList = context.nvrhiDevice->createCommandList();
    context.commandList->open();
}

Texture     getSwapchainTexture    () {return context.swapchainTextures[context.imageIndex];}
Texture     getSwapchainDepth      () {return context.depthTexture;}
Framebuffer getSwapchainFramebuffer() {return context.framebuffers[context.imageIndex];}
Format      swapchainFormat        () {return (Format)getNvTexture(context.swapchainTextures[0])->getDesc().format;}
ColorSpace  swapchainColorSpace    () {return context.colorSpace;}
uint        getWidth               () {return context.width;}
uint        getHeight              () {return context.height;}
struct SDL_Window* getWindow       () {return context.window;}
const char* getDeviceName          () {return context.device.physical_device.name.c_str();}
bool        raytracingEnabled      () {return context.raytracing;}

void beginTimerQuery() {
    context.commandList->beginTimerQuery(context.timerQuery);
}

void endTimerQuery() {
    context.commandList->endTimerQuery(context.timerQuery);
}

float timerQueryResult() {
    waitGPUIdle();
    const float result = context.nvrhiDevice->getTimerQueryTime(context.timerQuery);
    context.nvrhiDevice->resetTimerQuery(context.timerQuery);
    return result;
}

SDL_Window* getSDLWindow() {return context.window;}

}

nvrhi::IDevice*      getDevice     () {return context.nvrhiDevice;}
nvrhi::ICommandList* getCommandList() {return context.commandList;}
