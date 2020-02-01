'''
Copyright (C) 2020 by Jeremy Tan

Distributed under the original FontForge BSD 3-clause license.

Based on the Sphinx Python domain implementation, which is also BSD licensed:
https://github.com/sphinx-doc/sphinx/blob/14e69c21ed3552afc54b8f75377317765938bb60/sphinx/domains/python.py

-------------------------------------------------------------------------------

Adds a FontForge scripting domain for the native FontForge scripting language.
'''

from docutils.parsers.rst import directives
from docutils.nodes import Element

from sphinx import addnodes
from sphinx.addnodes import pending_xref, desc_signature
from sphinx.builders import Builder
from sphinx.directives import ObjectDescription
from sphinx.domains import Domain, ObjType
from sphinx.environment import BuildEnvironment
from sphinx.locale import _, __
from sphinx.roles import XRefRole
from sphinx.util import logging
from sphinx.util.nodes import make_refnode

from typing import Any, Dict, Iterator, List, Tuple
from typing import cast

import re

ff_sig_re = re.compile(r'^\$?(\w+)(?:\(\s*(.*)\s*\))?$')

logger = logging.getLogger(__name__)


# Lifted straight from Sphinx
def _pseudo_parse_arglist(signode: desc_signature, arglist: str) -> None:
    """"Parse" a list of arguments separated by commas.
    Arguments can have "optional" annotations given by enclosing them in
    brackets.  Currently, this will split at any comma, even if it's inside a
    string literal (e.g. default argument value).
    """
    paramlist = addnodes.desc_parameterlist()
    stack = [paramlist]  # type: List[Element]
    try:
        for argument in arglist.split(','):
            argument = argument.strip()
            ends_open = ends_close = 0
            while argument.startswith('['):
                stack.append(addnodes.desc_optional())
                stack[-2] += stack[-1]
                argument = argument[1:].strip()
            while argument.startswith(']'):
                stack.pop()
                argument = argument[1:].strip()
            while argument.endswith(']') and not argument.endswith('[]'):
                ends_close += 1
                argument = argument[:-1].strip()
            while argument.endswith('['):
                ends_open += 1
                argument = argument[:-1].strip()
            if argument:
                stack[-1] += addnodes.desc_parameter(argument, argument)
            while ends_open:
                stack.append(addnodes.desc_optional())
                stack[-2] += stack[-1]
                ends_open -= 1
            while ends_close:
                stack.pop()
                ends_close -= 1
        if len(stack) != 1:
            raise IndexError
    except IndexError:
        # if there are too few or too many elements on the stack, just give up
        # and treat the whole argument list as one argument, discarding the
        # already partially populated paramlist node
        paramlist = addnodes.desc_parameterlist()
        paramlist += addnodes.desc_parameter(arglist, arglist)
        signode += paramlist
    else:
        signode += paramlist


class FontForgeScriptingObject(ObjectDescription):
    """
    Description of a general FontForge native scripting object.
    """
    option_spec = {
        'noindex': directives.flag,
        'annotation': directives.unchanged,
    }

    def get_signature_prefix(self, sig: str) -> str:
        """May return a prefix to put before the object name in the
        signature.
        """
        return ''

    def needs_arglist(self) -> bool:
        """May return true if an empty argument list is to be generated even if
        the document contains none.
        """
        return False

    def handle_signature(self, sig: str, signode: desc_signature) -> Tuple[str, str]:
        """Transform a FontForge native scripting signature into RST nodes.
        Return (fully qualified name of the thing, classname if any).
        If inside a class, the current class name is handled intelligently:
        * it is stripped from the displayed name if present
        * it is added to the full name (return value) if not present
        """
        m = ff_sig_re.match(sig)
        if m is None:
            raise ValueError

        name, arglist = m.groups()

        signode['fullname'] = name

        sig_prefix = self.get_signature_prefix(sig)
        if sig_prefix:
            signode += addnodes.desc_annotation(sig_prefix, sig_prefix)

        signode += addnodes.desc_name(name, name)

        if arglist:
            _pseudo_parse_arglist(signode, arglist)
        elif self.needs_arglist():
            signode += addnodes.desc_parameterlist()

        anno = self.options.get('annotation')
        if anno:
            signode += addnodes.desc_annotation(' ' + anno, ' ' + anno)

        return name, ''

    def get_index_text(self, name: Tuple[str, str]) -> str:
        """Return the text for the index entry of the object."""
        raise NotImplementedError('must be implemented in subclasses')

    def add_target_and_index(self, name_cls: Tuple[str, str], sig: str,
                             signode: desc_signature) -> None:
        fullname = name_cls[0]
        # note target
        if fullname not in self.state.document.ids:
            signode['names'].append(fullname)
            signode['ids'].append(fullname)
            signode['first'] = (not self.names)
            self.state.document.note_explicit_target(signode)

            domain = cast(FontForgeScriptingDomain, self.env.get_domain('ff'))
            domain.note_object(fullname, self.objtype,
                               location=(self.env.docname, self.lineno))

        indextext = self.get_index_text(name_cls)
        if indextext:
            self.indexnode['entries'].append(('single', indextext,
                                              fullname, '', None))


class FontForgeScriptingFunction(FontForgeScriptingObject):
    def needs_arglist(self) -> bool:
        return True

    def get_index_text(self, name_cls: Tuple[str, str]) -> str:
        return _('%s() (native scripting function)') % name_cls[0]


class FontForgeScriptingVariable(FontForgeScriptingObject):
    def get_signature_prefix(self, sig):
        return '$'

    def get_index_text(self, name_cls: Tuple[str, str]) -> str:
        return _('$%s (native scripting built-in variable)') % name_cls[0]


class FontForgeScriptingXRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> Tuple[str, str]:
        if not has_explicit_title:
            title = title.lstrip('.')    # only has a meaning for the target
            target = target.lstrip('~')  # only has a meaning for the title
            # if the first character is a tilde, don't display the module/class
            # parts of the contents
            if title[0:1] == '~':
                title = title[1:]
                dot = title.rfind('.')
                if dot != -1:
                    title = title[dot + 1:]
        # if the first character is a dot, search more specific namespaces first
        # else search builtins first
        if target[0:1] == '.':
            target = target[1:]
            refnode['refspecific'] = True
        return title, target


class FontForgeScriptingDomain(Domain):
    name = 'ff'
    label = 'FontForge Native Scripting'
    object_types = {
        'function': ObjType(_('function'), 'func', 'obj'),
        'data': ObjType(_('data'), 'data', 'obj'),
    }

    directives = {
        'function': FontForgeScriptingFunction,
        'data': FontForgeScriptingVariable
    }

    roles = {
        'func': FontForgeScriptingXRefRole(),
        'data': FontForgeScriptingXRefRole()
    }
    initial_data = {
        'objects': {},  # fullname -> docname, objtype
    }  # type: Dict[str, Dict[str, Tuple[Any]]]

    @property
    def objects(self) -> Dict[str, Tuple[str, str]]:
        # fullname -> docname, objtype
        return self.data.setdefault('objects', {})

    def note_object(self, name: str, objtype: str, location: Any = None) -> None:
        """Note a FontForge native scripting object for cross reference.
        """
        if name in self.objects:
            docname = self.objects[name][0]
            logger.warning(__('duplicate object description of %s, '
                              'other instance in %s, use :noindex: for one of them'),
                           name, docname, location=location)
        self.objects[name] = (self.env.docname, objtype)

    def clear_doc(self, docname: str) -> None:
        for fullname, (fn, _l) in list(self.objects.items()):
            if fn == docname:
                del self.objects[fullname]

    def merge_domaindata(self, docnames: List[str], otherdata: Dict) -> None:
        # XXX check duplicates?
        for fullname, (fn, objtype) in otherdata['objects'].items():
            if fn in docnames:
                self.objects[fullname] = (fn, objtype)

    def find_obj(self, env: BuildEnvironment,
                 name: str, type: str, searchmode: int = 0) -> List[Tuple[str, Any]]:
        """Find a Fontforge native scripting object for "name".
        Returns a list of (name, object entry) tuples.
        """
        # skip parens
        if name[-2:] == '()':
            name = name[:-2]

        if not name:
            return []

        matches = []  # type: List[Tuple[str, Any]]

        newname = None
        if searchmode == 1:
            if type is None:
                objtypes = list(self.object_types)
            else:
                objtypes = self.objtypes_for_role(type)
            if objtypes is not None:
                if not newname:
                    if name in self.objects and self.objects[name][1] in objtypes:
                        newname = name
                    else:
                        # "fuzzy" searching mode
                        searchname = '.' + name
                        matches = [(oname, self.objects[oname]) for oname in self.objects
                                   if oname.endswith(searchname) and
                                   self.objects[oname][1] in objtypes]
        else:
            # NOTE: searching for exact match, object type is not considered
            if name in self.objects:
                newname = name
        if newname is not None:
            matches.append((newname, self.objects[newname]))
        return matches

    def resolve_xref(self, env: BuildEnvironment, fromdocname: str, builder: Builder,
                     type: str, target: str, node: pending_xref, contnode: Element
                     ) -> Element:
        searchmode = 1 if node.hasattr('refspecific') else 0
        matches = self.find_obj(env, target,
                                type, searchmode)

        if not matches:
            return None
        elif len(matches) > 1:
            logger.warning(__('more than one target found for cross-reference %r: %s'),
                           target, ', '.join(match[0] for match in matches),
                           type='ref', subtype='ff', location=node)

        name, obj = matches[0]
        return make_refnode(builder, fromdocname, obj[0], name, contnode, name)

    def resolve_any_xref(self, env: BuildEnvironment, fromdocname: str, builder: Builder,
                         target: str, node: pending_xref, contnode: Element
                         ) -> List[Tuple[str, Element]]:
        results = []  # type: List[Tuple[str, Element]]

        # always search in "refspecific" mode with the :any: role
        matches = self.find_obj(env, target, None, 1)
        for name, obj in matches:
            results.append(('ff:' + self.role_for_objtype(obj[1]),
                            make_refnode(builder, fromdocname, obj[0], name,
                                         contnode, name)))
        return results

    def get_objects(self) -> Iterator[Tuple[str, str, str, str, str, int]]:
        for refname, (docname, type) in self.objects.items():
            yield (refname, refname, type, docname, refname, 1)

    def get_full_qualified_name(self, node: Element) -> str:
        target = node.get('reftarget')
        if target is None:
            return None
        else:
            return target


def setup(app):
    app.add_domain(FontForgeScriptingDomain)

    return {
        'version': '1.0.0',
        'env_version': 1,
        'parallel_read_safe': True
    }
