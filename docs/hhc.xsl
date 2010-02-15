<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:output method="html" encoding="cp1252" doctype-public="-//IETF//DTD HTML//EN" />

  <xsl:template match="doc">
<HTML>
<HEAD>
<!--<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">-->
</HEAD><BODY>
<OBJECT type="text/site properties">
	<param name="Window Styles" value="0x800025" />
</OBJECT>
    <xsl:apply-templates />
</BODY></HTML>
  </xsl:template>
  <xsl:template match="ul">
<UL>
    <xsl:apply-templates />
</UL>
  </xsl:template>
  <xsl:template match="li">
<LI><OBJECT type="text/sitemap">

<param name="Name"><xsl:attribute name="value"><xsl:for-each select="text()|a/text()"><xsl:value-of select="normalize-space(string(.))" /></xsl:for-each></xsl:attribute></param>

<xsl:if test="a/@href"><param name="Local"><xsl:attribute name="value">sdk\<xsl:value-of select="translate(string(a/@href), '/', '\\')" /></xsl:attribute></param></xsl:if>
<!--<param name="Local" value="sdk\index.html">-->

</OBJECT>
    <xsl:apply-templates />
</LI>
  </xsl:template>
  <xsl:template match="*">
    <xsl:apply-templates  select="*" />
  </xsl:template>
  <xsl:template match="text()" />
</xsl:stylesheet>

