grafica.png: plot.py data.txt
	python plot.py
data.txt: placas
	qsub submit_job.sh
placas:
	mpicc placas.c -o placas
clean:
	rm -f *png
	rm *.txt
