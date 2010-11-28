<?php
/**
 * Provides functionality to parse key-value-pairs
 * from an ini-like string.
 *
 * @package C4Masterserver
 * @version 1.2.0-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */
abstract class ParseINI {

	/**
     * Normalizes the given text to unix line endings (lf).
     *
     * @param string $text
     * @return string
     */
	public static function normalize($text) {
		$text = str_replace("\r\n", "\n", $text);
		$text = str_replace("\r", "\n", $text);
		$text = preg_replace("/\n{2,}/", "\n\n", $text);
		return $text;
	}

    /**
     * Returns the value belonging to the key from an ini-like string.
     *
     * @param string $key
     * @param string $string
     * @return string
     */
    public static function parseValue($key, $string) {
		if(!$key || !$string) {
            return false;
        }
        $string = ParseINI::normalize($string);
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
        $value = trim($value, '"');
        if($value == 'true') $value = 1;
        if($value == 'false') $value = 0;
        return $value;
    }

    /**
     * Returns all values in a certain category and supports multiple appereances.
     *
     * @param string $key
     * @param string $category
     * @param string $string
     * @return array
     */
    public static function parseValuesByCategory($key, $category, $string) {
        if(!$key || !$string || !$category) {
            return false;
        }
		$string = ParseINI::normalize($string);
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
            $values[] = ParseINI::parseValue($key, $line);
        }
        return $values;
    }
}
?>
