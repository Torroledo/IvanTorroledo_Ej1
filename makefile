grafica.png: plot.py data.txt
	python plot.py
data.txt: placas
	qsub submit.job 
placas:
	mpicc placasVer2.c -o placas
clean:
	rm -f *png
	rm *.txt