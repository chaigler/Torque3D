function ETool_CopyProperties::onWake(%this)
{
   %this.setCopyObjects();
}

function ETool_CopyProperties::setCopyObjects(%this)
{
   // see if we have objects selected already
   %selSize = EWorldEditor.getSelectionSize();
   %this.copyToMultiple = false;
   
   if(%selSize > 0)
   {
      // fill in the 'Copy From' field with the first object selected
      %selObj = EWorldEditor.getSelectedObject(0);      
      if(strlen(%selObj.getName()) > 0)
         ETool_Txt_CopyFromID.setText(%selObj.getName());
      else
         ETool_Txt_CopyFromID.setText(%selObj.getId());
      
      // if two objects are selected we'll set the 'Copy To' field to the second object      
      if(%selSize == 2)
      {
         %selObj = EWorldEditor.getSelectedObject(1);
         // set the 'Copy To' field to either the selected object's name or SimId
         if(strlen(%selObj.getName()) > 0)
         {
            ETool_Txt_CopyToID.setText(%selObj.getName());
         }
         else
         {
            ETool_Txt_CopyToID.setText(%selObj.getId());
         }
      }
      else
      {
         // if more than 2 objects are selected, we simply let the user know
         // how many are selected so they'll be aware
         ETool_Txt_CopyToID.setText((EWorldEditor.getSelectionSize() - 1) SPC "Objects");
         
         // and save this state for use later
         %this.copyToMultiple = true;
      }
   }
   else
   {
      // if no objects are selected, set the 'Copy' fields to remind the user
      // to enter names or IDs of objects manually
      ETool_Txt_CopyFromID.setText("Enter object name or ID");
      ETool_Txt_CopyToID.setText("Enter object name or ID");
   }
}

function ETool_CopyProperties::copyProperties(%this)
{
   // grab our copy objects again
   %this.setCopyObjects();
   
   // dumb safeguard to make sure we actually have objects selected/entered
   %copyFromObj = nameToId(ETool_Txt_CopyFromID.getText());
   if(!isObject(%copyFromObj) || strlen(ETool_Txt_CopyToID.getText()) == 0)
   {
      error("ETool_CopyProperties: No objects selected!");
   }
   
   // copy!
   if(%this.copyToMultiple)
   {
      for(%i = 1; %i < EWorldEditor.getSelectionSize(); %i++)
      {
         %toObj = EWorldEditor.getSelectedObject(%i);
         %this.copyPropToObject(%copyFromObj, %toObj);
      }
   }
   else
   {
      %this.copyPropToObject(%copyFromObj, nameToId(ETool_Txt_CopyToID));
   }
}

function ETool_CopyProperties::copyPropToObject(%this, %copyFromObj, %toObj)
{   
   // copy position (weirdness with TScript ternary operator makes this longer than necessary...)
   %pos = %toObj.getPosition();
   if(ETool_Chk_PosXCpy.isStateOn())
      %pos.x = %copyFromObj.getPosition().x;
   if(ETool_Chk_PosYCpy.isStateOn())
      %pos.y = %copyFromObj.getPosition().y;
   if(ETool_Chk_PosZCpy.isStateOn())
      %pos.z = %copyFromObj.getPosition().z;
      
   %toObj.setPosition(%pos);
   
   // copy scale (weirdness with TScript ternary operator makes this longer than necessary...)
   %scale = %toObj.getScale();
   if(ETool_Chk_ScaleXCpy.isStateOn())
      %pos.x = %copyFromObj.getScale().x;
   if(ETool_Chk_ScaleYCpy.isStateOn())
      %pos.y = %copyFromObj.getScale().y;
   if(ETool_Chk_ScaleZCpy.isStateOn())
      %pos.z = %copyFromObj.getScale().z;
   %toObj.setScale(%scale);
   
   // copy rotation
   if(ETool_Chk_RotCpy.isStateOn())
      %toObj.rotation = %copyFromObj.rotation;
}