<?php

/*
 *  This dashboard is developed by KC1AWV for the mrefd M17
 *  reflector system. It is derived from the XLX dashboard
 *  originally developed by the DVBrazil team.
 *
 *  version 1.1.2
*/

if (file_exists("./include/functions.php")) {
    require_once("./include/functions.php");
} else {
    die("functions.php does not exist.");
}
if (file_exists("./include/config.inc.php")) {
    require_once("./include/config.inc.php");
} else {
    die("config.inc.php does not exist. Be sure to copy /include/config.inc.php.dist to /include/config.inc.php and edit the file accordingly.");
}

if (!class_exists('ParseXML')) require_once("./include/class.parsexml.php");
if (!class_exists('Node')) require_once("./include/class.node.php");
if (!class_exists('xReflector')) require_once("./include/class.reflector.php");
if (!class_exists('Station')) require_once("./include/class.station.php");
if (!class_exists('Peer')) require_once("./include/class.peer.php");
if (!class_exists('Interlink')) require_once("./include/class.interlink.php");

$Reflector = new xReflector();
$Reflector->SetFlagFile("./include/country.csv");
$Reflector->SetPIDFile($Service['PIDFile']);
$Reflector->SetXMLFile($Service['XMLFile']);

$Reflector->LoadXML();

?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="<?php echo $PageOptions['MetaDescription']; ?>"/>
    <meta name="keywords" content="<?php echo $PageOptions['MetaKeywords']; ?>"/>
    <meta name="author" content="<?php echo $PageOptions['MetaAuthor']; ?>"/>
    <meta name="revisit" content="<?php echo $PageOptions['MetaRevisit']; ?>"/>
    <meta name="robots" content="<?php echo $PageOptions['MetaAuthor']; ?>"/>

    <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
    <title><?php echo $Reflector->GetReflectorName(); ?> Reflector Dashboard</title>
    <link rel="icon" type="image/png" href="/images/icons/favicon-16x16.png" sizes="16x16">
    <link rel="icon" type="image/png" href="/images/icons/favicon-32x32.png" sizes="32x32">
    <link rel="icon" type="image/png" href="/images/icons/favicon-96x96.png" sizes="96x96">

    <!-- Bootstrap core CSS -->
    <link href="css/bootstrap.min.css" rel="stylesheet">

    <!-- Custom styles for this template -->
    <link href="css/dashboard.css" rel="stylesheet">
    <link href="css/navbar-top-fixed.css" rel="stylesheet">

    <?php

    if ($PageOptions['PageRefreshActive']) {
        echo '
   <script src="./js/jquery-3.7.1.min.js"></script>
   <script>
      var PageRefresh;

      function ReloadPage() {
         $.get("./index.php'.(isset($_GET['show'])?'?show='.$_GET['show']:'').'", function(data) {
            var BodyStart = data.indexOf("<bo"+"dy");
            var BodyEnd = data.indexOf("</bo"+"dy>");
            if ((BodyStart >= 0) && (BodyEnd > BodyStart)) {
               BodyStart = data.indexOf(">", BodyStart)+1;
               $("body").html(data.substring(BodyStart, BodyEnd));
            }
         })
            .always(function() {
               PageRefresh = setTimeout(ReloadPage, '.$PageOptions['PageRefreshDelay'].');
            });
      }';

	if (!isset($_GET['show']) || (($_GET['show'] != 'liveircddb') && ($_GET['show'] != 'reflectors') && ($_GET['show'] != 'interlinks') && ($_GET['show'] != 'livequadnet'))) {
            echo '
      PageRefresh = setTimeout(ReloadPage, ' . $PageOptions['PageRefreshDelay'] . ');';
        }
        echo '

      function SuspendPageRefresh() {
        clearTimeout(PageRefresh);
      }
   </script>';
    }
    if (!isset($_GET['show'])) $_GET['show'] = "";
    ?>
</head>
<body>
<?php if (file_exists("./tracking.php")) {
    include_once("tracking.php");
} ?>
<nav class="navbar navbar-expand-lg navbar-dark fixed-top bg-dark">
    <nav class="navbar-circular-container">
    <img style="background-color:white;" src="images/icons/favicon-96x96.png" class="navbar-circular-image">
    </img></nav>&nbsp;&nbsp;
    <a class="navbar-brand" href="#"><?php echo $Reflector->GetReflectorName(); ?> Reflector</a>
    <button class="navbar-toggler" type="button" data-toggle="collapse" data-target="#navbarCollapse" aria-controls="navbarCollapse" aria-expanded="false" aria-label="Toggle navigation">
      <span class="navbar-toggler-icon"></span>
    </button>
    <div id="navbarCollapse" class="collapse navbar-collapse">
        <ul class="navbar-nav mr-auto">
            <li<?php echo (($_GET['show'] == "users") || ($_GET['show'] == "")) ? ' class="nav-item active"' : ''; ?>><a class="nav-link" href="./index.php">Last Heard</a></li>
            <li<?php echo ($_GET['show'] == "links") ? ' class="nav-item active"' : ''; ?>><a class="nav-link" href="./index.php?show=links">Links (<?php echo $Reflector->NodeCount();  ?>)</a></li>
            <li<?php echo ($_GET['show'] == "peers") ? ' class="nav-item active"' : ''; ?>><a class="nav-link" href="./index.php?show=peers">Peers (<?php echo $Reflector->PeerCount();  ?>)</a></li>
        </ul>
        <span class="navbar-text px-2">mrefd v<?php echo $Reflector->GetVersion(); ?> - Dashboard v1.4.0 <?php echo $PageOptions['LocalModification']; ?></span>
        <span class="navbar-text px-2">Service uptime: <?php echo FormatSeconds($Reflector->GetServiceUptime()); ?></span>
    </div>
</nav>
<main role="main">
    <div class="container-fluid">
        <div class="row">
            <?php
                switch ($_GET['show']) {
					case 'users':
						require_once("./include/users.php");
						break;
					case 'links':
						require_once("./include/links.php");
						break;
					case 'peers':
						require_once("./include/peers.php");
						break;
					default:
						require_once("./include/users.php");
                }
            ?>
        </div>
    </div>
</main>
<footer class="container-fluid">
    <nav class="navbar navbar-expand-lg navbar-light fixed-bottom" style="background-color: #e3f2fd;">
        <div id="navbarCollapse" class="collapse navbar-collapse">
            <span class="navbar-text px-2">IPv4: <?php echo $PageOptions['IPV4']; ?></span>
            <span class="navbar-text px-2">IPv6: <?php echo $PageOptions['IPV6']; ?></span>
        </div>
        <span class="navbar-text px-2"><a href="mailto:<?php echo $PageOptions['ContactEmail']; ?>">Sysop Email: <?php echo $PageOptions['ContactEmail']; ?></a></span>
    </nav>
</footer>
</body>
</html>
