<?php
class Peer {
   
   private $Callsign;
   private $IP;
   private $LinkedModule;
   private $ConnectTime;
   private $LastHeardTime;
   
   public function __construct($Callsign, $IP, $LinkedModule, $ConnectTime, $LastHeardTime) {
      
      $this->IP            = $IP;
      $this->ConnectTime   = ParseTime($ConnectTime);
      $this->LastHeardTime = ParseTime($LastHeardTime);
      $this->Callsign      = trim($Callsign);      
      $this->LinkedModule  = trim($LinkedModule);
   }
   
   public function GetCallsign()             { return $this->Callsign;       }
   public function GetIP()                   { return $this->IP;             }
   public function GetLinkedModule()         { return $this->LinkedModule;   }
   public function GetConnectTime()          { return $this->ConnectTime;    }
   public function GetLastHeardTime()        { return $this->LastHeardTime;  }
}
?>
