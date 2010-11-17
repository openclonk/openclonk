<?php

  /**
   * C4Masterserver engine backend
   *
   * @package C4Masterserver
   * @version 1.2.0-en
   * @author  Benedict Etzel <b.etzel@live.de>
   * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
   */
//error_reporting(E_ALL); //suppress errors

  require_once('include/C4Masterserver.php');
  require_once('include/C4Network.php');
  require_once('include/FloodProtection.php');
  require_once('include/ParseINI.php');

  $config = file_get_contents('include/config.ini');
  $link = mysql_connect(
                  ParseINI::parseValue('mysql_host', $config),
                  ParseINI::parseValue('mysql_user', $config),
                  ParseINI::parseValue('mysql_password', $config)); //connect to MySQL
  $db = mysql_selectdb(ParseINI::parseValue('mysql_db', $config), $link); //select the database

  header('Content-Type: text/plain'); //output as plain text

  if ($link && $db) {
      $prefix = ParseINI::parseValue('mysql_prefix', $config);
      $server = new C4Masterserver($link, $config);
      $server->setTimeoutgames(intval(ParseINI::parseValue('c4ms_timeoutgames', $config)));
      $server->setDeletegames(intval(ParseINI::parseValue('c4ms_deletegames', $config)));
      $server->setMaxgames(intval(ParseINI::parseValue('c4ms_maxgames', $config)));
      $protect = new FloodProtection($link, $prefix);
      $protect->setMaxflood(intval(ParseINI::parseValue('flood_maxrequests', $config)));
      if ($protect->checkRequest($_SERVER['REMOTE_ADDR'])) { //flood protection
          C4Network::sendAnswer(C4Network::createError('Flood protection.'));
          die();
      }
      $server->cleanUp(true); //Cleanup old stuff
      if (ParseINI::parseValue('oc_enable_update', $config) == 1 && isset($_GET['action']) && $_GET['action'] == 'release-file' && isset($_GET['file']) && isset($_GET['hash']) && isset($_GET['new_version']) && isset($_GET['platform'])) {
          $file = ParseINI::parseValue('oc_update_path', $config) . $_GET['file'];
          if (file_exists($file) && hash_hmac_file('sha256', $file, ParseINI::parseValue('oc_update_secret', $config)) == $_GET['hash']) {
              $old_version = isset($_GET['old_version']) ? explode(',', mysql_real_escape_string($_GET['old_version'], $link)) : array();
              $new_version = mysql_real_escape_string($_GET['new_version'], $link);
              $platform = mysql_real_escape_string($_GET['platform'], $link);
              $file = mysql_real_escape_string($file, $link);
              if (!empty($old_version)) {
                  if (isset($_GET['delete_old_files']) && $_GET['delete_old_files'] == 'yes') {
                      $result = mysql_query('SELECT `file` FROM `' . $prefix . 'update` WHERE `new_version` != \'' . $new_version . '\' AND `old_version` != \'\' AND `platform` = \'' . $platform . '\'');
                      while (($row = mysql_fetch_assoc($result)) != false) {
                          unlink(ParseINI::parseValue('oc_update_path', $config) . $row['file']);
                      }
                  }
                  mysql_query('DELETE FROM `' . $prefix . 'update` WHERE `new_version` != \'' . $new_version . '\' AND `old_version` != \'\' AND `platform` = \'' . $platform . '\'');
                  foreach ($old_version as $version) {
                      mysql_query('INSERT INTO `' . $prefix . 'update` (`old_version`, `new_version`, `platform`, `file`) VALUES (\'' . $version . '\', \'' . $new_version . '\', \'' . $platform . '\', \'' . $file . '\')');
                  }
              } else {
                  if (isset($_GET['delete_old_files']) && $_GET['delete_old_files'] == 'yes') {
                      $row = mysql_fetch_assoc(mysql_query('SELECT `file` FROM `' . $prefix . 'update` WHERE `old_version` = \'\' AND `platform` = \'' . $platform . '\''));
                      unlink(ParseINI::parseValue('oc_update_path', $config) . $row['file']);
                  }
                  mysql_query('DELETE FROM `' . $prefix . 'update` WHERE `old_version` = \'\' AND `platform` = \'' . $platform . '\'');
                  mysql_query('INSERT INTO `' . $prefix . 'update` (`old_version`, `new_version`, `platform`, `file`) VALUES (\'\', \'' . $new_version . '\', \'' . $platform . '\', \'' . $file . '\')');
              }
          } else {
              C4Network::sendAnswer(C4Network::createError('File not found or hash incorrect.'));
          }
      } else if (isset($GLOBALS['HTTP_RAW_POST_DATA'])) { //data sent from engine?
          $input = $GLOBALS['HTTP_RAW_POST_DATA'];
          $action = ParseINI::parseValue('Action', $input);
          $csid = ParseINI::parseValue('CSID', $input);
          $csid = mysql_real_escape_string($csid, $link);
          $reference = mysql_real_escape_string(strstr($input, '[Reference]'), $link);
          $engine_string = ParseINI::parseValue('c4ms_engine', $config);
          if (empty($engine_string) || ParseINI::parseValue('Game', $input) == $engine_string) {
              switch ($action) {
                  case 'Start': //start a new round
                      if (ParseINI::parseValue('LeagueAddress', $reference)) {
                          C4Network::sendAnswer(C4Network::createError('League not supported!'));
                      } else {
                          $csid = $server->addReference($reference);
                          if ($csid) {
                              C4Network::sendAnswer(C4Network::createAnswer(array('Status' => 'Success', 'CSID' => $csid)));
                          } else {
                              C4Network::sendAnswer(C4Network::createError('Round signup failed. (To many tries?)'));
                          }
                      }
                      break;
                  case 'Update': //update an existing round
                      if ($server->updateReference($csid, $reference)) {
                          C4Network::sendAnswer(C4Network::createAnswer(array('Status' => 'Success')));
                      } else {
                          C4Network::sendAnswer(C4Network::createError('Round update failed.'));
                      }
                      break;
                  case 'End': //remove a round
                      if ($server->removeReference($csid)) {
                          C4Network::sendAnswer(C4Network::createAnswer(array('Status' => 'Success')));
                      } else {
                          C4Network::sendAnswer(C4Network::createError('Round end failed.'));
                      }
                      break;
                  default:
                      if (!empty($action)) {
                          C4Network::sendAnswer(C4Network::createError('Unknown action.'));
                      } else {
                          C4Network::sendAnswer(C4Network::createError('No action defined.'));
                      }
                      break;
              }
          } else {
              C4Network::sendAnswer(C4Network::createError('Wrong engine, "' . ParseINI::parseValue('Game', $input) . '" expected.'));
          }
      } else { //list availabe games
          if (!isset($_GET['action']) || $_GET['action'] == 'version')
              $list = $server->getReferenceArray(false);
          $message = '';
          $engine = ParseINI::parseValue('c4ms_title_engine', $config);
          $platform = isset($_GET['platform']) ? mysql_real_escape_string($_GET['platform'], $link) : 0;
          $client_version = isset($_GET['version']) ? mysql_real_escape_string($_GET['version'], $link) : 0;
          if (!empty($engine)) {
              $message .= '[' . $engine . ']' . PHP_EOL;
              if (ParseINI::parseValue('oc_enable_update', $config) == 1) {
                  if ($platform && $client_version) {
                      $result = mysql_query('SELECT `new_version` FROM `' . ParseINI::parseValue('mysql_prefix', $config) . 'update` WHERE `old_version` = \'\' AND `platform` = \'' . $platform . '\'');
                      $row = mysql_fetch_assoc($result);
                      $version = $row['new_version'];
                      if ($version) {
                          $message .= 'Version=' . $version . PHP_EOL;
                          if (version_compare($client_version, $version) < 0) { //We need to update
                              $result = mysql_query('SELECT `file` FROM `' . $prefix . 'update` WHERE `old_version` = \'' . $client_version . '\' AND `platform` = \'' . $platform . '\'');
                              $row = mysql_fetch_assoc($result);
                              if ($row['file'])
                                  $message .= 'UpdateURL=' . ParseINI::parseValue('oc_update_url', $config) . $row['file'] . PHP_EOL;
                          }
                      }
                  }
              }
              $motd = ParseINI::parseValue('c4ms_motd', $config);
              if (!empty($motd))
                  $message .= 'MOTD=' . $motd . PHP_EOL;
          }
          foreach ($list as $reference) {
              if (!empty($message))
                  $message .= PHP_EOL;
              $message .= $reference['data'];
              $message .= 'GameId=' . $reference['id'] . PHP_EOL;
              $message .= 'OfficialServer=false' . PHP_EOL;
          }
          C4Network::sendAnswer($message);
      }
      mysql_close($link);
  }
  else {
      C4Network::sendAnswer(C4Network::createError('Database error.'));
  }
?>
