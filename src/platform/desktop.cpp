#include "hardware.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "logging.hpp"

static struct {
    GLFWwindow * window = nullptr;
    int width = 1200;
    int height = 800;

    int pending_event_list[ 64 ];
    int pending_event_count = 0;

} intern;

static void push_event( int e )
{
    if ( intern.pending_event_count >= 64 ) {
        ERROR_LOG( "input overflow" );
    }

    intern.pending_event_list[ intern.pending_event_count ] = e;
    intern.pending_event_count++;
}

static void
handle_mouse_button( GLFWwindow * window, int button, int action, int mods )
{
    if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ) {
        push_event( EVENT_TOUCH );
    }
}

static void
handle_key( GLFWwindow * window, int key, int scancode, int action, int mods )
{
    if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS ) {
        glfwSetWindowShouldClose( window, GLFW_TRUE );
    }
    if ( key == GLFW_KEY_SPACE && action == GLFW_PRESS ) {
        push_event( EVENT_JUMP );
    }
    if ( key == GLFW_KEY_H && action == GLFW_PRESS ) {
        push_event( EVENT_FAST_HAMMER_CW );
    }
    if ( key == GLFW_KEY_L && action == GLFW_PRESS ) {
        push_event( EVENT_FAST_HAMMER_CCW );
    }
    if ( key == GLFW_KEY_F1 && action == GLFW_PRESS ) {
        push_event( EVENT_F1 );
    }
    if ( key == GLFW_KEY_F2 && action == GLFW_PRESS ) {
        push_event( EVENT_F2 );
    }
    if ( key == GLFW_KEY_F3 && action == GLFW_PRESS ) {
        push_event( EVENT_F3 );
    }
}

int hardware_init()
{
    if ( !glfwInit() ) {
        ERROR_LOG( "failed to initialize glfw" );
        return 1;
    }

    // glfwWindowHint( GLFW_CLIENT_API, GLFW_OPENGL_ES_API );
    // glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
    // glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
    // glfwWindowHint( GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API );

    glfwWindowHintString( GLFW_X11_CLASS_NAME, "floater" );
    glfwWindowHintString( GLFW_X11_INSTANCE_NAME, "floater" );
    glfwWindowHint( GLFW_RESIZABLE, 0 );

    intern.window = glfwCreateWindow(
        intern.width,
        intern.height,
        "don't",
        nullptr,
        nullptr
    );

    if ( !intern.window ) {
        ERROR_LOG( "failed to create window" );
        return 1;
    }

    glfwMakeContextCurrent( intern.window );

    if ( !gladLoadGLLoader( (GLADloadproc) glfwGetProcAddress ) ) {
        ERROR_LOG( "failed to load OpenGL" );
        return 1;
    }

    const char * gl_version = (const char *) glGetString( GL_VERSION );
    const char * vendor = (const char *) glGetString( GL_VENDOR );
    const char * renderer = (const char *) glGetString( GL_RENDERER );
    INFO_LOG( "opengl version: %s", gl_version );
    INFO_LOG( "opengl vendor: %s", vendor );
    INFO_LOG( "opengl renderer: %s", renderer );

    glfwSwapInterval( 1 );
    glViewport( 0, 0, intern.width, intern.height );

    glfwSetMouseButtonCallback( intern.window, handle_mouse_button );
    glfwSetKeyCallback( intern.window, handle_key );

    return 0;
}

void hardware_destroy()
{
    glfwDestroyWindow( intern.window );
    glfwTerminate();
}

using loop_function_t = void ( * )();

void hardware_set_loop( loop_function_t step )
{
    while ( !glfwWindowShouldClose( intern.window ) ) {
        intern.pending_event_count = 0;
        glfwPollEvents();

        if ( glfwGetKey( intern.window, GLFW_KEY_J ) == GLFW_PRESS ) {
            push_event( EVENT_HAMMER_CW );
        }
        if ( glfwGetKey( intern.window, GLFW_KEY_K ) == GLFW_PRESS ) {
            push_event( EVENT_HAMMER_CCW );
        }

        step();
        glfwSwapBuffers( intern.window );
    }
}

int hardware_width()
{
    return intern.width;
}

int hardware_height()
{
    return intern.height;
}

float hardware_time()
{
    return glfwGetTime();
}

void hardware_rumble()
{
}

int * hardware_events( int * out_count )
{
    *out_count = intern.pending_event_count;
    return intern.pending_event_list;
}

float hardware_x_axis()
{
    float dx = 0.0f;

    if ( glfwGetKey( intern.window, GLFW_KEY_A ) == GLFW_PRESS ) {
        dx = -1.0f;
    }

    if ( glfwGetKey( intern.window, GLFW_KEY_D ) == GLFW_PRESS ) {
        dx = 1.0f;
    }

    return dx;
}

float hardware_y_axis()
{
    float dy = 0.0f;

    if ( glfwGetKey( intern.window, GLFW_KEY_W ) == GLFW_PRESS ) {
        dy = -1.0f;
    }

    if ( glfwGetKey( intern.window, GLFW_KEY_S ) == GLFW_PRESS ) {
        dy = 1.0f;
    }

    return dy;
}

float hardware_touch_x()
{
    double x, y;
    glfwGetCursorPos( intern.window, &x, &y );
    return (float) x;
}

float hardware_touch_y()
{
    double x, y;
    glfwGetCursorPos( intern.window, &x, &y );
    return (float) y;
}

int hardware_touch_is_down()
{
    return glfwGetMouseButton( intern.window, GLFW_MOUSE_BUTTON_LEFT );
}

int hardware_number()
{
    if ( glfwGetKey( intern.window, GLFW_KEY_1 ) == GLFW_PRESS ) return 1;
    if ( glfwGetKey( intern.window, GLFW_KEY_2 ) == GLFW_PRESS ) return 2;
    if ( glfwGetKey( intern.window, GLFW_KEY_3 ) == GLFW_PRESS ) return 3;
    if ( glfwGetKey( intern.window, GLFW_KEY_4 ) == GLFW_PRESS ) return 4;
    if ( glfwGetKey( intern.window, GLFW_KEY_5 ) == GLFW_PRESS ) return 5;
    if ( glfwGetKey( intern.window, GLFW_KEY_6 ) == GLFW_PRESS ) return 6;
    if ( glfwGetKey( intern.window, GLFW_KEY_7 ) == GLFW_PRESS ) return 7;
    if ( glfwGetKey( intern.window, GLFW_KEY_8 ) == GLFW_PRESS ) return 8;
    if ( glfwGetKey( intern.window, GLFW_KEY_9 ) == GLFW_PRESS ) return 9;
    if ( glfwGetKey( intern.window, GLFW_KEY_0 ) == GLFW_PRESS ) return 10;

    return 0;
}
