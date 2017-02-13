#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Some parameters to use in the code
int L = 5, d = 1, l = 2, V0 = 100.00;
int m = 128, N;
double h;

// Functions 
int transformer(int i, int j);
double *init(int x0, int x1, int y0, int y1, double *array);
double *init_alt(int row, int col, double *array);

int main(int argc, char** argv){

   	MPI_Init(NULL, NULL);
		
	// Get the number of processes  
   	int world_size;
   	MPI_Comm_size(MPI_COMM_WORLD, &world_size); 
   	
   	// Get the rank of the process
   	int world_rank;
   	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
   	
   	//We have to ckeck if the number of processors is allowed : # must be 2^n n = 1,2,3,4
	if(world_rank==0)
	{
		if (world_size != 2 & world_size != 4 & world_size != 8 & world_size != 16 & world_size != 1)
		{
			printf("\n\n\n%d is not a number of processors allowed", world_size);
			fprintf(stderr, "\nPlease run again with some of these numbers : 2, 4, 6, 8 \n\n");
			abort();
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		printf("\n\n\nWe have %d processors avaliables\n", world_size);
	} 
	
	int up, down, left, right, x0, x1, y0, y1, i, j, k, n=0;
   	double average;
   	N = 2*m*m;
   	h = (double)L/(double)m;   		

	x0 = m/2 - (double)l/(double)L*m;
	x1 = m/2 + (double)l/(double)L*m;
	y0 = m/2 - (double)d/(double)L*m;
	y1 = m/2 + (double)d/(double)L*m;
	
	// Create the total array in each process
	double *V 		= malloc(m*m*sizeof(double));
	double *V_new 	= malloc(m*m*sizeof(double));
	V 		= init(x0, x1, y0, y1, V);
	V_new 	= init(x0, x1, y0, y1, V);
	
	double l_send,r_send,l_recv, r_recv, send, recv, test;
	
	MPI_Barrier( MPI_COMM_WORLD );
	int sub_m = (m/world_size);				// Size of each sub part of the total grid in each process

	int initial, final;
	if (world_rank == 0)
	{
		initial = 1;
		final 	= sub_m-1;
	}
	else if (world_rank == world_size-1)
	{
		initial = (sub_m * world_rank)   ;
		final 	= m -2;	
	}		
	else
	{
		initial = sub_m * world_rank  ;	
		final 	= sub_m * (world_rank +1) - 1 ; 
	}
	int size 	= final - initial + 1;

	while (n < N)
	{	
		// We divide the grid in equal-size parts according to the number of processes 	
		for(j = initial; j <= final; j++)
		{
			for(i = 1; i < m; i++)
			{				
				up 		= transformer(i-1, j);
				down 	= transformer(i+1, j);
				left 	= transformer(i, j-1);
				right 	= transformer(i, j+1);
				if (!(j >= x0 && j <= x1 && i == y0) && !(j >= x0 && j <= x1 && i == y1))
				{	
					average = (V[up] + V[down] + V[left] + V[right])/4;
					V_new[transformer(i,j)] = average;
				}
			}
		}
			
		MPI_Barrier( MPI_COMM_WORLD );
		
		for(j = initial; j <= final; j++)
		{
			for(i = 1; i < m; i++)
			{				
				V[transformer(i,j)] = V_new[transformer(i,j)];	
			}
		}
								
		// Preparing data to send between process
		if(world_rank == 0)
		{ 
			for (i = 0; i < m; i++)
			{
				r_send = V_new[transformer(i,final)];
				MPI_Send(&r_send, 1, MPI_DOUBLE, world_rank+1, m + i, MPI_COMM_WORLD);
			}
		}
		else if(world_rank == world_size-1)
		{
			for (i = 0; i < m; i++)
			{
				l_send = V_new[transformer(i,initial)];
				MPI_Send(&l_send, 1, MPI_DOUBLE, world_rank-1, i, MPI_COMM_WORLD);
			}	
		}
		else
		{
			for (i = 0; i < m; i++)
			{
				l_send = V_new[transformer(i,initial)];
				r_send = V_new[transformer(i,final)];
				MPI_Send(&r_send, 1, MPI_DOUBLE, world_rank+1, m+i, MPI_COMM_WORLD);
				MPI_Send(&l_send, 1, MPI_DOUBLE, world_rank-1, i, MPI_COMM_WORLD);
			}	
		}
		
		MPI_Barrier( MPI_COMM_WORLD );

		// Recieve information between process
		if(world_rank == 0)
		{	
			for (i = 0; i < m; i++)
			{
				MPI_Recv(&r_recv, 1, MPI_DOUBLE, world_rank+1, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				V[transformer(i,final+1)] = r_recv;

			}
		}
		else if(world_rank == world_size-1)
		{
			for (i = 0; i < m; i++)
			{
				MPI_Recv(&l_recv, 1, MPI_DOUBLE, world_rank-1, m + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				V[transformer(i,initial-1)] = l_recv;
			}
		}
		else
		{	
			for (i = 0; i < m; i++)
			{
				MPI_Recv(&l_recv, 1, MPI_DOUBLE, world_rank-1, m+i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Recv(&r_recv, 1, MPI_DOUBLE, world_rank+1,   i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				V[transformer(i,initial-1)] 	= l_recv;
				V[transformer(i,final+1)] 		= r_recv;
			}
		}
				
		MPI_Barrier( MPI_COMM_WORLD );

		n += 1;
		if( world_rank == 0)
		{
			printf("n : %d from %d\n",n, N);
		}
	}
	
	// Receive all data to one node. 

	MPI_Barrier( MPI_COMM_WORLD );

	if(world_rank != 0)
	{
		for(j = initial; j <= final; j++)
		{
			for(i = 1; i < m; i++)
			{				
				send = V_new[transformer(i,j)];
				MPI_Send(&send, 1, MPI_DOUBLE,0, transformer(i,j), MPI_COMM_WORLD);
			}
		}
	}
	else
	{	
		for (k = 1;k < world_size;k++)
		{ 
			int ini, fin;
			if (k == world_size-1)
			{
				ini = (sub_m * k)   ;
				fin = m -2;	
			}		
			else
			{
				ini = sub_m * k  ;	
				fin = sub_m * (k +1) - 1 ; 
			}		
			for(j = ini; j <= fin; j++)
			{
				for(i = 1; i < m; i++)
				{				
					MPI_Recv(&recv, 1, MPI_DOUBLE, k,transformer(i,j), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					V[transformer(i,j)] = recv;
				}
			}
		}			
	}			

	if (world_rank == 0 ){
	   	FILE *f = fopen("data.txt", "w");

		for(i=0;i < m; i++)
		{
			for(j=0;j < m; j++)
			{
				fprintf(f,"%f\n", V[transformer(i,j)]);
			}
		}
		fclose(f);
	}
	MPI_Finalize();	

	return(0);
}

int transformer(int i, int j){	
	
	return j*m + i;
}

double *init(int x0, int x1, int y0, int y1, double *array){	
	
	int a;
	for(a = x0; a <= x1; a++){
		array[transformer(y0, a)] = V0/2;
		array[transformer(y1, a)] = -V0/2;
	}
	return array;
}

double *init_alt(int row, int col, double *array){	
	
	int i,j;
	for(j = 0; j < col; j++){
		for(i = 0; i < row; i++){
			array[transformer(i, j)] = transformer(i, j);
		}
	}
	return array;
}