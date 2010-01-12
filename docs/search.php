<?php

//parameters: $_GET('func')

//search?
if(isset($_GET['func']) && strlen($_GET['func']) > 0) 
{
	$path = "sdk/script/fn/";
	$search = strtolower($_GET['func']);
	$funcs = array();
	
	
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
				
			array_push($funcs,$item);
		}
	}
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
<title><?php echo $lang == 'de' ? 'Entwicklermodus' : 'Developer Mode' ?></title>
<style>
ul {
list-style-position: inside;
list-style-image: url(images/bullet_sheet.gif);
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
	echo <<<HEADER
<ul class="nav">
<li class="fineprint">Clonk Entwicklermodus Dokumentation</li>
<li><a href="sdk/index.html">Einleitung</a></li>
<li><a href="content.html">Inhalt</a></li>
<li><a href="search.php">Suche</a></li>
<li><a href="sdk/console.html">Engine</a></li>
<li><a href="sdk/cmdline.html">Kommandozeile</a></li>
<li><a href="sdk/files.html">Spieldaten</a></li>
<li><a href="sdk/script/index.html">Script</a></li>
</ul>
<h1>Suche nach Scriptfunktionen</h1>
HEADER;
} else {
	echo <<<HEADER
<ul class="nav"><li class="fineprint">Clonk Developer Mode Documentation</li>
<li><a href="sdk/index.html">Introduction</a></li>
<li><a href="content.html">Contents</a></li>
<li><a href="search.php">Search</a></li>
<li><a href="sdk/console.html">Engine</a></li>
<li><a href="sdk/cmdline.html">Command Line</a></li>
<li><a href="sdk/files.html">Game Data</a></li>
<li><a href="sdk/script/index.html">Script</a></li>
</ul>
<h1>Search for Script Functions</h1>
HEADER;
}
?>
<div class="text">
<form action="search.php" method="get">
<?php
echo $lang == 'de' ? '<b>Suchbegriff:</b>' : '<b>Search term:</b>';
echo '&nbsp;<input type="text" name="func"';
if (isset($_GET['func'])) echo ' value="' . htmlspecialchars($_GET['func']) . '"';
echo '> ';
echo '<input type="submit" value="' . ($lang == 'de' ? 'Suchen' : 'Search') . '">';
?>
</form>
<?php
	if(isset($_GET['func']) && strlen($_GET['func']) > 0)
	{
	//nothing found
		if (count($funcs) == 0)
		{
			echo $lang == 'de' ? 'Es wurde keine Funktion gefunden.' : 'No function found.';
		}
		// something found
		else
		{
			echo "<ul>\n";
			for($i = 0; $i < count($funcs); ++$i)
			{
				$item = $funcs[$i];
				$name = substr($item,0,strpos($item,'.'));
				echo "<li><a href=\"$path$item\">$name</a></li>\n";
			}
			echo "</ul>\n";
		}
	}
?>
</div>

</body></html>
