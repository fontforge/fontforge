'''
Copyright (C) 2020 by Jeremy Tan

Distributed under the original FontForge BSD 3-clause license.
Based on the docutils table code, which is released into the public domain.

-------------------------------------------------------------------------------

Adds (HTML only) support for a flex-grid directive that allows writing
out elements in a generic grid fashion.

Example:

.. flex-grid::
  :class: custom_class

  * - Row 1 Col 1
    - Row 1 Col 2
    - Row 1 col 3
  * :flex-widths: 1 2
    - Row 2 Col 1
    - Row 2 Col 2

The syntax is very similar to the docutils list-table directive. However the
number of columns per row does *not* need to be homogeneous.

The flex-widths parameter is specified on a per-row basis. It should be a list
of numbers whose length is equal to the number of columns in that row. It
represents the ratio of space taken by that column. For example, with two
columns and a flex-widths of '1 2', the proportion of space taken is 1:2
between the first and second columns. A value of 0 disables flexing that
column.
'''

from docutils import nodes, statemachine
from docutils.utils import SystemMessagePropagation
from docutils.parsers.rst import Directive
from docutils.parsers.rst import directives


class flex_grid(nodes.General, nodes.Element): pass
class flex_row(nodes.Part, nodes.Element): pass
class flex_col(nodes.Part, nodes.Element): pass


class FlexGrid(Directive):

    final_argument_whitespace = True
    option_spec = {'class': directives.class_option, 'name': directives.unchanged}
    has_content = True
    valid_flexes = set((0, 1, 2, 3, 4, 5))

    def run(self):
        if not self.content:
            error = self.state_machine.reporter.error(
                'The "%s" directive is empty; content required.' % self.name,
                nodes.literal_block(self.block_text, self.block_text),
                line=self.lineno)
            return [error]

        node = nodes.Element()          # anonymous container for parsing
        self.state.nested_parse(self.content, self.content_offset, node)

        try:
            grid_data = self.get_grid(node)
        except SystemMessagePropagation as detail:
            return [detail.args[0]]

        grid_node = self.build_grid_node(grid_data)
        return [grid_node]

    def get_grid(self, node):
        if len(node) != 1 or not isinstance(node[0], nodes.bullet_list):
            error = self.state_machine.reporter.error(
                'Error parsing content block for the "%s" directive: '
                'exactly one bullet list expected.' % self.name,
                nodes.literal_block(self.block_text, self.block_text),
                line=self.lineno)
            raise SystemMessagePropagation(error)

        grid = []
        list_node = node[0]

        for item_index in range(len(list_node)):
            item = list_node[item_index]
            nitems = len(item)
            if not nitems or nitems > 2 or not isinstance(item[-1], nodes.bullet_list):
                error = self.state_machine.reporter.error(
                    'Error parsing content block for the "%s" directive: '
                    'two-level bullet list expected, but row %s does not '
                    'contain a second-level bullet list.'
                    % (self.name, item_index + 1), nodes.literal_block(
                        self.block_text, self.block_text), line=self.lineno)
                raise SystemMessagePropagation(error)
            elif nitems == 2 and not isinstance(item[0], nodes.field_list):
                error = self.state_machine.reporter.error(
                    'Error parsing content block for the "%s" directive: '
                    'expected options as a field list, but row %s does not '
                    'start with a field list.'
                    % (self.name, item_index + 1), nodes.literal_block(
                        self.block_text, self.block_text), line=self.lineno)
                raise SystemMessagePropagation(error)

            if nitems == 1:
                row = ({}, [col.children for col in item[0]])
            else:
                opts = {}
                for opt in item[0]:
                    key = opt[0].rawsource
                    value = opt[1].rawsource.strip()

                    if key != 'flex-widths':
                        error = self.state_machine.reporter.error(
                            'Error parsing content block for the "%s" directive: '
                            'received "%s" but only "flex-widths" is supported as '
                            'an option on row %s.'
                            % (self.name, key, item_index + 1), nodes.literal_block(
                                self.block_text, self.block_text), line=self.lineno)
                        raise SystemMessagePropagation(error)

                    try:
                        sep = ',' if ',' in value else ' '
                        fwidths = [int(x) for x in value.split(sep)]
                        if any(x not in self.valid_flexes for x in fwidths) or len(fwidths) != len(item[1]):
                            raise ValueError('invalid flex widths')
                        opts['flex-widths'] = fwidths
                    except:
                        error = self.state_machine.reporter.error(
                            'Error parsing content block for the "%s" directive: '
                            '"flex-widths" expects a list of %s widths with possible values of (1,2,3,4,5).'
                            'The input "%s" for row %s is malformed.'
                            % (self.name, len(item[1]), value, item_index + 1), nodes.literal_block(
                                self.block_text, self.block_text), line=self.lineno)
                        raise SystemMessagePropagation(error)
                row = (opts, [col.children for col in item[1]])
            grid.append(row)
        return grid

    def build_grid_node(self, grid_data):
        grid = flex_grid()
        self.add_name(grid)
        grid['classes'] += ['flex-container']
        grid['classes'] += self.options.get('class', [])
        for row_opts, row in grid_data:
            row_node = flex_row()
            row_node['classes'] += ['flex-row']
            widths = row_opts.get('flex-widths')

            for i, cell in enumerate(row):
                entry = flex_col()
                entry['classes'] += ['flex-col']
                if widths:
                    width = widths[i]
                    if width == 0:
                        entry['classes'] += ['flex-none']
                    elif width != 1:
                        entry['classes'] += ['flex-{}'.format(width)]
                entry += cell
                row_node += entry
            grid += row_node
        return grid


def visit_flex_node(self, node):
    self.body.append(self.starttag(node, 'div'))

def depart_flex_node(self, node):
    self.body.append('</div>\n')


def setup(app):
    app.add_directive('flex-grid', FlexGrid)
    app.add_node(flex_grid, html=(visit_flex_node, depart_flex_node))
    app.add_node(flex_row, html=(visit_flex_node, depart_flex_node))
    app.add_node(flex_col, html=(visit_flex_node, depart_flex_node))

    return {
        'version': '1.0.0',
        'env_version': 1,
        'parallel_read_safe': True
    }
