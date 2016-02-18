/*

This program, when compiled and run, will find glyphs and create cursored variants, and (not done) write an .fea lookup. Supply the glyph encoding range.

*/

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ostream>
#include <math.h>
#include <string>
#include <cstring>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

using namespace std;

// GLYPHS YOU WANT TO CONVERT

string FileName = "FONTNAME";

string suffix = ".smcp";

string tagname = "sc";

string spacedtagname = "s c";

string featurename = "SmallCaps";

//

string line;
string encoding;
vector<string> encodings;

vector<int> enc1;
vector<int> enc2;
vector<int> enc3;

vector<int> benc1;
vector<int> benc2;
vector<int> benc3;

string BaseEncoding;
vector<string> BaseEncodings;

string CursorEncoding;
vector<string> CursorEncodings;

string Width;
string VWidth;
string Flags;
string ReferEncoding;
vector<string> ReferEncodings;

vector<string> baseglyphs = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "space", "comma", "period", "colon", "semicolon", "hyphen", "parenleft", "parenright", "exclam", "question", "quotesingle", "quotedbl", "endash", "emdash", "quoteleft", "quoteright", "quotedblright", "quotedblleft", "quotedblbase", "ellipsis", "ampersand"};
string glyphname;
string writeglyphname;

int i = 0;


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

string parameter ( string param ) {
	string value;
	if (line.substr(0, param.length()) == param) { value = line.substr(param.length(),9999); }
	return value;
}

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

int main () 
{

fstream fontfile;
fontfile.open (FileName);

// FINDS OUT WHERE TO INSERT NEW GLYPHS
    while ( getline (fontfile,line) )
    {

	if (line.substr(0,10) == "Encoding: " && ( line.substr(0,16) != "Encoding: Custom")) {
		encoding = line;
		&split(encoding, ' ', encodings);

		enc1.push_back(atoi(encodings.at(i + 1).c_str()) );
		enc2.push_back(atoi(encodings.at(i + 2).c_str()) );
		enc3.push_back(atoi(encodings.at(i + 3).c_str()) );

		if (hasEnding(glyphname, suffix) ) {
			baseglyphs.push_back(glyphname.substr(11,99));


		}

		i = i + 4;
	}
	glyphname = line;


    }

fontfile.close();
fontfile.open (FileName);

// FINDS CURSOR

    bool act = false;

    while ( getline (fontfile,line) )
    {


	if (line.substr(0,11) == "StartChar: " && line.substr(11,99) == "start" + tagname) {

		act = true;
	}

	if (act) { 
		if ( parameter("Encoding: ").length() > 0 ) { CursorEncoding = parameter("Encoding: "); &split(CursorEncoding, ' ', CursorEncodings); }
	}
	if (act && line == "EndChar") {
		act = false;
	}

    }


fontfile.close();
fontfile.open (FileName);

// WRITES DUPLICATE GLYPHS

			ofstream glyphs;
			glyphs.open ("glyphs.txt");

    act = false;

int index = 1;

    while ( getline (fontfile,line) )
    {

	// FINDS BASE GLYPHS


	if (line.substr(0,11) == "StartChar: " && find(baseglyphs.begin(), baseglyphs.end(), line.substr(11,99)) != baseglyphs.end() ) {

		act = true;
		writeglyphname = line.substr(11,99);
	}

	if (act) { 

		if ( parameter("Width: ").length() > 0 ) { Width = parameter("Width: "); }
		if ( parameter("VWidth: ").length() > 0 ) { VWidth = parameter("VWidth: "); }
		if ( parameter("Flags: ").length() > 0 ) { Flags = parameter("Flags: "); }
		if ( parameter("Encoding: ").length() > 0 ) { ReferEncoding = parameter("Encoding: "); &split(ReferEncoding, ' ', ReferEncodings); }
	}
	if (act && line == "EndChar") {
		act = false;

			glyphs << "StartChar: " << writeglyphname.substr(0, writeglyphname.find(".")) << suffix << "flag" << "\n" ;
			glyphs << "Encoding: " << *max_element(enc1.begin(), enc1.end()) + index << " -1 " << *max_element(enc3.begin(), enc3.end()) + index << "\n" ;
			glyphs << "Width: " << Width << "\n" ;
			glyphs << "VWidth: " << VWidth << "\n" ;
			glyphs << "Flags: " << Flags << "\n" ;
			glyphs << "LayerCount: 2\nBack\nFore" << "\n" ;
			glyphs << "Refer: " << ReferEncodings.at(2) << " " << ReferEncodings.at(0) << " N 1 0 0 1 0 0 1" << "\n" ;
			glyphs << "Refer: " << CursorEncodings.at(2) << " " << CursorEncodings.at(0) << " N 1 0 0 1 " << Width << " 0 2" << "\n" ;
			glyphs << "Validated: 1" << "\n" ;
			glyphs << "EndChar" << "\n\n" ;
			ReferEncodings.clear();

			index = index + 1;

	}

    }

glyphs.close();

// START WRITING FEATURE RULES

ofstream featurefile;
featurefile.open ("feature.fea");

// Write scripts
featurefile << "languagesystem DFLT dflt;\nlanguagesystem grek dflt;\nlanguagesystem latn dflt;\nlanguagesystem math dflt;\n\n" ;

// Write letter class
featurefile << "@" << tagname << "characters = [" ;

for (int n = 1; n <= baseglyphs.size(); n++) {
	featurefile << "\\" << baseglyphs.at(n - 1).substr(0, baseglyphs.at(n - 1).find(".")) << " ";
}

featurefile << "];\n\n" ;

// Write base class
featurefile << "@" << tagname << "base = [" ;

for (int n = 1; n <= baseglyphs.size(); n++) {
	featurefile << "\\" << baseglyphs.at(n - 1) << " ";
}

featurefile << "];\n\n" ;

// Write flagged class
featurefile << "@" << tagname << "flags = [" ;

for (int n = 1; n <= baseglyphs.size(); n++) {
	featurefile << "\\" << baseglyphs.at(n - 1).substr(0, baseglyphs.at(n - 1).find(".")) << suffix << "flag" << " ";
}

featurefile << "];\n\n" ;

// Write initial lookup

featurefile << "feature liga {\n" ;

featurefile << "lookup " << featurename << "Open {\n" ;
featurefile << "sub " << "\\less " << spacedtagname << " \\greater" << " by " << "\\start" << tagname << ";\n" ;

featurefile << "} " << featurename << "Open;\n" ;

featurefile << "lookup " << featurename << "Initialize {\n" ;
for (int n = 1; n <= baseglyphs.size(); n++) {
	featurefile << "sub " << "\\start" << tagname << " " << "\\" << baseglyphs.at(n - 1).substr(0, baseglyphs.at(n - 1).find(".")) << " by " << "\\" << baseglyphs.at(n - 1).substr(0, baseglyphs.at(n - 1).find(".")) << suffix << "flag" << ";\n" ;
}

featurefile << "} " << featurename << "Initialize;\n" ;
featurefile << "} liga;\n\n" ;

// Write chaining lookup

featurefile << "feature calt {\n" ;
featurefile << "lookup " << featurename << "Chaining {\n" ;
featurefile << "sub " << "@" << tagname << "flags" << " " << "@" << tagname << "characters" << "'" << " by " << "@" << tagname << "flags" << ";\n" ;

featurefile << "} " << featurename << "Chaining;\n" ;

// Undo repeated flags
featurefile << "lookup " << featurename << "Rendering {\n" ;
featurefile << "sub " << "@" << tagname << "flags" << "'" << " " << "@" << tagname << "flags" << " by " << "@" << tagname << "base" << ";\n" ;

featurefile << "} " << featurename << "Rendering;\n" ;

featurefile << "} calt;\n\n" ;

// Write closing tag lookup

featurefile << "feature liga {\n" ;
featurefile << "lookup " << featurename << "Close {\n" ;
for (int n = 1; n <= baseglyphs.size(); n++) {
	featurefile << "sub " << "\\" << baseglyphs.at(n - 1).substr(0, baseglyphs.at(n - 1).find(".")) << suffix << "flag" << " " << "\\less \\slash " << spacedtagname << " \\greater" << " by " << "\\" << baseglyphs.at(n - 1) << ";\n" ;
}

featurefile << "} " << featurename << "Close;\n" ;
featurefile << "} liga;\n\n" ;

featurefile.close();

return(0);
}
