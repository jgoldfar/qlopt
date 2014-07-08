#include <iostream>
#include <map>

#include <Eigen/LU>
#include <unsupported/Eigen/MPRealSupport>

#include "misc.h"
#include "nonlinear_odes.h"
#include "numerical_integration.h"
#include "thesis_functions.h"

using namespace std;
using namespace mpfr;
using namespace Eigen;

const int bits = 128;
const int bytes = bits/8;

map<string, odeFuncPtr> odeFuncMap;
map<string, qLinFuncPtr> qLinFuncMap;

mat reshape(const mat& U, int n, int m);

int main(int argc, char *argv[])
{
	odeFuncMap["lotka_volterra"] = lotka_volterra;
	qLinFuncMap["lotka_volterra_linearization"] = lotka_volterra_linearization;

	mpreal::set_default_prec(bits);
	
	// Input
	string system = "lotka_volterra";
	vec t(11); 
	t << 0, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1;
	
	vec u(3);
	u << 1,2,1;
	
	vec yNot(2);
	yNot << 1,1;
	
	vec uNot(3);
	uNot << 7, 7, 7;
	//end input
	
	int lt = t.size();  
	int m = u.size();
	int n = yNot.size();

	mat msmt;
	msmt = rungekutta4(odeFuncMap[system], t, u, yNot);
	
	// refactor into a function
	vec zeros(n*m);
	zeros.fill(0);
	vec lyNot((m+1)*n);
	lyNot << yNot, zeros;
	
	mat xNminus(n, lt);
	xNminus << msmt; 
	
	mat bob(lyNot.size(), lt);
	mat U(zeros.size(), n*lt);
	mat A(m,m);
	vec P(m);
	vec du(m);
	system = system + "_linearization";
	
	//Run the rest of the iterations
	for(int i = 0; i<5; i++)
	{
		bob = rungekutta4(qLinFuncMap[system], t, uNot, lyNot, xNminus);

		U = reshape(bob.bottomRows(zeros.size()), m, n*lt);
		xNminus = bob.topRows(n);
		
		A = findA(t, U, m);
		P = findP(t, U, reshape(msmt - xNminus, 1, n*lt).row(0), m);
		
		du =  A.inverse()*P;
		uNot += du;
		
		if(du.norm() < 0.00001){
			break; 
		}
	}
	cout.precision(10);
	cout << uNot << endl;
	//getchar();	
	return 0;
}

mat reshape(const mat& U, int n, int m)
{
	mat newU(n,m);
	newU.fill(0);
	int olt = U.row(0).size();
	int on = m/olt;
	
	for(int i = 0; i<n; i++){
		for(int j = 0; j<on; j++){
			newU.block(i, j*olt, 1, olt) = U.row(i*on + j);
		}
	}
	return newU;
}