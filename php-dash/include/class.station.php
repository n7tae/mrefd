<?php

class Station {
   
   private $Source;
   private $Destination;
   private $Via;
   private $LastHeardTime;
   private $Mode;
   private $CallsignOnly;
   private $OnModule;
      
   public function __construct($Source, $Destination, $Mode, $Via, $OnModule, $LastHeardTime) {
      $this->Source        = trim($Source);
	  $this->Destination   = trim($Destination);
      $this->Via           = trim($Via);
      $this->LastHeardTime = ParseTime($LastHeardTime);
      $this->Mode          = $Mode;
      $this->CallsignOnly  = strtok($Source, " -/.");
      $this->OnModule      = $OnModule;
   }
 
   public function GetSource()               { return $this->Source;         }
   public function GetDestination()          { return $this->Destination;    }
   public function GetVIA()                  { return $this->Via;            }
   public function GetLastHeardTime()        { return $this->LastHeardTime;  }
   public function GetMode()                 { return $this->Mode;           }
   public function GetCallsignOnly()         { return $this->CallsignOnly;   }
   public function GetModule()               { return $this->OnModule;       }
   
}

?>
