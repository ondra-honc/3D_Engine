#include "3D engine.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <vector>
#include <iostream>

struct Vec3 {
  float x, y, z;

  Vec3 operator+(const Vec3& other) const {
    return { x + other.x, y + other.y, z + other.z };
  }
  
  Vec3 operator-(const Vec3& other) const {
    return { x - other.x, y - other.y, z - other.z };
  }

  Vec3 operator*(float scalar) const {
    return { x * scalar, y * scalar, z * scalar };
  }

  Vec3& operator+=(const Vec3& other) {
    x += other.x; 
    y += other.y;
    z += other.z;
    return *this;
  }

  Vec3& operator-=(const Vec3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  float length(Vec3 v) { return sqrt(float(pow(v.x, 2)) + float(pow(v.y, 2)) + float(pow(v.z, 2))); };
  Vec3 normalize(Vec3 v) { int len = length(v); if (len == 0) return { 0,0,0 }; return { x = v.x / len, y = v.y / len, x = v.z / len }; };
  Vec3 cross(Vec3 a, Vec3 b) { x = a.y * b.z - a.z * b.y; y = a.z * b.x - a.x * b.z; z = a.x * b.y - a.y * b.x; };
};

class Window {
  private: 
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    float backgroundColor[4] = { 0.39f, 0.58f, 0.93f, 1.0f };
  
  public:
    Window() {
      if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
        return;
      }

      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

      window = SDL_CreateWindow("3D Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
      
      if (!window) {
        std::cout << "Window couldn't be created" << SDL_GetError() << "\n";
        SDL_Quit();
        return;
      }

      context = SDL_GL_CreateContext(window);

      if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << "\n";
        SDL_Quit();
        return;
      }

    }
    
    bool isValid() const {
      return window != nullptr && context != nullptr;
    }
    
    void render() {
      glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
      glClear(GL_COLOR_BUFFER_BIT);
      SDL_GL_SwapWindow(window);
    }
      
    ~Window() {
      if (context) SDL_GL_DeleteContext(context);
      if (window) SDL_DestroyWindow(window);
      SDL_Quit();
    }
};

class Camera {
  private:
    Vec3 position;
    Vec3 forward;
    Vec3 up;
    Vec3 right;
    float speed;
 
};


class InputManager {
  private:
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    
  public:
    void handleEvent(const SDL_Event& event) {
      if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        if (event.key.keysym.sym == SDLK_w) forward = true;
        else if (event.key.keysym.sym == SDLK_s) backward = true;
        else if (event.key.keysym.sym == SDLK_a) left = true;
        else if (event.key.keysym.sym == SDLK_d) right = true;
      }

      if (event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == SDLK_w) forward = false;
        else if (event.key.keysym.sym == SDLK_s) backward = false;
        else if (event.key.keysym.sym == SDLK_a) left = false;
        else if (event.key.keysym.sym == SDLK_d) right = false;
      }
    }

    bool isForward() const { return forward; };
    bool isBackward() const { return backward; };
    bool isLeft() const { return left; };
    bool isRight() const { return right; };
};


int main(int argc, char* argv[]) {
  Window window;
  if (!window.isValid()) {
    return 1;
  }

  bool running = true;
  SDL_Event event;
  InputManager input;
  
  while (running) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
        running = false;
      }

      input.handleEvent(event);
    }

    window.render();
  }

  return 0;
}
