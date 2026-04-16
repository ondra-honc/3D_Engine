#include "3D engine.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>


int main(int argc, char* argv[]) {
  // 1. Initialize SDL Video
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    return -1;
  }

  // 2. Set OpenGL version (3.3 Core is the standard for learning)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // 3. Create Window
  SDL_Window* window = SDL_CreateWindow("3D Engine Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (!window) {
    std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return -1;
  }

  // 4. Create OpenGL Context (The bridge to the GPU)
  SDL_GLContext context = SDL_GL_CreateContext(window);

  // 5. Load GLAD (Crucial: Must happen AFTER context creation)
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // 6. The "Game Loop" (Wait for X button)
  bool running = true;
  SDL_Event e;
  while (running) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) running = false;
    }

    // Set background to a nice "Cornflower Blue"
    glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
