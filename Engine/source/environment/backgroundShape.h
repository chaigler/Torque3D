#ifndef _BACKDROP_SHAPE_H_
#define _BACKDROP_SHAPE_H_

#ifndef _TSSTATIC_H_
#include "T3D/tsStatic.h"
#endif

// Backdrop shape is designed to represent large, distant objects 
// which do not move relative to the viewer, and cannot be interacted 
// with by the player.
// Backdrop shape inherits from TSStatic, and overrides the rendering to
// keep the shape at a constant position relative to the camera.
class BackgroundShape : public TSStatic
{
   typedef TSStatic Parent;

protected:
   bool onAdd();

public:
   BackgroundShape();

   void prepRenderImage(SceneRenderState* state);

   DECLARE_CONOBJECT(BackgroundShape);
};

#endif
