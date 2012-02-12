# -*- coding: iso-8859-15 -*-
import xml.sax
#def htmlescape(str):
#    s = list(str)
#    for i in range(len(s)):
#        c = ord(s[i])
#        if c > 127:
#            s[i] = '&#%(#)d;' % {"#": c}
#    return "".join(s)

codeparents = {}
allcode = {}
c4scripter = None
class ExperimentParser(xml.sax.handler.ContentHandler):
    def __init__(self):
        global c4scripter
        self.statestack = []
        self.parameter = ""
        self.type = ""
        self.rtype = ""
        self.name = ""
        self.title = ""
        self.cur = ""
        self.desc = ""
        c4scripter = file("Functions.txt", 'w')
    def startElement(self, name, attr):
        self.statestack.append(name)
        if name == 'params':
            self.parameter = ''
        if 0 and name == 'img':
          for a in attr.getNames():
            if a in codeparents:
                codeparents[a] += 1
            else:
                codeparents[a] = 1
        if 1 and len(self.statestack) > 1 and self.statestack[-2] == 'dd':
            if name in codeparents:
                codeparents[name] += 1
            else:
                codeparents[name] = 1
        if False and name == 'text':
            if self.statestack[-2] in codeparents:
                codeparents[self.statestack[-2]] += 1
            else:
                codeparents[self.statestack[-2]] = 1
    def endElement(self, name):
        self.statestack = self.statestack[:-1]
        self.cur = self.cur.strip()
        if name == 'type':
            self.type = self.cur
        elif name == 'name':
            self.name = self.cur
        elif name == 'title':
            self.title = self.cur
        elif name == 'rtype':
            self.rtype = self.cur
        elif name == 'desc':
            self.desc = self.cur
        elif name == 'func':
            c4scripter.write("""[Function]
Name=%s
Return=%s
Parameter=%s
DescDE=%s

"""
                % (self.title, self.rtype, self.parameter, self.desc))
        elif name == 'param':
            if self.parameter != '':
                self.parameter += ', '
            self.parameter += self.type + ' ' + self.name
        self.cur = ''

    def characters(self, content):
        self.cur += content.encode('iso-8859-1')
        if self.statestack[-1] == 'code':
            for a in content:
                allcode[a] = a

def Result():
    print codeparents
    a = allcode.keys()
    a.sort()
    print '"' + ''.join(a) + '"'
    c4scripter.close()