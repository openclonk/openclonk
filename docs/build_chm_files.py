#!/usr/bin/env python
# -*- coding: iso-8859-15 -*-
import sys
import xml.sax
import experimental
import gettext

class ClonkEntityResolver(xml.sax.handler.EntityResolver):
    def resolveEntity(self, publicId, systemId):
        s = 'file:sdk/script/fn/' + systemId
        return s
    def setrelpath(self, path):
        dirlist = path.split('/')
        self.relpath = ''
        for d in dirlist[1:]:
            self.relpath = self.relpath + '../'

class Clonkparser(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.cats = { }
        self.subcats = { }
        self.versions = { }
        self.extversions = { }
        self.files = { }
    def setfilename(self, filename):
        self.filename = filename
        self.htmlfilename = filename[:-3] + 'html'
    def _setcurcat(self, curcat):
        self.curcat = curcat
        if self.curcat not in self.cats:
            self.cats[self.curcat] = { }
            self.subcats[self.curcat] = { }
    def startDocument(self):
        self.cur = ""
        self.curcat = ""
        self.title = ""
        self.state = None
    def startElement(self, name, attr):
        # subcat inside category?
        if self.state == 'category' and self.cur != "":
            self._setcurcat(self.cur)
        self.cur = ""
        self.state = name
    def characters(self, content):
        if self.state in ['category', 'subcat', 'version', 'extversion', 'title']:
            self.cur += content
    def endElement(self, name):
        self.cur = self.cur.strip()
        if name == 'category':
            if self.cur != "":
                self._setcurcat(self.cur)
                if self.title == "":
                    print "WARNING: category before title in " + self.filename
                self.cats[self.curcat][self.title] = self.htmlfilename
            else:
                print "WARNING: possibly broken category in " + self.filename
        elif name == 'subcat':
            if self.curcat != "" and self.cur != "":
                self.cats[self.curcat].pop(self.title, None)
                if self.cur not in self.subcats[self.curcat]:
                    self.subcats[self.curcat][self.cur] = { }
                self.subcats[self.curcat][self.cur][self.title] = self.htmlfilename
            else:
                print "WARNING: possibly broken subcategory in " + self.filename
        elif name == 'version':
            self.cur = self.cur.upper()
            if self.cur != "":
                if self.cur not in self.extversions:
                    self.extversions[self.cur] = { }
                if self.cur not in self.versions:
                    self.versions[self.cur] = { }
                self.versions[self.cur][self.title] = self.htmlfilename
        elif name == 'extversion':
            self.cur = self.cur.upper()
            if self.cur != "":
                if self.cur not in self.extversions:
                    self.extversions[self.cur] = { }
                if self.cur not in self.versions:
                    self.versions[self.cur] = { }
                self.extversions[self.cur][self.title] = self.htmlfilename
        elif name == 'title':
            self.title = self.cur
            self.files[self.title] = self.htmlfilename

        self.cur = ""
        self.state = None

def printcontents1(f, _):
    f.write('\n      <UL>\n')
    cats = parser.cats.keys()
    cats.sort()
    for cat in cats:
        f.write('        <LI> <OBJECT type="text/sitemap">\n' +
            '          <param name="Name" value="' + _(cat) + '">\n' +
            '          </OBJECT>\n' +
            '        <UL>\n')
        subcats = parser.subcats[cat].keys()
        subcats.sort()
        for subcat in subcats:
            f.write('          <LI> <OBJECT type="text/sitemap">\n' +
                '            <param name="Name" value="' + _(subcat) + '">\n' +
                '            </OBJECT>\n' +
                '          <UL>\n')
            titles = parser.subcats[cat][subcat].keys()
            titles.sort()
            for title in titles:
                f.write('            <LI> <OBJECT type="text/sitemap">\n' +
                    '              <param name="Name" value="' + _(title) + '">\n' +
                    '              <param name="Local" value="' +
                    parser.subcats[cat][subcat][title] + '#' + _(title) + '">\n' +
                    '              </OBJECT>\n')
            f.write('          </UL>\n')
        titles = parser.cats[cat].keys()
        titles.sort()
        for title in titles:
            f.write('          <LI> <OBJECT type="text/sitemap">\n' +
                '            <param name="Name" value="' + _(title) + '">\n' +
                '            <param name="Local" value="' +
                parser.cats[cat][title] + '#' + _(title) + '">\n' +
                '            </OBJECT>\n')
        f.write('        </UL>\n')
    f.write('        </UL>\n')

def printcontents2(f, _):
    f.write('        <UL>\n')
    versions = parser.versions.keys()
    versions.sort()
    for version in versions:
        f.write('        <LI> <OBJECT type="text/sitemap">\n' +
            '          <param name="Name" value="' + _(version) + '">\n' +
            '          </OBJECT>\n' +
            '        <UL>\n')
        titles = parser.versions[version].keys()
        titles.sort()
        for title in titles:
            f.write('          <LI> <OBJECT type="text/sitemap">\n' +
                '            <param name="Name" value="' + _(title) + '">\n' +
                '            <param name="Local" value="' +
                parser.versions[version][title] + '#' + _(title) + '">\n' +
                '            </OBJECT>\n')
        titles = parser.extversions[version].keys()
        titles.sort()
        for title in titles:
            f.write('          <LI> <OBJECT type="text/sitemap">\n' +
                '            <param name="Name" value="' + _(title) + ' (' + _('erweitert') + ')">\n' +
                '            <param name="Local" value="' +
                parser.extversions[version][title] + '#' + _(title) + '">\n' +
                '            </OBJECT>\n')
        f.write('        </UL>\n')
    f.write('      </UL>\n')

def printcontents3(f, _):
    ihack = [1000]
    def folder(name):
        i = str(ihack[0])
        f.write("<li><img id='tgl" + i + "' class='collapseimg' src='../images/bullet_folder.gif' alt='-' onclick='tb(" + i + ")' ondblclick='ta(" + i + ")' />\n" +
            name + "\n" +
            "<ul id='brn" + i + "' class='invisi'>\n")
        ihack[0] = ihack[0] + 1
    def sheet(url, name):
        f.write("<li><img src='../images/bullet_sheet.gif' alt='' />\n" +
            "<a href='" + url[4:] + "'>" + name + "</a></li>\n")
    def sheetE(url, name):
        f.write("<li><img src='../images/bullet_sheet.gif' alt='' />\n" +
            "<a href='" + url[4:] + "'>" + name + "</a> (erweitert)</li>\n")
    folder("Funktionen nach Kategorie")
    cats = parser.cats.keys()
    cats.sort()
    for cat in cats:
        folder(_(cat))
        subcats = parser.subcats[cat].keys()
        subcats.sort()
        for subcat in subcats:
            folder(_(subcat))
            titles = parser.subcats[cat][subcat].keys()
            titles.sort()
            for title in titles:
                sheet(parser.subcats[cat][subcat][title] + '#' + _(title), _(title))
            f.write('</ul></li>\n')
        titles = parser.cats[cat].keys()
        titles.sort()
        for title in titles:
            sheet(parser.cats[cat][title] + '#' + _(title), _(title))
        f.write('</ul></li>\n')
    f.write('</ul></li>\n')
    folder("Funktionen nach Version")
    versions = parser.versions.keys()
    versions.sort()
    for version in versions:
        folder(_(version))
        titles = parser.versions[version].keys()
        titles.sort()
        for title in titles:
            sheet(parser.versions[version][title] + '#' + _(title), _(title))
        titles = parser.extversions[version].keys()
        titles.sort()
        for title in titles:
            sheetE(parser.extversions[version][title] + '#' + _(title), _(title))
        f.write('</ul></li>\n')
    f.write('</ul></li>\n')

parser = Clonkparser()
reader = xml.sax.make_parser()
reader.setContentHandler(parser)
reader.setEntityResolver(ClonkEntityResolver())
for filename in sys.argv[1:]:
    reader.getEntityResolver().setrelpath(filename)
    parser.setfilename(filename)
    reader.parse(filename)

if 0:
    reader.setContentHandler(experimental.ExperimentParser())
    for filename in sys.argv[1:]:
        reader.getEntityResolver().setrelpath(filename)
        reader.parse(filename)
    experimental.Result()

mofile = open("de.mo", "rb")
gt = gettext.GNUTranslations(mofile)

#_ = lambda s: s.encode('iso-8859-1')
#for f, fin in ((file("chm/de/Output.hhc", "w"), file("Template.hhc", "r")),
#               (file("chm/en/Output.hhc", "w"), file("Template.en.hhc", "r"))):
#    for line in fin:
#        if line.find("<!-- Insert Functions here 1-->") != -1:
#            printcontents1(f, _)
#        elif line.find("<!-- Insert Functions here 2-->") != -1:
#            printcontents2(f, _)
#        else:
#            f.write(line)
#    f.close()
#    fin.close()
#    _ = lambda s: gt.ugettext(s).encode('iso-8859-1')

_ = lambda s: s.encode('utf-8')
f, fin = (file("sdk/content.xml", "w"), file("sdk/content.xml.in", "r"))
for line in fin:
    if line.find("<!-- Insert Functions here -->") != -1:
        printcontents3(f, _)
    else:
        f.write(line)
f.close()
fin.close()

for f, fin in ((file("chm/en/Output.hhp", "w"), file("Template.hhp", "r")),
               (file("chm/de/Output.hhp", "w"), file("Template.de.hhp", "r"))):
    for line in fin:
        if line.find("[INFOTYPES]") != -1:
            for filename in sys.argv[1:]:
                f.write(filename[:-3].replace("/", "\\") + 'html\r\n')
        f.write(line)
    f.close()
    fin.close()

_ = lambda s: s.encode('iso-8859-1')
for f, fin in ((file("chm/en/Output.hhk", "w"), file("Template.hhk", "r")),
               (file("chm/de/Output.hhk", "w"), file("Template.de.hhk", "r"))):
    for line in fin:
        if line.find("</UL>") != -1:
            for title, filename in parser.files.iteritems():
                f.write("  <LI> <OBJECT type=\"text/sitemap\">\n" +
                    "    <param name=\"Name\" value=\"" + _(title) + "\">\n" +
                    "    <param name=\"Local\" value=\"" + filename + "#" + _(title) + "\">\n" +
                    "    </OBJECT>\n")
        f.write(line)
    f.close()
    fin.close()
    _ = lambda s: gt.ugettext(s).encode('iso-8859-1')
