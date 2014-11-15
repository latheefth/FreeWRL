#include <stdlib.h>
#include <stdio.h>
#include <math.h>


int uniformKnot(int n, int p, float *U){
	int j, k, m, mm;
	float uniform;
	m = n + p + 1;
	k = 0;
	uniform = 1.0f/(float)(n-p);
	for(j=0;j<p;k++,j++){
		U[k] = 0.0f;
	}
	mm = n - p + 1;
	for(j=0;j<mm;j++,k++){
		U[k] = j*uniform;
	}
	for(j=0;j<p;j++,k++){
		U[k] = 1.0f;
	}
	U[8] = 1.0f;
	printf("U= ");
	for(j=0;j<m+1;j++){
		printf(" U[%d]=%f",j,U[j]);
	}
	return 1;
}

//ALGORITHM A2.1 p.68 Piegl
int FindSpan(int n, int p, float u, float *U)
{
	/*	Determine the knot span index: where u is in U[i]
		Input:
			n - # of control points == m - p - 1
			p - degree of curve = power + 1 ie linear 2, quadratic 3, cubic 4
			U - knot vector [0 ... m-1]
			u - scalar curve parameter in range u0 - um
		Return:
			knot span index ie if u is between U[i] and U[i+1] return i
	*/
	if(0){
		//dug algo, simpler search
		int i, span;
		span = -1;
		for(i=p;i<n;i++){
			if(u >= U[i] && u < U[i+1]){
				span = i;
				break;
			}
		}
		if(span < 0){
			span = n-1; //ending condition ie when u=1.0 it will divide by zero elsewhere when span length is zero, so keep it on a previous span
		}
		return span;
	}else{
		int low, high, mid;
		//if(u == U[n+1]) return n;
		if(u == U[n]) return n-1;  //this prevents divide by zero when u = 1
		low = p; high = n+1; mid = (low+high)/2;
		while(u < U[mid] || u >= U[mid+1]){
			if(u < U[mid]) high = mid;
			else low = mid;
			mid = (low + high)/2;
		}
		return mid;
	}
}
//ALGORITHM A2.2 p.70 Piegl
int BasisFuns(int span, float u, int p, float *U, float *N){
	/* Compute the non-vanishing Basis functions
		Input:
			span = knot span: which knots is this u in between: if between U[i] and U[i+1], span == i
			u - scalar curve parameter in range u0 - um
			p - degree of curve = power + 1 ie linear 2, quadratic 3, cubic 4
			U - knot vector [0 ... m-1]
		Output:
			N - precomputed rational bernstein basis functions for a given span
				- these are blending weights that say how much of each surrounding control point is used in a given span
	*/
	int j, r;
	float left[5], right[5], saved, temp;
	//float testzero;
	N[0] =1.0f;
	for(j=1;j<=p;j++){
		left[j] = u - U[span+1 - j];
		right[j] = U[span+j] - u;
		saved = 0.0f;
		for(r=0;r<j;r++){
			//testzero = right[r+1]+left[j-r];
			//if(fabs(testzero) < .00001) 
			//	printf("ouch divide by zero\n");
			temp = N[r]/(right[r+1]+left[j-r]);
			N[r] = saved + right[r+1]*temp;
			saved = left[j-r]*temp;
		}
		N[j] = saved;
	}
	return 1;
}

//ALGORITHM A4.1 p.124 Piegl
int CurvePoint(int n, int p, float* U, float *Pw, float u, float *C )
{
	/*	Compute point on rational B-spline curve
		Input:
			n - # of control points == m - p - 1
			p - degree of curve linear 1, quadratic 2, cubic 3
			U[] - knot vector [0 ... m], m = n + p + 1
			Pw[] - control point vector 
				where w means rational/homogenous: Pw[i] = {wi*xi,wi*yi,wi*zi,wi}
			u - scalar curve parameter in range u0 - um
		Output:
			C - 3D point = Cw/w
		Internal:
			span = knot span: which knots is this u in between: if between U[i] and U[i+1], span == i
			N[] - precomputed rational bernstein basis functions for a given span
				- these are blending weights that say how much of each surrounding control point is used in a given span
			w - weight, assuming it's uniform
	*/
	if(0){
		 //u == 1.0f){
		 //don't need - fixed in findspan
		int i;
		for(i=0;i<3;i++)
			C[i] = Pw[(n-1)*4 + i]/Pw[(n-1)*4 + 3];
	}else{
		int span,i,j;
		float N[100], w;
		float Cw[4];
		span = FindSpan(n,p,u,U);
		BasisFuns(span,u,p,U,N);
		w = 1.0f;
		for(i=0;i<4;i++) Cw[i] = 0.0f;
		//Cw[3] = w;
		for(j=0;j<=p;j++){
			for(i=0;i<4;i++){
				Cw[i] += N[j]*Pw[(span-p+j)*4 + i];
			}
		}
		for(i=0;i<3;i++)
			C[i] = Cw[i]/Cw[3];
	}
	return 1;
}

//ALGORITHM A4.3 p.134 Piegl
int SurfacePoint(int n,int p,float *U,int m, int q,float *V,float *Pw,float u,float v,float *S)
{
	/*	Compute point on rational B-Spline surface S(u,v)
		Input:
			u direction:
				n - # of control points 
				p - degree of curve linear 1, quadratic 2, cubic 3
				U[] - knot vector [0 ...  n + p + 1]
				u - scalar curve parameter 
			v direction:
				m - # of control points 
				q - degree of curve linear 1, quadratic 2, cubic 3
				V[] - knot vector [0 ... m + q + 1]
				v - scalar curve parameter 
			Pw[] - control point vector 
				where w means rational/homogenous: Pw[i] = {wi*xi,wi*yi,wi*zi,wi}
		Output:
			S - output 3D point = Sw/w
	*/
	int uspan, vspan, i, l, k;
	float Nu[100], Nv[100], temp[6][4], Sw[4];

	uspan = FindSpan(n,p,u,U);
	BasisFuns(uspan,u,p,U,Nu);
	vspan = FindSpan(m,q,v,V);
	BasisFuns(vspan,v,q,V,Nv);
	for(l=0;l<=q;l++){
		for(i=0;i<4;i++)
			temp[l][i] = 0.0f;
		for(k=0;k<=p;k++){
			//temp[l] += Nu[k]*Pw[uspan-p+k][vspan-q+l];
			for(i=0;i<4;i++)
				temp[l][i] += Nu[k]*Pw[((uspan-p+k)*n + (vspan-q+l))*4 + i];

		}
	}
	for(i=0;i<4;i++) Sw[i] = 0.0f;
	for(l=0;l<=q;l++){
		for(i=0;i<4;i++)
			Sw[i] += Nv[l]*temp[l][i];
	}
	for(i=0;i<3;i++)
		S[i] = Sw[i]/Sw[3];
	return 1;
}