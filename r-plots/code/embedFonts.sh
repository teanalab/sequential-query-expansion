#! /bin/bash

for origEpsFileFullName in ../images/*.eps; do
	filename=$(basename "$origEpsFileFullName")
	extension="${filename##*.}"
	origEpsFileName="${filename%.*}"
	echo ${origEpsFileName}
	epstopdf ${origEpsFileFullName}

	pdfname=../images/${origEpsFileName}.pdf
	newfilename=../images/${origEpsFileName}_.pdf

	gs -dSAFER -dNOPLATFONTS -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sPAPERSIZE=letter -dCompatibilityLevel=1.4 -dPDFSETTINGS=/printer -dCompatibilityLevel=1.4 -dMaxSubsetPct=100 -dSubsetFonts=true -dEmbedAllFonts=true -sOutputFile=${newfilename} -f ${pdfname}

	
	pdftops -eps -level2 ${newfilename} ${origEpsFileFullName}
	
	rm -f $pdfname
	rm -f $newfilename
done
