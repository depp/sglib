# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.

def yesno(value):
    """Convert a boolean value to 'yes' or 'no'."""
    if value:
        return 'yes'
    return 'no'

def indent_xml(root, *, indent=2):
    """Add indentation to an XML tree."""
    def pretty(element, level):
        if element.text or not len(element):
            return
        last = None
        istr = '\n' + ' ' * (level + indent)
        for child in element:
            pretty(child, level + indent)
            if last is None:
                element.text = istr
            else:
                last.tail = istr
            last = child
        last.tail = '\n' + ' ' * level
    pretty(root, 0)
