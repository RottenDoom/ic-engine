#include "logical_device.h"

// TODO: implement better queue selection

namespace ic
{
    logical_device::~logical_device() {}

    logical_device::logical_device(logical_device&& other) noexcept
    {
        moveFrom(std::move(other));
    }

    logical_device& logical_device::operator=(logical_device&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            moveFrom(std::move(other));
        }
        return *this;
    }

    bool logical_device::create(std::unique_ptr<physical_device> physicalDevice, VkSurfaceKHR surface)
    {
        if (m_device != VK_NULL_HANDLE)
        {
            IC_CORE_WARN("Logical device already created");
            return true;
        }

        if (!physicalDevice->get() || surface == VK_NULL_HANDLE)
        {
            IC_CORE_ERROR("Invalid physical device or surface for logical device creation");
            return false;
        }

        // store the physical device (move it)
        m_physicalDevice = std::move(physicalDevice);

        if (!createLogicalDevice(surface))
        {
            IC_CORE_ERROR("Failed to create logical device");
            return false;
        }

        // initialize queue manager with the created device and queue family indices
        m_queueManager = new queue_manager();
        const auto& queueFamilyIndices = m_physicalDevice->getQueueFamilyIndices();
        if (!m_queueManager->initialize(m_device, queueFamilyIndices))
        {
            IC_CORE_ERROR("Failed to initialize queue manager");
            destroy();
            return false;
        }

        IC_CORE_INFO("Logical device created successfully");
        IC_CORE_INFO("Device name: {}", m_physicalDevice->getProperties().deviceName);

        return true;
    }

    void logical_device::destroy()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            // wait for device to be idle before destroying
            waitIdle();

            // cleanup queue manager first
            // m_queueManager->cleanup();
            delete m_queueManager;

            // Destroy the logical device
            vkDestroyDevice(m_device, nullptr);

            reset();
            IC_CORE_INFO("Logical device destroyed");
        }
    }

    bool logical_device::createLogicalDevice(VkSurfaceKHR surface)
    {
        const auto& queueFamilyIndices = m_physicalDevice->getQueueFamilyIndices();

        // create queue create infos for unique queue families
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = queueFamilyIndices.getUniqueQueueFamilies();

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // get required features from physical device
        VkPhysicalDeviceFeatures deviceFeatures = m_physicalDevice->getRequirements().requiredFeatures;

        // create logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        // extensions
        auto extensions = getRequiredExtensionPtrs();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // TODO: setup validation layers here.
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;

        VkResult result = vkCreateDevice(m_physicalDevice->get(), &createInfo, nullptr, &m_device);

        if (result != VK_SUCCESS)
        {
            IC_CORE_ERROR("Failed to create logical device! VkResult: {}", static_cast<int>(result));
            return false;
        }

        IC_CORE_INFO("Logical device created with {} queue families", uniqueQueueFamilies.size());

        // log enabled extensions
        IC_CORE_INFO("Enabled device extensions:");
        for (const char* extension : extensions)
        {
            IC_CORE_INFO("  - {}", extension);
        }

        // log enabled features
        IC_CORE_INFO("Enabled device features:");
        if (deviceFeatures.samplerAnisotropy)
            IC_CORE_INFO("  - Sampler Anisotropy");
        if (deviceFeatures.fillModeNonSolid)
            IC_CORE_INFO("  - Fill Mode Non Solid");
        if (deviceFeatures.wideLines)
            IC_CORE_INFO("  - Wide Lines");
        if (deviceFeatures.geometryShader)
            IC_CORE_INFO("  - Geometry Shader");
        if (deviceFeatures.tessellationShader)
            IC_CORE_INFO("  - Tessellation Shader");
        // add more feature logging as needed...

        return true;
    }

    void logical_device::waitIdle() const
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
        }
    }

    std::vector<const char*> logical_device::getRequiredExtensionPtrs() const
    {
        const auto& requirements = m_physicalDevice->getRequirements();
        return requirements.requiredExtensions;
    }

    void logical_device::moveFrom(logical_device&& other) noexcept
    {
        m_device = other.m_device;
        m_physicalDevice = std::move(other.m_physicalDevice);
        m_queueManager = other.m_queueManager;

        other.reset();
    }

    void logical_device::reset() noexcept
    {
        m_device = VK_NULL_HANDLE;
        m_physicalDevice.reset();
        m_queueManager = nullptr;
    }

} // namespace ic
