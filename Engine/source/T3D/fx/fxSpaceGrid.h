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

#ifndef _FX_SPACEGRID_H_
#define _FX_SPACEGRID_H_

#ifndef _TSSTATIC_H_
#include "T3D/tsStatic.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif
#ifndef _RENDERPASSMANAGER_H_
#include "renderInstance/renderPassManager.h"
#endif

//------------------------------------------------------------------------------
// Class: fxShapeReplicator
//------------------------------------------------------------------------------
class fxSpaceGrid : public SceneObject
{
private:
   typedef SceneObject      Parent;

   LinearColorF mGridColor;
   Point2F mGridStep;
   Point2F mGridSize;

protected:
   enum {   ReplicationMask   = (1 << 0) };

public:
   fxSpaceGrid();
   ~fxSpaceGrid();

   void renderObject(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance*);

   // SceneObject
   virtual void prepRenderImage( SceneRenderState *state );

   // SimObject
   bool onAdd();
   void onRemove();
   void inspectPostApply();

   // NetObject
   U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn, BitStream *stream);

   // ConObject.
   static void initPersistFields();

   // Declare Console Object.
   DECLARE_CONOBJECT(fxSpaceGrid);
};

#endif // _FX_SPACEDUST_H_
