#include "renderBGShapeMeshTranslucentMgr.h"
#include "materials/sceneData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "math/util/matrixSet.h"

IMPLEMENT_CONOBJECT(RenderTranslucentBackdropMeshMgr);


RenderTranslucentBackdropMeshMgr::RenderTranslucentBackdropMeshMgr()
   : RenderBinManager(RenderPassManager::RIT_BackgroundShapeMeshTranslucent, 1.0f, 1.0f)
{
}

RenderTranslucentBackdropMeshMgr::RenderTranslucentBackdropMeshMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder)
   : RenderBinManager(riType, renderOrder, processAddOrder)
{
}

// taken from RenderTranslucentMgr::setupSGData
void RenderTranslucentBackdropMeshMgr::setupSGData(MeshRenderInst* ri, SceneData& data)
{
   Parent::setupSGData(ri, data);

   // We do not support these in the translucent bin.
   data.backBuffTex = NULL;
   data.cubemap = NULL;
   data.lightmap = NULL;
}

// based on RenderTranslucentMgr::addElement
void RenderTranslucentBackdropMeshMgr::addElement(RenderInst* inst)
{
   // Right off the bat if its not translucent skip it.
   if (!inst->translucentSort)
      return;

   // make sure it's the right type
   if (inst->type != RenderPassManager::RIT_BackgroundShapeMeshTranslucent)
      return;

   // Get its material.
   BaseMatInstance* matInst = NULL;
   matInst = static_cast<MeshRenderInst*>(inst)->matInst;

   // If the material isn't translucent the skip it.
   if (matInst && !matInst->getMaterial()->isTranslucent())
      return;

   // We made it this far, add the instance.
   mElementList.increment();
   MainSortElem& elem = mElementList.last();
   elem.inst = inst;

   // Override the instances default key to be the sort distance. All 
   // the pointer dereferencing is in there to prevent us from losing
   // information when converting to a U32.
   elem.key = *((U32*)&inst->sortDistSq);

   AssertFatal(inst->defaultKey != 0, "RenderTranslucentBackdropMeshMgr::addElement() - Got null sort key... did you forget to set it?");

   // Then use the instances primary key as our secondary key
   elem.key2 = inst->defaultKey;
}

// Based on RenderTranslucentMgr::render, but with unnecessary particle system and object stuff ripped out
void RenderTranslucentBackdropMeshMgr::render(SceneRenderState* state)
{
   PROFILE_SCOPE(RenderTranslucentBackdropMeshMgr_render);

   // Early out if nothing to draw.
   if (!mElementList.size())
      return;

   GFXDEBUGEVENT_SCOPE(RenderTranslucentBackdropMeshMgr_Render, ColorI::BLUE);

   GFXTransformSaver saver;

   SceneData sgData;
   sgData.init(state);

   // Restore transforms
   MatrixSet& matrixSet = getRenderPass()->getMatrixSet();
   matrixSet.restoreSceneViewProjection();

   U32 binSize = mElementList.size();
   for (U32 j = 0; j < binSize; )
   {
      RenderInst* baseRI = mElementList[j].inst;

      U32 matListEnd = j;

      if (baseRI->type == RenderPassManager::RIT_BackgroundShapeMeshTranslucent)
      {
         MeshRenderInst* ri = static_cast<MeshRenderInst*>(baseRI);
         BaseMatInstance* mat = ri->matInst;

         setupSGData(ri, sgData);

         while (mat->setupPass(state, sgData))
         {
            U32 a;
            for (a = j; a < binSize; a++)
            {
               RenderInst* nextRI = mElementList[a].inst;
               if (nextRI->type != RenderPassManager::RIT_BackgroundShapeMeshTranslucent)
                  break;

               MeshRenderInst* passRI = static_cast<MeshRenderInst*>(nextRI);

               // Check to see if we need to break this batch.
               if (newPassNeeded(ri, passRI))
                  break;

               // Z sorting and stuff is still not working in this mgr...
               setupSGData(passRI, sgData);
               mat->setSceneInfo(state, sgData);
               matrixSet.setWorld(*passRI->objectToWorld);
               matrixSet.setView(*passRI->worldToCamera);
               matrixSet.setProjection(*passRI->projection);
               mat->setTransforms(matrixSet, state);

               // If we're instanced then don't render yet.
               if (mat->isInstanced())
               {
                  // Let the material increment the instance buffer, but
                  // break the batch if it runs out of room for more.
                  if (!mat->stepInstance())
                  {
                     a++;
                     break;
                  }

                  continue;
               }

               // Setup the vertex and index buffers.
               mat->setBuffers(passRI->vertBuff, passRI->primBuff);

               // Render this sucker.
               if (passRI->prim)
                  GFX->drawPrimitive(*passRI->prim);
               else
                  GFX->drawPrimitive(passRI->primBuffIndex);
            }

            // Draw the instanced batch.
            if (mat->isInstanced())
            {
               // Sets the buffers including the instancing stream.
               mat->setBuffers(ri->vertBuff, ri->primBuff);

               // Render the instanced stream.
               if (ri->prim)
                  GFX->drawPrimitive(*ri->prim);
               else
                  GFX->drawPrimitive(ri->primBuffIndex);
            }

            matListEnd = a;
         }

         // force increment if none happened, otherwise go to end of batch
         j = (j == matListEnd) ? j + 1 : matListEnd;
      }
   }
}
