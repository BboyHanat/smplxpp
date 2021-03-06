#include "meshview/viewer.hpp"

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Eigen/Geometry>

#include "meshview/util.hpp"
#include "meshview/internal/shader_inline.hpp"

#ifdef MESHVIEW_IMGUI
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#endif

namespace {
void error_callback(int error, const char *description) {
    std::cerr << description << "\n";
}

void win_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    meshview::Viewer& viewer = *reinterpret_cast<meshview::Viewer*>(
            glfwGetWindowUserPointer(window));
    if(viewer.on_key && !viewer.on_key(key, (meshview::input::Action)action, mods)) return;

#ifdef MESHVIEW_IMGUI
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

    if (action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_ESCAPE: case 'Q':
                glfwSetWindowShouldClose(window, GL_TRUE); break;
            case 'Z': viewer.camera.reset_view(); break;
            case 'W':
                      viewer.wireframe = !viewer.wireframe; break;
            case 'C':
                      viewer.cull_face = !viewer.cull_face; break;
            case 'A':
                      viewer.draw_axes = !viewer.draw_axes; break;
            case 'M':
                      if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE) {
                          glfwRestoreWindow(window);
                      } else {
                          glfwMaximizeWindow(window);
                      }
                      break;
            case 'F':
                      {
                          int* backup = viewer._fullscreen_backup;
                          if (viewer._fullscreen) {
                              glfwSetWindowMonitor(window, nullptr,
                                      backup[0], backup[1], backup[2], backup[3], 0);
                              viewer._fullscreen = false;
                          } else {
                              glfwGetWindowPos(window, &backup[0],
                                      &backup[1] );
                              glfwGetWindowSize(window, &backup[2],
                                      &backup[3] );
                              const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                              glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(),
                                      0, 0, mode->width, mode->height, 0);
                              viewer._fullscreen = true;
                          }
                      }
                      break;
            case 'H':
                  std::cout <<
R"HELP(Meshview (c) Alex Yu 2020
left click + drag:         rotate view
shift + left click + drag: pan view
middle click + drag:       pan view (alt)
ctrl + left click + drag:  roll view
Z:                         reset view
A:                         toggle axes
W:                         toggle wireframe
C:                         toggle backface culling
M:                         toggle maximize window (may not work on some systems)
F:                         toggle fullscreen window
)HELP";
                      break;
        }
    }
}

void win_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    meshview::Viewer& viewer = *reinterpret_cast<meshview::Viewer*>(
            glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &viewer._mouse_x, &viewer._mouse_y);

    if (action == GLFW_RELEASE) viewer._mouse_button = -1;
    if (viewer.on_mouse_button &&
            !viewer.on_mouse_button(button, (meshview::input::Action)action, mods)) {
        return;
    }
#ifdef MESHVIEW_IMGUI
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;
#endif
    if (action == GLFW_PRESS) {
        viewer._mouse_button = button;
        viewer._mouse_mods = mods;
    }
}

void win_mouse_move_callback(GLFWwindow* window, double x, double y) {
    bool prevent_default = false;

    meshview::Viewer& viewer = *reinterpret_cast<meshview::Viewer*>(
            glfwGetWindowUserPointer(window));
    double prex = viewer._mouse_x, prey = viewer._mouse_y;
    viewer._mouse_x = x, viewer._mouse_y = y;
    if (viewer.on_mouse_move &&
            !viewer.on_mouse_move(x, y)) {
        return;
    }
    if (viewer._mouse_button != -1) {
        if ((viewer._mouse_button == GLFW_MOUSE_BUTTON_LEFT &&
            (viewer._mouse_mods & GLFW_MOD_SHIFT)) ||
             viewer._mouse_button == GLFW_MOUSE_BUTTON_MIDDLE) {
            // Pan
            viewer.camera.pan_with_mouse((float)(x - prex), (float)(y - prey));
        } else if (viewer._mouse_button == GLFW_MOUSE_BUTTON_LEFT &&
            (viewer._mouse_mods & GLFW_MOD_CONTROL)) {
            // Roll
            viewer.camera.roll_with_mouse((float)(x - prex), (float)(y - prey));
        } else if (viewer._mouse_button == GLFW_MOUSE_BUTTON_LEFT) {
            viewer.camera.rotate_with_mouse((float)(x - prex), (float)(y - prey));
        }
    }
}

void win_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    meshview::Viewer& viewer = *reinterpret_cast<meshview::Viewer*>(
            glfwGetWindowUserPointer(window));
    if (viewer.on_scroll &&
            !viewer.on_scroll(xoffset, yoffset)) {
        return;
    }
#ifdef MESHVIEW_IMGUI
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) return;
#endif
    viewer.camera.zoom_with_mouse((float)yoffset);
}

// Window resize
void win_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    meshview::Viewer& viewer = *reinterpret_cast<meshview::Viewer*>(
            glfwGetWindowUserPointer(window));
    viewer.camera.aspect = (float)width / (float)height;
    viewer.camera.update_proj();
    glViewport(0, 0, width, height);
}

// Axes data
const float AXIS_LEN = 0.5f;
const float axes_verts[] =  {
     0.0f, 0.0f, 0.0f,
     AXIS_LEN, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f,
     0.0f, AXIS_LEN, 0.0f,
     0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, AXIS_LEN
};
const float axes_rgb[] =  {
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f
};
}  // namespace

namespace meshview {

Viewer::Viewer() : _fullscreen(false) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::cerr << "GLFW failed to initialize\n";
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    background.setZero();

    ambient_light_color.setConstant(0.2f);
    light_color_diffuse.setConstant(0.8f);
    light_color_specular.setConstant(1.f);
    light_pos << 1.2f, 1.0f, 2.0f;
}

Viewer::~Viewer() {
    glfwTerminate();
}

void Viewer::show() {
    GLFWwindow* window = glfwCreateWindow(_width, _height, "meshview", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "GLFW window creation failed\n";
        return;
    }
    _window = (void*)window;

    camera.aspect = (float)_width / (float)_height;
    camera.update_proj();
    camera.update_view();

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        getchar();
        glfwTerminate();
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDepthFunc(GL_LESS);
    if (cull_face) glEnable(GL_CULL_FACE);

#ifdef MESHVIEW_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    char* glsl_version = NULL;
    ImGui_ImplOpenGL3_Init(glsl_version);
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
#endif

    // Compile shaders
    Shader shader_mesh(MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER);
    Shader shader_pc(POINTCLOUD_VERTEX_SHADER, POINTCLOUD_FRAGMENT_SHADER);

    // Construct axes object
    PointCloud axes(Eigen::template Map<const Points>{axes_verts, 6, 3},
                    Eigen::template Map<const Points>{axes_rgb, 6, 3});
    axes.draw_lines();
    axes.update(true);

    // Events
    glfwSetKeyCallback(window, win_key_callback);
    glfwSetMouseButtonCallback(window, win_mouse_button_callback);
    glfwSetCursorPosCallback(window, win_mouse_move_callback);
    glfwSetScrollCallback(window, win_scroll_callback);
    glfwSetFramebufferSizeCallback(window, win_framebuffer_size_callback);
    glfwSetWindowUserPointer(window, this);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    glfwSetWindowTitle(window, title.c_str());

    if (on_open) on_open();

    // Ask to re-create the buffers + textures in meshes/pointclouds
    for (auto& mesh : meshes) mesh.update(true);
    for (auto& pc : point_clouds) pc.update(true);

    shader_mesh.use();
    while (!glfwWindowShouldClose(window)) {
        glClearColor(background[0], background[1], background[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        if (cull_face)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
        axes.enable(draw_axes);

        shader_pc.use();
        axes.draw(shader_pc, camera);
        for (auto& pc : point_clouds) {
            pc.draw(shader_pc, camera);
        }

        shader_mesh.use();
        shader_mesh.set_vec3("light.ambient", ambient_light_color);
        shader_mesh.set_vec3("light.diffuse", light_color_diffuse);
        shader_mesh.set_vec3("light.specular", light_color_specular);
        shader_mesh.set_vec3("light.position",
                (camera.view.inverse() * light_pos.homogeneous()).head<3>());
        shader_mesh.set_vec3("viewPos", camera.get_pos());
        for (auto& mesh : meshes) {
            mesh.draw(shader_mesh, camera);
        }

        if (on_loop) on_loop();

#ifdef MESHVIEW_IMGUI
        glOrtho(0, _width, _height, 0, 0, 1);

        // feed inputs to dear imgui, start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#endif

        if (on_gui) on_gui();

#ifdef MESHVIEW_IMGUI
        // Render dear imgui into screen
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

        glfwSwapBuffers(window);
        if (loop_wait_events) {
            glfwWaitEvents();
        } else {
            glfwPollEvents();
        }
    }

    if (on_close) on_close();

    for (auto& mesh : meshes) {
        mesh.free_bufs(); // Delete any existing buffers to prevent memory leak
    }

#ifdef MESHVIEW_IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
    _window = nullptr;
    glfwDestroyWindow(window);
}

Mesh& Viewer::add(Mesh&& mesh) {
    meshes.push_back(mesh);
    return meshes.back();
}
Mesh& Viewer::add(const Mesh& mesh) {
    meshes.push_back(mesh);
    return meshes.back();
}

PointCloud& Viewer::add(PointCloud&& point_cloud) {
    point_clouds.push_back(point_cloud);
    return point_clouds.back();
}
PointCloud& Viewer::add(const PointCloud& point_cloud) {
    point_clouds.push_back(point_cloud);
    return point_clouds.back();
}
PointCloud& Viewer::add_line(const Eigen::Ref<const Vector3f>& a,
                     const Eigen::Ref<const Vector3f>& b,
                     const Eigen::Ref<const Vector3f>& color) {
    return add(PointCloud::Line(a, b, color));
}


}  // namespace meshview
