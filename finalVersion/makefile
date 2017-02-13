grafica.png: plot.py data.txt
	python plot.py

data.txt: placas
	qsub submit.job 

placas:
	mpicc placas.c -o placas

direct:
	mpicc placas.c -o placas
	mpiexec -n 4 ./placas 

clean:
	rm -f *txt
	rm -f *png
	rm test_schedule*