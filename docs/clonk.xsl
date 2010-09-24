<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:output method="html" encoding="utf-8" doctype-public="-//W3C//DTD HTML 4.01//EN" doctype-system="http://www.w3.org/TR/html4/strict.dtd"/>

  <xsl:variable name="procinst" select="processing-instruction('xml-stylesheet')" />
  <xsl:param name="relpath" select="substring-after(substring-before($procinst, 'clonk.xsl'),'href=&quot;')" />
  <xsl:param name="webnotes" />
  <xsl:param name="fileext" select="'.xml'" />
  <xsl:template name="head">
    <head>
      <xsl:apply-templates mode="head" />
      <link rel="stylesheet">
        <xsl:attribute name="href"><xsl:value-of select="$relpath" />doku.css</xsl:attribute>
      </link>
      <xsl:if test="$webnotes">
      <link rel="stylesheet" href="http://www.openclonk.org/header/header.css" />
      </xsl:if>
      <xsl:if test="descendant::table[bitmask]">
        <script>
          <xsl:attribute name="src"><xsl:value-of select="$relpath" />bitmasks.js</xsl:attribute>
        </script>
<script type="text/javascript">
  var BIT_COUNT = <xsl:value-of select="count(descendant::table[bitmask]/row)" />;		// Anzahl der Bits
  var PREFIX = "bit";		// Prefix für die numerierten IDs
</script>
      </xsl:if>
      <xsl:if test="$webnotes">
<!-- <xsl:processing-instruction name="php">
  $g_page_language = '<xsl:choose><xsl:when test='lang("en")'>english</xsl:when><xsl:otherwise>german</xsl:otherwise></xsl:choose>';
  require_once('<xsl:value-of select="$relpath" />../webnotes/core/api.html');
  pwn_head();
?</xsl:processing-instruction> -->
<script type="text/javascript">
  function switchLanguage() {
    var loc = window.location.href;
    if (loc.match(/\/en\//)) loc = loc.replace(/\/en\//, "/de/");
    else loc = loc.replace(/\/de\//, "/en/");
    window.location = loc;
  }
</script>
      </xsl:if>
    </head>
  </xsl:template>

  <xsl:template match="title" mode="head">
    <title><xsl:value-of select="." /><xsl:apply-templates select="../deprecated" /> -
      OpenClonk <xsl:choose>
        <xsl:when test='lang("en")'>Reference</xsl:when>
        <xsl:otherwise>Referenz</xsl:otherwise>
      </xsl:choose>
    </title>
  </xsl:template>
  <xsl:template match="script">
      <xsl:copy><xsl:apply-templates select="@*|node()" /></xsl:copy>
  </xsl:template>
  <xsl:template match="*" mode="head">
    <xsl:apply-templates mode="head" />
  </xsl:template>
  <xsl:template match="title" />

  <xsl:template name="header">
    <xsl:if test="$webnotes">
<!--<xsl:processing-instruction name="php">
  <xsl:choose><xsl:when test='lang("en")'>
  readfile("http://www.openclonk.org/header/header.html?p=docs");
  </xsl:when><xsl:otherwise>
  readfile("http://www.openclonk.org/header/header.html?p=docsde");
  </xsl:otherwise></xsl:choose>
?</xsl:processing-instruction> -->
      <!-- <xsl:copy-of select='document("header.xml")/*/*' /> -->
      <xsl:apply-templates select="document('header.xml')" />
    </xsl:if>
  </xsl:template>
<!--  <xsl:template match="header//@action">
    <xsl:attribute name="action"><xsl:value-of select="concat($relpath, current())" /></xsl:attribute>
  </xsl:template>-->
  <xsl:template match="header|header//*|header//@*">
      <xsl:copy><xsl:apply-templates select="@*|node()" /></xsl:copy>
  </xsl:template>


  <xsl:template match="deprecated">
    (<xsl:choose><xsl:when test='lang("en")'>deprecated</xsl:when><xsl:otherwise>veraltet</xsl:otherwise></xsl:choose>)
  </xsl:template>

  <xsl:template match="funcs">
    <html>
      <xsl:call-template name="head" />
      <body>
      <xsl:call-template name="header" />
      <div id="content">
        <xsl:call-template name="nav" />
        <xsl:for-each select="func">
          <xsl:apply-templates select="." />
        </xsl:for-each>
        <xsl:apply-templates select="author" />
        <xsl:if test="$webnotes">
<!-- <xsl:processing-instruction name="php">
  pwn_body(basename (dirname(__FILE__)) . basename(__FILE__,".html"), $_SERVER['SCRIPT_NAME']);
?</xsl:processing-instruction> -->
        </xsl:if>
        <xsl:call-template name="nav" />
      </div>
      </body>
    </html>
  </xsl:template>
  
  <xsl:template match="doc">
    <html>
      <xsl:call-template name="head" />
      <body>
      <xsl:call-template name="header" />
      <div id="content">
        <xsl:call-template name="nav" />
        <xsl:apply-templates />
       <xsl:if test="$webnotes">
<!-- <xsl:processing-instruction name="php">
  pwn_body(basename (dirname(__FILE__)) . basename(__FILE__,".html"), $_SERVER['SCRIPT_NAME']);
?</xsl:processing-instruction> -->
        </xsl:if>
        <xsl:call-template name="nav" />
      </div>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="func">
    <h1>
      <xsl:attribute name="id"><xsl:value-of select="title" /></xsl:attribute>
      <xsl:value-of select="title" /><xsl:apply-templates select="deprecated" />
    </h1>
    <div class="text">
    <xsl:apply-templates select="category" />
    <br/>
    <xsl:apply-templates select="version" />
    </div>
    <h2><xsl:choose>
      <xsl:when test='lang("en")'>Description</xsl:when>
      <xsl:otherwise>Beschreibung</xsl:otherwise>
    </xsl:choose></h2>
    <div class="text"><xsl:apply-templates select="desc" /></div>
    <xsl:apply-templates select="syntax" />
    <xsl:for-each select="syntax"><xsl:for-each select="params">
      <h2>Parameter<xsl:if test="count(param)!=1 and lang('en')">s</xsl:if></h2>
      <dl><xsl:for-each select="param">
        <dt><xsl:value-of select="name" />: </dt>
        <dd><div class="text">
          <xsl:apply-templates select="optional" />
          <xsl:apply-templates select="desc" />
        </div></dd>
      </xsl:for-each></dl>
    </xsl:for-each></xsl:for-each>
    <xsl:for-each select="remark">
      <xsl:if test="generate-id(.)=generate-id(../remark[1])">
        <h2><xsl:choose>
          <xsl:when test='lang("en")'>Remark<xsl:if test="count(../remark)!=1">s</xsl:if></xsl:when>
          <xsl:otherwise>Anmerkung<xsl:if test="count(../remark)!=1">en</xsl:if></xsl:otherwise>
        </xsl:choose></h2>
      </xsl:if>
      <div class="text"><xsl:apply-templates /></div>
    </xsl:for-each>
    <xsl:for-each select="examples">
      <h2><xsl:choose>
        <xsl:when test='lang("en")'>Example<xsl:if test="count(example)!=1">s</xsl:if></xsl:when>
        <xsl:otherwise>Beispiel<xsl:if test="count(example)!=1">e</xsl:if></xsl:otherwise>
      </xsl:choose></h2>
      <xsl:apply-templates />
    </xsl:for-each>
    <xsl:apply-templates select="related" />
  </xsl:template>

  <xsl:template match="syntax">
    <h2>Syntax</h2>
    <div class="text fnsyntax">
      <span class="type"><xsl:apply-templates select="rtype" /></span>
      <xsl:if test="not(contains(rtype[1],'&amp;'))"><xsl:text>&#160;</xsl:text></xsl:if>
      <xsl:value-of select="../title" />
      (<xsl:apply-templates select="params" />);
    </div>
  </xsl:template>

  <xsl:template match="params">
    <xsl:for-each select="param">
      <xsl:apply-templates select="." /><xsl:if test="position()!=last()">, </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="param">
    <span class="type"><xsl:value-of select="type" /></span>
    <xsl:if test="not(contains(type[1],'&amp;'))"><xsl:text>&#160;</xsl:text></xsl:if>
    <xsl:value-of select="name" />
  </xsl:template>

  <xsl:template match="optional">
    [opt]
  </xsl:template>
  
  <xsl:template match="category">
    <b><xsl:choose>
      <xsl:when test='lang("en")'>Category: </xsl:when>
      <xsl:otherwise>Kategorie: </xsl:otherwise>
    </xsl:choose></b>
    <xsl:value-of select="." /><xsl:apply-templates select="../subcat" />
  </xsl:template>

  <xsl:template match="subcat">
    / <xsl:value-of select="." />
  </xsl:template>

  <xsl:template match="version">
      <b><xsl:choose>
        <xsl:when test='lang("en")'>Since engine version: </xsl:when>
        <xsl:otherwise>Ab Engineversion: </xsl:otherwise>
      </xsl:choose></b>
      <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="extversion">
    <xsl:choose>
      <xsl:when test='lang("en")'>
        (extended in <xsl:value-of select="." />)
      </xsl:when>
      <xsl:otherwise>
        (erweitert ab <xsl:value-of select="." />)
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="example">
    <div class="example"><xsl:apply-templates /></div>
  </xsl:template>

  <xsl:template match="text">
    <div class="text"><xsl:apply-templates /></div>
  </xsl:template>
  
  <xsl:template match="part">
    <xsl:apply-templates />
  </xsl:template>
  
  <xsl:template match="@id">
    <xsl:attribute name="id"><xsl:value-of select="." /></xsl:attribute>
  </xsl:template>
  <xsl:template match="doc/h">
    <h1><xsl:apply-templates select="@id|node()" /></h1>
  </xsl:template>
  <xsl:template match="doc/part/h">
    <h2><xsl:apply-templates select="@id|node()" /></h2>
  </xsl:template>
  <xsl:template match="doc/part/part/h">
    <h3><xsl:apply-templates select="@id|node()" /></h3>
  </xsl:template>
  <xsl:template match="doc/part/part/part/h">
    <h4><xsl:apply-templates select="@id|node()" /></h4>
  </xsl:template>

  <!-- copy some HTML elements literally -->
  <xsl:template match="img|a|em|strong|br|code/i|code/b|ul|li">
    <xsl:copy>
      <!-- including every attribute -->
<!--      <xsl:for-each select="@*|node()">
        <xsl:copy />
      </xsl:for-each>-->
      <xsl:for-each select="@*">
        <xsl:copy />
      </xsl:for-each>
      <xsl:apply-templates />
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="dl">
    <dl><xsl:apply-templates select="dt|dd" /></dl>
  </xsl:template>
  <xsl:template match="dt">
    <dt><xsl:apply-templates select="@id|node()" /></dt>
  </xsl:template>
  <xsl:template match="dd">
    <dd><xsl:apply-templates /></dd>
  </xsl:template>

  <xsl:template match="related">
    <div class="text">
      <b><xsl:choose>
        <xsl:when test='lang("en")'>See also: </xsl:when>
        <xsl:otherwise>Siehe auch: </xsl:otherwise>
      </xsl:choose></b>
      <xsl:for-each select="*">
        <xsl:sort />
        <xsl:apply-templates select="." /><xsl:if test="position()!=last()">, </xsl:if>
      </xsl:for-each>
    </div>
  </xsl:template>

  <xsl:template match="funclink">
    <a><xsl:attribute name="href"><xsl:value-of select="$relpath" />sdk/script/fn/<xsl:value-of select="." /><xsl:value-of select="$fileext" /></xsl:attribute><xsl:value-of select="." /></a>
  </xsl:template>

  <xsl:template match="emlink" name="link">
    <!-- so this template can be called for the navigation -->
    <xsl:param name="href" select="@href" />
    <xsl:param name="text" select="." />
    <a>
      <xsl:attribute name="href">
        <xsl:value-of select="$relpath" />sdk/<xsl:choose>
          <!-- replace the .html extension with .xml (or whatever) -->
          <xsl:when test="substring-before($href,'.html')">
            <xsl:value-of select="concat(substring-before($href,'.html'), $fileext, substring-after($href,'.html'))" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$href" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:value-of select="$text" />
    </a>
  </xsl:template>

  <xsl:template match="author">
    <div class="author"><xsl:value-of select="." />, <xsl:value-of select="following-sibling::date" /></div>
  </xsl:template>

  <xsl:template match="date" />

  <xsl:template match="table">
    <table>
      <xsl:apply-templates select="caption" />
      <xsl:apply-templates select="rowh" />
      <tbody>
        <xsl:for-each select="row">
          <tr>
            <xsl:apply-templates select="@id" />
            <xsl:if test="../bitmask">
              <xsl:attribute name="style">cursor:pointer;</xsl:attribute>
              <xsl:attribute name="id">bit<xsl:value-of select="position() - 1" /></xsl:attribute>
              <xsl:attribute name="onClick">Switch(<xsl:value-of select="position() - 1" />);</xsl:attribute>
            </xsl:if>
            <xsl:if test="position() mod 2=0"><xsl:attribute name="class">dark</xsl:attribute></xsl:if>
            <xsl:for-each select="col|literal_col">
              <td><xsl:apply-templates select="@colspan|node()"/></td>
            </xsl:for-each>
          </tr>
        </xsl:for-each>
      </tbody>
    </table>
    <xsl:apply-templates select="bitmask" />
  </xsl:template>

  <xsl:template match="table/caption">
    <caption><xsl:apply-templates select="@id|node()" /></caption>
  </xsl:template>

  <xsl:template match="table/bitmask">
    <xsl:value-of select="." />:
    <input id="input" onKeyUp="Calc();" name="input" type="text">
      <xsl:variable name="num"><xsl:choose>
        <xsl:when test="count(../row)&lt;3">1</xsl:when>
        <xsl:when test="count(../row)&lt;6">2</xsl:when>
        <xsl:when test="count(../row)&lt;9">3</xsl:when>
        <xsl:when test="count(../row)&lt;13">4</xsl:when>
        <xsl:when test="count(../row)&lt;16">5</xsl:when>
        <xsl:when test="count(../row)&lt;19">6</xsl:when>
        <xsl:when test="count(../row)&lt;23">7</xsl:when>
        <xsl:when test="count(../row)&lt;26">8</xsl:when>
        <xsl:when test="count(../row)&lt;29">9</xsl:when>
        <xsl:when test="count(../row)&lt;33">10</xsl:when>
      </xsl:choose></xsl:variable>
      <xsl:attribute name="size"><xsl:value-of select="$num" /></xsl:attribute>
      <xsl:attribute name="maxlength"><xsl:value-of select="$num" /></xsl:attribute>
    </input>
  </xsl:template>

  <xsl:template match="rowh">
    <thead><tr>
      <xsl:apply-templates select="@id" />
      <xsl:for-each select="col|literal_col">
        <th><xsl:apply-templates select="@colspan|node()"/></th>
      </xsl:for-each>
    </tr></thead>
  </xsl:template>
  <xsl:template match="@colspan">
    <xsl:attribute name="colspan"><xsl:value-of select="." /></xsl:attribute>
  </xsl:template>

  <xsl:template name="nav"><xsl:if test="$webnotes">
    <ul class="nav">
      <li><xsl:call-template name="link">
        <xsl:with-param name="href" select="'index.html'" />
        <xsl:with-param name="text"><xsl:choose>
          <xsl:when test='lang("en")'>Introduction</xsl:when>
          <xsl:otherwise>Einleitung</xsl:otherwise>
        </xsl:choose></xsl:with-param>
      </xsl:call-template></li>
      <li><a>
        <xsl:attribute name="href"><xsl:value-of select="$relpath" />sdk/content.html</xsl:attribute>
        <xsl:choose>
          <xsl:when test='lang("en")'>Contents</xsl:when>
          <xsl:otherwise>Inhalt</xsl:otherwise>
        </xsl:choose>
      </a></li>
      <li><a>
        <xsl:attribute name="href"><xsl:value-of select="$relpath" />search.php</xsl:attribute>
        <xsl:choose>
          <xsl:when test='lang("en")'>Search</xsl:when>
          <xsl:otherwise>Suche</xsl:otherwise>
        </xsl:choose>
      </a></li>
      <li><xsl:call-template name="link">
        <xsl:with-param name="href" select="'console.html'" />
        <xsl:with-param name="text" select="'Engine'" />
      </xsl:call-template></li>
      <li><xsl:call-template name="link">
        <xsl:with-param name="href" select="'cmdline.html'" />
        <xsl:with-param name="text"><xsl:choose>
          <xsl:when test='lang("en")'>Command Line</xsl:when>
          <xsl:otherwise>Kommandozeile</xsl:otherwise>
        </xsl:choose></xsl:with-param>
      </xsl:call-template></li>
      <li><xsl:call-template name="link">
        <xsl:with-param name="href" select="'files.html'" />
        <xsl:with-param name="text"><xsl:choose>
          <xsl:when test='lang("en")'>Game Data</xsl:when>
          <xsl:otherwise>Spieldaten</xsl:otherwise>
        </xsl:choose></xsl:with-param>
      </xsl:call-template></li>
      <li><xsl:call-template name="link">
        <xsl:with-param name="href" select="'script/index.html'" />
        <xsl:with-param name="text" select="'Script'" />
      </xsl:call-template></li>
      <li class="switchlang"><xsl:choose>
        <xsl:when test='lang("en")'><a href='javascript:switchLanguage()'><img src='/deco/dco_de_sml.gif' alt='German' border='0'/></a></xsl:when>
        <xsl:otherwise><a href='javascript:switchLanguage()'><img src='/deco/dco_en_sml.gif' alt='English' border='0'/></a></xsl:otherwise>
      </xsl:choose></li>
      <!--<li><a><xsl:attribute name="href">index.xml</xsl:attribute>.</a></li>
      <xsl:if test="starts-with($relpath, '../..')">
        <li><a><xsl:attribute name="href">../index.xml</xsl:attribute>..</a></li>
      </xsl:if>-->
    </ul>
  </xsl:if></xsl:template>
  
  <!-- some code blocks are made into paragraphs -->
  <xsl:template match="example/code|part/code|doc/code|dd/code">
    <pre class="code"><xsl:apply-templates /></pre>
  </xsl:template>
  <xsl:template match="code|code/i|code/b">
    <xsl:copy><xsl:apply-templates /></xsl:copy>
  </xsl:template>
  
  <!-- Highlight Comments and Strings -->
  <xsl:template name="color1" match="code/text()">
    <xsl:param name="s" select="." />
    <!-- /**/, //\n -->
    <xsl:param name="tl" select="'/*|//|'" />
    <xsl:param name="tr" select="'*/|&#10;|'" />
    <xsl:param name="wl" select="substring-before($tl, '|')" />
    <xsl:param name="wr" select="substring-before($tr, '|')" />
    <!-- the text before the start marker -->
    <xsl:variable name="l" select="substring-before($s, $wl)" />
    <!-- the text between the start marker and the end marker -->
    <xsl:variable name="m" select="substring-before(substring-after($s, $wl), $wr)" />
    <!-- the text after $l and $m -->
    <xsl:variable name="r" select="substring($s, string-length(concat($l, $wl, $m, $wr)) + 1)" />
    <xsl:choose>
      <!-- something to highlight -->
      <xsl:when test="$m">
        <!-- look for the next pair -->
        <xsl:call-template name="color1">
          <xsl:with-param name="s" select="$l" />
          <!-- the text after the current keyword and before the next | -->
          <xsl:with-param name="wl" select="substring-before(substring-after($tl, concat($wl, '|')), '|')" />
          <xsl:with-param name="wr" select="substring-before(substring-after($tr, concat($wr, '|')), '|')" />
        </xsl:call-template>
        <i>
          <!-- comments in italic -->
          <xsl:value-of select="concat($wl, $m, $wr)" />
        </i>
        <!-- look for the next occurrence of the current pair -->
        <xsl:call-template name="color1">
          <xsl:with-param name="s" select="$r" />
        </xsl:call-template>
      </xsl:when>
      <!-- pairs left? -->
      <xsl:when test="string-length($wr)!=0">
        <xsl:call-template name="color1">
          <xsl:with-param name="s" select="$s" />
          <xsl:with-param name="wl" select="substring-before(substring-after($tl, concat($wl, '|')), '|')" />
          <xsl:with-param name="wr" select="substring-before(substring-after($tr, concat($wr, '|')), '|')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <!-- proceed with the keywords -->
        <xsl:call-template name="color-strings">
          <xsl:with-param name="s" select="$s" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
  <!-- Hightlight stuff like "foo \" bar" correctly. -->
  <xsl:template name="color-strings">
    <xsl:param name="s" select="." />
    <!-- the text before the start marker -->
    <xsl:variable name="l" select="substring-before($s, '&quot;')" />
    <!-- call a template to get the content of the C4Script string -->
    <xsl:variable name="m0">
      <xsl:call-template name="parse-string-escapes">
        <xsl:with-param name="s" select="substring-after($s, '&quot;')" />
      </xsl:call-template>
    </xsl:variable>
    <!-- then call string() on the resulting tree fragment to get it as an xpath string -->
    <xsl:variable name="m" select="string($m0)" />
    <!-- the text after $l and $m -->
    <xsl:variable name="r" select="substring($s, string-length(concat($l, $m)) + 3)" />
    <xsl:choose>
      <!-- something to highlight -->
      <xsl:when test="$m">
        <!-- look for the next pair -->
        <xsl:call-template name="color2">
          <xsl:with-param name="s" select="$l" />
        </xsl:call-template>
        <i>
          <!-- highlight strings in green -->
          <xsl:attribute name="class">string</xsl:attribute>
          <xsl:value-of select="concat('&quot;', $m, '&quot;')" />
        </i>
        <!-- look for the next string -->
        <xsl:call-template name="color-strings">
          <xsl:with-param name="s" select="$r" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <!-- proceed with the keywords -->
        <xsl:call-template name="color2">
          <xsl:with-param name="s" select="$s" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="parse-string-escapes">
    <xsl:param name="s" select="." />
    <xsl:choose>
      <!-- end of string -->
      <xsl:when test="substring($s, 1, 1)='&quot;'">
      </xsl:when>
      <!-- \" -->
      <xsl:when test="substring($s, 1, 2)='\&quot;'">
        <xsl:value-of select="'\&quot;'" />
        <xsl:call-template name="parse-string-escapes">
          <xsl:with-param name="s" select="substring($s, 3)" />
        </xsl:call-template>
      </xsl:when>
	  <!-- anything else -->
      <xsl:when test="$s">
        <xsl:value-of select="substring($s, 1, 1)" />
        <xsl:call-template name="parse-string-escapes">
          <xsl:with-param name="s" select="substring($s, 2)" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  
  <!-- Highlight keywords -->
  <xsl:template name="color2">
    <xsl:param name="s" select="." />
    <!-- the list of keywords -->
    <xsl:param name="t" select="'#include|#appendto|public|private|protected|global|static|var|local|const|int|proplist|object|array|string|bool|return|if|else|break|continue|while|for|func|true|false|nil|'" />
    <xsl:param name="w" select="substring-before($t, '|')" />
    <!-- text before the keyword -->
    <xsl:variable name="l" select="substring-before($s, $w)" />
    <!-- the charecter directly before the keyword -->
    <xsl:variable name="cb" select="substring($l, string-length($l))" />
    <!-- text after the keyword -->
    <xsl:variable name="r" select="substring-after($s, $w)" />
    <!-- the character directly after the keyword -->
    <xsl:variable name="ca" select="substring($r, 1, 1)" />
    <xsl:choose>
      <xsl:when test="string-length($w)=0">
        <xsl:value-of select="$s" />
      </xsl:when>
      <!-- only highlight when the text was found and is not surrounded by other text -->
      <xsl:when test="(contains($s, $w)) and (not(contains('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ', $cb)) or $cb='') and (not(contains('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ', $ca)) or $ca='')">
        <!-- look for the next keyword in the preceding text -->
        <xsl:call-template name="color2">
          <xsl:with-param name="s" select="$l" />
          <xsl:with-param name="w" select="substring-before(substring-after($t, concat($w, '|')), '|')" />
        </xsl:call-template>
        <!-- make the keyword bold -->
        <b><xsl:value-of select="$w" /></b>
        <!-- proceed with the remaining text -->
        <xsl:call-template name="color2">
          <xsl:with-param name="s" select="$r" />
          <xsl:with-param name="w" select="$w" />
        </xsl:call-template>
      </xsl:when>
      <!-- not found: look for the next keyword -->
      <xsl:otherwise>
        <xsl:call-template name="color2">
          <xsl:with-param name="s" select="$s" />
          <xsl:with-param name="w" select="substring-before(substring-after($t, concat($w, '|')), '|')" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>

