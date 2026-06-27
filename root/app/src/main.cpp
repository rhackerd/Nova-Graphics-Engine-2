#include "high-level/graphics.h"
#include "high-level/object.h"
#include "high-level/renderer.h"
#include <Nova/Desktop/core.h>
#include <Nova/Desktop/window.hpp>
#include <SDL3/SDL_events.h>
#include <vector>

int main() {
    #ifdef __linux__
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    if (desktop) {
        std::string d(desktop);
        std::transform(d.begin(), d.end(), d.begin(), ::tolower);
        if (d.find("gnome") != std::string::npos) {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
            printf("Detected gnome, using x11 video driver\n");
        }
    }
    #endif

    Nova::Desktop::CreateInfo::Man manCI = Nova::Desktop::CreateInfo::Man::Builder()
        .enableVideo()
        .enableEvents()
        .enableGamepad()
        .build();

    Nova::Desktop::Man man;
    if (!man.init(manCI)) return 1;

    Nova::Desktop::CreateInfo::Window windowCI = Nova::Desktop::CreateInfo::Window::Builder()
        .setTitle("Nova Graphics Testing window")
        .setSize({800, 600})
        .setResizable()
        .build();
    Nova::Desktop::Window win;
    win.create(windowCI);

    Nova::Graphics::Graphics gfx;
    if(!gfx.init(man, win)) return 1;


    // Renderer
    Nova::Graphics::Renderer renderer(gfx);

    // Setup scene
    auto camera = renderer.createCamera();
    auto _object = renderer.createObject();
    auto object = _object.lock();
    object->loadMesh("aircraft.obj");
    object->loadTexture("aircraft.png");

    camera.lock()->setPosition({0,20,-5.0f});
    camera.lock()->setPerspective(60.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
    camera.lock()->setAspect(800.0f / 600.0f);

    camera.lock()->setPosition({0, 5, 10.0f});  // closer, behind the object
    camera.lock()->setTarget({0, 0, 0});         // look at origin
    camera.lock()->setPerspective(60.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

    auto scene = Nova::Graphics::Scene{};
    scene.camera = camera;
    scene.objects.push_back(_object);

    scene.objects.push_back(_object);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }else if(e.type == SDL_EVENT_WINDOW_RESIZED) {
                printf("Window was resized\n");
                auto swap = renderer.getSwapchain();
                swap->handleRecreation();   
                camera.lock()->setAspect(swap->getExtent().width / (float)swap->getExtent().height);
            }
        }

        // camera.lock()->move({0,0.001,0});

        renderer.step(scene);
    };

    renderer.shutdown();
    gfx.shutdown();
    man.shutdown();

    
    return 0;
};