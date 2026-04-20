#include "3D engine.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <vector>
#include <cmath>
#include <iostream>

GLuint compileShader(GLenum type, const char* src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int ok;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(shader, 512, nullptr, log);
    std::cout << "Shader compile error:\n" << log << "\n";
  }

  return shader;
}

GLuint crateProgram(const char* vsSrc, const char* fsSrc) {
  GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  int ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(program, 512, nullptr, log);
    std::cout << "Program link error:\n" << log << "\n";
  }

  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}

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

  static float length(Vec3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  };

  static Vec3 normalize(Vec3 v) {
    float len = length(v);
    if (len == 0) return { 0,0,0 };
    return { v.x / len, v.y / len, v.z / len };
  };

  static Vec3 cross(Vec3 a, Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
  };
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

class InputManager {
private:
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  int dx = 0;
  int dy = 0;

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

    if (event.type == SDL_MOUSEMOTION) {
      dx += event.motion.xrel;
      dy += event.motion.yrel;
    }
  }

  void resetMouse() {
    dx = 0;
    dy = 0;
  }

  bool isForward() const { return forward; };
  bool isBackward() const { return backward; };
  bool isLeft() const { return left; };
  bool isRight() const { return right; };
  int getDX() const { return dx; };
  int getDY() const { return dy; };
};

class Camera {
private:
  Vec3 position;
  Vec3 forward;
  Vec3 up;
  Vec3 right;
  float speed;

  float yaw = -90.0f;
  float pitch = 0.0f;
  float sensitivity = 0.1f;

public:
  Camera(Vec3 pos, Vec3 forw, Vec3 up_, float spd) {
    position = pos;
    forward = Vec3::normalize(forw);
    up = Vec3::normalize(up_);
    right = Vec3::normalize(Vec3::cross(forw, up_));
    speed = spd;
  }

  void cameraUpdate(const InputManager& input, float dt) {
    float step = speed * dt;

    if (input.isForward()) position = position + forward * step;
    if (input.isBackward()) position = position - forward * step;
    if (input.isLeft()) position = position - right * step;
    if (input.isRight()) position = position + right * step;

    yaw += input.getDX() * sensitivity;
    pitch -= input.getDY() * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    float yawRad = yaw * (3.14159265f / 180.0f);
    float pitchRad = pitch * (3.14159265f / 180.0f);

    forward.x = cosf(yawRad) * cosf(pitchRad);
    forward.y = sinf(pitchRad);
    forward.z = sinf(yawRad) * cosf(pitchRad);
    forward = Vec3::normalize(forward);

    right = Vec3::normalize(Vec3::cross(forward, { 0,1,0 }));
    up = Vec3::normalize(Vec3::cross(right, forward));
  }
};




int main(int argc, char* argv[]) {
  Window window;
  if (!window.isValid()) {
    return 1;
  }

  SDL_SetRelativeMouseMode(SDL_TRUE);

  bool running = true;
  SDL_Event event;
  InputManager input;
  Camera camera({ 0,0,0 }, { 0,0,-1 }, { 0,1,0 }, 3.0f);
  float lastTick = SDL_GetTicks();

  while (running) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
        running = false;
      }
      
      input.handleEvent(event);
    }

    float current = SDL_GetTicks();
    float dt = (current - lastTick) / 1000.0f;
    lastTick = current;

    camera.cameraUpdate(input, dt);
    input.resetMouse();
    window.render();
  }

  return 0;
}