<?php
/**
 * C4Masterserver main frontend
 *
 * @package C4Masterserver
 * @version 1.2.0-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */

//error_reporting(E_NONE); //suppress errors

require_once('server/include/C4Masterserver.php');
require_once('server/include/C4Network.php');
require_once('server/include/FloodProtection.php');
require_once('server/include/ParseINI.php');

$config = file_get_contents('server/include/config.ini');
$link = mysql_connect(
        ParseINI::parseValue('mysql_host', $config),
        ParseINI::parseValue('mysql_user', $config),
        ParseINI::parseValue('mysql_password', $config)); //connect to MySQL
$db = mysql_selectdb(ParseINI::parseValue('mysql_db', $config), $link); //select the database

if($link && $db) {
    $server = new C4Masterserver($link, $config);
    $server->setTimeoutgames(intval(ParseINI::parseValue('c4ms_timeoutgames', $config)));
    $server->setDeletegames(intval(ParseINI::parseValue('c4ms_deletegames', $config)));
    $server->setMaxgames(intval(ParseINI::parseValue('c4ms_maxgames', $config)));
    $protect = new FloodProtection($link, ParseINI::parseValue('mysql_prefix', $config));
    $protect->setMaxflood(intval(ParseINI::parseValue('flood_maxrequests', $config)));
    if($protect->checkRequest($_SERVER['REMOTE_ADDR'])) { //flood protection
        header('Content-Type: text/plain');
        die('Flood protection.');
    }
    $games = '';
    $list = $server->getReferenceArray(true);
    $players = '';
    $count = 0;
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
        if((ParseINI::parseValue('State', $reference['data']) == 'Running') && $reference['time'] >= time() - 60*60*24) {
            $count++;
        }
    }
    $games = C4Network::cleanString($games);
    $server->cleanUp();
}

$dirname = dirname($_SERVER['SCRIPT_NAME']);
$path = '';
if($dirname != '/') {
    $path .= '/';
}
$dirname .= $path.'server/';
$server_link = strtolower($_SERVER['SERVER_NAME'].':'.$_SERVER['SERVER_PORT'].$dirname);
$engine = '';
$engine_string = ParseINI::parseValue('c4ms_engine', $config);
if(!empty($engine_string)) {
    $engine = '('.$engine_string.' only)';
}
?>
<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>
<html>
    <head>
        <title>C4Masterserver</title>
        <meta http-equiv='content-type' content='text/html; charset=utf-8'>
        <meta http-equiv='content-style-type' content='text/css'>
        <link rel='stylesheet' href='masterserver.css' type='text/css'>
    </head>
    <body>
        <div id="masterserver">
            <h1>Masterserver</h1>
            <?php
            if($link && $db) {
                echo '<p>You can reach the masterserver by using the address <strong>'.$server_link.'</strong>';
                if($engine) {
                    echo ' '.$engine;
                }
                echo '.</p>';
                if(!empty($games)) {
                    echo '<p style="color: gray;">Following games are running now:</p>';
                    echo '<table>';
                    echo '<tr><th>Round</th><th>State</th><th>Begin</th><th>Players</th></tr>';
                    echo $games;
                    echo '</table>';
                }
                else {
                    echo '<p style="color: gray;">No games are currently running.</p>';
                }
                if($count > 0) {
                    if($count > 1) {
                        echo '<p>There have been '.$count.' running games in the last 24 hours.</p>';
                    }
                    else {
                        echo '<p>There has been one running game in the last 24 hours.</p>';
                    }
                }
                else {
                    echo '<p>There have been no games running in the last 24 hours.</p>';
                }
            }
            else {
                echo '<p style="color: red;">Error: Could not connect to database specified in config.</p>';
            }
            ?>
            <div id="masterserver_footer">
                <p>Powered by C4Masterserver v<?php echo C4Masterserver::GetVersion(); ?> &raquo; Coded by Benedict Etzel</p>
            </div>
        </div>
    </body>
</html>