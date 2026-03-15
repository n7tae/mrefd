<?php
class Peer {
   
   private $Callsign;
   private $IP;
   private $LinkedModule;
   private $Capabilities;
   private $Url;
   private $ConnectTime;
   private $LastHeardTime;
   
   public function __construct($Callsign, $IP, $LinkedModule, $Capabilities, $Url, $ConnectTime, $LastHeardTime) {
      
      $this->IP            = $IP;
      $this->ConnectTime   = $ConnectTime;
      $this->LastHeardTime = $LastHeardTime;
      $this->Callsign      = trim($Callsign);      
      $this->LinkedModule  = trim($LinkedModule);
	  $this->Capabilities  = $Capabilities;
	  $this->Url           = $Url;
   }
   
   public function GetCallsign()             { return $this->Callsign;       }
   public function GetIP()                   { return $this->IP;             }
   public function GetCapabilities()         { return $this->Capabilities;   }
   public function GetUrl()                  { return $this->Url;            }
   public function GetLinkedModule()         { return $this->LinkedModule;   }
   public function GetConnectTime()          { return $this->ConnectTime;    }
   public function GetLastHeardTime()        { return $this->LastHeardTime;  }
}
?>
