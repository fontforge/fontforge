The `askMulti` Dialog
=====================

When called in a UI-enabled environment the Python
:py:func:`fontforge.askMulti()` method raises a modal dialog that prompts the
user to answer one or more questions, potentially organized into "categories"
displayed in separate tabs. It takes two parameters, ``title`` and ``specification``. 

The ``title`` parameter should be a string that will be displayed in the dialog
window title bar.

The ``specification`` paramter is either a Python list or dictionary object
specifying the questions. This is explained below.

Questions
---------

There are four question types, corresponding to the single-question
:py:func:`fontforge.askString()`, :py:func:`fontforge.askChoices()`,
:py:func:`fontforge.openFilename()`, and :py:func:`fontforge.saveFilename()`
dialog methods. Each individual question is specified as a dictionary. 

All question types use the keys ``type``, ``question````, ``tag`` and ``align``. The
``type`` value specifies the type of question as outlined below. 

The value corresponding to ``question`` is the label string, which is displayed
before the answer field. This key must be present but the value can be ``None``
if no label is needed.

The ``tag`` is optional but either it or ``question`` must have a value. When
present its value will be used as the key corresponding to this question in the
result dictionary returned by the method. Otherwise the ``question`` value 
will be used. The ``tag`` value can be any "hashable" type—that is, any type
that can serve as a key in a Python dictionary.

The ``align`` value must be boolean-evaluable (ideally ``True`` or ``False``).
It indicates whether, when the dialog is being laid out, the label for this
question should be aligned with the other labels of the same category. When
absent the default is ``True``. (This has no effect when the ``question`` value
is ``None``, so if you want an aligned, empty label use a space as the 
``question`` value.)

String Questions
^^^^^^^^^^^^^^^^

A string question displays the label followed by a text entry field.  Its
``type`` value must be "string".

The only additional key for a string question is ``default``, which is optional.
When the key present its value, which must be a string, will be the initial
value of the answer entry field. An example dictionary is::

    { 'type': 'string', 'question': 'Number of contours:',
       'tag': 3, 'default': '4' }

Choice Questions
^^^^^^^^^^^^^^^^

Choice questions ask the user to pick a subset of given answers. Its ``type``
value must be "choice". This question type has three additional dictionary
keys: ``answers``, ``multiple``, and ``checks``.

The ``answers`` value must be a Python list of potential answers, each of which is
its own small dictionary. The keys to an answer dictionary are ``name``, ``tag``,
and ``default``. The value of ``name`` must be the string the user will choose among.
The value of ``tag``, when present, will be used to report the answer in the
result dictionary—when not present the ``name`` string will be used. Unlike the
question ``tag`` an answer ``tag`` value can be of any type whatsoever. The ``default``
key is an optional key that must have a boolean-evaluable value. When ``True``
the answer is selected when the dialog is presented. 

The ``multiple`` value must be boolean-evaluable; when the key is absent the
default is ``False``. It indicates whether the user must choose exactly one
answer or can choose multiple or, potentially, no answers. When ``multiple`` is
false there must be at most one answer for which ``default`` is ``True``. 

The ``checks`` value must also be boolean-evaluable and its default is also
``False``. By default a choice question is presented as in the
:py:func:`fontforge.askChoices()` dialog, with a (potentially) scrolling
vertical list similar to pop-up menu. When ``checks`` is ``True`` and ``multiple``
is ``False`` the answers are presented as radio buttons. When ``checks`` is ``True``
and ``multiple`` is ``True`` the answers are presented as checkboxes. Radio
buttons and checkboxes allow better keyboard-based selection (see the 
mnemonics section) and may have other advantages. 

An example choice question dictionary is: ::

    { 'type': 'choice', 'question': 'What direction:',
      'tag': 'direction', 'checks': True,
      'answers': [ { 'name': 'Clockwise', 'tag': 'cw', 'default': True },
                   { 'name': 'Counter-Clockwise', 'tag': 'ccw' } ]
    }

Pathname Questions
^^^^^^^^^^^^^^^^^^

The open pathname and save pathname questions have the respective type values
"openpath" and "savepath". These differ from the
:py:func:`fontforge.openFilename()` and :py:func:`fontforge.saveFilename()` 
methods in that they do not raise a file browser directly. Instead they display
an entry field, similar to a string question, followed by a button to raise a
browser, similar to the path-specifying entries in :doc:``the preferences dialog
</ui/dialogs/prefs>``. Each also has an optional ``default`` key for an initial
pathname value, and an optional ``filter`` key to specify a file filter. Both
must be strings. An example dictionary is: ::

    { 'type': 'openpath', 'question': 'Glif file:', 'tag': 'file',
      'default': 'input.glif', 'filter': '*.glif' }

Categories
----------

Questions are organized into categories. A category is a dictionary with 
two keys: ``category`` and ``questions``. The value of ``category`` should be a
either string to display to the user or ``None`` if no label is needed. 
The value of ``questions`` must be a list of question dictionaries, as 
described above. A category containing the dictionaries above looks like:

.. figure:: /images/multi_example.png
   :alt: Example ``askMulti`` dialog

The Specification
-----------------

The specification passed as the ``spec`` parameter can be any of:

* A question dictionary
* A list of question dictionaries
* A category dictionary
* A list of category dictionaries

In the last case each category will be given its own tab, with the 
category names selectable on the left side of the dialog. (Whenever
there is only one category there is no list, even when the category
has a name.)

The Return Value
----------------

When the user cancels the dialog, either with the "Cancel" button or
by closing the window, the ``askMulti`` method returns None. Otherwise
it returns a Python dictionary of results, one for each question. The
key will be the ``tag`` value or, if none was present, the ``question``
string. For all question types other than "choice" the value is a 
string. For a choice question where ``multiple`` is ``False`` the value
is the ``tag`` or ``name`` of the answer. For a ``multiple`` choice question
the value is a tuple of ``tag`` or ``name`` values. The tuple may be 
empty. 

A result dictionary for the collection above could be: ::

    {3: '20', 'direction': 'ccw', 'file': '/tmp/in.glif'}

When the specification is invalid the method will throw a ``TypeError``
or ``ValueError`` exception with information about the problem.
