
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW\glfw3native.h>

#include <iostream>
#include <vector>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max
#include <fstream>

#include "vkapp.h"
#include "vkinit.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// 加载模型
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "vkobj.h"
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)


#define OBJECT_INSTANCES 64
namespace vkapp {
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	// 读文件
	static std::vector<char> ReadFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	static void FrameBufferResizeCallback(GLFWwindow* window, int width, int height) {
		VkApp* vkApp = reinterpret_cast<VkApp*>(glfwGetWindowUserPointer(window));
		vkApp->m_FrameBufferResized = true;
		std::cout << "窗口尺寸改变了: " << width << ", " << height << std::endl;
	}

	void* alignedAlloc(size_t size, size_t alignment) {
		void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
		data = _aligned_malloc(size, alignment);
#else
		int res = posix_memalign(&data, alignment, size);
		if (res != 0)
			data = nullptr;
#endif
		return data;
	}

	// 按键回调
	void KeyInputCalklback(GLFWwindow* window, int key, int scancode, int action, int mod) {
		VkApp* app = reinterpret_cast<VkApp*>(glfwGetWindowUserPointer(window));
		app->OnInput(key, action);
	}

	void VkApp::OnInput(int key, int action) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			if (key == GLFW_KEY_LEFT) {
				m_SpotLight.pos = glm::rotate(glm::mat4(1.0), glm::radians(10.0f), { 0.0f, 0.0f, 1.0f }) * glm::vec4(m_SpotLight.pos, 1.0f);
			}
			if (key == GLFW_KEY_RIGHT) {
				m_SpotLight.pos = glm::rotate(glm::mat4(1.0), -glm::radians(10.0f), { 0.0f, 0.0f, 1.0f }) * glm::vec4(m_SpotLight.pos, 1.0f);
			}
		}
		if (key == GLFW_MOUSE_BUTTON_1) {
			if (action == GLFW_PRESS) {
				double x, y;
				glfwGetCursorPos(m_Window, &x, &y);
				m_LastMouse.x = (float)x;
				m_LastMouse.y = (float)y;
			}
			else if (action == GLFW_RELEASE) {
				m_LastMouse.x = 0.0f;
				m_LastMouse.y = 0.0f;
			}
			else if (action == GLFW_REPEAT) {

			}
		}
	}

	// 初始化窗口
	void VkApp::InitWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(WINDOW_W, WINDOW_H, "VULKAN", nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetFramebufferSizeCallback(m_Window, FrameBufferResizeCallback);
		glfwSetKeyCallback(m_Window, KeyInputCalklback);
	}

	void VkApp::Init()
	{
		InitWindow();
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateRenderPass();
		InitCamera(&m_Camera);
		CreateFrameBuffers();
		CreateCommandPool();
		CreateDescriptorPool(32);
		CreateDescriptorSetLayout();
		CreateTextureSampler(&m_TextureSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		InitLight();
		CreateModel(&m_Model);
		CreateCommandBuffers();
		CreateShadowMapCommandBuffer(&m_PointLight.shadowPass);
		CreateSyncObjects();
	}

	void VkApp::Loop()
	{
		while (!glfwWindowShouldClose(m_Window)) {
			glfwPollEvents();
			DrawFrame();
		}
		vkDeviceWaitIdle(m_Device);
	}

	void VkApp::Clear() {
		ClearSwapChain();
		DestroyModel(&m_Model);
		DestroyLight();
		for (vkobj::Texture& texture : m_Textures) {
			if (texture.image != VK_NULL_HANDLE) {
				vkDestroyImage(m_Device, texture.image, nullptr);
			}
			if (texture.imageMemroy != VK_NULL_HANDLE) {
				vkFreeMemory(m_Device, texture.imageMemroy, nullptr);
			}
			if (texture.imageView != VK_NULL_HANDLE) {
				vkDestroyImageView(m_Device, texture.imageView, nullptr);
			}
		}
		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_ShadowMapDescriptorSetLayout, nullptr);
		vkDestroySampler(m_Device, m_TextureSampler, nullptr);
		//vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
		//vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InLightFences[i], nullptr);
		}
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		if (m_EnableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		glfwDestroyWindow(m_Window);
		glfwTerminate();

	}

	// 获取需要的扩展
	std::vector<const char*> VkApp::GetRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		std::cout << "需要的扩展:" << std::endl;
		for (const char* name : extensions) {
			std::cout << '\t' << name << std::endl;
		}
		if (m_EnableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	// 检查验证层是否可用
	bool VkApp::CheckValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	// 创建Vulkan 实例
	void VkApp::CreateInstance() {
		if (m_EnableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("验证层不可用!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));
	}


	// 初始化验证层
	void VkApp::SetupDebugMessenger()
	{
		if (!m_EnableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	// 创建显示表面
	void VkApp::CreateSurface() {
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
		else {
			std::cout << "创建显示表面成功!" << std::endl;
		}
	}


	// 获取队列族
	QueueFamilyIndices VkApp::FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.IsComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	bool VkApp::CheckExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		bool isSupport = false;
		for (const char* extensionName : deviceExtensions) {
			bool flag = false;
			for (auto& extension : availableExtensions) {
				if (strcmp(extensionName, extension.extensionName) != -1) {
					flag = true;
					isSupport = true;
					break;
				}
			}
			if (flag) break;
		}

		return isSupport;
	}

	SwapChainSupportDetails VkApp::QuerySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
		if (formatCount > 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
		if (presentModeCount > 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	// 设备是否支持各向异性
	bool VkApp::IsDeviceSupportAnisotropy(VkPhysicalDevice device) {
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
		return supportedFeatures.samplerAnisotropy;
	}

	// 设备是否满足需求
	bool VkApp::IsDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extensionSupported = CheckExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}
		bool supportAnisotropy = IsDeviceSupportAnisotropy(device);
		return indices.IsComplete() && extensionSupported && swapChainAdequate && supportAnisotropy;
	}


	// 选择物理设备
	void VkApp::PickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (IsDeviceSuitable(device)) {
				m_PhysicalDevice = device;
				break;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
	}

	VkSurfaceFormatKHR VkApp::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR VkApp::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VkApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(m_Window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	// 创建逻辑设备
	void VkApp::CreateLogicalDevice() {
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (m_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device));

		vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
		std::cout << "创建逻辑设备成功!" << std::endl;
	}

	void VkApp::CreateSwapChain() {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;
		
		// 图像视图
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
			CreateImageView(&m_SwapChainImageViews[i], m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	VkShaderModule VkApp::CreateShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderModule;
		VK_CHECK(vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule));
		return shaderModule;
	}

	void VkApp::CreateRenderPass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// 深度图
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 不存储深度图像数据
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 不关心上一个深度图数据
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// 子通道依赖
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
		else {
			std::cout << "创建渲染通道成功!" << std::endl;
		}
	}


	void VkApp::CreateGraphicsPipeline(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, std::string& vert, std::string& frag) {
		// 图形管线
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// 创建Shader Stage
		VkShaderModule vertShaderModule = (vert.data() != nullptr) ? vkinit::ShaderModule(m_Device, vert) : VK_NULL_HANDLE;
		VkShaderModule fragShaderModule = (frag.data() != nullptr) ? vkinit::ShaderModule(m_Device, frag) : VK_NULL_HANDLE;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = vkinit::PipelineShaderStageCreateInfo(vertShaderModule, fragShaderModule);
		pipelineInfo.stageCount = static_cast<uint32_t> (shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();

		// 顶点输入
		auto bindingDescription = vkobj::Vertex::GetBindDescription();
		auto attributeDescriptions = vkobj::Vertex::GetAttributeDescrioptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::PipelineVertexInputStateCreateInfo(1, &bindingDescription, attributeDescriptions.size(), attributeDescriptions.data());
		pipelineInfo.pVertexInputState = &vertexInputInfo;

		// 输入组件
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::PipelineInputAssemblyStateCreateInfo();
		pipelineInfo.pInputAssemblyState = &inputAssembly;

		// 视口和裁剪
		VkViewport viewport = vkinit::Viewport((float)m_SwapChainExtent.width, float(m_SwapChainExtent.height));
		VkRect2D scissor = vkinit::Rect2D(m_SwapChainExtent.width, m_SwapChainExtent.height);
		VkPipelineViewportStateCreateInfo viewportState = vkinit::PipelineViewportStateCreateInfo(&viewport, &scissor);
		pipelineInfo.pViewportState = &viewportState;


		// 光栅化
		VkPipelineRasterizationStateCreateInfo rasterizer = vkinit::PipelineRasterizationStateCreateInfo();
		pipelineInfo.pRasterizationState = &rasterizer;


		// 重采样
		VkPipelineMultisampleStateCreateInfo multisampling = vkinit::PipelineMultisampleStateCreateInfo();
		pipelineInfo.pMultisampleState = &multisampling;



		// 混合
		VkPipelineColorBlendAttachmentState blendAttachment = vkinit::PipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::PipelineColorBlendStateCreateInfo(1, &blendAttachment);
		pipelineInfo.pColorBlendState = &colorBlending;

		// 动态调整
		VkPipelineDynamicStateCreateInfo dynamicState = vkinit::PipelineDynamicStateCreateInfo();
		pipelineInfo.pDynamicState = nullptr; // Optional

		// 深度缓冲
		VkPipelineDepthStencilStateCreateInfo depthStencil = vkinit::PipelineDepthStencilStateCreateInfo();
		pipelineInfo.pDepthStencilState = &depthStencil;


		// 管线重布局
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::PipelineLayoutCreateInfo(1, &m_DescriptorSetLayout);
		VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
		pipelineInfo.layout = pipelineLayout;

		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
		// 清理着色器模型
		vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
	}


	void VkApp::CreateFrameBuffers() {
		size_t imageCount = m_SwapChainImageViews.size();
		m_SwapChainFrameBuffers.resize(imageCount);

		for (size_t i = 0; i < imageCount; i++) {
			VkImageView attachments[] = { m_SwapChainImageViews[i], m_Camera.depthImageView };
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_SwapChainExtent.width;
			framebufferInfo.height = m_SwapChainExtent.height;
			framebufferInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapChainFrameBuffers[i]));
		}
	}

	void VkApp::CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = 0; // Optional
		VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool));
	}

	void VkApp::CreateCommandBuffers() {
		size_t bufferSize = m_SwapChainFrameBuffers.size();
		m_CommandBuffers.resize(bufferSize);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = bufferSize;

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()));
		for (size_t i = 0; i < bufferSize; i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // Optional
			beginInfo.pInheritanceInfo = nullptr; // Optional

			VK_CHECK(vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo));

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = m_SwapChainFrameBuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_SwapChainExtent;

			VkClearValue clearValues[] = { {}, {} };
			clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 }; // 深度[0, 1]
			renderPassInfo.clearValueCount = 2;
			renderPassInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			for (vkobj::Mesh& mesh : m_Model.meshes) {
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mesh.material.pipeline);
				VkBuffer buffers[] = { mesh.vertexBuffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, buffers, offsets);
				vkCmdBindIndexBuffer(m_CommandBuffers[i], mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mesh.material.pipelineLayout, 0, 1, &mesh.material.descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
			}
			vkCmdEndRenderPass(m_CommandBuffers[i]);
			VK_CHECK(vkEndCommandBuffer(m_CommandBuffers[i]));
			std::cout << "指令缓冲区记录结束! index=" << i << std::endl;
		}
	}

	void VkApp::CreateSyncObjects() {
		//VkSemaphoreCreateInfo semaphoreInfo{};
		//semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		//VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore));
		m_ImageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAME_IN_FLIGHT);
		m_InLightFences.resize(MAX_FRAME_IN_FLIGHT);
		m_ImagesInLight.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
			VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InLightFences[i]));
		}
	}

	void VkApp::DrawFrame() {
		vkWaitForFences(m_Device, 1, &m_InLightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult rs = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
		if (rs == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (rs != VK_SUCCESS && rs != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("获取交换链图像失败!");
		}

		if (m_ImagesInLight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(m_Device, 1, &m_ImagesInLight[imageIndex], VK_TRUE, UINT64_MAX);
		}

		m_ImagesInLight[imageIndex] = m_InLightFences[m_CurrentFrame];
		UpdateUniformBuffer(&m_Model, &m_Camera, &m_Light);
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// 先提交阴影纹理
		VkSubmitInfo shadowMapSubmit = {};
		shadowMapSubmit.pNext = nullptr;
		shadowMapSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		shadowMapSubmit.waitSemaphoreCount = 1;
		shadowMapSubmit.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
		shadowMapSubmit.signalSemaphoreCount = 1;
		shadowMapSubmit.pSignalSemaphores = &m_PointLight.shadowPass.semaphore;
		shadowMapSubmit.pWaitDstStageMask = waitStages;
		shadowMapSubmit.commandBufferCount = 1;
		shadowMapSubmit.pCommandBuffers = &m_PointLight.shadowPass.cmdBuffer;
		VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &shadowMapSubmit, VK_NULL_HANDLE));
		// 提交指令缓冲
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_PointLight.shadowPass.semaphore };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// 提交前重置栅栏
		vkResetFences(m_Device, 1, &m_InLightFences[m_CurrentFrame]);
		VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InLightFences[m_CurrentFrame]));

		// 呈现
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		rs = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (rs == VK_ERROR_OUT_OF_DATE_KHR || rs == VK_SUBOPTIMAL_KHR || m_FrameBufferResized) {
			m_FrameBufferResized = false;
			RecreateSwapChain();
		}
		else if (rs != VK_SUCCESS) {
			throw std::runtime_error("呈现图像失败!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAME_IN_FLIGHT;

	}

	void VkApp::ClearSwapChain() {
		DestroyCamera(&m_Camera);
		for (auto framebuffer : m_SwapChainFrameBuffers) {
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
		}
		vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

		for (vkobj::Mesh& mesh : m_Model.meshes) {
			vkDestroyPipeline(m_Device, mesh.material.pipeline, nullptr);
			vkDestroyPipelineLayout(m_Device, mesh.material.pipelineLayout, nullptr);
		}
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		for (auto imageView : m_SwapChainImageViews) {
			vkDestroyImageView(m_Device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
	}

	void VkApp::RecreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_Window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_Device);
		ClearSwapChain();
		CreateSwapChain();
		CreateRenderPass();
		InitCamera(&m_Camera);
		CreateDescriptorPool(m_Model.meshes.size());
		for (vkobj::Mesh& mesh : m_Model.meshes) {
			CreateGraphicsPipeline(mesh.material.pipeline, mesh.material.pipelineLayout, mesh.material.vertexShader, mesh.material.fragmentShader);
			CreateDescriptorSet(mesh.material.descriptorSet, mesh.material.uniformBuffer, mesh.material.texture->imageView);
		}
		CreateFrameBuffers();
		CreateCommandBuffers();
	}

	uint32_t VkApp::FindMemoryType(uint32_t typrFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typrFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
				return i;
			}
		}
	}
	void VkApp::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory));
		vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
	}


	void VkApp::CreateVertexBuffer(std::vector<vkobj::Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory) {
		VkDeviceSize size = sizeof(vertices[0]) * vertices.size();
		// staging buffer
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);
		// map memory
		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, size, 0, &data);
		memcpy(data, vertices.data(), (size_t)size);
		vkUnmapMemory(m_Device, stagingBufferMemory);
		// vertex buffer
		CreateBuffer(size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);
		// copy buffer
		CopyBuffer(stagingBuffer, vertexBuffer, size);
		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	void VkApp::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		EndSingleTimeCommands(commandBuffer);
	}


	void VkApp::CreateIndexBuffer(std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory) {
		VkDeviceSize size = sizeof(indices[0]) * indices.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);
		// map memory
		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, size, 0, &data);
		memcpy(data, indices.data(), size);

		CreateBuffer(size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBuffer,
			indexBufferMemory);

		CopyBuffer(stagingBuffer, indexBuffer, size);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	void VkApp::CreateDescriptorSetLayout() {
		auto uboLayoutBinding = vkinit::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		// 图像采样
		auto samplerLayoutBinding = vkinit::DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		// 深度图
		auto shadowMapSampler = vkinit::DescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, shadowMapSampler };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

		// 创建阴影映射布局
		auto shadowMapBinding = vkinit::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		VkDescriptorSetLayoutCreateInfo shadowLayoutInfo{};
		shadowLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		shadowLayoutInfo.bindingCount = 1;
		shadowLayoutInfo.pBindings = &shadowMapBinding;
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_ShadowMapDescriptorSetLayout));

	}

	void VkApp::UpdateUniformBuffer(vkobj::Model* pModel, vkobj::Camera* pCamera, vkobj::PointLight* pLight) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		/*ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;*/
		glm::mat4 lightVP = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f) *
			glm::lookAt(m_PointLight.pos, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });

		// 阴影通道
		
		ShadowMVP shadowMvp{};
		shadowMvp.mvp = lightVP * pModel->transform;
		void* shadowMvpData;
		vkMapMemory(m_Device, pModel->uniformMemory, 0, sizeof(shadowMvpData), 0, &shadowMvpData);
		memcpy(shadowMvpData, &shadowMvp, sizeof(shadowMvp));
		vkUnmapMemory(m_Device, pModel->uniformMemory);

		for (vkobj::Mesh& mesh : pModel->meshes) {
			UniformBufferObject ubo{};
			ubo.model = pModel->transform;
			ubo.view = pCamera->transformMatrix;
			ubo.proj = pCamera->perspectiveMatrix;
			ubo.viewPos = pCamera->pos;
			ubo.diffuseColor = mesh.material.diffuseColor;
			ubo.specularColor = mesh.material.specularColor;
			ubo.gloss = mesh.material.gloss;
			// 平行光
			ubo.directLightColor = m_DirectLight.color;
			ubo.directLightDir = glm::normalize(m_DirectLight.dir);
			// 点光源
			ubo.pointLightColor = m_PointLight.color;
			ubo.pointLightPos = m_PointLight.pos;
			// 聚光灯
			ubo.spotLightColor = m_SpotLight.color;
			ubo.spotLightPos = m_SpotLight.pos;
			ubo.spotLightDir = glm::normalize(m_SpotLight.dir);
			ubo.spotLightPhi = glm::cos(m_SpotLight.phi);
			ubo.lightVP = lightVP;

			void* data;
			vkMapMemory(m_Device, mesh.material.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(m_Device, mesh.material.uniformBufferMemory);
		}
	}

	void VkApp::CreateDescriptorPool(uint32_t count) {
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count?count * 3:count+3},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count}
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = count * 2;
		VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));

	}

	void VkApp::CreateDescriptorSet(VkDescriptorSet& descriptorSet, VkBuffer uniformBuffer, VkImageView imageView) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayout;
		VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet));

		// 更新描述符配置
		VkWriteDescriptorSet descriptorWrites[3] = {};
		// 统一缓冲
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		// 图像采样
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = m_TextureSampler;
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		// 阴影纹理
		VkDescriptorImageInfo shadowMapInfo{};
		shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		shadowMapInfo.imageView = m_PointLight.shadowPass.depthImageView;
		shadowMapInfo.sampler = m_PointLight.shadowPass.depthSampler;
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &shadowMapInfo;
		vkUpdateDescriptorSets(m_Device, 3, descriptorWrites, 0, nullptr);
	}

	void VkApp::CreateShadowMapDescriptroSet(VkDescriptorSet& descriptorSet, VkBuffer uniformBuffer) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_ShadowMapDescriptorSetLayout;
		VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet));

		// 更新描述符配置
		VkWriteDescriptorSet descriptorWrites{};
		// 统一缓冲
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(ShadowMVP);
		descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet = descriptorSet;
		descriptorWrites.dstBinding = 0;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites.descriptorCount = 1;
		descriptorWrites.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(m_Device, 1, &descriptorWrites, 0, nullptr);
	}

	VkCommandBuffer VkApp::BeginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void VkApp::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue);
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
	}


	void VkApp::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VkPipelineStageFlags srcStage{};
		VkPipelineStageFlags dstStage{};
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndSingleTimeCommands(commandBuffer);
	}


	void VkApp::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };
		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		EndSingleTimeCommands(commandBuffer);
	}


	void VkApp::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateImage(m_Device, &imageInfo, nullptr, &image));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory));

		vkBindImageMemory(m_Device, image, memory, 0);
	}


	void VkApp::CreateImageView(VkImageView* imageView, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, imageView));
	}

	void VkApp::CreateTextureSampler(VkSampler* pSampler, VkSamplerAddressMode addressMode) {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// 缩放系数
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		// 填充模式
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		// 各向异性
		samplerInfo.anisotropyEnable = VK_TRUE;
		// 获取各向异性最大采样次数
		samplerInfo.maxAnisotropy = m_DeviceProperties.limits.maxSamplerAnisotropy;
		// 超出图片范围时的边界颜色
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		// 非统一坐标[0,1)
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		// 是否开启比较，一般用于绘制阴影
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		// 多层次细节
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, pSampler));
	}


	VkFormat VkApp::FindSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : formats) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) { return format; }
		}
		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat VkApp::FindDepthFormat() {
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool VkApp::HasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	vkobj::Texture* VkApp::GetTexture(const char* path) {
		if (m_TextureIndex >= 64) {
			std::cout << "Failed to get a texture!" << std::endl;
			return nullptr;
		}
		for (unsigned int i = 0; i < m_TextureIndex; i++) {
			if (m_Textures[i].path == path) {
				return &m_Textures[i];
			}
		}
		
		// 创建新纹理
		vkobj::Texture texture;
		texture.path = path;

		int texWidth, texHeight, texChannels;
		std::cout << "load image: " << path << std::endl;
		stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			std::cout << "Failed to load image file!" << std::endl;
			if (path != "empty.png") {
				return GetTexture("empty.png");
			}
			return nullptr;
		}
		// 暂存缓冲
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);
		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_Device, stagingBufferMemory);
		stbi_image_free(pixels);

		CreateImage(texWidth, texHeight,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			texture.image,
			texture.imageMemroy);

		TransitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		TransitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

		CreateImageView(&texture.imageView, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		m_Textures[m_TextureIndex] = texture;
		return &m_Textures[m_TextureIndex++];
	}

	// 获取面片数量
	unsigned int GetMeshCount(aiNode* ainode) {
		unsigned int count = ainode->mNumMeshes;
		for (unsigned int i = 0; i < ainode->mNumChildren; i++) {
			count += GetMeshCount(ainode->mChildren[i]);
		}
		return count;
	}

	// 读取节点
	void VkApp::LoadNode(aiNode* ainode, const aiScene* aiscene, vkobj::Model* model) {
		for (uint32_t i = 0; i < ainode->mNumMeshes; i++) {
			aiMesh* aimesh = aiscene->mMeshes[ainode->mMeshes[i]];
			vkobj::Mesh mesh;
			std::vector<vkobj::Vertex> vertices;
			std::vector<uint32_t> indices;
			// 顶点
			for (uint32_t i = 0; i < aimesh->mNumVertices; i++) {
				vkobj::Vertex vertex;
				// 坐标
				vertex.pos = { aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z };
				// 法线
				if (aimesh->HasNormals()) {
					vertex.normal = { aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z };
				}
				// 纹理坐标
				if (aimesh->HasTextureCoords(0)) {
					vertex.texCoord = { aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y }; // 纹理坐标
				}
				// 切线
				if (aimesh->HasTangentsAndBitangents()) {
					vertex.tangent = { aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z }; // 切线
				}
				// 顶点颜色
				if (aimesh->HasVertexColors(i)) {
					vertex.color = { aimesh->mColors[i]->r, aimesh->mColors[i]->g, aimesh->mColors[i]->b };
				}
				else {
					vertex.color = { 1.0f, 1.0f, 1.0f };
				}
				vertices.push_back(vertex);
			}
			mesh.vertices = vertices;
			// 索引
			for (uint32_t i = 0; i < aimesh->mNumFaces; i++) {
				aiFace face = aimesh->mFaces[i];
				for (uint32_t j = 0; j < face.mNumIndices; j++) {
					indices.push_back(face.mIndices[j]);
				}
			}
			mesh.indices = indices;
			// 纹理
			bool hasTexture = false;
			if (aimesh->mMaterialIndex >= 0) {
				aiMaterial* aimat = aiscene->mMaterials[aimesh->mMaterialIndex];
				aiTextureType type = aiTextureType_DIFFUSE; // 暂时只读取一个漫反射纹理
				for (unsigned int i = 0; i < aimat->GetTextureCount(type); i++)
				{
					aiString path;
					aimat->GetTexture(type, i, &path);
					std::cout << "纹理路径:" << path.C_Str() << std::endl;
					mesh.material.texture = GetTexture(path.C_Str());
					hasTexture = true;
					break;
				}

			}
			if(!hasTexture) {
				std::cout << "没有纹理!" << std::endl;
				mesh.material.texture = GetTexture("empty.png");
			}
			model->meshes.push_back(mesh);
		}
		for (uint32_t i = 0; i < ainode->mNumChildren; i++) {
			LoadNode(ainode->mChildren[i], aiscene, model);
		}
	}

	void VkApp::CreateModel(vkobj::Model* pModel) {
		const char* MODEL_PATH = "models/vk_room.obj";
		Assimp::Importer importer;
		const aiScene* aiscene = importer.ReadFile(MODEL_PATH, aiProcess_Triangulate | aiProcess_FlipUVs);
		if (aiscene != nullptr) {
			uint32_t count = GetMeshCount(aiscene->mRootNode);
			std::cout << "Meshes count of model is " << count << std::endl;
			LoadNode(aiscene->mRootNode, aiscene, pModel);
		}
		else {
			std::cout << "could not load model!" << std::endl;
			return;
		}
		// 创建管线和描述符
		for (vkobj::Mesh& mesh : pModel->meshes) {
			// 管线
			mesh.material.vertexShader = "shaders/model.vert.spv";
			mesh.material.fragmentShader = "shaders/model.frag.spv";
			CreateGraphicsPipeline(mesh.material.pipeline, mesh.material.pipelineLayout, mesh.material.vertexShader, mesh.material.fragmentShader);
			// 顶点索引缓冲
			CreateVertexBuffer(mesh.vertices, mesh.vertexBuffer, mesh.vertexBufferMemroy);
			CreateIndexBuffer(mesh.indices, mesh.indexBuffer, mesh.indexBufferMemory);
			// 统一缓冲
			VkDeviceSize size = sizeof(UniformBufferObject);
			CreateBuffer(size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				mesh.material.uniformBuffer,
				mesh.material.uniformBufferMemory);
			// 描述符
			CreateDescriptorSet(mesh.material.descriptorSet, mesh.material.uniformBuffer, mesh.material.texture->imageView);
		}
		// 阴影映射
		CreateBuffer(sizeof(ShadowMVP),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			pModel->uniform, pModel->uniformMemory);
		CreateShadowMapDescriptroSet(pModel->descriptorSet, pModel->uniform);

		pModel->transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.75f));
	}


	void VkApp::DrawModel(vkobj::Model* pModel) {

	}

	void VkApp::DestroyModel(vkobj::Model* pModel) {
		for (vkobj::Mesh& mesh : pModel->meshes) {
			vkDestroyBuffer(m_Device, mesh.vertexBuffer, nullptr);
			vkFreeMemory(m_Device, mesh.vertexBufferMemroy, nullptr);
			vkDestroyBuffer(m_Device, mesh.indexBuffer, nullptr);
			vkFreeMemory(m_Device, mesh.indexBufferMemory, nullptr);
			
			vkDestroyBuffer(m_Device, mesh.material.uniformBuffer, nullptr);
			vkFreeMemory(m_Device, mesh.material.uniformBufferMemory, nullptr);
		}
		vkDestroyBuffer(m_Device, pModel->uniform, nullptr);
		vkFreeMemory(m_Device, pModel->uniformMemory, nullptr);
	}

	void VkApp::InitCamera(vkobj::Camera* pCamera) {
		pCamera->SetTransform({ 1.0f, -2.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
		pCamera->SetPerspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 1.0f, 1000.0f);
		// 深度纹理
		VkFormat format = FindDepthFormat();
		CreateImage(m_SwapChainExtent.width, m_SwapChainExtent.height, format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			pCamera->depthImage, pCamera->depthImageMemory);
		CreateImageView(&pCamera->depthImageView, pCamera->depthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void VkApp::DestroyCamera(vkobj::Camera* pCamera) {
		vkDestroyImage(m_Device, pCamera->depthImage, nullptr);
		vkFreeMemory(m_Device, pCamera->depthImageMemory, nullptr);
		vkDestroyImageView(m_Device, pCamera->depthImageView, nullptr);
	}
	void VkApp::PrepareUniformBuffers() {
		size_t minUboAlignment = m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
		m_DynamicAlignment = sizeof(glm::mat4);
		if (minUboAlignment > 0) {
			m_DynamicAlignment = (m_DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		size_t bufferSize = OBJECT_INSTANCES * m_DynamicAlignment;
		m_DynamicUbo.model = (glm::mat4*)alignedAlloc(bufferSize, m_DynamicAlignment);
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_DynamicUniform, m_DynamicUniformMemory);
	}

	void VkApp::CreateDynamicDescriptorSet(VkDescriptorSet& descriptorSet, VkBuffer dynamicUniform) {
		// 动态统一缓冲

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = dynamicUniform;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(DynamicUBO);
		VkWriteDescriptorSet dynamicdescriptorWrite;
		dynamicdescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		dynamicdescriptorWrite.dstSet = descriptorSet;
		dynamicdescriptorWrite.dstBinding = 1;
		dynamicdescriptorWrite.dstArrayElement = 0;
		dynamicdescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		dynamicdescriptorWrite.descriptorCount = 1;
		dynamicdescriptorWrite.pBufferInfo = &bufferInfo;

	}

	void VkApp::InitLight() {
		// 方向光
		m_DirectLight.color = {1.0f, 1.0f, 1.0f };
		m_DirectLight.dir = {-1.0f, 1.0f, -1.0f };
		m_DirectLight.strength = 1.0;

		// 点光源
		m_PointLight.color = { 0.4f, 0.8f, 0.9f };
		m_PointLight.pos = { 1.0f, -1.0f, 1.0f };
		m_PointLight.strength = 1.0;
		CreateShadowMapPass(&m_PointLight.shadowPass);

		// 聚光灯
		m_SpotLight.color = { 0.9f, 0.9f, 0.6f };
		m_SpotLight.pos = { 1.0f, 1.0f, 1.0f };
		m_SpotLight.dir = { -1.0f, -1.0f, -1.0f };
		m_SpotLight.phi = glm::radians(22.0f);
	}

	void VkApp::DestroyLight() {
		DestroyShadowMapPass(&m_PointLight.shadowPass);
	}

	void VkApp::DestroyShadowMapPass(vkobj::ShadowMapPass* pPass) {
		vkDestroyImage(m_Device, pPass->depthImage, nullptr);
		vkFreeMemory(m_Device, pPass->depthImageMemory, nullptr);
		vkDestroyImageView(m_Device, pPass->depthImageView, nullptr);
		vkDestroyPipeline(m_Device, pPass->pipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, pPass->pipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, pPass->renderPass, nullptr);
		vkDestroySampler(m_Device, pPass->depthSampler, nullptr);
		vkDestroySemaphore(m_Device, pPass->semaphore, nullptr);
		vkDestroyFramebuffer(m_Device, pPass->frameBuffer, nullptr);
	}

	void VkApp::CreateShadowMapPass(vkobj::ShadowMapPass* pPass) {

		pPass->width = m_SwapChainExtent.width;
		pPass->height = m_SwapChainExtent.height;
		// 创建深度纹理
		VkFormat format = FindDepthFormat();
		CreateImage(pPass->width, pPass->height, format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			pPass->depthImage, pPass->depthImageMemory);
		CreateImageView(&pPass->depthImageView, pPass->depthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);

		// 创建通道
		VkRenderPassCreateInfo passInfo{};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		//     Depth attachment (shadow map)
		VkAttachmentDescription attachment = vkinit::AttachmentDescription(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		passInfo.attachmentCount = 1;
		passInfo.pAttachments = &attachment;
		//     Attachment references from subpasses
		VkAttachmentReference depthRef = vkinit::AttachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		//     Subpass 0: shadow map rendering
		VkSubpassDescription subpass = vkinit::SubpassDescription(0, nullptr, &depthRef);
		passInfo.subpassCount = 1;
		passInfo.pSubpasses = &subpass;
		//     Create render pass
		passInfo.pNext = nullptr;
		passInfo.dependencyCount = 0;
		passInfo.pDependencies = nullptr;
		passInfo.flags = 0;

		VK_CHECK(vkCreateRenderPass(m_Device, &passInfo, nullptr, &pPass->renderPass));

		// 创建帧缓冲
		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.pNext = nullptr;
		frameBufferInfo.renderPass = pPass->renderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.pAttachments = &pPass->depthImageView;
		frameBufferInfo.width = pPass->width;
		frameBufferInfo.height = pPass->height;
		frameBufferInfo.layers = 1;
		frameBufferInfo.flags = 0;

		VkFramebuffer shadow_map_fb;
		vkCreateFramebuffer(m_Device, &frameBufferInfo, nullptr, &pPass->frameBuffer);

		// 创建采样器
		CreateTextureSampler(&pPass->depthSampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		// 创建管线
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		//     顶点输入
		VkPipelineVertexInputStateCreateInfo vertexInfo = vkinit::PipelineVertexInputStateCreateInfo(1, &vkobj::Vertex::GetShadowMapBindDescription(), 1, vkobj::Vertex::GetShadowMapAttributeDescriptions().data());
		pipelineInfo.pVertexInputState = &vertexInfo;
		//    Shader
		VkShaderModule vert = vkinit::ShaderModule(m_Device, "shaders/shadowmap.vert.spv");
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = vkinit::PipelineShaderStageCreateInfo(vert, VK_NULL_HANDLE);
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		// 输入组件
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::PipelineInputAssemblyStateCreateInfo();
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		// 视口和裁剪
		VkViewport viewport = vkinit::Viewport((float)pPass->width, float(pPass->height));
		VkRect2D scissor = vkinit::Rect2D(pPass->width, pPass->height);
		VkPipelineViewportStateCreateInfo viewportState = vkinit::PipelineViewportStateCreateInfo(&viewport, &scissor);
		pipelineInfo.pViewportState = &viewportState;
		// 光栅化
		VkPipelineRasterizationStateCreateInfo rasterizer = vkinit::PipelineRasterizationStateCreateInfo();
		pipelineInfo.pRasterizationState = &rasterizer;

		// 重采样
		VkPipelineMultisampleStateCreateInfo multisampling = vkinit::PipelineMultisampleStateCreateInfo();
		pipelineInfo.pMultisampleState = &multisampling;
		// 混合
		VkPipelineColorBlendAttachmentState blendAttachment = vkinit::PipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::PipelineColorBlendStateCreateInfo(1, &blendAttachment);
		pipelineInfo.pColorBlendState = &colorBlending;
		// 动态调整
		VkPipelineDynamicStateCreateInfo dynamicState = vkinit::PipelineDynamicStateCreateInfo();
		pipelineInfo.pDynamicState = nullptr; // Optional
		// 深度缓冲
		VkPipelineDepthStencilStateCreateInfo depthStencil = vkinit::PipelineDepthStencilStateCreateInfo();
		pipelineInfo.pDepthStencilState = &depthStencil;
		// 管线布局
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::PipelineLayoutCreateInfo(1, &m_DescriptorSetLayout);
		// 推送常量
		VkPushConstantRange pushConstant = vkinit::PushConstantRange(0, sizeof(ShadowMVP), VK_SHADER_STAGE_VERTEX_BIT);
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
		VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &pPass->pipelineLayout));
		pipelineInfo.layout = pPass->pipelineLayout;

		pipelineInfo.renderPass = pPass->renderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pPass->pipeline));
		vkDestroyShaderModule(m_Device, vert, nullptr);

		//创建信号量
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &pPass->semaphore);
	}

	void VkApp::CreateShadowMapCommandBuffer(vkobj::ShadowMapPass* pPass) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &pPass->cmdBuffer));

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional
		VK_CHECK(vkBeginCommandBuffer(pPass->cmdBuffer, &beginInfo));

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = pPass->renderPass;
		renderPassInfo.framebuffer = pPass->frameBuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { pPass->width, pPass->height };
		VkClearValue clearValue = {};
		clearValue.depthStencil = { 1.0f, 0 }; // 深度[0, 1]
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(pPass->cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkinit::Viewport((float)pPass->width, (float)pPass->height);
		vkCmdSetViewport(pPass->cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkinit::Rect2D(pPass->width, pPass->height);
		vkCmdSetScissor(pPass->cmdBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(pPass->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPass->pipeline);
		vkCmdBindDescriptorSets(pPass->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPass->pipelineLayout, 0, 1, &m_Model.descriptorSet, 0, nullptr);
		for (vkobj::Mesh& mesh : m_Model.meshes) {
			VkBuffer buffers[] = { mesh.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(pPass->cmdBuffer, 0, 1, buffers, offsets);
			vkCmdBindIndexBuffer(pPass->cmdBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(pPass->cmdBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(pPass->cmdBuffer);
		VK_CHECK(vkEndCommandBuffer(pPass->cmdBuffer));

	}

}