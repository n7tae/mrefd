<?php
?>

<table class="table table-sm table-striped table-hover">
	<tr class="table-center">
		<th>#</th>
		<th>M17 Peer</th>
		<th>Last Heard</th>
		<th>Linked</th>
		<th>Module</th>
<?php
if ($PageOptions['PeerPage']['IPModus'] != 'HideIP') {
	echo "\t\t<th>IP</th>";
}
?>
		</tr>
<?php

$Reflector->LoadFlags();

for ($i=0;$i<$PeerCount();$i++) {
	echo '
	<tr class="table-center">
	<td>' . ($i+1).'</td>';
	echo '<td>' . $Json->Peers[$i]->CallSign . '</td>';
	echo '
	<td>' . date("d-m-Y H:i", $Json->Peers[$i]->LastHeardTime) . '<br />'
	. elapsedTime($Json->Peers[$i]->LastHeardTime) . ' ago</td>
	<td>' . date("Y-m-d H:i", $Json->Peers[$i]->ConnectTime) . '<br />for '
	. elapsedTime($Json->Peers[$i]->ConnectTime) . '</td>
	<td>' . $Json->Peers[$i]->Modules . '</td>';
	if ($PageOptions['PeerPage']['IPModus'] != 'HideIP') {
		echo '<td>';
		$Bytes = explode(".", $Json->Peers[$i]->IP);
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
					echo '<a href="http://'.$Json->Peers[$i]->IP.'" target="_blank" style="text-decoration:none;color:#000000;">'.$Json->Peers[$i]->IP.'</a>';
			}
		} else {
			$ipstr = $Json->Peers[$i]->IP;
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
