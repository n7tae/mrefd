<?php
class Peer {
   
   private $Callsign;
   private $IP;
   private $LinkedModule;
   private $DashboardUrl;
   private $ConnectTime;
   private $LastHeardTime;
   
   public function __construct($Callsign, $IP, $LinkedModule, $DashboardUrl, $ConnectTime, $LastHeardTime) {
      
      $this->IP            = $IP;
      $this->ConnectTime   = ParseTime($ConnectTime);
      $this->LastHeardTime = ParseTime($LastHeardTime);
      $this->Callsign      = trim($Callsign);      
      $this->LinkedModule  = trim($LinkedModule);
	  $this->DashboardUrl  = trim($DashboardUrl);
   }
   
   public function GetCallsign()             { return $this->Callsign;       }
   public function GetIP()                   { return $this->IP;             }
   public function GetLinkedModule()         { return $this->LinkedModule;   }
   public function GetDashboardUrl()         { return $this->DashboardUrl;   }
   public function GetConnectTime()          { return $this->ConnectTime;    }
   public function GetLastHeardTime()        { return $this->LastHeardTime;  }
}
?>
