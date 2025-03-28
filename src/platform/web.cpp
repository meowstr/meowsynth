#include "hardware.hpp"

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

// #include <emscripten.h>

extern "C" {
typedef void ( *em_callback_func )( void );
void emscripten_set_main_loop(
    em_callback_func func,
    int fps,
    bool simulate_infinite_loop
);
}

#include "logging.hpp"

using loop_function_t = void ( * )();

static struct {
    GLFWwindow * window = nullptr;
    int width = 1200;
    int height = 800;

    int pending_event_list[ 64 ];
    int pending_event_count = 0;

    loop_function_t step;

    int touch_state;
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
}

int hardware_init()
{
    if ( !glfwInit() ) {
        ERROR_LOG( "failed to initialize glfw" );
        return 1;
    }

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

    // glfwSetWindowSizeCallback( intern.window, handle_resize );

    // glfwSetMouseButtonCallback( intern.window, handle_mouse_button );
    glfwSetKeyCallback( intern.window, handle_key );

    glfwSetWindowSize( intern.window, intern.width, intern.height );
    glViewport( 0, 0, intern.width, intern.height );

    return 0;
}

void hardware_destroy()
{
    glfwDestroyWindow( intern.window );
    glfwTerminate();
}

static void loop()
{
    intern.pending_event_count = 0;
    glfwPollEvents();

    if ( glfwGetKey( intern.window, GLFW_KEY_J ) == GLFW_PRESS ) {
        push_event( EVENT_HAMMER_CW );
    }
    if ( glfwGetKey( intern.window, GLFW_KEY_K ) == GLFW_PRESS ) {
        push_event( EVENT_HAMMER_CCW );
    }

    static int last1 = 0;
    static int last2 = 0;
    static int last3 = 0;

    if ( glfwGetKey( intern.window, GLFW_KEY_SPACE ) == GLFW_PRESS ) {
        if ( !last1 ) {
            push_event( EVENT_JUMP );
        }
        last1 = 1;
    } else
        last1 = 0;

    if ( glfwGetKey( intern.window, GLFW_KEY_H ) == GLFW_PRESS ) {
        if ( !last2 ) {
            push_event( EVENT_FAST_HAMMER_CW );
        }
        last2 = 1;
    } else
        last2 = 0;

    if ( glfwGetKey( intern.window, GLFW_KEY_L ) == GLFW_PRESS ) {
        if ( !last3 ) {
            push_event( EVENT_FAST_HAMMER_CCW );
        }
        last3 = 1;
    } else
        last3 = 0;

    int touch_state =
        glfwGetMouseButton( intern.window, GLFW_MOUSE_BUTTON_LEFT );

    if ( touch_state && !intern.touch_state ) {
        push_event( EVENT_TOUCH );
    }

    intern.touch_state = touch_state;

    intern.step();
}

void hardware_set_loop( loop_function_t step )
{
    intern.step = step;
    emscripten_set_main_loop( loop, 0, 1 );
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
    return intern.touch_state;
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
