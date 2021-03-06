/*
 * Copyright 2011-2013 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ARX_GRAPHICS_OPENGL_OPENGLUTIL_H
#define ARX_GRAPHICS_OPENGL_OPENGLUTIL_H

#include "platform/Platform.h"

#include <GL/glew.h>

#include <io/log/Logger.h>

const char * getGLErrorString(GLenum error);

#ifdef ARX_DEBUG
#define CHECK_GL if(GLenum error = glGetError()) \
	LogError << "GL error in " << __func__ << ": " << error << " = " << getGLErrorString(error)
#else
#define CHECK_GL
#endif

#endif // ARX_GRAPHICS_OPENGL_OPENGLUTIL_H
