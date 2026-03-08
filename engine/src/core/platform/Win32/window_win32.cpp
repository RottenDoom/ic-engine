#include "core/platform/win32/window_win32.h"
#include <GLFW/glfw3.h>

namespace ic
{

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
        IC_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

std::unique_ptr<Window> Window::create(const window_props& props)
{
        return std::make_unique<win32_window>(props);
}

win32_window::win32_window(const window_props& props)
{
        init(props);
}

win32_window::~win32_window()
{
        shutdown();
}

void win32_window::init(const window_props& props)
{
        m_data.title  = props.title;
        m_data.width  = props.width;
        m_data.height = props.height;

        // TODO: make this API specific
        if (!s_GLFWInitialized)
        {
                int success = glfwInit();
#if IC_ENGINE_USE_VULKAN
                IC_CORE_INFO("Using Vulkan API");
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
#elif IC_ENGINE_USE_OPENGL
                IC_CORE_INFO("Using OpenGL API");
                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(DEBUG) || defined(_DEBUG)
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
#else
#error "No graphics backend defined"
#endif
                IC_CORE_ASSERT(success, "Could not initialize GLFW!");
                glfwSetErrorCallback(GLFWErrorCallback);
                s_GLFWInitialized = true;
        }

        m_Window = glfwCreateWindow((int)props.width, (int)props.height, m_data.title, nullptr, nullptr);

        glfwSetWindowUserPointer(m_Window, &m_data);
        glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
        // setVSync(true);

        // Set GLFW callbacks
        glfwSetWindowSizeCallback(m_Window,
                                  [](GLFWwindow* window, int width, int height)
                                  {
                                          window_data& data = *(window_data*)glfwGetWindowUserPointer(window);
                                          data.width        = width;
                                          data.height       = height;

                                          WindowResizedEvent event(width, height);
                                          data.eventCallback(event);
                                  });

        glfwSetWindowCloseCallback(m_Window,
                                   [](GLFWwindow* window)
                                   {
                                           window_data& data = *(window_data*)glfwGetWindowUserPointer(window);
                                           WindowClosedEvent event;
                                           data.eventCallback(event);
                                   });

        glfwSetKeyCallback(m_Window,
                           [](GLFWwindow* window, int key, int scancode, int action, int mods)
                           {
                                   window_data& data = *(window_data*)glfwGetWindowUserPointer(window);

                                   switch (action)
                                   {
                                   case GLFW_PRESS:
                                   {
                                           KeyPressedEvent event(key, 0);
                                           data.eventCallback(event);
                                           break;
                                   }
                                   case GLFW_RELEASE:
                                   {
                                           KeyReleasedEvent event(key);
                                           data.eventCallback(event);
                                           break;
                                   }
                                   case GLFW_REPEAT:
                                   {
                                           KeyPressedEvent event(key, 1);
                                           data.eventCallback(event);
                                           break;
                                   }
                                   }
                           });

        glfwSetCharCallback(m_Window,
                            [](GLFWwindow* window, unsigned int keycode)
                            {
                                    window_data& data = *(window_data*)glfwGetWindowUserPointer(window);
                                    KeyTypedEvent event(keycode);
                                    data.eventCallback(event);
                            });

        glfwSetMouseButtonCallback(m_Window,
                                   [](GLFWwindow* window, int button, int action, int mods)
                                   {
                                           window_data& data = *(window_data*)glfwGetWindowUserPointer(window);

                                           switch (action)
                                           {
                                           case GLFW_PRESS:
                                           {
                                                   MouseButtonPressedEvent event(button);
                                                   data.eventCallback(event);
                                                   break;
                                           }
                                           case GLFW_RELEASE:
                                           {
                                                   MouseButtonReleasedEvent event(button);
                                                   data.eventCallback(event);
                                                   break;
                                           }
                                           }
                                   });

        glfwSetScrollCallback(m_Window,
                              [](GLFWwindow* window, double xOffset, double yOffset)
                              {
                                      window_data& data = *(window_data*)glfwGetWindowUserPointer(window);
                                      MouseScrolledEvent event((float)xOffset, (float)yOffset);
                                      data.eventCallback(event);
                              });

        glfwSetCursorPosCallback(m_Window,
                                 [](GLFWwindow* window, double xPos, double yPos)
                                 {
                                         window_data& data = *(window_data*)glfwGetWindowUserPointer(window);
                                         MouseMovedEvent event((float)xPos, (float)yPos);
                                         data.eventCallback(event);
                                 });
}

void win32_window::shutdown()
{
        glfwDestroyWindow(m_Window);
        glfwTerminate();
}

void win32_window::onUpdate()
{
        glfwPollEvents();
        // Present the backbuffer so rendered content becomes visible
        // [TODO] Make a function for this since Vulkan doesnt has this
        if (m_Window)
        {
                glfwSwapBuffers(m_Window);
        }
}

void win32_window::setVSync(bool enabled)
{
        if (enabled)
                glfwSwapInterval(1);
        else
                glfwSwapInterval(0);

        m_data.VSync = enabled;
}

bool win32_window::isVSync() const
{
        return m_data.VSync;
}

void win32_window::framebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
        auto w    = reinterpret_cast<window_data*>(glfwGetWindowUserPointer(handle));
        w->width  = width;
        w->height = height;
}

}  // namespace ic
