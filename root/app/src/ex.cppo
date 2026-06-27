#include "commandBuffer.h"
#include "cpuf.h"
#include "device.h"
#include "gui.h"
#include "imgui.h"
#include "layout.h"
#include "mesh.h"
#include "pipeline.h"
#include "swapchain.h"
#include "system.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/structs.hpp>
#include <Nova/Desktop/core.h>
#include <Nova/Desktop/window.hpp>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <cstdlib>
#include <fmt/format.h>
#include <stdio.h>
#include <dlfcn.h>
#include <filesystem>
#include <chrono>
#include <string_view>
#include <thread>
#include <core.h>
#include <vector>
#include <vulkan/vulkan_core.h>

int main() {
    // ── Platform ──────────────────────────────────────────────────────
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    if (desktop) {
        std::string d(desktop);
        std::transform(d.begin(), d.end(), d.begin(), ::tolower);
        if (d.find("gnome") != std::string::npos) {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
            printf("Detected gnome, using x11 video driver\n");
        }
    }

    // ── Core ──────────────────────────────────────────────────────────
    Nova::Desktop::CreateInfo::Man manCI = Nova::Desktop::CreateInfo::Man::Builder().build();
    Nova::Desktop::Man man;
    if (!man.init(manCI)) return 0;

    Nova::GE::CI::System systemCI = Nova::GE::CI::System::Builder()
        .setAppName("MyEngine")
        .setAppVersion(VK_MAKE_VERSION(1, 0, 0))
        .addExtensions(man.getExtensions())
        .enableValidation()
        .addRecommendedValidationFeatures()
        .build();
    Nova::GE::System system;
    if (!system.init(systemCI)) { system.shutdown(); return 0; }

    Nova::Desktop::CreateInfo::Window windowCI = Nova::Desktop::CreateInfo::Window::Builder()
        .setSize({800, 600})
        .setResizable()
        .build();
    Nova::Desktop::Window window;
    window.create(windowCI);
    window.createSurface(system.getInstance());

    // ── Device + Swapchain ────────────────────────────────────────────
    Nova::GE::CreateInfo::Device deviceCI = Nova::GE::CreateInfo::Device::Builder()
        .setSystem(&system)
        .pickPhysicalDevice()
        .enableBindless()
        .setSurface(window.getSurface())
        .enableDefaultExt()
        .enableDescriptorBuffer()
        .build();
    Nova::GE::Device device;
    if (!device.init(deviceCI)) { device.shutdown(); system.shutdown(); return 0; }

    Nova::GE::CreateInfo::Swapchain swapCI = Nova::GE::CreateInfo::Swapchain::Builder()
        .assignWindow(window)
        .setDevice(device)
        .setExtent({800, 600})
        .build();
    Nova::GE::Swapchain swapchain;
    swapchain.init(swapCI);

    // ── Pipeline ──────────────────────────────────────────────────────
    auto vertex = device.createShader("shaders/vert.spv", vk::ShaderStageFlagBits::eVertex);
    auto frag   = device.createShader("shaders/frag.spv", vk::ShaderStageFlagBits::eFragment);

    Nova::GE::ShaderSet cameraSet;
    cameraSet.addUniformBuffer(0, vk::ShaderStageFlagBits::eVertex);

    Nova::GE::ShaderSet texSet;
    texSet.addSampler(0, vk::ShaderStageFlagBits::eFragment)
          .addSampledImage(1, vk::ShaderStageFlagBits::eFragment);

    auto cameraSetLayout = cameraSet.bake(device.getDevice(), device.getDld());
    auto texSetLayout    = texSet.bake(device.getDevice(), device.getDld());

    Nova::GE::ShaderLayout shaderLayout;
    shaderLayout.addPushConstant(sizeof(Nova::Core::Mat4), vk::ShaderStageFlagBits::eVertex);
    auto bakedLayout = shaderLayout.bake({cameraSetLayout, texSetLayout});

    Nova::GE::CreateInfo::Pipeline pipelineCI = Nova::GE::CreateInfo::Pipeline::Builder()
        .prepareDefault()
        .addShader(vertex)
        .addShader(frag)
        .setLayout(bakedLayout)
        .setSwapchainFormat(swapchain.getFormat())
        .setDepthFormat(swapchain.getDepthFormat())
        .build();
    auto pipeline = device.createPipeline(pipelineCI);

    // ── Scene ─────────────────────────────────────────────────────────
    Nova::GE::Mesh mesh;
    mesh.init("aircraft.obj", device.getAllocator());

    Nova::GE::Camera camera;
    camera.init(device.getAllocator(), device.getDescriptorManager());
    camera.initDescriptor(device.getDescriptorManager(), cameraSetLayout);
    camera.getPosition() = {0.0f, 20.0f, -5.0f};
    camera.setAspect(swapchain.getExtent().width / (float)swapchain.getExtent().height);
    camera.setPerspective(60.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    camera.update();

    auto tex = device.createTexture("aircraft.png");
    tex.lock()->initDescriptor(device.getDescriptorManager(), texSetLayout);

    Nova::Core::Mat4 objectPos;

    // ── Render infra ──────────────────────────────────────────────────
    vk::FenceCreateInfo fenceCI{};
    fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);
    vk::Fence fence = device.getDevice().createFence(fenceCI);

    u32 imageCount = swapchain.getImages().size();
    std::vector<vk::Semaphore> renderFinishedSems(imageCount);
    for (auto& s : renderFinishedSems)
        s = device.getDevice().createSemaphore({});

    Nova::GE::Render::CreateInfo::CPUF cpufCI{};
    cpufCI.device = &device;
    Nova::GE::Render::CPUF cpuf;
    cpuf.init(cpufCI);
    cpuf.setClearColor({0.1f, 0.1f, 0.1f});

    Nova::GE::Render::ImGuiContext gui;
    gui.init(device, swapchain, &window.get(), system.getInstance(),
             device.getIndices().graphicsFamily.value());

    Nova::GE::Render::RenderTarget renderTarget = Nova::GE::Render::createRenderTarget(swapchain);
    Nova::GE::Render::CBSlot* lastSlot = nullptr;

    // ── Input state ───────────────────────────────────────────────────
    float yaw = 0.0f, pitch = 0.0f;
    float camSpeed    = 0.01f;
    float sensitivity = 0.001f;
    bool  moving      = true;
    bool  running     = true;
    bool  resized     = false;

    NOVA_SUPPRESS_INTERNAL_BEGIN
    SDL_HideCursor();
    SDL_SetWindowRelativeMouseMode(&window.get(), true);

    // ── Loop ──────────────────────────────────────────────────────────
    while (running) {
        // Events
        SDL_Event e;
        SDL_PumpEvents();
        while (SDL_PollEvent(&e)) {
            gui.processEvent(e);
            if (e.type == SDL_EVENT_QUIT)           running = false;
            if (e.type == SDL_EVENT_WINDOW_RESIZED) resized = true;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                SDL_SetWindowRelativeMouseMode(&window.get(), true);
                SDL_HideCursor();
                moving = true;
            }
        }

        // Input
        ImGuiIO& io = ImGui::GetIO();
        const bool* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_ESCAPE]) {
            SDL_SetWindowRelativeMouseMode(&window.get(), false);
            SDL_ShowCursor();
            moving = false;
        }

        if (!io.WantCaptureKeyboard && moving) {
            Nova::Core::Vec3 forward = camera.getForward(); forward[1] = 0.0f; forward.normalize();
            Nova::Core::Vec3 right   = camera.getRight();   right[1]   = 0.0f; right.normalize();
            Nova::Core::Vec3 delta   = {0, 0, 0};

            if (keys[SDL_SCANCODE_W]) delta += forward *  camSpeed;
            if (keys[SDL_SCANCODE_S]) delta += forward * -camSpeed;
            if (keys[SDL_SCANCODE_A]) delta += right   * -camSpeed;
            if (keys[SDL_SCANCODE_D]) delta += right   *  camSpeed;
            if (keys[SDL_SCANCODE_E]) delta[1] +=  camSpeed;
            if (keys[SDL_SCANCODE_Q]) delta[1] -= camSpeed;
            camera.move(delta);
        }

        if (!io.WantCaptureMouse && moving) {
            float dx, dy;
            SDL_GetRelativeMouseState(&dx, &dy);
            yaw   -= dx * sensitivity;
            pitch -= dy * sensitivity;
            pitch  = std::clamp(pitch, -1.5f, 1.5f);
            camera.setRotation(
                Nova::Core::Quat::fromAxisAngle({0, 1, 0}, yaw) *
                Nova::Core::Quat::fromAxisAngle({1, 0, 0}, pitch));
        }

        // Frame start
        device.getDevice().waitForFences(fence, true, UINT64_MAX);
        if (lastSlot) lastSlot->inUse = false;

        vk::Semaphore imageAvailable = swapchain.getImageAvailableSemaphore();
        if (!swapchain.advanceFrame() || resized) {
            resized = false;
            camera.setAspect(swapchain.getExtent().width / (float)swapchain.getExtent().height);
            continue;
        }

        camera.update();
        device.getDevice().resetFences(fence);
        Nova::GE::Render::updateRenderTarget(renderTarget);

        // ImGui + Render
        gui.beginFrame();
        gui.updateTarget(swapchain.getCurrentImageView(), renderTarget.extent.x(), renderTarget.extent.y());
        lastSlot = cpuf.batchSubmit(renderTarget,
            {
                [&](Nova::GE::Render::Cmd cmd) {
                    cmd.setSwapchain(swapchain);
                    cmd.bindDescriptorBuffer();
                    cmd.bindPipeline(pipeline.lock());
                    cmd.bindSet(0, camera.getHandle(), pipeline.lock()->getLayout());
                    cmd.bindSet(1, tex.lock()->getHandle(), pipeline.lock()->getLayout());
                    cmd.pushConstant(objectPos, pipeline.lock()->getLayout());
                    cmd.drawMesh(mesh);

                    cmd.pushConstant(Nova::Core::Mat4(), pipeline.lock()->getLayout());
                    cmd.drawMesh(mesh);
                }
            },
            [&](vk::CommandBuffer primaryCmd) {
                ImGui::Begin("Camera");
                float pos[3] = { camera.getPosition().x(), camera.getPosition().y(), camera.getPosition().z() };
                if (ImGui::DragFloat3("Position", pos, 0.1f))
                    camera.setPosition({pos[0], pos[1], pos[2]});
                ImGui::DragFloat("Speed",       &camSpeed,    0.001f, 0.001f, 1.0f);
                ImGui::DragFloat("Sensitivity", &sensitivity, 0.0001f, 0.0001f, 0.01f);
                ImGui::DragFloat("Yaw",         &yaw,         0.01f);
                ImGui::DragFloat("Pitch",       &pitch,       0.01f, -1.5f, 1.5f);
                static float fov = 60.0f;
                if (ImGui::DragFloat("FOV", &fov, 0.5f, 10.0f, 120.0f))
                    camera.setPerspective(fov, 800.0f / 600.0f, 0.1f, 1000.0f);
                ImGui::Separator();
                ImGui::Text("Forward: %.2f %.2f %.2f",
                    camera.getForward().x(), camera.getForward().y(), camera.getForward().z());
                ImGui::End();

                ImGui::Begin("Object");
                static float objPos[3]   = {0, 0, 0};
                static float objRot[3]   = {0, 0, 0};
                static float objScale[3] = {1, 1, 1};
                bool dirty = false;
                dirty |= ImGui::DragFloat3("Position", objPos,   0.1f);
                dirty |= ImGui::DragFloat3("Rotation", objRot,   0.5f);
                dirty |= ImGui::DragFloat3("Scale",    objScale, 0.01f, 0.01f, 100.0f);
                if (dirty) {
                    objectPos =
                        Nova::Core::Mat4::translation({objPos[0], objPos[1], objPos[2]})
                      * Nova::Core::Mat4::rotation(objRot[1] * (3.14159f/180.f), {0,1,0})
                      * Nova::Core::Mat4::rotation(objRot[0] * (3.14159f/180.f), {1,0,0})
                      * Nova::Core::Mat4::rotation(objRot[2] * (3.14159f/180.f), {0,0,1})
                      * Nova::Core::Mat4::scale({objScale[0], objScale[1], objScale[2]});
                }
                ImGui::End();

                gui.endFrame(primaryCmd);
            }
        );

        // Submit + Present
        vk::Semaphore renderFinished = renderFinishedSems[swapchain.getImageIndex()];
        vk::CommandBuffer cb = lastSlot->cmd->getCommandBuffer();

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo{};
        submitInfo.setWaitSemaphores(imageAvailable)
                  .setWaitDstStageMask(waitStage)
                  .setCommandBuffers(cb)
                  .setSignalSemaphores(renderFinished);
        device.getGraphicsQueue().submit(submitInfo, fence);

        vk::SwapchainKHR sc  = swapchain.getSwapchain();
        uint32_t         idx = swapchain.getImageIndex();
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphores(renderFinished).setSwapchainCount(1);
        presentInfo.pSwapchains   = &sc;
        presentInfo.pImageIndices = &idx;
        device.getPresentQueue().presentKHR(presentInfo);
    }
    NOVA_SUPPRESS_INTERNAL_END

    // ── Cleanup ───────────────────────────────────────────────────────
    device.getDevice().waitIdle();
    gui.shutdown(device);
    camera.shutdown();
    mesh.shutdown();
    for (auto& s : renderFinishedSems)
        device.getDevice().destroySemaphore(s);
    device.getDevice().destroyFence(fence);
    swapchain.shutdown();
    window.destroy(system.getInstance());
    device.shutdown();
    system.shutdown();
    man.shutdown();
    return 0;
}