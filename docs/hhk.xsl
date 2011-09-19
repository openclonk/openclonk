<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" encoding="cp1252" doctype-public="-//IETF//DTD HTML//EN" />

<!-- Take care: Apparently microsofts html help compiler doesn't parse the html properly,
and needs at least some of the whitespace added with xsl:text below, and perhaps that there not be any in other places. -->

<xsl:template match="toc">
<HTML>
	<HEAD>
	</HEAD>
	<BODY>
		<UL>
			<xsl:apply-templates select=".//li[@class='index']/ul/li" />
		</UL>
	</BODY>
</HTML>
</xsl:template>

<xsl:template match="li">
<LI><xsl:text> </xsl:text><OBJECT type="text/sitemap"><xsl:text>
</xsl:text>
	<param name="Name">
		<xsl:attribute name="value"><xsl:for-each select="text()|emlink/text()"><xsl:value-of select="normalize-space(string(.))" /></xsl:for-each></xsl:attribute>
	</param><xsl:text>
</xsl:text>
	<xsl:if test="emlink/@href"><param name="Local">
		<xsl:attribute name="value">sdk\<xsl:value-of select="translate(string(emlink/@href), '/', '\\')" /></xsl:attribute>
	</param><xsl:text>
</xsl:text></xsl:if>
</OBJECT>
<xsl:apply-templates />
</LI>
</xsl:template>

<xsl:template match="*">
	<xsl:apply-templates  select="*" />
</xsl:template>

<xsl:template match="text()" />

</xsl:stylesheet>
