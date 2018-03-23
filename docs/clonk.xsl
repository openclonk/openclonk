<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:output method="html" encoding="utf-8" doctype-public="-//W3C//DTD HTML 4.01//EN" doctype-system="http://www.w3.org/TR/html4/strict.dtd"/>

  <xsl:variable name="procinst" select="processing-instruction('xml-stylesheet')" />
  <xsl:param name="relpath" select="substring-after(substring-before($procinst, 'clonk.xsl'),'href=&quot;')" />
  <xsl:param name="chm" />
  <xsl:param name="fileext" select="'.xml'" />
  <xsl:template name="head">
    <head>
      <xsl:apply-templates mode="head" />
      <link rel="stylesheet">
        <xsl:attribute name="href"><xsl:value-of select="$relpath" />doku.css</xsl:attribute>
      </link>
      <xsl:if test="descendant::table[bitmask]">
        <script>
          <xsl:attribute name="src"><xsl:value-of select="$relpath" />bitmasks.js</xsl:attribute>
        </script>
        <script type="text/javascript">
          var BIT_COUNT = <xsl:value-of select="count(descendant::table[bitmask]/row)" />;		// Anzahl der Bits
          var PREFIX = "bit";		// Prefix für die numerierten IDs
        </script>
      </xsl:if>
    </head>
  </xsl:template>

  <xsl:template match="title" mode="head">
    <title><xsl:value-of select="." /><xsl:apply-templates select="../deprecated" /> -
      OpenClonk <xsl:choose>
        <xsl:when test='lang("de")'>Referenz</xsl:when>
        <xsl:otherwise>Reference</xsl:otherwise>
      </xsl:choose>
    </title>
  </xsl:template>
  <xsl:template match="script">
      <xsl:copy>
          <xsl:for-each select="@*">
              <xsl:copy />
          </xsl:for-each>
          <xsl:apply-templates />
      </xsl:copy>
  </xsl:template>
  <xsl:template match="func|const" mode="head">
    <xsl:apply-templates mode="head" />
  </xsl:template>
  <xsl:template match="*" mode="head" />
  <xsl:template match="title" />

  <xsl:template name="header">
    <xsl:apply-templates select="document('header.xml')" />
  </xsl:template>
  <xsl:template match="header|header//*|header//@*">
      <xsl:copy><xsl:apply-templates select="@*|node()" /></xsl:copy>
  </xsl:template>


	
  <xsl:template match="deprecated">
    (<xsl:choose><xsl:when test='lang("de")'>veraltet</xsl:when><xsl:otherwise>deprecated</xsl:otherwise></xsl:choose>)
  </xsl:template>

  <xsl:template match="doc|funcs">
    <html>
      <xsl:call-template name="head" />
      <body>
      <xsl:if test="$chm">
        <xsl:attribute name="id">chm</xsl:attribute>
        <xsl:apply-templates />
      </xsl:if>
      <xsl:if test="not($chm)">
        <xsl:call-template name="header" />
        <div id="iframe"><iframe>
          <xsl:attribute name="src"><xsl:value-of select="$relpath" />sdk/content<xsl:value-of select="$fileext" /></xsl:attribute>
        </iframe></div>
        <div id="content">
          <xsl:apply-templates />
        </div>
      </xsl:if>
      </body>
    </html>
  </xsl:template>
  
  <xsl:template match="toc">
    <html>
      <xsl:call-template name="head" />
      <body>
      <div id="toc">
        <xsl:apply-templates />
      </div>
      </body>
    </html>
  </xsl:template>
	
  <xsl:template match="func|const">
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
      <xsl:when test='lang("de")'>Beschreibung</xsl:when>
      <xsl:otherwise>Description</xsl:otherwise>
    </xsl:choose></h2>
    <div class="text"><xsl:apply-templates select="desc" /></div>
    <xsl:apply-templates select="syntax" />
    <xsl:for-each select="syntax"><xsl:for-each select="params">
      <h2><xsl:choose>
          <xsl:when test='lang("de")'>Parameter</xsl:when>
          <xsl:otherwise>Parameter<xsl:if test="count(param)!=1">s</xsl:if></xsl:otherwise>
        </xsl:choose></h2>
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
          <xsl:when test='lang("de")'>Anmerkung<xsl:if test="count(../remark)!=1">en</xsl:if></xsl:when>
          <xsl:otherwise>Remark<xsl:if test="count(../remark)!=1">s</xsl:if></xsl:otherwise>
        </xsl:choose></h2>
      </xsl:if>
      <div class="text"><xsl:apply-templates /></div>
    </xsl:for-each>
    <xsl:for-each select="examples">
      <h2><xsl:choose>
        <xsl:when test='lang("de")'>Beispiel<xsl:if test="count(example)!=1">e</xsl:if></xsl:when>
        <xsl:otherwise>Example<xsl:if test="count(example)!=1">s</xsl:if></xsl:otherwise>
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
			<xsl:if test="parent::func">(<xsl:apply-templates select="params" />);</xsl:if>
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
      <xsl:when test='lang("de")'>Kategorie: </xsl:when>
      <xsl:otherwise>Category: </xsl:otherwise>
    </xsl:choose></b>
    <xsl:value-of select="." /><xsl:apply-templates select="../subcat" />
  </xsl:template>

  <xsl:template match="subcat">
    / <xsl:value-of select="." />
  </xsl:template>

  <xsl:template match="version">
      <b><xsl:choose>
        <xsl:when test='lang("de")'>Ab Engineversion: </xsl:when>
        <xsl:otherwise>Since engine version: </xsl:otherwise>
      </xsl:choose></b>
      <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="extversion">
    <xsl:choose>
      <xsl:when test='lang("de")'>
        (erweitert ab <xsl:value-of select="." />)
      </xsl:when>
      <xsl:otherwise>
        (extended in <xsl:value-of select="." />)
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
  <xsl:template match="doc/part/h|toc/h">
    <h2><xsl:apply-templates select="@id|node()" /></h2>
  </xsl:template>
  <xsl:template match="doc/part/part/h">
    <h3><xsl:apply-templates select="@id|node()" /></h3>
  </xsl:template>
  <xsl:template match="doc/part/part/part/h">
    <h4><xsl:apply-templates select="@id|node()" /></h4>
  </xsl:template>

  <!-- content.xml -->
  <xsl:template match="toc//li">
    <xsl:copy>
      <xsl:for-each select="@*">
        <xsl:copy />
      </xsl:for-each>
      <xsl:choose><xsl:when test="ul">
        <xsl:if test="ancestor::ul/ancestor::ul/ancestor::ul">
          <xsl:attribute name="class">invisi</xsl:attribute>
        </xsl:if>
        <img class='collapseimg'>
          <xsl:attribute name="src">../images/<xsl:choose>
            <xsl:when test="ancestor::ul/ancestor::ul/ancestor::ul">bullet_folder.png</xsl:when>
            <xsl:otherwise>bullet_folder_open.png</xsl:otherwise>
          </xsl:choose></xsl:attribute>
          <xsl:attribute name="alt"><xsl:choose>
            <xsl:when test="ancestor::ul/ancestor::ul/ancestor::ul">+</xsl:when>
            <xsl:otherwise>-</xsl:otherwise>
          </xsl:choose></xsl:attribute>
          <xsl:attribute name="id">tgl<xsl:number level="any" count="ul"/></xsl:attribute>
          <xsl:attribute name="onclick">tb(<xsl:number level="any" count="ul"/>)</xsl:attribute>
          <xsl:attribute name="ondblclick">ta(<xsl:number level="any" count="ul"/>)</xsl:attribute>
        </img>
      </xsl:when><xsl:otherwise>
        <img src='../images/bullet_sheet.png' alt='' />
      </xsl:otherwise></xsl:choose>
      <xsl:apply-templates />
    </xsl:copy>
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
        <xsl:when test='lang("de")'>Siehe auch: </xsl:when>
        <xsl:otherwise>See also: </xsl:otherwise>
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
      <xsl:if test="/toc">
        <xsl:attribute name="target">_top</xsl:attribute>
      </xsl:if>
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
              <td><xsl:apply-templates select="@colspan|@id|node()"/></td>
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
    <xsl:param name="t" select="'#include|#appendto|#warning|public|private|protected|global|static|var|local|const|any|int|bool|def|effect|object|proplist|string|array|func|return|if|else|break|continue|while|for|new|true|false|nil|'" />
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

