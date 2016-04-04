#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import sys
import xml.sax
from xml.sax.saxutils import escape, quoteattr
import experimental

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
        self.funcs = { }
        self.files = { }
    def setfilename(self, filename):
        self.filename = filename
        self.htmlfilename = filename[:-3] + 'html'
    def _setcurcat(self, curcat):
        self.curcat = curcat
        if self.curcat not in self.cats:
            self.cats[self.curcat] = { }
            self.subcats[self.curcat] = { }
    def _addToIndex(self, title, href):
        if not title in self.files:
            self.files[title] = { self.title: href }
        else:
            if self.title in self.files[title]:
                print "WARNING: duplicate " + title + " in " + href + " and " + self.files[title][self.title]
            self.files[title][self.title] = href
    def startDocument(self):
        self.cur = ""
        self.curcat = ""
        self.title = ""
        self.func = 0
        self.state = None
        self.id = None
        self.idTitle = None
        self.idStackdepth = 0
        self.Stackdepth = 0
    def startElement(self, name, attr):
        # subcat inside category?
        if self.state == 'category' and self.cur != "":
            self._setcurcat(self.cur)
        # is func
        if name == 'funcs':
            self.func = 1
        self.cur = unicode("")
        self.state = name
        self.Stackdepth = self.Stackdepth + 1
        if 'id' in attr:
            self.id = attr["id"]
            self.idTitle = unicode("")
            self.idStackdepth = self.Stackdepth
    def characters(self, content):
        if self.state in ['category', 'subcat', 'version', 'extversion', 'title', 'funcs']:
            self.cur += content
        if self.id:
            self.idTitle += content
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
        elif name == 'funcs':
            self.func = 0
        elif name == 'title':
            self.title = self.cur
            if self.func == 1:
                self._addToIndex(self.title, self.htmlfilename + '#' + self.title.encode('utf-8'))
                self.funcs[self.title] = self.htmlfilename
            else:
                self._addToIndex(self.title, self.htmlfilename)

        self.Stackdepth = self.Stackdepth - 1
        if self.id and (self.idStackdepth > self.Stackdepth or name == 'col' or name == 'literal_col'):
            title = self.idTitle.strip()
            href = self.htmlfilename + '#' + self.id.encode('utf-8')
            if title == "":
                print "WARNING: id " + self.id.encode('utf-8') + " without text content in " + self.filename
            else:
                self._addToIndex(title, href)
            self.id = None
            self.idTitle = None
        self.cur = ""
        self.state = None

def printfunctions(f, _):
    def folder(name):
        f.write("<li>" + escape(name) + "\n<ul>\n")
    def sheet(url, name):
        f.write("<li><emlink href=" + quoteattr(url[4:]) + ">" + escape(name) + "</emlink></li>\n")
    def sheetE(url, name):
        f.write("<li><emlink href=" + quoteattr(url[4:]) + ">" + escape(name) + "</emlink> (extended)</li>\n")
    folder("Functions by Category")
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

def printindex(f, _):
    def folder(name):
        f.write("<li class='index'>" + escape(name) + "\n<ul>\n")
    def sheet(url, name):
        f.write("<li><emlink href=" + quoteattr(url[4:]) + ">" + escape(name) + "</emlink></li>\n")
    folder("Index")
    titles = parser.files.keys()
    titles.sort(key=unicode.lower)
    for title in titles:
        ctitles = parser.files[title].keys()
        ctitles.sort(key=unicode.lower)
        if len(ctitles) == 1:
            sheet(parser.files[title][ctitles[0]], _(title))
        else:
            for ctitle in ctitles:
                sheet(parser.files[title][ctitle], _(title + " (" + ctitle + ")"))
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

_ = lambda s: s.encode('utf-8')
f, fin = (file("sdk/content.xml", "w"), file("sdk/content.xml.in", "r"))
for line in fin:
    if line.find("<!-- Insert Functions here -->") != -1:
        printfunctions(f, _)
    elif line.find("<!-- Insert Index here -->") != -1:
        printindex(f, _)
    else:
        f.write(line)
f.close()
fin.close()
