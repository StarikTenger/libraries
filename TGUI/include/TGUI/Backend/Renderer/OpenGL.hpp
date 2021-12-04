/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TGUI - Texus' Graphical User Interface
// Copyright (C) 2012-2021 Bruno Van de Velde (vdv_b@tgui.eu)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef TGUI_OPENGL_HPP
#define TGUI_OPENGL_HPP

#include <TGUI/Config.hpp>

#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

// Include all functions from OpenGL 4.6 and OpenGL ES 3.2
#include <TGUI/extlibs/glad/gl.h>

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace tgui
{
#if !defined(NDEBUG)
    #define TGUI_GL_CHECK(expr) do { expr; priv::checkAndLogErrorOpenGL(__FILE__, __LINE__, #expr); } while (false)
#else
    #define TGUI_GL_CHECK(expr) expr
#endif

    namespace priv
    {
        void checkAndLogErrorOpenGL(const char* file, unsigned int line, const char* expression);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // TGUI_OPENGL_HPP
