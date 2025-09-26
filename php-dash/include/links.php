<table class="table table-sm table-striped table-hover">
	<tr class="table-center">
		<th>#</th>
		<th>Flag</th>
		<th>Station</th>
		<th>Last Heard</th>
		<th>Linked</th>
		<th>ListenOnly</th>
		<th>Module</th>
<?php
if ($PageOptions['LinksPage']['IPModus'] != 'HideIP') {
	echo "\t\t<th>IP</th>";
}
?>
		</tr>
<?php

$Reflector->LoadFlags();

for ($i=0;$i<$ClientCount;$i++) {
	echo '<tr class="table-center">
	<td>'.($i+1).'</td>
	<td>';
	list ($Flag, $Name) = $Reflector->GetFlag($Json->Clients[$i]->Callsign);
	if (file_exists("./images/flags/".$Flag.".svg")) {
		echo '<a href="#" class="tip"><img src="./images/flags/'.$Flag.'.svg" class="table-flag" alt="'.$Name.'"><span>'.$Name.'</span></a>';
	}
	echo '</td>
	<td>'.$Json->Clients[$i]->Callsign;
	echo '</td>
	<td>' . date("Y-m-d H:i", $Json->Clients[$i]->LastHeardTime) . '<br />'.elapsedTime($Json->Clients[$i]->LastHeardTime) . ' ago</td>
	<td>' . date("Y-m-d H:i", $Json->Clients[$i]->ConnectTime) . '<br />for ' . elapsedTime($Json->Clients[$i]->ConnectTime) . '</td>
	<td>'.($Json->Clients[$i]->GetListenOnly?'Yes':'No').'</td>
	<td>'.$Json->Clients[$i]->Module.'</td>';
	if ($PageOptions['LinksPage']['IPModus'] != 'HideIP') {
		echo '<td>';
		$Bytes = explode(".", $Json->Clients[$i]->IP);
		$MC = $PageOptions['LinksPage']['MasqueradeCharacter'];
		if ($Bytes !== false && count($Bytes) == 4) {
			switch ($PageOptions['LinksPage']['IPModus']) {
				case 'ShowLast1ByteOfIP':
					echo $MC.'.'.$MC.'.'.$MC.'.'.$Bytes[3];
					break;
				case 'ShowLast2ByteOfIP':
					echo $MC.'.'.$MC.'.'.$Bytes[2].'.'.$Bytes[3]; break;
				case 'ShowLast3ByteOfIP':
					echo $MC.'.'.$Bytes[1].'.'.$Bytes[2].'.'.$Bytes[3];
					break;
				default:
					echo $Json->Clients[$i]->IP;
			}
		} else {
			$ipstr = $Json->Clients[$i]->IP;
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
   if ($i == $PageOptions['LinksPage']['LimitTo']) { $i = $ClientCount+1; }
}

?>

</table>
