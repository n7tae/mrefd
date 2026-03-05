<?php

class Station {
   
   private $Source;
   private $Maidenhead;
   private $Latitude;
   private $Longitude;
   private $Destination;
   private $Via;
   private $LastHeardTime;
   private $Mode;
   private $CallsignOnly;
   private $OnModule;
      
   public function __construct($Source, $Maidenhead, $Latitude, $Longitude, $Destination, $Mode, $Via, $OnModule, $LastHeardTime) {
      $this->Source        = trim($Source);
	  $this->Maidenhead    = trim($Maidenhead);
	  $this->Latitude      = trim($Latitude);
	  $this->Longitude     = trim($Longitude);
	  $this->Destination   = trim($Destination);
      $this->Via           = trim($Via);
      $this->LastHeardTime = $LastHeardTime;
      $this->Mode          = $Mode;
      $this->CallsignOnly  = strtok($Source, " -/.");
      $this->OnModule      = $OnModule;
   }
 
   public function GetSource()               { return $this->Source;         }
   public function GetMaidenhead()           { return $this->Maidenhead;     }
   public function GetLatitude()             { return $this->Latitude;       }
   public function GetLongitude()            { return $this->Longitude;      }
   public function GetDestination()          { return $this->Destination;    }
   public function GetVIA()                  { return $this->Via;            }
   public function GetLastHeardTime()        { return $this->LastHeardTime;  }
   public function GetMode()                 { return $this->Mode;           }
   public function GetCallsignOnly()         { return $this->CallsignOnly;   }
   public function GetModule()               { return $this->OnModule;       }
   
}

?>
