// OpenGL shadow rendering view.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _OPENGL_SHADOW_RENDERING_VIEW_H_
#define _OPENGL_SHADOW_RENDERING_VIEW_H_

#include <Meta/OpenGL.h>
#include <Renderers/IRenderingView.h>
#include <Scene/RenderStateNode.h>
#include <Scene/BlendingNode.h>
#include <vector>

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

using namespace OpenEngine::Renderers;
using namespace OpenEngine::Scene;
using namespace OpenEngine::Geometry;
using namespace std;

/**
 * Concrete ShadowRenderingView using OpenGL.
 */
class ShadowRenderingView : virtual public IRenderingView {
    IRenderer* renderer;
    vector<RenderStateNode*> stateStack;

    void RenderLine(Vector<3,float> vert,
                    Vector<3,float> norm,
                    Vector<3,float> color);

public:
    ShadowRenderingView(Viewport& viewport, Viewport& shadowMapViewport);
    virtual ~ShadowRenderingView();
    void VisitGeometryNode(GeometryNode* node);
    void VisitVertexArrayNode(VertexArrayNode* node);
    void VisitTransformationNode(TransformationNode* node);
    void VisitRenderStateNode(RenderStateNode* node);
    void VisitRenderNode(RenderNode* node);
    void VisitDisplayListNode(DisplayListNode* node);
    void VisitBlendingNode(BlendingNode* node);
    void Render(IRenderer* renderer, ISceneNode* root);
    void Handle(RenderingEventArg arg);
    IRenderer* GetRenderer();
    virtual void SetBackgroundColor(Vector<4,float> color);
    virtual Vector<4,float> GetBackgroundColor();

private:
    bool renderBinormal, renderTangent, renderSoftNormal, renderHardNormal;
    bool renderTexture, renderShader;
    int currentTexture;
    IShaderResourcePtr currentShader;
    int binormalid;
    int tangentid;

    Viewport& shadowMapViewport;

    Vector<4,float> backgroundColor;

    void EnableBlending(BlendingNode::BlendingFactor source, 
                        BlendingNode::BlendingFactor destination,
                        BlendingNode::BlendingEquation equation);
    void DisableBlending();

    inline void EnableBlending(GLenum source, GLenum destination,
                               GLenum eqation);
    inline GLenum ConvertBlendingFactor(BlendingNode::BlendingFactor factor);
    inline GLenum ConvertBlendingEquation(BlendingNode::BlendingEquation equation);
    inline void RenderDebugGeometry(FacePtr face);
    inline void RenderBinormals(FacePtr face);
    inline void RenderTangents(FacePtr face);
    inline void RenderNormals(FacePtr face);
    inline void RenderHardNormal(FacePtr face);
    inline void ApplyMaterial(MaterialPtr mat);
    inline void ApplyRenderState(RenderStateNode* node);
};

} // NS OpenGL
} // NS Renderers
} // NS OpenEngine

#endif // _OPENGL_SHADOW_RENDERING_VIEW_H_
