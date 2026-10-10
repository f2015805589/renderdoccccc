/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2023-2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "gl_test.h"

RD_TEST(GL_Annotations, OpenGLGraphicsTest)
{
  static constexpr const char *Description = "Test annotations via the OpenGL API.";

  int main()
  {
    // initialise, create window, create context, etc
    if(!Init())
      return 3;

    GLuint tex = MakeTexture();
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 4, 4);

    glObjectLabel(GL_TEXTURE, tex, -1, "Annotated Image");
    glObjectLabel(GL_BUFFER, DefaultTriVB, -1, "Vertex Buffer");

    // cache the device pointer we pass in
    void *d = mainContext;

    if(rdoc)
    {
      // GL needs a helper struct to specify the type and handle together
      RDGLObjectHelper img(GL_TEXTURE, tex);
      RDGLObjectHelper buf(GL_BUFFER, DefaultTriVB);

      rdoc->SetObjectAnnotation(d, img, "basic.bool", eRENDERCCCCC_Bool, 0, RDAnnotationHelper(true));
      rdoc->SetObjectAnnotation(d, img, "basic.int32", eRENDERCCCCC_Int32, 0, RDAnnotationHelper(-3));
      rdoc->SetObjectAnnotation(d, img, "basic.int64", eRENDERCCCCC_Int64, 0,
                                RDAnnotationHelper((int64_t)-3000000000000LL));
      rdoc->SetObjectAnnotation(d, img, "basic.uint32", eRENDERCCCCC_UInt32, 0, RDAnnotationHelper(3));
      rdoc->SetObjectAnnotation(d, img, "basic.uint64", eRENDERCCCCC_UInt64, 0,
                                RDAnnotationHelper((uint64_t)3000000000000LL));
      rdoc->SetObjectAnnotation(d, img, "basic.float", eRENDERCCCCC_Float, 0,
                                RDAnnotationHelper(3.25f));
      rdoc->SetObjectAnnotation(d, img, "basic.double", eRENDERCCCCC_Double, 0,
                                RDAnnotationHelper(3.25000000001));
      rdoc->SetObjectAnnotation(d, img, "basic.string", eRENDERCCCCC_String, 0,
                                RDAnnotationHelper("Hello, World!"));

      RENDERCCCCC_AnnotationValue val;
      val.apiObject = (void *)(RENDERCCCCC_GLResourceReference *)buf;
      rdoc->SetObjectAnnotation(d, img, "basic.object", eRENDERCCCCC_APIObject, 0, &val);

      rdoc->SetObjectAnnotation(d, img, "basic.object.__offset", eRENDERCCCCC_UInt32, 0,
                                RDAnnotationHelper(64));
      rdoc->SetObjectAnnotation(d, img, "basic.object.__size", eRENDERCCCCC_UInt32, 0,
                                RDAnnotationHelper(32));
      rdoc->SetObjectAnnotation(d, img, "basic.object.__rd_format", eRENDERCCCCC_String, 0,
                                RDAnnotationHelper("float4 vertex_data;"));

      rdoc->SetObjectAnnotation(d, buf, "__rd_format", eRENDERCCCCC_String, 0,
                                RDAnnotationHelper("float3 pos;\n"
                                                   "float4 col;\n"
                                                   "float2 uv;\n"));

      val = {};
      val.vector.float32[0] = 1.1f;
      val.vector.float32[1] = 2.2f;
      val.vector.float32[2] = 3.3f;
      val.vector.float32[3] = 4.4f;    // should be ignored
      rdoc->SetObjectAnnotation(d, img, "basic.vec3", eRENDERCCCCC_Float, 3, &val);

      rdoc->SetObjectAnnotation(d, img, "deep.nested.path.to.annotation", eRENDERCCCCC_Int32, 0,
                                RDAnnotationHelper(-4));
      rdoc->SetObjectAnnotation(d, img, "deep.nested.path.to.annotation2", eRENDERCCCCC_Int32, 0,
                                RDAnnotationHelper(-5));
      rdoc->SetObjectAnnotation(d, img, "deep.alternate.path.to.annotation", eRENDERCCCCC_Int32, 0,
                                RDAnnotationHelper(-6));

      // deleted paths should not stay around
      rdoc->SetObjectAnnotation(d, img, "deleteme", eRENDERCCCCC_Int32, 0, RDAnnotationHelper(-7));
      rdoc->SetObjectAnnotation(d, img, "deleteme", eRENDERCCCCC_Empty, 0, NULL);

      rdoc->SetObjectAnnotation(d, img, "path.deleted.by.parent", eRENDERCCCCC_Int32, 0,
                                RDAnnotationHelper(-8));
      rdoc->SetObjectAnnotation(d, img, "path.deleted.by.parent2", eRENDERCCCCC_Int32, 0,
                                RDAnnotationHelper(-9));

      // this will delete all children. `path` will still exist, but will be empty
      rdoc->SetObjectAnnotation(d, img, "path.deleted", eRENDERCCCCC_Empty, 0, NULL);
    }

    while(Running())
    {
      if(rdoc)
      {
        // queue annotations don't exist in GL, but we set these to share the same test code.
        if(curFrame == 2)
          rdoc->SetCommandAnnotation(d, NULL, "queue.too_old", eRENDERCCCCC_Int32, 0,
                                     RDAnnotationHelper(1000));

        rdoc->SetCommandAnnotation(d, NULL, "queue.value", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(1000));

        rdoc->SetCommandAnnotation(d, NULL, "command.overwritten", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(9999));

        rdoc->SetCommandAnnotation(d, NULL, "command.inherited", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(1234));

        rdoc->SetCommandAnnotation(d, NULL, "command.deleted", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(50));
      }

      setMarker("Start");

      if(rdoc)
      {
        rdoc->SetCommandAnnotation(d, NULL, "new.value", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(2000));

        rdoc->SetCommandAnnotation(d, NULL, "command.overwritten", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(-3333));

        rdoc->SetCommandAnnotation(d, NULL, "command.new", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(3333));

        rdoc->SetCommandAnnotation(d, NULL, "command.deleted", eRENDERCCCCC_Empty, 0, NULL);
      }

      setMarker("Initial");

      glClearBufferfv(GL_COLOR, 0, DefaultClearCol);

      glBindTexture(GL_TEXTURE_2D, tex);

      if(rdoc)
        rdoc->SetCommandAnnotation(d, NULL, "command.new", eRENDERCCCCC_Float, 0,
                                   RDAnnotationHelper(1.75f));

      glBindVertexArray(DefaultTriVAO);
      glUseProgram(DefaultTriProgram);
      glViewport(0, 0, GLsizei(screenWidth), GLsizei(screenHeight));

      setMarker("Pre-Draw");

      // deleting a value is fine if it's re-added before the next event
      if(rdoc)
      {
        rdoc->SetCommandAnnotation(d, NULL, "new.value", eRENDERCCCCC_Empty, 0, NULL);
        rdoc->SetCommandAnnotation(d, NULL, "new.value", eRENDERCCCCC_Int32, 0,
                                   RDAnnotationHelper(4000));
      }

      setMarker("Draw 1");

      glDrawArrays(GL_TRIANGLES, 0, 3);

      glViewport(0, 0, GLsizei(screenWidth) / 2, GLsizei(screenHeight) / 2);

      setMarker("Draw 2");

      glDrawArrays(GL_TRIANGLES, 0, 3);

      Present();
    }

    return 0;
  }
};

REGISTER_TEST();
