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
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _LIGHTINFO_H_
#include "lighting/lightInfo.h"
#endif
#ifndef _LIGHTFLAREDATA_H_
#include "T3D/lightFlareData.h"
#endif

class SphereMesh;
class TimeOfDay;
class CubemapData;
class MatrixSet;

class BasicSky : public SceneObject, public ISceneLight
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

   // ISceneLight
   virtual void submitLights(LightManager *lm, bool staticLighting);
   virtual LightInfo* getLight() { return mLight; }

   // ProcessObject
   virtual void advanceTime(F32 dt);

   ///
   void setAzimuth(F32 azimuth);

   ///
   void setElevation(F32 elevation);

   ///
   void setColor(const LinearColorF &color);

   ///
   void animate(F32 duration, F32 startAzimuth, F32 endAzimuth, F32 startElevation, F32 endElevation);

protected:

   F32 mSunAzimuth;

   F32 mSunElevation;

   LinearColorF mLightColor;

   LinearColorF mLightAmbient;

   F32 mBrightness;

   bool mAnimateSun;
   F32  mTotalTime;
   F32  mCurrTime;
   F32  mStartAzimuth;
   F32  mEndAzimuth;
   F32  mStartElevation;
   F32  mEndElevation;

   bool mCastShadows;
   S32 mStaticRefreshFreq;
   S32 mDynamicRefreshFreq;

   LightInfo *mLight;

   LightFlareData *mFlareData;
   LightFlareState mFlareState;
   F32 mFlareScale;

   bool mCoronaEnabled;
   String mCoronaMatName;
   BaseMatInstance *mCoronaMatInst;
   F32 mCoronaScale;
   LinearColorF mCoronaTint;
   bool mCoronaUseLightColor;

   // These are not user specified.
   // These hold data calculated once used across several methods.
   F32 mCoronaWorldRadius;
   Point3F mLightWorldPos;

   void _conformLights();
   void _initCorona();
   void _renderCorona(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);
   void _updateTimeOfDay(TimeOfDay *timeOfDay, F32 time);

   void _render(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);
   void _renderMoon(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);

   void _initVBIB();
   bool _initShader();
   void _initMoon();
   void _initCurves();

   void _generateSkyPoints();

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

   F32 mSphereInnerRadius;
   F32 mSphereOuterRadius;

   F32 mDayToNightLerpFactor;

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
   GFXShaderConstHandle *mCubeLerpFactors;            // vec3 - x/y/z contain 0.0-1.0f interpolate factors used to blend between the three cubemap textures (day, night, storm)

};

#endif // _SCATTERSKY_H_
