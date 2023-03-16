/*
 ==============================================================================

 Copyright (c) 2020 - 2021, Foleys Finest Audio - Daniel Walz
 All rights reserved.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 ==============================================================================
 */

#pragma once

#if FOLEYS_USE_OPENGL

namespace foleys
{

namespace OpenGLDrawing
{
using namespace juce::gl;

#if JUCE_DEBUG
static const char* getGLErrorMessage (const GLenum e) noexcept
{
    switch (e)
    {
		case juce::gl::GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
		case juce::gl::GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
		case juce::gl::GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
		case juce::gl::GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
       #ifdef GL_STACK_OVERFLOW
		case juce::gl::GL_STACK_OVERFLOW:                 return "GL_STACK_OVERFLOW";
       #endif
       #ifdef GL_STACK_UNDERFLOW
		case juce::gl::GL_STACK_UNDERFLOW:                return "GL_STACK_UNDERFLOW";
       #endif
       #ifdef GL_INVALID_FRAMEBUFFER_OPERATION
		case juce::gl::GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
       #endif
        default: break;
    }

    return "Unknown error";
}

static void checkGLError (const char* file, const int line)
{
    for (;;)
    {
        const GLenum e = juce::gl::glGetError();

		if (e == juce::gl::GL_NO_ERROR)
            break;

        DBG ("***** " << getGLErrorMessage (e) << "  at " << file << " : " << line);
          jassertfalse;
    }
}

#define JUCE_CHECK_OPENGL_ERROR checkGLError (__FILE__, __LINE__);
#else
#define JUCE_CHECK_OPENGL_ERROR ;
#endif

static void clearGLError() noexcept
{
    while (juce::gl::glGetError() != juce::gl::GL_NO_ERROR) {}
}


struct DepthTestDisabler
{
    DepthTestDisabler() noexcept
    {
		juce::gl::glGetBooleanv (juce::gl::GL_DEPTH_TEST, &wasEnabled);

        if (wasEnabled)
			juce::gl::glDisable (juce::gl::GL_DEPTH_TEST);
    }

    ~DepthTestDisabler() noexcept
    {
        if (wasEnabled)
			juce::gl::glEnable (juce::gl::GL_DEPTH_TEST);
    }

	GLboolean wasEnabled;
};

static inline void drawTexture (juce::OpenGLContext& context,
                                const juce::Rectangle<int>& targetClipArea,
                                const juce::Rectangle<int>& anchorPosAndTextureSize,
                                const int contextWidth, const int contextHeight,
                                bool flippedVertically,
                                float alpha,
                                juce::AffineTransform transform)
{
    if (contextWidth <= 0 || contextHeight <= 0)
        return;

    juce::ignoreUnused (transform);

    JUCE_CHECK_OPENGL_ERROR
	juce::gl::glBlendFunc (juce::gl::GL_ONE, juce::gl::GL_ONE_MINUS_SRC_ALPHA);
	juce::gl::glEnable (juce::gl::GL_BLEND);

    DepthTestDisabler depthDisabler;

    if (context.areShadersAvailable())
    {
        struct OverlayShaderProgram  : public juce::ReferenceCountedObject
        {
            OverlayShaderProgram (juce::OpenGLContext& context)
                : program (context), builder (program), params (program)
            {}

            static const OverlayShaderProgram& select (juce::OpenGLContext& context)
            {
                static const char programValueID[] = "foleysVideoShader";
                OverlayShaderProgram* program = static_cast<OverlayShaderProgram*> (context.getAssociatedObject (programValueID));

                if (program == nullptr)
                {
                    program = new OverlayShaderProgram (context);
                    context.setAssociatedObject (programValueID, program);
                }

                program->program.use();
                return *program;
            }

            struct ProgramBuilder
            {
                ProgramBuilder (juce::OpenGLShaderProgram& prog)
                {
                    prog.addVertexShader (juce::OpenGLHelpers::translateVertexShaderToV3 (
                        "attribute " JUCE_HIGHP " vec2 position;"
                        "uniform " JUCE_HIGHP " vec2 screenSize;"
                        "uniform " JUCE_HIGHP " float textureBounds[4];"
                        "uniform " JUCE_HIGHP " mat4 transformation;"
                        "uniform " JUCE_HIGHP " vec2 vOffsetAndScale;"
                        "varying " JUCE_HIGHP " vec2 texturePos;"
                        "void main()"
                        "{"
                          JUCE_HIGHP " vec2 scaled = position / (0.5 * screenSize.xy);"
                          "gl_Position = vec4 (scaled.x - 1.0, 1.0 - scaled.y, 0, 1.0) * transformation;"
                          "texturePos = (position - vec2 (textureBounds[0], textureBounds[1])) / vec2 (textureBounds[2], textureBounds[3]);"
                          "texturePos = vec2 (texturePos.x, vOffsetAndScale.x + vOffsetAndScale.y * texturePos.y);"
                        "}"));

                    prog.addFragmentShader (juce::OpenGLHelpers::translateFragmentShaderToV3 (
                        "uniform sampler2D imageTexture;"
                        "varying " JUCE_HIGHP " vec2 texturePos;"
                        "uniform " JUCE_HIGHP " float alphaMultiplier;"
                        "void main()"
                        "{"
                          "gl_FragColor = texture2D (imageTexture, texturePos) * alphaMultiplier;"
                        "}"));

                    prog.link();
                }
            };

            struct Params
            {
                Params (juce::OpenGLShaderProgram& prog)
                    : positionAttribute (prog, "position"),
                      screenSize (prog, "screenSize"),
                      imageTexture (prog, "imageTexture"),
                      textureBounds (prog, "textureBounds"),
                      vOffsetAndScale (prog, "vOffsetAndScale"),
                      transformation (prog, "transformation"),
                      alphaMultiplier (prog, "alphaMultiplier")
                {}

                void set (const float targetWidth, const float targetHeight, const juce::Rectangle<float>& bounds, bool flipVertically, const juce::AffineTransform& transform, const float alpha) const
                {
                    const GLfloat m[] = { bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight() };
                    textureBounds.set (m, 4);
                    imageTexture.set (0);
                    screenSize.set (targetWidth, targetHeight);
                    vOffsetAndScale.set (flipVertically ? 0.0f : 1.0f,
                                         flipVertically ? 1.0f : -1.0f);

                    juce::Matrix3D<GLfloat> matrix (transform);
                    transformation.setMatrix4 (matrix.mat, 1, false);

                    alphaMultiplier.set (alpha);
                }

                juce::OpenGLShaderProgram::Attribute positionAttribute;
                juce::OpenGLShaderProgram::Uniform screenSize, imageTexture, textureBounds, vOffsetAndScale, transformation, alphaMultiplier;
            };

            juce::OpenGLShaderProgram program;
            ProgramBuilder builder;
            Params params;
        };

        auto left   = (GLshort) targetClipArea.getX();
        auto top    = (GLshort) targetClipArea.getY();
        auto right  = (GLshort) targetClipArea.getRight();
        auto bottom = (GLshort) targetClipArea.getBottom();
        const GLshort vertices[] = { left, bottom, right, bottom, left, top, right, top };

        auto& program = OverlayShaderProgram::select (context);
        program.params.set ((float) contextWidth, (float) contextHeight, anchorPosAndTextureSize.toFloat(), flippedVertically, transform, alpha);
        JUCE_CHECK_OPENGL_ERROR

        GLuint vertexBuffer = 0;
        context.extensions.glGenBuffers (1, &vertexBuffer);
        context.extensions.glBindBuffer (juce::gl::GL_ARRAY_BUFFER, vertexBuffer);
        context.extensions.glBufferData (juce::gl::GL_ARRAY_BUFFER, sizeof (vertices), vertices, juce::gl::GL_STATIC_DRAW);
        JUCE_CHECK_OPENGL_ERROR

        auto index = (GLuint) program.params.positionAttribute.attributeID;
        context.extensions.glVertexAttribPointer (index, 2, juce::gl::GL_SHORT, juce::gl::GL_FALSE, 4, nullptr);
        context.extensions.glEnableVertexAttribArray (index);
        JUCE_CHECK_OPENGL_ERROR

        if (context.extensions.glCheckFramebufferStatus (juce::gl::GL_FRAMEBUFFER) == juce::gl::GL_FRAMEBUFFER_COMPLETE)
        {
			juce::gl::glDrawArrays (juce::gl::GL_TRIANGLE_STRIP, 0, 4);

            context.extensions.glBindBuffer (juce::gl::GL_ARRAY_BUFFER, 0);
            context.extensions.glUseProgram (0);
            context.extensions.glDisableVertexAttribArray (index);
            context.extensions.glDeleteBuffers (1, &vertexBuffer);
        }
        else
        {
            clearGLError();
        }
    }

    JUCE_CHECK_OPENGL_ERROR
}

} // namespace OpenGLDrawing

} // namespace foleys

#endif
