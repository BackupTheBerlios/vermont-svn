.PHONY: all clean

all: vermont-paper.pdf

vermont-paper.pdf: meta-paper vermont-paper.bib IEEEtran.bst IEEEtran.cls
	pdflatex -interaction batchmode vermont-paper
	bibtex -terse vermont-paper
	pdflatex -interaction batchmode vermont-paper
	pdflatex vermont-paper

meta-paper: vermont-paper.tex 0_abstract.tex 1_intro.tex 2_related.tex 3_arch.tex 4_eval.tex 5_conclusion.tex

test: vermont-paper.tex vermont-paper.bib IEEEtran.bst IEEEtran.cls
	latex -interaction errorstopmode -file-line-error -halt-on-error vermont-paper

run: vermont-paper.pdf
	gpdf vermont-paper.pdf

distclean: clean
	rm -f *~
clean:
	rm -f vermont-paper.aux
	rm -f vermont-paper.bbl
	rm -f vermont-paper.blg
	rm -f vermont-paper.dvi
	rm -f vermont-paper.log
	rm -f vermont-paper.pdf
	rm -f vermont-paper.out
	rm -f vermont-paper.tcp
	rm -f vermont-paper.tps

package:
	( cd .. && tar cjf vermont-paper.tar.bz2 vermont-paper )
