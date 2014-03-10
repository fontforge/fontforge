FontCompare
===========

To implement a tool for improving the aesthetic quality of fonts of Indic scripts

Project Abstract
----------------

There are a lot of fonts for Roman scripts but when it comes to fonts for Indic scripts like Devanagari, Bengali, Telugu, there is a dearth of good quality Open Type fonts. This tool would facilitate in providing tests enabling Font developers to contribute more to Open Type Indic fonts.

This project is part of Google Summer of Code 2013.

Student Name: Mayank Jha

Mentor: Shreyank Gupta

Melange URL: http://www.google-melange.com/gsoc/project/google/gsoc2013/mjnovice/42001

How to Install and Run the application: 

A) Installation

    1. Change into the folder containing the source. 

    2. As this is a fontforge utility so to be able to use it you need to
		install the fontforge bindings for python onto your system.
		
		Run the required command on the terminal as given below
		
		For Debian based Linux distros:

			sudo apt-get install python-fontforge
		
		For Redhat based Linux distros:

			yum install fontforge-python

	You also need to install PyQt4 separately, as currently the setuptools
	does not support automatic detection and installation of Python bindings
	for Qt.

	The commands for the installation:
		
               For Debian based Linux distros:

                        sudo apt-get install python-qt4

                For Redhat based Linux distros:

                        yum install pyqt4

    3. Run "sudo python setup.py install" from the shell prompt, supply the 
        password if prompted for.

B) Running
    From the shell prompt, type in "fontcompare" and you would be able to
    run and use the application. 

C) How to Use:
    The very basic feature of Font Compare is to compare two fonts based 
    on their glyphs, bearing, size and many other factors.
    
    Basic Usage Steps:
    i) Click on "Load Test font" button , which would load the font to be
       tested.

    ii) Now click on the "Begin Test!" button to run the various tests.
    
    You can see that the detailed glyph wise test results can be viewed 
    in the lower right corner, whereas categorised scored based on the
    degree of similarity can be seen graphically in the upper part of the 
    GUI.

    This branch of FontCompare compares the fonts, with a preset values
    of standards and bitmaps, implemented using the concept of mockfonts.

    NOTE:: Support for the user to be able to generate these mockfonts as 
    standards at will, would be added soon. The reason I chose to create
    mockfonts, is that it would prevent the need for the user to download 
    standard fonts separately, as it was used to be done in the master 
    branch, although it does occupy a bit more memory but works smoother.
