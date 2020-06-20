
singleton Material(TreeBarkMat)
{
   mapTo = "TreeBarkMat";
   diffuseColor[0] = "1 1 0.991102 1";
   specular[0] = "0.5 0.5 0.5 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
   diffuseMap[0] = "data/pkg/art/shapes/palette0.png";
};

singleton Material(tree_live_TreeLeavesMat)
{
   mapTo = "TreeLeavesMat";
   diffuseColor[0] = "1 1 0.991102 1";
   specular[0] = "0.5 0.5 0.5 1";
   specular[1] = "1 1 1 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
   subSurface[0] = "0";
   diffuseMap[0] = "data/pkg/art/shapes/palette0.png";
   useAnisotropic[0] = "0";
};
