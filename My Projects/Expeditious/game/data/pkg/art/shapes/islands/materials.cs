
singleton Material(island0_IslandGrassMat)
{
   mapTo = "IslandGrassMat";
   diffuseMap[0] = "data/pkg/art/shapes/islands/grass_0";
   specular[0] = "0.5 0.5 0.5 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
   useAnisotropic[0] = "1";
};

singleton Material(island0_IslandRockFaceMat)
{
   mapTo = "IslandRockFaceMat";
   diffuseColor[2] = "White";
   diffuseMap[0] = "data/pkg/art/shapes/islands/rock_face_0.png";
   specular[0] = "0.5 0.5 0.5 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
   useAnisotropic[0] = "1";
};

singleton Material(island0_grass_IslandGrassMat0)
{
   mapTo = "IslandGrassMat0";
   diffuseMap[0] = "data/pkg/art/shapes/islands/grass_l2";
   specular[0] = "0.5 0.5 0.5 1";
   specularPower[0] = "50";
   translucentBlendOp = "Sub";
   diffuseColor[0] = "0.187821 0.991102 0.162029 1";
   useAnisotropic[0] = "1";
   castShadows = "1";
   translucent = "0";
   alphaRef = "15";
   scrollDir[0] = "0 -0.1";
   scrollSpeed[0] = "0.588";
   rotSpeed[0] = "0.353";
   rotPivotOffset[0] = "0 -0.14";
   waveType[0] = "Triangle";
   waveFreq[0] = "0.625";
   waveAmp[0] = "1";
   castDynamicShadows = "1";
   alphaTest = "1";
   showFootprints = "0";
   doubleSided = "1";
   subSurface[0] = "1";
};

singleton Material(island_rock_base_IslandRockMat)
{
   mapTo = "IslandRockMat";
   diffuseColor[3] = "White";
   diffuseMap[0] = "rock_face_0";
   specular[0] = "0.5 0.5 0.5 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
};
