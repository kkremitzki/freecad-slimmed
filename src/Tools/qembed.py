# FreeCAD qembed script to embed arbitrary text into an executable
# (c) 2006 Werner Mayer
#
# Creates a C++ header file of arbitrary text files that gets compiled into 
# an executable. It's a reimplementation of Qt's qembed tool in Python

#***************************************************************************
#*   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

#!/usr/bin/python
import os,sys,string

def convertFileNameToCIdentifier(s):
    r = ''
    t = s
    if (len(t) > 0):
        if (t[0].isalpha()==False):
            r='_'
            t=t[1:]
        for i in t:
            if (i.isalnum()==False):
                i='_'
            r=r+i
    return r

def embedData(input, output):
    i = 0
    nbytes=len(input)
    s=''
    while(i<nbytes):
        if (i%14==0):
            s=s+"\n    "
            output.write(s)
            s=''
        v=ord(input[i])
        a=(v >> 4) & 15
        b=v&15
        s=s+"0x"
        s=s+(hex(a))[2]
        s=s+(hex(b))[2]
        if (i<nbytes-1):
            s=s+','
        i=i+1
    if (len(s)>0):
        output.write(s)

if(len(sys.argv) > 4 or len(sys.argv) < 2):
    sys.stderr.write("Wrong Parameter\n  Usage:\n  qembed Infile [Outfile][Identifier]\n")
    sys.exit()

file = open(sys.argv[1])

if(len(sys.argv) > 2):
    out = open(sys.argv[2],"w");
else:
    out = sys.stdout

# Filenamw without path separators
if (len(sys.argv) > 3):
    filenm = sys.argv[3]+"_h"
else:
    filenm = file.name
    
p=string.rfind(filenm,'\\')
filenm=filenm[p+1:]
p=string.rfind(filenm,'/')
filenm=filenm[p+1:]

basenm = convertFileNameToCIdentifier(filenm)
define = basenm.upper()

input=file.read()

out.write("#ifndef _"+define+"_\n")
out.write("#define _"+define+"_\n")

out.write("static const unsigned int  "+basenm+"_len = "+str(len(input))+";\n")
out.write("static const unsigned char "+basenm+"_data[] = {")
embedData(input,out)
out.write("\n};\n\n")

"""
out.write(
"/* Generated by qembed */\n"
"#include <qcstring.h>\n"
"#include <qdict.h>\n"
"static struct Embed {\n"
"    unsigned int size;\n"
"    const unsigned char *data;\n"
"    const char *name;\n"
"} embed_vec[] = {\n"
"    { 74029, "+basenm+"_data, \""+filenm+"\" },\n"
"    { 0, 0, 0 }\n"
"};\n"
"\n"
"static const QByteArray& qembed_findData( const char* name )\n"
"{\n"
"    static QDict<QByteArray> dict;\n"
"    QByteArray* ba = dict.find( name );\n"
"    if ( !ba ) {\n"
"	for ( int i = 0; embed_vec[i].data; i++ ) {\n"
"	    if ( strcmp(embed_vec[i].name, name) == 0 ) {\n"
"		ba = new QByteArray;\n"
"		ba->setRawData( (char*)embed_vec[i].data,\n"
"				embed_vec[i].size );\n"
"		dict.insert( name, ba );\n"
"		break;\n"
"	    }\n"
"	}\n"
"	if ( !ba ) {\n"
"	    static QByteArray dummy;\n"
"	    return dummy;\n"
"	}\n"
"    }\n"
"    return *ba;\n"
"}\n"
"\n")
"""

out.write("#endif // _"+define+"_\n")
