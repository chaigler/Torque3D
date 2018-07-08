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

#ifndef _BASICSKY_H_
#define _BASICSKY_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif
#ifndef _RENDERPASSMANAGER_H_
#include "renderInstance/renderPassManager.h"
#endif
#ifndef _PRIMBUILDER_H_
#include "gfx/primBuilder.h"
#endif
#ifndef _TRESPONSECURVE_H_
#include "math/util/tResponseCurve.h"
#endif

class SphereMesh;
class TimeOfDay;
class CubemapData;
class MatrixSet;

class BasicSky : public SceneObject
{
   typedef SceneObject Parent;

public:

   enum MaskBits
   {
      UpdateMask = Parent::NextFreeMask,
      TimeMask = Parent::NextFreeMask << 1,
      NextFreeMask = Parent::NextFreeMask << 2
   };

   BasicSky();
   ~BasicSky();

   // SimObject
   bool onAdd();
   void onRemove();

   // ConsoleObject
   DECLARE_CONOBJECT(BasicSky);
   void inspectPostApply();
   static void initPersistFields();

   // Network
   U32  packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn, BitStream *stream);

   void prepRenderImage(SceneRenderState* state);

protected:

   void _render(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);
   void _renderMoon(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);

   void _initVBIB();
   bool _initShader();
   void _initMoon();
   void _initCurves();

   void _generateSkyPoints();

   void _updateTimeOfDay(TimeOfDay *timeofDay, F32 time);

protected:

   static const F32 smEarthRadius;
   static const F32 smAtmosphereRadius;
   static const F32 smViewerHeight;

#define CURVE_COUNT 5

   FloatCurve mCurves[CURVE_COUNT];

   U32 mVertCount;
   U32 mPrimCount;

   F32 mOuterRadius;
   F32 mScale;

   SimObjectPtr<Sun> mSun;

   F32 mSphereInnerRadius;
   F32 mSphereOuterRadius;

   F32 mNightInterpolant;
   F32 mZOffset;

   F32 mMoonAzimuth;
   F32 mMoonElevation;

   F32 mTimeOfDay;

   bool mDirty;

   String mDayCubeMapName;
   CubemapData *mDayCubeMap;

   bool mMoonEnabled;
   String mMoonMatName;
   BaseMatInstance *mMoonMatInst;
   F32 mMoonScale;
   LinearColorF mMoonTint;
   VectorF mMoonLightDir;
   CubemapData *mNightCubemap;
   String mNightCubemapName;
   bool mUseNightCubemap;
   MatrixSet *mMatrixSet;

   Vector<Point3F> mSkyPoints;

   // Prim buffer, vertex buffer and shader for rendering.
   GFXPrimitiveBufferHandle mPrimBuffer;
   GFXVertexBufferHandle<GFXVertexP> mVB;
   GFXShaderRef mShader;

   GFXStateBlockRef mStateBlock;

   // Shared shader constant blocks
   GFXShaderConstBufferRef mShaderConsts;
   GFXShaderConstHandle *mModelViewProjSC;
   GFXShaderConstHandle *mSphereRadiiSC;              // Inner and out radius, and inner and outer radius squared.
   GFXShaderConstHandle *mCamPosSC;
   GFXShaderConstHandle *mNightColorSC;
   GFXShaderConstHandle *mNightInterpolantAndExposureSC;
   GFXShaderConstHandle *mUseCubemapSC;

};

#endif // _SCATTERSKY_H_
