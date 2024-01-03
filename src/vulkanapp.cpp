#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_image.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

namespace ManageSDL{
    void SDLHandleEvents(bool& isRunning) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
        }
    }
}

class VulkanApp {
public:
    void run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        CleanUp();
    }
private:
    SDL_Window* window;

    void InitWindow() {
        SDL_Init(SDL_INIT_VIDEO);

        Uint32 WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
        window = SDL_CreateWindow("Vulkan App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, WindowFlags);
    }
    void InitVulkan() {

    }
    
    void MainLoop() {
        bool isRunning = true;

        while (isRunning) {
            ManageSDL::SDLHandleEvents(isRunning);
        }
    }

    void CleanUp() {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[])
{
    VulkanApp app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return EXIT_SUCCESS;
}