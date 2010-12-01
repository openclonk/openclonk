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

require_once('frontend.php');

?>
<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>
<html>
    <head>
        <title>OpenClonk Masterserver</title>
        <meta http-equiv='content-type' content='text/html; charset=utf-8'>
        <meta http-equiv='content-style-type' content='text/css'>
        <link rel='stylesheet' href='masterserver.css' type='text/css'>
        <link rel='stylesheet' href='http://www.openclonk.org/header/header.css' type='text/css'>
    </head>
    <body>
		<?php include("http://www.openclonk.org/header/header.php?p=masterserver"); ?>
        <div id="masterserver">
            <h1>Masterserver</h1>
            <?php
            if($link && $db) {
				echo GetGamesList();				
				echo "<p>";
				echo GetGamesCountText();
				echo "</p>";
                echo '<p>Server address <strong>'.GetServerLink().'</strong>';
            }
            else {
                echo '<p class="error">Error: Could not connect to database specified in config.</p>';
            }
            ?>
            <div class="footer">
                <p>Powered by C4Masterserver v<?php echo C4Masterserver::GetVersion(); ?> &raquo; by Benedict Etzel</p>
            </div>
        </div>
    </body>
</html>