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

#include <assimp/scene.h>  // ����ģ��

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

        // �����
        alignas(16)glm::vec3 directLightColor;
        alignas(16)glm::vec3 directLightDir;
        // ���Դ
        alignas(16)glm::vec3 pointLightColor;
        alignas(16)glm::vec3 pointLightPos;
        // �۹��
        alignas(16)glm::vec3 spotLightColor;
        alignas(16)glm::vec3 spotLightPos;
        alignas(16)glm::vec3 spotLightDir;
        alignas(4)float spotLightPhi;

        alignas(16)glm::mat4 lightVP;
    };

    struct DynamicUBO {
        glm::mat4* model = nullptr;
    };

    // ��Ӱ����
    struct ShadowMVP {
        glm::mat4 mvp;
    };

    class VkApp {
    public:
        void Init();
        void Loop();
        void Clear();
        void OnInput(int key, int action);
        bool m_FrameBufferResized;  // �������ڳߴ�

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
        VkDescriptorSetLayout m_ShadowMapDescriptorSetLayout;
        VkPipelineLayout m_PipelineLayout;

        std::vector<VkFramebuffer> m_SwapChainFrameBuffers;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        VkSemaphore m_ImageAvailableSemaphore;
        VkSemaphore m_RenderFinishedSemaphore;

        // ��֡��Ⱦ
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        int m_CurrentFrame = 0;
        std::vector<VkFence> m_InLightFences;
        std::vector<VkFence> m_ImagesInLight;// ����ͼ����ʾʱ
        VkDescriptorPool m_DescriptorPool;
        VkSampler m_TextureSampler;

        vkobj::Model m_Model;
        vkobj::Camera m_Camera;
        vkobj::PointLight m_Light;
        // ��������
        vkobj::Texture m_Textures[64] = {};
        unsigned int m_TextureIndex = 0;

        // ��̬ͳһ�������
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

        // ����
        vkobj::DirectLight m_DirectLight;
        vkobj::PointLight m_PointLight;
        vkobj::SpotLight m_SpotLight;
        // ��̬ͳһ����
        size_t m_Alignment;

        // ��������
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
        void CreateTextureSampler(VkSampler* pSampler, VkSamplerAddressMode addressMode);
        void CreateVertexBuffer(std::vector<vkobj::Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
        void CreateIndexBuffer(std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
        void CreateDescriptorPool(uint32_t count);
        void CreateDescriptorSet(VkDescriptorSet& descriptorSet, VkBuffer uniformBuffer, VkImageView imageView);
        void CreateShadowMapDescriptroSet(VkDescriptorSet& descriptorSet, VkBuffer uniformBuffer);
        void CreateFrameBuffers();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void DrawFrame();
        void ClearSwapChain();
        void RecreateSwapChain();
        void UpdateUniformBuffer(vkobj::Model* pModel, vkobj::Camera* pCamera, vkobj::PointLight* pLight);

        bool CheckValidationLayerSupport();
        // ��ȡ��Ҫ����չ
        std::vector<const char*> GetRequiredExtensions();
        // ���������չ�Ƿ����
        bool CheckExtensionSupport(VkPhysicalDevice device);
        // �����豸�Ƿ����
        bool IsDeviceSuitable(VkPhysicalDevice device);
        // ��ѯ������֧�ֵ�����
        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
        // ��ȡ����������
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        // ѡ������ʽ
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        // ѡ�����ģʽ
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
        // ѡ��Χ
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        // ������ɫ��ģ��
        VkShaderModule CreateShaderModule(const std::vector<char>& code);
        // ��ȡ�ڴ�����
        uint32_t FindMemoryType(uint32_t typrFilter, VkMemoryPropertyFlags properties);
        // ��������
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        // ���ƻ���
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        // ��������ָ���
        VkCommandBuffer BeginSingleTimeCommands();
        // ��������ָ���
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        // ת��ͼ�񲼾�
        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        // ���ƻ��嵽ͼ��
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        // ����ͼ��
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);
        // ����ͼ����ͼ
        void CreateImageView(VkImageView* imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        // �豸�Ƿ�֧�ָ�������
        bool IsDeviceSupportAnisotropy(VkPhysicalDevice device);
        // Ѱ�Һ��ʵĸ�ʽ
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
        // ��ȡ���ͼ���ʽ
        VkFormat FindDepthFormat();
        // ͼ���ʽ�Ƿ�֧��ģ��
        bool HasStencilComponent(VkFormat format);
        // ��������
        vkobj::Texture* GetTexture(const char* path);
        // ��ȡfbx�����ڵ�
        void LoadNode(aiNode* ainode, const aiScene* aiscene, vkobj::Model* model);
        // ׼����̬ͳһ����
        void PrepareUniformBuffers();
        void CreateDynamicDescriptorSet(VkDescriptorSet& descriptorSet, VkBuffer dynamicUniform);
        // �������
        void InitCamera(vkobj::Camera* pCamera);
        // �������
        void DestroyCamera(vkobj::Camera* pCamera);
        void DrawMesh(vkobj::Mesh& mesh);//����ģ��
        // �ƹ�
        void InitLight();//��ʼ���ƹ�
        void DrawLight();// ��Ⱦ�ƹ�
        void DestroyLight();// ���ٵƹ�
        // ģ��
        void CreateModel(vkobj::Model* pModel);//����ģ��
        void DrawModel(vkobj::Model* pModel);//����ģ��
        void DestroyModel(vkobj::Model* pModel);//����ģ��
        // ����
        void CreateCube(vkobj::Cube* pCube); // ��������

        void CreateShadowMapPass(vkobj::ShadowMapPass* pPass);//������Ӱ����ͨ��
        void DestroyShadowMapPass(vkobj::ShadowMapPass* pPass);//������Ӱ����ͨ��
        void CreateShadowMapCommandBuffer(vkobj::ShadowMapPass* pPass); // ������Ӱ�������ָ��
    };
}
