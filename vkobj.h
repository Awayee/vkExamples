#pragma once
#include <vulkan\vulkan.h>

#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <string>


namespace vkobj {
    // 顶点
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec3 tangent;

        static VkVertexInputBindingDescription GetBindDescription() {
            VkVertexInputBindingDescription bindDescription{};
            bindDescription.binding = 0;
            bindDescription.stride = sizeof(Vertex);
            bindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 5> GetAttributeDescrioptions() {
            std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, normal);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(Vertex, tangent);

            return attributeDescriptions;
        }

        // 阴影映射用
        static VkVertexInputBindingDescription GetShadowMapBindDescription() {
            VkVertexInputBindingDescription bindDescription{};
            bindDescription.binding = 0;
            bindDescription.stride = sizeof(glm::vec3);
            bindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindDescription;
        }

        static std::array< VkVertexInputAttributeDescription, 1> GetShadowMapAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            return attributeDescriptions;
        }
    };

    struct Texture {
        std::string path;
        uint32_t index;
        VkImage image;
        VkDeviceMemory imageMemroy;
        VkImageView imageView;
    };


    struct Material {
        std::string vertexShader;
        std::string fragmentShader;
        Texture* texture;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSet descriptorSet;
        VkBuffer uniformBuffer;
        VkDeviceMemory uniformBufferMemory;
        glm::vec3 diffuseColor = { 1.0f, 1.0f, 1.0f };
        glm::vec3 specularColor = { 1.0f, 1.0f, 1.0f };
        float gloss = 32;
    };


    struct Mesh {
        Material material;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemroy;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
    };

    struct Model {
        const char* path;
        std::vector<Mesh> meshes;
        glm::mat4 transform;
        // 阴影映射
        VkBuffer uniform;
        VkDeviceMemory uniformMemory;
        VkDescriptorSet descriptorSet;
    };

    struct Cube {
        glm::mat4 transform;
        Mesh mesh;
        // 阴影映射
        VkBuffer uniform;
        VkDeviceMemory uniformMemory;
        VkDescriptorSet descriptorSet;
    };

    struct Camera {
        // Transform
        glm::vec3 pos;
        glm::vec3 at;
        glm::vec3 up;
        glm::mat4 transformMatrix;
        // Perspective
        float fov;
        float aspect;
        float nearClip;
        float farClip;
        glm::mat4 perspectiveMatrix;
        // depth
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
        void SetPerspective(float camFov, float camAspect, float camNear, float camFar) {
            fov = camFov;
            aspect = camAspect;
            nearClip = camNear;
            farClip = camFar;
            perspectiveMatrix = glm::perspective(fov, aspect, nearClip, farClip);
            perspectiveMatrix[1][1] *= -1;
        }
        void SetTransform(glm::vec3 camPos, glm::vec3 camAt, glm::vec3 camUp) {
            pos = camPos;
            at = camAt;
            up = camUp;
            transformMatrix = glm::lookAt(pos, at, up);
        }
    };

    struct ShadowMapPass {
        uint32_t width, height;
        // 深度图
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
        VkSemaphore semaphore;

        VkFramebuffer frameBuffer;
        VkRenderPass renderPass;
        VkSampler depthSampler;
        VkCommandBuffer cmdBuffer;

        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };
    // 方向光
    struct DirectLight {
        float strength;
        glm::vec3 color;
        glm::vec3 dir;
    };
    // 点光
    struct PointLight {
        float strength;
        glm::vec3 color;
        glm::vec3 pos;
        ShadowMapPass shadowPass;
    };

    // 聚光灯
    struct SpotLight {
        float stength;
        glm::vec3 color;
        glm::vec3 pos;
        glm::vec3 dir;
        float phi;
    };

}