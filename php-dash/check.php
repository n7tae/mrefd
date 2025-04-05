<?php

if (file_exists("./include/config.inc.php")) {
    require_once("./include/config.inc.php");
} else {
    die("config.inc.php does not exist. Be sure to copy /include/config.inc.php.dist to /include/config.inc.php and edit the file accordingly.");
}

exec("pgrep mrefd", $output, $return);
if ($return == 0) {
  echo "mrefd running.\n";
  file_get_contents("https://check.m17.link/ping/" . $CallHome['GUID']);
} else {
  echo "mrefd not running?\n";
  file_get_contents("https://check.m17.link/ping/" . $CallHome['GUID'] . "/fail");
}

?>