<?php
/**
 * C4Masterserver main frontend
 *
 * @package C4Masterserver
 * @version 1.1.5-en
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
        ParseINI::ParseValue('mysql_host', $config),
        ParseINI::ParseValue('mysql_user', $config),
        ParseINI::ParseValue('mysql_password', $config)); //connect to MySQL
$db = mysql_selectdb(ParseINI::ParseValue('mysql_db', $config), $link); //select the database

if($link && $db) {
    $server = new C4Masterserver($link, ParseINI::ParseValue('mysql_prefix', $config));
    $server->SetTimeoutgames(intval(ParseINI::ParseValue('c4ms_timeoutgames', $config)));
    $server->SetDeletegames(intval(ParseINI::ParseValue('c4ms_deletegames', $config)));
    $server->SetMaxgames(intval(ParseINI::ParseValue('c4ms_maxgames', $config)));
    $protect = new FloodProtection($link, ParseINI::ParseValue('mysql_prefix', $config));
    $protect->SetMaxflood(intval(ParseINI::ParseValue('flood_maxrequests', $config)));
    if($protect->CheckRequest($_SERVER['REMOTE_ADDR'])) { //flood protection
        header('Content-Type: text/plain');
        die('Flood protection.');
    }
    $games = '';
    $list = $server->GetReferenceArray(true);
    $players = '';
    $count = 0;
    foreach($list as $reference) {
        if($reference['valid']) {
            $games .= '<tr>';
            $games .= '<td>'.htmlspecialchars(ParseINI::ParseValue('Title', $reference['data'])).'</td>';
            $games .= '<td>'.htmlspecialchars(ParseINI::ParseValue('State', $reference['data'])).'</td>';
            $games .= '<td>'.date("Y-m-d H:i", $reference['start']).'</td>';
            $players = '';
            $player_list = ParseINI::ParseValuesByCategory('Name', 'Player', $reference['data']);
            foreach($player_list as $player) {
                if(!empty($players)) $players .= ', ';
                $players .= $player;
            }
            $games .= '<td>'.htmlspecialchars($players).'</td>';
        }
        if((ParseINI::ParseValue('State', $reference['data']) == 'Running') && $reference['time'] >= time() - 60*60*24) {
            $count++;
        }
    }
    $games = C4Network::CleanString($games);
    $server->CleanUp();
}

$dirname = dirname($_SERVER['SCRIPT_NAME']);
$path = '';
if($dirname != '/') {
    $path .= '/';
}
$dirname .= $path.'server/';
$server_link = strtolower($_SERVER['SERVER_NAME'].':'.$_SERVER['SERVER_PORT'].$dirname);
$engine = '';
$engine_string = ParseINI::ParseValue('c4ms_engine', $config);
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
                <p>Powered by C4Masterserver v<?php echo C4Masterserver::GetVersion(); ?> &bull; Coded by Benedict Etzel</p>
            </div>
        </div>
    </body>
</html>