<?php
/**
 * Provides functionality to add, update and remove
 * game references based on a MySQL table.
 *
 * @package C4Masterserver
 * @version 1.1.5-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */
class C4Masterserver {

    /**
     * Stores the masterserver version.
     *
     * @var string
     */
    static public $version = '1.1.5';

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
     * Stores the seconds after which games timeout.
     *
     * @var int
     */
    private $timeoutgames;

    /**
     * Stores the seconds after which games are removed.
     *
     * @var int
     */
    private $deletegames;

    /**
     * Stores the maximum amount of games per ip.
     *
     * @var int
     */
    private $maxgames;


    /**
     * The C4Masterserver constructor.
     *
     * @param resource $link
     * @return C4Masterserver
     */
    public function __construct($link, $prefix) {
        $this->link = $link;
        $this->prefix = $prefix;
        $this->timeoutgames = 600;
        $this->deletegames = 60*60*24;
        $this->maxgames = 5;
    }

    /**
     * Returns the C4Masterserver version.
     *
     * @param void
     * @return string
     */
    public static function GetVersion() {
        return(C4Masterserver::$version);
    }

    /**
     * Sets the seconds after which games timeout.
     *
     * @param int $timeoutgames
     * @return void
     */
    public function SetTimeoutgames($timeoutgames) {
        $this->timeoutgames = $timeoutgames;
    }

    /**
     * Sets the seconds after which games are removed.
     *
     * @param int $deletegames
     * @return void
     */
    public function SetDeletegames($deletegames) {
        $this->deletegames = $deletegames;
    }

    /**
     * Sets  the maximum amount of games per ip.
     *
     * @param int $maxgames
     * @return void
     */
    public function SetMaxgames($maxgames) {
        $this->maxgames = $maxgames;
    }

    /**
     * Returns all valid references. If $show_all is true, it returns all references.
     *
     * @param bool $show_all
     * @return array
     */
    public function GetReferenceArray($show_all = false) {
        if(!$this->link) return false;
        $append = $show_all ? '' : 'WHERE `valid` = \'1\'';
        $result = mysql_query('SELECT * FROM `'.$this->prefix.'games`'.$append);
        $list = array();
        while($row = mysql_fetch_assoc($result)) {
            $list[$row['id']] = $row;
        }
        return $list;
    }

    /**
     * Adds a new reference and returns the new CSID.
     *
     * @param string $reference
     * @return string
     */
    public function AddReference($reference) {
        if(!$this->link) return false;
        $reference = str_replace('0.0.0.0', $_SERVER['REMOTE_ADDR'], $reference);
        $query = mysql_query('SELECT * FROM `'.$this->prefix.'games` WHERE `ip` = \''.$_SERVER['REMOTE_ADDR'].'\' AND `valid` = \'1\'', $this->link);
        if(mysql_num_rows($query) > $this->maxgames) {
            return false;
        }
        $csid = sha1(uniqid(mt_rand(), true));
        mysql_query('INSERT INTO `'.$this->prefix.'games` (`id`, `ip`,`csid`, `data`, `start`, `time`, `valid`) VALUES (\'\', \''.$_SERVER['REMOTE_ADDR'].'\', \''.$csid.'\', \''.$reference.'\', \''.time().'\', \''.time().'\', \'1\')', $this->link);
        return $csid;
    }

    /**
     * Updates a reference.
     *
     * @param string $csid
     * @param string $newreference
     * @return bool
     */
    public function UpdateReference($csid, $newreference) {
        if(!$this->link) return false;
        $newreference = str_replace('0.0.0.0', $_SERVER['REMOTE_ADDR'], $newreference);
        mysql_query('UPDATE `'.$this->prefix.'games` SET `data`=\''.$newreference.'\',`time` = \''.time().'\', `valid` = \'1\' WHERE `csid` = \''.$csid.'\'', $this->link);
        return true;
    }

    /**
     * Removes a reference by setting it invalid.
     *
     * @param string $csid
     * @return bool
     */
    public function RemoveReference($csid) {
        if(!$this->link) return false;
        mysql_query('UPDATE `'.$this->prefix.'games` SET `time` = \''.time().'\', `valid` = \'0\' WHERE `csid` = \''.$csid.'\'', $this->link);
        return true;
    }

    /**
     * Sets old references invalid and removes very old ones if $remove is true.
     *
     * @param void
     * @return void
     */
    public function CleanUp() {
        if(!$this->link) return false;
        if($this->timeoutgames != 0) {
            mysql_query('UPDATE `'.$this->prefix.'games` SET `valid` = \'0\' WHERE `time` < \''.(time() - $this->timeoutgames).'\'', $this->link);
        }
        if($this->deletegames != 0) {
            mysql_query('DELETE FROM `'.$this->prefix.'games` WHERE `valid` = \'0\' AND `time` < \''.(time() - $this->deletegames).'\'', $this->link);
        }
    }
}
?>
