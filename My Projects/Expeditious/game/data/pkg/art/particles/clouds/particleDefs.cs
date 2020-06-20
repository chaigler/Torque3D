datablock ParticleData(CloudParticle0 : DefaultParticle)
{
   textureName = "data/pkg/art/particles/clouds/cloud0.png";
   animTexName = "data/pkg/art/particles/clouds/cloud0.png";
   colors[0] = "1 0.991 0.991 0";
   colors[1] = "1 1 0.991102 0.638";
   colors[2] = "1 1 0.991 1";
   sizes[0] = "50";
   sizes[1] = "50";
   sizes[2] = "50";
   sizes[3] = "50";
   spinSpeed = "0.021";
   spinRandomMin = "-10";
   colors[3] = "1 1 0.991102 0";
   lifetimeMS = "10000";
   lifetimeVarianceMS = "1";
   spinRandomMax = "20";
};


datablock ParticleEmitterData(CloudEmitter : DefaultEmitter)
{
   ejectionOffset = "200";
   particles = "CloudParticle0";
   blendStyle = "NORMAL";
   ejectionPeriodMS = "20";
   thetaMax = "180";
   ejectionVelocity = "3";
   velocityVariance = "2";
   ejectionOffsetVariance = "25";
   softParticles = "1";
};
