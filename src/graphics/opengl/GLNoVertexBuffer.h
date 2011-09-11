
#ifndef ARX_GRAPHICS_OPENGL_GLNOVERTEXBUFFER_H
#define ARX_GRAPHICS_OPENGL_GLNOVERTEXBUFFER_H

#include "graphics/VertexBuffer.h"
#include "graphics/Vertex.h"
#include "graphics/opengl/OpenGLRenderer.h"
#include "graphics/opengl/OpenGLUtil.h"

template <class Vertex>
static void renderVertex(const Vertex & vertex);

template <>
void renderVertex(const TexturedVertex & vertex) {
	
	Color c = Color::fromBGRA(vertex.color);
	glColor4ub(c.r, c.g, c.b, c.a);
	
	glMultiTexCoord2f(GL_TEXTURE0, vertex.uv.x, vertex.uv.y);
	
	GLfloat w = 1.0f / vertex.rhw; 
	glVertex4f(vertex.p.x * w, vertex.p.y * w, vertex.p.z * w, w);
}

template <>
void renderVertex(const SMY_VERTEX & vertex) {
	
	Color c = Color::fromBGRA(vertex.color);
	glColor4ub(c.r, c.g, c.b, c.a);
	
	glMultiTexCoord2f(GL_TEXTURE0, vertex.uv.x, vertex.uv.y);
	
	glVertex3f(vertex.p.x, vertex.p.y, vertex.p.z);
}

template <>
void renderVertex(const SMY_VERTEX3 & vertex) {
	
	Color c = Color::fromBGRA(vertex.color);
	glColor4ub(c.r, c.g, c.b, c.a);
	
	glMultiTexCoord2f(GL_TEXTURE0, vertex.uv[0].x, vertex.uv[0].y);
	glMultiTexCoord2f(GL_TEXTURE1, vertex.uv[1].x, vertex.uv[1].y);
	glMultiTexCoord2f(GL_TEXTURE2, vertex.uv[2].x, vertex.uv[2].y);
	
	glVertex3f(vertex.p.x, vertex.p.y, vertex.p.z);
}

extern const GLenum arxToGlPrimitiveType[];

template <class Vertex>
class GLNoVertexBuffer : public VertexBuffer<Vertex> {
	
public:
	
	using VertexBuffer<Vertex>::capacity;
	
	GLNoVertexBuffer(OpenGLRenderer * _renderer, size_t capacity) : VertexBuffer<Vertex>(capacity), renderer(_renderer), buffer(new Vertex[capacity]) { }
	
	void setData(const Vertex * vertices, size_t count, size_t offset, BufferFlags flags) {
		ARX_UNUSED(flags);
		
		arx_assert(offset + count <= capacity());
		
		std::copy(vertices, vertices + count, buffer + offset);
	}
	
	Vertex * lock(BufferFlags flags, size_t offset, size_t count) {
		ARX_UNUSED(flags), ARX_UNUSED(count);
		return buffer + offset;
	}
	
	void unlock() {
		// nothing to do
	}
	
	void draw(Renderer::Primitive primitive, size_t count, size_t offset) const {
		
		arx_assert(offset + count <= capacity());
		
		renderer->beforeDraw<Vertex>();
		
		glBegin(arxToGlPrimitiveType[primitive]);
		
		for(size_t i = 0; i < count; i++) {
			renderVertex(buffer[offset + i]);
		}
		
		glEnd();
		
		CHECK_GL;
	}
	
	void drawIndexed(Renderer::Primitive primitive, size_t count, size_t offset, unsigned short * indices, size_t nbindices) const {
		ARX_UNUSED(count), ARX_UNUSED(offset);
		
		arx_assert(offset + count <= capacity());
		arx_assert(indices != NULL);
		
		renderer->beforeDraw<Vertex>();

		Vertex * pBuf = buffer + offset;
		
		glBegin(arxToGlPrimitiveType[primitive]);
		
		for(size_t i = 0; i < nbindices; i++) {
			renderVertex(pBuf[indices[i]]);
		}
		
		glEnd();
		
		CHECK_GL;
	}
	
	~GLNoVertexBuffer() {
		delete[] buffer;
	};
	
private:
	
	OpenGLRenderer * renderer;
	Vertex * buffer;
	
};

#endif // ARX_GRAPHICS_OPENGL_GLNOVERTEXBUFFER_H
