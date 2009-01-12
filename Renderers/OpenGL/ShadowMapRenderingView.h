// OpenGL shadow map rendering view.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _OPENGL_SHADOW_MAP_RENDERING_VIEW_H_
#define _OPENGL_SHADOW_MAP_RENDERING_VIEW_H_

#include <Meta/OpenGL.h>
#include <Renderers/IRenderingView.h>
#include <vector>

namespace OpenEngine {
namespace Renderers {
namespace OpenGL {

using namespace OpenEngine::Renderers;
using namespace OpenEngine::Scene;
using namespace OpenEngine::Geometry;
using namespace std;

/**
 * Concrete RenderingView using OpenGL.
 */
class ShadowMapRenderingView : virtual public IRenderingView {
    IRenderer* renderer;


public:
    ShadowMapRenderingView(Viewport& viewport);
    virtual ~ShadowMapRenderingView();
    void VisitGeometryNode(GeometryNode* node);
    void VisitTransformationNode(TransformationNode* node);
    void Render(IRenderer* renderer, ISceneNode* root);
    void Handle(RenderingEventArg arg);
    IRenderer* GetRenderer();
    virtual void SetBackgroundColor(Vector<4,float> color);
    virtual Vector<4,float> GetBackgroundColor();
    PointLightNode* GetLightNode();

private:
    Vector<4,float> backgroundColor;
    PointLightNode* lightNode;

    inline void ApplyRenderState(RenderStateNode* node);
};

} // NS OpenGL
} // NS Renderers
} // NS OpenEngine

#endif // _OPENGL_SHADOW_MAP_RENDERING_VIEW_H_
