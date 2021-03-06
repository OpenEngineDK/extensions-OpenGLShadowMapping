// OpenGL shadow map renderer implementation.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include <Renderers/IRenderingView.h>
#include "ShadowMapRenderer.h"
#include <Scene/ISceneNode.h>
#include <Logging/Logger.h>
#include <Meta/OpenGL.h>
#include <Math/Math.h>
#include <Display/IViewingVolume.h>

#include <Scene/TransformationNode.h>
#include <Scene/LightNode.h>
#include <Scene/DirectionalLightNode.h>
#include <Scene/PointLightNode.h>
#include <Scene/SpotLightNode.h>

#include <Resources/ITextureResource.h>
using namespace OpenEngine::Resources;

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

using namespace OpenEngine::Math;

using OpenEngine::Math::Vector;
using OpenEngine::Math::Matrix;
using OpenEngine::Display::IViewingVolume;

GLSLVersion ShadowMapRenderer::glslversion = GLSL_UNKNOWN;

ShadowMapRenderer::ShadowMapRenderer(Viewport* viewport)
    : root(NULL) 
    , viewport(viewport)
{

}

/**
 * Renderer destructor.
 * Deletes the internal viewport.
 */
ShadowMapRenderer::~ShadowMapRenderer() {
    delete viewport;
}

int ShadowMapRenderer::GetShadowMapID(){
    return shadowMapID;
}

Viewport& ShadowMapRenderer::GetViewport() const {
    return *viewport;
}

void ShadowMapRenderer::SetSceneRoot(ISceneNode* root) {
    this->root = root;
}

ISceneNode* ShadowMapRenderer::GetSceneRoot() const {
    return root;
}

void ShadowMapRenderer::InitializeGLSLVersion() {
    // Initialize the "OpenGL Extension Wrangler" library
    GLenum err = glewInit();
    if (err!=GLEW_OK)
        logger.error << "GLEW: "
                     << (const char*)glewGetErrorString(err)
                     << logger.end;
    else {
        logger.info << "OpenGL: "
                    << (const char*)glGetString(GL_VERSION)
                    << " GLEW: "
                    << (const char*)glewGetString(GLEW_VERSION)
                    << logger.end;

		if (glewIsSupported("GL_VERSION_2_0")) {
            glslversion = GLSL_20;
			logger.info << "Using OpenGL version 2.0 with GLSL: "
                        << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)
                        << logger.end;
		}
		else if (glewIsSupported("GL_VERSION_1_4") &&
                 GLEW_ARB_vertex_shader &&
                 GLEW_ARB_fragment_shader) {
            glslversion = GLSL_14;
			logger.info << "Using OpenGL version 1.4 with shaders as extensions"
                        << logger.end;
		}
		else {
            glslversion = GLSL_NONE;
            logger.info << "GLSL not supported - shaders are disabled"
                        << logger.end;
		}
    }
}

void ShadowMapRenderer::Handle(InitializeEventArg arg) {
    CHECK_FOR_GL_ERROR();

    InitializeGLSLVersion(); //@todo: HACK - to get Inseminator to work
    CHECK_FOR_GL_ERROR();

    // Clear the OpenGL frame buffer.
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); 
    CHECK_FOR_GL_ERROR();

    // Generate the texture id
    GLuint tmp;
    glGenTextures(1, &tmp);
    shadowMapID = tmp;

    glBindTexture(GL_TEXTURE_2D, shadowMapID);
    
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    CHECK_FOR_GL_ERROR();
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);						   
    CHECK_FOR_GL_ERROR();

    // Set perspective calculations to most accurate
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    CHECK_FOR_GL_ERROR();

    // Check that we have a scene.
    if (root == NULL)
        throw Exception("No scene root found while rendering.");

    this->initialize.Notify(RenderingEventArg(*this));
    this->stage = RENDERER_PREPROCESS;
    CHECK_FOR_GL_ERROR();
}

/**
 * @note The processing function assumes that the scene has not been
 *       replaced by null since the initialization face. 
 */
void ShadowMapRenderer::Handle(ProcessEventArg arg) {
    // @todo: assert we are in preprocess stage
    // run the processing phases
    RenderingEventArg rarg(*this, arg.start, arg.approx);
    this->preProcess.Notify(rarg);
    this->stage = RENDERER_PROCESS;

    CHECK_FOR_GL_ERROR();
    this->process.Notify(rarg);
    RenderTest();
    this->stage = RENDERER_POSTPROCESS;
    this->postProcess.Notify(rarg);
    this->stage = RENDERER_PREPROCESS;
}

void ShadowMapRenderer::Handle(DeinitializeEventArg arg) {
    this->stage = RENDERER_DEINITIALIZE;
    this->deinitialize.Notify(RenderingEventArg(*this));
}

IEvent<RenderingEventArg>& ShadowMapRenderer::InitializeEvent() {
    return initialize;
}
IEvent<RenderingEventArg>& ShadowMapRenderer::PreProcessEvent() {
    return preProcess;
}
IEvent<RenderingEventArg>& ShadowMapRenderer::ProcessEvent() {
    return process;
}
IEvent<RenderingEventArg>& ShadowMapRenderer::PostProcessEvent() {
    return postProcess;
}
IEvent<RenderingEventArg>& ShadowMapRenderer::DeinitializeEvent() {
    return deinitialize;
}

void ShadowMapRenderer::ApplyViewingVolume(IViewingVolume& volume) {
    // Select The Projection Matrix
    glMatrixMode(GL_PROJECTION);
    CHECK_FOR_GL_ERROR();

    // Reset The Projection Matrix
    glLoadIdentity();
    CHECK_FOR_GL_ERROR();

    // Setup OpenGL with the volumes projection matrix
    Matrix<4,4,float> projMatrix = volume.GetProjectionMatrix();
    float arr[16] = {0};
    projMatrix.ToArray(arr);
    glMultMatrixf(arr);
    CHECK_FOR_GL_ERROR();

    // Select the modelview matrix
    glMatrixMode(GL_MODELVIEW);
    CHECK_FOR_GL_ERROR();

    // Reset the modelview matrix
    glLoadIdentity();
    CHECK_FOR_GL_ERROR();

    // Get the view matrix and apply it
    Matrix<4,4,float> matrix = volume.GetViewMatrix();
    float f[16] = {0};
    matrix.ToArray(f);
    glMultMatrixf(f);
    CHECK_FOR_GL_ERROR();
}

bool ShadowMapRenderer::IsGLSLSupported() {
    return (glslversion != GLSL_NONE && glslversion != GLSL_UNKNOWN);
}

GLSLVersion ShadowMapRenderer::GetGLSLVersion() {
    return glslversion;
}

void ShadowMapRenderer::LoadTexture(ITextureResourcePtr texr) {
    // check for null pointers
    if (texr == NULL) return;

    // signal we need the texture data
    texr->Load();

    // bind the texture
    RebindTexture(texr);

    // signal we are done with the texture data
    texr->Unload();
}

void ShadowMapRenderer::RebindTexture(ITextureResourcePtr texr) {
    // check for null pointers
    if (texr == NULL) return;

    GLuint texid = 0;
    bool firstload = (texr->GetID() == 0);

    // first time, generate id and uploade, else update texture
    if (firstload) {
        //@todo: check that there is a gl context
        glGenTextures(1, &texid);
        CHECK_FOR_GL_ERROR();
        texr->SetID(texid);
    } else 
        texid = texr->GetID();

    glBindTexture(GL_TEXTURE_2D, texid);
    CHECK_FOR_GL_ERROR();

    GLuint depth = 0;
    switch (texr->GetColorFormat()) {
    case LUMINANCE:  depth = GL_LUMINANCE; break;
    case RGB: depth = GL_RGB;   break;
    case RGBA: depth = GL_RGBA;  break;
    case BGR: depth = GL_BGR;   break;
    case BGRA: depth = GL_BGRA;  break;
    default: logger.warning << "Unsupported color depth: " 
                            << texr->GetDepth() << logger.end;
    }

    if (firstload) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        CHECK_FOR_GL_ERROR();
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        CHECK_FOR_GL_ERROR();
        
        glTexImage2D(GL_TEXTURE_2D,
                     0, // mipmap level
                     texr->GetDepth()/8,
                     texr->GetWidth(),
                     texr->GetHeight(),
                     0, // border
                     depth,
                     GL_UNSIGNED_BYTE,
                     texr->GetData());
    }
    else
        glTexSubImage2D(GL_TEXTURE_2D,
                        0, // mipmap level
                        0, // offset x
                        0, // offset y
                        texr->GetWidth(),
                        texr->GetHeight(),
                        depth,
                        GL_UNSIGNED_BYTE,
                        texr->GetData());
      // @todo: when updating, what if the size if greater than the old texture
    CHECK_FOR_GL_ERROR();
}

void ShadowMapRenderer::DrawFace(FacePtr f) {
    if (f->mat->texr == NULL) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    } else {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, f->mat->texr->GetID());
    }
    float col[4];
    f->mat->diffuse.ToArray(col);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col);
    f->mat->ambient.ToArray(col);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, col);
    f->mat->specular.ToArray(col);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, col);
    f->mat->emission.ToArray(col);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, col);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, f->mat->shininess);
    glBegin(GL_TRIANGLES);
    // for each vertex ...
    for (int i=0; i<3; i++) {
        Vector<3,float> v = f->vert[i];
        Vector<2,float> t = f->texc[i];
        Vector<3,float> n = f->norm[i];
        Vector<4,float> c = f->colr[i];
        glTexCoord2f(t[0],t[1]);
        glColor4f (c[0],c[1],c[2],c[3]);
        glNormal3f(n[0],n[1],n[2]);
        glVertex3f(v[0],v[1],v[2]);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

/**
 * Helper function drawing a wired face.
 *
 * @param face face to draw.
 * @param color line color.
 * @param width line width, default i one.
 */
void ShadowMapRenderer::DrawFace(FacePtr face, Vector<3,float> color, float width) {
    GLboolean t = glIsEnabled(GL_TEXTURE_2D);
    GLboolean l = glIsEnabled(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();

    glLineWidth(width);
    CHECK_FOR_GL_ERROR();

    glBegin(GL_LINES);
        glColor3f(color[0],color[1],color[2]);
        glVertex3f(face->vert[0][0], face->vert[0][1], face->vert[0][2]);
        glVertex3f(face->vert[1][0], face->vert[1][1], face->vert[1][2]);

        glVertex3f(face->vert[1][0], face->vert[1][1], face->vert[1][2]);
        glVertex3f(face->vert[2][0], face->vert[2][1], face->vert[2][2]);

        glVertex3f(face->vert[2][0], face->vert[2][1], face->vert[2][2]);
        glVertex3f(face->vert[0][0], face->vert[0][1], face->vert[0][2]);
    glEnd();
    CHECK_FOR_GL_ERROR();

    // reset state
    if (t) glEnable(GL_TEXTURE_2D);
    if (l) glEnable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
}

/**
 * Helper function drawing a line.
 *
 * @param line line to draw.
 * @param color line color.
 * @param width line width, default i one.
 */
void ShadowMapRenderer::DrawLine(Line line, Vector<3,float> color, float width) {
    GLboolean t = glIsEnabled(GL_TEXTURE_2D);
    GLboolean l = glIsEnabled(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();

    glLineWidth(width);
    CHECK_FOR_GL_ERROR();

    glBegin(GL_LINES);
        glColor3f(color[0],color[1],color[2]);
        glVertex3f(line.point1[0],line.point1[1],line.point1[2]);
        glVertex3f(line.point2[0],line.point2[1],line.point2[2]);
    glEnd();
    CHECK_FOR_GL_ERROR();

    // reset state 
    if (t) glEnable(GL_TEXTURE_2D);
    if (l) glEnable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
}

/**
 * Helper function drawing a point.
 *
 * @param point point to draw.
 * @param color line color.
 * @param size dot size, default i one.
 */
void ShadowMapRenderer::DrawPoint(Vector<3,float> point, Vector<3,float> color , float size) {
    GLboolean t = glIsEnabled(GL_TEXTURE_2D);
    GLboolean l = glIsEnabled(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();

    glPointSize(size);
    CHECK_FOR_GL_ERROR();

    glBegin(GL_POINTS);
        glColor3f(color[0],color[1],color[2]);
        glVertex3f(point[0],point[1],point[2]);
    glEnd();
    CHECK_FOR_GL_ERROR();

    // reset state
    if (t) glEnable(GL_TEXTURE_2D);
    if (l) glEnable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
}

void ShadowMapRenderer::RenderTest() {

    CHECK_FOR_GL_ERROR();

    // quad rendering of texture
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    CHECK_FOR_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, shadowMapID);
    CHECK_FOR_GL_ERROR();
    

    CHECK_FOR_GL_ERROR();

    glColor4f(1.0, 0.5, 0.5, 1.0);

    glBegin(GL_QUADS);
    glVertex3f(-100.0, 100.0, 100.0);
    glVertex3f(100.0, 100.0, 100.0);
    glVertex3f(100.0, -200.0, 200.0);
    glVertex3f(-100.0, -200.0, 200.0);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(-100.0, -200.0, 200.0);
    glVertex3f(100.0, -200.0, 200.0);
    glVertex3f(100.0, -200.0, 400.0);
    glVertex3f(-100.0, -200.0, 400.0);
    glEnd();

    CHECK_FOR_GL_ERROR();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    CHECK_FOR_GL_ERROR();
}

} // NS OpenGL
} // NS OpenEngine
} // NS Renderers
