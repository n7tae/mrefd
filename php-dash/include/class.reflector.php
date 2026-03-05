<?php

class xReflector {
	public $Nodes                    = null;
	public $Stations                 = null;
	public $Peers                    = null;
	private $Flagarray               = null;
	private $Flagarray_DXCC          = null;
	private $Flagfile                = null;
	private $ReflectorName           = null;
	private $ServiceUptime           = null;
	private $ProcessIDFile           = null;
	private $JsonFile                = null;
	private $Version                 = null;

	public function __construct() {
		$this->Nodes             = array();
		$this->Stations          = array();
		$this->Peers             = array();
	}

	public function LoadJson() {
		if ($this->JsonFile != null) {
			$JsonString = file_get_contents($this->JsonFile);
			if ($JsonString === false) {
				die('Could not read '.$this->JsonFile);
			}
			$Json = json_decode($JsonString);
			if ($Json === null) {
				die('Could not parse '.$this->JsonFile);
			}

			foreach ($Json->Clients as $client) {
				$this->AddNode(new Node($client->Callsign, $client->IP, $client->Module, $client->ListenOnly, $client->ConnectTime, $client->LastHeardTime, CreateCode(16)));
			}

			foreach ($Json->Users as $user) {
				$Station = new Station($user->Source, $user->Maidenhead, $user->Latitude, $user->Longitude, $user->Destination, $user->Mode, $user->Via, $user->Module, $user->LastHeardTime);
				$this->AddStation($Station, false);
			}

			foreach ($Json->Peers as $peer) {
				$this->AddPeer(new Peer($peer->Callsign, $peer->IP, $peer->Modules, $peer->ConnectTime, $peer->LastHeardTime));
			}

			$this->Version       = $Json->Version;
			$this->ReflectorName = $Json->Callsign;
		}
	}

	public function GetVersion() {
		return $this->Version;
	}

	public function GetReflectorName() {
		return $this->ReflectorName;
	}

	public function SetJsonFile($JsonFile) {
		if (file_exists($JsonFile) && (is_readable($JsonFile))) {
			$this->JsonFile = $JsonFile;
		}
		else {
			die("File ".$JsonFile." does not exist or is not readable");
			$this->XMLContent = null;
		}
	}

	public function SetPIDFile($ProcessIDFile) {
		if (file_exists($ProcessIDFile)) {
			$this->ProcessIDFile = $ProcessIDFile;
			$this->ServiceUptime = time() - filectime($ProcessIDFile);
		}
		else {
			$this->ProcessIDFile = null;
			$this->ServiceUptime = null;
		}
	}

	public function GetServiceUptime() {
		return $this->ServiceUptime;
	}

	public function SetFlagFile($Flagfile) {
		if (file_exists($Flagfile) && (is_readable($Flagfile))) {
			$this->Flagfile = $Flagfile;
			return true;
		}
		return false;
	}

	public function LoadFlags() {
		if ($this->Flagfile != null) {
			$this->Flagarray = array();
			$this->Flagarray_DXCC = array();
			$handle = fopen($this->Flagfile,"r");
			if ($handle) {
			$i = 0;
			while(!feof($handle)) {
				$row = fgets($handle,1024);
				$tmp = explode(";", $row);

				if (isset($tmp[0])) { $this->Flagarray[$i]['Country'] = $tmp[0]; } else { $this->Flagarray[$i]['Country'] = 'Undefined'; }
				if (isset($tmp[1])) { $this->Flagarray[$i]['ISO']     = $tmp[1]; } else { $this->Flagarray[$i]['ISO'] = "Undefined"; }
				//$this->Flagarray[$i]['DXCC']    = array();
				if (isset($tmp[2])) {
					$tmp2 = explode("-", $tmp[2]);
					for ($j=0;$j<count($tmp2);$j++) {
						//$this->Flagarray[$i]['DXCC'][] = $tmp2[$j];
						$this->Flagarray_DXCC[ trim($tmp2[$j]) ] = $i;
					}
				}
				$i++;
			}
			fclose($handle);
			}
			return true;
		}
		return false;
	}

	public function AddNode($NodeObject) {
		if (is_object($NodeObject)) {
			$this->Nodes[] = $NodeObject;
		}
	}

	public function NodeCount() {
		return count($this->Nodes);
	}

	public function GetNode($ArrayIndex) {
		if (isset($this->Nodes[$ArrayIndex])) {
			return $this->Nodes[$ArrayIndex];
		}
		return false;
	}

	public function AddPeer($PeerObject) {
		if (is_object($PeerObject)) {
			$this->Peers[] = $PeerObject;
		}
	}

	public function PeerCount() {
		return count($this->Peers);
	}

	public function GetPeer($ArrayIndex) {
		if (isset($this->Peers[$ArrayIndex])) {
			return $this->Peers[$ArrayIndex];
		}
		return false;
	}

	public function AddStation($StationObject, $AllowDouble = false) {
		if (is_object($StationObject)) {

			if ($AllowDouble) {
				$this->Stations[] = $StationObject;
			}
			else {
				$FoundStationInList = false;
				$i = 0;

				while (!$FoundStationInList && $i<$this->StationCount()) {
					if ($this->Stations[$i]->GetSource() == $StationObject->GetSource()) {
						$FoundStationInList = true;
					}
					$i++;
				}

				if (!$FoundStationInList) {
					$this->Stations[] = $StationObject;
				}
			}
		}
	}

	public function StationCount() {
		return count($this->Stations);
	}

	public function GetStation($ArrayIndex) {
		if (isset($this->Stations[$ArrayIndex])) {
			return $this->Stations[$ArrayIndex];
		}
		return false;
	}

	public function GetCallsignByID($RandomId) {
		$suffix   = "";
		$callsign = "";
		$i        = 0;
		while ($i < $this->NodeCount()) {
			if ($this->Nodes[$i]->GetRandomID() == $RandomId) {
			return $this->Nodes[$i]->GetCallSign();
			}
			$i++;
		}
		return 'N/A';
	}

	public function GetFlag($Callsign) {
		$Image     = "";
		$Letters = 4;
		$Name = "";
		while ($Letters >= 2) {
			$Prefix = substr(trim($Callsign), 0, $Letters);
				if (isset($this->Flagarray_DXCC[$Prefix])) {
					$Image = $this->Flagarray[ $this->Flagarray_DXCC[$Prefix] ]['ISO'];
					$Name  = $this->Flagarray[ $this->Flagarray_DXCC[$Prefix] ]['Country'];
					break;
				}
			$Letters--;
		}
		return array(strtolower($Image), $Name);
	}

	public function GetModules() {
		$out = array();
		for ($i=0;$i<$this->NodeCount();$i++) {
			$Found = false;
			$b = 0;
			while ($b < count($out) && !$Found) {
				if ($out[$b] == $this->Nodes[$i]->GetLinkedModule()) {
				$Found = true;
				}
				$b++;
			}
			if (!$Found && (trim($this->Nodes[$i]->GetLinkedModule()) != "")) {
				$out[] = $this->Nodes[$i]->GetLinkedModule();
			}
		}
		return $out;
	}

	public function GetNodesInModulesById($Module) {
		$out = array();
		for ($i=0;$i<$this->NodeCount();$i++) {
			if ($this->Nodes[$i]->GetLinkedModule() == $Module) {
				$out[] = $this->Nodes[$i]->GetRandomID();
			}
		}
		return $out;
	}
}

?>
