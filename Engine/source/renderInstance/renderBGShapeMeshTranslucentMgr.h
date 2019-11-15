#ifndef _RENDERTRANSLUCENTBACKDROPMESHMGR_H_
#define _RENDERTRANSLUCENTBACKDROPMESHMGR_H_

#ifndef _RENDER_TRANSLUCENT_MGR_H_
#include "renderInstance\renderTranslucentMgr.h"
#endif

// TBD: Translucent background shapes don't work.
class RenderTranslucentBackdropMeshMgr : public RenderBinManager
{
   typedef RenderBinManager Parent;

public:
   RenderTranslucentBackdropMeshMgr();
   RenderTranslucentBackdropMeshMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder);

   void render(SceneRenderState* state);
   void addElement(RenderInst* inst);
   void setupSGData(MeshRenderInst* ri, SceneData& data);

   DECLARE_CONOBJECT(RenderTranslucentBackdropMeshMgr);
};

#endif
