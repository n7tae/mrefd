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
	private $XMLContent              = null;
	private $XMLFile                 = null;
	private $ServiceName             = null;
	private $Version                 = null;
	public $Interlinks               = null;

	public function __construct() {
		$this->Nodes             = array();
		$this->Stations          = array();
		$this->Peers             = array();
		$this->Interlinks        = array();
	}

	public function LoadXML() {
		if ($this->XMLFile != null) {
			$handle = fopen($this->XMLFile, 'r');
			$this->XMLContent = fread($handle, filesize($this->XMLFile));
			fclose($handle);

			$this->ServiceName = substr($this->XMLContent, strpos($this->XMLContent, "M17")+3, 4);

			$this->ReflectorName = "M17".$this->ServiceName;

			$XML       = new ParseXML();



			$AllNodesString    = $XML->GetElement($this->XMLContent, "NODES");
			$tmpNodes          = $XML->GetAllElements($AllNodesString, "NODE");

			for ($i=0;$i<count($tmpNodes);$i++) {
				$Node = new Node(
				$XML->GetElement($tmpNodes[$i], 'CALLSIGN'),
				$XML->GetElement($tmpNodes[$i], 'IP'),
				$XML->GetElement($tmpNodes[$i], 'LINKEDMODULE'),
				$XML->GetElement($tmpNodes[$i], 'LISTENONLY'),
				$XML->GetElement($tmpNodes[$i], 'CONNECTTIME'),
				$XML->GetElement($tmpNodes[$i], 'LASTHEARDTIME'),
				CreateCode(16));
				$this->AddNode($Node);
			}

			$AllStationsString = $XML->GetElement($this->XMLContent, "STATIONS");
			$tmpStations       = $XML->GetAllElements($AllStationsString, "STATION");

			for ($i=0;$i<count($tmpStations);$i++) {
				$Station = new Station(
				$XML->GetElement($tmpStations[$i], 'SOURCE'),
				$XML->GetElement($tmpStations[$i], 'DESTINATION'),
				$XML->GetElement($tmpStations[$i], 'MODE'),
				$XML->GetElement($tmpStations[$i], 'VIA'),
				$XML->GetElement($tmpStations[$i], 'ONMODULE'),
				$XML->GetElement($tmpStations[$i], 'LASTHEARDTIME'));
				$this->AddStation($Station, false);
			}
			$AllPeersString = $XML->GetElement($this->XMLContent, "PEERS");
			$tmpPeers = $XML->GetAllElements($AllPeersString, "PEER");

			for ($i=0;$i<count($tmpPeers);$i++) {
				$Peer = new Peer(
				$XML->GetElement($tmpPeers[$i], 'CALLSIGN'),
				$XML->GetElement($tmpPeers[$i], 'IP'),
				$XML->GetElement($tmpPeers[$i], 'LINKEDMODULE'),
				$XML->GetElement($tmpPeers[$i], 'DASHBOARDURL'),
				$XML->GetElement($tmpPeers[$i], 'CONNECTTIME'),
				$XML->GetElement($tmpPeers[$i], 'LASTHEARDTIME'));
				$this->AddPeer($Peer, false);
			}

			$this->Version = $XML->GetElement($this->XMLContent, "VERSION");
		}
	}

	public function GetVersion() {
		return $this->Version;
	}

	public function GetReflectorName() {
		return $this->ReflectorName;
	}

	public function SetXMLFile($XMLFile) {
		if (file_exists($XMLFile) && (is_readable($XMLFile))) {
			$this->XMLFile = $XMLFile;
		}
		else {
			die("File ".$XMLFile." does not exist or is not readable");
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
		if (isset($this->Peer[$ArrayIndex])) {
			return $this->Peer[$ArrayIndex];
		}
		return false;
	}

	public function AddStation($StationObject, $AllowDouble = false) {
		if (is_object($StationObject)) {
			if ($AllowDouble) {
				$this->Stations[] = $StationObject;
			} else {
				$FoundStationInList = false;
				$i                  = 0;

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

	public function StationCount() {
		return count($this->Stations);
	}

	public function GetStation($ArrayIndex) {
		if (isset($this->Stations[$ArrayIndex])) {
			return $this->Stations[$ArrayIndex];
		}
		return false;
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


	public function InterlinkCount() {
		return count($this->Interlinks);
	}

	public function GetInterlink($Index) {
		if (isset($this->Interlinks[$Index])) return $this->Interlinks[$Index];
		return array();
	}
}

?>
