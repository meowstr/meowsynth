#include "render.hpp"

#ifdef __EMSCRIPTEN__
#include <GLES2/gl2.h>
#else
#include <glad/glad.h>
#endif

renderstate_t rstate;

// render state
struct {
} intern;

void render( float x )
{
    //glClearColor( 0.2f, 0.0f, 0.0f, 1.0f );
    glClearColor( x, x, x, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
}
