#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Kuka export lib (c) Jürgen Riegel 2009 LGPL 2.1 or higher

import time

TeachPointFold = """
;FOLD LIN P4  Vel= 0.2 m/s CPDAT1 Tool[1] Base[0];%{PE}%R 5.4.27,%MKUKATPBASIS,%CMOVE,%VLIN,%P 1:LIN, 2:P4, 3:, 5:0.2, 7:CPDAT1
$BWDSTART = FALSE
LDAT_ACT=LCPDAT1
FDAT_ACT=FP4
BAS(#CP_PARAMS,0.2)
LIN XP4 
;ENDFOLD
"""

TeachPointDat = """
DECL E6POS XP4={X -25.1844196,Y 1122.42603,Z 1158.07996,A -14.3267002,B 0.537901878,C 179.028305,S 6,T 59,E1 0.0,E2 0.0,E3 0.0,E4 0.0,E5 0.0,E6 0.0}
DECL FDAT FP4={TOOL_NO 1,BASE_NO 0,IPO_FRAME #BASE,POINT2[] " "}
DECL LDAT LCPDAT1={VEL 2.0,ACC 100.0,APO_DIST 100.0,APO_FAC 50.0,ORI_TYP #VAR}
"""

HeaderSrc = """&ACCESS RVP
&REL 1
&PARAM TEMPLATE = C:\KRC\Roboter\Template\ExpertVorgabe
&PARAM EDITMASK = * 
"""


def ExportCompactSub(Rob,Trak,FileName):
	print(Rob,Trak,FileName)
	Traj = Trak.Trajectory
	# open the output file
	SrcFile = open(FileName,'w')
	# header
	SrcFile.write(HeaderSrc)
	# subroutine definition
	SrcFile.write("DEF "+Trak.Name+"( )\n\n")
	SrcFile.write(";- Kuka src file, generated by FreeCAD (https://www.freecad.org)\n")
	SrcFile.write(";- "+ time.asctime()+"\n\n")
	# defining world and base
	SrcFile.write(";------------- definitions ------------\n")
	SrcFile.write("EXT BAS (BAS_COMMAND :IN,REAL :IN ) ;set base to World\n")
	SrcFile.write("BAS (#INITMOV,0 ) ;Initialicing the defaults for Vel and so on \n\n")
	
	SrcFile.write("\n;------------- main part ------------\n")

	for w in Traj.Waypoints:
		(X,Y,Z) = (w.Pos.Base.x,w.Pos.Base.x,w.Pos.Base.x)
		(A,B,C) = (w.Pos.Rotation.toEuler())
		V = w.Velocity / 1000.0 # from mm/s to m/s
		SrcFile.write("$VEL.CP = %f ; m/s ; m/s \n"%V)
		SrcFile.write("LIN {X %.3f,Y %.3f,Z %.3f,A %.3f,B %.3f,C %.3f} ; %s\n"%(w.Pos.Base.x,w.Pos.Base.y,w.Pos.Base.z,A,B,C,w.Name))
	
	# end of subroutine
	SrcFile.write("\n;------------- end ------------\n")
	SrcFile.write("END \n\n")

	SrcFile.close()
	# open the output file
	#DatFile = open(FileName[:-4]+'.dat','w')
	# header
	#DatFile.write(HeaderSrc)
	# subroutine definition
	#DatFile.write("DEFDAT "+Trak.Name+" PUBLIC\n\n")
	#DatFile.write("ENDDAT\n")
	#DatFile.close()
	
def ExportFullSub(Rob,Trak,FileName):
	print(Trak,FileName)

