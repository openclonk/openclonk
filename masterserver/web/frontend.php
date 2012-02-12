<?php

require_once(dirname(__FILE__).'/server/include/C4Masterserver.php');
require_once(dirname(__FILE__).'/server/include/C4Network.php');
require_once(dirname(__FILE__).'/server/include/FloodProtection.php');
require_once(dirname(__FILE__).'/server/include/ParseINI.php');

$config = file_get_contents(dirname(__FILE__).'/server/include/config.ini');
$link = mysql_connect(
        ParseINI::parseValue('mysql_host', $config),
        ParseINI::parseValue('mysql_user', $config),
        ParseINI::parseValue('mysql_password', $config)); //connect to MySQL
$db = mysql_selectdb(ParseINI::parseValue('mysql_db', $config), $link); //select the database
$reflist;
$server;

if($link && $db) {
    $server = new C4Masterserver($link, $config);
    $server->setTimeoutgames(intval(ParseINI::parseValue('c4ms_timeoutgames', $config)));
    $server->setDeletegames(intval(ParseINI::parseValue('c4ms_deletegames', $config)));
    $server->setMaxgames(intval(ParseINI::parseValue('c4ms_maxgames', $config)));
    $protect = new FloodProtection($link, ParseINI::parseValue('mysql_prefix', $config));
    $protect->setMaxflood(intval(ParseINI::parseValue('flood_maxrequests', $config)));
    if($protect->checkRequest($_SERVER['REMOTE_ADDR'])) { //flood protection
        $link = NULL;
		$db = NULL;
    }
	else {
		$server->cleanUp();
	}
}

function GetGamesList() {

	if(!gotConnection()) return;
	
    $games = '';
    $list = references();
    $players = '';
    foreach($list as $reference) {
        if($reference['valid']) {
            $games .= '<tr>';
            $games .= '<td>'.htmlspecialchars(ParseINI::parseValue('Title', $reference['data'])).'</td>';
            $games .= '<td>'.htmlspecialchars(ParseINI::parseValue('State', $reference['data'])).'</td>';
            $games .= '<td>'.date("Y-m-d H:i", $reference['start']).'</td>';
            $players = '';
            $player_list = ParseINI::parseValuesByCategory('Name', 'Player', $reference['data']);
            foreach($player_list as $player) {
                if(!empty($players)) $players .= ', ';
                $players .= $player;
            }
            $games .= '<td>'.htmlspecialchars($players).'</td>';
        }
    }
	$games = C4Network::cleanString($games);
	
	$result = "";
	if(!empty($games)) {
		$result .= '<table>';
		$result .= '<tr><th>Round</th><th>State</th><th>Begin</th><th>Players</th></tr>';
		$result .= $games;
		$result .= '</table>';
	}
	else {
		$result .= '<p style="color: gray;">No games are currently running.</p>';
	}
	return $result;
}

function GetGamesCountText($time = 86400) { // default: last 24 hours

	if(!gotConnection()) return;

	$count = 0;
	$list = references();
	foreach($list as $reference) {
		if((ParseINI::parseValue('State', $reference['data']) == 'Running') && $reference['time'] >= time() - $time) {
			$count++;
		}
	}
	$last = $time / 60 / 60;

	if($count > 0) {
		if($count > 1) {
			return $count.' games in the last '.$last.' hours.';
		}
		else {
			return 'One game in the last '.$last.' hours.';
		}
	}
	else {
		return 'No games running in the last '.$last.' hours.';
	}
}

function GetServerLink() {
	
	$dirname = dirname($_SERVER['SCRIPT_NAME']);
	$path = '';
	if($dirname != '/') {
		$path .= '/';
	}
	$dirname .= $path.'server/';
	$server_link = strtolower($_SERVER['SERVER_NAME'].':'.$_SERVER['SERVER_PORT'].$dirname);
	
	return $server_link;
}

function gotConnection() {
	global $link, $db;
	return $link && $db;
}

function references() {
	global $reflist, $server;
	
	if(!gotConnection()) return;
	
	if(!$reflist) $reflist = $server->getReferenceArray(true);
	return $reflist;
}

?>