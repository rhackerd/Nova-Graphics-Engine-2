#include "gui.h"

namespace Nova::GE::Render {

    void ImGuiContext::init(Device& device, Swapchain& swapchain, SDL_Window* window, vk::Instance instance, uint32_t graphicsQueueFamily) {
        // ----------------------------------------------------------------
        // Descriptor pool
        // ----------------------------------------------------------------
        VkDescriptorPoolSize poolSize{};
        poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets       = poolSize.descriptorCount;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSize;

        vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &m_pool);

        // ----------------------------------------------------------------
        // ImGui core
        // ----------------------------------------------------------------
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // ----------------------------------------------------------------
        // SDL3 backend
        // ----------------------------------------------------------------
        ImGui_ImplSDL3_InitForVulkan(window);

        // ----------------------------------------------------------------
        // Vulkan backend — dynamic rendering
        // ----------------------------------------------------------------
        m_format = static_cast<VkFormat>(swapchain.getFormat());

        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
        pipelineRenderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingInfo.colorAttachmentCount    = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &m_format;

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance            = instance;
        initInfo.PhysicalDevice      = device.getPhysicalDevice();
        initInfo.Device              = device.getDevice();
        initInfo.QueueFamily         = graphicsQueueFamily;
        initInfo.Queue               = device.getGraphicsQueue();
        initInfo.DescriptorPool      = m_pool;
        initInfo.MinImageCount       = swapchain.getImages().size();
        initInfo.ImageCount          = swapchain.getImages().size();
        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo; // ← changed

        ImGui_ImplVulkan_Init(&initInfo);

        io = ImGui::GetIO();
    }

    void ImGuiContext::shutdown(Device& device) {
        vkDeviceWaitIdle(device.getDevice());
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(device.getDevice(), m_pool, nullptr);
    }

    void ImGuiContext::beginFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }



    void ImGuiContext::endFrame(vk::CommandBuffer cmd) {
        ImGui::Render();

        // ImGui needs its own rendering context
        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment
            .setImageView(view)
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)  // eLoad — don't clear, draw on top
            .setStoreOp(vk::AttachmentStoreOp::eStore);

        vk::RenderingInfo renderingInfo{};
        renderingInfo
            .setRenderArea(vk::Rect2D{{0,0}, {width, height}})
            .setLayerCount(1)
            .setColorAttachments(colorAttachment);

        cmd.beginRendering(renderingInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        cmd.endRendering();
    }

    void ImGuiContext::processEvent(SDL_Event& e) {
        ImGui_ImplSDL3_ProcessEvent(&e);
    }

} // namespace Nova::GE::Render