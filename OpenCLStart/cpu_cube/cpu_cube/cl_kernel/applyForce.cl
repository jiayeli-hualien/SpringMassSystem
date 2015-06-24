int vertex_index(int i, int j, int k, int size_j, int size_k)
{
	return (((i*size_j)+j)*size_k+k)*4;
}
__kernel
void applyForce(float frameT,
__global float *force,
__global float *velositySrc,
__global float *velosityDest)
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
	
	//velosityDest[index + 1] -= 0.01f;
	
	velosityDest[index + 3] = velositySrc[index + 3];//copy mass
	velosityDest[index] = velositySrc[index] + force[index] * frameT / velositySrc[index + 3];
	velosityDest[index + 1] = velositySrc[index + 1] + force[index + 1] * frameT / velositySrc[index + 3];
	velosityDest[index + 2] = velositySrc[index + 2] + force[index + 2] * frameT / velositySrc[index + 3];
};