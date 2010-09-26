<?php
/**
 * Provides functionality to parse key-value-pairs
 * from an ini-like string.
 *
 * @package C4Masterserver
 * @version 1.1.5-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */
abstract class ParseINI {

    /**
     * Returns the value belonging to the key from an ini-like string.
     *
     * @param string $key
     * @param string $string
     * @return string
     */
    public static function ParseValue($key, $string) {
        if(!$key || !$string) {
            return false;
        }
        if(strpos($string, $key) === false) {
            return false;
        }
        $value = '';
        $key_start = strpos($string, $key."=") + strlen($key) + 1;
        $key_end = strpos($string, "\n", $key_start);
        if($key_end === false) {
            $value = substr($string, $key_start);
        }
        else {
            $value = substr($string, $key_start, $key_end - $key_start);
        }
        $value = trim($value);
        return trim($value, '"');
    }

    /**
     * Returns all values in a certain category and supports multiple appereances.
     *
     * @param string $key
     * @param string $category
     * @param string $string
     * @return array
     */
    public static function ParseValuesByCategory($key, $category, $string) {
        if(!$key || !$string || !$category) {
            return false;
        }
        if(strpos($string, $key) === false || strpos($string, '['.$category.']') === false) {
            return false;
        }
        $values = array();
        $lines = explode("\n", $string);
        $current_category = '';
        foreach($lines as $line) {
            $line = trim($line);
            if(strpos($line, '[') !== false && strpos($line, ']') !== false && strpos($line, '=') === false) {
                $start = strpos($line, '[') + 1;
                $end = strpos($line, ']') - 1;
                $current_category = substr($line, $start, $end);
            }
            if($current_category != $category) continue; //wrong category
            if(strpos($line, ';') === 0) continue; //comment
            if(strpos($line, $key) === false) continue; //not needed
            $value = ParseINI::ParseValue($key, $line);
            $value = trim($value);
            $values[] = trim($value, '"');
        }
        return $values;
    }
}
?>
