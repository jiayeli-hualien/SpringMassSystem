int vertex_index(int i, int j, int k, int size_j, int size_k)
{
	return (((i*size_j)+j)*size_k+k)*4;
}
__kernel
void updatePosition(float frameT,
__global float *velosity,
__global float *positionsSrc,
__global float *positionsDst,
int enableCollision)
{
	 int i = get_global_id(0);
	 int size_i = get_global_size(0);
	 if(i>=size_i)
		 return;
	  
	 int j = get_global_id(1);
	 int size_j = get_global_size(1);
	 if(j>=size_j)
		 return;
	 
	 int size_k = get_global_size(2);
	 int k = get_global_id(2);
	 if(k>=size_k)
		 return;
	 
	int index = vertex_index(i, j, k, size_j, size_k);//x,y,z,mass	
	
	 
	positionsDst[index] = positionsSrc[index] + velosity[index] * frameT;
	positionsDst[index + 1] = positionsSrc[index + 1] + velosity[index + 1] * frameT;
	positionsDst[index + 2] = positionsSrc[index + 2] + velosity[index + 2] * frameT;
	
	if(enableCollision)
	{
		if(positionsDst[index + 1]<=0.0f)
		{
			positionsDst[index + 1]  = -positionsDst[index + 1];
			velosity[index+1]=-velosity[index+1];
		}
	}
}