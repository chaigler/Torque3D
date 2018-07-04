//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/fx/fxSpaceGrid.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"
#include "T3D/gameBase/gameConnection.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "console/engineAPI.h"

//------------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(fxSpaceGrid);

ConsoleDocClass( fxSpaceGrid,
   "@brief Simply draws a grid.\n"
   "@ingroup Foliage\n"
);

//------------------------------------------------------------------------------
// Class: fxSpaceGrid
//------------------------------------------------------------------------------

fxSpaceGrid::fxSpaceGrid()
{
   // Setup NetObject.
   mTypeMask |= StaticObjectType | StaticShapeObjectType;
   mNetFlags.set(Ghostable | ScopeAlways);

   mGridColor.set(0.8f, 0.5f, 0.1f, 1.0f);
   mGridStep.set(128.0f, 128.0f);
   mGridSize.set(1024.0f, 1024.0f);
}

//------------------------------------------------------------------------------

fxSpaceGrid::~fxSpaceGrid()
{
}

//------------------------------------------------------------------------------

void fxSpaceGrid::initPersistFields()
{
   // Add out own persistent fields.
   addGroup( "Grid" );	// MM: Added Group Header.
      addField( "color",               TypeColorF,    Offset( mGridColor,                       fxSpaceGrid ), "Color of the grid lines." );
      addField( "gridStep",            TypePoint2F,   Offset( mGridStep,                        fxSpaceGrid),  "X/Y spacing between grid lines.");
      addField( "gridSize",            TypePoint2F,   Offset(  mGridSize,                       fxSpaceGrid),  "X/Y size of grid.");
   endGroup( "Grid" );	// MM: Added Group Footer.

   // Initialise parents' persistent fields.
   Parent::initPersistFields();
}

//------------------------------------------------------------------------------

bool fxSpaceGrid::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   setGlobalBounds();
   resetWorldBox();

   // Add to Scene.
   addToScene();

   return true;
}

//------------------------------------------------------------------------------

void fxSpaceGrid::onRemove()
{
   removeFromScene();

   // Do Parent.
   Parent::onRemove();
}

//------------------------------------------------------------------------------

void fxSpaceGrid::inspectPostApply()
{
   // Set Parent.
   Parent::inspectPostApply();

   setMaskBits(ReplicationMask);
}

//------------------------------------------------------------------------------

void fxSpaceGrid::prepRenderImage( SceneRenderState* state )
{
   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &fxSpaceGrid::renderObject);
   ri->type = RenderPassManager::RIT_Foliage;

   state->getRenderPass()->addInst( ri );
}

//------------------------------------------------------------------------------

void fxSpaceGrid::renderObject(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance* overrideMat)
{
   if (overrideMat)
      return;

   GFXTransformSaver saver;

   // Calculate our object to world transform matrix
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale(getScale());

   // Apply our object transform
   GFX->multWorld(objectToWorld);

   GFXStateBlockDesc transparent;
   transparent.setCullMode(GFXCullNone);
   transparent.alphaTestEnable = true;
   transparent.setZReadWrite(true);
   transparent.zWriteEnable = false;
   transparent.setBlend(true, GFXBlendSrcAlpha, GFXBlendOne);

   GFXDrawUtil *drawer = GFX->getDrawUtil();
   AssertFatal(drawer, "Got NULL GFXDrawUtil!");
   drawer->drawPlaneGrid(transparent, getPosition(), mGridSize, mGridStep, mGridColor.toColorI());
}

//------------------------------------------------------------------------------

U32 fxSpaceGrid::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   // Pack Parent.
   U32 retMask = Parent::packUpdate(con, mask, stream);

   // Write Replication Flag.
   if (stream->writeFlag(mask & ReplicationMask))
   {
      stream->writeAffineTransform(mObjToWorld);						// Replicator Position.
      stream->write(mGridColor);
      mathWrite(*stream, mGridStep);
      mathWrite(*stream, mGridSize);
   }

   // Were done ...
   return(retMask);
}

//------------------------------------------------------------------------------

void fxSpaceGrid::unpackUpdate(NetConnection * con, BitStream * stream)
{
   // Unpack Parent.
   Parent::unpackUpdate(con, stream);

   // Read Replication Details.
   if(stream->readFlag())
   {
      MatrixF		ReplicatorObjectMatrix;
      stream->readAffineTransform(&ReplicatorObjectMatrix);				// Replication Position.
      stream->read(&mGridColor);
      mathRead(*stream, &mGridStep);
      mathRead(*stream, &mGridSize);

      // Set Transform.
      setTransform(ReplicatorObjectMatrix);

   }
}

