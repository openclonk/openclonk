<?php
/**
 * Provides functionality to check and block
 * flooding attempts by ip.
 *
 * @package C4Masterserver
 * @version 1.1.5-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */
class FloodProtection {

    /**
     * Stores the MySQL connection resource.
     *
     * @var resource
     */
    private $link;

    /**
     * Stores the MySQL table prefix.
     *
     * @var string
     */
    private $prefix;

    /**
     * Stores the maximum alloud requests per second per ip.
     *
     * @var int
     */
    private $maxflood;

    /**
     * The FloodProtection constructor.
     *
     * @param  resource  $link
     * @return FloodProtection
     */
    public function __construct($link, $prefix) {
        $this->link = $link;
        $this->prefix = $prefix;
        $this->maxflood = 5;
    }

    /**
     * Sets the maximum alloud requests per second per ip.
     *
     * @param  int $maxflood
     * @return void
     */
    public function SetMaxflood($maxflood) {
        $this->maxflood = $maxflood;
    }

    /**
     * Checks a request and returns true if the user is flooding.
     *
     * @param  string $ip
     * @return bool
     */
    public function CheckRequest($ip) {
        if(!$this->link) return false;
        if($this->UserKnown($ip)) {
            $this->UpdateUser($ip);
            $this->CleanUp();
            return $this->UserFlooding($ip);
        }
        $this->AddUser($ip);
        $this->CleanUp();
        return false;
    }

    /**
     * Returns, if a user is already in the table.
     *
     * @param  string $ip
     * @return bool
     */
    private function UserKnown($ip) {
        if(!$this->link) return false;
        $query = mysql_query('SELECT `time` FROM `'.$this->prefix.'flood` WHERE `ip` = \' '.$ip. '\' LIMIT 1', $this->link);
        if(mysql_num_rows($query) > 0) {
            return true;
        }
        return false;
    }

    /**
     * Adds a new user to the table.
     *
     * @param  string $ip
     * @return bool
     */
    private function AddUser($ip) {
        if(!$this->link) return false;
        $query = mysql_query('INSERT INTO `'.$this->prefix.'flood` (`ip`, `count`, `time`) VALUES (\' '.$ip. '\',  \'0\',\''. time() .'\') ', $this->link);
        if(!$query) {
            return false;
        }
        return true;
    }

    /**
     * Checks if the given user is flooding the server.
     *
     * @param  string $ip
     * @return bool
     */
    private function UpdateUser($ip) {
        if(!$this->link) return false;
        mysql_query('UPDATE `'.$this->prefix.'flood` SET `count` = \'0\'  WHERE `ip` = \' '.$ip.'\' AND `time` != \''.time().'\'', $this->link);
        mysql_query('UPDATE `'.$this->prefix.'flood` SET `count` = `count`+\'1\', `time` = \''.time().'\'  WHERE `ip` = \' '. mysql_real_escape_string($ip, $this->link).'\'', $this->link);
    }

    /**
     * Checks if the given user is flooding the server.
     *
     * @param  string $ip
     * @return bool
     */
    private function UserFlooding($ip) {
        if(!$this->link) return false;
        $query = mysql_query('SELECT `time` FROM `'.$this->prefix.'flood` WHERE `ip` = \' '.$ip.'\' AND `count` >= \''.$this->maxflood.'\' LIMIT 1', $this->link);
        if(mysql_num_rows($query) > 0) {
            return true;
        }
        return false;
    }

    /**
     * Removes old entrys.
     *
     * @return void
     */
    private function CleanUp() {
        mysql_query('DELETE FROM `'.$this->prefix.'flood` WHERE `time` <= \'' . (time()- 600) . '\'',  $this->link);
    }
}
?>