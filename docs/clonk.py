# the description of the format used for the clonk developer docs for xml2po
class clonkXmlMode:
    """Clonks propietary xml format"""
    def getIgnoredTags(self):
        "Returns array of tags to be ignored."
        return ['emlink', 'em', 'strong', 'code']
    def getFinalTags(self):
        "Returns array of tags to be considered 'final'."
        return ['table', 'text', 'desc', 'remark', 'col', 'li', 'funclink', 'ul', 'h', 'dt']
    def getSwallowTags(self):
        "Return array of tags which content is not translated."
        return self._stuff
    def getSpacePreserveTags(self):
        "Returns array of tags in which spaces are to be preserved."
        return ['code']

    _stuff = ['funclink', 'version', 'extversion', 'rtype', 'author', 'date',
        'type', 'code', 'code/i', 'code/b', 'name', 'func/title', 'const/title', 'literal_col']
    def _delete_stuff(self, node, msg):
        #print "looking at " + str(node.name)
        if node and node.children:
            child = node.children
            while child:
                n = child
                child = child.next
                if n.type=='element' and n.name in self._stuff:
                    #print "!"
                    if n.name == 'name' and node.name != 'param':
                        print node + "!" + n
                    #n.unlinkNode()
                    #n.freeNode()
                    n.setContent('')
                else:
                    self._delete_stuff(n, msg)


    def preProcessXml(self, doc, msg):
        """Add additional messages of interest here."""
        return
        #root = doc.getRootElement()
        #self._delete_stuff(root,msg)

    def postProcessXmlTranslation(self, doc, language, translators):
        """Sets a language and translators in "doc" tree.
        
        "translators" is a string consisted of translator credits.
        "language" is a simple string.
        "doc" is a libxml2.xmlDoc instance."""
        root = doc.getRootElement()
        root.setProp('xml:lang', language)

    def getStringForTranslators(self):
        """Returns None or a string to be added to PO files.

        Common example is 'translator-credits'."""
        return None

    def getCommentForTranslators(self):
        """Returns a comment to be added next to string for crediting translators.

        It should explain the format of the string provided by getStringForTranslators()."""
        return None
