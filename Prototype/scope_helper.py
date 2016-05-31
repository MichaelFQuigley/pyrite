from types_util import *

class ScopeNode:
    def __init__(self, parent=None, namedVals = None, isFunctionScope=False):
        self.namedVals = {} if namedVals is None else namedVals
        self.parent    = parent
        self.isFunctionScope = isFunctionScope

class ScopeHelper:
    def __init__(self):
        self.parentScope = ScopeNode()
        self.currScope   = self.parentScope
        self.popCallback = None

    def pushScope(self, isFunctionScope = False):
        scopeNode        = ScopeNode(isFunctionScope=isFunctionScope)
        scopeNode.parent = self.currScope
        self.currScope   = scopeNode

    def setNamedVal(self, name, val):
        assert name not in self.currScope.namedVals, str(name) + " is already declared."
        self.currScope.namedVals[name] = val

    def getNamedVal(self, name, walkScopes = False):
        if walkScopes:
            tempScope = self.currScope
            while (tempScope is not None):
                if name in tempScope.namedVals:
                    return tempScope.namedVals[name]
                tempScope = tempScope.parent
            return None
        else:
            return (None 
                    if name not in self.currScope.namedVals
                    else self.currScope.namedVals[name])

    #gets all named values
    #returns list of tuples [(name, val), ...]
    def getNamedVals(self):
        named_vals = []
        tempScope = self.currScope
        while (tempScope is not None):
            named_vals += [(name, tempScope.namedVals[name]) for name in tempScope.namedVals]
            tempScope = tempScope.parent
        return named_vals

    #registers a callback function to be called after scope is popped
    def registerPopCallback(self, callback):
        self.popCallback = callback

    #pops scope as well as calling a callback with the provided args if a callback is available
    def popScope(self, *args):
        self.currScope = self.currScope.parent
        if self.popCallback is not None:
            self.popCallback(*args)


