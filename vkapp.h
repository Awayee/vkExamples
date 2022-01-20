#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vector>
#include <optional>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <array>
#include "vkobj.h"

#include <assimp/scene.h>  // 加载模型

namespace vkapp {
    const uint32_t WINDOW_W = 800;
    const uint32_t WINDOW_H = 600;

    const int MAX_FRAME_IN_FLIGHT = 2;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };


    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;

        alignas(16)glm::vec3 viewPos;
        alignas(16)glm::vec3 diffuseColor;
        alignas(16)glm::vec3 specularColor;
        alignas(4)float gloss;

        // 方向光
        alignas(16)glm::vec3 directLightColor;
        alignas(16)glm::vec3 directLightDir;
        // 点光源
        alignas(16)glm::vec3 pointLightColor;
        alignas(16)glm::vec3 pointLightPos;
        // 聚光灯
        alignas(16)glm::vec3 spotLightColor;
        alignas(16)glm::vec3 spotLightPos;
        alignas(16)glm::vec3 spotLightDir;
        alignas(4)float spotLightPhi;
    };

    struct DynamicUBO {
        glm::mat4* model = nullptr;
    };

    class VkApp {
    public:
        void Init();
        void Loop();
        void Clear();
        void OnInput(int key, int action);
        bool m_FrameBufferResized;  // 调整窗口尺寸

    private:
        bool m_EnableValidationLayers = true;
        GLFWwindow* m_Window;
        VkInstance m_Instance;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkSurfaceKHR m_Surface;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_DeviceProperties;
        VkDevice m_Device;

        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        VkSwapchainKHR m_SwapChain;
        std::vector<VkImage> m_SwapChainImages;

        VkFormat m_SwapChainImageFormat;
        VkExtent2D m_SwapChainExtent;

        std::vector<VkImageView> m_SwapChainImageViews;

        VkRenderPass m_RenderPass;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        VkPipelineLayout m_PipelineLayout;

        std::vector<VkFramebuffer> m_SwapChainFrameBuffers;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        VkSemaphore m_ImageAvailableSemaphore;
        VkSemaphore m_RenderFinishedSemaphore;

        // 多帧渲染
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        int m_CurrentFrame = 0;
        std::vector<VkFence> m_InLightFences;
        std::vector<VkFence> m_ImagesInLight;// 处理图像显示时
        VkDescriptorPool m_DescriptorPool;
        VkSampler m_TextureSampler;

        vkobj::Model m_Model;
        vkobj::Camera m_Camera;
        vkobj::PointLight m_Light;
        // 缓存纹理
        vkobj::Texture m_Textures[64] = {};
        unsigned int m_TextureIndex = 0;

        // 动态统一缓冲相关
        std::vector<void*> m_DynamicUboData;
        size_t m_DynamicAlignment;
        size_t m_NormalUBOAlignment;
        DynamicUBO m_DynamicUbo;
        VkBuffer m_DynamicUniform;
        VkBuffer m_DynamicUniformMemory;
        std::vector<vkobj::Vertex> m_DynamicVertices = {
        };
        std::vector<uint32_t> m_DynamicIndices = {};
        std::vector<VkDescriptorSet> m_DynamicDescriptorSets = {};

        // 光照
        vkobj::DirectLight m_DirectLight;
        vkobj::PointLight m_PointLight;
        vkobj::SpotLight m_SpotLight;
        // 动态统一缓冲
        size_t m_Alignment;

        // 处理输入
        glm::vec2 m_LastMouse = { 0, 0 };
        

        void InitWindow();

        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapChain();
        void CreateRenderPass();
        void CreateDescriptorSetLayout();
        void CreateGraphicsPipeline(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, std::string& vert, std::string& frag);
        void CreateCommandPool();
        void CreateTextureSampler();
        void CreateVertexBuffer(std::vector<vkobj::Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
        void CreateIndexBuffer(std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
        void CreateDescriptorPool(uint32_t count);
        void CreateDescriptorSet(VkDescriptorSet& descriptorSet, VkBuffer uniformBuffer, VkImageView imageView);
        void CreateFrameBuffers();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void DrawFrame();
        void ClearSwapChain();
        void RecreateSwapChain();
        void UpdateUniformBuffer(vkobj::Model* pModel, vkobj::Camera* pCamera, vkobj::PointLight* pLight);

        bool CheckValidationLayerSupport();
        // 获取需要的扩展
        std::vector<const char*> GetRequiredExtensions();
        // 检查物理扩展是否可用
        bool CheckExtensionSupport(VkPhysicalDevice device);
        // 物理设备是否可用
        bool IsDeviceSuitable(VkPhysicalDevice device);
        // 查询交换链支持的详情
        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
        // 获取队列族属性
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        // 选择表面格式
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        // 选择呈现模式
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
        // 选择范围
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        // 创建着色器模型
        VkShaderModule CreateShaderModule(const std::vector<char>& code);
        // 获取内存类型
        uint32_t FindMemoryType(uint32_t typrFilter, VkMemoryPropertyFlags properties);
        // 创建缓冲
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        // 复制缓冲
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        // 创建单次指令缓冲
        VkCommandBuffer BeginSingleTimeCommands();
        // 结束单次指令缓冲
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        // 转换图像布局
        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        // 复制缓冲到图像
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        // 创建图像
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);
        // 创建图像视图
        void CreateImageView(VkImageView* imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        // 设备是否支持各向异性
        bool IsDeviceSupportAnisotropy(VkPhysicalDevice device);
        // 寻找合适的格式
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
        // 获取深度图像格式
        VkFormat FindDepthFormat();
        // 图像格式是否支持模板
        bool HasStencilComponent(VkFormat format);
        // 创建纹理
        vkobj::Texture* GetTexture(const char* path);
        // 读取fbx场景节点
        void LoadNode(aiNode* ainode, const aiScene* aiscene, vkobj::Model* model);
        // 准备动态统一缓冲
        void PrepareUniformBuffers();
        void CreateDynamicDescriptorSet(VkDescriptorSet& descriptorSet, VkBuffer dynamicUniform);
        // 创建相机
        void InitCamera(vkobj::Camera* pCamera);
        // 销毁相机
        void DestroyCamera(vkobj::Camera* pCamera);
        void DrawMesh(vkobj::Mesh& mesh);//绘制模型
        // 灯光
        void InitLight();//初始化灯光
        void DrawLight();// 渲染灯光
        void DestroyLight();// 销毁灯光
        // 模型
        void CreateModel(vkobj::Model* pModel);//创建模型
        void DrawModel(vkobj::Model* pModel);//绘制模型
        void DestroyModel(vkobj::Model* pModel);//销毁模型

        void CreateShadowMapPass(vkobj::ShadowMapPass* pPass);//创建阴影纹理通道
        void CreateShadowMapCommandBuffer(VkCommandBuffer* pCmdBuffer); // 创建阴影纹理绘制指令
    };
}
