<?php
/**
 * C4Masterserver main frontend
 *
 * @package C4Masterserver
 * @version 1.2.0-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */

error_reporting(E_NONE); //suppress errors
header('Content-Type: text/html; charset=utf-8'); //utf-8

require_once('frontend.php');

?>
<!doctype html>
<html>
    <head>
        <title>OpenClonk Masterserver</title>
        <meta charset='utf-8'>
        <link rel='stylesheet' href='masterserver.css'>
        <link rel='stylesheet' href='http://www.openclonk.org/header/header.css'>
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
