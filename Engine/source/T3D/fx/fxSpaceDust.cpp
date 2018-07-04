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
#include "../source/fx/fxSpaceDust.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "math/mRandom.h"
#include "math/mathIO.h"
#include "T3D/gameBase/gameConnection.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "console/engineAPI.h"

//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/editor.cs in function [Editor::create()] (around line 66).
//
//   // Ignore Replicated fxStatic Instances.
//   EWorldEditor.ignoreObjClass("fxShapeReplicatedStatic");
//
//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/EditorGui.cs in [function Creator::init( %this )]
//
//   %Environment_Item[8] = "fxSpaceDust";  <-- ADD THIS.
//
//------------------------------------------------------------------------------
//
//	Put the function in /example/common/editor/ObjectBuilderGui.gui [around line 458] ...
//
//	function ObjectBuilderGui::buildfxSpaceDust(%this)
//	{
//		%this.className = "fxSpaceDust";
//		%this.process();
//	}
//
//------------------------------------------------------------------------------
//
//	Put this in /example/common/client/missionDownload.cs in [function clientCmdMissionStartPhase3(%seq,%missionName)] (line 65)
//	after codeline 'onPhase2Complete();'.
//
//	StartClientReplication();
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simBase.h (around line 509) in
//
//	namespace Sim
//  {
//	   DeclareNamedSet(fxReplicatorSet)  <-- ADD THIS (Note no semi-colon).
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simBase.cc (around line 19) in
//
//  ImplementNamedSet(fxReplicatorSet)  <-- ADD THIS
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simManager.cc [function void init()] (around line 269).
//
//	namespace Sim
//  {
//		InstantiateNamedSet(fxReplicatorSet);  <-- ADD THIS
//
//------------------------------------------------------------------------------

extern bool gEditingMission;

//------------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(fxSpaceDust);
IMPLEMENT_CO_NETOBJECT_V1(fxSpaceDustReplicatedMesh);

ConsoleDocClass( fxSpaceDust,
   "@brief An emitter for objects to replicate across an area.\n"
   "@ingroup Foliage\n"
);

ConsoleDocClass( fxSpaceDustReplicatedMesh,
   "@brief The object definition for shapes that will be replicated across an area using an fxSpaceDust.\n"
   "@ingroup Foliage\n"
);

//------------------------------------------------------------------------------
// Class: fxSpaceDust
//------------------------------------------------------------------------------

fxSpaceDust::fxSpaceDust()
{
   // Setup NetObject.
   mTypeMask |= StaticObjectType;
   mNetFlags.set(Ghostable | ScopeAlways);

   // Reset Shape Count.
   mCurrentShapeCount = 0;

   // Reset Creation Area Angle Animation.
   mCreationAreaAngle = 0;

   // Reset Last Render Time.
   mLastRenderTime = 0;
}

//------------------------------------------------------------------------------

fxSpaceDust::~fxSpaceDust()
{
}

//------------------------------------------------------------------------------

void fxSpaceDust::initPersistFields()
{
   // Add out own persistent fields.
   addGroup( "Debugging" );	// MM: Added Group Header.
      addField( "HideReplications",    TypeBool,      Offset( mFieldData.mHideReplications,     fxSpaceDust ), "Replicated shapes are hidden when set to true." );
      addField( "ShowPlacementArea",   TypeBool,      Offset( mFieldData.mShowPlacementArea,    fxSpaceDust ), "Draw placement rings when set to true." );
      addField( "PlacementColour",     TypeColorF,    Offset( mFieldData.mPlaceAreaColour,      fxSpaceDust ), "Color of the placement ring." );
   endGroup( "Debugging" );	// MM: Added Group Footer.

   addGroup( "Media" );	// MM: Added Group Header.
      addField( "ShapeFile",           TypeShapeFilename,  Offset( mFieldData.mShapeFile,            fxSpaceDust ), "Filename of shape to replicate." );
   endGroup( "Media" );	// MM: Added Group Footer.

   addGroup( "Replications" );	// MM: Added Group Header.
      addField( "Seed",                TypeS32,       Offset( mFieldData.mSeed,                 fxSpaceDust ), "Random seed for shape placement." );
      addField( "ShapeCount",          TypeS32,       Offset( mFieldData.mShapeCount,           fxSpaceDust ), "Maximum shape instance count." );
      addField( "ShapeRetries",        TypeS32,       Offset( mFieldData.mShapeRetries,         fxSpaceDust ), "Number of times to try placing a shape instance before giving up." );
   endGroup( "Replications" );	// MM: Added Group Footer.

   addGroup( "Placement Radius" );	// MM: Added Group Header.
      addField( "Radius",              TypeS32,       Offset( mFieldData.mPlacementRadius,      fxSpaceDust ), "Placement area inner radius on the X axis" );
      addField("ZOffset",              TypeS32,       Offset( mFieldData.mPlacementZOffset,     fxSpaceDust ), "Radius offset on the Z axis");
   endGroup( "Placement Radius" );	// MM: Added Group Footer.

   addGroup( "Object Transforms" );	// MM: Added Group Header.
      addField( "ShapeScaleMin",       TypePoint3F,   Offset( mFieldData.mShapeScaleMin,        fxSpaceDust ), "Minimum shape scale." );
      addField( "ShapeScaleMax",       TypePoint3F,   Offset( mFieldData.mShapeScaleMax,        fxSpaceDust ), "Maximum shape scale." );
      addField( "ShapeRotateMin",      TypePoint3F,   Offset( mFieldData.mShapeRotateMin,       fxSpaceDust ), "Minimum shape rotation angles.");
      addField( "ShapeRotateMax",      TypePoint3F,   Offset( mFieldData.mShapeRotateMax,       fxSpaceDust ), "Maximum shape rotation angles." );
   endGroup( "Object Transforms" );	// MM: Added Group Footer.

   // Initialise parents' persistent fields.
   Parent::initPersistFields();
}

//------------------------------------------------------------------------------

void fxSpaceDust::CreateShapes(void)
{
   F32				HypX, HypY, HypZ;
   F32				Angle;
   U32				RelocationRetry;
   Point3F			ShapePosition;
   Point3F			ShapeStart;
   Point3F			ShapeEnd;
   Point3F			ShapeScale;
   EulerF			ShapeRotation;
   QuatF			QRotation;
   TSShape*		pShape;


   // Don't create shapes if we are hiding replications.
   if (mFieldData.mHideReplications) return;

   // Cannot continue without shapes!
   if (dStrcmp(mFieldData.mShapeFile, "") == 0 || !mFieldData.mShapeFile) return;

   // Check Shapes.
   AssertFatal(mCurrentShapeCount==0,"Shapes already present, this should not be possible!");

   // Set Seed.
   RandomGen.setSeed(mFieldData.mSeed);

   // Set shape vector.
   mReplicatedShapes.clear();

   // Add shapes.
   for (U32 idx = 0; idx < mFieldData.mShapeCount; idx++)
   {
      fxSpaceDustReplicatedMesh*	fxStatic;

      // Create our static shape.
      fxStatic = new fxSpaceDustReplicatedMesh();

      // Set the 'shapeName' field.
      fxStatic->setField("shapeName", mFieldData.mShapeFile);

      // Is this Replicator on the Server?
      if (isServerObject())
         // Yes, so stop it from Ghosting. (Hack, Hack, Hack!)
         fxStatic->touchNetFlags(Ghostable, false);
      else
         // No, so flag as ghost object. (Another damn Hack!)
         fxStatic->touchNetFlags(IsGhost, true);

      // Register the Object.
      if (!fxStatic->registerObject())
      {
         // Problem ...
         Con::warnf(ConsoleLogEntry::General, "[%s] - Could not load shape file '%s'!", getName(), mFieldData.mShapeFile);

         // Destroy Shape.
         delete fxStatic;

         // Destroy existing hapes.
         DestroyShapes();

         // Quit.
         return;
      }

      // Get Allocated Shape.
      pShape = fxStatic->getShape();

      // Reset Relocation Retry.
      RelocationRetry = mFieldData.mShapeCount; // mFieldData.mShapeRetries;

      // Find it a home ...
      // Get the Replicator Position.
      ShapePosition = getPosition();

      // Calculate a random offset
      HypX	= RandomGen.randF(-mFieldData.mPlacementRadius, mFieldData.mPlacementRadius);
      HypY	= RandomGen.randF(-mFieldData.mPlacementRadius, mFieldData.mPlacementRadius);
      HypZ  = RandomGen.randF(-(mFieldData.mPlacementRadius + mFieldData.mPlacementZOffset), mFieldData.mPlacementRadius + mFieldData.mPlacementZOffset);
      Angle	= RandomGen.randF(0, (F32)M_2PI);

      // Calcualte the new position.
      ShapePosition.x += HypX * mCos(Angle);
      ShapePosition.y += HypY * mSin(Angle);
      ShapePosition.z += HypZ;

      // Get Shape Transform.
      MatrixF XForm = fxStatic->getTransform();

      // No, so choose a new Rotation (in Radians).
      ShapeRotation.set(	mDegToRad(RandomGen.randF(mFieldData.mShapeRotateMin.x, mFieldData.mShapeRotateMax.x)),
         mDegToRad(RandomGen.randF(mFieldData.mShapeRotateMin.y, mFieldData.mShapeRotateMax.y)),
         mDegToRad(RandomGen.randF(mFieldData.mShapeRotateMin.z, mFieldData.mShapeRotateMax.z)));

      // Set Quaternion Roation.
      QRotation.set(ShapeRotation);

      // Set Transform Rotation.
      QRotation.setMatrix(&XForm);

      // Set Position.
      XForm.setColumn(3, ShapePosition);

      // Set Shape Position / Rotation.
      fxStatic->setTransform(XForm);

      // Choose a new Scale.
      ShapeScale.set(	RandomGen.randF(mFieldData.mShapeScaleMin.x, mFieldData.mShapeScaleMax.x),
         RandomGen.randF(mFieldData.mShapeScaleMin.y, mFieldData.mShapeScaleMax.y),
         RandomGen.randF(mFieldData.mShapeScaleMin.z, mFieldData.mShapeScaleMax.z));

      // Set Shape Scale.
      fxStatic->setScale(ShapeScale);

      // Lock it.
      fxStatic->setLocked(true);

      // Store Shape in Replicated Shapes Vector.
      //mReplicatedShapes[mCurrentShapeCount++] = fxStatic;
      mReplicatedShapes.push_back(fxStatic);

   }

   mCurrentShapeCount = mReplicatedShapes.size();

   // Take first Timestamp.
   mLastRenderTime = Platform::getVirtualMilliseconds();
}

//------------------------------------------------------------------------------

void fxSpaceDust::DestroyShapes(void)
{
   // Finish if we didn't create any shapes.
   if (mCurrentShapeCount == 0) return;

   // Remove shapes.
   for (U32 idx = 0; idx < mCurrentShapeCount; idx++)
   {
      fxSpaceDustReplicatedMesh* fxStatic;

      // Fetch the Shape Object.
      fxStatic = mReplicatedShapes[idx];

      // Got a Shape?
      if (fxStatic)
      {
         // Unlock it.
         fxStatic->setLocked(false);

         // Unregister the object.
         fxStatic->unregisterObject();

         // Delete it.
         delete fxStatic;
      }
   }

   // Empty the Replicated Shapes Vector.
   mReplicatedShapes.clear();

   // Reset Shape Count.
   mCurrentShapeCount = 0;
}

//------------------------------------------------------------------------------

void fxSpaceDust::RenewShapes(void)
{
   // Destroy any shapes.
   DestroyShapes();

   // Don't create shapes on the Server.
   if (isServerObject()) return;



   // Create Shapes.
   CreateShapes();
}

//------------------------------------------------------------------------------

void fxSpaceDust::StartUp(void)
{
   RenewShapes();
}

//------------------------------------------------------------------------------

bool fxSpaceDust::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   // Add the Replicator to the Replicator Set.
   dynamic_cast<SimSet*>(Sim::findObject("fxSpaceDustReplicatorSet"))->addObject(this);

   // Set Default Object Box.
   mObjBox.minExtents.set( -0.5, -0.5, -0.5 );
   mObjBox.maxExtents.set(  0.5,  0.5,  0.5 );
   resetWorldBox();

   // Add to Scene.
   setRenderTransform(mObjToWorld);
   addToScene();

   // Register for notification when GhostAlways objects are done loading
   NetConnection::smGhostAlwaysDone.notify( this, &fxSpaceDust::onGhostAlwaysDone );

   return true;
}

//------------------------------------------------------------------------------

void fxSpaceDust::onRemove()
{
   // Remove the Replicator from the Replicator Set.
   dynamic_cast<SimSet*>(Sim::findObject("fxReplicatorSet"))->removeObject(this);

   NetConnection::smGhostAlwaysDone.remove( this, &fxSpaceDust::onGhostAlwaysDone );

   removeFromScene();

   // Destroy Shapes.
   DestroyShapes();

   // Do Parent.
   Parent::onRemove();
}

//------------------------------------------------------------------------------

void fxSpaceDust::onGhostAlwaysDone()
{
   RenewShapes();
}

//------------------------------------------------------------------------------

void fxSpaceDust::inspectPostApply()
{
   // Set Parent.
   Parent::inspectPostApply();

   // Renew Shapes.
   RenewShapes();

   // Set Replication Mask.
   setMaskBits(ReplicationMask);
}

//------------------------------------------------------------------------------

DefineEngineFunction(StartDustReplication, void, (),, "Activates the fxSpaceDust replicator.\n"
													"@tsexample\n"
														"// Call the function\n"
														"StartDustReplication()\n"
													"@endtsexample\n"
													"@ingroup Foliage"
					)
{
   // Find the Replicator Set.
   SimSet *fxReplicatorSet = dynamic_cast<SimSet*>(Sim::findObject("fxSpaceDustReplicatorSet"));

   // Return if Error.
   if (!fxReplicatorSet) return;

   // StartUp Replication Object.
   for (SimSetIterator itr(fxReplicatorSet); *itr; ++itr)
   {
      // Fetch the Replicator Object.
      fxSpaceDust* Replicator = static_cast<fxSpaceDust*>(*itr);
      // Start Client Objects Only.
      if (Replicator->isClientObject()) Replicator->StartUp();
   }
   // Info ...
   Con::printf("Client Dust Replication Startup has Happened!");
}


//------------------------------------------------------------------------------

void fxSpaceDust::prepRenderImage( SceneRenderState* state )
{
   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &fxSpaceDust::renderObject);
   // The fxSpaceDust isn't technically foliage but our debug
   // effect seems to render best as a Foliage type (translucent,
   // renders itself, no sorting)
   ri->type = RenderPassManager::RIT_Foliage;
   state->getRenderPass()->addInst( ri );
}

//------------------------------------------------------------------------------

// Renders a triangle stripped oval
void fxSpaceDust::renderArc(const F32 fRadiusX, const F32 fRadiusY)
{
}

// This currently uses the primbuilder, could convert out, but why allocate the buffer if we
// never edit the misison?
void fxSpaceDust::renderPlacementArea(const F32 ElapsedTime)
{
   if (gEditingMission && mFieldData.mShowPlacementArea)
   {
      GFXTransformSaver saver;

      GFXStateBlockDesc transparent;
      transparent.setCullMode(GFXCullNone);
      transparent.alphaTestEnable = true;
      transparent.setZReadWrite(true);
      transparent.zWriteEnable = false;
      transparent.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
      transparent.setFillModeWireframe();

      GFXDrawUtil *drawer = GFX->getDrawUtil();
      AssertFatal(drawer, "Got NULL GFXDrawUtil!");
      drawer->drawSphere(transparent, mFieldData.mPlacementRadius, getPosition(), mFieldData.mPlaceAreaColour.toColorI());

      mCreationAreaAngle = (U32)(mCreationAreaAngle + (1000 * ElapsedTime));
      mCreationAreaAngle = mCreationAreaAngle % 360;
   }
}

//------------------------------------------------------------------------------

void fxSpaceDust::renderObject(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance* overrideMat)
{
   if (overrideMat)
      return;

   // Return if placement area not needed.
   if (!mFieldData.mShowPlacementArea)
      return;

   // Calculate Elapsed Time and take new Timestamp.
   S32 Time = Platform::getVirtualMilliseconds();
   F32 ElapsedTime = (Time - mLastRenderTime) * 0.001f;
   mLastRenderTime = Time;	

   renderPlacementArea(ElapsedTime);
}

//------------------------------------------------------------------------------

U32 fxSpaceDust::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   // Pack Parent.
   U32 retMask = Parent::packUpdate(con, mask, stream);

   // Write Replication Flag.
   if (stream->writeFlag(mask & ReplicationMask))
   {
      stream->writeAffineTransform(mObjToWorld);						// Replicator Position.

      stream->writeInt(mFieldData.mSeed, 32);							// Replicator Seed.
      stream->writeInt(mFieldData.mShapeCount, 32);					// Shapes Count.
      stream->writeInt(mFieldData.mShapeRetries, 32);					// Shapes Retries.
      stream->writeString(mFieldData.mShapeFile);
      stream->writeSignedInt(mFieldData.mPlacementRadius, 32);		// Shapes Placement Radius
      stream->writeSignedInt(mFieldData.mPlacementZOffset, 32);   // Shapes Placement Offset on Z Axis
      mathWrite(*stream, mFieldData.mShapeScaleMin);					// Shapes Scale Min.
      mathWrite(*stream, mFieldData.mShapeScaleMax);					// Shapes Scale Max.
      mathWrite(*stream, mFieldData.mShapeRotateMin);					// Shapes Rotate Min.
      mathWrite(*stream, mFieldData.mShapeRotateMax);					// Shapes Rotate Max.
      stream->writeFlag(mFieldData.mHideReplications);				// Hide Replications.
      stream->writeFlag(mFieldData.mShowPlacementArea);				// Show Placement Area Flag.
      stream->write(mFieldData.mPlaceAreaColour);
   }

   // Were done ...
   return(retMask);
}

//------------------------------------------------------------------------------

void fxSpaceDust::unpackUpdate(NetConnection * con, BitStream * stream)
{
   // Unpack Parent.
   Parent::unpackUpdate(con, stream);

   // Read Replication Details.
   if(stream->readFlag())
   {
      MatrixF		ReplicatorObjectMatrix;

      stream->readAffineTransform(&ReplicatorObjectMatrix);				// Replication Position.

      mFieldData.mSeed					   = stream->readInt(32);			// Replicator Seed.
      mFieldData.mShapeCount				= stream->readInt(32);			// Shapes Count.
      mFieldData.mShapeRetries			= stream->readInt(32);			// Shapes Retries.
      mFieldData.mShapeFile				= stream->readSTString();		// Shape File.
      mFieldData.mPlacementRadius		= stream->readSignedInt(32);	// Shapes Inner Radius X.
      mFieldData.mPlacementZOffset     = stream->readSignedInt(32);
      mathRead(*stream, &mFieldData.mShapeScaleMin);						// Shapes Scale Min.
      mathRead(*stream, &mFieldData.mShapeScaleMax);						// Shapes Scale Max.
      mathRead(*stream, &mFieldData.mShapeRotateMin);						// Shapes Rotate Min.
      mathRead(*stream, &mFieldData.mShapeRotateMax);						// Shapes Rotate Max.
      mFieldData.mHideReplications		= stream->readFlag();			// Hide Replications.
      mFieldData.mShowPlacementArea	   = stream->readFlag();				// Show Placement Area Flag..
      stream->read(&mFieldData.mPlaceAreaColour);

      // Set Transform.
      setTransform(ReplicatorObjectMatrix);

      RenewShapes();
   }
}

