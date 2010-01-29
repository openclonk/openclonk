#!/usr/bin/perl -w

use strict;

# A quick and dirty script to turn Microsofts hhc-format to strict html in Clonkstyle
# usage: build_contents.pl infile.hhc > outfile.php

#print header

my $lang = 'de';
$lang = 'en' if $ARGV[0] =~ m!/en/!;

sub printheader {
my $title = shift(@_);
print <<HEADER;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
	<title>$title</title>
	<link rel=stylesheet type="text/css" href="doku.css">
	<link rel=stylesheet type="text/css" href="http://www.openclonk.org/header/header.css">
	<style type="text/css">
	ul {
		list-style:none;
		padding:0;
		margin:0;
	}
	ul ul {
		padding-left:1em;
	}
	ul h1, ul h2, ul h3, ul h4, ul h5, ul h6 {
		font-size: 100%;
		font-weight: bold;
		font-style: normal;
		margin: 0;
		border: none;
		padding: 0;
	}
	ul a {
		color: navy; 
		text-decoration: none;  
	}  
	ul a.visited {
		color: navy;  
		text-decoration: none;  
	}
	.collapseimg {
		cursor: pointer;
	}
	.toggleinvisi * .invisi {
		display: none;
	}
	</style>
</head>

<body id='thebody'>
<script type='text/javascript'>
	<!--
	//# Javascript aus mwForum, www.mwforum.org
	function tb(listId) {
		var branch = document.getElementById('brn' + listId);
		var toggle = document.getElementById('tgl' + listId);
		if (!branch || !toggle) return;
		if (branch.style.display != 'none' && branch.className!='invisi') {
			branch.style.display = 'none';
			toggle.src = 'images/bullet_folder.gif';
			toggle.title = 'Zweig expandieren';
			toggle.alt = '+';
		} else {
			branch.style.display = '';
			toggle.src = 'images/bullet_folder_open.gif';
			toggle.title = 'Zweig zusammenklappen';
			toggle.alt = '-';
		}
		branch.className='';
		return true;
	}

	function ta(listId) {
		var branch = document.getElementById('brn' + listId);
		var divs = document.getElementsByTagName('ul');
		var display = '', img = 'bullet_folder_open.gif';
		if (branch.style.display != 'none' && branch.className!='invisi') {
			display = 'none';
			img = 'bullet_folder.gif';
		}
		for (var i=0; i < divs.length; i++) {
			if (divs[i].id.indexOf('brn') == 0) {
				divs[i].style.display = display;
				divs[i].className='';
			}
		}
		var imgs = document.getElementsByTagName('img');
		for (var i=0; i < imgs.length; i++) {
			if (imgs[i].id.indexOf('tgl') == 0) imgs[i].src = 'images/' + img;
		}
		return true;
	}
	
	//# Alle als anfaenglich geschlossen markierten Listitems unsichtbar machen
	//# Beim Laden sind sie sichtbar, damit die Seite ohne Javascript benutzbar ist
	document.getElementById('thebody').className = 'toggleinvisi';
	//-->
</script>
HEADER

if ($lang eq 'de') {
	print <<HEADER;
<?php readfile("http://www.openclonk.org/header/header.php?p=docsde"); ?>
<div id="content">
<ul class="nav">
<li><a href="sdk/index.php">Einleitung</a></li>
<li><a href="content.php">Inhalt</a></li>
<li><a href="search.php">Suche</a></li>
<li><a href="sdk/console.php">Engine</a></li>
<li><a href="sdk/cmdline.php">Kommandozeile</a></li>
<li><a href="sdk/files.php">Spieldaten</a></li>
<li><a href="sdk/script/index.php">Script</a></li>
</ul>
<h1>Inhalt</h1>
<div class="text">
<ul>
HEADER
} else {
	print <<HEADER;
<?php readfile("http://www.openclonk.org/header/header.php?p=docs"); ?>
<div id="content">
<ul class="nav">
<li><a href="sdk/index.php">Introduction</a></li>
<li><a href="content.php">Contents</a></li>
<li><a href="search.php">Search</a></li>
<li><a href="sdk/console.php">Engine</a></li>
<li><a href="sdk/cmdline.php">Command Line</a></li>
<li><a href="sdk/files.php">Game Data</a></li>
<li><a href="sdk/script/index.php">Script</a></li>
</ul>
<h1>Contents</h1>
<div class="text">
<ul>
HEADER
}
}
# skip strange header

while (<>) { last if (m/\<UL\>/); };

my $name = "";
my $h = 2;
my $ids = 0;
my $tabs;
my $header_was_printed = 0;
while (<>) { # assigns each line in turn to $_
	# Assume that html comments appear only as oneliners, multiline comments are a problem for when they are used
	next if (/\<\!\-\-/);
	# This only works when the title appears before the linktarget
	if (m/\<param name="Name" value="([^"]+)"\>/) {
		$name=$1;
		if (!$header_was_printed) {
			printheader($name);
			$header_was_printed = 1;
		}
	}
	# Turn the title to a link when there is a target
	if (m/\<param name="Local" value="([^"]+)"\>/) {
		$name = "<a href=\"$1\">$name</a>";
		# Damn windows-backslashes.
		$name =~ s,\\,/,g;
	}
	# indenting
	$tabs = "\t" x ($h - 2);
	# We have a title?
	if ($name) {
		# the next list item
		if (m/\<LI\>/) {
			print "$tabs\t<li><img src='images/bullet_sheet.gif' alt='-'>\n",
				"$tabs\t$name</li>\n";
			$name="";
		}
		# the list ends here, no more list items to come
		if (m/\<\/UL\>/) {
			print "$tabs\t<li><img src='images/bullet_sheet.gif' alt='-'>\n",
				"$tabs\t$name</li>\n",
				"$tabs</ul>\n",
				"$tabs</li>\n";				
			$name="";
			--$h;
		}
		# a new list which deserves a headline
		if (m/\<UL\>/) {
			++$ids;
			my $x = $h > 2 || "0";
			print "$tabs\t<li>\n",
				"$tabs\t<h$h><img id='tgl$ids' class='collapseimg' src='images/bullet_folder", $x ? "" : "_open", ".gif'",
				" alt='-' onclick='tb($ids)' ondblclick='ta($ids)'>\n",
				"$tabs\t$name</h$h>\n",
				"$tabs\t<ul id='brn$ids'", $x ? " class='invisi'" : "", ">\n";
			$name="";
			++$h;
		}
	} else {
		if (m/\<\/UL\>/) {
		 	print "$tabs</ul>\n" if ($h > 1);
			print "$tabs</li>\n" if ($h > 2);
			--$h;
		}
	}
}

# some additional text at the bottom, too
print <<FOOTER;
</div>
</div>
</body>
</html>
FOOTER
