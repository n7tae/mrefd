<?php

class xReflector {
	private $Flagarray               = null;
	private $Flagarray_DXCC          = null;
	private $Flagfile                = null;
	private $ServiceUptime           = null;
	private $ProcessIDFile           = null;

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
