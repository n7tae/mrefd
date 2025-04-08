<?php

class Node {

	private $Callsign;
	private $IP;
	private $LinkedModule;
	private $ListenOnly;
	private $ConnectTime;
	private $LastHeardTime;
	private $RandomID;

	public function __construct($Callsign, $IP, $LinkedModule, $ListenOnly, $ConnectTime, $LastHeardTime, $RandomID) {

		$this->IP            = $IP;

		$this->ListenOnly    = ($ListenOnly === 'true') ? 'Yes' : 'No';
		$this->ConnectTime   = ParseTime($ConnectTime);
		$this->LastHeardTime = ParseTime($LastHeardTime);
		$this->Callsign      = trim($Callsign);
		$this->LinkedModule  = trim($LinkedModule);
		$this->RandomID      = $RandomID;
	}

	public function GetCallsign()             { return $this->Callsign;       }
	public function GetIP()                   { return $this->IP;             }
	public function GetLinkedModule()         { return $this->LinkedModule;   }
	public function GetListenOnly()           { return $this->ListenOnly;     }
	public function GetConnectTime()          { return $this->ConnectTime;    }
	public function GetLastHeardTime()        { return $this->LastHeardTime;  }
	public function GetRandomID()             { return $this->RandomID;       }
   
}

?>
