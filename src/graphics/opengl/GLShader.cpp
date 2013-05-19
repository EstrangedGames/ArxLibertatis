/*
 * Copyright 2011-2012 Arx Libertatis Team (see the AUTHORS file)
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

#include "GLShader.h"

static bool checkShader(GLuint object, const char * op, GLuint check) {

	GLint status;
	glGetObjectParameterivARB(object, check, &status);
	if(!status) {
		int logLength;
		glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
		char * log = new char[logLength];
		glGetInfoLogARB(object, logLength, NULL, log);
		LogWarning << "Failed to " << op << " vertex shader: " << log;
		delete[] log;
		return false;
	}

	return true;
}

static const char nopVertexShaderSource[] =
	"attribute vec2 v_coord; \n"
	"uniform sampler2D fbo_texture; \n"
	"varying vec2 f_texcoord; \n"
	" \n"
	"void main(void) { \n"
	"	gl_Position = vec4(v_coord, 0.0, 1.0); \n"
	"	f_texcoord = (v_coord + 1.0) / 2.0; \n"
	"} \n";

static const char nopFragmentShaderSource[] =
	"uniform sampler2D fbo_texture; \n"
	"varying vec2 f_texcoord; \n"
	" \n"
	"void main(void) { \n"
	"	gl_FragColor = texture2D(fbo_texture, f_texcoord); \n"
	"} \n";

GLShader::GLShader()
{
	const char * vertSource = nopVertexShaderSource;
	const char * fragSource = nopFragmentShaderSource;

	GLuint shader = glCreateProgramObjectARB();
	if(!shader) {
		LogWarning << "Failed to create program object";
		return;
	}

	GLuint obj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	if(!obj) {
		LogWarning << "Failed to create shader object";
		glDeleteObjectARB(shader);
		return;
	}

	GLuint fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	if(!fragObj) {
		LogWarning << "Failed to create shader object";
		glDeleteObjectARB(shader);
		return;
	}

	glShaderSourceARB(obj, 1, &vertSource, NULL);
	glCompileShaderARB(obj);
	if(!checkShader(obj, "compile", GL_OBJECT_COMPILE_STATUS_ARB)) {
		glDeleteObjectARB(obj);
		glDeleteObjectARB(shader);
		return;
	}

	glShaderSourceARB(fragObj, 1, &fragSource, NULL);
	glCompileShaderARB(fragObj);
	if(!checkShader(fragObj, "compile", GL_OBJECT_COMPILE_STATUS_ARB)) {
		glDeleteObjectARB(fragObj);
		glDeleteObjectARB(shader);
		return;
	}

	glAttachObjectARB(shader, obj);
	glAttachObjectARB(shader, fragObj);
	//glDeleteObjectARB(fragObj);

	glLinkProgramARB(shader);
	if(!checkShader(shader, "link", GL_OBJECT_LINK_STATUS_ARB)) {
		glDeleteObjectARB(shader);
		return;
	}

	const char* attribute_name;
	const char* uniform_name;

	attribute_name = "v_coord";
	attribute_v_coord_postproc = glGetAttribLocation(shader, attribute_name);
	if(attribute_v_coord_postproc == -1) {
		LogWarning << "Could not bind attribute: " << attribute_name;
	}

	uniform_name = "fbo_texture";
	uniform_fbo_texture = glGetUniformLocation(shader, uniform_name);
	if(uniform_fbo_texture == -1) {
		LogWarning << "Could not bind uniform: " << uniform_name;
	}

	m_program = shader;
}

GLShader::~GLShader()
{
	glDeleteProgram(m_program);
}
