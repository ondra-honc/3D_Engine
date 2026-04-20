#include "3D engine.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <vector>
#include <cmath>
#include <math.h>
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

struct Vec4 {
  float x, y, z, w;
};

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

  static float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }
};

struct mat4 {
  float m[16] = {
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
  };

  mat4() {
    m[0] = m[5] = m[10] = m[15] = 1;
  }

  static mat4 perspective(float fovDegrees, float aspect, float near, float far) {
    float fov = fovDegrees * (M_PI / 180);

    mat4 r;
    for (size_t i = 0; i < 16; i++) r.m[i] = 0;

    float f = 1 / tan(fov / 2);

    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1;
    r.m[14] = (2 * far * near) / (near - far);

    return r;
  }

  static mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    mat4 r;
    for (size_t i = 0; i < 16; i++) r.m[i] = 0;

    Vec3 f = Vec3::normalize(center - eye);
    Vec3 s = Vec3::normalize(Vec3::cross(f, up));
    Vec3 u = Vec3::cross(s, f);

    r.m[0] = s.x;
    r.m[1] = u.x;
    r.m[2] = -f.x;
    
    r.m[4] = s.y;
    r.m[5] = u.y;
    r.m[6] = -f.y;

    r.m[8] = s.z;
    r.m[9] = u.z;
    r.m[10] = -f.z;

    r.m[12] = -Vec3::dot(s, eye);
    r.m[13] = -Vec3::dot(u, eye);
    r.m[14] = Vec3::dot(f, eye);

    r.m[15] = 1;

    return r;
  }

  static mat4 multiplyMat4Mat4(const mat4& a, const mat4& b) {
    mat4 C;
    for (int i = 0; i < 16; i++) C.m[i] = 0;

    for (int c = 0; c < 4; c++) {
      for (int r = 0; r < 4; r++) {
        for (int k = 0; k < 4; k++) {
          C.m[c * 4 + r] += a.m[k * 4 + r] * b.m[c * 4 + k];
        }
      }
    }
    return C;
  }

  static Vec4 multiplyMat4Vec4(const mat4& M, const Vec4& v) {
    Vec4 r;
    r.x = M.m[0] * v.x + M.m[4] * v.y + M.m[8] * v.z + M.m[12] * v.w;
    r.y = M.m[1] * v.x + M.m[5] * v.y + M.m[9] * v.z + M.m[13] * v.w;
    r.z = M.m[2] * v.x + M.m[6] * v.y + M.m[10] * v.z + M.m[14] * v.w;
    r.w = M.m[3] * v.x + M.m[7] * v.y + M.m[11] * v.z + M.m[15] * v.w;

    return r;
  }
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

  Vec3 getPosition() const { return position; };
  Vec3 getForward() const { return forward; };
  Vec3 getUp() const { return up; };
};




int main(int argc, char* argv[]) {
  Window window;
  if (!window.isValid()) {
    return 1;
  }

  const char* vsSrc = R"(
  #version 330 core
  layout (location = 0) in vec3 aPos;
  uniform mat4 uMVP;
  void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
  }
  )";

  const char* fsSrc = R"(
  #version 330 core
  out vec4 FragColor;
  void main() {
    FragColor = vec4(1.0, 0.8, 0.2, 1.0);
  }
  )";

  GLuint program = crateProgram(vsSrc, fsSrc);
  GLint uMVPLoc = glGetUniformLocation(program, "uMVP");

  float verts[] = {
    -0.5f, -0.5f, -2.0f,
     0.5f, -0.5f, -2.0f,
     0.0f,  0.5f, -2.0f
  };

  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);

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

    mat4 projection = mat4::perspective(90, 800.0f / 600.0f, 0.1f, 100.0f);
    mat4 view = mat4::lookAt(camera.getPosition(), camera.getPosition() + camera.getForward(), camera.getUp());
    mat4 mvp = mat4::multiplyMat4Mat4(projection, view);

    glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, mvp.m);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(SDL_GL_GetCurrentWindow());
  }

  return 0;
}