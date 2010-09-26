<?php
/**
 * C4Masterserver engine backend
 *
 * @package C4Masterserver
 * @version 1.1.5-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */

//error_reporting(E_NONE); //suppress errors

require_once('include/C4Masterserver.php');
require_once('include/C4Network.php');
require_once('include/FloodProtection.php');
require_once('include/ParseINI.php');

$config = file_get_contents('include/config.ini');
$link = mysql_connect(
        ParseINI::ParseValue('mysql_host', $config),
        ParseINI::ParseValue('mysql_user', $config),
        ParseINI::ParseValue('mysql_password', $config)); //connect to MySQL
$db = mysql_selectdb(ParseINI::ParseValue('mysql_db', $config), $link); //select the database

header('Content-Type: text/plain'); //output as plain text

if($link && $db) {
    $server = new C4Masterserver($link, ParseINI::ParseValue('mysql_prefix', $config));
    $server->SetTimeoutgames(intval(ParseINI::ParseValue('c4ms_timeoutgames', $config)));
    $server->SetDeletegames(intval(ParseINI::ParseValue('c4ms_deletegames', $config)));
    $server->SetMaxgames(intval(ParseINI::ParseValue('c4ms_maxgames', $config)));
    $protect = new FloodProtection($link, ParseINI::ParseValue('mysql_prefix', $config));
    $protect->SetMaxflood(intval(ParseINI::ParseValue('flood_maxrequests', $config)));
    if($protect->CheckRequest($_SERVER['REMOTE_ADDR'])) { //flood protection
        C4Network::SendAnswer(C4Network::CreateError('Flood protection.'));
        die();
    }
    $server->CleanUp(true); //Cleanup old stuff
    if(isset($GLOBALS['HTTP_RAW_POST_DATA'])) { //data sent from engine?
        $input = $GLOBALS['HTTP_RAW_POST_DATA'];
        $action = ParseINI::ParseValue('Action', $input);
        $csid = ParseINI::ParseValue('CSID', $input);
        $csid = mysql_real_escape_string($csid, $link);
        $reference = mysql_real_escape_string(strstr($input, '[Reference]'), $link);
        $engine_string = ParseINI::ParseValue('c4ms_engine', $config);
        if(empty($engine_string) || ParseINI::ParseValue('Game', $input) == $engine_string) {
            switch($action) {
                case 'Start': //start a new round
                    if(ParseINI::ParseValue('LeagueAddress', $reference)) {
                        C4Network::SendAnswer(C4Network::CreateError('League not supported!'));
                    }
                    else {
                        $csid = $server->AddReference($reference);
                        if($csid) {
                            C4Network::SendAnswer(C4Network::CreateAnswer(array('Status' => 'Success', 'CSID' => $csid)));
                        }
                        else {
                            C4Network::SendAnswer(C4Network::CreateError('Round signup failed. (To many tries?)'));
                        }
                    }
                    break;
                case 'Update': //update an existing round
                    if($server->UpdateReference($csid, $reference)) {
                        C4Network::SendAnswer(C4Network::CreateAnswer(array('Status' => 'Success')));
                    }
                    else {
                        C4Network::SendAnswer(C4Network::CreateError('Round update failed.'));
                    }
                    break;
                case 'End': //remove a round
                    if($server->RemoveReference($csid)) {
                        C4Network::SendAnswer(C4Network::CreateAnswer(array('Status' => 'Success')));
                    }
                    else {
                        C4Network::SendAnswer(C4Network::CreateError('Round end failed.'));
                    }
                    break;
                default:
                    if (!empty($action)) {
                        C4Network::SendAnswer(C4Network::CreateError('Unknown action.'));
                    }
                    else {
                        C4Network::SendAnswer(C4Network::CreateError('No action defined.'));
                    }
                    break;
            }
        }
        else {
            C4Network::SendAnswer(C4Network::CreateError('Wrong engine.'));
        }
    }
    else { //list availabe games
        $list = $server->GetReferenceArray(false);
        $message = '';
        foreach($list as $reference) {
            if(!empty($message)) $message .= "\n\n";
            $message .= $reference['data'];
            $message .= 'GameId='.$reference['id']."\n";
        }
        C4Network::SendAnswer($message);
    }
    mysql_close($link);
}
else {
    C4Network::SendAnswer(C4Network::CreateError('Database error.'));
}
?>