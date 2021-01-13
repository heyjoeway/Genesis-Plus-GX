#include "video_base.h"

#if defined(GL_LOADER_GLEW)
  #include <GL/glew.h>
#elif defined(GL_LOADER_GLAD)
  #include <glad/glad.h>
#else
  #error No GL_LOADER_x defined.
#endif
#include <GLFW/glfw3.h> // GLFW helper library

#include "shared.h"

const char *shader_src_vert_builtin =
  "attribute vec3 vp;"
  "attribute vec2 vUV;"
  "varying vec2 UV;"
  "void main() {"
    "gl_Position.xyz = vp;"
    "UV = vUV;"
  "}";

const char *shader_src_frag_builtin =
  "varying vec2 UV;"
  "uniform sampler2D tex;"
  "void main(){gl_FragColor = texture2D(tex,UV);}";

float points[12];

const float uv_1p[] = {
   0.0f,  0.0f,
   1.0f,  0.0f,
   1.0f,  0.5f,
   0.0f,  0.5f
};

const float uv_2p[] = {
   0.0f,  0.0f,
   1.0f,  0.0f,
   1.0f,  1.0f,
   0.0f,  1.0f
};

int fullscreen = 0;

GLuint shader, buf_vert, array_vert, buf_uv, tex_screen;
GLFWwindow *window;
void *window_shared;

int Backend_Video_Close() {
	glfwTerminate();
  return 1;
};

char *Load_Text_File(char *path) {
  char * buffer = 0;
  long length;
  FILE * f = fopen(path, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = (char *)malloc(length+1);
    if (buffer) fread(buffer, 1, length, f);
    fclose(f);
    buffer[length] = '\0';
  }

  return buffer;
}

GLuint Shader_Compile(const GLchar *shader_source, GLuint shader_type) {
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_source, NULL);
  glCompileShader(shader);

  int bufflen = 256;
  int success;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufflen);

  GLchar* log_string = new char[bufflen + 1];
  log_string[0] = '\0';
  
  glGetShaderInfoLog(shader, bufflen, 0, log_string);
  printf("Shader Log:\n%s", log_string);

  delete[] log_string;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success != GL_TRUE) { 
      printf("Failed to compile vertex shader.\n");
      return 0;
  }
  return shader;
}

void Adjust_Points() {
  if (!(bitmap.viewport.changed & 1)) return;
  bitmap.viewport.changed &= ~1;

  int screen_width, screen_height,
      rect_dest_w, rect_dest_h,
      rect_dest_x, rect_dest_y;

  glfwGetFramebufferSize(window, &screen_width, &screen_height);

  rect_dest_h = screen_height;

  int video_height = VIDEO_HEIGHT;
  if (interlaced) video_height *= 2;

  float aspect_ratio = VIDEO_WIDTH / (float)video_height;
  rect_dest_w = aspect_ratio * rect_dest_h;

  if (rect_dest_w > screen_width) {
    rect_dest_w = screen_width;
    rect_dest_h = rect_dest_w / aspect_ratio;
  }

  if (option_scaling) { // Integer scaling
    rect_dest_h = (rect_dest_h / video_height) * (float)video_height;
    rect_dest_w = aspect_ratio * rect_dest_h;
  }

  rect_dest_x = (screen_width / 2.0) - (rect_dest_w / 2.0);
  rect_dest_y = (screen_height / 2.0) - (rect_dest_h / 2.0);

  float final_x1 = ((rect_dest_x / (float)screen_width) * 2) - 1;
  float final_x2 = (((rect_dest_x + rect_dest_w) / (float)screen_width) * 2) - 1;
  float final_y1 = ((rect_dest_y / (float)screen_height) * 2) - 1;
  float final_y2 = (((rect_dest_y + rect_dest_h) / (float)screen_height) * 2) - 1;

  points[0] = final_x1;
  points[1] = final_y2;

  points[3] = final_x2;
  points[4] = final_y2;

  points[6] = final_x2;
  points[7] = final_y1;

  points[9] = final_x1;
  points[10] = final_y1;

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, buf_vert);
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, buf_uv);
  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(interlaced ? uv_2p : uv_1p),
    interlaced ? uv_2p : uv_1p,
    GL_STATIC_DRAW
  );
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

int Backend_Video_Present() {
  Adjust_Points();

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  glUseProgram(shader);
  glBindVertexArray(array_vert);
  // draw points 0-3 from the currently bound array_vert with current in-use shader
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  // update other events like input handling 
  glfwPollEvents();
  // put the stuff we've been drawing onto the display
  glfwSwapBuffers(window);

  if (glfwWindowShouldClose(window)) running = 0;

  return 1;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
  bitmap.viewport.changed = 1;
  Backend_Video_Present();
}

int Backend_Video_Init() {
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  } 

  // uncomment these lines if on Apple OS X
  /*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

  window = glfwCreateWindow(
    VIDEO_WIDTH * WINDOW_SCALE,
    VIDEO_HEIGHT * WINDOW_SCALE,
    "Loading...",
    NULL,
    NULL
  );
  window_shared = (void *)window;

  if (!window) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);

  #if defined(GL_LOADER_GLEW)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
  #elif defined(GL_LOADER_GLAD)
    gladLoadGL();
  #endif

  // get version info
  const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte* version = glGetString(GL_VERSION); // version as a string
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);

  bitmap.data = (unsigned char *)malloc(bitmap.pitch * bitmap.height * 2);

  glGenVertexArrays(1, &array_vert);
  glBindVertexArray(array_vert);

  glGenBuffers(1, &buf_vert);
  glGenBuffers(1, &buf_uv);

  char *vs_src = Load_Text_File("vertex.glsl");
  if (vs_src == NULL) vs_src = (char *)shader_src_vert_builtin;
  GLuint vs = Shader_Compile(
    vs_src,
    GL_VERTEX_SHADER
  );

  char *fs_src = Load_Text_File("fragment.glsl");
  if (fs_src == NULL) fs_src = (char *)shader_src_frag_builtin;
  GLuint fs = Shader_Compile(
    fs_src,
    GL_FRAGMENT_SHADER
  );

  shader = glCreateProgram();
  glAttachShader(shader, vs);
  glAttachShader(shader, fs);
  glLinkProgram(shader);

  glGenTextures(1, &tex_screen);
  glBindTexture(GL_TEXTURE_2D, tex_screen);
  glTexImage2D(
    GL_TEXTURE_2D,
    0, // lod
    GL_RGB,
    400, // width
    448, // height (* 2 for multiplayer)
    0, // border (useless)
    GL_BGRA,
    GL_UNSIGNED_BYTE,
    bitmap.data
  );

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  return 1;
};

int windowed_width, windowed_height, windowed_x, windowed_y;

int Backend_Video_SetFullscreen(int arg_fullscreen) {
  if (!fullscreen) {
    glfwGetWindowPos(window, &windowed_x, &windowed_y);
    glfwGetFramebufferSize(window, &windowed_width, &windowed_height);
  }

  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowMonitor(
    window,
    arg_fullscreen ? monitor : NULL,
    arg_fullscreen ? 0 : windowed_x,
    arg_fullscreen ? 0 : windowed_y,
    arg_fullscreen ? mode->width : windowed_width,
    arg_fullscreen ? mode->height : windowed_height,
    arg_fullscreen ? mode->refreshRate : 0
  );
  fullscreen = arg_fullscreen;
  return 1;
};

int Backend_Video_ToggleFullscreen() {
  Backend_Video_SetFullscreen(!fullscreen);
  return 1;
};

int Backend_Video_Update() {
  glBindTexture(GL_TEXTURE_2D, tex_screen);
  glTexSubImage2D(
    GL_TEXTURE_2D,
    0, // lod
    0,
    0,
    400, // width
    448, // height (* 2 for multiplayer)
    GL_BGRA,
    GL_UNSIGNED_BYTE,
    bitmap.data
  );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  return 1;
};

int Backend_Video_Clear() {
  glClear(GL_COLOR_BUFFER_BIT);
  return 1;
};

int Backend_Video_SetWindowTitle(char *caption) {
  glfwSetWindowTitle(window, caption);
  return 1;
};

void *Backend_Video_LoadImage(char *path) {
  return NULL;
};
void *Backend_Video_GetRenderer() {
  return NULL;
};

int Backend_Video_SetVsync(int vsync) { return -1 };
int Backend_Video_GetRefreshRate()  { return -1 };