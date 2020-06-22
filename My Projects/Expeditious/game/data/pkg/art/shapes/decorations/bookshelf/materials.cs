singleton Material(BookcaseMat)
{
   mapTo = "BookcaseMat";
   diffuseColor[0] = "1 1 1 1";
   specular[0] = "1 1 1 1";
   specular[2] = "1 1 1 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
   diffuseMap[0] = "data/pkg/art/shapes/palette0.png";
};

singleton Material(BooksOnShelfMat)
{
   mapTo = "BooksOnShelfMat";
   diffuseColor[0] = "1 1 1 1";
   specular[0] = "1 1 1 1";
   specular[2] = "1 1 1 1";
   specularPower[0] = "50";
   translucentBlendOp = "None";
   diffuseMap[0] = "data/pkg/art/shapes/palette0.png";
   useAnisotropic[0] = "1";
   showFootprints = "0";
};
