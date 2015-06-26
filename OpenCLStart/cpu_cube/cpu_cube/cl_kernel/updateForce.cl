
int vertex_index(int i, int j, int k, int size_j, int size_k)
{
	return (((i*size_j)+j)*size_k+k)*4;
}

void springForce(float *sumForce, float *p1, float *p2, float *v1, float *v2, float dis, float springK, float springDampK);

__kernel
void updateForce(float frameT,
__global float *positions,
__global float *velosity,
__global float *force,
float gravity, float distance, float springK, float springDampK)
{
	 //Get the index of the work-item
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
	 
	 int index = vertex_index(i,j,k,size_j,size_k);

	 
	 
	 float sumForce[3]={0.0f,0.0f,0.0f};
	 float p1[3] = {positions[index],positions[index+1],positions[index+2]};
	 float v1[3] = {velosity[index], velosity[index+1], velosity[index+2]};
	 for(int s=-1; s<=1; s++)
	 {
		 if(i+s<0||i+s>=size_i)
			 continue;
		 for(int t=-1; t<=1; t++)
		 {
			 if(j+t<0||j+t>=size_j)
				 continue;
			for(int u=-1; u<=1; u++)
			{
				if(k+u<0||k+u>=size_k)
					continue;
				if(s==0&&t==0&&u==0)
					continue;
					
				int indexNN = vertex_index(i+s,j+t,k+u,size_j,size_k);
						
				float p2[3] = {positions[indexNN],positions[indexNN+1],positions[indexNN+2]};
				float v2[3] = {velosity[indexNN], velosity[indexNN+1], velosity[indexNN+2]};
				
				springForce(sumForce,p1,p2,v1,v2,distance*sqrt((float)(abs(s)+abs(t)+abs(u))), springK, springDampK);
			}
		 }
	 }
	
	force[index  ] = sumForce[0];
	force[index+1] = sumForce[1]+gravity*positions[index+3];
	force[index+2] = sumForce[2];	
	
//test
		
		/*
	velosity[index]+=force[index]*frameT;
	velosity[index+1]+=(force[index+1])*frameT;
	velosity[index+2]+=force[index+2]*frameT;
	
	
	positions[index]+=velosity[index]*frameT;
	positions[index+1]+=velosity[index+1]*frameT;
	positions[index+2]+=velosity[index+2]*frameT;

	if(positions[index+1]<0.0f)
		positions[index+1] = 0.0f;*/
}

#define VEC3_SUB(dest, v1, v2) \
dest[0]=v1[0]-v2[0], dest[1]=v1[1]-v2[1], dest[2]=v1[2]-v2[2]

#define VEC3_NORM(v)\
sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])

#define VEC3_DIV(v,d)\
v[0]/=d,v[1]/=d,v[2]/=d

#define VEC3_DOT(v1,v2)\
v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2]

#define VEC3_MUL(v,m)\
v[0]*=m, v[1]*=m, v[2]*=m

#define VEC3_ASSIGN(v1,v2)\
v1[0] = v2[0], v1[1] = v2[1], v1[2] = v2[2]

#define VEC3_ACC(v1,v2)\
v1[0]+=v2[0], v1[1]+=v2[1], v1[2]+=v2[2]

void springForce(float *sumForce, float *p1, float *p2, float *v1, float *v2, float springLen, float springK, float springDampK)
{
	
	float dir[3];
	float diffV[3];
	VEC3_SUB(dir,p1,p2);
	VEC3_SUB(diffV, v1, v2);
	float distance = VEC3_NORM(dir);
	if(distance==0.0f)
		return;
	VEC3_DIV(dir,distance);
	float springForceSize = (distance-springLen)*springK;
	if(springDampK!=0.0f)
	{
		float damp = VEC3_DOT(dir, diffV)*springDampK;
		springForceSize-=damp;
	}
	VEC3_MUL(dir,springForceSize);//calc force
	
	VEC3_ACC(sumForce, dir);//sum force
}