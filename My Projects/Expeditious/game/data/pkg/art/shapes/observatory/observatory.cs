
singleton TSShapeConstructor(ObservatoryDae)
{
   baseShape = "./observatory.dae";
   loadLights = "0";
};

function ObservatoryDae::onLoad(%this)
{
   %this.setSequenceCyclic("ambient", "0");
}
