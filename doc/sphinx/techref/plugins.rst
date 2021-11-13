Using FontForge Plugins
=======================

A FontForge plugin is a Python package that contains code to enhance
the FontForge application in some way, most often by adding one or more
entries to the Font View or Char View :doc:`Tools menu </ui/menus/toolsmenu>`.
Using a plugin is as easy as installing the package (often with
`pip <https:pypi.org/project/pip/>`_) and, when necessary, "configuring"
the plugin. Almost any plugin of general interest will be listed in
the `PyPI package database <https:pypi.org/search/?q=%22FontForge_plugin%22>`_.

Plugin Preferences
------------------

Two FontForge preference values affect the behavior of plugins. The first is
"UsePlugins" in the "Preferences ..." dialog (under :doc:`the File menu
</ui/menus/filemenu>`), which is On by default. Turning this Off, temporarily
or permanently, will keep FontForge from attempting to discover and load
plugins.

.. figure:: /images/plugdlg.png
   :alt: Configure Plugins... Dialog

The second preference is "PluginStartupMode" in the "Configure Plugins..."
dialog (also under the File menu), which can be set to "On", "Off", or
"Ask" (the default). When this preference is set to "On" a newly discovered
plugins is automatically loaded. When set to "Off" it is not loaded
until you decide to Enable it in the same dialog. When set to "Ask" the
dialog will appear on startup whenever a new plugin is discovered so that
you can decide whether or not to use it. Which value makes the most sense for
you will depend on the degree of control you have over what packages are
installed in your python path.

Enabling and Disabling Plugins
------------------------------

The "Configure Plugins..." dialog allows you to Enable or Disable individual
plugins and also Delete entries for once-discovered plugins that are no
longer found. You can also "reorder" plugins—changing the load order and
therefore the order of Tools menu entries. There are also buttons for
loading a so-far-unloaded plugin (in case you do not want to wait for the
next application restart), getting more (mostly technical) information
about the plugin, and opening the web page associated with the plugin
package.

Finally, if a plugin has its own preferences you can set those with the
"Configure" button (which is disabled otherwise). The content of preferences
and the process of setting them will vary by plugin.

Loading Plugins For Scripts
---------------------------

When a script is run with FontForge directly using the ``-script`` flag each
enabled and discovered plugin will be loaded before control is passed off to
the script (unless the ``-skippyplug`` flag is also specified).
In contrast, running a python script with the ``python`` command and importing
the ``fontforge`` module will not cause plugins to be loaded. If you have a
script that needs the functionality of a plugin (perhaps because it adds a
new font export format) you must load the plugins explicitly by calling
``fontforge.loadPlugins()``. Calling this function when plugins are already
loaded should be harmless.

Troubleshooting
---------------

The FontForge project has no control over who can make a plugin. At
some point you may install one that causes problems, potentially even
preventing you from successfully running FontForge. One work-around for
that situation is to run FontForge with the ``-skippyplug`` flag.
When started with that flag FontForge will still "discover" plugins but
will not automatically load them. You can then open the "Configure Plugins..."
dialog, Disable the misbehaving plugin, and restart the program.

When an individual plugin is present but cannot be loaded there will be
relevant warnings logged on startup or when a load is attempted via the dialog
or the python API. There will also be a warning for each *enabled* plugin
that is not discovered.
