#include <stdbool.h>
// Standalone repro: link a tessellation pipeline from graphics pipeline libraries
// where the hull shader reads its input control points at a data-computed index.
#include <vulkan/vulkan.h>
#include <stdio.h>
static VkInstance instance;
static VkDevice device;
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CHECK(x) do { VkResult r_ = (x); if (r_ != VK_SUCCESS) { printf("FAIL %s -> %d\n", #x, (int)r_); exit(1); } } while (0)

static uint32_t* load(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint32_t* words = malloc(*size);
    assert(fread(words, 1, *size, f) == *size);
    fclose(f);
    return words;
}

int main(void) {
    CHECK(vkCreateInstance(&(VkInstanceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo){ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3 },
    }, NULL, &instance));
    printf("instance\n");

    uint32_t count = 0;
    CHECK(vkEnumeratePhysicalDevices(instance, &count, NULL));
    VkPhysicalDevice* devices = malloc(count * sizeof(VkPhysicalDevice));
    CHECK(vkEnumeratePhysicalDevices(instance, &count, devices));
    VkPhysicalDevice physical = devices[0];
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceDriverProperties driver = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };
        VkPhysicalDeviceProperties2 properties = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &driver };
        vkGetPhysicalDeviceProperties2(devices[i], &properties);
        printf("device %u: %s (%s)\n", i, properties.properties.deviceName, driver.driverName);
        if (strcmp(driver.driverName, "radv") == 0 || strstr(properties.properties.deviceName, "RADV")) physical = devices[i];
    }

    float priorities = 1.0f;
    CHECK(vkCreateDevice(physical, &(VkDeviceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = &priorities,
        },
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char*[]){ VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME },
        .pEnabledFeatures = &(VkPhysicalDeviceFeatures){ .tessellationShader = VK_TRUE },
    }, NULL, &device));
    printf("device\n");
    VkQueue queue;
    vkGetDeviceQueue(device, 0, 0, &queue);

    size_t vertSize, tescSize, teseSize, fragSize;
    uint32_t* vert = load("tess.vert.spv", &vertSize);
    uint32_t* tesc = load("tess.tesc.spv", &tescSize);
    uint32_t* tese = load("tess.tese.spv", &teseSize);
    uint32_t* frag = load("tess.frag.spv", &fragSize);

    VkShaderModuleCreateInfo module = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    };
    VkDescriptorSetLayout setLayout;
    CHECK(vkCreateDescriptorSetLayout(device, &(VkDescriptorSetLayoutCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1, .pBindings = &binding,
    }, NULL, &setLayout));
    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1, .pSetLayouts = &setLayout,
    };
    VkPipelineLayout layout;
    CHECK(vkCreatePipelineLayout(device, &layoutInfo, NULL, &layout));

    VkPipelineRenderingCreateInfo rendering = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = (VkFormat[]){ VK_FORMAT_B8G8R8A8_UNORM },
    };

    printf("pre-raster library\n");
    module.codeSize = vertSize; module.pCode = vert;
    VkShaderModule vertModule;
    CHECK(vkCreateShaderModule(device, &(VkShaderModuleCreateInfo){ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = module.codeSize, .pCode = module.pCode }, NULL, &vertModule));
    module.codeSize = tescSize; module.pCode = tesc;
    VkShaderModule tescModule;
    CHECK(vkCreateShaderModule(device, &(VkShaderModuleCreateInfo){ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = module.codeSize, .pCode = module.pCode }, NULL, &tescModule));
    module.codeSize = teseSize; module.pCode = tese;
    VkShaderModule teseModule;
    CHECK(vkCreateShaderModule(device, &(VkShaderModuleCreateInfo){ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = module.codeSize, .pCode = module.pCode }, NULL, &teseModule));

    VkPipeline preRaster;
    CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo){
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR,
        .stageCount = 3,
        .pStages = (VkPipelineShaderStageCreateInfo[]){
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vertModule, .pName = "main" },
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, .module = tescModule, .pName = "main" },
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, .module = teseModule, .pName = "main" },
        },
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST },
        .pTessellationState = &(VkPipelineTessellationStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, .patchControlPoints = 3 },
        .pViewportState = &(VkPipelineViewportStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f },
        .layout = layout,
    }, NULL, &preRaster));
    printf("pre-raster library ok\n");

    module.codeSize = fragSize; module.pCode = frag;
    VkShaderModule fragModule;
    CHECK(vkCreateShaderModule(device, &(VkShaderModuleCreateInfo){ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = module.codeSize, .pCode = module.pCode }, NULL, &fragModule));
    VkPipeline fragment;
    CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo){
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR,
        .stageCount = 1,
        .pStages = (VkPipelineShaderStageCreateInfo[]){
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fragModule, .pName = "main" },
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f, .cullMode = VK_CULL_MODE_NONE },
        .layout = layout,
    }, NULL, &fragment));
    printf("fragment library ok\n");

    if (getenv("DIRECT")) {
        printf("direct pipeline\n");
        VkPipeline direct;
        CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo){
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering,
            .stageCount = 4,
            .pStages = (VkPipelineShaderStageCreateInfo[]){
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vertModule, .pName = "main" },
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, .module = tescModule, .pName = "main" },
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, .module = teseModule, .pName = "main" },
                { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fragModule, .pName = "main" },
            },
            .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO },
            .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST },
            .pTessellationState = &(VkPipelineTessellationStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, .patchControlPoints = 3 },
            .pViewportState = &(VkPipelineViewportStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 },
            .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f, .cullMode = VK_CULL_MODE_NONE },
            .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT },
            .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1,
                .pAttachments = (VkPipelineColorBlendAttachmentState[]){ { .colorWriteMask = 0xF } } },
            .layout = layout,
            .renderPass = VK_NULL_HANDLE,
        }, NULL, &direct));
        printf("DIRECT OK - no crash\n");
        return 0;
    }
    printf("linking\n");
    VkPipelineLibraryCreateInfoKHR libraries = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
        .libraryCount = 2,
        .pLibraries = (VkPipeline[]){ preRaster, fragment },
    };
    VkPipeline linked;
    CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo){
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraries,
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST },
        .pViewportState = &(VkPipelineViewportStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1 },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f, .cullMode = VK_CULL_MODE_NONE },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT },
        .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO },
        .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1,
            .pAttachments = (VkPipelineColorBlendAttachmentState[]){ { .colorWriteMask = 0xF } } },
        .layout = layout,
        .renderPass = VK_NULL_HANDLE,
    }, NULL, &linked));
    printf("LINKED OK - no crash\n");
    return 0;
}
