# Create the extension library
ADD_LIBRARY(Extensions_OpenGLShadowMapping
 
  Renderers/OpenGL/ShadowMapRenderer.cpp
  Renderers/OpenGL/ShadowMapRenderingView.cpp
  Renderers/OpenGL/ShadowRenderingView.cpp
)

TARGET_LINK_LIBRARIES(Extensions_OpenGLShadowMapping
  OpenEngine_Core
  OpenEngine_Scene
  OpenEngine_Geometry
  OpenEngine_Renderers
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARIES}
  ${SDL_LIBRARY}
)
