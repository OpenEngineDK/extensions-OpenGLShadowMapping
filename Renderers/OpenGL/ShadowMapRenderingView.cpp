// OpenGL shadow map rendering view.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#include "ShadowMapRenderingView.h"
#include "ShadowMapRenderer.h"
#include <Geometry/FaceSet.h>
#include <Scene/GeometryNode.h>
#include <Scene/TransformationNode.h>
#include <Scene/RenderStateNode.h>
#include <Scene/PointLightNode.h>
#include <Resources/IShaderResource.h>
#include <Display/Viewport.h>
#include <Display/IViewingVolume.h>

#include <Meta/OpenGL.h>
#include <Math/Math.h>

// Logging
#include <Logging/Logger.h>

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

using OpenEngine::Math::Quaternion;
using OpenEngine::Math::Vector;
using OpenEngine::Math::Matrix;
using OpenEngine::Geometry::FaceSet;
using OpenEngine::Resources::IShaderResource;
using OpenEngine::Display::Viewport;
using OpenEngine::Display::IViewingVolume;

/**
 * Rendering view constructor.
 *
 * @param viewport Viewport in which to render.
 */
    ShadowMapRenderingView::ShadowMapRenderingView(Viewport& viewport)
    : IRenderingView(viewport),
      renderer(NULL) {
    backgroundColor = Vector<4,float>(1.0);
    this->lightNode = new PointLightNode();
}

/**
 * Rendering view destructor.
 */
ShadowMapRenderingView::~ShadowMapRenderingView() {}

/**
 * Get the renderer that the view is processing for.
 *
 * @return Current renderer, NULL if no renderer processing is active.
 */
IRenderer* ShadowMapRenderingView::GetRenderer() {
    return renderer;
}

void ShadowMapRenderingView::Handle(RenderingEventArg arg) {
    

    glPushMatrix();
    CHECK_FOR_GL_ERROR();
    // the following is moved from the previous Renderer::Process

    Viewport& viewport = this->GetViewport();
    IViewingVolume* volume = viewport.GetViewingVolume();

    //update the position of the lights viewing volume
    Vector<3,float>* position = new Vector<3,float>(0.0,0.0,0.0);
    Quaternion<float>* rotation = new Quaternion<float>();
    if(dynamic_cast<TransformationNode*>(lightNode->GetParent())) {
        TransformationNode* parent = static_cast<TransformationNode*>(lightNode->GetParent());
        parent->GetAccumulatedTransformations(position, rotation);
    }
    volume->SetPosition(*position);
    //logger.info << volume->GetPosition() << "\n" << logger.end;

    // If no viewing volume is set for the viewport ignore it.
    if (volume == NULL) return;
    volume->SignalRendering(arg.approx);

    // Set viewport size
    Vector<4,int> dim = viewport.GetDimension();
    glViewport((GLsizei)dim[0], (GLsizei)dim[1], (GLsizei)dim[2], (GLsizei)dim[3]);
    CHECK_FOR_GL_ERROR();
    
    // apply the volume
    arg.renderer.ApplyViewingVolume(*volume);

    // Really Nice Perspective Calculations
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // setup default render state
    RenderStateNode* renderStateNode = new RenderStateNode();
    renderStateNode->DisableOption(RenderStateNode::TEXTURE);
    renderStateNode->DisableOption(RenderStateNode::SHADER);
    renderStateNode->DisableOption(RenderStateNode::BACKFACE);
    renderStateNode->EnableOption(RenderStateNode::DEPTH_TEST);
    renderStateNode->DisableOption(RenderStateNode::LIGHTING); //@todo
    renderStateNode->DisableOption(RenderStateNode::WIREFRAME);
    ApplyRenderState(renderStateNode);
    delete renderStateNode;

    Vector<4,float> bgc = backgroundColor;
    glClearColor(bgc[0], bgc[1], bgc[2], bgc[3]);

    // Clear the screen and the depth buffer.
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    Render(&arg.renderer, arg.renderer.GetSceneRoot());

    ShadowMapRenderer* shadRend = static_cast<const ShadowMapRenderer*>(&arg.renderer);
    GLuint texName = shadRend->GetShadowMapID();

    
    glBindTexture(GL_TEXTURE_2D, texName);
    // overwrite the previous shadow map
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, dim[2], dim[3], 0);

    glPopMatrix();
}

/**
 * Renderer the scene.
 *
 * @param renderer a Renderer
 * @param root The scene to be rendered
 */
void ShadowMapRenderingView::Render(IRenderer* renderer, ISceneNode* root) {
    this->renderer = renderer;
    root->Accept(*this);
    this->renderer = NULL;
}

void ShadowMapRenderingView::ApplyRenderState(RenderStateNode* node) {
    if (node->IsOptionEnabled(RenderStateNode::WIREFRAME)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        CHECK_FOR_GL_ERROR();
    }
    else if (node->IsOptionDisabled(RenderStateNode::WIREFRAME)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        CHECK_FOR_GL_ERROR();
    }

    if (node->IsOptionEnabled(RenderStateNode::BACKFACE)) {
        glDisable(GL_CULL_FACE);
        CHECK_FOR_GL_ERROR();
    }
    else if (node->IsOptionDisabled(RenderStateNode::BACKFACE)) {
        glEnable(GL_CULL_FACE);
        CHECK_FOR_GL_ERROR();
    }

    if (node->IsOptionEnabled(RenderStateNode::LIGHTING)) {
        glEnable(GL_LIGHTING);
        CHECK_FOR_GL_ERROR();
    }
    else if (node->IsOptionDisabled(RenderStateNode::LIGHTING)) {
        glDisable(GL_LIGHTING);
        CHECK_FOR_GL_ERROR();
    }

    if (node->IsOptionEnabled(RenderStateNode::DEPTH_TEST)) {
        glEnable(GL_DEPTH_TEST);
        CHECK_FOR_GL_ERROR();
    }
    else if (node->IsOptionDisabled(RenderStateNode::DEPTH_TEST)) {
        glDisable(GL_DEPTH_TEST);
        CHECK_FOR_GL_ERROR();
    }

}

/**
 * Process a transformation node.
 *
 * @param node Transformation node to apply.
 */
void ShadowMapRenderingView::VisitTransformationNode(TransformationNode* node) {
    // push transformation matrix
    Matrix<4,4,float> m = node->GetTransformationMatrix();
    float f[16];
    m.ToArray(f);
    glPushMatrix();
    CHECK_FOR_GL_ERROR();
    glMultMatrixf(f);
    CHECK_FOR_GL_ERROR();
    // traverse sub nodes
    node->VisitSubNodes(*this);
    // pop transformation matrix
    glPopMatrix();
    CHECK_FOR_GL_ERROR();
}


/**
 * Process a geometry node.
 *
 * @param node Geometry node to render
 */
void ShadowMapRenderingView::VisitGeometryNode(GeometryNode* node) {

    // Remember last bound texture and shader
    FaceList::iterator itr;
    FaceSet* faces = node->GetFaceSet();
    if (faces == NULL) return;

    // for each face ...
    for (itr = faces->begin(); itr != faces->end(); itr++) {
        FacePtr f = (*itr);

        //        ApplyMaterial(f->mat);

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
            // apply tangent and binormal per vertex for the shader to use
//             if (currentShader != NULL) {
//                 if (binormalid != -1)
//                     currentShader->VertexAttribute(binormalid, f->bino[i]);
//                 if (tangentid != -1)
//                     currentShader->VertexAttribute(tangentid, f->tang[i]);
//             }
			glVertex3f(v[0],v[1],v[2]);
        }
        glEnd();
        CHECK_FOR_GL_ERROR();

        //        RenderDebugGeometry(f);
    }

    // last we release the final shader
//     if (currentShader != NULL)
//         currentShader->ReleaseShader();

    // disable textures if it has been enabled
    glBindTexture(GL_TEXTURE_2D, 0); // @todo, remove this if not needed, release texture
    glDisable(GL_TEXTURE_2D);
    CHECK_FOR_GL_ERROR();
}


void ShadowMapRenderingView::SetBackgroundColor(Vector<4,float> color) {
    backgroundColor = color;
}

Vector<4,float> ShadowMapRenderingView::GetBackgroundColor() {
    return backgroundColor;
}

PointLightNode* ShadowMapRenderingView::GetLightNode() {
    return lightNode;
}


} // NS OpenGL
} // NS Renderers
} // NS OpenEngine
