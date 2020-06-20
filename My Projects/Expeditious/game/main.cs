$Core::windowIcon = "data/icon.png";

// Console does something.
setLogMode(6);

// Disable script trace.
trace(false);

// Set the name of our application
$appName = "Expeditious";
   
//-----------------------------------------------------------------------------
// Load up scripts to initialise subsystems.
ModuleDatabase.setModuleExtension("module");
ModuleDatabase.scanModules( "core", false );
ModuleDatabase.LoadExplicit( "CoreModule" );

//-----------------------------------------------------------------------------
// Load any gameplay modules
ModuleDatabase.scanModules( "data", false );
ModuleDatabase.LoadGroup( "Game" );

if( !$isDedicated )
{
   // Start rendering and stuff.
   initRenderManager();
   initLightingSystems("Advanced Lighting"); 

   //load prefs
   %prefPath = getPrefpath();
   if ( isFile( %prefPath @ "/clientPrefs.cs" ) )
      exec( %prefPath @ "/clientPrefs.cs" );
   else
      exec("data/defaults.cs");
   
   configureCanvas();
   
   //Autodetect settings if it's our first time
   if($pref::Video::autoDetect)
      GraphicsMenu.Autodetect();

   postFXInit();
   
   // As we know at this point that the initial load is complete,
   // we can hide any splash screen we have, and show the canvas.
   // This keeps things looking nice, instead of having a blank window
   Canvas.showWindow();
}

echo("Engine initialized...");
