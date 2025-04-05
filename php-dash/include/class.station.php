<?php

class Station {
   
   private $Callsign;
   private $Via;
   private $LastHeardTime;
   private $Mode;
   private $CallsignOnly;
   private $Peer;
   private $OnModule;
      
   public function __construct($Callsign, $Mode, $Via, $OnModule, $Peer, $LastHeardTime) {
      $this->Callsign      = trim($Callsign);
      $this->Via           = trim($Via);
      $this->Peer          = trim($Peer);
      $this->LastHeardTime = ParseTime($LastHeardTime);
      $this->Mode          = $Mode;
      $tmp = explode(" ", $Callsign);
      $this->CallsignOnly  = $tmp[0];
      $this->OnModule      = $OnModule;
   }
 
   public function GetCallsign()             { return $this->Callsign;       }
   public function GetVIA()                  { return $this->Via;            }
   public function GetPeer()                 { return $this->Peer;           }
   public function GetLastHeardTime()        { return $this->LastHeardTime;  }
   public function GetMode()                 { return $this->Mode;           }
   public function GetCallsignOnly()         { return $this->CallsignOnly;   }
   public function GetModule()               { return $this->OnModule;       }
   
}

?>
