<?php
/**
 * Provides functionality to communicate with a
 * Clonk 4 engine with ini-style strings.
 *
 * @package C4Masterserver
 * @version 1.1.5-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */
abstract class C4Network {

    /**
     * Creates a Clonk 4 conform answer string.
     *
     * @param array $data
     * @return string
     */
    public static function CreateAnswer($data) {
        $message = '[Response]'."\n";
        foreach ($data as $key => $value) {
            $message .= $key.'='.$value."\n";
        }
        return $message;
    }

    /**
     * Returns a Clonk 4 conform error string.
     *
     * @param string message
     * @return string
     */
    public static function CreateError($message) {
        return C4Network::CreateAnswer(array("Status" => "Failure", "Message" => $message));
    }

    /**
     * Sends a Clonk 4 conform answer string.
     *
     * @param string $message
     * @return void
     */
    public static function SendAnswer($message) {
        header('Content-Length: '.strlen($message));
        echo $message;
    }

    /**
     * Cleans a Clonk 4 conform text sting human readable.
     *
     * @param string $message
     * @return string
     */
    public static function CleanString($message) {
        $coded = $decoded = array();
        preg_match_all('|\\\[0-9]{3}|', $message, $coded);
        foreach($coded[0] as $numstr) {
            $num = ereg_replace("[^0-9]", "", $numstr);
            $decoded[$num] = C4Network::DecodeEntitiyString($num);
        }
        foreach($decoded as $num => $entity) {
            $message = str_replace('\\'.$num, $entity, $message);
        }
        return $message;
    }

    /**
     * Decodes Clonk 4 conform entitiy string to its character.
     *
     * @param string $string
     * @return string
     */
    public static function DecodeEntitiyString($string) {
        $num = ereg_replace("[^0-9]", "", $string);
        $num = octdec($num);
        return iconv('Windows-1252', 'UTF-8', chr($num));
    }
}
?>