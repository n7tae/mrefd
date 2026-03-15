<?php

class Node {

	private $Callsign;
	private $IP;
	private $LinkedModule;
	private $Protocol;
	private $ListenOnly;
	private $ConnectTime;
	private $LastHeardTime;
	private $RandomID;

	public function __construct($Callsign, $IP, $LinkedModule, $Protocol, $ListenOnly, $ConnectTime, $LastHeardTime, $RandomID) {

		$this->IP            = $IP;

		$this->ListenOnly    = ($ListenOnly === 'true') ? 'Yes' : 'No';
		$this->ConnectTime   = $ConnectTime;
		$this->LastHeardTime = $LastHeardTime;
		$this->Callsign      = trim($Callsign);
		$this->LinkedModule  = trim($LinkedModule);
		$this->Protocol      = $Protocol;
		$this->RandomID      = $RandomID;
	}

	public function GetCallsign()             { return $this->Callsign;       }
	public function GetIP()                   { return $this->IP;             }
	public function GetLinkedModule()         { return $this->LinkedModule;   }
	public function GetListenOnly()           { return $this->ListenOnly;     }
	public function GetProtocol()             { return $this->Protocol;       }
	public function GetConnectTime()          { return $this->ConnectTime;    }
	public function GetLastHeardTime()        { return $this->LastHeardTime;  }
	public function GetRandomID()             { return $this->RandomID;       }
   
}

?>
