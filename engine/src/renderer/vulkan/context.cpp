#include "context.h"
#include "debug.h"
#include "extensions.h"

namespace ic
{
    vulkan_context* vulkan_context::s_context = nullptr;

    vulkan_context::~vulkan_context()
    {
        cleanUp();
        if (s_context == this)
        {
            s_context = nullptr;
        }
    }

    vulkan_context::vulkan_context(vulkan_context&& other) noexcept
        : m_initialized(other.m_initialized), m_vk_instance(std::move(other.m_vk_instance)),
          m_surface(std::move(other.m_surface)), m_device(std::move(other.m_device)),
          m_swapchain(std::move(other.m_swapchain)), m_debugMessenger(std::move(other.m_debugMessenger)),
          m_enableValidation(std::move(other.m_enableValidation))
    {
        // steal static context pointer if it points to the other
        if (s_context == &other)
            s_context = this;

        // null out other's pointers so destructor / cleanup won't free them twice
        other.m_vk_instance = VK_NULL_HANDLE;
        other.m_surface = nullptr;
        other.m_device = nullptr;
        other.m_swapchain = nullptr;
        other.m_debugMessenger = VK_NULL_HANDLE;
        other.m_initialized = false;
    }

    vulkan_context& vulkan_context::operator=(vulkan_context&& other) noexcept
    {
        if (this == &other)
            return *this;

        // clean current resources
        cleanUp();

        // move members
        m_initialized = other.m_initialized;
        m_vk_instance = std::move(other.m_vk_instance);
        m_surface = std::move(other.m_surface);
        m_device = std::move(other.m_device);
        m_swapchain = std::move(other.m_swapchain);
        m_debugMessenger = other.m_debugMessenger;
        m_enableValidation = other.m_enableValidation;

        // fix static pointer
        if (s_context == &other)
            s_context = this;

        // null-out other
        other.m_vk_instance = VK_NULL_HANDLE;
        other.m_surface = nullptr;
        other.m_device = nullptr;
        other.m_swapchain = nullptr;
        other.m_debugMessenger = VK_NULL_HANDLE;
        other.m_initialized = false;

        return *this;
    }

    bool vulkan_context::initialize(GLFWwindow* window, bool enableValidation)
    {
        if (m_initialized)
        {
            IC_CORE_WARN("VulkanContext already initialized");
            return true;
        }
        IC_CORE_ASSERT(window != nullptr, "Invalid GLFW window");

        if (!s_context)
        {
            s_context = this;
        }

        m_enableValidation = enableValidation;

        if (!createInstance())
        {
            IC_CORE_ERROR("Failed to create Vulkan Instance!");
            cleanUp();
            return false;
        }

        if (!createSurface(window))
        {
            IC_CORE_ERROR("Failed to create window surface!");
            cleanUp();
            return false;
        }

        if (!createDevice())
        {
            IC_CORE_ERROR("Failed to create device!");
            cleanUp();
            return false;
        }

        if (!createSwapChain())
        {
            IC_CORE_ERROR("Failed to create swap chain!");
            cleanUp();
            return false;
        }

        m_initialized = true;
        IC_CORE_INFO("Initializad Vulkan Context!");
        return true;
    }

    void vulkan_context::cleanUp()
    {
        if (!m_initialized && m_swapchain == nullptr && m_device == nullptr && m_surface == nullptr &&
            m_vk_instance == nullptr)
        {
            // nothing to do
            return;
        }

        // wait for device to be idle before cleanup
        if (m_device)
        {
            m_device->waitIdle();
        }

        if (m_enableValidation && m_debugMessenger != VK_NULL_HANDLE)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*m_vk_instance,
                                                                                    "vkDestroyDebugUtilsMessengerEXT");

            if (func)
            {
                func(*m_vk_instance, m_debugMessenger, nullptr);
            }
            m_debugMessenger = VK_NULL_HANDLE;
        }

        if (m_swapchain)
        {
            // swapchain->destroy() should be safe even if partially constructed
            m_swapchain->destroy();
            m_swapchain = nullptr;
        }

        if (m_device)
        {
            m_device->destroy();
            m_device = nullptr;
        }

        if (m_surface)
        {
            m_surface->destroy(m_vk_instance.get());
            m_surface = nullptr;
        }

        if (m_vk_instance)
        {
            // vkDestroyInstance expects VkInstance, not pointer-to-pointer. We stored a pointer
            // to an allocated VkInstance object (see createInstance()).
            if (*m_vk_instance != VK_NULL_HANDLE)
            {
                vkDestroyInstance(*m_vk_instance, nullptr);
            }
            m_vk_instance = nullptr;
        }

        IC_CORE_INFO("Destroyed Vulkan Context!");
    }

    bool vulkan_context::createInstance()
    {
        if (!m_vk_instance)
        {
            m_vk_instance = std::make_unique<VkInstance>();
        }

        // vulkan app info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan IC Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "IC Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        // instance create info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // setup extensions
        auto extensions = vk::extensions::getRequiredInstanceExtensions(m_enableValidation);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // validatoin layers setup
        if (m_enableValidation && !vk::extensions::checkValidationLayerSupport(m_validationLayers))
        {
            IC_CORE_ERROR("Validation layers requested but not available.");
            return false;
        }

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_enableValidation)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            vk::debug::setupDebugMassenger(m_enableValidation, debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
        VkResult res = vkCreateInstance(&createInfo, nullptr, m_vk_instance.get());
        if (res != VK_SUCCESS)
        {
            IC_CORE_ERROR("Failed to Create Vulkan Instance");
            return false;
        }

        return true;
    }

    bool vulkan_context::createSwapChain()
    {
        m_swapchain = std::make_unique<swapchain>();
        if (!m_swapchain->create(*m_device.get(), *m_surface.get()))
        {
            IC_CORE_ERROR("SwapChain Creation Failed.");
            return false;
        }

        IC_CORE_INFO("Swap chain created successfully!");
        return true;
    }

    void vulkan_context::recreateSwapChain()
    {
        if (!m_initialized || !m_swapchain)
        {
            IC_CORE_ERROR("Cannot recreate swap chain - context not initialized");
            return;
        }

        // wait for device to be idle
        m_device->waitIdle();

        // recreate swap chain
        m_swapchain->recreate();

        IC_CORE_INFO("Swap chain recreated!");
    }

    bool vulkan_context::createSurface(GLFWwindow* window)
    {
        if (!m_surface)
            m_surface = std::make_unique<vulkan_surface>();
        if (!m_surface->create(m_vk_instance.get(), window))
        {
            IC_CORE_ERROR("Surface Creation Failed!");
            return false;
        }

        IC_CORE_INFO("Window surface created successfully");
        return true;
    }

    bool vulkan_context::createDevice()
    {
        if (!m_device)
            m_device = std::make_unique<logical_device>();

        std::unique_ptr<physical_device> physicalDevice = std::make_unique<physical_device>();
        VkSurfaceKHR surface = m_surface->get();
        if (!physicalDevice->select(*m_vk_instance, surface))
        {
            IC_CORE_ERROR("Physical Device Selection Failed!");
            return false;
        }

        if (!m_device->create(std::move(physicalDevice), m_surface->get()))
        {
            IC_CORE_ERROR("Logical Device Creation Failed!");
            return false;
        }

        IC_CORE_INFO("Vulkan Device Creation Successful.");
        return true;
    }

} // namespace ic
