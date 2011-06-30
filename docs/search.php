<?php

//parameters: $_GET('func')

//search?
if(isset($_GET['search'])) 
{
	if(strlen($_GET['search']) < 3) {
		$less = true;
	}
	else {
		if(isset($_GET['func'])) {
			$path = "sdk/script/fn/";
			$search = strtolower($_GET['search']);
			$result = array();
			
			
			$dir = opendir($path);
			//search
			while (($item = readdir($dir)) !== FALSE) 
			{
				$name = substr($item,0,strpos($item,'.'));
				if ("." != $item && ".." != $item
					&& (strpos(strtolower($name), $search) !== FALSE)
					&& !is_dir($path.$item))
				{
					// exact match -> redirect
					if ($search == strtolower($name))
					{
						header("Location: $path$item");
						exit;
					}
						
					array_push($result,array($path,$item));
				}
			}
			$showresults = 1;
		}
		elseif(isset($_GET['fulltext'])) {
			$result = SearchDir('sdk/');
			$showresults = 2;
		}
	}
}

function SearchDir($path) {
	if(!$dir = opendir($path))
		return;
	
	$result = array();
	
	while (false !== ($file = readdir($dir))) {
		if ($file != "." && $file != "..") {
			if(is_dir($path.$file))
				$result = array_merge($result, SearchDir($path.$file.'/'));
			else {
				// HTML-Dokument auslesen
				$doc = new DOMDocument();
				@$doc->loadHTMLFile($path.$file);
				$divs = $doc->getElementsByTagName('div');
				foreach($divs as $div) {
					if(strpos($div->getAttribute('class'), 'text') !== false) {
						if(strpos(strip_tags($div->nodeValue),htmlspecialchars($_GET['search'])) !== false) {
							$dirname = basename(rtrim($path, '/'));
							if(!isset($result[$dirname]))
								$result[$dirname] = array();
							
							$name = $doc->getElementsByTagName('h1')->item(0)->nodeValue;
							array_push($result[$dirname], array($path.$file,$name));
							break;
						}
					}
				}
			}
		}
	}
	closedir($dir);
	return $result;
}
?>

<?php
$lang = basename(dirname(__FILE__));
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<link rel="stylesheet" type="text/css" href="doku.css">
<link rel="stylesheet" type="text/css" href="http://www.openclonk.org/header/header.css">
<title>OpenClonk <?php echo $lang == 'de' ? 'Referenz' : 'Reference' ?></title>
<style>
ul {
list-style-position: inside;
list-style-image: url(images/bullet_sheet.png);
}
ul a {
color: navy;  
text-decoration: none;  
}  
ul a.visited {
color: navy;  
text-decoration: none;  
}  
</style>
</head>
<body>
<?php
if ($lang == 'de') {
	readfile("http://www.openclonk.org/header/header.php?p=docsde");
} else {
	readfile("http://www.openclonk.org/header/header.php?p=docs");
}
?>
<div id="iframe"><iframe src="sdk/content.html"></iframe></div>
<div id="content">
<h1><?php print ($lang == 'de' ? 'Suche' : 'Search'); ?></h1>
<div class="text">
<form action="search.php" method="get">
<?php
echo '&nbsp;<input type="text" name="search"';
if (isset($_GET['search'])) echo ' value="' . htmlspecialchars($_GET['search']) . '"';
echo '> ';
echo '<input type="submit" name="func" value="' . ($lang == 'de' ? 'Suche' : 'Search') . '"> ';
echo '<input type="submit" name="fulltext" value="' . ($lang == 'de' ? 'Volltext' : 'Fulltext') . '">';
?>
</form>
<?php
	if($less) {
		echo $lang == 'de' ? 'Mindestens 3 Zeichen.' : '3 characters minimum.';
	}
	
	$dirtrans = array('de' => array('sdk' => 'Dokumentation', 'script' => 'Script', 'fn' => 'Funktionen', 'scenario' => 'Szenario', 'particle' => 'Partikel', 'material' => 'Material', 'folder' => 'Rundenordner', 'definition' => 'Objektdefinition'),
	                  'en' => array('sdk' => 'Documentation', 'script' => 'Script', 'fn' => 'Functions', 'scenario' => 'Scenario', 'particle' => 'Particle', 'material' => 'Material', 'folder' => 'Folder', 'definition' => 'Definition'));
	//nothing found
	if($showresults == 1) {
		if (count($result) == 0)
		{
			echo $lang == 'de' ? 'Es wurde keine Funktion gefunden.' : 'No function found.';
		}
		else {
			echo "<ul>\n";
			for($i = 0; $i < count($result); ++$i)
			{
				$item = $result[$i][1];
				if(!$name = $result[$i][2])
					$name = substr($item,0,strpos($item,'.'));
				$path = $result[$i][0];
				echo "<li><a href=\"$path$item\">$name</a></li>\n";
			}
			echo "</ul>\n";
		}
	}
	elseif($showresults == 2) {
		if (count($result) == 0)
		{
			echo $lang == 'de' ? 'Nichts gefunden.' : 'Nothing found.';
		}
		else {
			foreach($result as $dirname => $values) {
				$dirname = $dirtrans[$lang][$dirname];
				echo "<b>$dirname</b>\n";
				echo "<ul>\n";
				foreach($values as $val)
				{
					$item = $val[0];
					$name = $val[1];
					echo "<li><a href=\"$item\">$name</a></li>\n";
				}
				echo "</ul>\n";
			}
		}
	}
?>
</div>
</div>
</body></html>
