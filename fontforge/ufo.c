/* Copyright (C) 2003-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <fontforge-config.h>

#ifndef _NO_PYTHON
# include "Python.h"
# include "structmember.h"
#else
# include <utype.h>
#endif

#include "autohint.h"
#include "dumppfa.h"
#include "featurefile.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "fvfonts.h"
#include "lookups.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "svg.h"
#include "tottf.h"
#include "tottfgpos.h"
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <chardata.h>
#include <gfile.h>
#include <ustring.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifndef _NO_PYTHON
# include "ffpython.h"
#endif

#include <stdarg.h>
#include <string.h>

#undef extended			/* used in xlink.h */
#include <libxml/tree.h>

#ifdef FF_UTHASH_GLIF_NAMES
#include "glif_name_hash.h"
#endif

/* The UFO (Unified Font Object) format ( http://unifiedfontobject.org/ ) */
/* is a directory containing a bunch of (mac style) property lists and another*/
/* directory containing glif files (and contents.plist). */

/* Property lists contain one <dict> element which contains <key> elements */
/*  each followed by an <integer>, <real>, <true/>, <false/> or <string> element, */
/*  or another <dict> */

/* UFO format 2.0 includes an adobe feature file "features.fea" and slightly */
/*  different/more tags in fontinfo.plist */

static char *buildname(const char *basedir, const char *sub) {
    char *fname = malloc(strlen(basedir)+strlen(sub)+2);

    strcpy(fname, basedir);
    if ( fname[strlen(fname)-1]!='/' )
	strcat(fname,"/");
    strcat(fname,sub);
return( fname );
}

static void extractNumericVersion(const char * textVersion, int * versionMajor, int * versionMinor) {
  // We extract integer values for major and minor versions from a version string.
  *versionMajor = -1; *versionMinor = -1;
  if (textVersion == NULL) return;
  char *nextText1 = NULL;
  char *nextText2 = NULL;
  int tempVersion = -1;
  tempVersion = strtol(textVersion, &nextText1, 10);
  if (tempVersion != -1 && nextText1 != NULL && (nextText1[0] == '\0' || nextText1[0] == ' ' || nextText1[0] == '.')) {
    *versionMajor = tempVersion;
  } else return;
  if (nextText1[0] == '\0') return;
  tempVersion = strtol(nextText1+1, &nextText2, 10);
  if (tempVersion != -1 && nextText2 != NULL && (nextText2[0] == '\0' || nextText2[0] == ' ' || nextText2[0] == '.')) {
    *versionMinor = tempVersion;
  } else return;
  return;
}

static void injectNumericVersion(char ** textVersion, int versionMajor, int versionMinor) {
  // We generate a version string from numeric values if available.
  int err = 0;
  if (versionMajor == -1) err |= (asprintf(textVersion, "%s", "") < 0);
  else if (versionMinor == -1) err |= (asprintf(textVersion, "%d", versionMajor) < 0);
  else err |= (asprintf(textVersion, "%d.%d", versionMajor, versionMinor) < 0);
  return;
}

/* The spec does not really require padding the version str but it clears */
/* 1.8 vs. 1.008 ambiguities and makes us inline with the AFDKO. */
char* paddedVersionStr(const char * textVersion, char * buffer) {
    int major = -1;
    int minor = -1;
    extractNumericVersion(textVersion, &major, &minor);
    if (major < 0 || minor < 0) return (char *) textVersion;
    snprintf(buffer, 6, "%d.%03d", major, minor);
    return buffer;
}

const char * DOS_reserved[12] = {"CON", "PRN", "AUX", "CLOCK$", "NUL", "COM1", "COM2", "COM3", "COM4", "LPT1", "LPT2", "LPT3"};
const int DOS_reserved_count = 12;

int polyMatch(const char * input, int reference_count, const char ** references) {
  for (off_t pos = 0; pos < reference_count; pos++) {
    if (strcmp(references[pos], input) == 0) return 1;
  }
  return 0;
}

int is_DOS_drive(char * input) {
  if ((input != NULL) &&
     (strlen(input) == 2) &&
     (((input[0] >= 'A') && (input[0] <= 'Z')) || ((input[0] >= 'a') && (input[0] <= 'z'))) &&
     (input[1] == ':'))
       return 1;
  return 0;
}

char * ufo_name_mangle(const char * input, const char * prefix, const char * suffix, int flags) {
  // This does not append the prefix or the suffix.
  // flags & 1 determines whether to post-pad caps (something implemented in the standard).
  // flags & 2 determines whether to replace a leading '.' (standard).
  // flags & 4 determines whether to restrict DOS names (standard).
  // flags & 8 determines whether to implement additional character restrictions.
  // The specification lists '"' '*' '+' '/' ':' '<' '>' '?' '[' '\\' ']' '|'
  // and also anything in the range 0x00-0x1F and 0x7F.
  // Our additional restriction list includes '\'' '&' '%' '$' '#' '`' '=' '!' ';'
  // Standard behavior comes from passing a flags value of 7.
  const char * standard_restrict = "\"*+/:<>?[]\\]|";
  const char * extended_restrict = "\'&%$#`=!;";
  size_t prefix_length = strlen(prefix);
  size_t max_length = 255 - prefix_length - strlen(suffix);
  size_t input_length = strlen(input);
  size_t stop_pos = ((max_length < input_length) ? max_length : input_length); // Minimum.
  size_t output_length_1 = input_length;
  if (flags & 1) output_length_1 += count_caps(input); // Add space for underscore pads on caps.
  off_t output_pos = 0;
  char * output = malloc(output_length_1 + 1);
  for (int i = 0; i < input_length; i++) {
    if (strchr(standard_restrict, input[i]) || (input[i] <= 0x1F) || (input[i] >= 0x7F)) {
      output[output_pos++] = '_'; // If the character is restricted, place an underscore.
    } else if ((flags & 8) && strchr(extended_restrict, input[i])) {
      output[output_pos++] = '_'; // If the extended restriction list is enabled and matches, ....
    } else if ((flags & 1) && (input[i] >= 'A') && (input[i] <= 'Z')) {
      output[output_pos++] = input[i];
      output[output_pos++] = '_'; // If we have a capital letter, we post-pad if desired.
    } else if ((flags & 2) && (i == 0) && (prefix_length == 0) && (input[i] == '.')) {
      output[output_pos++] = '_'; // If we have a leading '.', we convert to an underscore.
    } else {
      output[output_pos++] = input[i];
    }
  }
  output[output_pos] = '\0';
  if (output_pos > max_length) {
    output[max_length] = '\0';
  }
  char * output2 = NULL;
  off_t output2_pos = 0;
  {
    char * disposable = malloc(output_length_1 + 1);
    strcpy(disposable, output); // strtok rewrites the input string, so we make a copy.
    output2 = malloc((2 * output_length_1) + 1); // It's easier to pad than to calculate.
    output2_pos = 0;
    char * saveptr = NULL;
    char * current = strtok_r(disposable, ".", &saveptr); // We get the first name part.
    while (current != NULL) {
      char * uppered = upper_case(output);
      if (polyMatch(uppered, DOS_reserved_count, DOS_reserved) || is_DOS_drive(uppered)) {
        output2[output2_pos++] = '_'; // Prefix an underscore if it's a reserved name.
      }
      free(uppered); uppered = NULL;
      for (off_t parti = 0; current[parti] != '\0'; parti++) {
        output2[output2_pos++] = current[parti];
      }
      current = strtok_r(NULL, ".", &saveptr);
      if (current != NULL) output2[output2_pos++] = '.';
    }
    output2[output2_pos] = '\0';
    output2 = realloc(output2, output2_pos + 1);
    free(disposable); disposable = NULL;
  }
  free(output); output = NULL;
  return output2;
}

char * ufo_name_number(
#ifdef FF_UTHASH_GLIF_NAMES
struct glif_name_index * glif_name_hash,
#else
void * glif_name_hash,
#endif
int index, const char * input, const char * prefix, const char * suffix, int flags) {
        // This does not append the prefix or the suffix.
        // The specification deals with name collisions by appending a 15-digit decimal number to the name.
        // But the name length cannot exceed 255 characters, so it is necessary to crop the base name if it is too long.
        // Name exclusions are case insensitive, so we uppercase.
        // flags & 16 forces appending a number.
	int err = 0; // Not terribly useful but for suppressing warnings.
        char * name_numbered = upper_case(input);
        char * full_name_base = same_case(input); // This is in case we do not need a number added.
        if (strlen(input) > (255 - strlen(prefix) - strlen(suffix))) {
          // If the numbered name base is too long, we crop it, even if we are not numbering.
          full_name_base[(255 - strlen(suffix))] = '\0';
          full_name_base = realloc(full_name_base, ((255 - strlen(prefix) - strlen(suffix)) + 1));
        }
        char * name_base = same_case(input); // This is in case we need a number added.
        long int name_number = 0;
#ifdef FF_UTHASH_GLIF_NAMES
        if (glif_name_hash != NULL) {
          if (strlen(input) > (255 - 15 - strlen(prefix) - strlen(suffix))) {
            // If the numbered name base is too long, we crop it.
            name_base[(255 - 15 - strlen(suffix))] = '\0';
            name_base = realloc(name_base, ((255 - 15 - strlen(prefix) - strlen(suffix)) + 1));
          }
          int number_once = ((flags & 16) ? 1 : 0);
          // Check the resulting name against a hash table of names.
          if (glif_name_search_glif_name(glif_name_hash, name_numbered) != NULL || number_once) {
            // If the name is taken, we must make space for a 15-digit number.
            char * name_base_upper = upper_case(name_base);
            while (glif_name_search_glif_name(glif_name_hash, name_numbered) != NULL || number_once) {
              name_number++; // Remangle the name until we have no more matches.
              free(name_numbered); name_numbered = NULL;
              err |= (asprintf(&name_numbered, "%s%015ld", name_base_upper, name_number) < 0);
              number_once = 0;
            }
            free(name_base_upper); name_base_upper = NULL;
          }
          // Insert the result into the hash table.
          glif_name_track_new(glif_name_hash, index, name_numbered);
        }
#endif
        // Now we want the correct capitalization.
        free(name_numbered); name_numbered = NULL;
        if (name_number > 0) {
          err |= (asprintf(&name_numbered, "%s%015ld", name_base, name_number) < 0);
        } else {
          err |= (asprintf(&name_numbered, "%s", full_name_base) < 0);
        }
        free(name_base); name_base = NULL;
        free(full_name_base); full_name_base = NULL;
	if (err) {
	  // This is not really good error handling.
	  // But we are mostly just trying to make the compiler happy.
          LogError(_("Error generating names in ufo_name_number."));
	}
        return name_numbered;
}

static xmlNodePtr xmlNewChildInteger(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, long int value) {
  char * valtmp = NULL;
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%ld", value) >= 0) {
    xmlNodePtr childtmp = xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST valtmp); // Make a text node for the value.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
    return childtmp;
  }
  return NULL;
}
static xmlNodePtr xmlNewNodeInteger(xmlNsPtr ns, const xmlChar * name, long int value) {
  char * valtmp = NULL;
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%ld", value) >= 0) {
    xmlNodePtr valtmpxml = xmlNewText(BAD_CAST valtmp); // Make a text node for the value.
    xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
  } else {
    xmlFreeNode(childtmp); childtmp = NULL;
  }
  return childtmp;
}
static xmlNodePtr xmlNewChildFloat(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, double value) {
  char * valtmp = NULL;
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%g", value) >= 0) {
    xmlNodePtr childtmp = xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST valtmp); // Make a text node for the value.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
    return childtmp;
  }
  return NULL;
}
static xmlNodePtr xmlNewNodeFloat(xmlNsPtr ns, const xmlChar * name, double value) {
  char * valtmp = NULL;
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  // Textify the value to be enclosed.
  if (asprintf(&valtmp, "%g", value) >= 0) {
    xmlNodePtr valtmpxml = xmlNewText(BAD_CAST valtmp); // Make a text node for the value.
    xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
    free(valtmp); valtmp = NULL; // Free the temporary text store.
  } else {
    xmlFreeNode(childtmp); childtmp = NULL;
  }
  return NULL;
}
static xmlNodePtr xmlNewChildString(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, char * value) {
  xmlNodePtr childtmp = xmlNewTextChild(parent, ns, BAD_CAST name, BAD_CAST value); // Make a text node for the value.
  return childtmp;
}
static xmlNodePtr xmlNewNodeString(xmlNsPtr ns, const xmlChar * name, char * value) {
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  xmlNodePtr valtmpxml = xmlNewText(BAD_CAST value); // Make a text node for the value.
  xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
  return childtmp;
}
static xmlNodePtr xmlNewNodeVPrintf(xmlNsPtr ns, const xmlChar * name, char * format, va_list arguments) {
  char * valtmp = NULL;
  if (vasprintf(&valtmp, format, arguments) < 0) {
    return NULL;
  }
  xmlNodePtr childtmp = xmlNewNode(NULL, BAD_CAST name); // Create a named node.
  xmlNodePtr valtmpxml = xmlNewText(BAD_CAST valtmp); // Make a text node for the value.
  xmlAddChild(childtmp, valtmpxml); // Attach the text node as content of the named node.
  free(valtmp); valtmp = NULL; // Free the temporary text store.
  return childtmp;
}
static xmlNodePtr xmlNewNodePrintf(xmlNsPtr ns, const xmlChar * name, char * format, ...) {
  va_list arguments;
  va_start(arguments, format);
  xmlNodePtr output = xmlNewNodeVPrintf(ns, name, format, arguments);
  va_end(arguments);
  return output;
}
static xmlNodePtr xmlNewChildVPrintf(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, char * format, va_list arguments) {
  xmlNodePtr output = xmlNewNodeVPrintf(ns, name, format, arguments);
  xmlAddChild(parent, output);
  return output;
}
static xmlNodePtr xmlNewChildPrintf(xmlNodePtr parent, xmlNsPtr ns, const xmlChar * name, char * format, ...) {
  va_list arguments;
  va_start(arguments, format);
  xmlNodePtr output = xmlNewChildVPrintf(parent, ns, name, format, arguments);
  va_end(arguments);
  return output;
}
static void xmlSetPropVPrintf(xmlNodePtr target, const xmlChar * name, char * format, va_list arguments) {
  char * valtmp = NULL;
  // Generate the value.
  if (vasprintf(&valtmp, format, arguments) < 0) {
    return;
  }
  xmlSetProp(target, name, valtmp); // Set the property.
  free(valtmp); valtmp = NULL; // Free the temporary text store.
  return;
}
static void xmlSetPropPrintf(xmlNodePtr target, const xmlChar * name, char * format, ...) {
  va_list arguments;
  va_start(arguments, format);
  xmlSetPropVPrintf(target, name, format, arguments);
  va_end(arguments);
  return;
}

/* ************************************************************************** */
/* ****************************    PList Output    ************************** */
/* ************************************************************************** */

int count_occurrence(const char* big, const char* little) {
    const char * tmp = big;
    int output = 0;
    while (tmp = strstr(tmp, little)) { output ++; tmp ++; }
    return output;
}

static char* normalizeToASCII(char *str) {
    if ( str!=NULL && !AllAscii(str))
        return StripToASCII(str);
    else
        return copy(str);
}

xmlDocPtr PlistInit() {
    // Some of this code is pasted from libxml2 samples.
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL, dict_node = NULL;
    xmlDtdPtr dtd = NULL;
    
    char buff[256];
    int i, j;

    LIBXML_TEST_VERSION;

    doc = xmlNewDoc(BAD_CAST "1.0");
    dtd = xmlCreateIntSubset(doc, BAD_CAST "plist", BAD_CAST "-//Apple Computer//DTD PLIST 1.0//EN", BAD_CAST "http://www.apple.com/DTDs/PropertyList-1.0.dtd");
    root_node = xmlNewNode(NULL, BAD_CAST "plist");
    xmlSetProp(root_node, BAD_CAST "version", BAD_CAST "1.0");
    xmlDocSetRootElement(doc, root_node);
    return doc;
}

static void PListAddInteger(xmlNodePtr parent, const char *key, int value) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChildPrintf(parent, NULL, BAD_CAST "integer", "%d", value);
}

static void PListAddReal(xmlNodePtr parent, const char *key, double value) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChildPrintf(parent, NULL, BAD_CAST "real", "%g", value);
}

static void PListAddIntegerOrReal(xmlNodePtr parent, const char *key, double value) {
    if (value == floor(value)) PListAddInteger(parent, key, (int)value);
    else PListAddReal(parent, key, value);
}

static void PListAddBoolean(xmlNodePtr parent, const char *key, int value) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChild(parent, NULL, BAD_CAST (value ? "true": "false"), NULL);
}

static void PListAddDate(xmlNodePtr parent, const char *key, time_t timestamp) {
/* openTypeHeadCreated = string format as \"YYYY/MM/DD HH:MM:SS\".	*/
/* \"YYYY/MM/DD\" is year/month/day. The month is in the range 1-12 and	*/
/* the day is in the range 1-end of month.				*/
/*  \"HH:MM:SS\" is hour:minute:second. The hour is in the range 0:23.	*/
/* Minutes and seconds are in the range 0-59.				*/
    struct tm *tm = gmtime(&timestamp);
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNewChildPrintf(parent, NULL, BAD_CAST "string",
            "%4d/%02d/%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1,
	    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void PListAddString(xmlNodePtr parent, const char *key, const char *value) {
    if ( value==NULL ) value = "";
    xmlNodePtr keynode = xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key); // "<key>%s</key>" key
#ifdef ESCAPE_LIBXML_STRINGS
    size_t main_size = strlen(value) +
                       (count_occurrence(value, "<") * (strlen("&lt")-strlen("<"))) +
                       (count_occurrence(value, ">") * (strlen("&gt")-strlen("<"))) +
                       (count_occurrence(value, "&") * (strlen("&amp")-strlen("<")));
    char *tmpstring = malloc(main_size + 1); tmpstring[0] = '\0';
    off_t pos1 = 0;
    while ( *value ) {
	if ( *value=='<' ) {
	    strcat(tmpstring, "&lt;");
            pos1 += strlen("&lt;");
	} else if ( *value=='>' ) {
	    strcat(tmpstring, "&gt;");
            pos1 += strlen("&gt;");
	} else if ( *value == '&' ) {
	    strcat(tmpstring, "&amp;");
	    pos1 += strlen("&amp;");
	} else {
	    tmpstring[pos1++] = *value;
            tmpstring[pos1] = '\0';
        }
	++value;
    }
    xmlNodePtr valnode = xmlNewChild(parent, NULL, BAD_CAST "string", tmpstring); // "<string>%s</string>" tmpstring
#else
    xmlNodePtr valnode = xmlNewTextChild(parent, NULL, BAD_CAST "string", value); // "<string>%s</string>" tmpstring
#endif
}

static void PListAddNameString(xmlNodePtr parent, const char *key, const SplineFont *sf, int strid) {
    char *value=NULL, *nonenglish=NULL, *freeme=NULL;
    struct ttflangname *nm;

    for ( nm=sf->names; nm!=NULL; nm=nm->next ) {
	if ( nm->names[strid]!=NULL ) {
	    nonenglish = nm->names[strid];
	    if ( nm->lang == 0x409 ) {
		value = nm->names[strid];
    break;
	    }
	}
    }
    if ( value==NULL && strid==ttf_version && sf->version!=NULL ) {
	char versionStr[6];
	paddedVersionStr(sf->version, versionStr);
	value = freeme = strconcat("Version ", versionStr);
    }
    if ( value==NULL && strid==ttf_copyright && sf->copyright!=NULL )
	value = sf->copyright;
    if ( value==NULL )
	value=nonenglish;
    if ( value!=NULL ) {
	PListAddString(parent,key,value);
    }
    free(freeme);
}

static void PListAddIntArray(xmlNodePtr parent, const char *key, const char *entries, int len) {
    int i;
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key);
    xmlNodePtr arrayxml = xmlNewChild(parent, NULL, BAD_CAST "array", NULL);
    for ( i=0; i<len; ++i ) {
      xmlNewChildInteger(arrayxml, NULL, BAD_CAST "integer", entries[i]);
    }
}

static void PListAddPrivateArray(xmlNodePtr parent, const char *key, struct psdict *private) {
    char *value;
    if ( private==NULL )
return;
    value = PSDictHasEntry(private,key);
    if ( value==NULL )
return;
    xmlNewChildPrintf(parent, NULL, BAD_CAST "key", "postscript%s", key); // "<key>postscript%s</key>" key
    xmlNodePtr arrayxml = xmlNewChild(parent, NULL, BAD_CAST "array", NULL); // "<array>"
    while ( *value==' ' || *value=='[' ) ++value;
    for (;;) {
	int havedot=0;
	int skipping=0;
        size_t tmpsize = 8;
        char * tmp = malloc(tmpsize);
        off_t tmppos = 0;
	while ( *value!=']' && *value!='\0' && *value!=' ' && tmp!=NULL) {
            // We now deal with non-integers as necessary.
            if (*value=='.') {
              if (havedot) skipping = true;
              else havedot = 1;
            }
	    if (skipping)
		++value;
	    else
                tmp[tmppos++] = *value++;
            if (tmppos == tmpsize) { tmpsize *= 2; tmp = realloc(tmp, tmpsize); }
	}
        tmp[tmppos] = '\0';
        if (tmp != NULL) {
          if (havedot)
            xmlNewChildString(arrayxml, NULL, BAD_CAST "real", BAD_CAST tmp); // "<real>%s</real>" tmp
          else
            xmlNewChildString(arrayxml, NULL, BAD_CAST "integer", BAD_CAST tmp); // "<integer>%s</integer>" tmp
          free(tmp); tmp = NULL;
        }
	while ( *value==' ' ) ++value;
	if ( *value==']' || *value=='\0' ) break;
    }
    // "</array>"
}

static void PListAddPrivateThing(xmlNodePtr parent, const char *key, struct psdict *private, char *type) {
    char *value;

    if ( private==NULL ) return;
    value = PSDictHasEntry(private,key);
    if ( value==NULL ) return;

    while ( *value==' ' || *value=='[' ) ++value;

    xmlNewChildPrintf(parent, NULL, BAD_CAST "key", "postscript%s", key); // "<key>postscript%s</key>" key
    while ( *value==' ' || *value=='[' ) ++value;
    {
	int havedot=0;
	int skipping=0;
        size_t tmpsize = 8;
        char * tmp = malloc(tmpsize);
        off_t tmppos = 0;
	while ( *value!=']' && *value!='\0' && *value!=' ' && tmp!=NULL) {
            // We now deal with non-integers as necessary.
            if (*value=='.') {
              if (havedot) skipping = true;
              else havedot = 1;
            }
	    if (skipping)
		++value;
	    else
                tmp[tmppos++] = *value++;
            if (tmppos == tmpsize) { tmpsize *= 2; tmp = realloc(tmp, tmpsize); }
	}
        tmp[tmppos] = '\0';
        if (tmp != NULL) {
          if (havedot)
            xmlNewChildString(parent, NULL, BAD_CAST "real", BAD_CAST tmp); // "<real>%s</real>" tmp
          else
            xmlNewChildString(parent, NULL, BAD_CAST "integer", BAD_CAST tmp); // "<integer>%s</integer>" tmp
          free(tmp); tmp = NULL;
        }
	while ( *value==' ' ) ++value;
    }
}

/* ************************************************************************** */
/* *************************   Python lib Output    ************************* */
/* ************************************************************************** */
#ifndef _NO_PYTHON
static int PyObjDumpable(PyObject *value, int has_lists);
xmlNodePtr PyObjectToXML( PyObject *value, int has_lists );
xmlNodePtr PythonDictToXML(PyObject *dict, xmlNodePtr target, const char **exclusions, int has_lists);
#endif

int stringInStrings(const char *target, const char **reference) {
	if (reference == NULL) return false;
	off_t pos;
	for (pos = 0; reference[pos] != NULL; pos++)
		if (strcmp(target, reference[pos]) == 0)
			return true;
	return false;
}

xmlNodePtr PythonLibToXML(void *python_persistent, const SplineChar *sc, int has_lists) {
    int has_hints = (sc!=NULL && (sc->hstem!=NULL || sc->vstem!=NULL ));
    xmlNodePtr retval = NULL, dictnode = NULL, keynode = NULL, valnode = NULL;
    // retval = xmlNewNode(NULL, BAD_CAST "lib"); //     "<lib>"
    dictnode = xmlNewNode(NULL, BAD_CAST "dict"); //     "  <dict>"
    if ( has_hints 
#ifndef _NO_PYTHON
         || (python_persistent!=NULL && PyMapping_Check((PyObject *)python_persistent))
#endif
       ) {

        xmlAddChild(retval, dictnode);
        /* Not officially part of the UFO/glif spec, but used by robofab */
	if ( has_hints ) {
            // Remember that the value of the plist key is in the node that follows it in the dict (not an x.m.l. child).
            xmlNewChild(dictnode, NULL, BAD_CAST "key", BAD_CAST "com.fontlab.hintData"); // Label the hint data block.
	    //                                           "    <key>com.fontlab.hintData</key>\n"
	    //                                           "    <dict>"
            xmlNodePtr hintdict = xmlNewChild(dictnode, NULL, BAD_CAST "dict", NULL);
            if ( sc != NULL ) {
	        if ( sc->hstem!=NULL ) {
                    StemInfo *h;
                    xmlNewChild(hintdict, NULL, BAD_CAST "key", BAD_CAST "hhints");
                    //                                   "      <key>hhints</key>"
                    //                                   "      <array>"
                    xmlNodePtr hintarray = xmlNewChild(hintdict, NULL, BAD_CAST "array", NULL);
                    for ( h = sc->hstem; h!=NULL; h=h->next ) {
                        char * valtmp = NULL;
                        xmlNodePtr stemdict = xmlNewChild(hintarray, NULL, BAD_CAST "dict", NULL);
		        //                               "        <dict>"
                        xmlNewChild(stemdict, NULL, BAD_CAST "key", "position");
		        //                               "          <key>position</key>"
                        xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->start));
		        //                               "          <integer>%d</integer>\n" ((int) rint(h->start))
                        xmlNewChild(stemdict, NULL, BAD_CAST "key", "width");
		        //                               "          <key>width</key>"
                        xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->width));
		        //                               "          <integer>%d</integer>\n" ((int) rint(h->width))
		        //                               "        </dict>\n"
		    }
		    //                                   "      </array>\n"
	        }
	        if ( sc->vstem!=NULL ) {
                    StemInfo *h;
                    xmlNewChild(hintdict, NULL, BAD_CAST "key", BAD_CAST "vhints");
                    //                                   "      <key>vhints</key>"
                    //                                   "      <array>"
                    xmlNodePtr hintarray = xmlNewChild(hintdict, NULL, BAD_CAST "array", NULL);
                    for ( h = sc->vstem; h!=NULL; h=h->next ) {
                        char * valtmp = NULL;
                        xmlNodePtr stemdict = xmlNewChild(hintarray, NULL, BAD_CAST "dict", NULL);
                        //                               "        <dict>"
                        xmlNewChild(stemdict, NULL, BAD_CAST "key", "position");
                        //                               "          <key>position</key>"
                        xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->start));
                        //                               "          <integer>%d</integer>\n" ((int) rint(h->start))
                        xmlNewChild(stemdict, NULL, BAD_CAST "key", "width");
                        //                               "          <key>width</key>"
                        xmlNewChildInteger(stemdict, NULL, BAD_CAST "integer", (int) rint(h->width));
                        //                               "          <integer>%d</integer>\n" ((int) rint(h->width))
                        //                               "        </dict>\n"
                    }
                    //                                   "      </array>\n"
	        }
	    }
	    //                                           "    </dict>"
	}
#ifndef _NO_PYTHON
	/* Ok, look at the persistent data and output it (all except for a */
	/*  hint entry -- we've already handled that with the real hints, */
	/*  no point in retaining out of date hints too */
	const char *sc_exclusions[] = {"com.fontlab.hintData", NULL};
	const char *no_exclusions[] = {NULL};
	if ( python_persistent != NULL ) {
          if (!PyMapping_Check((PyObject *)python_persistent)) fprintf(stderr, "python_persistent is not a mapping.\n");
          else {
		PythonDictToXML((PyObject *)python_persistent, dictnode, (sc ? sc_exclusions : no_exclusions), has_lists);
	  }
	}
#endif
    }
    //                                                 "  </dict>"
    // //                                                 "</lib>"
    return dictnode;
}

#ifndef _NO_PYTHON
static int PyObjDumpable(PyObject *value, int has_lists) {
    if ( PyTuple_Check(value))
return( true ); // Note that this means two different things depending upon has_lists.
    if ( PyList_Check(value))
return( true );
    if ( PyInt_Check(value))
return( true );
    if ( PyFloat_Check(value))
return( true );
	if ( PyDict_Check(value))
return( true );
    if ( PyBytes_Check(value))
return( true );
    if ( has_lists && PyList_Check(value))
return( true );
    if ( PyMapping_Check(value))
return( true );
    if ( PyBool_Check(value))
return( true );
    if ( value == Py_None )
return( true );

return( false );
}

xmlNodePtr PyObjectToXML( PyObject *value, int has_lists ) {
    xmlNodePtr childtmp = NULL;
    xmlNodePtr valtmpxml = NULL;
    char * valtmp = NULL;
    if (has_lists && PyTuple_Check(value) && (PyTuple_Size(value) == 3) &&
       PyBytes_Check(PyTuple_GetItem(value,0)) && PyBytes_Check(PyTuple_GetItem(value,1))) {
      // This is a chunk of unrecognized data.
      // Since there's no equivalent in X. M. L., we can use the Tuple for this special case.
      // But we can only do this if the arrays are being mapped as lists rather than as tuples.
      // So we miss foreign data in old S. F. D. versions.
      // childtmp = xmlNewNode(NULL, (xmlChar*)(PyBytes_AsString(PyTuple_GetItem(value,0)))); // Set name.
      // xmlNodeSetContent(childtmp, (xmlChar*)(PyBytes_AsString(PyTuple_GetItem(value,1)))); // Set contents.
      xmlDocPtr innerdoc = xmlReadMemory((xmlChar*)(PyBytes_AsString(PyTuple_GetItem(value,1))),
                                         PyBytes_Size(PyTuple_GetItem(value,1)), "noname.xml", NULL, 0);
      childtmp = xmlCopyNode(xmlDocGetRootElement(innerdoc), 1);
      xmlFreeDoc(innerdoc); innerdoc = NULL;
    } else if (PyDict_Check(value)) {
      childtmp = PythonLibToXML(value,NULL,has_lists);
    } else if ( PyMapping_Check(value)) {
      childtmp = PythonLibToXML(value,NULL,has_lists);
    } else if ( PyBytes_Check(value)) {		/* Must precede the sequence check */
      char *str = PyBytes_AsString(value);
      if (str != NULL) {
        childtmp = xmlNewNodeString(NULL, BAD_CAST "string", str); // Create a string node.
          // "<string>%s</string>" str
      }
    } else if ( value==Py_True )
	childtmp = xmlNewNode(NULL, BAD_CAST "true"); // "<true/>"
    else if ( value==Py_False )
        childtmp = xmlNewNode(NULL, BAD_CAST "false"); // "<false/>"
    else if ( value==Py_None )
        childtmp = xmlNewNode(NULL, BAD_CAST "none");  // "<none/>"
    else if (PyInt_Check(value)) {
        childtmp = xmlNewNodeInteger(NULL, BAD_CAST "integer", PyInt_AsLong(value)); // Create an integer node.
        // "<integer>%ld</integer>"
    } else if (PyFloat_Check(value)) {
        childtmp = xmlNewNode(NULL, BAD_CAST "real");
        if (asprintf(&valtmp, "%g", PyFloat_AsDouble(value)) >= 0) {
          valtmpxml = xmlNewText(BAD_CAST valtmp);
          xmlAddChild(childtmp, valtmpxml);
          free(valtmp); valtmp = NULL;
        } else {
          xmlFreeNode(childtmp); childtmp = NULL;
        }
        // "<real>%g</real>"
    } else if ((!has_lists && PyTuple_Check(value)) || (has_lists && PyList_Check(value))) {
        // Note that PyList is an extension of PySequence, so the original PySequence code would work either way.
        // But we want to be able to detect mismatches.
	int i, len = ( has_lists ? PyList_Size(value) : PyTuple_Size(value) );
        xmlNodePtr itemtmp = NULL;
        childtmp = xmlNewNode(NULL, BAD_CAST "array");
        // "<array>"
	for ( i=0; i<len; ++i ) {
	    PyObject *obj = ( has_lists ? PyList_GetItem(value,i) : PyTuple_GetItem(value,i) );
	    if (obj != NULL) {
	      if ( PyObjDumpable(obj, has_lists)) {
	        itemtmp = PyObjectToXML(obj, has_lists);
                xmlAddChild(childtmp, itemtmp);
	      }
              obj = NULL; // PyTuple_GetItem and PyList_GetItem both return borrowed references.
	    }
	}
        // "</array>"
    }
    return childtmp;
}

xmlNodePtr PythonDictToXML(PyObject *dict, xmlNodePtr target, const char **exclusions, int has_lists) {
	// dict is the Python dictionary from which to extract data.
	// target is the xml node to which to add the contents of that dict.
	// exclusions is a NULL-terminated array of strings naming keys to exclude.
	PyObject *items, *key, *value;
	int i, len;
	char *str;
	items = PyMapping_Items(dict);
	len = PySequence_Size(items);
	for ( i=0; i<len; ++i ) {
		// According to the Python reference manual,
		// PySequence_GetItem returns a reference that we must release,
		// but PyTuple_GetItem returns a borrowed reference.
		PyObject *item = PySequence_GetItem(items,i);
		key = PyTuple_GetItem(item,0);
		if ( !PyBytes_Check(key))		/* Keys need not be strings */
			{ Py_DECREF(item); item = NULL; continue; }
		str = PyBytes_AsString(key);
		if ( !str || (stringInStrings(str, exclusions)) )	/* Already done */ // TODO: Fix this!
			{ Py_DECREF(item); item = NULL; continue; }
		value = PyTuple_GetItem(item,1);
		if ( !value || !PyObjDumpable(value, has_lists))
			{ Py_DECREF(item); item = NULL; continue; }
		// "<key>%s</key>" str
		xmlNewChild(target, NULL, BAD_CAST "key", str);
		xmlNodePtr tmpNode = PyObjectToXML(value, has_lists);
		xmlAddChild(target, tmpNode);
		// "<...>...</...>"
		Py_DECREF(item); item = NULL;
	}
	return target;
}
#endif

/* ************************************************************************** */
/* ****************************   GLIF Output    **************************** */
/* ************************************************************************** */

static int refcomp(const void *_r1, const void *_r2) {
    const RefChar *ref1 = *(RefChar * const *)_r1;
    const RefChar *ref2 = *(RefChar * const *)_r2;
return( strcmp( ref1->sc->name, ref2->sc->name) );
}

xmlNodePtr _GlifToXML(const SplineChar *sc, int layer, int version) {
    if (layer > sc->layer_cnt) return NULL;
    const struct altuni *altuni;
    int isquad = sc->layers[layer].order2;
    const SplineSet *spl;
    const SplinePoint *sp;
    const AnchorPoint *ap;
    const RefChar *ref;
    int err;
    char * stringtmp = NULL;
    char numstring[32];
    memset(numstring, 0, sizeof(numstring));

    // "<?xml version=\"1.0\" encoding=\"UTF-8\""
    /* No DTD for these guys??? */
    // Is there a DTD for glif data? (asks Frank)
    // Perhaps we need to make one.

    xmlNodePtr topglyphxml = xmlNewNode(NULL, BAD_CAST "glyph"); // Create the glyph node.
    xmlSetProp(topglyphxml, "name", sc->name); // Set the name for the glyph.
    char *formatString = "1";
    // If UFO is version 1 or 2, use "1" for format. Otherwise, use "2".
    if (version >= 3) formatString = "2";
    xmlSetProp(topglyphxml, "format", formatString); // Set the format of the glyph.
    // "<glyph name=\"%s\" format=\"1\">" sc->name

    xmlNodePtr tmpxml2 = xmlNewChild(topglyphxml, NULL, BAD_CAST "advance", NULL); // Create the advance node.
    xmlSetPropPrintf(tmpxml2, BAD_CAST "width", "%d", sc->width);
    if ( sc->parent->hasvmetrics ) {
      if (asprintf(&stringtmp, "%d", sc->width) >= 0) {
        xmlSetProp(tmpxml2, BAD_CAST "height", stringtmp);
        free(stringtmp); stringtmp = NULL;
      }
    }
    // "<advance width=\"%d\" height=\"%d\"/>" sc->width sc->vwidth

    if ( sc->unicodeenc!=-1 ) {
      xmlNodePtr unicodexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "unicode", NULL);
      xmlSetPropPrintf(unicodexml, BAD_CAST "hex", "%04X", sc->unicodeenc);
    }
    // "<unicode hex=\"%04X\"/>\n" sc->unicodeenc

    for ( altuni = sc->altuni; altuni!=NULL; altuni = altuni->next )
	if ( altuni->vs==-1 && altuni->fid==0 ) {
          xmlNodePtr unicodexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "unicode", NULL);
          xmlSetPropPrintf(unicodexml, BAD_CAST "hex", "%04X", altuni->unienc);
        }
        // "<unicode hex=\"%04X\"/>" altuni->unienc
    
	if (version >= 3) {
		// Handle the guidelines.
		GuidelineSet *gl;
		for ( gl=sc->layers[layer].guidelines; gl != NULL; gl = gl->next ) {
		    xmlNodePtr guidelinexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "guideline", NULL);
		    // gl->flags & 0x10 indicates whether to use the abbreviated format in the UFO specification if possible.
		    if (fmod(gl->angle, 180) || !(gl->flags & 0x10))
		        xmlSetPropPrintf(guidelinexml, BAD_CAST "x", "%g", gl->point.x);
		    if (fmod(gl->angle + 90, 180) || !(gl->flags & 0x10))
		        xmlSetPropPrintf(guidelinexml, BAD_CAST "y", "%g", gl->point.y);
		    if (fmod(gl->angle, 90) || !(gl->flags & 0x10))
		        xmlSetPropPrintf(guidelinexml, BAD_CAST "angle", "%g", fmod(gl->angle + 360, 360));
		    if (gl->name != NULL)
		        xmlSetPropPrintf(guidelinexml, BAD_CAST "name", "%s", gl->name);
		    if (gl->flags & 0x20) // color is set. Repack RGBA from a uint32 to a string with 0-1 scaled values comma-joined.
		        xmlSetPropPrintf(guidelinexml, BAD_CAST "color", "%g,%g,%g,%g",
		        (((double)((gl->color >> 24) & 0xFF)) / 255),
		        (((double)((gl->color >> 16) & 0xFF)) / 255),
		        (((double)((gl->color >> 8) & 0xFF)) / 255),
		        (((double)((gl->color >> 0) & 0xFF)) / 255)
		        );
		    if (gl->identifier != NULL)
		        xmlSetPropPrintf(guidelinexml, BAD_CAST "identifier", "%s", gl->identifier);
		    // "<guideline/>\n"
		    
		}
		// Handle the anchors. Put global anchors only in the foreground layer.
		if (layer == ly_fore)
			for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
			    int ismark = (ap->type==at_mark || ap->type==at_centry);
			    xmlNodePtr anchorxml = xmlNewChild(topglyphxml, NULL, BAD_CAST "anchor", NULL);
			    xmlSetPropPrintf(anchorxml, BAD_CAST "x", "%g", ap->me.x);
			    xmlSetPropPrintf(anchorxml, BAD_CAST "y", "%g", ap->me.y);
			    xmlSetPropPrintf(anchorxml, BAD_CAST "name", "%s%s", ismark ? "_" : "", ap->anchor->name);
			    // "<anchor x=\"%g\" y=\"%g\" name=\"%s%s\"/>" ap->me.x ap->me.y ap->anchor->name
			}
	}
    if (sc->comment) PListAddString(topglyphxml, "note", sc->comment);
    if ( sc->layers[layer].refs!=NULL || sc->layers[layer].splines!=NULL ) {
      xmlNodePtr outlinexml = xmlNewChild(topglyphxml, NULL, BAD_CAST "outline", NULL);
	// "<outline>"
	// Distinguish UFO 3 from UFO 2.
	if (version < 3) {
		if (layer == ly_fore)
			for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
			    int ismark = (ap->type==at_mark || ap->type==at_centry);
			    xmlNodePtr contourxml = xmlNewChild(outlinexml, NULL, BAD_CAST "contour", NULL);
			    // "<contour>"
			    xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
			    xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", ap->me.x);
			    xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", ap->me.y);
			    xmlSetPropPrintf(pointxml, BAD_CAST "type", "move");
			    xmlSetPropPrintf(pointxml, BAD_CAST "name", "%s%s", ismark ? "_" : "", ap->anchor->name);
			    // "<point x=\"%g\" y=\"%g\" type=\"move\" name=\"%s%s\"/>" ap->me.x ap->me.y (ismark ? "_" : "") ap->anchor->name
			    // "</contour>"
			}
	}
	for ( spl=sc->layers[layer].splines; spl!=NULL; spl=spl->next ) {
            xmlNodePtr contourxml = xmlNewChild(outlinexml, NULL, BAD_CAST "contour", NULL);
	    // "<contour>"
	    // We write any leading control points.
	    if (spl->start_offset == -2) {
		if (spl->first && spl->first->prev && spl->first->prev->from && !spl->first->prev->from->nonextcp && !spl->first->prev->order2) {
                          xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
                          xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)spl->first->prev->from->nextcp.x);
                          xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)spl->first->prev->from->nextcp.y);
                          // "<point x=\"%g\" y=\"%g\"/>\n" (double)sp->prevcp.x (double)sp->prevcp.y
		}
	    }
	    if (spl->start_offset <= -1) {
		if (spl->first && !spl->first->noprevcp) {
                          xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
                          xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)spl->first->prevcp.x);
                          xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)spl->first->prevcp.y);
                          // "<point x=\"%g\" y=\"%g\"/>\n" (double)sp->prevcp.x (double)sp->prevcp.y
		}
	    }
	    for ( sp=spl->first; sp!=NULL; ) {
		/* Undocumented fact: If a contour contains a series of off-curve points with no on-curve then treat as quadratic even if no qcurve */
		// We write the next on-curve point.
		if (!isquad || sp->ttfindex != 0xffff || !SPInterpolate(sp) || sp->pointtype!=pt_curve || sp->name != NULL) {
		  xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
		  xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)sp->me.x);
		  xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)sp->me.y);
		  xmlSetPropPrintf(pointxml, BAD_CAST "type", BAD_CAST (
		  sp->prev==NULL        ? "move"   :
					sp->noprevcp ? "line"   :
					isquad 		      ? "qcurve" :
					"curve"));
		  if (sp->pointtype != pt_corner) xmlSetProp(pointxml, BAD_CAST "smooth", BAD_CAST "yes");
		  if (sp->name !=NULL) xmlSetProp(pointxml, BAD_CAST "name", BAD_CAST sp->name);
                  // "<point x=\"%g\" y=\"%g\" type=\"%s\"%s%s%s%s/>\n" (double)sp->me.x (double)sp->me.y
		  // (sp->prev==NULL ? "move" : sp->prev->knownlinear ? "line" : isquad ? "qcurve" : "curve")
		  // (sp->pointtype!=pt_corner?" smooth=\"yes\"":"")
		  // (sp->name?" name=\"":"") (sp->name?sp->name:"") (sp->name?"\"":"")
		}
		if ( sp->next==NULL )
	    	  break;
		// We write control points.
		// The conditionals regarding the start offset avoid duplicating points previously written.
		if (sp && !sp->nonextcp && sp->next && (sp->next->to != spl->first || spl->start_offset > -2) && sp->next && !sp->next->order2) {
                          xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
                          xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)sp->nextcp.x);
                          xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)sp->nextcp.y);
		    	  // "<point x=\"%g\" y=\"%g\"/>\n" (double)sp->nextcp.x (double)sp->nextcp.y
		}
		sp = sp->next->to;
		if (sp && !sp->noprevcp && (sp != spl->first || spl->start_offset > -1)) {
                          xmlNodePtr pointxml = xmlNewChild(contourxml, NULL, BAD_CAST "point", NULL);
                          xmlSetPropPrintf(pointxml, BAD_CAST "x", "%g", (double)sp->prevcp.x);
                          xmlSetPropPrintf(pointxml, BAD_CAST "y", "%g", (double)sp->prevcp.y);
                          // "<point x=\"%g\" y=\"%g\"/>\n" (double)sp->prevcp.x (double)sp->prevcp.y
		}
		if ( sp==spl->first )
	    		break;
	    }
	    // "</contour>"
	}
	/* RoboFab outputs components in alphabetic (case sensitive) order. */
	/* Somebody asked George to do that too (as in the disabled code below). */
	/* But it seems important to leave the ordering as it is. */
	/* And David Raymond advises that tampering with the ordering can break things. */
	if ( sc->layers[layer].refs!=NULL ) {
	    const RefChar **refs;
	    int i, cnt;
	    for ( cnt=0, ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ((SCWorthOutputting(ref->sc) || SCHasData(ref->sc) || ref->sc->glif_name != NULL))
		++cnt;
	    refs = malloc(cnt*sizeof(RefChar *));
	    for ( cnt=0, ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next ) if ((SCWorthOutputting(ref->sc) || SCHasData(ref->sc) || ref->sc->glif_name != NULL))
		refs[cnt++] = ref;
	    // It seems that sorting these breaks something.
#if 0
	    if ( cnt>1 )
		qsort(refs,cnt,sizeof(RefChar *),refcomp);
#endif // 0
	    for ( i=0; i<cnt; ++i ) {
		ref = refs[i];
    xmlNodePtr componentxml = xmlNewChild(outlinexml, NULL, BAD_CAST "component", NULL);
    xmlSetPropPrintf(componentxml, BAD_CAST "base", "%s", ref->sc->name);
		// "<component base=\"%s\"" ref->sc->name
    char *floattmp = NULL;
		if ( ref->transform[0]!=1 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "xScale", "%g", (double) ref->transform[0]);
		    // "xScale=\"%g\"" (double)ref->transform[0]
                }
		if ( ref->transform[3]!=1 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "yScale", "%g", (double) ref->transform[3]);
		    // "yScale=\"%g\"" (double)ref->transform[3]
                }
		if ( ref->transform[1]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "xyScale", "%g", (double) ref->transform[1]);
		    // "xyScale=\"%g\"" (double)ref->transform[1]
                }
		if ( ref->transform[2]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "yxScale", "%g", (double) ref->transform[2]);
		    // "yxScale=\"%g\"" (double)ref->transform[2]
                }
		if ( ref->transform[4]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "xOffset", "%g", (double) ref->transform[4]);
		    // "xOffset=\"%g\"" (double)ref->transform[4]
                }
		if ( ref->transform[5]!=0 ) {
                  xmlSetPropPrintf(componentxml, BAD_CAST "yOffset", "%g", (double) ref->transform[5]);
		    // "yOffset=\"%g\"" (double)ref->transform[5]
                }
		// "/>"
	    }
	    free(refs);
	}

	// "</outline>"
    }
    if (sc->layers[layer].python_persistent != NULL || (layer == ly_fore && (sc->hstem!=NULL || sc->vstem!=NULL ))) {
      // If the layer has lib data or if this is the foreground and the glyph has hints, we output lib data.
      xmlNodePtr libxml = xmlNewChild(topglyphxml, NULL, BAD_CAST "lib", NULL);
      xmlNodePtr pythonblob = PythonLibToXML(sc->layers[layer].python_persistent, (layer == ly_fore ? sc : NULL), sc->layers[layer].python_persistent_has_lists);
      xmlAddChild(libxml, pythonblob);
    }
    return topglyphxml;
}

static int GlifDump(const char *glyphdir, const char *gfname, const SplineChar *sc, int layer, int version) {
    char *gn = buildname(glyphdir,gfname);
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    if (doc == NULL) {
        free(gn);
        return 0;
    }
    xmlNodePtr root_node = _GlifToXML(sc, layer, version);
    if (root_node == NULL) {xmlFreeDoc(doc); doc = NULL; free(gn); return 0;}
    xmlDocSetRootElement(doc, root_node);
    int ret = (xmlSaveFormatFileEnc(gn, doc, "UTF-8", 1) != -1);
    xmlFreeDoc(doc); doc = NULL;
    free(gn);
    return ret;
}

int _ExportGlif(FILE *glif,SplineChar *sc, int layer, int version) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_node = _GlifToXML(sc, layer, version);
    xmlDocSetRootElement(doc, root_node);
    int output_status = xmlDocFormatDump(glif, doc, 1);
    xmlFreeDoc(doc); doc = NULL;
    return ( output_status != -1 );
}

/* ************************************************************************** */
/* ****************************    UFO Output    **************************** */
/* ************************************************************************** */

void clear_cached_ufo_paths(SplineFont * sf) {
  // We cache the glif names and the layer paths.
  // This is helpful for preserving the structure of a U. F. O. to be edited.
  // But it may be desirable to purge that data on final output for consistency.
  // This function does that.
  int i;
  // First we clear the glif names.
  for (i = 0; i < sf->glyphcnt; i++) {
    struct splinechar * sc = sf->glyphs[i];
    if (sc->glif_name != NULL) { free(sc->glif_name); sc->glif_name = NULL; }
  }
  // Then we clear the layer names.
  for (i = 0; i < sf->layer_cnt; i++) {
    struct layerinfo * ly = &(sf->layers[i]);
    if (ly->ufo_path != NULL) { free(ly->ufo_path); ly->ufo_path = NULL; }
  }
}

void clear_cached_ufo_point_starts(SplineFont * sf) {
  // We store the offset from the leading spline point at which to start output
  // so as to be able to start curves on control points as some incoming U. F. O. files do.
  // But we may want to clear these sometimes.
  int splinechar_index;
  for (splinechar_index = 0; splinechar_index < sf->glyphcnt; splinechar_index ++) {
    struct splinechar *sc = sf->glyphs[splinechar_index];
    if (sc != NULL) {
      int layer_index;
      for (layer_index = 0; layer_index < sc->layer_cnt; layer_index ++) {
        // We look at the actual shapes for this layer.
        {
          struct splinepointlist *spl;
          for (spl = sc->layers[layer_index].splines; spl != NULL; spl = spl->next) {
            spl->start_offset = 0;
          }
        }
        // And then we go hunting for shapes in the refchars.
        {
          struct refchar *rc;
          for (rc = sc->layers[layer_index].refs; rc != NULL; rc = rc->next) {
            int reflayer_index;
            for (reflayer_index = 0; reflayer_index < rc->layer_cnt; reflayer_index ++) {
              struct splinepointlist *spl;
              for (spl = rc->layers[reflayer_index].splines; spl != NULL; spl = spl->next) {
                spl->start_offset = 0;
              }
            }
          }
        }
      }
    }
  }
  // The SplineFont also has a grid.
  {
    struct splinepointlist *spl;
    for (spl = sf->grid.splines; spl != NULL; spl = spl->next) {
      spl->start_offset = 0;
    }
  }
}

static char* fetchTTFAttribute(const SplineFont *sf, int strid) {
    char *value=NULL, *nonenglish=NULL;
    struct ttflangname *nm;

    for ( nm=sf->names; nm!=NULL; nm=nm->next ) {
		if ( nm->names[strid]!=NULL ) {
	    	nonenglish = nm->names[strid];
	    	if ( nm->lang == 0x409 ) {
				value = nm->names[strid];
    			break;
	    	}
		}
    }
    if ( value==NULL ) value=nonenglish;
    return value;
}

static int UFOOutputMetaInfo(const char *basedir, SplineFont *sf, int version) {
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.
    PListAddString(dictnode,"creator","net.GitHub.FontForge");
    PListAddInteger(dictnode,"formatVersion", version);
    char *fname = buildname(basedir, "metainfo.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static real PointsDistance(BasePoint p0, BasePoint p1) {
	return sqrt(pow((p1.x - p0.x), 2) + pow((p1.y - p0.y), 2));
}

static real AngleFromPoints(BasePoint p0, BasePoint p1) {
	return atan2(p1.y - p0.y, p1.x - p0.x);
}

static GuidelineSet *SplineToGuideline(SplineFont *sf, SplineSet *ss) {
	SplinePoint *sp1, *sp2;
	if (ss == NULL) return NULL;
	if (ss->first == NULL || ss->last == NULL || ss->first == ss->last)
	return NULL;
	GuidelineSet *gl = chunkalloc(sizeof(GuidelineSet));
	gl->point.x = (ss->first->me.x + ss->last->me.x) / 2;
	gl->point.y = (ss->first->me.y + ss->last->me.y) / 2;
	real angle_radians = AngleFromPoints(ss->first->me, ss->last->me);
	real angle_degrees = 180.0*angle_radians/acos(-1);
	gl->angle = fmod(angle_degrees, 360);
	if (ss->first->name != NULL)
		gl->name = copy(ss->first->name);
	return gl;
}

static int UFOOutputFontInfo(const char *basedir, SplineFont *sf, int layer, int version) {
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    DBounds bb;
    double test;

/* Same keys in both formats */
    PListAddString(dictnode,"familyName",sf->familyname_with_timestamp ? sf->familyname_with_timestamp : sf->familyname);
    const char *styleNameSynthetic = NULL;
    if (sf->fontname) {
        styleNameSynthetic = SFGetModifiers(sf);
	char *lastdash = strrchr(sf->fontname, (int)"-");
	if (lastdash && strlen(lastdash) > 2)
	    styleNameSynthetic = lastdash + 1;
    }
    if (styleNameSynthetic)
	    PListAddString(dictnode,"styleName",styleNameSynthetic);
    {
        char* preferredFamilyName = fetchTTFAttribute(sf,ttf_preffamilyname);
        char* preferredSubfamilyName = fetchTTFAttribute(sf,ttf_prefmodifiers);
        char* styleMapFamily = NULL;
        if (sf->styleMapFamilyName != NULL) {
            /* Empty styleMapStyleName means we imported a UFO that does not have this field. Bypass the fallback. */
            if (sf->styleMapFamilyName[0]!='\0')
                styleMapFamily = sf->styleMapFamilyName;
        } else if (preferredFamilyName != NULL && preferredSubfamilyName != NULL) {
            styleMapFamily = malloc(strlen(preferredFamilyName)+strlen(preferredSubfamilyName)+2);
            strcpy(styleMapFamily, preferredFamilyName);
            strcat(styleMapFamily, " ");
            strcat(styleMapFamily, preferredSubfamilyName);
        } else if (sf->fullname != NULL) styleMapFamily = sf->fullname;
        if (styleMapFamily != NULL) PListAddString(dictnode,"styleMapFamilyName", styleMapFamily);
    }
    {
        char* styleMapName = NULL;
        if (sf->pfminfo.stylemap != -1) {
            if (sf->pfminfo.stylemap == 0x21) styleMapName = "bold italic";
            else if (sf->pfminfo.stylemap == 0x20) styleMapName = "bold";
            else if (sf->pfminfo.stylemap == 0x01) styleMapName = "italic";
            else if (sf->pfminfo.stylemap == 0x40) styleMapName = "regular";
        } else {
            /* Figure out styleMapStyleName automatically. */
            if (sf->pfminfo.weight == 700 && sf->italicangle < 0) styleMapName = "bold italic";
            else if (sf->italicangle < 0) styleMapName = "italic";
            else if (sf->pfminfo.weight == 700) styleMapName = "bold";
            else if (sf->pfminfo.weight == 400) styleMapName = "regular";
        }
        if (styleMapName != NULL) PListAddString(dictnode,"styleMapStyleName", styleMapName);
    }
    {
      // We attempt to get numeric major and minor versions for U. F. O. out of the FontForge version string.
      int versionMajor = -1;
      int versionMinor = -1;
      if (sf->version != NULL) extractNumericVersion(sf->version, &versionMajor, &versionMinor);
      if (versionMajor >= 0) PListAddInteger(dictnode,"versionMajor", versionMajor);
      if (versionMinor >= 0) PListAddInteger(dictnode,"versionMinor", versionMinor);
    }
    PListAddNameString(dictnode,"copyright",sf,ttf_copyright);
    PListAddNameString(dictnode,"trademark",sf,ttf_trademark);
    PListAddInteger(dictnode,"unitsPerEm",sf->ascent+sf->descent);
// We decided that it would be more helpful to round-trip the U. F. O. data.
#if 0
    test = SFXHeight(sf,layer,true);
    if ( test>0 )
	PListAddInteger(dictnode,"xHeight",(int) rint(test));
    test = SFCapHeight(sf,layer,true);
    if ( test>0 )
	PListAddInteger(dictnode,"capHeight",(int) rint(test));
#else
    if (sf->pfminfo.os2_capheight) PListAddInteger(dictnode,"capHeight",sf->pfminfo.os2_capheight);
    if (sf->pfminfo.os2_xheight) PListAddInteger(dictnode,"xHeight",sf->pfminfo.os2_xheight);
#endif // 0
    if ( sf->invalidem ) {
	PListAddIntegerOrReal(dictnode,"ascender",sf->ufo_ascent);
	PListAddIntegerOrReal(dictnode,"descender",sf->ufo_descent);
    } else {
	PListAddIntegerOrReal(dictnode,"ascender",sf->ascent);
	PListAddIntegerOrReal(dictnode,"descender",-sf->descent);
    }
    PListAddReal(dictnode,"italicAngle",sf->italicangle);
    if (sf->comments) PListAddString(dictnode,"note",sf->comments);
    PListAddDate(dictnode,"openTypeHeadCreated",sf->creationtime);
    SplineFontFindBounds(sf,&bb);

    if ( sf->pfminfo.hheadascent_add )
	PListAddInteger(dictnode,"openTypeHheaAscender",bb.maxy+sf->pfminfo.hhead_ascent);
    else
	PListAddInteger(dictnode,"openTypeHheaAscender",sf->pfminfo.hhead_ascent);
    if ( sf->pfminfo.hheaddescent_add )
	PListAddInteger(dictnode,"openTypeHheaDescender",bb.miny+sf->pfminfo.hhead_descent);
    else
	PListAddInteger(dictnode,"openTypeHheaDescender",sf->pfminfo.hhead_descent);
    PListAddInteger(dictnode,"openTypeHheaLineGap",sf->pfminfo.linegap);

    PListAddNameString(dictnode,"openTypeNameDesigner",sf,ttf_designer);
    PListAddNameString(dictnode,"openTypeNameDesignerURL",sf,ttf_designerurl);
    PListAddNameString(dictnode,"openTypeNameManufacturer",sf,ttf_manufacturer);
    PListAddNameString(dictnode,"openTypeNameManufacturerURL",sf,ttf_venderurl);
    PListAddNameString(dictnode,"openTypeNameLicense",sf,ttf_license);
    PListAddNameString(dictnode,"openTypeNameLicenseURL",sf,ttf_licenseurl);
    PListAddNameString(dictnode,"openTypeNameVersion",sf,ttf_version);
    PListAddNameString(dictnode,"openTypeNameUniqueID",sf,ttf_uniqueid);
    PListAddNameString(dictnode,"openTypeNameDescription",sf,ttf_descriptor);
    PListAddNameString(dictnode,"openTypeNamePreferredFamilyName",sf,ttf_preffamilyname);
    PListAddNameString(dictnode,"openTypeNamePreferredSubfamilyName",sf,ttf_prefmodifiers);
    PListAddNameString(dictnode,"openTypeNameCompatibleFullName",sf,ttf_compatfull);
    PListAddNameString(dictnode,"openTypeNameSampleText",sf,ttf_sampletext);
    PListAddNameString(dictnode,"openTypeWWSFamilyName",sf,ttf_wwsfamily);
    PListAddNameString(dictnode,"openTypeWWSSubfamilyName",sf,ttf_wwssubfamily);
    if ( sf->pfminfo.panose_set )
	PListAddIntArray(dictnode,"openTypeOS2Panose",sf->pfminfo.panose,10);
    if ( sf->pfminfo.pfmset ) {
	char vendor[8], fc[2];
	PListAddInteger(dictnode,"openTypeOS2WidthClass",sf->pfminfo.width);
	PListAddInteger(dictnode,"openTypeOS2WeightClass",sf->pfminfo.weight);
	memcpy(vendor,sf->pfminfo.os2_vendor,4);
	vendor[4] = 0;
	PListAddString(dictnode,"openTypeOS2VendorID",vendor);
	fc[0] = sf->pfminfo.os2_family_class>>8; fc[1] = sf->pfminfo.os2_family_class&0xff;
	PListAddIntArray(dictnode,"openTypeOS2FamilyClass",fc,2);
	if ( sf->pfminfo.fstype!=-1 ) {
	    int fscnt,i;
	    char fstype[16];
	    for ( i=fscnt=0; i<16; ++i )
		if ( sf->pfminfo.fstype&(1<<i) )
		    fstype[fscnt++] = i;
	    /*
	     * Note that value 0x0 is represented by an empty bit, so in that case
	     * we output an empty array, otherwise compilers will fallback to their
	     * default fsType value.
	     */
	    PListAddIntArray(dictnode,"openTypeOS2Type",fstype,fscnt);
	}
	if ( sf->pfminfo.typoascent_add )
	    PListAddInteger(dictnode,"openTypeOS2TypoAscender",sf->ascent+sf->pfminfo.os2_typoascent);
	else
	    PListAddInteger(dictnode,"openTypeOS2TypoAscender",sf->pfminfo.os2_typoascent);
	if ( sf->pfminfo.typodescent_add )
	    PListAddInteger(dictnode,"openTypeOS2TypoDescender",-sf->descent+sf->pfminfo.os2_typodescent);
	else
	    PListAddInteger(dictnode,"openTypeOS2TypoDescender",sf->pfminfo.os2_typodescent);
	PListAddInteger(dictnode,"openTypeOS2TypoLineGap",sf->pfminfo.os2_typolinegap);
	if ( sf->pfminfo.winascent_add )
	    PListAddInteger(dictnode,"openTypeOS2WinAscent",bb.maxy+sf->pfminfo.os2_winascent);
	else
	    PListAddInteger(dictnode,"openTypeOS2WinAscent",sf->pfminfo.os2_winascent);
	if ( sf->pfminfo.windescent_add )
	    PListAddInteger(dictnode,"openTypeOS2WinDescent",-bb.miny+sf->pfminfo.os2_windescent);
	else
	    PListAddInteger(dictnode,"openTypeOS2WinDescent",sf->pfminfo.os2_windescent);
    }
    if ( sf->pfminfo.subsuper_set ) {
	PListAddInteger(dictnode,"openTypeOS2SubscriptXSize",sf->pfminfo.os2_subxsize);
	PListAddInteger(dictnode,"openTypeOS2SubscriptYSize",sf->pfminfo.os2_subysize);
	PListAddInteger(dictnode,"openTypeOS2SubscriptXOffset",sf->pfminfo.os2_subxoff);
	PListAddInteger(dictnode,"openTypeOS2SubscriptYOffset",sf->pfminfo.os2_subyoff);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptXSize",sf->pfminfo.os2_supxsize);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptYSize",sf->pfminfo.os2_supysize);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptXOffset",sf->pfminfo.os2_supxoff);
	PListAddInteger(dictnode,"openTypeOS2SuperscriptYOffset",sf->pfminfo.os2_supyoff);
	PListAddInteger(dictnode,"openTypeOS2StrikeoutSize",sf->pfminfo.os2_strikeysize);
	PListAddInteger(dictnode,"openTypeOS2StrikeoutPosition",sf->pfminfo.os2_strikeypos);
    }
    if ( sf->pfminfo.vheadset )
	PListAddInteger(dictnode,"openTypeVheaTypoLineGap",sf->pfminfo.vlinegap);
    if ( sf->pfminfo.hasunicoderanges ) {
	char ranges[128];
	int i, j, c = 0;

	for ( i = 0; i<4; i++ )
	    for ( j = 0; j<32; j++ )
		if ( sf->pfminfo.unicoderanges[i] & (1 << j) )
		    ranges[c++] = i*32+j;
	if ( c!=0 )
	    PListAddIntArray(dictnode,"openTypeOS2UnicodeRanges",ranges,c);
    }
    if ( sf->pfminfo.hascodepages ) {
	char pages[64];
	int i, j, c = 0;

	for ( i = 0; i<2; i++)
	    for ( j=0; j<32; j++ )
		if ( sf->pfminfo.codepages[i] & (1 << j) )
		    pages[c++] = i*32+j;
	if ( c!=0 )
	    PListAddIntArray(dictnode,"openTypeOS2CodePageRanges",pages,c);
    }
    if (sf->fontname)
        PListAddString(dictnode,"postscriptFontName",sf->fontname);
    if (sf->fullname)
        PListAddString(dictnode,"postscriptFullName",sf->fullname);
    if (sf->weight)
        PListAddString(dictnode,"postscriptWeightName",sf->weight);
    /* Spec defines a "postscriptSlantAngle" but I don't think postscript does*/
    /* PS does define an italicAngle, but presumably that's the general italic*/
    /* angle we output earlier */
    /* UniqueID is obsolete */
    PListAddInteger(dictnode,"postscriptUnderlineThickness",sf->uwidth);
    PListAddInteger(dictnode,"postscriptUnderlinePosition",sf->upos);
    if ( sf->private!=NULL ) {
	char *pt;
	PListAddPrivateArray(dictnode, "BlueValues", sf->private);
	PListAddPrivateArray(dictnode, "OtherBlues", sf->private);
	PListAddPrivateArray(dictnode, "FamilyBlues", sf->private);
	PListAddPrivateArray(dictnode, "FamilyOtherBlues", sf->private);
	PListAddPrivateArray(dictnode, "StemSnapH", sf->private);
	PListAddPrivateArray(dictnode, "StemSnapV", sf->private);
	PListAddPrivateThing(dictnode, "BlueFuzz", sf->private, "integer");
	PListAddPrivateThing(dictnode, "BlueShift", sf->private, "integer");
	PListAddPrivateThing(dictnode, "BlueScale", sf->private, "real");
	if ( (pt=PSDictHasEntry(sf->private,"ForceBold"))!=NULL )
	    PListAddBoolean(dictnode, "postscriptForceBold", strstr(pt,"true")!=NULL ? true : false );
    }
    if ( sf->fondname!=NULL )
    PListAddString(dictnode,"macintoshFONDName",sf->fondname);
    // If the version is 3 and the grid layer exists, emit a guidelines group.
    if (version > 2) {
        xmlNodePtr glkeynode = xmlNewChild(dictnode, NULL, BAD_CAST "key", "guidelines");
        xmlNodePtr gllistnode = xmlNewChild(dictnode, NULL, BAD_CAST "array", NULL);
        SplineSet *ss = NULL;
        for (ss = sf->grid.splines; ss != NULL; ss = ss->next) {
            // Convert to a standard guideline.
            GuidelineSet *gl = SplineToGuideline(sf, ss);
            if (gl) {
                xmlNodePtr gldictnode = xmlNewChild(gllistnode, NULL, BAD_CAST "dict", NULL);
                {
		    // These are figured from splines, so we have a synthetic set of flags.
		    int glflags = 0x10; // This indicates that we want to omit unnecessary values, which seems to be standard.
		    // We also output all coordinates if there is a non-zero coordinate.
		    if (fmod(gl->angle, 180) || !(glflags & 0x10) || gl->point.x != 0)
		        PListAddReal(gldictnode, "x", gl->point.x);
		    if (fmod(gl->angle + 90, 180) || !(glflags & 0x10) || gl->point.y != 0)
		        PListAddReal(gldictnode, "y", gl->point.y);
		    // If x and y are both present, we must add angle.
		    if (fmod(gl->angle, 90) || !(glflags & 0x10) || (gl->point.x != 0 && gl->point.y != 0))
		        PListAddReal(gldictnode, "angle", fmod(gl->angle + 360, 360));
		    if (gl->name != NULL)
		        PListAddString(gldictnode, "name", gl->name);
                }
                free(gl);
                gl = NULL;
            }
        }
    }
    // TODO: Output unrecognized data.
    char *fname = buildname(basedir, "fontinfo.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

int kernclass_for_groups_plist(struct splinefont *sf, struct kernclass *kc, int flags) {
  // Note that this is not a complete logical inverse of sister function kernclass_for_feature_file.
  return ((flags & FF_KERNCLASS_FLAG_NATIVE) ||
  (!(flags & FF_KERNCLASS_FLAG_FEATURE) && !kc->feature && (sf->preferred_kerning & 1)));
}

void ClassKerningAddExtensions(struct kernclass * target) {
  if (target->firsts_names == NULL && target->first_cnt) target->firsts_names = calloc(target->first_cnt, sizeof(char *));
  if (target->seconds_names == NULL && target->second_cnt) target->seconds_names = calloc(target->second_cnt, sizeof(char *));
  if (target->firsts_flags == NULL && target->first_cnt) target->firsts_flags = calloc(target->first_cnt, sizeof(int));
  if (target->seconds_flags == NULL && target->second_cnt) target->seconds_flags = calloc(target->second_cnt, sizeof(int));
  if (target->offsets_flags == NULL && (target->first_cnt * target->second_cnt)) target->offsets_flags = calloc(target->first_cnt * target->second_cnt, sizeof(int));
}

void UFONameKerningClasses(SplineFont *sf, int version) {
#ifdef FF_UTHASH_GLIF_NAMES
    struct glif_name_index _class_name_hash;
    struct glif_name_index * class_name_hash = &_class_name_hash; // Open the hash table.
    memset(class_name_hash, 0, sizeof(struct glif_name_index));
#else
    void * class_name_hash = NULL;
#endif
    struct kernclass *current_kernclass;
    int isv;
    int isr;
    int i;
    int absolute_index = 0; // This gives us a unique index for each kerning class.
    // First we catch the existing names.
#ifdef FF_UTHASH_GLIF_NAMES
    HashKerningClassNamesCaps(sf, class_name_hash); // Note that we use the all-caps hasher for compatibility with the official naming scheme and the following code.
#endif
    // Next we create names for the unnamed. Note that we currently avoid naming anything that might go into the feature file (since that handler currently creates its own names).
    absolute_index = 0;
    for (isv = 0; isv < 2; isv++)
    for ( current_kernclass = (isv ? sf->vkerns : sf->kerns); current_kernclass != NULL; current_kernclass = current_kernclass->next )
    for (isr = 0; isr < 2; isr++) {
      // If the special class kerning storage blocks are unallocated, we allocate them if using native U. F. O. class kerning or skip the naming otherwise.
      if ( (isr ? current_kernclass->seconds_names : current_kernclass->firsts_names) == NULL ) {
        if ( !(current_kernclass->feature) && (sf->preferred_kerning & 1) ) {
          ClassKerningAddExtensions(current_kernclass);
        } else {
          continue;
        }
      }
      for ( i=0; i< (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt); i++ )
      if ( ((isr ? current_kernclass->seconds_names[i] : current_kernclass->firsts_names[i]) == NULL) &&
        kernclass_for_groups_plist(sf, current_kernclass, (isr ? current_kernclass->seconds_flags[i] : current_kernclass->firsts_flags[i]))
        ) {
        int classflags = isr ? current_kernclass->seconds_flags[i] : current_kernclass->firsts_flags[i];
        // If the splinefont is not forcing a default group type and the group is already flagged for a feature file or to have a feature-compatible name, we give it a feature-compatible name.
        // Otherwise, we give it a native U. F. O. name.
	// TODO: Use the UFO version.
        char * startname =
          (
            (
              (
                (sf->preferred_kerning == 0) &&
                (classflags & (FF_KERNCLASS_FLAG_FEATURE | FF_KERNCLASS_FLAG_NAMETYPE))
              ) ||
              (sf->preferred_kerning & 2) || (sf->preferred_kerning & 4)
            ) ?
            (
              isv ?
              (isr ? "@MMK_B_FF" : "@MMK_A_FF") :
              (isr ? "@MMK_R_FF" : "@MMK_L_FF")
            ) :
            (
              isv ?
              (isr ? "public.vkern2.FF" : "public.vkern1.FF") :
              (isr ? "public.kern2.FF" : "public.kern1.FF")
            )
          );
        // We must flag the group as being destined for the native file if it has a non-feature-compatible name. Otherwise, we might corrupt the feature file if it ever starts using the names.
        if (startname[0] != '@') {
          if (isr) { current_kernclass->seconds_flags[i] |= FF_KERNCLASS_FLAG_NATIVE; current_kernclass->seconds_flags[i] &= ~FF_KERNCLASS_FLAG_FEATURE; }
          else { current_kernclass->firsts_flags[i] |= FF_KERNCLASS_FLAG_NATIVE; current_kernclass->firsts_flags[i] &= ~FF_KERNCLASS_FLAG_FEATURE; }
        }
        // Number the name uniquely. (Note the number-forcing flag.)
        // And add it to the hash table with its index.
        char * numberedname = ufo_name_number(class_name_hash, absolute_index + i, startname, "", "", 23);
        if (isr) current_kernclass->seconds_names[i] = numberedname;
        else current_kernclass->firsts_names[i] = numberedname;
	numberedname = NULL;
      }
      absolute_index +=i;
    }
#ifdef FF_UTHASH_GLIF_NAMES
    glif_name_hash_destroy(class_name_hash); // Close the hash table.
#endif
}

static int UFOOutputGroups(const char *basedir, SplineFont *sf, int version) {
    SplineChar *sc;
    int has_content = 0;

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    // In order to preserve ordering, we do some very icky and inefficient things.
    // First, we assign names to any unnamed kerning groups using the public.kern syntax if the splinefont is set to use native U. F. O. kerning.
    // We assign @MMK names otherwise.
    // In assigning names, we check collisions both with other kerning classes and with natively listed groups.
    UFONameKerningClasses(sf, version);

    // Once names are assigned, we start outputting the native groups, with some exceptions.
    // Since we consider kerning groups to be natively handled, any group with a kerning-style name gets checked against kerning classes.
    // If it exists, we output it and flag it as output in a temporary array. If it does not exist, we skip it.
    // When this is complete, we go through the left and right kerning classes and output any that have not been output, adding them to the native list as we do so.

    int kerning_class_count = CountKerningClasses(sf);
    char *output_done = kerning_class_count ? calloc(kerning_class_count, sizeof(char)) : NULL;

#ifdef FF_UTHASH_GLIF_NAMES
    struct glif_name_index _class_name_hash;
    struct glif_name_index * class_name_hash = &_class_name_hash; // Open the hash table.
    memset(class_name_hash, 0, sizeof(struct glif_name_index));
    HashKerningClassNames(sf, class_name_hash);
#else
    void * class_name_hash = NULL;
#endif
    struct ff_glyphclasses *current_group;
    for (current_group = sf->groups; current_group != NULL; current_group = current_group->next) {
      if (current_group->classname != NULL) {
        int grouptype = GroupNameType(current_group->classname);
        if (grouptype > 0) {
          // We skip the group if it has a corrupt feature-like name.
          if (grouptype & (GROUP_NAME_KERNING_UFO | GROUP_NAME_KERNING_FEATURE)) {
            // If the group is a kerning group, we defer to the native kerning data for existence and content.
            struct glif_name *class_name_record = glif_name_search_glif_name(class_name_hash, current_group->classname);
            if (class_name_record != NULL) {
              int absolute_index = class_name_record->gid; // This gives us a unique index for the kerning class.
              struct kernclass *kc;
              int isv;
              int isr;
              int i;
              int offset;
              if (KerningClassSeekByAbsoluteIndex(sf, absolute_index, &kc, &isv, &isr, &offset)) {
                // The group still exists.
                xmlNewChild(dictnode, NULL, BAD_CAST "key", current_group->classname);
                xmlNodePtr grouparray = xmlNewChild(dictnode, NULL, BAD_CAST "array", NULL);
                // We use the results of the preceding search in order to get the list.
                char *rawglyphlist = (
                  (isr ? kc->seconds[offset] : kc->firsts[offset])
                );
                // We need to convert from the space-delimited string to something more easily accessed on a per-item basis.
                char **glyphlist = StringExplode(rawglyphlist, ' ');
                // We will then output only those entries for which SFGetChar succeeds.
                int index = 0;
                while (glyphlist[index] != NULL) {
                  if (SFGetChar(sf, -1, glyphlist[index]))
                    xmlNewChild(grouparray, NULL, BAD_CAST "string", glyphlist[index]);
                  index++;
                }
                ExplodedStringFree(glyphlist);
                // We flag the output of this kerning class as complete.
                output_done[absolute_index] |= 1;
                has_content = 1;
              }
            }
          }
        } else {
          // If the group is not a kerning group, we just output it raw (for now).
          xmlNewChild(dictnode, NULL, BAD_CAST "key", current_group->classname);
          xmlNodePtr grouparray = xmlNewChild(dictnode, NULL, BAD_CAST "array", NULL);
          // We need to convert from the space-delimited string to something more easily accessed on a per-item basis.
          char **glyphlist = StringExplode(current_group->glyphs, ' ');
          // We will then output only those entries for which SFGetChar succeeds.
          int index = 0;
          while (glyphlist[index] != NULL) {
            if (SFGetChar(sf, -1, glyphlist[index]))
              xmlNewChild(grouparray, NULL, BAD_CAST "string", glyphlist[index]);
            index++;
          }
          ExplodedStringFree(glyphlist);
          has_content = 1;
        }
      }
    }
    {
      // Oh. But we've not finished yet. Some kerning classes may not be in the groups listing.
      struct kernclass *current_kernclass;
      int isv;
      int isr;
      int i;
      int absolute_index = 0;
      for (isv = 0; isv < 2; isv++)
      for (current_kernclass = (isv ? sf->vkerns : sf->kerns); current_kernclass != NULL; current_kernclass = current_kernclass->next)
      for (isr = 0; isr < 2; isr++)
      if (isr ? current_kernclass->seconds_names : current_kernclass->firsts_names) {
        for (i=0; i < (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt); ++i) {
          const char *classname = (isr ? current_kernclass->seconds_names[i] : current_kernclass->firsts_names[i]);
          const char *rawglyphlist = (isr ? current_kernclass->seconds[i] : current_kernclass->firsts[i]);
          int classflags = (isr ? current_kernclass->seconds_flags[i] : current_kernclass->firsts_flags[i]);
          // Note that we only output if the kernclass is destined for U. F. O. and has not already met said fate.
          if (classname != NULL && rawglyphlist != NULL &&
              !(output_done[absolute_index + i]) && kernclass_for_groups_plist(sf, current_kernclass, classflags)) {
                xmlNewChild(dictnode, NULL, BAD_CAST "key", classname);
                xmlNodePtr grouparray = xmlNewChild(dictnode, NULL, BAD_CAST "array", NULL);
                // We need to convert from the space-delimited string to something more easily accessed on a per-item basis.
                char **glyphlist = StringExplode(rawglyphlist, ' ');
                // We will then output only those entries for which SFGetChar succeeds.
                int index = 0;
                while (glyphlist[index] != NULL) {
                  if (SFGetChar(sf, -1, glyphlist[index]))
                    xmlNewChild(grouparray, NULL, BAD_CAST "string", glyphlist[index]);
                  index++;
                }
                ExplodedStringFree(glyphlist);
                // We flag the output of this kerning class as complete.
                output_done[absolute_index + i] |= 1;
                                has_content = 1;
          }
        }
        absolute_index +=i;
      }
    }

#ifdef FF_UTHASH_GLIF_NAMES
    glif_name_hash_destroy(class_name_hash); // Close the hash table.
#endif
    if (output_done != NULL) { free(output_done); output_done = NULL; }

    char *fname = buildname(basedir, "groups.plist"); // Build the file name.
    if (has_content) xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static void KerningPListAddGlyph(xmlNodePtr parent, const char *key, const KernPair *kp) {
    xmlNewChild(parent, NULL, BAD_CAST "key", BAD_CAST key); // "<key>%s</key>" key
    xmlNodePtr dictxml = xmlNewChild(parent, NULL, BAD_CAST "dict", NULL); // "<dict>"
    while ( kp!=NULL ) {
      PListAddInteger(dictxml, kp->sc->name, kp->off); // "<key>%s</key><integer>%d</integer>" kp->sc->name kp->off
      kp = kp->next;
    }
}

#ifndef FF_UTHASH_GLIF_NAMES
static int UFOOutputKerning(const char *basedir, const SplineFont *sf) {
    SplineChar *sc;
    int i;
    int has_content = 0;

    if (!(sf->preferred_kerning & 1)) return true; // This goes into the feature file by default now.

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) && sc->kerns!=NULL ) {
	KerningPListAddGlyph(dictnode,sc->name,sc->kerns);
	has_content = 1;
    }

    char *fname = buildname(basedir, "kerning.plist"); // Build the file name.
    if (has_content) xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document if it's not empty.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static int UFOOutputVKerning(const char *basedir, const SplineFont *sf) {
    SplineChar *sc;
    int i;
    int has_content = 0;

    if (!(sf->preferred_kerning & 1)) return true; // This goes into the feature file by default now.

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    for ( i=sf->glyphcnt-1; i>=0; --i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) && sc->vkerns!=NULL ) break;
    if ( i<0 ) return( true );
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL && sc->vkerns!=NULL ) {
	KerningPListAddGlyph(dictnode,sc->name,sc->vkerns);
	has_content = 1;
    }

    char *fname = buildname(basedir, "vkerning.plist"); // Build the file name.
    if (has_content) xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document if it's not empty.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

static int UFOOutputKerning2(const char *basedir, const SplineFont *sf, int isv, int version) {
  if (isv) return UFOOutputVKerning(basedir, sf);
  else return UFOOutputKerning(basedir, sf);
}

#else // FF_UTHASH_GLIF_NAMES
// New approach.
// We will build a tree.

struct ufo_kerning_tree_left;
struct ufo_kerning_tree_right;
struct ufo_kerning_tree_left {
  char *name;
  struct ufo_kerning_tree_right *first_right;
  struct ufo_kerning_tree_right *last_right;
  struct ufo_kerning_tree_left *next;
};

struct ufo_kerning_tree_right {
  char *name;
  int value;
  struct ufo_kerning_tree_right *next;
};

struct ufo_kerning_tree_session {
  struct ufo_kerning_tree_left *first_left;
  struct ufo_kerning_tree_left *last_left;
  int left_group_count;
  struct glif_name_index _left_group_name_hash;
  int class_pair_count;
  struct glif_name_index _class_pair_hash;
};

void ufo_kerning_tree_destroy_contents(struct ufo_kerning_tree_session *session) {
  struct ufo_kerning_tree_left *current_left;
  struct ufo_kerning_tree_left *next_left;
  for (current_left = session->first_left; current_left != NULL; current_left = next_left) {
    next_left = current_left->next;
    struct ufo_kerning_tree_right *current_right;
    struct ufo_kerning_tree_right *next_right;
    for (current_right = current_left->first_right; current_right != NULL; current_right = next_right) {
      next_right = current_right->next;
      if (current_right->name != NULL) free(current_right->name);
      free(current_right);
    }
    if (current_left->name != NULL) free(current_left->name);
    free(current_left);
  }
  memset(session, 0, sizeof(struct ufo_kerning_tree_session));
}

int ufo_kerning_tree_attempt_insert(struct ufo_kerning_tree_session *session, const char *left_name, const char *right_name, int value) {
  int err = 0;
  struct glif_name_index *left_group_name_hash = &(session->_left_group_name_hash);
  struct glif_name_index *class_pair_hash = &(session->_class_pair_hash);
  char *tmppairname = NULL;
  err |= (asprintf(&tmppairname, "%s %s", left_name, right_name) < 0);
  struct ufo_kerning_tree_left *first_left = NULL;
  struct ufo_kerning_tree_left *last_left = NULL;
  if (!glif_name_search_glif_name(class_pair_hash, tmppairname)) {
    struct ufo_kerning_tree_left *current_left;
    // We look for a tree node matching the left side of the pair.
    for (current_left = session->first_left; current_left != NULL &&
      (current_left->name == NULL || strcmp(current_left->name, left_name));
      current_left = current_left->next);
    // If the search fails, we make a new node.
    if (current_left == NULL) {
      current_left = calloc(1, sizeof(struct ufo_kerning_tree_left));
      current_left->name = copy(left_name);
      if (session->last_left != NULL) session->last_left->next = current_left;
      else session->first_left = current_left;
      session->last_left = current_left;
    }
    {
      // We already know from the pair hash search that this pair does not exist.
      // So we go right to the end.
      struct ufo_kerning_tree_right *current_right = calloc(1, sizeof(struct ufo_kerning_tree_right));
      current_right->name = copy(right_name);
      current_right->value = value;
      if (current_left->last_right != NULL) current_left->last_right->next = current_right;
      else current_left->first_right = current_right;
      current_left->last_right = current_right;
      char *newpairname = NULL;
      err |= (asprintf(&newpairname, "%s %s", left_name, right_name) < 0);
      glif_name_track_new(class_pair_hash, session->class_pair_count++, newpairname);
      free(newpairname); newpairname = NULL;
    }
  }
  free(tmppairname); tmppairname = NULL;
  if (err) {
    LogError(_("Error generating names in ufo_kerning_tree_attempt_insert."));
  }
  return 0;
}


static int UFOOutputKerning2(const char *basedir, SplineFont *sf, int isv, int version) {
    int i, j;
    int has_content = 0;

    if (!(sf->preferred_kerning & 1)) return true; // This goes into the feature file by default now.

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Add the dict.

    // In order to preserve ordering, we do some very icky and inefficient things.
    // First, we assign names to any unnamed kerning groups using the public.kern syntax if the splinefont is set to use native U. F. O. kerning.
    // We assign @MMK names otherwise.
    // In assigning names, we check collisions both with other kerning classes and with natively listed groups.
    UFONameKerningClasses(sf, version);

    // Once names are assigned, we start outputting the native groups, with some exceptions.
    // Since we consider kerning groups to be natively handled, any group with a kerning-style name gets checked against kerning classes.
    // If it exists, we output it and flag it as output in a temporary array. If it does not exist, we skip it.
    // When this is complete, we go through the left and right kerning classes and output any that have not been output, adding them to the native list as we do so.

    int kerning_class_count = CountKerningClasses(sf);
    int kerning_class_pair_count = kerning_class_count * kerning_class_count;
    char *output_done = kerning_class_pair_count ? calloc(kerning_class_pair_count, sizeof(char)) : NULL;

    struct ufo_kerning_tree_session _session;
    memset(&_session, 0, sizeof(struct ufo_kerning_tree_session));
    struct ufo_kerning_tree_session *session = &_session;

    struct glif_name_index _class_name_hash;
    struct glif_name_index * class_name_hash = &_class_name_hash; // Open the hash table.
    memset(class_name_hash, 0, sizeof(struct glif_name_index));
    HashKerningClassNames(sf, class_name_hash);

    // We process the raw kerning list first in order to give preference to the original ordering.
    struct ff_rawoffsets *current_groupkern;
    for (current_groupkern = (isv ? sf->groupvkerns : sf->groupkerns); current_groupkern != NULL; current_groupkern = current_groupkern->next) {
      if (current_groupkern->left != NULL && current_groupkern->right != NULL) {
        int left_grouptype = GroupNameType(current_groupkern->left);
        int right_grouptype = GroupNameType(current_groupkern->right);
        int offset = 0;
        int valid = 0;
        if (left_grouptype > 0 && right_grouptype > 0) {
          // It's a pure class look-up.
          struct glif_name *left_class_name_record = glif_name_search_glif_name(class_name_hash, current_groupkern->left);
          struct glif_name *right_class_name_record = glif_name_search_glif_name(class_name_hash, current_groupkern->right);
          if ((left_grouptype & right_grouptype & (GROUP_NAME_KERNING_UFO | GROUP_NAME_KERNING_FEATURE)) &&
            !(left_grouptype & GROUP_NAME_RIGHT) && (right_grouptype & GROUP_NAME_RIGHT) &&
            ((left_grouptype & GROUP_NAME_VERTICAL) == (isv * GROUP_NAME_VERTICAL)) &&
            ((right_grouptype & GROUP_NAME_VERTICAL) == (isv * GROUP_NAME_VERTICAL)) &&
            left_class_name_record != NULL && right_class_name_record != NULL) {
            // If the group is a kerning group, we defer to the native kerning data for existence and content.
            {
              int left_absolute_index = left_class_name_record->gid; // This gives us a unique index for the kerning class.
              int right_absolute_index = right_class_name_record->gid; // This gives us a unique index for the kerning class.
              struct kernclass *left_kc, *right_kc;
              int left_isv, right_isv;
              int left_isr, right_isr;
              int left_offset, right_offset;
              if (KerningClassSeekByAbsoluteIndex(sf, left_absolute_index, &left_kc, &left_isv, &left_isr, &left_offset) &&
                KerningClassSeekByAbsoluteIndex(sf, right_absolute_index, &right_kc, &right_isv, &right_isr, &right_offset) &&
                left_kc == right_kc) {
                offset = left_kc->offsets[left_offset*left_kc->second_cnt+right_offset];
                valid = 1;
              }
            }
          }
        } else if (left_grouptype == 0 && right_grouptype == 0) {
          // It's a plain kerning pair.
          struct splinechar *sc = SFGetChar(sf, -1, current_groupkern->left);
          struct splinechar *ssc = SFGetChar(sf, -1, current_groupkern->right);
          if (sc && ssc) {
            struct kernpair *current_kernpair = (isv ? sc->vkerns : sc->kerns);
            while (current_kernpair != NULL && current_kernpair->sc != ssc) current_kernpair = current_kernpair->next;
            if (current_kernpair != NULL && current_kernpair->sc == ssc) {
              offset = current_kernpair->off;
              valid = 1;
            }
          }
        } else if (left_grouptype == 0 && right_grouptype > 0) {
          // It's a mixed pair. FontForge does not handle these natively right now, so, if the two references are valid, we output the raw value.
          struct splinechar *sc = SFGetChar(sf, -1, current_groupkern->left);
          struct glif_name *right_class_name_record = glif_name_search_glif_name(class_name_hash, current_groupkern->right);
          if ((right_grouptype & (GROUP_NAME_KERNING_UFO | GROUP_NAME_KERNING_FEATURE)) &&
            (right_grouptype & GROUP_NAME_RIGHT) &&
            ((right_grouptype & GROUP_NAME_VERTICAL) == (isv * GROUP_NAME_VERTICAL)) &&
            right_class_name_record != NULL &&
            sc != NULL) {
            offset = current_groupkern->offset;
            valid = 1;
          }
        } else if (left_grouptype > 0 && right_grouptype == 0) {
          // It's a mixed pair. FontForge does not handle these natively right now, so, if the two references are valid, we output the raw value.
          struct splinechar *ssc = SFGetChar(sf, -1, current_groupkern->right);
          struct glif_name *left_class_name_record = glif_name_search_glif_name(class_name_hash, current_groupkern->left);
          if ((left_grouptype & (GROUP_NAME_KERNING_UFO | GROUP_NAME_KERNING_FEATURE)) &&
            !(left_grouptype & GROUP_NAME_RIGHT) &&
            ((left_grouptype & GROUP_NAME_VERTICAL) == (isv * GROUP_NAME_VERTICAL)) &&
            left_class_name_record != NULL &&
            ssc != NULL) {
            offset = current_groupkern->offset;
            valid = 1;
          }
        } else {
          // Something is wrong.
        }
        if (valid) {
          ufo_kerning_tree_attempt_insert(session, current_groupkern->left, current_groupkern->right, offset);
        }
      }
    }
    {
      // Oh. But we've not finished yet. New class kerns may not be in the original list.
      struct kernclass *kc;
      char *left_name;
      char *right_name;
      for (kc = isv ? sf->vkerns : sf->kerns; kc != NULL; kc = kc->next)
      if (kc->firsts_names && kc->seconds_names && kc->firsts_flags && kc->seconds_flags &&
        kc->offsets_flags)
      for ( i=0; i<kc->first_cnt; ++i ) if ( kc->firsts[i]!=NULL && kc->firsts_names[i]!=NULL && kernclass_for_groups_plist(sf, kc, kc->firsts_flags[i]))
        for ( j=0; j<kc->second_cnt; ++j ) if ( kc->seconds[j]!=NULL && kc->seconds_names[j]!=NULL && kernclass_for_groups_plist(sf, kc, kc->firsts_flags[j]))
          if (kernclass_for_groups_plist(sf, kc, kc->offsets_flags[i*kc->second_cnt+j]) && kc->offsets[i*kc->second_cnt+j] != 0)
            ufo_kerning_tree_attempt_insert(session, kc->firsts_names[i], kc->seconds_names[j], kc->offsets[i*kc->second_cnt+j]);
    }
    {
      // And don't forget about pair kerns.
      for ( i=0; i<sf->glyphcnt; ++i ) {
        struct splinechar *sc = sf->glyphs[i];
        struct kernpair *kp;
        if ( (SCWorthOutputting(sc) || SCHasData(sc) || sc->glif_name != NULL) && (isv ? sc->vkerns : sc->kerns ) !=NULL )
          for (kp = (isv ? sc->vkerns : sc->kerns); kp != NULL; kp = kp->next)
            if (kp->sc != NULL && sc->name != NULL && kp->sc->name != NULL)
              ufo_kerning_tree_attempt_insert(session, sc->name, kp->sc->name, kp->off);
      }
    }

    {
      // Output time has arrived.
      struct ufo_kerning_tree_left *current_left;
      for (current_left = session->first_left; current_left != NULL; current_left = current_left->next) {
        if (current_left->name != NULL) {
          xmlNewChild(dictnode, NULL, BAD_CAST "key", BAD_CAST current_left->name); // "<key>%s</key>" key
          xmlNodePtr dictxml = xmlNewChild(dictnode, NULL, BAD_CAST "dict", NULL); // "<dict>"
          struct ufo_kerning_tree_right *current_right;
          for (current_right = current_left->first_right; current_right != NULL; current_right = current_right->next)
            if (current_right->name != NULL) PListAddInteger(dictxml, current_right->name, current_right->value);
          has_content = 1;
        }
      }
    }

    glif_name_hash_destroy(class_name_hash); // Close the hash table.
    ufo_kerning_tree_destroy_contents(session);

    if (output_done != NULL) { free(output_done); output_done = NULL; }

    char *fname = buildname(basedir, (isv ? "vkerning.plist" : "kerning.plist")); // Build the file name.
    if (has_content) xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    return true;
}

#endif // FF_UTHASH_GLIF_NAMES

static int UFOOutputLib(const char *basedir, const SplineFont *sf, int version) {
#ifndef _NO_PYTHON
    if ( sf->python_persistent==NULL || PyMapping_Check(sf->python_persistent) == 0) return true;

    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) return false; // Find the root node.

    xmlNodePtr dictnode = PythonLibToXML(sf->python_persistent,NULL,sf->python_persistent_has_lists);
    xmlAddChild(rootnode, dictnode);

    char *fname = buildname(basedir, "lib.plist"); // Build the file name.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
#endif
return( true );
}

static int UFOOutputFeatures(const char *basedir, SplineFont *sf, int version) {
    char *fname = buildname(basedir,"features.fea");
    FILE *feats = fopen( fname, "w" );
    int err;

    free(fname);
    if ( feats==NULL )
return( false );
    FeatDumpFontLookups(feats,sf);
    err = ferror(feats);
    fclose(feats);
return( !err );
}

int WriteUFOLayer(const char * glyphdir, SplineFont * sf, int layer, int version) {
    xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
    xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
    xmlNodePtr dictnode = xmlNewChild(rootnode, NULL, BAD_CAST "dict", NULL); if (dictnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Make the dict.

    GFileMkDir( glyphdir );
    int i;
    SplineChar * sc;
    int err = 0;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCLWorthOutputtingOrHasData(sc=sf->glyphs[i], layer) ||
      ( layer == ly_fore && (SCWorthOutputting(sc) || SCHasData(sc) || sc->glif_name != NULL) ) ) {
        // TODO: Optionally skip rewriting an untouched glyph.
        // Do we track modified glyphs carefully enough for this?
        char * final_name;
        if (asprintf(&final_name, "%s%s%s", "", sc->glif_name, ".glif") >=0) { // Generate the final name with prefix and suffix.
		PListAddString(dictnode,sc->name,final_name); // Add the glyph to the table of contents.
		err |= !GlifDump(glyphdir,final_name,sc,layer,version);
        	free(final_name); final_name = NULL;
	} else {
		err |= 1;
	}
    }

    char *fname = buildname(glyphdir, "contents.plist"); // Build the file name for the contents.
    xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
    free(fname); fname = NULL;
    xmlFreeDoc(plistdoc); // Free the memory.
    xmlCleanupParser();
    if (err) {
	LogError(_("Error in WriteUFOLayer."));
    }
    return err;
}

int WriteUFOFontFlex(const char *basedir, SplineFont *sf, enum fontformat ff, int flags,
	const EncMap *map, int layer, int all_layers, int version) {
    char *glyphdir, *gfname;
    int err;
    FILE *plist;
    int i;
    SplineChar *sc;

    /* Clean it out, if it exists */
    if (!GFileRemove(basedir, true)) {
        LogError(_("Error clearing %s."), basedir);
    }

    /* Create it */
    if (GFileMkDir( basedir ) == -1) return false;

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.


    err  = !UFOOutputMetaInfo(basedir,sf,version);
    err |= !UFOOutputFontInfo(basedir,sf,layer,version);
    err |= !UFOOutputGroups(basedir,sf,version);
    err |= !UFOOutputKerning2(basedir,sf,0,version); // Horizontal.
    err |= !UFOOutputKerning2(basedir,sf,1,version); // Vertical.
    err |= !UFOOutputLib(basedir,sf,version);
    err |= !UFOOutputFeatures(basedir,sf,version);

    if ( err ) {
        switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
        return false;
    }

#ifdef FF_UTHASH_GLIF_NAMES
    struct glif_name_index _glif_name_hash;
    struct glif_name_index * glif_name_hash = &_glif_name_hash; // Open the hash table.
    memset(glif_name_hash, 0, sizeof(struct glif_name_index));
#else
    void * glif_name_hash = NULL;
#endif
    // First we generate glif names.
    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc=sf->glyphs[i]) || SCHasData(sf->glyphs[i]) ) {
        char * startname = NULL;
        if (sc->glif_name != NULL)
          startname = strdup(sc->glif_name); // If the splinechar has a glif name, try to use that.
        else
          startname = ufo_name_mangle(sc->name, "", ".glif", 7); // If not, call the mangler.
        // Number the name (as the specification requires) if there is a collision.
        // And add it to the hash table with its index.
        char * numberedname = ufo_name_number(glif_name_hash, i, startname, "", ".glif", 7);
        free(startname); startname = NULL;
        // We update the saved glif_name only if it is different (so as to minimize churn).
        if ((sc->glif_name != NULL) && (strcmp(sc->glif_name, numberedname) != 0)) {
          free(sc->glif_name); sc->glif_name = NULL;
	}
        if (sc->glif_name == NULL) {
          sc->glif_name = numberedname;
        } else {
	  free(numberedname);
	}
	numberedname = NULL;
    }
#ifdef FF_UTHASH_GLIF_NAMES
    glif_name_hash_destroy(glif_name_hash); // Close the hash table.
#endif
    
    if (all_layers) {
#ifdef FF_UTHASH_GLIF_NAMES
      struct glif_name_index _layer_name_hash;
      struct glif_name_index * layer_name_hash = &_layer_name_hash; // Open the hash table.
      memset(layer_name_hash, 0, sizeof(struct glif_name_index));
      struct glif_name_index _layer_path_hash;
      struct glif_name_index * layer_path_hash = &_layer_path_hash; // Open the hash table.
      memset(layer_path_hash, 0, sizeof(struct glif_name_index));
#else
      void * layer_name_hash = NULL;
#endif
      switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
      xmlDocPtr plistdoc = PlistInit(); if (plistdoc == NULL) return false; // Make the document.
      xmlNodePtr rootnode = xmlDocGetRootElement(plistdoc); if (rootnode == NULL) { xmlFreeDoc(plistdoc); return false; } // Find the root node.
      xmlNodePtr arraynode = xmlNewChild(rootnode, NULL, BAD_CAST "array", NULL); if (arraynode == NULL) { xmlFreeDoc(plistdoc); return false; } // Make the dict.
      switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
      int layer_pos;
      for (layer_pos = 0; layer_pos < sf->layer_cnt; layer_pos++) {
        // We don't want to emit the default background layer unless it has stuff in it or was in the input U. F. O..
        if (layer_pos == ly_back && !LayerWorthOutputting(sf, layer_pos) && sf->layers[layer_pos].ufo_path == NULL) continue;
        // We start building the layer contents entry.
        xmlNodePtr layernode = xmlNewChild(arraynode, NULL, BAD_CAST "array", NULL);
        // We make the layer name.
        char * layer_name_start = NULL;
        if (layer_pos == ly_fore) layer_name_start = "public.default";
        else if (layer_pos == ly_back) layer_name_start = "public.background";
        else layer_name_start = sf->layers[layer_pos].name;
        if (layer_name_start == NULL) layer_name_start = "unnamed"; // The remangle step adds any needed numbers.
        char * numberedlayername = ufo_name_number(layer_name_hash, layer_pos, layer_name_start, "", "", 7);
        // We make the layer path.
        char * layer_path_start = NULL;
        char * numberedlayerpath = NULL;
        char * numberedlayerpathwithglyphs = NULL;
	int name_err = 0;
        if (layer_pos == ly_fore) {
          numberedlayerpath = strdup("glyphs");
          name_err |= (asprintf(&numberedlayerpathwithglyphs, "%s", numberedlayerpath) < 0);
        } else if (sf->layers[layer_pos].ufo_path != NULL) {
          layer_path_start = strdup(sf->layers[layer_pos].ufo_path);
          numberedlayerpath = ufo_name_number(layer_path_hash, layer_pos, layer_path_start, "", "", 7);
          name_err |= (asprintf(&numberedlayerpathwithglyphs, "%s", numberedlayerpath) < 0);
        } else {
          layer_path_start = ufo_name_mangle(sf->layers[layer_pos].name, "glyphs.", "", 7);
          numberedlayerpath = ufo_name_number(layer_path_hash, layer_pos, layer_path_start, "glyphs.", "", 7);
          name_err |= (asprintf(&numberedlayerpathwithglyphs, "glyphs.%s", numberedlayerpath) < 0);
        }
        if (layer_path_start != NULL) { free(layer_path_start); layer_path_start = NULL; }
	if (name_err) {
          err |= name_err;
	} else {
	  // We write to the layer contents.
	  xmlNewTextChild(layernode, NULL, BAD_CAST "string", numberedlayername);
	  xmlNewTextChild(layernode, NULL, BAD_CAST "string", numberedlayerpathwithglyphs);
	  glyphdir = buildname(basedir, numberedlayerpathwithglyphs);
	  // We write the glyph directory.
	  err |= WriteUFOLayer(glyphdir, sf, layer_pos, version);
	}
        free(numberedlayername); numberedlayername = NULL;
        free(numberedlayerpath); numberedlayerpath = NULL;
        free(numberedlayerpathwithglyphs); numberedlayerpathwithglyphs = NULL;
        free(glyphdir); glyphdir = NULL;
      }
      char *fname = buildname(basedir, "layercontents.plist"); // Build the file name for the contents.
      xmlSaveFormatFileEnc(fname, plistdoc, "UTF-8", 1); // Store the document.
      free(fname); fname = NULL;
      xmlFreeDoc(plistdoc); // Free the memory.
      xmlCleanupParser();
#ifdef FF_UTHASH_GLIF_NAMES
      glif_name_hash_destroy(layer_name_hash); // Close the hash table.
      glif_name_hash_destroy(layer_path_hash); // Close the hash table.
#endif
    } else {
        glyphdir = buildname(basedir,"glyphs");
        WriteUFOLayer(glyphdir, sf, layer, version);
        free(glyphdir); glyphdir = NULL;
    }
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    return !err;
}

int SplineFontHasUFOLayerNames(SplineFont *sf) {
  if (sf == NULL || sf->layers == NULL) return 0;
  int layer_pos = 0;
  for (layer_pos = 0; layer_pos < sf->layer_cnt; layer_pos++) {
    if (sf->layers[layer_pos].ufo_path != NULL) return 1;
  }
  return 0;
}

int WriteUFOFont(const char *basedir, SplineFont *sf, enum fontformat ff, int flags,
	const EncMap *map, int layer, int version) {
  if (SplineFontHasUFOLayerNames(sf))
    return WriteUFOFontFlex(basedir, sf, ff, flags, map, layer, 1, version);
  else
    return WriteUFOFontFlex(basedir, sf, ff, flags, map, layer, 0, version);
}

/* ************************************************************************** */
/* *****************************    UFO Input    **************************** */
/* ************************************************************************** */

static char *get_thingy(FILE *file,char *buffer,char *tag) {
    int ch;
    char *pt;

    for (;;) {
	while ( (ch=getc(file))!='<' && ch!=EOF );
	if ( ch==EOF )
return( NULL );
	while ( (ch=getc(file))!=EOF && isspace(ch) );
	pt = tag;
	while ( ch==*pt || tolower(ch)==*pt ) {
	    ++pt;
	    ch = getc(file);
	}
	if ( *pt=='\0' )
    continue;
	if ( ch==EOF )
return( NULL );
	while ( isspace(ch)) ch=getc(file);
	if ( ch!='>' )
    continue;
	pt = buffer;
	while ( (ch=getc(file))!='<' && ch!=EOF && pt<buffer+1000)
	    *pt++ = ch;
	*pt = '\0';
return( buffer );
    }
}

char **NamesReadUFO(char *filename) {
    char *fn = buildname(filename,"fontinfo.plist");
    FILE *info = fopen(fn,"r");
    char buffer[1024];
    char **ret;

    free(fn);
    if ( info==NULL )
return( NULL );
    while ( get_thingy(info,buffer,"key")!=NULL ) {
	if ( strcmp(buffer,"fontName")!=0 ) {
	    if ( get_thingy(info,buffer,"string")!=NULL ) {
		ret = calloc(2,sizeof(char *));
		ret[0] = copy(buffer);
		fclose(info);
return( ret );
	    }
	    fclose(info);
return( NULL );
	}
    }
    fclose(info);
return( NULL );
}

#include <libxml/parser.h>

static int libxml_init_base() {
return( true );
}

static xmlNodePtr FindNode(xmlNodePtr kids,char *name) {
    while ( kids!=NULL ) {
	if ( xmlStrcmp(kids->name,(const xmlChar *) name)== 0 )
return( kids );
	kids = kids->next;
    }
return( NULL );
}

#ifndef _NO_PYTHON
static PyObject *XMLEntryToPython(xmlDocPtr doc, xmlNodePtr entry, int has_lists);

static PyObject *LibToPython(xmlDocPtr doc, xmlNodePtr dict, int has_lists) {
	// This function is responsible for parsing keys in dicts.
    PyObject *pydict = PyDict_New();
    PyObject *item = NULL;
    xmlNodePtr keys, temp;

	// Get the first item, then iterate through all items in the dict.
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		// See that the item is in fact a key.
		if ( xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
			// Fetch the key name, which, according to the libxml specification, is the first child of the key entry.
			char *keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
			// In a property list, the value entry is a sibling of the key entry. The value itself is a child.
			// Iterate through the following siblings (including keys (!)) until we find a text entry.
			for ( temp=keys->next; temp!=NULL; temp=temp->next ) {
				if ( xmlStrcmp(temp->name,(const xmlChar *) "text")!=0 ) break;
			}
			// Convert the X.M.L. entry into a Python object.
			item = NULL;
			if ( temp!=NULL) item = XMLEntryToPython(doc,temp,has_lists);
			if ( item!=NULL ) PyDict_SetItemString(pydict, keyname, item );
			if ( temp==NULL ) break;
			else if ( xmlStrcmp(temp->name,(const xmlChar *) "key")!=0 ) keys = temp;
			// If and only if the parsing succeeds, jump over any entries we read when searching for a text block.
			free(keyname);
		}
    }
return( pydict );
}

static PyObject *XMLEntryToPython(xmlDocPtr doc,xmlNodePtr entry, int has_lists) {
    char *contents;

    if ( xmlStrcmp(entry->name,(const xmlChar *) "true")==0 ) {
	Py_INCREF(Py_True);
return( Py_True );
    }
    if ( xmlStrcmp(entry->name,(const xmlChar *) "false")==0 ) {
	Py_INCREF(Py_False);
return( Py_False );
    }
    if ( xmlStrcmp(entry->name,(const xmlChar *) "none")==0 ) {
	Py_INCREF(Py_None);
return( Py_None );
    }

    if ( xmlStrcmp(entry->name,(const xmlChar *) "dict")==0 )
return( LibToPython(doc,entry, has_lists));
    if ( xmlStrcmp(entry->name,(const xmlChar *) "array")==0 ) {
	xmlNodePtr sub;
	int cnt;
	PyObject *ret, *item;
	/* I'm translating "Arrays" as tuples... not sure how to deal with */
	/*  actual python arrays. But these look more like tuples than arrays*/
	/*  since each item gets its own type */

	for ( cnt=0, sub=entry->children; sub!=NULL; sub=sub->next ) {
	    if ( xmlStrcmp(sub->name,(const xmlChar *) "text")==0 )
	continue;
	    ++cnt;
	}
	ret = ( has_lists ? PyList_New(cnt) : PyTuple_New(cnt) );
	for ( cnt=0, sub=entry->children; sub!=NULL; sub=sub->next ) {
	    if ( xmlStrcmp(sub->name,(const xmlChar *) "text")==0 )
	continue;
	    item = XMLEntryToPython(doc,sub,has_lists);
	    if ( item==NULL ) {
		item = Py_None;
		Py_INCREF(item);
	    }
	    if (has_lists) PyList_SetItem(ret,cnt,item);
	    else PyTuple_SetItem(ret,cnt,item);
	    ++cnt;
	}
return( ret );
    }
    if ((entry->children != NULL) && ((contents = (char *) xmlNodeListGetString(doc,entry->children,true)) != NULL) &&
       (( xmlStrcmp(entry->name,(const xmlChar *) "integer")==0 ) || ( xmlStrcmp(entry->name,(const xmlChar *) "real")==0 ) ||
       ( xmlStrcmp(entry->name,(const xmlChar *) "string")==0 ))) {
      contents = (char *) xmlNodeListGetString(doc,entry->children,true);
      if ( xmlStrcmp(entry->name,(const xmlChar *) "integer")==0 ) {
	long val = strtol(contents,NULL,0);
	free(contents);
return( Py_BuildValue("i",val));
      }
      if ( xmlStrcmp(entry->name,(const xmlChar *) "real")==0 ) {
	double val = strtod(contents,NULL);
	free(contents);
return( Py_BuildValue("d",val));
      }
      if ( xmlStrcmp(entry->name,(const xmlChar *) "string")==0 ) {
	PyObject *ret = Py_BuildValue("s",contents);
	free(contents);
return( ret );
      }
      
      free( contents );
    }
    if (has_lists) {
      // This special handler for unrecognized content depends
      // on the fact that there's no X. M. L. equivalent for the Python tuple.
      xmlBufferPtr buf = xmlBufferCreate(); // Create a buffer for dumping this portion of the tree.
      xmlNodeDump(buf, doc, entry, 0, 0); // Dump the tree into the buffer.
      const xmlChar* tmpcontent = xmlBufferContent(buf); // Get the string from the buffer.
      PyObject* ret = PyTuple_New(3); // Make a tuple in the Python tree.
      PyTuple_SetItem(ret, 0, PyBytes_FromString((const char*)entry->name)); // Store the node name.
      PyTuple_SetItem(ret, 1, PyBytes_FromString((const char*)tmpcontent)); // Store the node content.
      PyTuple_SetItem(ret, 2, Py_None);
      Py_INCREF(Py_None);
      xmlBufferFree(buf);
      return ret;
    }
    LogError(_("Unknown python type <%s> when reading UFO/GLIF lib data."), (char *) entry->name);
return( NULL );
}
#endif

static StemInfo *GlifParseHints(xmlDocPtr doc,xmlNodePtr dict,char *hinttype) {
    StemInfo *head=NULL, *last=NULL, *h;
    xmlNodePtr keys, array, kids, poswidth,temp;
    double pos, width;

    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
	    char *keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
	    int found = strcmp(keyname,hinttype)==0;
	    free(keyname);
	    if ( found ) {
		for ( array=keys->next; array!=NULL; array=array->next ) {
		    if ( xmlStrcmp(array->name,(const xmlChar *) "array")==0 )
		break;
		}
		if ( array!=NULL ) {
		    for ( kids = array->children; kids!=NULL; kids=kids->next ) {
			if ( xmlStrcmp(kids->name,(const xmlChar *) "dict")==0 ) {
			    pos = -88888888; width = 0;
			    for ( poswidth=kids->children; poswidth!=NULL; poswidth=poswidth->next ) {
				if ( xmlStrcmp(poswidth->name,(const xmlChar *) "key")==0 ) {
				    char *keyname2 = (char *) xmlNodeListGetString(doc,poswidth->children,true);
				    int ispos = strcmp(keyname2,"position")==0, iswidth = strcmp(keyname2,"width")==0;
				    double value;
				    free(keyname2);
				    for ( temp=poswidth->next; temp!=NULL; temp=temp->next ) {
					if ( xmlStrcmp(temp->name,(const xmlChar *) "text")!=0 )
				    break;
				    }
				    if ( temp!=NULL ) {
					char *valname = (char *) xmlNodeListGetString(doc,temp->children,true);
					if ( xmlStrcmp(temp->name,(const xmlChar *) "integer")==0 )
					    value = strtol(valname,NULL,10);
					else if ( xmlStrcmp(temp->name,(const xmlChar *) "real")==0 )
					    value = strtod(valname,NULL);
					else
					    ispos = iswidth = false;
					free(valname);
					if ( ispos )
					    pos = value;
					else if ( iswidth )
					    width = value;
					poswidth = temp;
				    }
				}
			    }
			    if ( pos!=-88888888 && width!=0 ) {
				h = chunkalloc(sizeof(StemInfo));
			        h->start = pos;
			        h->width = width;
			        if ( width==-20 || width==-21 )
				    h->ghost = true;
				if ( head==NULL )
				    head = last = h;
				else {
				    last->next = h;
			            last = h;
				}
			    }
			}
		    }
		}
	    }
	}
    }
return( head );
}

static AnchorPoint *UFOLoadAnchor(SplineFont *sf, SplineChar *sc, xmlNodePtr xmlAnchor, AnchorPoint **lastap) {
        xmlNodePtr points = xmlAnchor;
        char *sname = (char *) xmlGetProp(points, (xmlChar *) "name");
        if ( sname!=NULL) {

            /* make an AP and if necessary an AC */
            AnchorPoint *ap = chunkalloc(sizeof(AnchorPoint));
            AnchorClass *ac;
            char *namep = *sname=='_' ? sname + 1 : sname;
            char *xs = (char *) xmlGetProp(points, (xmlChar *) "x");
            char *ys = (char *) xmlGetProp(points, (xmlChar *) "y");
            if (xs) { ap->me.x = strtod(xs,NULL); free(xs); }
            if (ys) { ap->me.y = strtod(ys,NULL); free(ys); }

            ac = SFFindOrAddAnchorClass(sf,namep,NULL);
            if (*sname=='_')
                ap->type = ac->type==act_curs ? at_centry : at_mark;
            else
                ap->type = ac->type==act_mkmk   ? at_basemark :
                            ac->type==act_curs  ? at_cexit :
                            ac->type==act_mklg  ? at_baselig :
                                                  at_basechar;
            ap->anchor = ac;
	    if ( *lastap==NULL ) {
			// If there are no existing anchors, we point the main spline reference to this one.
			sc->anchor = ap;
	    } else {
			// If there are existing anchors, we attach to the last one.
			(*lastap)->next = ap;
	    }
	    *lastap = ap;

            free(sname);
            return ap;
        }
        return NULL;
}

static real PointsSame(BasePoint p0, BasePoint p1) {
	if (p0.x == p1.x && p0.y == p1.y)
		return 1;
	return 0;
}

static SplineSet *GuidelineToSpline(SplineFont *sf, GuidelineSet *gl) {
	SplinePoint *sp1, *sp2;
	SplineSet *ss;
	real emsize = sf->ascent+sf->descent;
	// fprintf(stderr, "Em: %g.\n", emsize);
	real angle_radians = acos(-1)*gl->angle/180;
	real x_off = emsize*cos(angle_radians);
	real y_off = emsize*sin(angle_radians);
	// fprintf(stderr, "Offsets: %g, %g.\n", x_off, y_off);
	sp1 = SplinePointCreate(gl->point.x-x_off,gl->point.y-y_off);
	if (gl->name != NULL)
		sp1->name = copy(gl->name);
	sp2 = SplinePointCreate(gl->point.x+x_off,gl->point.y+y_off);
	SplineMake(sp1,sp2,sf->grid.order2);
	ss = chunkalloc(sizeof(SplineSet));
	ss->first = sp1; ss->last = sp2;
	return ss;
}

static void *UFOLoadGuideline(SplineFont *sf, SplineChar *sc, int layer, xmlDocPtr doc, xmlNodePtr xmlGuideline, GuidelineSet **lastgl, SplinePointList **lastspl) {
	// It is easier to use the free mechanism for the guideline to clean up than to do it manually.
	// So we create one speculatively and then check whether it is valid.
	GuidelineSet *gl = chunkalloc(sizeof(GuidelineSet));
	char *xs = NULL;
	char *ys = NULL;
	char *as = NULL;
	char *colors = NULL;
	if (xmlStrcmp(xmlGuideline->name, (const xmlChar *)"guideline") == 0) {
		// Guidelines in the glif have attributes.
		gl->name = (char *) xmlGetProp(xmlGuideline, (xmlChar *) "name");
		gl->identifier = (char *) xmlGetProp(xmlGuideline, (xmlChar *) "identifier");
		xs = (char *) xmlGetProp(xmlGuideline, (xmlChar *) "x");
		ys = (char *) xmlGetProp(xmlGuideline, (xmlChar *) "y");
		as = (char *) xmlGetProp(xmlGuideline, (xmlChar *) "angle");
		colors = (char *) xmlGetProp(xmlGuideline, (xmlChar *) "color");
	} else if (xmlStrcmp(xmlGuideline->name, (const xmlChar *)"dict") == 0) {
		// fprintf(stderr, "Got global guideline definition.\n");
		// Guidelines in fontinfo.plist are in a dictionary format.
		xmlNodePtr dictnode = xmlGuideline;
		{
		    xmlNodePtr keynode = NULL;
		    for (keynode=dictnode->children; keynode!=NULL; keynode=keynode->next ) {
			if (xmlStrcmp(keynode->name, (const xmlChar *)"key") == 0) {
			    // fprintf(stderr, "Got key.\n");
			    char *keyname2 = (char *) xmlNodeListGetString(doc,keynode->children,true);
			    if (keyname2 != NULL) {
				    // Skip unstructured data.
				    xmlNodePtr valnode = NULL;
				    for ( valnode=keynode->next; valnode!=NULL; valnode=valnode->next ) {
					if ( xmlStrcmp(valnode->name,(const xmlChar *) "text")!=0 )
				    break;
				    }
				    char *valtext = valnode->children ?
					(char *) xmlNodeListGetString(doc,valnode->children,true) : NULL;
				    if (valtext != NULL) {
					if (xmlStrcmp(valnode->name, (const xmlChar *)"string") == 0) {
						// Parse strings.
						if (gl->name == NULL && strcmp(keyname2,"name") == 0)
						    gl->name = valtext;
						else if (gl->identifier == NULL && strcmp(keyname2,"identifier") == 0)
						    gl->identifier = valtext;
						else if (colors == NULL && strcmp(keyname2,"color") == 0)
						    colors = valtext;
						else {
						    // Free the temporary value if not assigned to a field.
						    free(valtext);
						    valtext = NULL;
						}
					} else if (xmlStrcmp(valnode->name, (const xmlChar *)"integer") == 0 ||
							xmlStrcmp(valnode->name, (const xmlChar *)"real") == 0 ||
							xmlStrcmp(valnode->name, (const xmlChar *)"float") == 0) {
						// Parse numbers.
						if (xs == NULL && strcmp(keyname2,"x") == 0)
						    xs = valtext;
						else if (ys == NULL && strcmp(keyname2,"y") == 0)
						    ys = valtext;
						else if (as == NULL && strcmp(keyname2,"angle") == 0)
						    as = valtext;
						else {
						    // Free the temporary value if not assigned to a field.
						    free(valtext);
						    valtext = NULL;
						}
					} else {
					    // Free the temporary value if not assigned to a field.
					    free(valtext);
					    valtext = NULL;
					}
				    }
				    free(keyname2);
				    keyname2 = NULL;
			    }
			}
		    }
		}
	}
	int what_is_defined = 0x0;
	if (xs) { gl->point.x = strtod(xs,NULL); what_is_defined |= 0x1; xmlFree(xs); xs = NULL; }
	if (ys) { gl->point.y = strtod(ys,NULL); what_is_defined |= 0x2; xmlFree(ys); ys = NULL; }
	if (as) { gl->angle = strtod(as,NULL); what_is_defined |= 0x4; xmlFree(as); as = NULL; }
	if (colors) {
		// fprintf(stderr, "Checking color.\n");
		// The color arrives as a set of 0-1-range values for RGBA in a string.
		// We need to repack those into a single 32-bit word.
		what_is_defined |= 0x20;
		gl->flags |= 0x20;
		int colori;
		off_t colorp, colorps;
		double colorv;
		for (colori = 0; colori < 4; colori++) {
			while (colors[colorp] == ' ' || colors[colorp] == ',') colorp++;
			colorps = colorp;
			while (colors[colorp] >= '0' && colors[colorp] <= '9') colorp++;
			if (colorp > colorps) {
				char *after_color = NULL;
				colorv = strtod(colors + colorps, &after_color);
				if (after_color != colors + colorp)
					LogError(_("Error parsing color component.\n"));
				gl->color |= (((uint32)(colorv * 255.0)) << (8 * (4 - colori)));
			} else {
				LogError(_("Missing color component.\n"));
			}
		}
		xmlFree(colors);
		colors = NULL;
	}
	// fprintf(stderr, "Checking validity.\n");
	// fprintf(stderr, "Definition flags: %x.\n", what_is_defined);
	// fprintf(stderr, "x: %g, y: %f, angle: %f.\n", gl->point.x, gl->point.y, gl->angle);
	if (
		(fmod(gl->angle, 180) && !(what_is_defined & 0x1)) || // Non-horizontal guideline without x.
		(fmod(gl->angle + 90, 180) && !(what_is_defined & 0x2)) || // Non-vertical guideline without y.
		isnan(gl->point.x) || isnan(gl->point.y) || isnan(gl->angle) ||
		isinf(gl->point.x) || isinf(gl->point.y) || isinf(gl->angle)
	) {
		// Invalid data; abort.
		// fprintf(stderr, "Invalid guideline.\n");
		LogError(_("Invalid guideline.\n"));
		GuidelineSetFree(gl);
		gl = NULL;
		return NULL;
	}
	// If the guideline is valid but not all values are defined, we flag it as using abbreviated syntax.
	if (what_is_defined & 7 != 7)
		gl->flags |= 0x10;
	// fprintf(stderr, "Setting reference.\n");
	if (sc) {
		if ( *lastgl==NULL ) {
			// If there are no existing guidelines, we point the main reference to this one.
			sc->layers[layer].guidelines = gl;
		} else {
			// If there are existing anchors, we attach to the last one.
			(*lastgl)->next = gl;
		}
		*lastgl = gl;
		return (void *)gl;
	} else {
		// fprintf(stderr, "Creating spline for guideline.");
		// Convert from a guideline to a spline.
		SplineSet *spl = GuidelineToSpline(sf, gl);
		// Free the guideline.
		GuidelineSetFree(gl);
		// Attach the spline.
		if ( *lastspl==NULL ) {
			// fprintf(stderr, "First guideline spline.");
			// If there are no existing guidelines, we point the main reference to this one.
			sf->grid.splines = spl;
		} else {
			// fprintf(stderr, "Not first guideline spline.");
			// If there are existing anchors, we attach to the last one.
			(*lastspl)->next = spl;
		}
		*lastspl = spl;
		return (void *)spl;
	}
}

static void UFOLoadGuidelines(SplineFont *sf, SplineChar *sc, int layer, xmlDocPtr doc, xmlNodePtr xmlGuidelines, GuidelineSet **lastgl, SplinePointList **lastspl) {
	if (xmlStrcmp(xmlGuidelines->name, (const xmlChar *)"array") == 0) {
		// fprintf(stderr, "Got global guidelines array.\n");
		// Guidelines in fontinfo.plist are in a dictionary format.
		xmlNodePtr array = xmlGuidelines;
		if ( array!=NULL ) {
			xmlNodePtr dictnode = NULL;
			for ( dictnode = array->children; dictnode != NULL; dictnode = dictnode->next )
				if (xmlStrcmp(dictnode->name, (const xmlChar *)"dict") == 0)
					if (!UFOLoadGuideline(sf, sc, layer, doc, dictnode, lastgl, lastspl))
						LogError(_("Failed to read guideline."));
		}
	}
}

static SplineChar *_UFOLoadGlyph(SplineFont *sf, xmlDocPtr doc, char *glifname, char* glyphname, SplineChar* existingglyph, int layerdest) {
    xmlNodePtr glyph, kids, contour, points;
    SplineChar *sc;
    xmlChar *format, *width, *height, *u;
    char *name, *tmpname;
    int uni;
    char *cpt;
    int newsc = 0;

    glyph = xmlDocGetRootElement(doc);
    format = xmlGetProp(glyph,(xmlChar *) "format");
    if ( xmlStrcmp(glyph->name,(const xmlChar *) "glyph")!=0 ||
	    (format!=NULL && xmlStrcmp(format,(xmlChar *) "1")!=0 && xmlStrcmp(format,(xmlChar *) "2")!=0)) {
		LogError(_("Expected glyph file with format==1 or 2"));
		xmlFreeDoc(doc);
		free(format);
		return( NULL );
    }
	free(format);
	tmpname = (char *) xmlGetProp(glyph,(xmlChar *) "name");
	if (glyphname != NULL) {
		// We use the provided name from the glyph listing since the specification says to trust that one more.
		name = copy(glyphname);
		// But we still fetch the internally listed name for verification and fail on a mismatch.
		if ((name == NULL) || ((name != NULL) && (tmpname != NULL) && (strcmp(glyphname, name) != 0))) {
			LogError(_("Bad glyph name."));
			if ( tmpname != NULL ) { free(tmpname); tmpname = NULL; }
			if ( name != NULL ) { free(name); name = NULL; }
			xmlFreeDoc(doc);
			return NULL;
		}
		if ( tmpname != NULL ) { free(tmpname); tmpname = NULL; }
	} else {
		name = tmpname;
	}
    if ( name==NULL && glifname!=NULL ) {
		char *pt = strrchr(glifname,'/');
		name = copy(pt+1);
		for ( pt=cpt=name; *cpt!='\0'; ++cpt ) {
			if ( *cpt!='_' )
			*pt++ = *cpt;
			else if ( islower(*name))
			*name = toupper(*name);
		}
		*pt = '\0';
    } else if ( name==NULL )
		name = copy("nameless");
	// We assign a placeholder name if no name exists.
	// We create a new SplineChar 
	if (existingglyph != NULL) {
		sc = existingglyph;
		free(name); name = NULL;
	} else {
    	sc = SplineCharCreate(2);
    	sc->name = name;
		newsc = 1;
	}
	if (sc == NULL) {
		xmlFreeDoc(doc);
		return NULL;
	}

	// Check layer availability here.
	if ( layerdest>=sc->layer_cnt ) {
		sc->layers = realloc(sc->layers,(layerdest+1)*sizeof(Layer));
		memset(sc->layers+sc->layer_cnt,0,(layerdest+1-sc->layer_cnt)*sizeof(Layer));
		sc->layer_cnt = layerdest + 1;
	}
	if (sc->layers == NULL) {
		if ((newsc == 1) && (sc != NULL)) {
			SplineCharFree(sc);
		}
		xmlFreeDoc(doc);
		return NULL;
	}

    // We track the last splineset so that we can add to the end of the chain.
    SplineSet *last = sc->layers[layerdest].splines;
    while (last != NULL && last->next != NULL) last = last->next;
    // We track the last anchor point.
    AnchorPoint *lastap = sc->anchor;
    while (lastap != NULL && lastap->next != NULL) lastap = lastap->next;
    // We track the last guideline.
    GuidelineSet *lastgl = sc->layers[layerdest].guidelines;
    while (lastgl != NULL && lastgl->next != NULL) lastgl = lastgl->next;
    // We track the last reference.
    RefChar *lastref = sc->layers[layerdest].refs;
    while (lastref != NULL && lastref->next != NULL) lastref = lastref->next;
    for ( kids = glyph->children; kids!=NULL; kids=kids->next ) {
	if ( xmlStrcmp(kids->name,(const xmlChar *) "advance")==0 ) {
		if ((layerdest == ly_fore) || newsc) {
			width = xmlGetProp(kids,(xmlChar *) "width");
			height = xmlGetProp(kids,(xmlChar *) "height");
			if ( width!=NULL )
			sc->width = strtol((char *) width,NULL,10);
			if ( height!=NULL )
			sc->vwidth = strtol((char *) height,NULL,10);
			sc->widthset = true;
			free(width); free(height);
		}
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "unicode")==0 ) {
		if ((layerdest == ly_fore) || newsc) {
			u = xmlGetProp(kids,(xmlChar *) "hex");
			uni = strtol((char *) u,NULL,16);
			if ( sc->unicodeenc == -1 )
			sc->unicodeenc = uni;
			else
			AltUniAdd(sc,uni);
			free(u);
		}
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "note")==0 ) {
		char *tval = xmlNodeListGetString(doc,kids->children,true);
		if (tval != NULL) {
			sc->comment = copy(tval);
			free(tval);
			tval = NULL;
		}
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "anchor")==0 ){
		if (UFOLoadAnchor(sf, sc, kids, &lastap))
			continue;
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "guideline")==0 ){
		if (UFOLoadGuideline(sf, sc, layerdest, doc, kids, &lastgl, NULL))
			continue;
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "outline")==0 ) {
	    for ( contour = kids->children; contour!=NULL; contour=contour->next ) {
		if ( xmlStrcmp(contour->name,(const xmlChar *) "component")==0 ) {
		    // We have a reference.
		    char *base = (char *) xmlGetProp(contour,(xmlChar *) "base"),
			*xs = (char *) xmlGetProp(contour,(xmlChar *) "xScale"),
			*ys = (char *) xmlGetProp(contour,(xmlChar *) "yScale"),
			*xys = (char *) xmlGetProp(contour,(xmlChar *) "xyScale"),
			*yxs = (char *) xmlGetProp(contour,(xmlChar *) "yxScale"),
			*xo = (char *) xmlGetProp(contour,(xmlChar *) "xOffset"),
			*yo = (char *) xmlGetProp(contour,(xmlChar *) "yOffset");
		    RefChar *r;
		    if ( base==NULL || strcmp(base,"") == 0 )
				LogError(_("component with no base glyph"));
		    else {
				r = RefCharCreate();
				r->sc = SplineCharCreate(0);
				r->sc->name = base;
				r->transform[0] = r->transform[3] = 1;
				if ( xs!=NULL )
					r->transform[0] = strtod(xs,NULL);
				if ( ys!=NULL )
					r->transform[3] = strtod(ys,NULL);
				if ( xys!=NULL )
					r->transform[1] = strtod(xys,NULL);
				if ( yxs!=NULL )
					r->transform[2] = strtod(yxs,NULL);
				if ( xo!=NULL )
					r->transform[4] = strtod(xo,NULL);
				if ( yo!=NULL )
					r->transform[5] = strtod(yo,NULL);

				if ( lastref==NULL ) {
				  // If there are no existing references, we point the main spline reference to this one.
				  sc->layers[layerdest].refs = r;
				} else {
				  // If there are existing references, we attach to the last one.
				  lastref->next = r;
				}
				lastref = r;
		    }
		    if (xs) free(xs); if (ys) free(ys); if (xys) free(xys); if (yxs) free(yxs); if (xo) free(xo); if (yo) free(yo);
		} else if ( xmlStrcmp(contour->name,(const xmlChar *) "contour")==0 ) {
		    xmlNodePtr npoints;

			// We now look for anchor points.
            char *sname;

            for ( points=contour->children; points!=NULL; points=points->next )
                if ( xmlStrcmp(points->name,(const xmlChar *) "point")==0 )
            break;
            for ( npoints=points->next; npoints!=NULL; npoints=npoints->next )
                if ( xmlStrcmp(npoints->name,(const xmlChar *) "point")==0 )
            break;
			// If the contour has a single point without another point after it, we assume it to be an anchor point.
            if ( points!=NULL && npoints==NULL ) {
              if (UFOLoadAnchor(sf, sc, points, &lastap))
                continue; // We stop processing the contour at this point.
            }

			// If we have not identified the contour as holding an anchor point, we continue processing it as a rendered shape.
			SplineSet *ss;
			SplinePoint *sp;
			SplinePoint *sp2;
			BasePoint pre[2], init[4];
			int precnt=0, initcnt=0, open=0;
			// precnt seems to count control points leading into the next on-curve point. pre stores those points.
			// initcnt counts the control points that appear before the first on-curve point. This can get updated at the beginning and/or the end of the list.
			// This is important for determining the order of the closing curve.
			// A further improvement would be to prefetch the entire list so as to know the declared order of a curve before processing the point.

			int wasquad = -1; // This tracks whether we identified the previous curve as quadratic. (-1 means undefined.)
			int firstpointsaidquad = -1; // This tracks the declared order of the curve leading into the first on-curve point.

		    ss = chunkalloc(sizeof(SplineSet));
			ss->first = NULL;

		    for ( points = contour->children; points!=NULL; points=points->next ) {
			char *xs, *ys, *type, *pname, *smooths;
			double x,y;
			int smooth = 0;
			// We discard any entities in the splineset that are not points.
			if ( xmlStrcmp(points->name,(const xmlChar *) "point")!=0 )
		    continue;
			// Read as strings from xml.
			xs = (char *) xmlGetProp(points,(xmlChar *) "x");
			ys = (char *) xmlGetProp(points,(xmlChar *) "y");
			type = (char *) xmlGetProp(points,(xmlChar *) "type");
			pname = (char *) xmlGetProp(points,(xmlChar *) "name");
			smooths = (char *) xmlGetProp(points,(xmlChar *) "smooth");
			if (smooths != NULL) {
				if (strcmp(smooths,"yes") == 0) smooth = 1;
				free(smooths); smooths=NULL;
			}
			if ( xs==NULL || ys == NULL ) {
				if (xs != NULL) { free(xs); xs = NULL; }
				if (ys != NULL) { free(ys); ys = NULL; }
				if (type != NULL) { free(type); type = NULL; }
				if (pname != NULL) { free(pname); pname = NULL; }
		    	continue;
			}
			x = strtod(xs,NULL); y = strtod(ys,NULL);
			if ( type!=NULL && (strcmp(type,"move")==0 ||
					    strcmp(type,"line")==0 ||
					    strcmp(type,"curve")==0 ||
					    strcmp(type,"qcurve")==0 )) {
				// This handles only actual points.
				// We create and label the point.
			    sp = SplinePointCreate(x,y);
				sp->dontinterpolate = 1;
				if (pname != NULL) {
					sp->name = copy(pname);
				}
				if (smooth == 1) sp->pointtype = pt_curve;
				else sp->pointtype = pt_corner;

			    if ( ss->first==NULL ) {
			        // So this is the first real point!
			        ss->first = ss->last = sp;
			        // We move the lead-in points to the init buffer as we may need them for the final curve.
			        memcpy(init,pre,sizeof(pre));
			        initcnt = precnt;
			        if ( strcmp(type,"move")==0 ) {
			          open = true;
			          if (initcnt != 0) LogError(_("We cannot have lead-in points for an open curve.\n"));
			        }
			    }

			    if ( strcmp(type,"move")==0 ) {
			        if (ss->first != sp) {
			          LogError(_("The move point must be at the beginning of the contour.\n"));
			          SplinePointFree(sp); sp = NULL;
			        }
			    } else if ( strcmp(type,"line")==0 ) {
				SplineMake(ss->last,sp,false);
			        ss->last = sp;
			    } else if ( strcmp(type,"curve")==0 ) {
				wasquad = false;
				if (ss->first == sp) {
				  firstpointsaidquad = false;
				}
				if ( precnt==2 && ss->first != sp ) {
				    ss->last->nextcp = pre[0];
				    ss->last->nonextcp = false;
				    sp->prevcp = pre[1];
				    sp->noprevcp = false;
				    SplineMake(ss->last,sp,false);
				}
			        ss->last = sp;
			    } else if ( strcmp(type,"qcurve")==0 ) {
					wasquad = true;
				if (ss->first == sp) {
				  firstpointsaidquad = true;
				}
					if ( precnt>0 && precnt<=2 ) {
						if ( precnt==2 ) {
							// If we have two cached control points and the end point is quadratic, we need an implied point between the two control points.
							sp2 = SplinePointCreate((pre[1].x+pre[0].x)/2,(pre[1].y+pre[0].y)/2);
							sp2->prevcp = ss->last->nextcp = pre[0];
							sp2->noprevcp = ss->last->nonextcp = false;
							sp2->ttfindex = 0xffff;
							SplineMake(ss->last,sp2,true);
							ss->last = sp2;
						}
						// Now we connect the real point.
						sp->prevcp = ss->last->nextcp = pre[precnt-1];
						sp->noprevcp = ss->last->nonextcp = false;
					}
					SplineMake(ss->last,sp,true);
					ss->last = sp;
			    } else {
			        SplinePointFree(sp); sp = NULL;
			    }
			    precnt = 0;
			} else {
				// This handles off-curve points (control points).
			    if ((wasquad == true || wasquad==-1) && precnt==2 ) {
				// We don't know whether the current curve is quadratic or cubic, but, if we're hitting three off-curve points in a row, something is off.
				// As mentioned below, we assume in this case that we're dealing with a quadratic TrueType curve that needs implied points.
				// We create those points since they are adjustable in Fontforge.
				// There is not a valid case as far as Frank knows in which a cubic curve would have implied points.
				/* Undocumented fact: If there are no on-curve points (and therefore no indication of quadratic/cubic), assume truetype implied points */
					// We make the point between the two already cached control points.
					sp = SplinePointCreate((pre[1].x+pre[0].x)/2,(pre[1].y+pre[0].y)/2);
					sp->ttfindex = 0xffff;
					if (pname != NULL) {
						sp->name = copy(pname);
					}
			        sp->nextcp = pre[1];
			        sp->nonextcp = false;
			        if ( ss->first==NULL ) {
				    // This is indeed possible if the first three points are control points.
				    ss->first = sp;
				    memcpy(init,pre,sizeof(pre));
				    initcnt = 1;
				} else {
				    ss->last->nextcp = sp->prevcp = pre[0];
				    ss->last->nonextcp = sp->noprevcp = false;
				    initcnt = 0;
				    SplineMake(ss->last,sp,true);
				}
			        ss->last = sp;
#if 1
			        // We make the point between the previously cached control point and the new control point.
			        // We have decided that the curve is quadratic, so we can make the next implied point as well.
			        sp = SplinePointCreate((x+pre[1].x)/2,(y+pre[1].y)/2);
			        sp->prevcp = pre[1];
			        sp->noprevcp = false;
					sp->ttfindex = 0xffff;
			        SplineMake(ss->last,sp,true);
			        ss->last = sp;
			        pre[0].x = x; pre[0].y = y;
			        precnt = 1;
#else
			        // Let us instead save the second implied point for later.
			        pre[0].x = pre[1].x; pre[0].y = pre[1].y;
			        pre[1].x = x; pre[1].y = y;
			        precnt = 2;
#endif
					wasquad = true;
			    } else if ( wasquad==true && precnt==1) {
					// Frank thinks that this might generate false positives for qcurves.
					// This seems not to be the best way to handle it, but mixed-order spline sets are rare.
					sp = SplinePointCreate((x+pre[0].x)/2,(y+pre[0].y)/2);
					if (pname != NULL) {
						sp->name = copy(pname);
					}
			        sp->prevcp = pre[0];
			        sp->noprevcp = false;
					sp->ttfindex = 0xffff;
			        if ( ss->first==NULL ) {
				    	ss->first = sp;
			            memcpy(init,pre,sizeof(pre));
			            initcnt = 1;
					} else {
					    ss->last->nextcp = sp->prevcp;
			            ss->last->nonextcp = false;
				    	SplineMake(ss->last,sp,true);
					}
					ss->last = sp;
			        pre[0].x = x; pre[0].y = y;
			    } else if ( precnt<2 ) {
					pre[precnt].x = x;
			        pre[precnt].y = y;
			        ++precnt;
			    }
			}
                        if (xs != NULL) { free(xs); xs = NULL; }
                        if (ys != NULL) { free(ys); ys = NULL; }
                        if (type != NULL) { free(type); type = NULL; }
                        if (pname != NULL) { free(pname); pname = NULL; }
		    }
		    // We are finished looping, so it's time to close the curve if it is to be closed.
		    if ( !open && ss->first != NULL ) {
			ss->start_offset = -initcnt;
			// init has a list of control points leading into the first point. pre has a list of control points trailing the last processed on-curve point.
			// We merge pre into init and use init as the list of control points between the last processed on-curve point and the first on-curve point.
			if ( precnt!=0 ) {
			    BasePoint temp[2];
			    memcpy(temp,init,sizeof(temp));
			    memcpy(init,pre,sizeof(pre));
			    memcpy(init+precnt,temp,sizeof(temp));
			    initcnt += precnt;
			}
			if ( ((firstpointsaidquad==true || (firstpointsaidquad == -1 && wasquad == true)) && initcnt>0) || initcnt==1 ) {
				// If the final curve is declared quadratic or is assumed to be by control point count, we proceed accordingly.
			    int i;
			    for ( i=0; i<initcnt-1; ++i ) {
					// If the final curve is declared quadratic but has more than one control point, we add implied points.
					sp = SplinePointCreate((init[i+1].x+init[i].x)/2,(init[i+1].y+init[i].y)/2);
			        sp->prevcp = ss->last->nextcp = init[i];
			        sp->noprevcp = ss->last->nonextcp = false;
					sp->ttfindex = 0xffff;
			        SplineMake(ss->last,sp,true);
			        ss->last = sp;
			    }
			    ss->last->nextcp = ss->first->prevcp = init[initcnt-1];
			    ss->last->nonextcp = ss->first->noprevcp = false;
			    wasquad = true;
			} else if ( initcnt==2 ) {
			    ss->last->nextcp = init[0];
			    ss->first->prevcp = init[1];
			    ss->last->nonextcp = ss->first->noprevcp = false;
				wasquad = false;
			}
			SplineMake(ss->last, ss->first, (firstpointsaidquad==true || (firstpointsaidquad == -1 && wasquad == true)));
			ss->last = ss->first;
		    }
		    if (ss->first == NULL) {
				LogError(_("This spline set has no points.\n"));
				SplinePointListFree(ss); ss = NULL;
		    } else {
		        if ( last==NULL ) {
				// If there are no existing spline sets, we point the main spline reference to this set.
				sc->layers[layerdest].splines = ss;
		        } else {
				// If there are existing spline sets, we attach to the last one.
				last->next = ss;
		        }
				last = ss;
		    }
		    }
	    }
	} else if ( xmlStrcmp(kids->name,(const xmlChar *) "lib")==0 ) {
	    xmlNodePtr keys, temp, dict = FindNode(kids->children,"dict");
	    if ( dict!=NULL ) {
		for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		    if ( xmlStrcmp(keys->name,(const xmlChar *) "key")== 0 ) {
				char *keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
				if ( strcmp(keyname,"com.fontlab.hintData")==0 ) {
			    	for ( temp=keys->next; temp!=NULL; temp=temp->next ) {
						if ( xmlStrcmp(temp->name,(const xmlChar *) "dict")==0 )
						    break;
			    	}
			    	if ( temp!=NULL ) {
						if (layerdest == ly_fore) {
							if (sc->hstem == NULL) {
								sc->hstem = GlifParseHints(doc,temp,"hhints");
								SCGuessHHintInstancesList(sc,ly_fore);
							}
							if (sc->vstem == NULL) {
								sc->vstem = GlifParseHints(doc,temp,"vhints");
			        			SCGuessVHintInstancesList(sc,ly_fore);
			        		}
						}
			    	}
					break;
				}
				free(keyname);
		    }
		}
#ifndef _NO_PYTHON
		if (sc->layers[layerdest].python_persistent == NULL) {
		  sc->layers[layerdest].python_persistent = LibToPython(doc,dict,1);
		  sc->layers[layerdest].python_persistent_has_lists = 1;
		} else LogError(_("Duplicate lib data.\n"));
#endif
	    }
	}
    }
    xmlFreeDoc(doc);
    SPLCategorizePointsKeepCorners(sc->layers[layerdest].splines);
return( sc );
}

static SplineChar *UFOLoadGlyph(SplineFont *sf,char *glifname, char* glyphname, SplineChar* existingglyph, int layerdest) {
    xmlDocPtr doc;

    doc = xmlParseFile(glifname);
    if ( doc==NULL ) {
	LogError(_("Bad glif file %s"), glifname);
return( NULL );
    }
return( _UFOLoadGlyph(sf,doc,glifname,glyphname,existingglyph,layerdest));
}


static void UFORefFixup(SplineFont *sf, SplineChar *sc, int layer ) {
    RefChar *r, *prev;
    SplineChar *rsc;

    if ( sc==NULL || sc->ticked )
		return;
    sc->ticked = true;
    prev = NULL;
	// For each reference, attempt to locate the real splinechar matching the name stored in the fake splinechar.
	// Free the fake splinechar afterwards.
    r=sc->layers[layer].refs;
    while ( r!=NULL ) {
		if (r->sc->name == NULL || strcmp(r->sc->name, "") == 0) {
			LogError(_("There's a reference to a glyph with no name."));
			prev = r; r = r->next; continue;
		}
		if (r->sc->ticked) {
		  // We've already fixed this one.
		  prev = r; r = r->next; continue;
		}
		rsc = SFGetChar(sf,-1, r->sc->name);
		if ( rsc==NULL || rsc->name == NULL || strcmp(rsc->name,"") == 0 ) {
			if (rsc != NULL) {
			  LogError(_("Invalid glyph for %s when fixing up references."), r->sc->name);
			} else
			LogError(_("Failed to find glyph %s when fixing up references."), r->sc->name);
			SplineCharFree(r->sc); // Delete the fake glyph.
			r->sc = NULL;
			// Elide r from the list and free it.
			if ( prev==NULL ) sc->layers[layer].refs = r->next;
			else prev->next = r->next;
			RefCharFree(r);
			if ( prev==NULL ) r = sc->layers[layer].refs;
			else r = prev->next;
		} else {
			UFORefFixup(sf,rsc, layer);
			if (r->sc->layer_cnt > 0) {
			  fprintf(stderr, "Danger!\n");
			}
			SplineCharFree(r->sc);
			r->sc = rsc;
			SCReinstanciateRefChar(sc,r,layer);
			prev = r; r = r->next;
		}
    }
}

static void UFOLoadGlyphs(SplineFont *sf,char *glyphdir, int layerdest) {
    char *glyphlist = buildname(glyphdir,"contents.plist");
    xmlDocPtr doc;
    xmlNodePtr plist, dict, keys, value;
    char *valname, *glyphfname;
    int i;
    SplineChar *sc;
    int tot;

    doc = xmlParseFile(glyphlist);
    free(glyphlist);
    if ( doc==NULL ) {
	LogError(_("Bad contents.plist"));
return;
    }
    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict == NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return;
    }
	// Count glyphs for the benefit of measuring progress.
    for ( tot=0, keys=dict->children; keys!=NULL; keys=keys->next ) {
		if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 )
		    ++tot;
    }
    ff_progress_change_total(tot);
	// Start reading in glyph name to file name mappings.
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
		for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
			value = value->next );
		if ( value==NULL )
			break;
		if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
			char * glyphname = (char *) xmlNodeListGetString(doc,keys->children,true);
			int newsc = 0;
			SplineChar* existingglyph = NULL;
			if (glyphname != NULL) {
				existingglyph = SFGetChar(sf,-1,glyphname);
				if (existingglyph == NULL) newsc = 1;
				valname = (char *) xmlNodeListGetString(doc,value->children,true);
				glyphfname = buildname(glyphdir,valname);
				sc = UFOLoadGlyph(sf, glyphfname, glyphname, existingglyph, layerdest);
				// We want to stash the glif name (minus the extension) for future use.
				if (sc != NULL && sc->glif_name == NULL && valname != NULL) {
				  char * tmppos = strrchr(valname, '.'); if (tmppos) *tmppos = '\0';
				  sc->glif_name = copy(valname);
				  if (tmppos) *tmppos = '.';
				}
				free(valname);
				if ( ( sc!=NULL ) && newsc ) {
					sc->parent = sf;
					if ( sf->glyphcnt>=sf->glyphmax )
						sf->glyphs = realloc(sf->glyphs,(sf->glyphmax+=100)*sizeof(SplineChar *));
					sc->orig_pos = sf->glyphcnt;
					sf->glyphs[sf->glyphcnt++] = sc;
				}
			}
			keys = value;
			ff_progress_next();
		}
    }
    xmlFreeDoc(doc);

    GlyphHashFree(sf);
    for ( i=0; i<sf->glyphcnt; ++i )
	UFORefFixup(sf,sf->glyphs[i], layerdest);
}

static struct ff_glyphclasses *GlyphGroupDeduplicate(struct ff_glyphclasses *group_base, struct splinefont *sf, int check_kerns) {
  // This removes internal duplicates from the specified group and also, if desired, the groups duplicating entities already named as kerning classes.
  // It takes the list head as its argument and returns the new list head (which may be the same unless the first item duplicates a kerning class).
  int temp_index = 0;
#ifdef FF_UTHASH_GLIF_NAMES
  struct glif_name_index _group_name_hash;
  struct glif_name_index * group_name_hash = &_group_name_hash; // Open the group hash table.
  memset(group_name_hash, 0, sizeof(struct glif_name_index));
  struct glif_name_index _class_name_hash;
  struct glif_name_index * class_name_hash = &_class_name_hash; // Open the class hash table.
  memset(class_name_hash, 0, sizeof(struct glif_name_index));
  if (check_kerns && sf) HashKerningClassNames(sf, class_name_hash);
  struct ff_glyphclasses *group_current = group_base;
  struct ff_glyphclasses *group_prev = NULL;
  while (group_current != NULL) {
    if (group_current->classname == NULL || group_current->classname[0] == '\0' ||
      glif_name_search_glif_name(group_name_hash, group_current->classname) ||
      (check_kerns && sf && glif_name_search_glif_name(class_name_hash, group_current->classname))) {
      if (group_prev != NULL) group_prev->next = group_current->next;
      else group_base = group_current->next;
      GlyphGroupFree(group_current);
      if (group_prev != NULL) group_current = group_prev->next;
      else group_current = group_base;
    } else {
      glif_name_track_new(group_name_hash, temp_index++, group_current->classname);
      group_prev = group_current; group_current = group_current->next;
    }
  }
  glif_name_hash_destroy(class_name_hash);
  glif_name_hash_destroy(group_name_hash);
#endif
  return group_base;
}

static uint32 script_from_glyph_list(SplineFont *sf, const char *glyph_names) {
  uint32 script = DEFAULT_SCRIPT;
  char *delimited_names;
  off_t name_char_pos;
  name_char_pos = 0;
  delimited_names = delimit_null(glyph_names, ' ');
  while (script == DEFAULT_SCRIPT && glyph_names[name_char_pos] != '\0') {
    SplineChar *sc = SFGetChar(sf, -1, delimited_names + name_char_pos);
    script = SCScriptFromUnicode(sc);
    name_char_pos += strlen(delimited_names + name_char_pos);
    if (glyph_names[name_char_pos] != '\0') name_char_pos ++;
  }
  free(delimited_names); delimited_names = NULL;
  return script;
}

#define GROUP_NAME_KERNING_UFO 1
#define GROUP_NAME_KERNING_FEATURE 2
#define GROUP_NAME_VERTICAL 4 // Otherwise horizontal.
#define GROUP_NAME_RIGHT 8 // Otherwise left (or above).

static void MakeKerningClasses(SplineFont *sf, struct ff_glyphclasses *group_base) {
  // This silently ignores already extant groups for now but avoids duplicates unless group_base has internal duplication.
  int left_count = 0, right_count = 0, above_count = 0, below_count = 0;
  int left_start = 0, right_start = 0, above_start = 0, below_start = 0;
#ifdef FF_UTHASH_GLIF_NAMES
    struct glif_name_index _class_name_hash;
    struct glif_name_index * class_name_hash = &_class_name_hash; // Open the hash table.
    memset(class_name_hash, 0, sizeof(struct glif_name_index));
    HashKerningClassNames(sf, class_name_hash);
#else
    void * class_name_hash = NULL;
#endif
  // It is very difficult to create speculative indices for the unmerged group members during the size calculation.
  // So we expect that the incoming group list has no duplicates (as after a run through GlyphGroupDeduplicate).
  struct ff_glyphclasses *current_group;
  for (current_group = group_base; current_group != NULL; current_group = current_group->next) {
    int group_type = GroupNameType(current_group->classname);
    if ((group_type != -1) && (group_type & (GROUP_NAME_KERNING_UFO|GROUP_NAME_KERNING_FEATURE))) {
      if (group_type & GROUP_NAME_VERTICAL) {
        if (group_type & GROUP_NAME_RIGHT) {
          below_count++;
        } else {
          above_count++;
        }
      } else {
        if (group_type & GROUP_NAME_RIGHT) {
          right_count++;
        } else {
          left_count++;
        }
      }
    }
  }
  // Allocate lookups if needed.
  if (sf->kerns == NULL && (left_count || right_count)) {
    sf->kerns = calloc(1, sizeof(struct kernclass));
    sf->kerns->subtable = SFSubTableFindOrMake(sf, CHR('k','e','r','n'), DEFAULT_SCRIPT, gpos_pair);
    sf->kerns->firsts = calloc(1, sizeof(char *));
    sf->kerns->firsts_names = calloc(1, sizeof(char *));
    sf->kerns->firsts_flags = calloc(1, sizeof(int));
    sf->kerns->seconds = calloc(1, sizeof(char *));
    sf->kerns->seconds_names = calloc(1, sizeof(char *));
    sf->kerns->seconds_flags = calloc(1, sizeof(int));
    sf->kerns->offsets = calloc(1, sizeof(int16));
    sf->kerns->offsets_flags = calloc(1, sizeof(int));
    sf->kerns->first_cnt = 1;
    sf->kerns->second_cnt = 1;
  }
  if (sf->vkerns == NULL && (above_count || below_count)) {
    sf->vkerns = calloc(1, sizeof(struct kernclass));
    sf->vkerns->subtable = SFSubTableFindOrMake(sf, CHR('v','k','r','n'), DEFAULT_SCRIPT, gpos_pair);
    sf->vkerns->firsts = calloc(1, sizeof(char *));
    sf->vkerns->firsts_names = calloc(1, sizeof(char *));
    sf->vkerns->firsts_flags = calloc(1, sizeof(int));
    sf->vkerns->seconds = calloc(1, sizeof(char *));
    sf->vkerns->seconds_names = calloc(1, sizeof(char *));
    sf->vkerns->seconds_flags = calloc(1, sizeof(int));
    sf->vkerns->offsets = calloc(1, sizeof(int16));
    sf->vkerns->offsets_flags = calloc(1, sizeof(int));
    sf->vkerns->first_cnt = 1;
    sf->vkerns->second_cnt = 1;
  }
  // Set starts.
  if (sf->kerns != NULL) { left_start = sf->kerns->first_cnt; right_start = sf->kerns->second_cnt; }
  if (sf->vkerns != NULL) { above_start = sf->vkerns->first_cnt; below_start = sf->vkerns->second_cnt; }
  // Make space for the new entries.
  // We start by allocating space for the offsets and offsets flags. We then copy the old contents, row-by-row.
  if ((left_count > 0 || right_count > 0) && ((sf->kerns->first_cnt + left_count) * (sf->kerns->second_cnt + right_count) > 0)) {
    // Offsets.
    int16 *tmp_offsets = calloc((sf->kerns->first_cnt + left_count) * (sf->kerns->second_cnt + right_count), sizeof(int16));
    if (sf->kerns->offsets) {
      int rowpos;
      for (rowpos = 0; rowpos < sf->kerns->first_cnt; rowpos ++) {
        memcpy((void *)tmp_offsets + (rowpos * (sf->kerns->second_cnt + right_count)) * sizeof(int16), (void *)(sf->kerns->offsets) + (rowpos * sf->kerns->second_cnt) * sizeof(int16), sf->kerns->second_cnt * sizeof(int16));
      }
      free(sf->kerns->offsets);
    }
    sf->kerns->offsets = tmp_offsets;
    // Offset flags.
    int *tmp_offsets_flags = calloc((sf->kerns->first_cnt + left_count) * (sf->kerns->second_cnt + right_count), sizeof(int));
    if (sf->kerns->offsets_flags) {
      int rowpos;
      for (rowpos = 0; rowpos < sf->kerns->first_cnt; rowpos ++) {
        memcpy((void *)tmp_offsets_flags + (rowpos * (sf->kerns->second_cnt + right_count)) * sizeof(int), (void *)(sf->kerns->offsets_flags) + (rowpos * sf->kerns->second_cnt) * sizeof(int), sf->kerns->second_cnt * sizeof(int));
      }
      free(sf->kerns->offsets_flags);
    }
    sf->kerns->offsets_flags = tmp_offsets_flags;
    // Adjusts.
    DeviceTable *tmp_adjusts = calloc((sf->kerns->first_cnt + left_count) * (sf->kerns->second_cnt + right_count), sizeof(DeviceTable));
    if (sf->kerns->adjusts) {
      int rowpos;
      for (rowpos = 0; rowpos < sf->kerns->first_cnt; rowpos ++) {
        memcpy((void *)tmp_adjusts + (rowpos * (sf->kerns->second_cnt + right_count)) * sizeof(DeviceTable), (void *)(sf->kerns->adjusts) + (rowpos * sf->kerns->second_cnt) * sizeof(DeviceTable), sf->kerns->second_cnt * sizeof(DeviceTable));
      }
      free(sf->kerns->adjusts);
    }
    sf->kerns->adjusts = tmp_adjusts;
  }
  if ((above_count > 0 || below_count > 0) && ((sf->vkerns->first_cnt + above_count) * (sf->vkerns->second_cnt + below_count) > 0)) {
    // Offsets.
    int16 *tmp_offsets = calloc((sf->vkerns->first_cnt + above_count) * (sf->vkerns->second_cnt + below_count), sizeof(int16));
    if (sf->vkerns->offsets) {
      int rowpos;
      for (rowpos = 0; rowpos < sf->vkerns->first_cnt; rowpos ++) {
        memcpy((void *)tmp_offsets + (rowpos * (sf->vkerns->second_cnt + below_count)) * sizeof(int16), (void *)(sf->vkerns->offsets) + (rowpos * sf->vkerns->second_cnt) * sizeof(int16), sf->vkerns->second_cnt * sizeof(int16));
      }
      free(sf->vkerns->offsets);
    }
    sf->vkerns->offsets = tmp_offsets;
    // Offset flags.
    int *tmp_offsets_flags = calloc((sf->vkerns->first_cnt + above_count) * (sf->vkerns->second_cnt + below_count), sizeof(int));
    if (sf->vkerns->offsets_flags) {
      int rowpos;
      for (rowpos = 0; rowpos < sf->vkerns->first_cnt; rowpos ++) {
        memcpy((void *)tmp_offsets_flags + (rowpos * (sf->vkerns->second_cnt + below_count)) * sizeof(int), (void *)(sf->vkerns->offsets_flags) + (rowpos * sf->vkerns->second_cnt) * sizeof(int), sf->vkerns->second_cnt * sizeof(int));
      }
      free(sf->vkerns->offsets_flags);
    }
    sf->vkerns->offsets_flags = tmp_offsets_flags;
    // Adjusts.
    DeviceTable *tmp_adjusts = calloc((sf->vkerns->first_cnt + above_count) * (sf->vkerns->second_cnt + below_count), sizeof(DeviceTable));
    if (sf->vkerns->adjusts) {
      int rowpos;
      for (rowpos = 0; rowpos < sf->vkerns->first_cnt; rowpos ++) {
        memcpy((void *)tmp_adjusts + (rowpos * (sf->vkerns->second_cnt + above_count)) * sizeof(DeviceTable), (void *)(sf->vkerns->adjusts) + (rowpos * sf->vkerns->second_cnt) * sizeof(DeviceTable), sf->vkerns->second_cnt * sizeof(DeviceTable));
      }
      free(sf->vkerns->adjusts);
    }
    sf->vkerns->adjusts = tmp_adjusts;
  }
  // Since the linear data need no repositioning, we can just use realloc. But it's important that we zero the new space in case it does not get filled.
  if (left_count > 0) {
    sf->kerns->firsts = realloc(sf->kerns->firsts, sizeof(char *) * (sf->kerns->first_cnt + left_count));
    memset((void*)sf->kerns->firsts + sf->kerns->first_cnt * sizeof(char *), 0, left_count * sizeof(char *));
    sf->kerns->firsts_names = realloc(sf->kerns->firsts_names, sizeof(char *) * (sf->kerns->first_cnt + left_count));
    memset((void*)sf->kerns->firsts_names + sf->kerns->first_cnt * sizeof(char *), 0, left_count * sizeof(char *));
    sf->kerns->firsts_flags = realloc(sf->kerns->firsts_flags, sizeof(int) * (sf->kerns->first_cnt + left_count));
    memset((void*)sf->kerns->firsts_flags + sf->kerns->first_cnt * sizeof(int), 0, left_count * sizeof(int));
    sf->kerns->first_cnt += left_count;
  }
  if (right_count > 0) {
    sf->kerns->seconds = realloc(sf->kerns->seconds, sizeof(char *) * (sf->kerns->second_cnt + right_count));
    memset((void*)sf->kerns->seconds + sf->kerns->second_cnt * sizeof(char *), 0, right_count * sizeof(char *));
    sf->kerns->seconds_names = realloc(sf->kerns->seconds_names, sizeof(char *) * (sf->kerns->second_cnt + right_count));
    memset((void*)sf->kerns->seconds_names + sf->kerns->second_cnt * sizeof(char *), 0, right_count * sizeof(char *));
    sf->kerns->seconds_flags = realloc(sf->kerns->seconds_flags, sizeof(int) * (sf->kerns->second_cnt + right_count));
    memset((void*)sf->kerns->seconds_flags + sf->kerns->second_cnt * sizeof(int), 0, right_count * sizeof(int));
    sf->kerns->second_cnt += right_count;
  }
  if (above_count > 0) {
    sf->vkerns->firsts = realloc(sf->vkerns->firsts, sizeof(char *) * (sf->vkerns->first_cnt + above_count));
    memset((void*)sf->vkerns->firsts + sf->vkerns->first_cnt * sizeof(char *), 0, above_count * sizeof(char *));
    sf->vkerns->firsts_names = realloc(sf->vkerns->firsts_names, sizeof(char *) * (sf->vkerns->first_cnt + above_count));
    memset((void*)sf->vkerns->firsts_names + sf->vkerns->first_cnt * sizeof(char *), 0, above_count * sizeof(char *));
    sf->vkerns->firsts_flags = realloc(sf->vkerns->firsts_flags, sizeof(int) * (sf->vkerns->first_cnt + above_count));
    memset((void*)sf->vkerns->firsts_flags + sf->vkerns->first_cnt * sizeof(int), 0, above_count * sizeof(int));
    sf->vkerns->first_cnt += above_count;
  }
  if (below_count > 0) {
    sf->vkerns->seconds = realloc(sf->vkerns->seconds, sizeof(char *) * (sf->vkerns->second_cnt + below_count));
    memset((void*)sf->vkerns->seconds + sf->vkerns->second_cnt * sizeof(char *), 0, below_count * sizeof(char *));
    sf->vkerns->seconds_names = realloc(sf->vkerns->seconds_names, sizeof(char *) * (sf->vkerns->second_cnt + below_count));
    memset((void*)sf->vkerns->seconds_names + sf->vkerns->second_cnt * sizeof(char *), 0, below_count * sizeof(char *));
    sf->vkerns->seconds_flags = realloc(sf->vkerns->seconds_flags, sizeof(int) * (sf->vkerns->second_cnt + below_count));
    memset((void*)sf->vkerns->seconds_flags + sf->vkerns->second_cnt * sizeof(char *), 0, below_count * sizeof(int));
    sf->vkerns->second_cnt += below_count;
  }
  // Start copying.
  left_count = 0; right_count = 0; above_count = 0; below_count = 0;
  for (current_group = group_base; current_group != NULL; current_group = current_group->next) {
    int group_type = GroupNameType(current_group->classname);
    // This function only gets used in processing groups.plist right now, so it assumes that the groups are native as below.
    if ((group_type != -1) && (group_type & (GROUP_NAME_KERNING_UFO|GROUP_NAME_KERNING_FEATURE))) {
      if (group_type & GROUP_NAME_VERTICAL) {
        if (group_type & GROUP_NAME_RIGHT) {
          sf->vkerns->seconds[below_start + below_count] = copy(current_group->glyphs);
          sf->vkerns->seconds_names[below_start + below_count] = copy(current_group->classname);
          sf->vkerns->seconds_flags[below_start + below_count] = FF_KERNCLASS_FLAG_NATIVE | ((group_type & GROUP_NAME_KERNING_FEATURE) ? FF_KERNCLASS_FLAG_NAMETYPE : 0);
          below_count++;
        } else {
          sf->vkerns->firsts[above_start + above_count] = copy(current_group->glyphs);
          sf->vkerns->firsts_names[above_start + above_count] = copy(current_group->classname);
          sf->vkerns->firsts_flags[above_start + above_count] = FF_KERNCLASS_FLAG_NATIVE | ((group_type & GROUP_NAME_KERNING_FEATURE) ? FF_KERNCLASS_FLAG_NAMETYPE : 0);
          above_count++;
        }
      } else {
        if (group_type & GROUP_NAME_RIGHT) {
          sf->kerns->seconds[right_start + right_count] = copy(current_group->glyphs);
          sf->kerns->seconds_names[right_start + right_count] = copy(current_group->classname);
          sf->kerns->seconds_flags[right_start + right_count] = FF_KERNCLASS_FLAG_NATIVE | ((group_type & GROUP_NAME_KERNING_FEATURE) ? FF_KERNCLASS_FLAG_NAMETYPE : 0);
          right_count++;
        } else {
          sf->kerns->firsts[left_start + left_count] = copy(current_group->glyphs);
          sf->kerns->firsts_names[left_start + left_count] = copy(current_group->classname);
          sf->kerns->firsts_flags[left_start + left_count] = FF_KERNCLASS_FLAG_NATIVE | ((group_type & GROUP_NAME_KERNING_FEATURE) ? FF_KERNCLASS_FLAG_NAMETYPE : 0);
          left_count++;
        }
      }
    }
  }
#ifdef UFO_GUESS_SCRIPTS
  // Check the script in each element of each group (for each polarity) until a character is of a script other than DFLT.
  if (sf->kerns != NULL) {
    uint32 script = DEFAULT_SCRIPT;
    int class_index;
    class_index = 0;
    while (script == DEFAULT_SCRIPT && class_index < sf->kerns->first_cnt) {
      if (sf->kerns->firsts[class_index] != NULL) script = script_from_glyph_list(sf, sf->kerns->firsts[class_index]);
      class_index++;
    }
    class_index = 0;
    while (script == DEFAULT_SCRIPT && class_index < sf->kerns->second_cnt) {
      if (sf->kerns->seconds[class_index] != NULL) script = script_from_glyph_list(sf, sf->kerns->seconds[class_index]);
      class_index++;
    }
    sf->kerns->subtable = SFSubTableFindOrMake(sf, CHR('k','e','r','n'), script, gpos_pair);
  }
  if (sf->vkerns != NULL) {
    uint32 script = DEFAULT_SCRIPT;
    int class_index;
    class_index = 0;
    while (script == DEFAULT_SCRIPT && class_index < sf->vkerns->first_cnt) {
      if (sf->vkerns->firsts[class_index] != NULL) script = script_from_glyph_list(sf, sf->vkerns->firsts[class_index]);
      class_index++;
    }
    class_index = 0;
    while (script == DEFAULT_SCRIPT && class_index < sf->vkerns->second_cnt) {
      if (sf->vkerns->seconds[class_index] != NULL) script = script_from_glyph_list(sf, sf->vkerns->seconds[class_index]);
      class_index++;
    }
    sf->vkerns->subtable = SFSubTableFindOrMake(sf, CHR('v','k','r','n'), script, gpos_pair);
  }
#else
  // Some test cases have proven that FontForge would do best to avoid classifying these.
  uint32 script = DEFAULT_SCRIPT;
  if (sf->kerns != NULL) {
    sf->kerns->subtable = SFSubTableFindOrMake(sf, CHR('k','e','r','n'), script, gpos_pair);
  }
  if (sf->vkerns != NULL) {
    sf->vkerns->subtable = SFSubTableFindOrMake(sf, CHR('v','k','r','n'), script, gpos_pair);
  }
#endif // UFO_GUESS_SCRIPTS

}

static void UFOHandleGroups(SplineFont *sf, char *basedir) {
    char *fname = buildname(basedir, "groups.plist");
    xmlDocPtr doc=NULL;
    xmlNodePtr plist,dict,keys,value,subkeys;
    char *keyname, *valname;
    struct ff_glyphclasses *current_group = NULL;
    // We want to start at the end of the list of groups already in the SplineFont (probably not any right now).
    for (current_group = sf->groups; current_group != NULL && current_group->next != NULL; current_group = current_group->next);
    int group_count;

    if ( GFileExists(fname))
	doc = xmlParseFile(fname);
    free(fname);
    if ( doc==NULL )
return;

    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return;
    }
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
	    SplineChar *sc = SFGetChar(sf,-1,keyname);
	    if ( sc!=NULL ) { LogError(_("Skipping group %s with same name as a glyph.\n"), keyname); free(keyname); keyname = NULL; continue; }
            struct ff_glyphclasses *sfg = SFGetGroup(sf,-1,keyname);
	    if ( sfg!=NULL ) { LogError(_("Skipping duplicate group %s.\n"), keyname); free(keyname); keyname = NULL; continue; }
	    sfg = calloc(1, sizeof(struct ff_glyphclasses)); // We allocate space for the new group.
	    sfg->classname = keyname; keyname = NULL; // We name it.
	    if (current_group == NULL) sf->groups = sfg;
	    else current_group->next = sfg;
	    current_group = sfg; // And we put it at the end of the list.
	    // We prepare to populate it. We will match to native glyphs first (in order to validate) and then convert back to strings later.
	    RefChar *members_native = NULL;
	    RefChar *member_native_current = NULL;
	    int member_count = 0;
	    int member_list_length = 0; // This makes it easy to allocate a string at the end.
	    // We fetch the contents now. They are in an array, but we do not verify that.
	    keys = value;
	    for ( subkeys = keys->children; subkeys!=NULL; subkeys = subkeys->next ) {
		if ( xmlStrcmp(subkeys->name,(const xmlChar *) "string")==0 ) {
		    keyname = (char *) xmlNodeListGetString(doc,subkeys->children,true); // Get the member name.
		    SplineChar *ssc = SFGetChar(sf,-1,keyname); // Try to match an existing glyph.
		    if ( ssc==NULL ) { LogError(_("Skipping non-existent glyph %s in group %s.\n"), keyname, current_group->classname); free(keyname); keyname = NULL; continue; }
		    member_list_length += strlen(keyname) + 1; member_count++; // Make space for its name.
		    free(keyname); // Free the name for now. (We get it directly from the SplineChar later.)
		    RefChar *member_native_temp = calloc(1, sizeof(RefChar)); // Make an entry in the list for the native reference.
		    member_native_temp->sc = ssc; ssc = NULL;
		    if (member_native_current == NULL) members_native = member_native_temp;
		    else member_native_current->next = member_native_temp;
		    member_native_current = member_native_temp; // Add it to the end of the list.
		}
	    }
	    if (member_list_length == 0) member_list_length++; // We must have space for a zero-terminator even if the list is empty. A non-empty list has space for a space at the end that we can use.
	    current_group->glyphs = malloc(member_list_length); // We allocate space for the list.
	    current_group->glyphs[0] = '\0';
	    for (member_native_current = members_native; member_native_current != NULL; member_native_current = member_native_current->next) {
                if (member_native_current != members_native) strcat(current_group->glyphs, " ");
	        strcat(current_group->glyphs, member_native_current->sc->name);
	    }
	    RefCharsFree(members_native); members_native = NULL;
	}
    }
    // The preceding routine was sufficiently complicated that it seemed like a good idea to perform the deduplication in a separate block.
    sf->groups = GlyphGroupDeduplicate(sf->groups, sf, 1);
    // We now add kerning classes for any groups that are named like kerning classes.
    MakeKerningClasses(sf, sf->groups);
    xmlFreeDoc(doc);
}

static void UFOHandleKern(SplineFont *sf,char *basedir,int isv) {
    char *fname = buildname(basedir,isv ? "vkerning.plist" : "kerning.plist");
    xmlDocPtr doc=NULL;
    xmlNodePtr plist,dict,keys,value,subkeys;
    char *keyname, *valname;
    int offset;
    SplineChar *sc, *ssc;
    KernPair *kp;
    char *end;
    uint32 script;

    if ( GFileExists(fname))
	doc = xmlParseFile(fname);
    free(fname);
    if ( doc==NULL )
return;

    // If there is native kerning (as we would expect if the function has not returned), set the SplineFont flag to prefer it on output.
    sf->preferred_kerning = 1;

    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return;
    }
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
	    sc = SFGetChar(sf,-1,keyname);
	    free(keyname);
	    if ( sc==NULL )
	continue;
	    keys = value;
	    for ( subkeys = value->children; subkeys!=NULL; subkeys = subkeys->next ) {
		for ( value = subkeys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
			value = value->next );
		if ( value==NULL )
	    break;
		if ( xmlStrcmp(subkeys->name,(const xmlChar *) "key")==0 ) {
		    keyname = (char *) xmlNodeListGetString(doc,subkeys->children,true);
		    ssc = SFGetChar(sf,-1,keyname);
		    free(keyname);
		    if ( ssc==NULL )
		continue;
		    for ( kp=isv?sc->vkerns:sc->kerns; kp!=NULL && kp->sc!=ssc; kp=kp->next );
		    if ( kp!=NULL )
		continue;
		    subkeys = value;
		    valname = (char *) xmlNodeListGetString(doc,value->children,true);
		    offset = strtol(valname,&end,10);
		    if ( *end=='\0' ) {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = offset;
			kp->sc = ssc;
			if ( isv ) {
			    kp->next = sc->vkerns;
			    sc->vkerns = kp;
			} else {
			    kp->next = sc->kerns;
			    sc->kerns = kp;
			}
#ifdef UFO_GUESS_SCRIPTS
			script = SCScriptFromUnicode(sc);
			if ( script==DEFAULT_SCRIPT )
			    script = SCScriptFromUnicode(ssc);
#else
			// Some test cases have proven that FontForge would do best to avoid classifying these.
			script = DEFAULT_SCRIPT;
#endif // UFO_GUESS_SCRIPTS
			kp->subtable = SFSubTableFindOrMake(sf,
				isv?CHR('v','k','r','n'):CHR('k','e','r','n'),
				script, gpos_pair);
		    }
		    free(valname);
		}
	    }
	}
    }
    xmlFreeDoc(doc);
}

int TryAddRawGroupKern(struct splinefont *sf, int isv, struct glif_name_index *class_name_pair_hash, int *current_groupkern_index_p, struct ff_rawoffsets **current_groupkern_p, const char *left, const char *right, int offset) {
  char *pairtext;
  int success = 0;
  if (left && right && asprintf(&pairtext, "%s %s", left, right) > 0 && pairtext) {
    if (!glif_name_search_glif_name(class_name_pair_hash, pairtext)) {
      glif_name_track_new(class_name_pair_hash, (*current_groupkern_index_p)++, pairtext);
      struct ff_rawoffsets *tmp_groupkern = calloc(1, sizeof(struct ff_rawoffsets));
      tmp_groupkern->left = copy(left);
      tmp_groupkern->right = copy(right);
      tmp_groupkern->offset = offset;
      if (*current_groupkern_p == NULL) {
        if (isv) sf->groupvkerns = tmp_groupkern;
        else sf->groupkerns = tmp_groupkern;
      } else (*current_groupkern_p)->next = tmp_groupkern;
      *current_groupkern_p = tmp_groupkern;
      success = 1;
    }
    free(pairtext); pairtext = NULL;
  }
  return success;
}

static void UFOHandleKern3(SplineFont *sf,char *basedir,int isv) {
    char *fname = buildname(basedir,isv ? "vkerning.plist" : "kerning.plist");
    xmlDocPtr doc=NULL;
    xmlNodePtr plist,dict,keys,value,subkeys;
    char *keyname, *valname;
    int offset;
    SplineChar *sc, *ssc;
    KernPair *kp;
    char *end;
    uint32 script;

    if ( GFileExists(fname))
	doc = xmlParseFile(fname);
    free(fname);
    if ( doc==NULL )
return;

    // If there is native kerning (as we would expect if the function has not returned), set the SplineFont flag to prefer it on output.
    sf->preferred_kerning = 1;

    plist = xmlDocGetRootElement(doc);
    dict = FindNode(plist->children,"dict");
    if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
return;
    }

    // We want a hash table of group names for reference.
    struct glif_name_index _group_name_hash;
    struct glif_name_index * group_name_hash = &_group_name_hash; // Open the group hash table.
    memset(group_name_hash, 0, sizeof(struct glif_name_index));
    struct ff_glyphclasses *current_group = NULL;
    int current_group_index = 0;
    for (current_group = sf->groups, current_group_index = 0; current_group != NULL; current_group = current_group->next, current_group_index++)
      if (current_group->classname != NULL) glif_name_track_new(group_name_hash, current_group_index, current_group->classname);
    // We also want a hash table of kerning class names. (We'll probably standardize on one approach or the other later.)
    struct glif_name_index _class_name_hash;
    struct glif_name_index * class_name_hash = &_class_name_hash; // Open the group hash table.
    memset(class_name_hash, 0, sizeof(struct glif_name_index));
    HashKerningClassNames(sf, class_name_hash);
    // We also want a hash table of the lookup pairs.
    struct glif_name_index _class_name_pair_hash;
    struct glif_name_index * class_name_pair_hash = &_class_name_pair_hash; // Open the group hash table.
    memset(class_name_pair_hash, 0, sizeof(struct glif_name_index));
    // We need to track the head of the group kerns.
    struct ff_rawoffsets *current_groupkern = NULL;
    int current_groupkern_index = 0;
    // We want to start at the end of the list of kerns already in the SplineFont (probably not any right now).
    for (current_groupkern = (isv ? sf->groupvkerns : sf->groupkerns); current_groupkern != NULL && current_groupkern->next != NULL; current_groupkern = current_groupkern->next);

    // Read the left node. Set sc if it matches a character or isgroup and the associated values if it matches a group.
    // Read the right node. Set ssc if it matches a character or isgroup and the associated values if it matches a group.
    // If sc and ssc, add a kerning pair to sc for ssc.
    // If left_isgroup and right_isgroup, use the processed values in order to offset.
    for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
    break;
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = (char *) xmlNodeListGetString(doc,keys->children,true);
            // Search for a glyph first.
	    sc = SFGetChar(sf,-1,keyname);
            // Search for a group.
            struct glif_name *left_class_name_record = glif_name_search_glif_name(class_name_hash, keyname);
	    free(keyname);
            if (sc == NULL && left_class_name_record == NULL) { LogError(_("kerning.plist references an entity that is neither a glyph nor a group.")); continue; }
	    keys = value; // Set the offset for the next round.
            // This key represents the left/above side of the pair. The child keys represent its right/below complements.
	    for ( subkeys = value->children; subkeys!=NULL; subkeys = subkeys->next ) {
		for ( value = subkeys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
			value = value->next );
		if ( value==NULL )
	    break;
		if ( xmlStrcmp(subkeys->name,(const xmlChar *) "key")==0 ) {
		    // Get the second name of the pair.
		    keyname = (char *) xmlNodeListGetString(doc,subkeys->children,true);
		    // Get the offset in numeric form.
		    valname = (char *) xmlNodeListGetString(doc,value->children,true);
		    offset = strtol(valname,&end,10);
		    if (*end != '\0') { LogError(_("kerning.plist has a non-numeric offset.")); continue; }
		    free(valname); valname = NULL;
		    subkeys = value; // Set the iterator for the next use.
		    // Search for a character.
		    ssc = SFGetChar(sf,-1,keyname);
		    // Search for a group.
		    struct glif_name *right_class_name_record = glif_name_search_glif_name(class_name_hash, keyname);
		    free(keyname);
		    if (ssc == NULL && right_class_name_record == NULL) { LogError(_("kerning.plist references an entity that is neither a glyph nor a group.")); continue; }

		  if (sc && ssc) {
		    KernPair *lastkp = NULL;
		    for ( kp=(isv?sc->vkerns:sc->kerns); kp!=NULL && kp->sc!=ssc; lastkp = kp, kp=kp->next );
		    if ( kp!=NULL ) { LogError(_("kerning.plist defines kerning between two glyphs that are already kerned.")); continue; }
		    // We do not want to add the virtual entry until we have confirmed the possibility of adding the real entry as precedes this.
		    if (!TryAddRawGroupKern(sf, isv, class_name_pair_hash, &current_groupkern_index, &current_groupkern, sc->name, ssc->name, offset)) {
		      LogError(_("kerning.plist defines kerning between two glyphs that are already partially kerned.")); continue;
		    }
		    {
			kp = chunkalloc(sizeof(KernPair));
			kp->off = offset;
			kp->sc = ssc;
			if ( isv ) {
			    if (lastkp != NULL) lastkp->next = kp;
			    else sc->vkerns = kp;
			    lastkp = kp;
			    // kp->next = sc->vkerns;
			    // sc->vkerns = kp;
			} else {
			    if (lastkp != NULL) lastkp->next = kp;
			    else sc->kerns = kp;
			    lastkp = kp;
			    // kp->next = sc->kerns;
			    // sc->kerns = kp;
			}
#ifdef UFO_GUESS_SCRIPTS
			script = SCScriptFromUnicode(sc);
			if ( script==DEFAULT_SCRIPT )
			    script = SCScriptFromUnicode(ssc);
#else
			// Some test cases have proven that FontForge would do best to avoid classifying these.
			script = DEFAULT_SCRIPT;
#endif // UFO_GUESS_SCRIPTS
			kp->subtable = SFSubTableFindOrMake(sf,
				isv?CHR('v','k','r','n'):CHR('k','e','r','n'),
				script, gpos_pair);
		    }
		  } else if (sc && right_class_name_record) {
		    if (!TryAddRawGroupKern(sf, isv, class_name_pair_hash, &current_groupkern_index, &current_groupkern, sc->name, right_class_name_record->glif_name, offset)) {
		      LogError(_("kerning.plist defines kerning between two glyphs that are already partially kerned.")); continue;
		    }
		  } else if (ssc && left_class_name_record) {
		    if (!TryAddRawGroupKern(sf, isv, class_name_pair_hash, &current_groupkern_index, &current_groupkern, left_class_name_record->glif_name, ssc->name, offset)) {
		      LogError(_("kerning.plist defines kerning between two glyphs that are already partially kerned.")); continue;
		    }
		  } else if (left_class_name_record && right_class_name_record) {
		    struct kernclass *left_kc, *right_kc;
		    int left_isv, right_isv, left_isr, right_isr, left_offset, right_offset;
		    int left_exists = KerningClassSeekByAbsoluteIndex(sf, left_class_name_record->gid, &left_kc, &left_isv, &left_isr, &left_offset);
		    int right_exists = KerningClassSeekByAbsoluteIndex(sf, right_class_name_record->gid, &right_kc, &right_isv, &right_isr, &right_offset);
		    if ((left_kc == NULL) || (right_kc == NULL)) { LogError(_("kerning.plist references a missing kerning class.")); continue; } // I don't know how this would happen, at least as the code is now, but we do need to throw an error.
		    if ((left_kc != right_kc)) { LogError(_("kerning.plist defines an offset between classes in different lookups.")); continue; }
		    if ((left_offset > left_kc->first_cnt) || (right_offset > right_kc->second_cnt)) { LogError(_("There is a kerning class index error.")); continue; }
		    if (left_kc->offsets_flags[left_offset*left_kc->second_cnt+right_offset]) { LogError(_("kerning.plist attempts to redefine a class kerning offset.")); continue; }
		    // All checks pass. We add the virtual pair and then the real one.
		    if (!TryAddRawGroupKern(sf, isv, class_name_pair_hash, &current_groupkern_index, &current_groupkern, left_class_name_record->glif_name, right_class_name_record->glif_name, offset)) {
		      LogError(_("kerning.plist defines kerning between two glyphs that are already partially kerned.")); continue;
		    }
		    left_kc->offsets[left_offset*left_kc->second_cnt+right_offset] = offset;
		    left_kc->offsets_flags[left_offset*left_kc->second_cnt+right_offset] |= FF_KERNCLASS_FLAG_NATIVE;
		  }
		}
	    }
	}
    }
    glif_name_hash_destroy(group_name_hash);
    glif_name_hash_destroy(class_name_hash);
    glif_name_hash_destroy(class_name_pair_hash);
    xmlFreeDoc(doc);
}

static void UFOAddName(SplineFont *sf,char *value,int strid) {
    /* We simply assume that the entries in the name table are in English */
    /* The format doesn't say -- which bothers me */
    struct ttflangname *names;

    for ( names=sf->names; names!=NULL && names->lang!=0x409; names=names->next );
    if ( names==NULL ) {
	names = chunkalloc(sizeof(struct ttflangname));
	names->next = sf->names;
	names->lang = 0x409;
	sf->names = names;
    }
    names->names[strid] = value;
}

static void UFOAddPrivate(SplineFont *sf,char *key,char *value) {
    char *pt;

    if ( sf->private==NULL )
	sf->private = chunkalloc(sizeof(struct psdict));
    for ( pt=value; *pt!='\0'; ++pt ) {	/* Value might contain white space. turn into spaces */
	if ( *pt=='\n' || *pt=='\r' || *pt=='\t' )
	    *pt = ' ';
    }
    PSDictChangeEntry(sf->private, key, value);
}

static void UFOAddPrivateArray(SplineFont *sf,char *key,xmlDocPtr doc,xmlNodePtr value) {
    char space[400], *pt, *end;
    xmlNodePtr kid;

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    pt = space; end = pt+sizeof(space)-10;
    *pt++ = '[';
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ||
		xmlStrcmp(kid->name,(const xmlChar *) "real")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    if ( pt+1+strlen(valName)<end ) {
		if ( pt!=space+1 )
		    *pt++=' ';
		strcpy(pt,valName);
		pt += strlen(pt);
	    }
	    free(valName);
	}
    }
    if ( pt!=space+1 ) {
	*pt++ = ']';
	*pt++ = '\0';
	UFOAddPrivate(sf,key,space);
    }
}

static void UFOGetByteArray(char *array,int cnt,xmlDocPtr doc,xmlNodePtr value) {
    xmlNodePtr kid;
    int i;

    memset(array,0,cnt);

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    i=0;
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    if ( i<cnt )
		array[i++] = strtol(valName,NULL,10);
	    free(valName);
	}
    }
}

static long UFOGetBits(xmlDocPtr doc,xmlNodePtr value) {
    xmlNodePtr kid;
    long mask=0;

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return( 0 );
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    mask |= 1<<strtol(valName,NULL,10);
	    free(valName);
	}
    }
return( mask );
}

static void UFOGetBitArray(xmlDocPtr doc,xmlNodePtr value,uint32 *res,int len) {
    xmlNodePtr kid;
    int index;

    if ( xmlStrcmp(value->name,(const xmlChar *) "array")!=0 )
return;
    for ( kid = value->children; kid!=NULL; kid=kid->next ) {
	if ( xmlStrcmp(kid->name,(const xmlChar *) "integer")==0 ) {
	    char *valName = (char *) xmlNodeListGetString(doc,kid->children,true);
	    index = strtol(valName,NULL,10);
	    if ( index < len<<5 )
		res[index>>5] |= 1<<(index&31);
	    free(valName);
	}
    }
}

SplineFont *SFReadUFO(char *basedir, int flags) {
    xmlNodePtr plist, dict, keys, value;
    xmlNodePtr guidelineNode = NULL;
    xmlDocPtr doc;
    SplineFont *sf;
    SplineSet *lastglspl = NULL;
    xmlChar *keyname, *valname;
    char *stylename=NULL;
    char *temp, *glyphlist, *glyphdir;
    char *end;
    int as = -1, ds= -1, em= -1;

    if ( !libxml_init_base()) {
	LogError(_("Can't find libxml2."));
return( NULL );
    }

    sf = SplineFontEmpty();
    SFDefaultOS2Info(&sf->pfminfo, sf, ""); // We set the default pfm values.
    sf->pfminfo.pfmset = 1; // We flag the pfminfo as present since we expect the U. F. O. to set any desired values.
    int versionMajor = -1; // These are not native SplineFont values.
    int versionMinor = -1; // We store the U. F. O. values and then process them at the end.
    sf->pfminfo.stylemap = 0x0;

    temp = buildname(basedir,"fontinfo.plist");
    doc = xmlParseFile(temp);
    free(temp);
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    if ( doc!=NULL ) {
      plist = xmlDocGetRootElement(doc);
      dict = FindNode(plist->children,"dict");
      if ( xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 || dict==NULL ) {
	LogError(_("Expected property list file"));
	xmlFreeDoc(doc);
      return( NULL );
      }
      for ( keys=dict->children; keys!=NULL; keys=keys->next ) {
	for ( value = keys->next; value!=NULL && xmlStrcmp(value->name,(const xmlChar *) "text")==0;
		value = value->next );
	if ( value==NULL )
          break;
	if ( xmlStrcmp(keys->name,(const xmlChar *) "key")==0 ) {
	    keyname = xmlNodeListGetString(doc,keys->children,true);
	    valname = xmlNodeListGetString(doc,value->children,true);
	    keys = value;
	    if ( xmlStrcmp(keyname,(xmlChar *) "familyName")==0 ) {
		if (sf->familyname == NULL) sf->familyname = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "styleName")==0 ) {
		if (stylename == NULL) stylename = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "styleMapFamilyName")==0 ) {
		if (sf->styleMapFamilyName == NULL) sf->styleMapFamilyName = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "styleMapStyleName")==0 ) {
		if (strcmp((char *) valname, "regular")==0) sf->pfminfo.stylemap = 0x40;
        else if (strcmp((char *) valname, "italic")==0) sf->pfminfo.stylemap = 0x01;
        else if (strcmp((char *) valname, "bold")==0) sf->pfminfo.stylemap = 0x20;
        else if (strcmp((char *) valname, "bold italic")==0) sf->pfminfo.stylemap = 0x21;
		free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "fullName")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptFullName")==0 ) {
		if (sf->fullname == NULL) sf->fullname = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "fontName")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptFontName")==0 ) {
		if (sf->fontname == NULL) sf->fontname = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "weightName")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptWeightName")==0 ) {
		if (sf->weight == NULL) sf->weight = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "note")==0 ) {
		if (sf->comments == NULL) sf->comments = (char *) valname;
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "copyright")==0 ) {
		UFOAddName(sf,(char *) valname,ttf_copyright);
        /* sf->copyright hosts the old ASCII-only PS attribute */
        if (sf->copyright == NULL) sf->copyright = normalizeToASCII((char *) valname);
		else free(valname);
	    }
	    else if ( xmlStrcmp(keyname,(xmlChar *) "trademark")==0 )
		UFOAddName(sf,(char *) valname,ttf_trademark);
	    else if ( strncmp((char *) keyname,"openTypeName",12)==0 ) {
		if ( xmlStrcmp(keyname+12,(xmlChar *) "Designer")==0 )
		    UFOAddName(sf,(char *) valname,ttf_designer);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "DesignerURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_designerurl);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "Manufacturer")==0 )
		    UFOAddName(sf,(char *) valname,ttf_manufacturer);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "ManufacturerURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_venderurl);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "License")==0 )
		    UFOAddName(sf,(char *) valname,ttf_license);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "LicenseURL")==0 )
		    UFOAddName(sf,(char *) valname,ttf_licenseurl);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "Version")==0 )
		    UFOAddName(sf,(char *) valname,ttf_version);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "UniqueID")==0 )
		    UFOAddName(sf,(char *) valname,ttf_uniqueid);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "Description")==0 )
		    UFOAddName(sf,(char *) valname,ttf_descriptor);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "PreferredFamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_preffamilyname);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "PreferredSubfamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_prefmodifiers);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "CompatibleFullName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_compatfull);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "SampleText")==0 )
		    UFOAddName(sf,(char *) valname,ttf_sampletext);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "WWSFamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_wwsfamily);
		else if ( xmlStrcmp(keyname+12,(xmlChar *) "WWSSubfamilyName")==0 )
		    UFOAddName(sf,(char *) valname,ttf_wwssubfamily);
		else
		    free(valname);
	    } else if ( strncmp((char *) keyname, "openTypeHhea",12)==0 ) {
		if ( xmlStrcmp(keyname+12,(xmlChar *) "Ascender")==0 ) {
		    sf->pfminfo.hhead_ascent = strtol((char *) valname,&end,10);
		    sf->pfminfo.hheadascent_add = false;
		} else if ( xmlStrcmp(keyname+12,(xmlChar *) "Descender")==0 ) {
		    sf->pfminfo.hhead_descent = strtol((char *) valname,&end,10);
		    sf->pfminfo.hheaddescent_add = false;
		} else if ( xmlStrcmp(keyname+12,(xmlChar *) "LineGap")==0 )
		    sf->pfminfo.linegap = strtol((char *) valname,&end,10);
		free(valname);
		sf->pfminfo.hheadset = true;
	    } else if ( strncmp((char *) keyname,"openTypeVhea",12)==0 ) {
		if ( xmlStrcmp(keyname+12,(xmlChar *) "LineGap")==0 )
		    sf->pfminfo.vlinegap = strtol((char *) valname,&end,10);
		sf->pfminfo.vheadset = true;
		free(valname);
	    } else if ( strncmp((char *) keyname,"openTypeOS2",11)==0 ) {
		sf->pfminfo.pfmset = true;
		if ( xmlStrcmp(keyname+11,(xmlChar *) "Panose")==0 ) {
		    UFOGetByteArray(sf->pfminfo.panose,sizeof(sf->pfminfo.panose),doc,value);
		    sf->pfminfo.panose_set = true;
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "Type")==0 ) {
		    sf->pfminfo.fstype = UFOGetBits(doc,value);
		    if ( sf->pfminfo.fstype<0 ) {
			/* all bits are set, but this is wrong, OpenType spec says */
			/* bits 0, 4-7 and 10-15 must be unset, go see		   */
			/* http://www.microsoft.com/typography/otspec/os2.htm#fst  */
			LogError(_("Bad openTypeOS2type key: all bits are set. It will be ignored"));
			sf->pfminfo.fstype = 0;
		    }
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "FamilyClass")==0 ) {
		    char fc[2];
		    UFOGetByteArray(fc,sizeof(fc),doc,value);
		    sf->pfminfo.os2_family_class = (fc[0]<<8)|fc[1];
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "WidthClass")==0 )
		    sf->pfminfo.width = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "WeightClass")==0 )
		    sf->pfminfo.weight = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "VendorID")==0 )
		{
		    const int os2_vendor_sz = sizeof(sf->pfminfo.os2_vendor);
		    const int valname_len = c_strlen(valname);

		    if( valname && valname_len <= os2_vendor_sz )
			strncpy(sf->pfminfo.os2_vendor,valname,valname_len);

		    char *temp = sf->pfminfo.os2_vendor + os2_vendor_sz - 1;
		    while ( *temp == 0 && temp >= sf->pfminfo.os2_vendor )
			*temp-- = ' ';
		}
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "TypoAscender")==0 ) {
		    sf->pfminfo.typoascent_add = false;
		    sf->pfminfo.os2_typoascent = strtol((char *) valname,&end,10);
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "TypoDescender")==0 ) {
		    sf->pfminfo.typodescent_add = false;
		    sf->pfminfo.os2_typodescent = strtol((char *) valname,&end,10);
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "TypoLineGap")==0 )
		    sf->pfminfo.os2_typolinegap = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+11,(xmlChar *) "WinAscent")==0 ) {
		    sf->pfminfo.winascent_add = false;
		    sf->pfminfo.os2_winascent = strtol((char *) valname,&end,10);
		} else if ( xmlStrcmp(keyname+11,(xmlChar *) "WinDescent")==0 ) {
		    sf->pfminfo.windescent_add = false;
		    sf->pfminfo.os2_windescent = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11,"Subscript",9)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( xmlStrcmp(keyname+20,(xmlChar *) "XSize")==0 )
			sf->pfminfo.os2_subxsize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "YSize")==0 )
			sf->pfminfo.os2_subysize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "XOffset")==0 )
			sf->pfminfo.os2_subxoff = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "YOffset")==0 )
			sf->pfminfo.os2_subyoff = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "Superscript",11)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( xmlStrcmp(keyname+22,(xmlChar *) "XSize")==0 )
			sf->pfminfo.os2_supxsize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+22,(xmlChar *) "YSize")==0 )
			sf->pfminfo.os2_supysize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+22,(xmlChar *) "XOffset")==0 )
			sf->pfminfo.os2_supxoff = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+22,(xmlChar *) "YOffset")==0 )
			sf->pfminfo.os2_supyoff = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "Strikeout",9)==0 ) {
		    sf->pfminfo.subsuper_set = true;
		    if ( xmlStrcmp(keyname+20,(xmlChar *) "Size")==0 )
			sf->pfminfo.os2_strikeysize = strtol((char *) valname,&end,10);
		    else if ( xmlStrcmp(keyname+20,(xmlChar *) "Position")==0 )
			sf->pfminfo.os2_strikeypos = strtol((char *) valname,&end,10);
		} else if ( strncmp((char *) keyname+11, "CodePageRanges",14)==0 ) {
		    UFOGetBitArray(doc,value,sf->pfminfo.codepages,2);
		    sf->pfminfo.hascodepages = true;
		} else if ( strncmp((char *) keyname+11, "UnicodeRanges",13)==0 ) {
		    UFOGetBitArray(doc,value,sf->pfminfo.unicoderanges,4);
		    sf->pfminfo.hasunicoderanges = true;
		}
		free(valname);
	    } else if ( strncmp((char *) keyname, "postscript",10)==0 ) {
		if ( xmlStrcmp(keyname+10,(xmlChar *) "UnderlineThickness")==0 )
		    sf->uwidth = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "UnderlinePosition")==0 )
		    sf->upos = strtol((char *) valname,&end,10);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueFuzz")==0 )
		    UFOAddPrivate(sf,"BlueFuzz",(char *) valname);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueScale")==0 )
		    UFOAddPrivate(sf,"BlueScale",(char *) valname);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueShift")==0 )
		    UFOAddPrivate(sf,"BlueShift",(char *) valname);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "BlueValues")==0 )
		    UFOAddPrivateArray(sf,"BlueValues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "OtherBlues")==0 )
		    UFOAddPrivateArray(sf,"OtherBlues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "FamilyBlues")==0 )
		    UFOAddPrivateArray(sf,"FamilyBlues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "FamilyOtherBlues")==0 )
		    UFOAddPrivateArray(sf,"FamilyOtherBlues",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "StemSnapH")==0 )
		    UFOAddPrivateArray(sf,"StemSnapH",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "StemSnapV")==0 )
		    UFOAddPrivateArray(sf,"StemSnapV",doc,value);
		else if ( xmlStrcmp(keyname+10,(xmlChar *) "ForceBold")==0 )
		    UFOAddPrivate(sf,"ForceBold",(char *) value->name);
			/* value->name is either true or false */
		free(valname);
	    } else if ( strncmp((char *)keyname,"macintosh",9)==0 ) {
		if ( xmlStrcmp(keyname+9,(xmlChar *) "FONDName")==0 )
		    sf->fondname = (char *) valname;
		else
		    free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "unitsPerEm")==0 ) {
		em = strtol((char *) valname,&end,10);
		if ( *end!='\0' || em < 0 ) em = -1;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "ascender")==0 ) {
		as = strtod((char *) valname,&end);
		if ( *end!='\0' ) as = -1;
		else sf->ufo_ascent = as;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "descender")==0 ) {
		ds = -strtod((char *) valname,&end);
		if ( *end!='\0' ) ds = -1;
		else sf->ufo_descent = -ds;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "xHeight")==0 ) {
		sf->pfminfo.os2_xheight = strtol((char *) valname,&end,10); free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "capHeight")==0 ) {
		sf->pfminfo.os2_capheight = strtol((char *) valname,&end,10); free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "italicAngle")==0 ||
		    xmlStrcmp(keyname,(xmlChar *) "postscriptSlantAngle")==0 ) {
		sf->italicangle = strtod((char *) valname,&end);
		if ( *end!='\0' ) sf->italicangle = 0;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "versionMajor")==0 ) {
		versionMajor = strtol((char *) valname,&end, 10);
		if ( *end!='\0' ) versionMajor = -1;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "versionMinor")==0 ) {
		versionMinor = strtol((char *) valname,&end, 10);
		if ( *end!='\0' ) versionMinor = -1;
		free(valname);
	    } else if ( xmlStrcmp(keyname,(xmlChar *) "guidelines")==0 ) {
		guidelineNode = value; // Guideline figuring needs ascent and descent, so we must parse this later.
	    } else
		free(valname);
	    free(keyname);
	}
      }
    }
    if ( em==-1 && as>=0 && ds>=0 )
	em = as + ds;
    if ( em==as+ds ) {
	/* Yay! They follow my conventions */;
    } else if ( em!=-1 ) {
	as = 800*em/1000;
	ds = em-as;
	sf->invalidem = 1;
    }
    if ( em==-1 ) {
	LogError(_("This font does not specify unitsPerEm, so we guess 1000."));
	em = 1000;
    }
    sf->ascent = as; sf->descent = ds;
    // Ascent and descent are set, so we can parse the guidelines now.
    if (guidelineNode)
	UFOLoadGuidelines(sf, NULL, 0, doc, guidelineNode, NULL, &lastglspl);
    if ( sf->fontname==NULL ) {
	if ( stylename!=NULL && sf->familyname!=NULL )
	    sf->fontname = strconcat3(sf->familyname,"-",stylename);
	else
	    sf->fontname = copy("Untitled");
    }
    if ( sf->fullname==NULL ) {
	if ( stylename!=NULL && sf->familyname!=NULL )
	    sf->fullname = strconcat3(sf->familyname," ",stylename);
	else
	    sf->fullname = copy(sf->fontname);
    }
    if ( sf->familyname==NULL )
	sf->familyname = copy(sf->fontname);
    free(stylename); stylename = NULL;
    if (sf->styleMapFamilyName == NULL)
        sf->styleMapFamilyName = ""; // Empty default to disable fallback at export (not user-accessible anyway as of now).
    if ( sf->weight==NULL )
	sf->weight = copy("Regular");
    // We can now free the document.
    if ( doc!=NULL )
        xmlFreeDoc(doc);
    // We first try to set the SplineFont version by using the native numeric U. F. O. values.
    if ( sf->version==NULL && versionMajor != -1 )
      injectNumericVersion(&sf->version, versionMajor, versionMinor);
    // If that fails, we attempt to use the TrueType values.
    if ( sf->version==NULL && sf->names!=NULL &&
	    sf->names->names[ttf_version]!=NULL &&
	    strncmp(sf->names->names[ttf_version],"Version ",8)==0 )
	sf->version = copy(sf->names->names[ttf_version]+8);

	char * layercontentsname = buildname(basedir,"layercontents.plist");
	char ** layernames = NULL;
	if (layercontentsname == NULL) {
		switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
		return( NULL );
	} else if ( GFileExists(layercontentsname)) {
		xmlDocPtr layercontentsdoc = NULL;
		xmlNodePtr layercontentsplist = NULL;
		xmlNodePtr layercontentsdict = NULL;
		xmlNodePtr layercontentslayer = NULL;
		xmlNodePtr layercontentsvalue = NULL;
		int layercontentslayercount = 0;
		int layernamesbuffersize = 0;
		int layercontentsvaluecount = 0;
		if ( (layercontentsdoc = xmlParseFile(layercontentsname)) ) {
			// The layercontents plist contains an array of double-element arrays. There is no top-level dict. Note that the indices in the layercontents array may not match those in the Fontforge layers array due to reserved spaces.
			if ( ( layercontentsplist = xmlDocGetRootElement(layercontentsdoc) ) && ( layercontentsdict = FindNode(layercontentsplist->children,"array") ) ) {
				layercontentslayercount = 0;
				layernamesbuffersize = 2;
				layernames = malloc(2*sizeof(char*)*layernamesbuffersize);
				// Look through the children of the top-level array. Stop if one of them is not an array. (Ignore text objects since these probably just have whitespace.)
				for ( layercontentslayer = layercontentsdict->children ;
				( layercontentslayer != NULL ) && ( ( xmlStrcmp(layercontentslayer->name,(const xmlChar *) "array")==0 ) || ( xmlStrcmp(layercontentslayer->name,(const xmlChar *) "text")==0 ) ) ;
				layercontentslayer = layercontentslayer->next ) {
					if ( xmlStrcmp(layercontentslayer->name,(const xmlChar *) "array")==0 ) {
						xmlChar * layerlabel = NULL;
						xmlChar * layerglyphdirname = NULL;
						layercontentsvaluecount = 0;
						// Look through the children (effectively columns) of the layer array (the row). Reject non-string values.
						for ( layercontentsvalue = layercontentslayer->children ;
						( layercontentsvalue != NULL ) && ( ( xmlStrcmp(layercontentsvalue->name,(const xmlChar *) "string")==0 ) || ( xmlStrcmp(layercontentsvalue->name,(const xmlChar *) "text")==0 ) ) ;
						layercontentsvalue = layercontentsvalue->next ) {
							if ( xmlStrcmp(layercontentsvalue->name,(const xmlChar *) "string")==0 ) {
								if (layercontentsvaluecount == 0) layerlabel = xmlNodeListGetString(layercontentsdoc, layercontentsvalue->xmlChildrenNode, true);
								if (layercontentsvaluecount == 1) layerglyphdirname = xmlNodeListGetString(layercontentsdoc, layercontentsvalue->xmlChildrenNode, true);
								layercontentsvaluecount++;
								}
						}
						// We need two values (as noted above) per layer entry and ignore any layer lacking those.
						if ((layercontentsvaluecount > 1) && (layernamesbuffersize < INT_MAX/2)) {
							// Resize the layer names array as necessary.
							if (layercontentslayercount >= layernamesbuffersize) {
								layernamesbuffersize *= 2;
								layernames = realloc(layernames, 2*sizeof(char*)*layernamesbuffersize);
							}
							// Fail silently on allocation failure; it's highly unlikely.
							if (layernames != NULL) {
								layernames[2*layercontentslayercount] = copy((char*)(layerlabel));
								if (layernames[2*layercontentslayercount]) {
									layernames[(2*layercontentslayercount)+1] = copy((char*)(layerglyphdirname));
									if (layernames[(2*layercontentslayercount)+1])
										layercontentslayercount++; // We increment only if both pointers are valid so as to avoid read problems later.
									else
										free(layernames[2*layercontentslayercount]);
								}
							}
						}
						if (layerlabel != NULL) { xmlFree(layerlabel); layerlabel = NULL; }
						if (layerglyphdirname != NULL) { xmlFree(layerglyphdirname); layerglyphdirname = NULL; }
					}
				}
				{
					// Some typefaces (from very reputable shops) identify as following version 2 of the U. F. O. specification
					// but have multiple layers and a layercontents.plist and omit the foreground layer from layercontents.plist.
					// So, if the layercontents.plist includes no foreground layer and makes no other use of the directory glyphs
					// and if that directory exists within the typeface, we map it to the foreground.
					// Note that FontForge cannot round-trip this anomaly at present and shall include the foreground in
					// layercontents.plist in any exported U. F. O..
					int tmply = 0; // Temporary layer index.
					while (tmply < layercontentslayercount && strcmp(layernames[2*tmply], "public.default") &&
					  strcmp(layernames[2*tmply+1], "glyphs")) tmply ++;
					// If tmply == layercontentslayercount then we know that no layer was named public.default and that no layer
					// used the glyphs directory.
					char * layerpath = buildname(basedir, "glyphs");
					if (tmply == layercontentslayercount && layerpath != NULL && GFileExists(layerpath)) {
						layercontentsvaluecount = 2;
						// Note the copying here.
						xmlChar * layerlabel = (xmlChar*)"public.default";
						xmlChar * layerglyphdirname = (xmlChar*)"glyphs";
						// We need two values (as noted above) per layer entry and ignore any layer lacking those.
						if ((layercontentsvaluecount > 1) && (layernamesbuffersize < INT_MAX/2)) {
							// Resize the layer names array as necessary.
							if (layercontentslayercount >= layernamesbuffersize) {
								layernamesbuffersize *= 2;
								layernames = realloc(layernames, 2*sizeof(char*)*layernamesbuffersize);
							}
							// Fail silently on allocation failure; it's highly unlikely.
							if (layernames != NULL) {
								layernames[2*layercontentslayercount] = copy((char*)(layerlabel));
								if (layernames[2*layercontentslayercount]) {
									layernames[(2*layercontentslayercount)+1] = copy((char*)(layerglyphdirname));
									if (layernames[(2*layercontentslayercount)+1])
										layercontentslayercount++; // We increment only if both pointers are valid so as to avoid read problems later.
									else
										free(layernames[2*layercontentslayercount]);
								}
							}
						}
					}
					if (layerpath != NULL) { free(layerpath); layerpath = NULL; }
				}

				if (layernames != NULL) {
					int lcount = 0;
					int auxpos = 2;
					int layerdest = 0;
					int bg = 1;
					if (layercontentslayercount > 0) {
						// Start reading layers.
						for (lcount = 0; lcount < layercontentslayercount; lcount++) {
							// We refuse to load a layer with an incorrect prefix.
                                                	if (
							(((strcmp(layernames[2*lcount],"public.default")==0) &&
							(strcmp(layernames[2*lcount+1],"glyphs") == 0)) ||
							(strstr(layernames[2*lcount+1],"glyphs.") == layernames[2*lcount+1])) &&
							(glyphdir = buildname(basedir,layernames[2*lcount+1]))) {
                                                        	if ((glyphlist = buildname(glyphdir,"contents.plist"))) {
									if ( !GFileExists(glyphlist)) {
										LogError(_("No glyphs directory or no contents file"));
									} else {
										// Only public.default gets mapped as a foreground layer.
										bg = 1;
										// public.default and public.background have fixed mappings. Other layers start at 2.
										if (strcmp(layernames[2*lcount],"public.default")==0) {
											layerdest = ly_fore;
											bg = 0;
										} else if (strcmp(layernames[2*lcount],"public.background")==0) {
											layerdest = ly_back;
										} else {
											layerdest = auxpos++;
										}

										// We ensure that the splinefont layer list has sufficient space.
										if ( layerdest+1>sf->layer_cnt ) {
 										    sf->layers = realloc(sf->layers,(layerdest+1)*sizeof(LayerInfo));
										    memset(sf->layers+sf->layer_cnt,0,((layerdest+1)-sf->layer_cnt)*sizeof(LayerInfo));
										    sf->layer_cnt = layerdest+1;
										}

										// The check is redundant, but it allows us to copy from sfd.c.
										if (( layerdest<sf->layer_cnt ) && sf->layers) {
											if (sf->layers[layerdest].name)
												free(sf->layers[layerdest].name);
											sf->layers[layerdest].name = strdup(layernames[2*lcount]);
											if (sf->layers[layerdest].ufo_path)
												free(sf->layers[layerdest].ufo_path);
											sf->layers[layerdest].ufo_path = strdup(layernames[2*lcount+1]);
											sf->layers[layerdest].background = bg;
											// Fetch glyphs.
											UFOLoadGlyphs(sf,glyphdir,layerdest);
											// Determine layer spline order.
											sf->layers[layerdest].order2 = SFLFindOrder(sf,layerdest);
											// Conform layer spline order (reworking control points if necessary).
											SFLSetOrder(sf,layerdest,sf->layers[layerdest].order2);
											// Set the grid order to the foreground order if appropriate.
											if (layerdest == ly_fore) sf->grid.order2 = sf->layers[layerdest].order2;
										}
									}
									free(glyphlist);
								}
								free(glyphdir);
							}
						}
					} else {
						LogError(_("layercontents.plist lists no valid layers."));
					}
					// Free layer names.
					for (lcount = 0; lcount < layercontentslayercount; lcount++) {
						if (layernames[2*lcount]) free(layernames[2*lcount]);
						if (layernames[2*lcount+1]) free(layernames[2*lcount+1]);
					}
					free(layernames);
				}
			}
			xmlFreeDoc(layercontentsdoc);
		}
	} else {
		glyphdir = buildname(basedir,"glyphs");
    	glyphlist = buildname(glyphdir,"contents.plist");
    	if ( !GFileExists(glyphlist)) {
			LogError(_("No glyphs directory or no contents file"));
    	} else {
			UFOLoadGlyphs(sf,glyphdir,ly_fore);
			sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = sf->grid.order2 =
		    SFFindOrder(sf);
   	    	SFSetOrder(sf,sf->layers[ly_fore].order2);
		}
	    free(glyphlist);
		free(glyphdir);
	}
	free(layercontentsname);

    sf->map = EncMap1to1(sf->glyphcnt);

    UFOHandleGroups(sf, basedir);

    UFOHandleKern3(sf,basedir,0);
    UFOHandleKern3(sf,basedir,1);

    /* Might as well check for feature files even if version 1 */
    temp = buildname(basedir,"features.fea");
    if ( GFileExists(temp))
	SFApplyFeatureFilename(sf,temp);
    free(temp);

#ifndef _NO_PYTHON
    temp = buildname(basedir,"lib.plist");
    doc = NULL;
    if ( GFileExists(temp))
	doc = xmlParseFile(temp);
    free(temp);
    if ( doc!=NULL ) {
		plist = xmlDocGetRootElement(doc);
		dict = NULL;
		if ( plist!=NULL )
			dict = FindNode(plist->children,"dict");
		if ( plist==NULL ||
			xmlStrcmp(plist->name,(const xmlChar *) "plist")!=0 ||
			dict==NULL ) {
			LogError(_("Expected property list file"));
		} else {
			sf->python_persistent = LibToPython(doc,dict,1);
			sf->python_persistent_has_lists = 1;
		}
		xmlFreeDoc(doc);
    }
#endif
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( sf );
}

SplineSet *SplinePointListInterpretGlif(SplineFont *sf,char *filename,char *memory, int memlen,
	int em_size,int ascent,int is_stroked) {
    xmlDocPtr doc;
    SplineChar *sc;
    SplineSet *ss;

    if ( !libxml_init_base()) {
	LogError(_("Can't find libxml2."));
return( NULL );
    }
    if ( filename!=NULL )
	doc = xmlParseFile(filename);
    else
	doc = xmlParseMemory(memory,memlen);
    if ( doc==NULL )
return( NULL );

    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    setlocale(LC_NUMERIC,"C");
    sc = _UFOLoadGlyph(sf,doc,filename,NULL,NULL,ly_fore);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.

    if ( sc==NULL )
return( NULL );

    ss = sc->layers[ly_fore].splines;
    sc->layers[ly_fore].splines = NULL;
    SplineCharFree(sc);
return( ss );
}

int HasUFO(void) {
return( libxml_init_base());
}
