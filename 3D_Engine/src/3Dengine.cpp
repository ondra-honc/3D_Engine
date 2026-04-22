#include "3D engine.h"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>
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

struct ArrowConfig {
  static constexpr float shaftLen = 0.8f;
  static constexpr float shaftThick = 0.04f;
  static constexpr float tipSize = 0.12f;
  static constexpr float tipHeight = 0.25f;
  static constexpr float halfThick = tipSize / 2.0f;
};

struct GlobalConfig {
  static constexpr int windowWidth = 800;
  static constexpr int windowHeight = 600;
  static constexpr int tip1 = 90;
  static constexpr int tip2 = -90;

  static constexpr float FOV = 90.0f;
  static constexpr float farClipPlane = 100.0f;
  static constexpr float nearClipPlane = 0.1f;
  static constexpr float camSpeed = 3.0f;
  static constexpr float startingYaw = -90.0f;
  static constexpr float mouseSens = 0.1f;
  static constexpr float pitchFlip1 = 89.0f;
  static constexpr float pitchFlip2 = -pitchFlip1;
  static constexpr float panelW = 360.0f;
  static constexpr float panelH = 260.0f;
  static constexpr float speedV = 0.1f;
  static constexpr float stepV = 1.0f;
  static constexpr float halfExtent = 0.5f;
  static constexpr float movementSensitivity = 0.01f;
  static constexpr float highlight = 0.3f;
  static constexpr float outlineColor = 0.05f;
  static constexpr float glLineW = 2.0f;
  static constexpr float pad = 12.0f;


  static constexpr Vec3 floorPos = { 0,-0.1f,0 };
  static constexpr Vec3 floorScale = { 20,0.2f,20 };
  static constexpr Vec3 floorColor = { 0.2f,0.2f,0.2f };

  static constexpr Vec3 palette[] = {
    {1.0f, 0.0f, 0.0f}, // Red
    {0.0f, 1.0f, 0.0f}, // Green
    {0.0f, 0.0f, 1.0f}, // Blue
    {1.0f, 1.0f, 0.0f}, // Yellow
    {1.0f, 0.5f, 0.0f}, // Orange
    {0.5f, 0.0f, 0.5f}, // Purple
    {1.0f, 1.0f, 1.0f}, // White
    {0.1f, 0.1f, 0.1f}  // Dark Gray
  };
};

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

  static mat4 translate(const Vec3& t) {
    mat4 r; 
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
  }

  static mat4 scale(const Vec3& v) {
    mat4 r;
    r.m[0] = v.x;
    r.m[5] = v.y;
    r.m[10] = v.z;
    r.m[15] = 1;
    return r;
  }

  static mat4 rotateX(float angleDeg) {
    float a = angleDeg * (M_PI / 180.0f);
    mat4 r;
    r.m[5] = cosf(a);  
    r.m[6] = sinf(a);   
    r.m[9] = -sinf(a);
    r.m[10] = cosf(a);  
    return r;
  }

  static mat4 rotateY(float angleDeg) {
    float a = angleDeg * (M_PI / 180.0f);
    mat4 r;
    r.m[0] = cosf(a); 
    r.m[2] = -sinf(a);
    r.m[8] = sinf(a);  
    r.m[10] = cosf(a);
    return r;
  }

  static mat4 rotateZ(float angleDeg) {
    float a = angleDeg * (M_PI / 180.0f);
    mat4 r;
    r.m[0] = cosf(a); 
    r.m[1] = sinf(a);
    r.m[4] = -sinf(a); 
    r.m[5] = cosf(a);
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

    window = SDL_CreateWindow("3D Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GlobalConfig::windowWidth, GlobalConfig::windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);

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
  bool up = false;
  bool down = false;
  int dx = 0;
  int dy = 0;

public:
  void handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
      if (event.key.keysym.sym == SDLK_w) forward = true;
      else if (event.key.keysym.sym == SDLK_s) backward = true;
      else if (event.key.keysym.sym == SDLK_a) left = true;
      else if (event.key.keysym.sym == SDLK_d) right = true;
      else if (event.key.keysym.sym == SDLK_SPACE) up = true;
      else if (event.key.keysym.sym == SDLK_LCTRL) down = true;
      else if (event.key.keysym.sym == SDLK_RCTRL) down = true;
    }

    if (event.type == SDL_KEYUP) {
      if (event.key.keysym.sym == SDLK_w) forward = false;
      else if (event.key.keysym.sym == SDLK_s) backward = false;
      else if (event.key.keysym.sym == SDLK_a) left = false;
      else if (event.key.keysym.sym == SDLK_d) right = false;
      else if (event.key.keysym.sym == SDLK_SPACE) up = false;
      else if (event.key.keysym.sym == SDLK_LCTRL) down = false;
      else if (event.key.keysym.sym == SDLK_RCTRL) down = false;
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
  bool isUp() const { return up; };
  bool isDown() const { return down; };
};

class Camera {
private:
  Vec3 position;
  Vec3 forward;
  Vec3 up;
  Vec3 right;
  float speed;
  Vec3 worldUp = { 0,1,0 };

  float yaw = GlobalConfig::startingYaw;
  float pitch = 0.0f;
  float sensitivity = GlobalConfig::mouseSens;

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
    if (input.isUp()) position = position + worldUp * step;
    if (input.isDown()) position = position - worldUp * step;

    yaw += input.getDX() * sensitivity;
    pitch -= input.getDY() * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    float yawRad = yaw * (M_PI / 180.0f);
    float pitchRad = pitch * (M_PI / 180.0f);

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
  Vec3 getRight() const { return right; };
};

enum ShapeType {
  cube,
  sphere,
  rectangle,
};

enum Axis {
  NONE,
  X,
  Y,
  Z,
};

class Object {
  public:
    int id;
    ShapeType shape;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    Vec3 color;
};

std::vector<Object> objects;
std::vector<int> selectedIDs;

Object* selectedObject = nullptr;
Axis activeAxis = NONE;

bool editorMode = true;

int nextID = 0;

bool rayIntersectAABB(Vec3 rayOrigin, Vec3 rayDir, Vec3 boxMin, Vec3 boxMax, float& tOut) {
  Vec3 invDir = { 1.0f / rayDir.x, 1.0f / rayDir.y, 1.0f / rayDir.z };

  float t1 = (boxMin.x - rayOrigin.x) * invDir.x;
  float t2 = (boxMax.x - rayOrigin.x) * invDir.x;
  float tmin = std::min(t1, t2);
  float tmax = std::max(t1, t2);

  t1 = (boxMin.y - rayOrigin.y) * invDir.y;
  t2 = (boxMax.y - rayOrigin.y) * invDir.y;
  tmin = std::max(tmin, std::min(t1, t2));
  tmax = std::min(tmax, std::max(t1, t2));

  t1 = (boxMin.z - rayOrigin.z) * invDir.z;
  t2 = (boxMax.z - rayOrigin.z) * invDir.z;
  tmin = std::max(tmin, std::min(t1, t2));
  tmax = std::min(tmax, std::max(t1, t2));

  if (tmax >= tmin && tmax >= 0.0f) {
    tOut = tmin > 0.0f ? tmin : tmax;
    return true;
  }
  return false;
}

int main(int argc, char* argv[]) {
  bool running = true;
  SDL_Event event;
  InputManager input;
  Camera camera({ 0,1.5f,3 }, { 0,0,-1 }, { 0,1,0 }, GlobalConfig::camSpeed);
  Window window;
  Vec3 floorPos = { 0,-0.1,0 };
  Vec3 floorSize = { 20,0.2,20 };
  Vec3 floorColor = { 0.2f, 0.2f, 0.2f };
  
  if (!window.isValid()) {
    return 1;
  }

  SDL_SetRelativeMouseMode(editorMode ? SDL_FALSE : SDL_TRUE);
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
  ImGui_ImplOpenGL3_Init("#version 330");
  
  int width = 0, height = 0;
  SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &width, &height);
  glViewport(0, 0, width, height);

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
  uniform vec3 uColor;
  void main() {
    FragColor = vec4(uColor, 1.0);
  }
  )";

  GLuint program = crateProgram(vsSrc, fsSrc);
  GLint uMVPLoc = glGetUniformLocation(program, "uMVP");
  GLint uColorLoc = glGetUniformLocation(program, "uColor");
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  float pyramidVerts[] = {
    // Base (Two triangles)
    -0.5f, 0.0f, -0.5f,  0.5f, 0.0f, -0.5f,  0.5f, 0.0f,  0.5f,
     0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f, -0.5f,
     // Sides
     -0.5f, 0.0f, -0.5f,  0.5f, 0.0f, -0.5f,  0.0f, 1.0f,  0.0f,
      0.5f, 0.0f, -0.5f,  0.5f, 0.0f,  0.5f,  0.0f, 1.0f,  0.0f,
      0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.5f,  0.0f, 1.0f,  0.0f,
     -0.5f, 0.0f,  0.5f, -0.5f, 0.0f, -0.5f,  0.0f, 1.0f,  0.0f
  };
  
  float verts[] = {
    // back face (-Z)
    -0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,   0.5f,-0.5f,-0.5f,
     0.5f, 0.5f,-0.5f,  -0.5f,-0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,

     // front face (+Z)
     -0.5f,-0.5f, 0.5f,   0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,  -0.5f,-0.5f, 0.5f,

      // left face (-X)
      -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,  -0.5f,-0.5f,-0.5f,
      -0.5f,-0.5f,-0.5f,  -0.5f,-0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,

      // right face (+X)
       0.5f, 0.5f, 0.5f,   0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,
       0.5f,-0.5f,-0.5f,   0.5f, 0.5f, 0.5f,   0.5f,-0.5f, 0.5f,

       // bottom face (-Y)
       -0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,   0.5f,-0.5f, 0.5f,
        0.5f,-0.5f, 0.5f,  -0.5f,-0.5f, 0.5f,  -0.5f,-0.5f,-0.5f,

        // top face (+Y)
        -0.5f, 0.5f,-0.5f,   0.5f, 0.5f, 0.5f,   0.5f, 0.5f,-0.5f,
         0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,  -0.5f, 0.5f, 0.5f
  };

  GLuint pyramidVao, pyramidVbo;
  glGenVertexArrays(1, &pyramidVao);
  glGenBuffers(1, &pyramidVbo);
  glBindVertexArray(pyramidVao);
  glBindBuffer(GL_ARRAY_BUFFER, pyramidVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVerts), pyramidVerts, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

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

  float lastTick = SDL_GetTicks();

  while (running) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
        running = false;
      }
      
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        width = event.window.data1;
        height = event.window.data2;
        glViewport(0, 0, width, height);
      }

      if (event.type == SDL_KEYDOWN && event.key.repeat == 0 && event.key.keysym.sym == SDLK_TAB) {
        editorMode = !editorMode;
        SDL_SetRelativeMouseMode(editorMode ? SDL_FALSE : SDL_TRUE);
      }
      
      ImGui_ImplSDL2_ProcessEvent(&event);
      
      if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (!ImGui::GetIO().WantCaptureMouse && editorMode) {
          int mouseX, mouseY;
          SDL_GetMouseState(&mouseX, &mouseY);

          // Ray Calculation
          float aspect = (float)width / (float)height;
          float fovRad = GlobalConfig::FOV * (M_PI / 180.0f);
          float halfTan = tanf(fovRad / 2.0f);
          float px = (2.0f * mouseX / width - 1.0f) * aspect * halfTan;
          float py = (1.0f - 2.0f * mouseY / height) * halfTan;
          Vec3 rayDir = Vec3::normalize(camera.getForward() + camera.getRight() * px + camera.getUp() * py);
          Vec3 rayOrigin = camera.getPosition();

          if (selectedObject != nullptr) {
            float t;
            Vec3 pos = selectedObject->position;

            const float shaftLen = ArrowConfig::shaftLen;
            const float tipHeight = ArrowConfig::tipHeight;
            const float totalLen = shaftLen + tipHeight;
            const float hitboxThickness = ArrowConfig::tipSize;
            const float halfThick = ArrowConfig::halfThick;

            // X-Axis Hitbox
            Vec3 xMin = pos + Vec3{ 0.0f, -halfThick, -halfThick };
            Vec3 xMax = pos + Vec3{ totalLen, halfThick, halfThick };

            // Y-Axis Hitbox
            Vec3 yMin = pos + Vec3{ -halfThick, 0.0f, -halfThick };
            Vec3 yMax = pos + Vec3{ halfThick, totalLen, halfThick };

            // Z-Axis Hitbox
            Vec3 zMin = pos + Vec3{ -halfThick, -halfThick, 0.0f };
            Vec3 zMax = pos + Vec3{ halfThick, halfThick, totalLen };

            if (rayIntersectAABB(rayOrigin, rayDir, xMin, xMax, t)) activeAxis = X;
            else if (rayIntersectAABB(rayOrigin, rayDir, yMin, yMax, t)) activeAxis = Y;
            else if (rayIntersectAABB(rayOrigin, rayDir, zMin, zMax, t)) activeAxis = Z;

            if (activeAxis != NONE) goto skipSelection;
          }

          float closestDist = INFINITY;
          int hitIndex = -1;
          for (int i = 0; i < (int)objects.size(); i++) {
            Vec3 boxMin = objects[i].position - (objects[i].scale * 0.5f);
            Vec3 boxMax = objects[i].position + (objects[i].scale * 0.5f);
            float hitDist;
            if (rayIntersectAABB(rayOrigin, rayDir, boxMin, boxMax, hitDist)) {
              if (hitDist < closestDist) { closestDist = hitDist; hitIndex = i; }
            }
          }

          if (hitIndex != -1) {
            selectedObject = &objects[hitIndex];
            if (!(SDL_GetModState() & KMOD_CTRL)) selectedIDs.clear();
            selectedIDs.push_back(objects[hitIndex].id);
          }
          else if (!(SDL_GetModState() & KMOD_CTRL)) {
            selectedObject = nullptr;
            selectedIDs.clear();
          }

        skipSelection:;
        }
      }

      if (event.type == SDL_MOUSEBUTTONUP) {
        activeAxis = NONE;
      }

      if (event.type == SDL_MOUSEMOTION && activeAxis != NONE) {
        float sensitivity = GlobalConfig::movementSensitivity;
        float dx = (float)event.motion.xrel * sensitivity;
        float dy = (float)-event.motion.yrel * sensitivity; 

        Vec3 delta = { 0, 0, 0 };
        if (activeAxis == X) delta.x = dx + dy; 
        if (activeAxis == Y) delta.y = dy;
        if (activeAxis == Z) delta.z = -(dx + dy);

        if (selectedObject) {
          selectedObject->position += delta;
          for (auto& obj : objects) {
            bool isMultiSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
            if (isMultiSel && &obj != selectedObject) {
              obj.position += delta;
            }
          }
        }
      }
      
      if (!editorMode) {
        input.handleEvent(event);
      }
    }

    float current = SDL_GetTicks();
    float dt = (current - lastTick) / 1000.0f;
    lastTick = current;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(GlobalConfig::pad, GlobalConfig::pad), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(GlobalConfig::panelW, GlobalConfig::panelH), ImGuiCond_Always);

    ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Editor", nullptr, flags);
    
    ImGui::Text("TAB = toggle Editor/Camera");
    ImGui::Text("Mode: %s", editorMode ? "Editor (mouse free)" : "Camera (mouse locked)");

    if (ImGui::Button("Add Cube")) {
      int currentIndex = -1;
      if (selectedObject != nullptr) {
        currentIndex = (int)(selectedObject - &objects[0]);
      }

      Object obj;
      obj.id = nextID;
      obj.shape = cube;
      obj.position = { (float)objects.size() * 1.0f, GlobalConfig::halfExtent, 0.0f };
      obj.rotation = { 0,0,0 };
      obj.scale = { 1,1,1 };
      obj.color = { 0.9f, 0.7f, 0.2f };
      objects.push_back(obj);
      nextID++;

      if (currentIndex != -1) {
        selectedObject = &objects[currentIndex];
      }
      else {
        selectedObject = &objects.back();
      }
    }

    if (!objects.empty()) {
      if (selectedObject != nullptr) {
        ImGui::Separator();
        ImGui::Text("Transform");

        // --- POSITION ---
        Vec3 oldPos = selectedObject->position;
        if (ImGui::DragFloat3("Position", &selectedObject->position.x, 0.1f)) {
          Vec3 delta = selectedObject->position - oldPos;
          for (auto& obj : objects) {
            bool isSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
            if (isSel && &obj != selectedObject) {
              obj.position += delta;
            }
          }
        }

        // --- ROTATION ---
        Vec3 oldRot = selectedObject->rotation;
        if (ImGui::DragFloat3("Rotation", &selectedObject->rotation.x, 1.0f)) {
          Vec3 delta = selectedObject->rotation - oldRot;
          for (auto& obj : objects) {
            bool isSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
            if (isSel && &obj != selectedObject) {
              obj.rotation += delta;
            }
          }
        }

        // --- SCALE ---
        Vec3 oldScale = selectedObject->scale;
        if (ImGui::DragFloat3("Scale", &selectedObject->scale.x, 0.1f)) {
          Vec3 delta = selectedObject->scale - oldScale;
          for (auto& obj : objects) {
            bool isSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
            if (isSel && &obj != selectedObject) {
              obj.scale += delta;
            }
          }
        }

        // --- COLOR ---
        if (ImGui::ColorEdit3("Color", &selectedObject->color.x)) {
          for (auto& obj : objects) {
            bool isSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
            if (isSel && &obj != selectedObject) {
              obj.color = selectedObject->color;
            }
          }
        }
        
        int paletteSize = sizeof(GlobalConfig::palette) / sizeof(GlobalConfig::palette[0]);
        ImGui::Text("Quick Palette:");
        for (int n = 0; n < paletteSize; n++) {
          ImGui::PushID(n);

          if (ImGui::ColorButton("##palette", ImVec4(GlobalConfig::palette[n].x, GlobalConfig::palette[n].y, GlobalConfig::palette[n].z, 1.0f))) {
            selectedObject->color = GlobalConfig::palette[n];

            for (auto& obj : objects) {
              bool isSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
              if (isSel) {
                obj.color = GlobalConfig::palette[n];
              }
            }
          }

          if ((n + 1) % 8 != 0) ImGui::SameLine();

          ImGui::PopID();
        }

        if (ImGui::Button("Delete Selected")) {
          if (!selectedIDs.empty()) {
            objects.erase(
              std::remove_if(objects.begin(), objects.end(), [&](const Object& obj) {
                return std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();
                }),
              objects.end()
            );

            selectedIDs.clear();
            selectedObject = nullptr;
          }
        }
      }
    }


    ImGui::Text("Objects: %d", (int)objects.size());
    ImGui::Text("FPS: %.1f", (dt > 0.0f) ? (1.0f / dt) : 0.0f);
    ImGui::Text("Position: %.2f %.2f %.2f", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
    ImGui::End();
    
    if (!editorMode) {
      camera.cameraUpdate(input, dt);
    }
    input.resetMouse();
 
    mat4 projection = mat4::perspective(
      GlobalConfig::FOV,
      (float)width / (float)height,
      GlobalConfig::nearClipPlane,
      GlobalConfig::farClipPlane
    );
    mat4 view = mat4::lookAt(camera.getPosition(), camera.getPosition() + camera.getForward(), camera.getUp());

    glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);

    mat4 T = mat4::translate(floorPos);
    mat4 S = mat4::scale(floorSize);

    mat4 floorModel = mat4::multiplyMat4Mat4(T, S);
    mat4 vp = mat4::multiplyMat4Mat4(projection, view);
    mat4 floorMVP = mat4::multiplyMat4Mat4(vp, floorModel);

    glUniform3f(uColorLoc, floorColor.x, floorColor.y, floorColor.z);
    glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, floorMVP.m);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // --- PASS 1: normal cubes ---
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);

    for (const auto& obj : objects) {
      if (obj.shape != cube) continue;

      mat4 T = mat4::translate(obj.position);
      mat4 S = mat4::scale(obj.scale);
      mat4 model = mat4::multiplyMat4Mat4(T, S);
      mat4 mvpObj = mat4::multiplyMat4Mat4(vp, model);

      Vec3 renderColor = obj.color;

      bool isSel = std::find(selectedIDs.begin(), selectedIDs.end(), obj.id) != selectedIDs.end();

      if (isSel) {
        renderColor.x = std::min(1.0f, renderColor.x + GlobalConfig::highlight);
        renderColor.y = std::min(1.0f, renderColor.y + GlobalConfig::highlight);
        renderColor.z = std::min(1.0f, renderColor.z + GlobalConfig::highlight);
      }
      
      glUniform3f(uColorLoc, renderColor.x, renderColor.y, renderColor.z);
      glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, mvpObj.m);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // --- PASS 2: wireframe outline overlay ---
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(GlobalConfig::glLineW);

    for (const auto& obj : objects) {
      if (obj.shape != cube) continue;

      mat4 T = mat4::translate(obj.position);
      mat4 S = mat4::scale(obj.scale);
      mat4 model = mat4::multiplyMat4Mat4(T, S);
      mat4 mvpObj = mat4::multiplyMat4Mat4(vp, model);

      glUniform3f(uColorLoc, GlobalConfig::outlineColor, GlobalConfig::outlineColor, GlobalConfig::outlineColor);
      glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, mvpObj.m);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    if (selectedObject != nullptr) {
      glDisable(GL_DEPTH_TEST);
      Vec3 pos = selectedObject->position;

      float shaftLen = ArrowConfig::shaftLen;  
      float shaftThick = ArrowConfig::shaftThick; 
      float tipSize = ArrowConfig::tipSize;   
      float tipHeight = ArrowConfig::tipHeight;  

      auto drawAxis = [&](Vec3 color, mat4 rotation, Vec3 direction) {
        glUniform3f(uColorLoc, color.x, color.y, color.z);

        glBindVertexArray(vao);
        mat4 sScale = mat4::scale({ shaftThick, shaftLen, shaftThick });
        mat4 sModel = mat4::multiplyMat4Mat4(mat4::translate(pos + direction * (shaftLen * GlobalConfig::halfExtent)),
          mat4::multiplyMat4Mat4(rotation, sScale));
        glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, mat4::multiplyMat4Mat4(vp, sModel).m);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(pyramidVao);
        mat4 tScale = mat4::scale({ tipSize, tipHeight, tipSize });
        mat4 tModel = mat4::multiplyMat4Mat4(mat4::translate(pos + direction * shaftLen),
          mat4::multiplyMat4Mat4(rotation, tScale));
        glUniformMatrix4fv(uMVPLoc, 1, GL_FALSE, mat4::multiplyMat4Mat4(vp, tModel).m);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        };

      drawAxis({ 1, 0, 0 }, mat4::rotateZ(GlobalConfig::tip2), { 1, 0, 0 });
      drawAxis({ 0, 1, 0 }, mat4(), { 0, 1, 0 });
      drawAxis({ 0, 0, 1 }, mat4::rotateX(GlobalConfig::tip1), { 0, 0, 1 });

      glEnable(GL_DEPTH_TEST);
    }

    // restore
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(SDL_GL_GetCurrentWindow());
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  return 0;
}