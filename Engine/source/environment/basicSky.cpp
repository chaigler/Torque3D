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
#include "basicSky.h"

#include "core/stream/bitStream.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "math/util/sphereMesh.h"
#include "math/mathUtils.h"
#include "math/util/matrixSet.h"
#include "scene/sceneRenderState.h"
#include "lighting/lightInfo.h"
#include "gfx/sim/gfxStateBlockData.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/sim/cubemapData.h"
#include "materials/shaderData.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "materials/sceneData.h"
#include "environment/timeOfDay.h"
#include "environment/sun.h"


ConsoleDocClass(BasicSky,
   "@brief Represents both the sun and sky for scenes with a dynamic time of day.\n\n"

   "%BasicSky renders as a dome shaped mesh which is camera relative and always overhead. "
   "It is intended to be part of the background of your scene and renders before all "
   "other objects types.\n\n"

   "%BasicSky is designed for outdoor scenes which need to transition fluidly "
   "between radically different times of day. It will respond to time changes "
   "originating from a TimeOfDay object or the elevation field can be directly "
   "adjusted.\n\n"

   "During day, %BasicSky uses atmosphereic sunlight scattering "
   "aproximations to generate a sky gradient and sun corona. It also calculates "
   "the fog color, ambient color, and sun color, which are used for scene "
   "lighting. This is user controlled by fields within the BasicSky group.\n\n"

   "During night, %BasicSky supports can transition to a night sky cubemap and "
   "moon sprite. The user can control this and night time colors used for scene "
   "lighting with fields within the Night group.\n\n"

   "A scene with a BasicSky should not have any other sky or sun objects "
   "as it already fulfills both roles.\n\n"

   "%BasicSky is intended to be used with CloudLayer and TimeOfDay as part of "
   "a scene with dynamic lighting. Having a %BasicSky without a changing "
   "time of day would unnecessarily give up artistic control compared and fillrate "
   "compared to a SkyBox + Sun setup.\n\n"

   "@ingroup Atmosphere"
);


IMPLEMENT_CO_NETOBJECT_V1(BasicSky);

const F32 BasicSky::smEarthRadius = (6378.0f * 1000.0f);
const F32 BasicSky::smAtmosphereRadius = 200000.0f;
const F32 BasicSky::smViewerHeight = 1.0f;

BasicSky::BasicSky()
{
   mPrimCount = 0;
   mVertCount = 0;

   mSphereInnerRadius = 1.0f;
   mSphereOuterRadius = 1.0f * 1.025f;
   mScale = 1.0f / (mSphereOuterRadius - mSphereInnerRadius);

   mSun = NULL;

   mNightInterpolant = 0;
   mZOffset = 0.0f;

   mShader = NULL;

   mTimeOfDay = 0;

   mMoonAzimuth = 0.0f;
   mMoonElevation = 45.0f;

   mDirty = true;

   mDayCubeMapName = "";
   mDayCubeMap = NULL;

   mMoonEnabled = true;
   mMoonScale = 0.2f;
   mMoonTint.set(0.192157f, 0.192157f, 0.192157f, 1.0f);
   MathUtils::getVectorFromAngles(mMoonLightDir, 0.0f, 45.0f);
   mMoonLightDir.normalize();
   mMoonLightDir = -mMoonLightDir;
   mNightCubemap = NULL;
   mUseNightCubemap = false;

   mMoonMatInst = NULL;

   mNetFlags.set(Ghostable | ScopeAlways);
   mTypeMask |= EnvironmentObjectType | LightObjectType | StaticObjectType;

   _generateSkyPoints();

   mMatrixSet = reinterpret_cast<MatrixSet *>(dMalloc_aligned(sizeof(MatrixSet), 16));
   constructInPlace(mMatrixSet);
}

BasicSky::~BasicSky()
{
   SAFE_DELETE(mMoonMatInst);

   dFree_aligned(mMatrixSet);
}

bool BasicSky::onAdd()
{
   PROFILE_SCOPE(BasicSky_onAdd);

   // onNewDatablock for the server is called here
   // for the client it is called in unpackUpdate

   if (!Parent::onAdd())
      return false;

   setGlobalBounds();
   resetWorldBox();

   Sun* pSun = new Sun;
   if (pSun->registerObject() == false)
   {
      Con::warnf(ConsoleLogEntry::General, "Could not register Sun for BasicSky object!");
      delete pSun;
      pSun = NULL;
   }
   mSun = pSun;

   SimGroup *missionGroup;
   if (!Sim::findObject("MissionGroup", missionGroup))
      Con::errorf("GuiMeshRoadEditorCtrl - could not find MissionGroup to add new Sun");
   else
      missionGroup->addObject(mSun);

   addToScene();

   if (isClientObject())
   {
      TimeOfDay::getTimeOfDayUpdateSignal().notify(this, &BasicSky::_updateTimeOfDay);

      Sim::findObject(mDayCubeMapName, mDayCubeMap);
      _initMoon();
      Sim::findObject(mNightCubemapName, mNightCubemap);
   }

   return true;
}

void BasicSky::onRemove()
{
   if (!mSun.isNull())
   {
      mSun->safeDeleteObject();
      mSun = NULL;
   }

   removeFromScene();

   if (isClientObject())
      TimeOfDay::getTimeOfDayUpdateSignal().remove(this, &BasicSky::_updateTimeOfDay);

   Parent::onRemove();
}

void BasicSky::inspectPostApply()
{
   mDirty = true;
   setMaskBits(0xFFFFFFFF);
}

void BasicSky::initPersistFields()
{
   addGroup("BasicSky",
      "Only azimuth and elevation are networked fields. To trigger a full update of all other fields use the applyChanges ConsoleMethod.");

   addField("zOffset", TypeF32, Offset(mZOffset, BasicSky),
      "Offsets the BasicSky to avoid canvas rendering. Use 5000 or greater for the initial adjustment");

   endGroup("BasicSky");

   addGroup("Orbit");

   addField("moonAzimuth", TypeF32, Offset(mMoonAzimuth, BasicSky),
      "The horizontal angle of the moon measured clockwise from the positive Y world axis. This is not animated by time or networked.");

   addField("moonElevation", TypeF32, Offset(mMoonElevation, BasicSky),
      "The elevation angle of the moon above or below the horizon. This is not animated by time or networked.");

   endGroup("Orbit");

   addGroup("Night");

   addField("moonEnabled", TypeBool, Offset(mMoonEnabled, BasicSky),
      "Enable or disable rendering of the moon sprite during night.");

   addField("moonMat", TypeMaterialName, Offset(mMoonMatName, BasicSky),
      "Material for the moon sprite.");

   addField("moonScale", TypeF32, Offset(mMoonScale, BasicSky),
      "Controls size the moon sprite renders, specified as a fractional amount of the screen height.");

   addField("moonLightColor", TypeColorF, Offset(mMoonTint, BasicSky),
      "Color of light cast by the directional light during night.");

   addField("useNightCubemap", TypeBool, Offset(mUseNightCubemap, BasicSky),
      "Transition to the nightCubemap during night. If false we use nightColor.");

   addField("dayCubemap", TypeCubemapName, Offset(mDayCubeMapName, BasicSky),
      "Cubemap visible during day.");

   addField("nightCubemap", TypeCubemapName, Offset(mNightCubemapName, BasicSky),
      "Cubemap visible during night.");

   endGroup("Night");

   Parent::initPersistFields();
}

U32 BasicSky::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (stream->writeFlag(mask & TimeMask))
   {
   }

   if (stream->writeFlag(mask & UpdateMask))
   {
      stream->write(mSphereInnerRadius);
      stream->write(mSphereOuterRadius);

      stream->write(mScale);

      stream->write(mZOffset);

      stream->write(mDayCubeMapName);
      stream->writeFlag(mMoonEnabled);
      stream->write(mMoonMatName);
      stream->write(mMoonScale);
      stream->write(mMoonTint);
      stream->writeFlag(mUseNightCubemap);
      stream->write(mNightCubemapName);

      stream->write(mMoonAzimuth);
      stream->write(mMoonElevation);
   }

   return retMask;
}

void BasicSky::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag()) // TimeMask
   {
   }

   if (stream->readFlag()) // UpdateMask
   {
      stream->read(&mSphereInnerRadius);
      stream->read(&mSphereOuterRadius);

      stream->read(&mScale);

      stream->read(&mZOffset);

      stream->read(&mDayCubeMapName);
      mMoonEnabled = stream->readFlag();
      stream->read(&mMoonMatName);
      stream->read(&mMoonScale);
      stream->read(&mMoonTint);
      mUseNightCubemap = stream->readFlag();
      stream->read(&mNightCubemapName);

      stream->read(&mMoonAzimuth);
      stream->read(&mMoonElevation);

      if (isProperlyAdded())
      {
         mDirty = true;
         Sim::findObject(mDayCubeMapName, mDayCubeMap);
         _initMoon();
         Sim::findObject(mNightCubemapName, mNightCubemap);
      }
   }
}

void BasicSky::prepRenderImage(SceneRenderState *state)
{
   // Only render into diffuse and reflect passes.

   if (!state->isDiffusePass() &&
      !state->isReflectPass())
      return;

   // Regular sky render instance.
   RenderPassManager* renderPass = state->getRenderPass();
   ObjectRenderInst *ri = renderPass->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &BasicSky::_render);
   ri->type = RenderPassManager::RIT_Sky;
   ri->defaultKey = 10;
   ri->defaultKey2 = 0;
   renderPass->addInst(ri);

   // Render instances for Night effects.
   if (mNightInterpolant <= 0.0f)
      return;

   // Render instance for Moon sprite.
   if (mMoonEnabled && mMoonMatInst)
   {
      mMatrixSet->setSceneView(GFX->getWorldMatrix());
      mMatrixSet->setSceneProjection(GFX->getProjectionMatrix());
      mMatrixSet->setWorld(GFX->getWorldMatrix());

      ObjectRenderInst *moonRI = renderPass->allocInst<ObjectRenderInst>();
      moonRI->renderDelegate.bind(this, &BasicSky::_renderMoon);
      moonRI->type = RenderPassManager::RIT_Sky;
      // Render after sky objects and before CloudLayer!
      moonRI->defaultKey = 5;
      moonRI->defaultKey2 = 0;
      renderPass->addInst(moonRI);
   }
}

bool BasicSky::_initShader()
{
   ShaderData *shaderData;
   if (!Sim::findObject("BasicSkyShaderData", shaderData))
   {
      Con::warnf("BasicSky::_initShader - failed to locate shader BasicSkyShaderData!");
      return false;
   }
   Vector<GFXShaderMacro> macros;

   mShader = shaderData->getShader(macros);

   if (!mShader)
      return false;

   if (mStateBlock.isNull())
   {
      GFXStateBlockData *data = NULL;
      if (!Sim::findObject("BasicSkySBData", data))
         Con::warnf("BasicSky::_initShader - failed to locate BasicSkySBData!");
      else
         mStateBlock = GFX->createStateBlock(data->getState());
   }

   if (!mStateBlock)
      return false;

   mShaderConsts = mShader->allocConstBuffer();
   mModelViewProjSC = mShader->getShaderConstHandle("$modelView");

   // Inner and out radius, and inner and outer radius squared.
   mSphereRadiiSC = mShader->getShaderConstHandle("$sphereRadii");

   mCamPosSC = mShader->getShaderConstHandle("$camPos");
   mNightColorSC = mShader->getShaderConstHandle("$nightColor");
   mNightInterpolantAndExposureSC = mShader->getShaderConstHandle("$nightInterpAndExposure");
   mUseCubemapSC = mShader->getShaderConstHandle("$useCubemap");

   return true;
}

void BasicSky::_initVBIB()
{
   // Vertex Buffer...
   U32 vertStride = 50;
   U32 strideMinusOne = vertStride - 1;
   mVertCount = vertStride * vertStride;
   mPrimCount = strideMinusOne * strideMinusOne * 2;

   Point3F vertScale(16.0f, 16.0f, 4.0f);

   F32 zOffset = -(mCos(mSqrt(1.0f)) + 0.01f);

   mVB.set(GFX, mVertCount, GFXBufferTypeStatic);
   GFXVertexP *pVert = mVB.lock();
   if (!pVert) return;

   for (U32 y = 0; y < vertStride; y++)
   {
      F32 v = ((F32)y / (F32)strideMinusOne - 0.5f) * 2.0f;

      for (U32 x = 0; x < vertStride; x++)
      {
         F32 u = ((F32)x / (F32)strideMinusOne - 0.5f) * 2.0f;

         F32 sx = u;
         F32 sy = v;
         F32 sz = (mCos(mSqrt(sx*sx + sy * sy)) * 1.0f) + zOffset;
         //F32 sz = 1.0f;
         pVert->point.set(sx, sy, sz);
         pVert->point *= vertScale;

         pVert->point.normalize();
         pVert->point *= 200000.0f;

         pVert++;
      }
   }

   mVB.unlock();

   // Primitive Buffer...
   mPrimBuffer.set(GFX, mPrimCount * 3, mPrimCount, GFXBufferTypeStatic);

   U16 *pIdx = NULL;
   mPrimBuffer.lock(&pIdx);
   U32 curIdx = 0;

   for (U32 y = 0; y < strideMinusOne; y++)
   {
      for (U32 x = 0; x < strideMinusOne; x++)
      {
         U32 offset = x + y * vertStride;

         pIdx[curIdx] = offset;
         curIdx++;
         pIdx[curIdx] = offset + 1;
         curIdx++;
         pIdx[curIdx] = offset + vertStride + 1;
         curIdx++;

         pIdx[curIdx] = offset;
         curIdx++;
         pIdx[curIdx] = offset + vertStride + 1;
         curIdx++;
         pIdx[curIdx] = offset + vertStride;
         curIdx++;
      }
   }

   mPrimBuffer.unlock();
}

void BasicSky::_initMoon()
{
   if (isServerObject())
      return;

   if (mMoonMatInst)
      SAFE_DELETE(mMoonMatInst);

   if (mMoonMatName.isNotEmpty())
      mMoonMatInst = MATMGR->createMatInstance(mMoonMatName, MATMGR->getDefaultFeatures(), getGFXVertexFormat<GFXVertexPCT>());
}

void BasicSky::_initCurves()
{
   if (mCurves->getSampleCount() > 0)
      return;

   // Takes time of day (0-2) and returns
   // the night interpolant (0-1) day/night factor.
   // moonlight = 0, sunlight > 0
   mCurves[0].clear();
   mCurves[0].addPoint(0.0f, 0.5f);// Sunrise
   mCurves[0].addPoint(0.025f, 1.0f);//
   mCurves[0].addPoint(0.975f, 1.0f);//
   mCurves[0].addPoint(1.0f, 0.5f);//Sunset
   mCurves[0].addPoint(1.02f, 0.0f);//Sunlight ends
   mCurves[0].addPoint(1.98f, 0.0f);//Sunlight begins
   mCurves[0].addPoint(2.0f, 0.5f);// Sunrise

                                   //  Takes time of day (0-2) and returns mieScattering factor
                                   //   Regulates the size of the sun's disk
   mCurves[1].clear();
   mCurves[1].addPoint(0.0f, 0.0006f);
   mCurves[1].addPoint(0.01f, 0.00035f);
   mCurves[1].addPoint(0.03f, 0.00023f);
   mCurves[1].addPoint(0.1f, 0.00022f);
   mCurves[1].addPoint(0.2f, 0.00043f);
   mCurves[1].addPoint(0.3f, 0.00062f);
   mCurves[1].addPoint(0.4f, 0.0008f);
   mCurves[1].addPoint(0.5f, 0.00086f);// High noon
   mCurves[1].addPoint(0.6f, 0.0008f);
   mCurves[1].addPoint(0.7f, 0.00062f);
   mCurves[1].addPoint(0.8f, 0.00043f);
   mCurves[1].addPoint(0.9f, 0.00022f);
   mCurves[1].addPoint(0.97f, 0.00023f);
   mCurves[1].addPoint(0.99f, 0.00035f);
   mCurves[1].addPoint(1.0f, 0.0006f);
   mCurves[1].addPoint(2.0f, 0.0006f);

   // Takes time of day and returns brightness
   // Controls sunlight and moonlight brightness
   mCurves[2].clear();
   mCurves[2].addPoint(0.0f, 0.2f);// Sunrise
   mCurves[2].addPoint(0.1f, 1.0f);
   mCurves[2].addPoint(0.9f, 1.0f);// Sunset
   mCurves[2].addPoint(1.008f, 0.0f);//Adjust end of sun's reflection
   mCurves[2].addPoint(1.02001f, 0.0f);
   mCurves[2].addPoint(1.05f, 0.5f);// Turn brightness up for moonlight
   mCurves[2].addPoint(1.93f, 0.5f);
   mCurves[2].addPoint(1.97999f, 0.0f);// No brightness when sunlight starts
   mCurves[2].addPoint(1.992f, 0.0f);//Adjust start of sun's reflection
   mCurves[2].addPoint(2.0f, 0.2f); // Sunrise

                                    // Interpolation of day/night color sets
                                    // 0/1  ambient/nightcolor
                                    // 0 = day colors only anytime
                                    // 1 = night colors only anytime
                                    // between 0 and 1 renders both color sets anytime

   mCurves[3].clear();
   mCurves[3].addPoint(0.0f, 0.8f);//Sunrise
   mCurves[3].addPoint(0.1f, 0.0f);
   mCurves[3].addPoint(0.99f, 0.0f);
   mCurves[3].addPoint(1.0f, 0.8f);// Sunset
   mCurves[3].addPoint(1.01999f, 1.0f);//
   mCurves[3].addPoint(1.98001f, 1.0f);// Sunlight begins with full night colors
   mCurves[3].addPoint(2.0f, 0.8f);  //Sunrise

                                     //  Takes time of day (0-2) and returns smoothing factor
                                     //  Interpolates between mMoonTint color and mNightColor

   mCurves[4].clear();
   mCurves[4].addPoint(0.0f, 1.0f);
   mCurves[4].addPoint(0.96f, 1.0f);
   mCurves[4].addPoint(1.01999f, 0.5f);
   mCurves[4].addPoint(1.02001f, 0.5f);
   mCurves[4].addPoint(1.08f, 1.0f);
   mCurves[4].addPoint(1.92f, 1.0f);
   mCurves[4].addPoint(1.97999f, 0.5f);
   mCurves[4].addPoint(1.98001f, 0.5f);
   mCurves[4].addPoint(2.0f, 1.0f);
}
void BasicSky::_updateTimeOfDay(TimeOfDay *timeOfDay, F32 time)
{
}

void BasicSky::_render(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat)
{
   if (overrideMat || (!mShader && !_initShader()))
      return;

   GFXTransformSaver saver;

   if (mVB.isNull() || mPrimBuffer.isNull())
      _initVBIB();

   GFX->setShader(mShader);
   GFX->setShaderConstBuffer(mShaderConsts);

   Point4F sphereRadii(mSphereOuterRadius, mSphereOuterRadius * mSphereOuterRadius,
      mSphereInnerRadius, mSphereInnerRadius * mSphereInnerRadius);

   Point3F camPos(0, 0, smViewerHeight);

   Frustum frust = state->getCameraFrustum();
   frust.setFarDist(smEarthRadius + smAtmosphereRadius);
   MatrixF proj(true);
   frust.getProjectionMatrix(&proj);

   Point3F camPos2 = state->getCameraPosition();
   MatrixF xfm(true);

   GFX->multWorld(xfm);
   MatrixF xform(proj);
   xform *= GFX->getViewMatrix();
   xform *= GFX->getWorldMatrix();

   if (state->isReflectPass())
   {
      static MatrixF rotMat(EulerF(0.0, 0.0, M_PI_F));
      xform.mul(rotMat);
      rotMat.set(EulerF(M_PI_F, 0.0, 0.0));
      xform.mul(rotMat);
   }
   xform.setPosition(xform.getPosition() - Point3F(0, 0, mZOffset));

   mShaderConsts->setSafe(mModelViewProjSC, xform);
   mShaderConsts->setSafe(mSphereRadiiSC, sphereRadii);
   mShaderConsts->setSafe(mCamPosSC, camPos);
   mShaderConsts->setSafe(mNightInterpolantAndExposureSC, Point2F(0.0f, mNightInterpolant));

   if (GFXDevice::getWireframe())
   {
      GFXStateBlockDesc desc(mStateBlock->getDesc());
      desc.setFillModeWireframe();
      GFX->setStateBlockByDesc(desc);
   }
   else
      GFX->setStateBlock(mStateBlock);

   if (mDayCubeMap)
   {
      if (!mDayCubeMap->mCubemap)
         mDayCubeMap->createMap();

      GFX->setCubeTexture(0, mDayCubeMap->mCubemap);
   }
   else
      GFX->setCubeTexture(0, NULL);

   if (mUseNightCubemap && mNightCubemap)
   {
      mShaderConsts->setSafe(mUseCubemapSC, 1.0f);

      if (!mNightCubemap->mCubemap)
         mNightCubemap->createMap();

      GFX->setCubeTexture(1, mNightCubemap->mCubemap);
   }
   else
   {
      GFX->setCubeTexture(1, NULL);
      mShaderConsts->setSafe(mUseCubemapSC, 0.0f);
   }

   GFX->setPrimitiveBuffer(mPrimBuffer);
   GFX->setVertexBuffer(mVB);

   GFX->drawIndexedPrimitive(GFXTriangleList, 0, 0, mVertCount, 0, mPrimCount);
}

void BasicSky::_renderMoon(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat)
{
   if (!mMoonMatInst)
      return;

   Point3F moonlightPosition = state->getCameraPosition() - /*mLight->getDirection()*/ mMoonLightDir * state->getFarPlane() * 0.9f;
   F32 dist = (moonlightPosition - state->getCameraPosition()).len();

   // worldRadius = screenRadius * dist / worldToScreen
   // screenRadius = worldRadius / dist * worldToScreen

   //
   F32 screenRadius = GFX->getViewport().extent.y * mMoonScale * 0.5f;
   F32 worldRadius = screenRadius * dist / state->getWorldToScreenScale().y;

   // Calculate Billboard Radius (in world units) to be constant, independent of distance.
   // Takes into account distance, viewport size, and specified size in editor

   F32 BBRadius = worldRadius;


   mMatrixSet->restoreSceneViewProjection();

   if (state->isReflectPass())
      mMatrixSet->setProjection(state->getSceneManager()->getNonClipProjection());

   mMatrixSet->setWorld(MatrixF::Identity);

   // Initialize points with basic info
   Point3F points[4];
   points[0] = Point3F(-BBRadius, 0.0, -BBRadius);
   points[1] = Point3F(-BBRadius, 0.0, BBRadius);
   points[2] = Point3F(BBRadius, 0.0, -BBRadius);
   points[3] = Point3F(BBRadius, 0.0, BBRadius);

   static const Point2F sCoords[4] =
   {
      Point2F(0.0f, 0.0f),
      Point2F(0.0f, 1.0f),
      Point2F(1.0f, 0.0f),
      Point2F(1.0f, 1.0f)
   };

   // Get info we need to adjust points
   const MatrixF &camView = state->getCameraTransform();

   // Finalize points
   for (S32 i = 0; i < 4; i++)
   {
      // align with camera
      camView.mulV(points[i]);
      // offset
      points[i] += moonlightPosition;
   }

   // Vertex color.
   LinearColorF moonVertColor(1.0f, 1.0f, 1.0f, mNightInterpolant);

   // Copy points to buffer.

   GFXVertexBufferHandle< GFXVertexPCT > vb;
   vb.set(GFX, 4, GFXBufferTypeVolatile);
   GFXVertexPCT *pVert = vb.lock();
   if (!pVert) return;

   for (S32 i = 0; i < 4; i++)
   {
      pVert->color.set(moonVertColor.toColorI());
      pVert->point.set(points[i]);
      pVert->texCoord.set(sCoords[i].x, sCoords[i].y);
      pVert++;
   }

   vb.unlock();

   // Setup SceneData struct.

   SceneData sgData;
   sgData.wireframe = GFXDevice::getWireframe();
   sgData.visibility = 1.0f;

   // Draw it

   while (mMoonMatInst->setupPass(state, sgData))
   {
      mMoonMatInst->setTransforms(*mMatrixSet, state);
      mMoonMatInst->setSceneInfo(state, sgData);

      GFX->setVertexBuffer(vb);
      GFX->drawPrimitive(GFXTriangleStrip, 0, 2);
   }
}

void BasicSky::_generateSkyPoints()
{
   U32 rings = 60, segments = 20;//rings=160, segments=20;

   Point3F tmpPoint(0, 0, 0);

   // Establish constants used in sphere generation.
   F32 deltaRingAngle = (M_PI_F / (F32)(rings * 2));
   F32 deltaSegAngle = (2.0f * M_PI_F / (F32)segments);

   // Generate the group of rings for the sphere.
   for (S32 ring = 0; ring < 2; ring++)
   {
      F32 r0 = mSin(ring * deltaRingAngle);
      F32 y0 = mCos(ring * deltaRingAngle);

      // Generate the group of segments for the current ring.
      for (S32 seg = 0; seg < segments + 1; seg++)
      {
         F32 x0 = r0 * sinf(seg * deltaSegAngle);
         F32 z0 = r0 * cosf(seg * deltaSegAngle);

         tmpPoint.set(x0, z0, y0);
         tmpPoint.normalizeSafe();

         tmpPoint.x *= smEarthRadius + smAtmosphereRadius;
         tmpPoint.y *= smEarthRadius + smAtmosphereRadius;
         tmpPoint.z *= smEarthRadius + smAtmosphereRadius;
         tmpPoint.z -= smEarthRadius;

         if (ring == 1)
            mSkyPoints.push_back(tmpPoint);
      }
   }
}

// ConsoleMethods

DefineEngineMethod(BasicSky, applyChanges, void, (), ,
   "Apply a full network update of all fields to all clients."
)
{
   object->inspectPostApply();
}