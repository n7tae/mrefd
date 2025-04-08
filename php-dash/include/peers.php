<?php
?>

<table class="table table-sm table-striped table-hover">
	<tr class="table-center">
		<th>#</th>
		<th>M17 Peer</th>
		<th>Last Heard</th>
		<th>Linked</th>
		<th>Module</th><?php

if ($PageOptions['PeerPage']['IPModus'] != 'HideIP') {
	echo '<th>IP</th>';
}

?>
 </tr>
<?php

$Reflector->LoadFlags();

for ($i=0;$i<$Reflector->PeerCount();$i++) {
	echo '
	<tr class="table-center">
	<td>' . ($i+1).'</td>';
	echo '<td>' . $Reflector->Peers[$i]->GetCallSign() . '</td>';
	echo '
	<td>' . date("d-m-Y H:i", $Reflector->Peers[$i]->GetLastHeardTime()) . '<br />'
	. elapsedTime($Reflector->Peers[$i]->GetLastHeardTime()) . '</td>
	<td>' . date("Y-m-d H:i", $Reflector->Peers[$i]->GetConnectTime()) . '<br />'
	. elapsedTime($Reflector->Peers[$i]->GetConnectTime()) . '</td>
	<td>' . $Reflector->Peers[$i]->GetLinkedModule() . '</td>';
	if ($PageOptions['PeerPage']['IPModus'] != 'HideIP') {
		echo '<td>';
		$Bytes = explode(".", $Reflector->Peers[$i]->GetIP());
		$MC = $PageOptions['PeerPage']['MasqueradeCharacter'];
		if ($Bytes !== false && count($Bytes) == 4) {
			switch ($PageOptions['PeerPage']['IPModus']) {
				case 'ShowLast1ByteOfIP':
					echo $MC.'.'.$MC.'.'.$MC.'.'.$Bytes[3];
					break;
				case 'ShowLast2ByteOfIP':
					echo $MC.'.'.$MC.'.'.$Bytes[2].'.'.$Bytes[3];
					break;
				case 'ShowLast3ByteOfIP':
					echo $MC.'.'.$Bytes[1].'.'.$Bytes[2].'.'.$Bytes[3];
					break;
				default:
					echo '<a href="http://'.$Reflector->Peers[$i]->GetIP().'" target="_blank" style="text-decoration:none;color:#000000;">'.$Reflector->Peers[$i]->GetIP().'</a>';
			}
		} else {
			$ipstr = $Reflector->Peers[$i]->GetIP();
			$count = substr_count($ipstr, ":");
			if ($count > 1) {
				if (1 == substr_count($ipstr, "::")) { $ipstr = str_replace("::", str_repeat(":", 9 - $count), $ipstr); }
				if (7 == substr_count($ipstr, ":")) {
					echo $MC.':'.$MC.':'.$MC.':'.$MC.':'.$MC.':'.$MC;
					$Bytes = explode(":", $ipstr);
					for( $k=6; $k<8; $k++) { echo (0==strlen($Bytes[$k])) ? ':0' : ':'.$Bytes[$k]; }
				}
			}
		}
		echo '</td>';
	}
	echo '</tr>';
	if ($i == $PageOptions['PeerPage']['LimitTo']) { $i = $Reflector->PeerCount()+1; }
}

?>

</table>
