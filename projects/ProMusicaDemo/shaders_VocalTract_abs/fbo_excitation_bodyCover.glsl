//Line 865
//=================================================================================================
//	BODY COVER 
//=================================================================================================

//NOTE:AV 
//The following are variables that are needed from previous time step:
//1. F[1]->F[6]
//2. tcos if ramping of pressure is needed 
//3. psg (used in calc_pressure) as it is calculated using new flow (Change for conceptual elegance instead of using PL)
//4. f[jtrch] and b[ieplx] only if no trachea
//5. Else whole f[jtrch] and b[ieplx] of 33 units EACH for trachea! This sucks and life is horrible.
//6. Finally dt is needed. However, this should be available from the system outside easily.
//Look at a set of cells to use just for the excitation for this. Maybe the right most column?

//1. ug, x1, x2, xb are stored in the first simulation pixel
//2. x2dot, x2dot, xbdot are stored in the second pixel
//3. No ramping is needed and can be done from the outside.
//4. Calculation of f[jtrch] and b[ieplx] is moved forward

//Initialize BC
float neq 		= 6;
float F[6];
float A1; // previous supraglottal area
float dt 		= 0; 

//Read from Uniforms
//Rho and Mu are give already
//float c 		= 346.71; // UNIFORM FROM OUTSIDE!
//float rho_c 	= rho*c;  // UNIFORM 
// previous output
vec4 prev_out;  //ug/A1, x1, x2, xb at N
vec4 prev_out_fu;  //x1dot, x2dot, xbdot, A1

//Calculated Values
float x02 		= 0; //Upper Prephonatory Displacement (Value of 0.00179 From BC 1256)
float x01 		= 5.000000000000000e-06; //Lower Prephonatory Displacement (Value of 0.0018 From BC 1256)
float xb0 		= 0.006; //Body prephonatory displacement 
float th0 		= 0; //Prephonatory convergence angle
float T 		= 0.003000000000000; //Thickness of the vocal folds 
float L 		= 0.016000000000000; //Length of the vocal folds 
float Dc 		= 0.003000000000000; //Depth of cover 
float Db 		= 0.002000000000000; //Depth of body 
float zn 		= 0.001250000000000; //Nodal point as measured from the bottom of the vocal folds 
float znot 		= 0; //Ratio of Nodal Point by Thickness
float a1 		= 0; //Glottal entry area 
float a2 		= 0; //Glottal exit area 
float ad 		= 0; //Flow detachment area (T374 A26)
float an 		= 0; //Area at the nodal point 
float ga 		= 0; //Glottal area 
float ca 		= 0; //Contact area 
float m1 		= 6.240000000000001e-05; //Lower mass 
float m2 		= 8.736000000000000e-05; //Upper mass 
float M			= 9.984000000000002e-05; //Body mass
float k1 		= 1.060377116279860e+02; //Lower stiffness 
float k2 		= 1.484527962791804e+02; //Upper stiffness 
float K			= 1.938401939072659e+02; //Body stiffness
float kc 		= 6.880341880341878; //Coupling stiffness 
float B 		= 0.027823015623546; //Body damping coeff
float b1 		= 0.016268685510005; //Lower damping coeff 
float b2 		= 0.136656958284044; //Upper damping coeff 
float p1 		= 0; //Pressure on lower mass 
float p2 		= 0; //Pressure on upper mass 
float pkd 		= 0; //Kinetic Pressure T374
float ph 		= 0; //Hydrostatic pressure 
float f1 		= 0; //Force on lower mass 
float f2 		= 0; //Force on upper mass 
float u 		= 0; //Glottal flow 
float zc 		= 0; //Contact point of Masses
float xc		= 0; //Glottal convergence
float xn		= 0; //XNode, Simplification of A44, T375
float tangent	= 0; //Simplification of A44, T375 
float zd		= 0; //Height of Separation Point
float x1		= 0; //Lower mass displacement
float x2		= 0; //Upper mass displacement
float delta     = 1e-11; //Minimum Area of a1 and a2
float ac		= 0; //VF Contact Area
float ke 		= 0; //Pressure Recovery Coefficient T374 A23

float PL 		= 0; //Respiratory driving pressure from lungs (converted from dyn/cm^2) Can also be 800 
float ps 		= 0; //First segment of Trachea Pressure
float pe 		= 0; //Supraglottal (epilaryngeal) pressure (Just above the glottis)
float psg 		= 0; //Subglottal Pressure for Flow calc. 300-700. Important change from model (Just below the glottis)

//-----------------------------------------------------------------------------------------------
//Trachea and Epilaryngeal area;
//-----------------------------------------------------------------------------------------------
//float Ae      = 100; //Epilaryngeal Area. Should be read as a uniform and stored in ar[0] ieplx
float vtatten 	= 5e-7; //Attenuation Factor Value
float SUB 		= 0; //Inclusion of Subglotal tract //NOTE:AV SUB can be passed as a uniform
//ar holds all the trachea areas in m2 plus supraglottal area in ar[0]
float ar[33] = {
100.0000, // bullshit value that will be overwritten using uniform
0.00022,
0.00021,
0.00021,
0.00022,
0.00023,
0.00024,
0.00025,
0.00026,
0.00028,
0.00029,
0.00030,
0.00031,
0.00031,
0.00032,
0.00032,
0.00032,
0.00032,
0.00032,
0.00032,
0.00031,
0.00030,
0.00028,
0.00026,
0.00024,
0.00021,
0.00017,
0.00014,
0.00010,
0.00007,
0.00004,
0.00003,
0.00003,
};
int Nsect 		= ar.length(); //Number of Subglottal Sections + Epilarynx
int ieplx 		= 0; //Location of Epilarynx
int itrch		= ieplx + 1; //Location of segment just above the lungs
int jtrch 		= Nsect - 1; //Location of segment just below the glottis
//Or just Nsect directly instead of 33
float f[33]; //Forward Pressures
float b[33]; //Backward Pressures
float alpha[33]; //Propagation Factor
float D[32]; //For Kelly-Lochbaum Method -For the interfaces so N-1
float r1[32]; //For Kelly-Lochbaum Method
float r2[32]; //For Kelly-Lochbaum Method
float Psi[16]; //Even and Odd Sections
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------

//Variables for Equations of Motion
float yo[6];
float dyo[6];
float k1_eom[6];
float k2_eom[6];
float k3_eom[6];
float k4_eom[6];

//Variables for Flow
float Pos;
float Prg;
float Atrch;
float Aphrx;
float kt;
float Astar;
float Q;
float R;
float ug;

//Result Variables have _ at the end
float x1_;
float x2_;
float xb_;
float ug_;
float ga_;
float ad_;
float ps_;
float pe_;
float f1_;
float f2_;

float x1dot_;
float x2dot_;
float xbdot_;

//Debug Variables
vec4 debug_dy;

void BC_calc_pressures() {
	// Driving pressure calculations for the three-mass model based on Titze 2002

	//ke = (2*ad/Ae)*(1- ad/Ae); %A23
	//Above changed to below for consistency
	ke = (2*ad/ar[ieplx])*(1- ad/ar[ieplx]);
	pkd = (psg - pe)/(1-ke);
	ph = (psg + pe)/2;
	
	//-------------No contact -----------------------------------
	//Check!

	if((a1 > delta) && (a2 > delta) ) {
    
		if(a1 < a2) {
			if(zd <= zn) {
				f1 = L*zn*psg - L*(zn - zd + (ad/a1)*zd)*pkd;
				f2 = L*(T-zn)*(psg - pkd);
			} else {
				//f1 = L*zn*(psg - (ad^2/(an*a1))*pkd);
				f1 = L*zn*(psg - ((ad*ad)/(an*a1))*pkd);
				f2 = L*(T-zn)*psg - L*( (T-zd) + (ad/an)*(zd - zn))*pkd;
			}
			
		} else if(a1 >= a2) {
			
			//f1 = L*zn*(psg - (a2^2/(an*a1))*pkd);
			f1 = L*zn*(psg - ((a2*a2)/(an*a1))*pkd);
			f2 = L*(T-zn)*(psg - (a2/an)*pkd);
			
		}	
    
	}
	
	//--------------------------------------------------
    //Upper Contact Only
    //--------------------------------------------------
    else if( (a1 > delta) && (a2 <= delta) ) {
    
        if(zc >= zn) {
            f1 = L*zn*psg;
            f2 = L*(zc-zn)*psg + L*(T-zc)*ph;
        } else {
            f1 = L*zc*psg + L*(zn-zc)*ph;
            f2 = L*(T-zn)*ph;
        }
        
    }
    //--------------------------------------------------
	//Lower Contact Only
	//--------------------------------------------------
    else if( (a1 <= delta) && (a2 > delta) ) {
        
        if(zc < zn) {
            f1 = L*zc*ph + L*(zn-zc)*pe;
            f2 = L*(T-zn)*pe;
		}	
        
        if(zc >= zn) {
            f1 = L*zn*ph;
            f2 = L*(zc-zn)*ph + L*(T-zc)*pe;
        }
        
    }
	//--------------------------------------------------
	//Contact at Top and Bottom
	//--------------------------------------------------
	else if( (a1 <= delta) && (a2 <= delta) ) {
    
		f1 = L*zn*ph;   
		f2 = L*(T-zn)*ph;
   
	}
	
}

void BC_eom_3m() {
	//Equations of motion for Three-Mass model
	//From Brad Story Code 98
	//x1=yo(0); x2=yo(1); xb=yo(2);
	//x1dot=yo(3); x2dot=yo(4); xbdot=yo(6)
	// f1,f2: forces
	// b1,b2,B: damping constant (mass 1, mass 2, body mass)
	// k1,k2,kc,K: spring constant (mass1, mass2, shear-coupling, body mass
	
	dyo[0] = yo[3];  // velocity of mass 1
	dyo[1] = yo[4];  // v of mass 2
	dyo[2] = yo[5];  // v of body mass
	
	dyo[3] = (f1 - b1*(yo[3]-yo[5]) - k1*((yo[0])-(yo[2])) - kc*((yo[0])-(yo[1])) )/m1;
	dyo[4] = (f2 - b2*(yo[4]-yo[5]) - k2*((yo[1])-(yo[2])) - kc*((yo[1])-(yo[0])) )/m2;
	dyo[5] = (k1*((yo[0])-(yo[2])) + k2*((yo[1])-(yo[2])) + b1*(yo[3]-yo[5])+b2*(yo[4]-yo[5]) - K*(yo[2])-B*yo[5])/M;
}

void BC_rk3m() {
	//Fourth order Runge-Kutta algorithm for three-model model
	//From BS Code 98.
	for (int i=0; i<neq ; i++) {
		yo[i] = F[i];
	}
	
	BC_eom_3m();
	
	for (int i=0; i<neq ; i++) {
		k1_eom[i] = dt * dyo[i];
		yo[i] = F[i] + k1_eom[i]/2.0; 
	}

	BC_eom_3m();
	
	for (int i=0; i<neq ; i++) {
		k2_eom[i] = dt * dyo[i];
		yo[i] = F[i] + k2_eom[i]/2.0; 
	}
	 
	BC_eom_3m();
	
	for (int i=0; i<neq ; i++) {
		k3_eom[i] = dt * dyo[i];
		yo[i] = F[i] + k3_eom[i]/2.0; 
	}
	
	BC_eom_3m();

	for (int i=0; i<neq ; i++) {
		k4_eom[i] = dt * dyo[i];
	}
	
	//Final Calculation of new x1, x2, xb values
	for (int i=0; i<neq ; i++) {
		F[i] = F[i] + k1_eom[i]/6.0 + k2_eom[i]/3.0 + k3_eom[i]/3.0 + k4_eom[i]/6.0; 
	}

}

void BC_calcflow() {
	// Flow calculation based on Titze 2002 and 1984
	// Author: Brad Story
	// Pos = forward pressure wave at subglottis */
	// Prg = reflected pressure wave at supraglottis */
	
	Pos = f[jtrch];
	Prg = b[ieplx];
	
	Atrch = ar[jtrch];
	Aphrx = ar[ieplx];
	
	ke = 2*ga/Aphrx*(1-ga/Aphrx);
	kt = 1-ke;
	
	Astar = (Atrch * Aphrx) / (Atrch + Aphrx);
	Q = 4.0*kt * (Pos - Prg) / (c*rho_c);
	R = (ga / Astar);
	
	//Titze Formulation- A53. R and Q are abbreviations
	if( Q >= 0.0 ) {	
      ug =  (ga*c/kt) * (-R + sqrt( R*R + Q ));	
	} else if( Q < 0.0 ) {
      ug =  (ga*c/kt) * (+R - sqrt( R*R - Q )); 
	}
}



//=================================================================================================
// START OF ACTUAL BC SIMULATION
//=================================================================================================
vec4 body_cover_excitation(bool isFollowup) { 
	
	//Assignment of UNIFORM
	ar[ieplx] 	= a1Input[0];
	//MODIFICATION TESTING
	//c 			= 350.0; //UNIFORM FROM OUTSIDE
	//rho_c 		= rho*c; //ALSO UNIFORM
	dt 			= rates[2]; //1/rates[0]; //UNIFORM
	psg 		= excitationInput * maxPressure;
	f[jtrch]    = psg;
	
	A1 = a1Input[1];
	
	if (!isFollowup) {
		prev_out	= texture(inOutTexture, texture0);
		prev_out_fu = texture(inOutTexture, texture1);		
	} else {
		prev_out	= texture(inOutTexture, texture1); 
		prev_out_fu = texture(inOutTexture, texture0);
	}
	
	ug			= prev_out.x;
	F[0]  		= prev_out.y;
	F[1] 		= prev_out.z;
	F[2] 		= prev_out_fu.x;
	F[3]  		= prev_out_fu.y;
	F[4] 		= prev_out_fu.z;
	F[5] 		= prev_out_fu.w;
	//A1			= prev_out_fu.w;

	ug 			= ug*A1; //We retrieve previous flow using previous velocity and previous supraglottal area A1

	//This is there for cases where M, K changes for pitch
	B  = 2*(0.1)*sqrt(M*K);
	b1 = 2*(0.1)*sqrt(m1*k1); //Recalculation of Damping (Can be removed if need be)
	b2 = 2*(0.6)*sqrt(m2*k2);
	
	if (SUB == 0) {
		ar[32] = 100; //If no trachea, the subglottal area is taken as a massive open region
		//This might not be enough. So check after simulation is run. This is for attenuation of the tract segments
		alpha[ieplx] = 1 - (vtatten/sqrt(ar[ieplx]));
		alpha[jtrch] = 1 - (vtatten/sqrt(ar[jtrch]));
	} else {
		ps = psg; // psg then will be calculated through the trachea
		for (int i = 0; i <Nsect; i++) {
			alpha[i] = 1 - (vtatten/sqrt(ar[i])); // this should be hardcoded
		}
	}
	
	//Does atan work in GLSL? [Yes.]
	th0 = atan(xc/T);
	
	//Initialize
	a1 = 2*L*F[0]; //This will change every step
	a2 = 2*L*F[1]; //All values of F need to carried forward
	ga = max(0,min(a1,a2));
	
	//Start of a time step
	
	//================ Titze 1 ==========================
    znot = zn/T;
    xn = (1-znot)*(x01+F[0])+znot*(x02 + F[1]); //See Titze2002 Pg.375
    tangent = (x01-x02 + 2*(F[0]-F[1]))/T;
    x1 = xn - (-zn)*tangent;
    x2 = xn - (T - zn)*tangent;
    //====================================================
	
	//Vocal fold area calculations
    //Titze 374
    zc = min(T, max(0, zn+(xn/tangent)));
    a1 = max(delta, 2*L*x1);
    a2 = max(delta, 2*L*x2 );
    an = max(delta, 2*L*xn);
    zd = min(T, max(0, -0.2*x1/tangent));
    ad = min(a2, 1.2*a1);
    
    //Contact area calculations
    if((a1 > delta) && (a2 <= delta)) {
        ac = L*(T - zc);
	} else if((a1 <= delta) && (a2 > delta)) {
        ac = L*zc;
    } else if((a1 <= delta) && (a2 <= delta)) {
        ac = L*T;
    }
    
    //NOTE:AV
    //Calculate driving pressures
    //pe = f[ieplx]+b[ieplx]; // This has to read from outside in Aerophones
    //Two options
    //1. Either pe in total is read from outside. We can calculate f below or b as currently
    //2. Read f(ieplx) from outside and then use that to calculate b below
    //This is the system feedback!
   	if (!isFollowup)
    	pe = getFeedback(quad-numOfStates);
    else
    	pe = getFeedback(quad-2*numOfStates);
    	
    pe/=300; //VIC it seems that these feedback values are too big...
    
    b[ieplx] = alpha[ieplx]*f[ieplx] - ug*(rho_c/ar[ieplx]);
    //pe = f[ieplx]+b[ieplx];
    //MODIFICATION TESTING
    
    //Calculate Pressures
    BC_calc_pressures();
    
    //Integrate EOMs with Runge-Kutta
    BC_rk3m();
    
    //Glottal area calculation
    ga = max(0,min(a1,a2));
    
    //Calculate the glottal flow
    BC_calcflow();
	
	//Traverse through the Trachea
	if (SUB == 1) {
		
		//============== Wave propagation in VT/Trachea =======================
		for (int i=0; i< jtrch; i++) {
			D[i] 	= ar[i] + ar[i+1]; //Define these variables
			r1[i] 	= (ar[i] - ar[i+1])/D[i]; //From Kelly-Lochbaun see phon.ax.ac.uk for details
			r2[i] 	= -r1[i];
		}
		
		psg 		= 	2*f[jtrch]*alpha[jtrch] - ug*(rho_c/ar[jtrch]); //Also needs f[jtrch]
		f[itrch] 	=	0.9*ps - 0.8*b[itrch]*alpha[itrch]; //SHIT this needs b[itrch] :(
		
		//NOTE:AV. Moved this from outside. See if correct for f[jtrch] and b[ieplx]
		//Actually all the trachea portions might be needed :|
		for (int i = 0; i< Nsect; i++) {
			f[i] = alpha[i]*f[i];
			b[i] = alpha[i]*b[i];
		}
		
		//Even Sections of the Trachea
		//Should be i<15 checking with MATLAB code (RUNS 15 times)
		int j = itrch + 1;
		for (int i = 0; i < (Psi.length()-1) ; i++) {
			Psi[i] = f[j]*r1[j] + b[j+1]*r2[j];
			j += 2;
		}
		
		j = itrch + 1;
		for (int i = 0; i < (Psi.length()-1) ; i++) {
			b[j] 	= b[j+1] + Psi[i];
			f[j+1]	= f[j] + Psi[i];
			j += 2;
		}
		
		b[jtrch] = f[jtrch]*alpha[jtrch] - (ug*rho_c)/ar[jtrch];	//Must have f[jtrch] from previous step as it is calculated later.
		
		//Odd Sections of the Trachea
		j = itrch;
		//This runs 16 times and new f[jtrch] is calculated in it
		for (int i = 0; i < Psi.length() ; i++) {
			Psi[i] = f[j]*r1[j] + b[j+1]*r2[j];
			j += 2;
		}
		
		j = itrch;
		for (int i = 0; i < (Psi.length()-1) ; i++) {
			b[j] 	= b[j+1] + Psi[i];
			f[j+1]	= f[j] + Psi[i];
			j += 2;
		}
		
	}
	
	//Assign Output Variables
	//x1_ 	= x1;
	//x2_ 	= x2;
	x1_ 	= F[0];
	x2_ 	= F[1];
	xb_ 	= F[2];
	x1dot_	= F[3];
	x2dot_	= F[4];
	xbdot_	= F[5];
	ug_ 	= ug;
	ga_ 	= ga;
	ad_ 	= ad;
	ps_ 	= psg;
	pe_ 	= pe;
	f1_ 	= f1;
	f2_ 	= f2;
	

	float vout = ug_/ar[ieplx];
	
	if (!isFollowup) {
		//-------------------------------------------------------------------
		// butterworth low pass second order filter
		//-------------------------------------------------------------------
		// current and previous excitation values, inputs of the filter
		float filter_x0 = vout;           					 // input at N+1
		float filter_x1 = prev_out.r;     					 // input at N
		float filter_x2 = texture(inOutTexture, texture2).r; // input at N-1
		
		// previous filtered outputs
		float filter_y1 = texture(inOutTexture, texture1).a; // output at N  
		float filter_y2 = texture(inOutTexture, texture3).a; // output at N-1
	
		// current filtered output
		float filter_y0 = exLp_b0*filter_x0 + exLp_b1*filter_x1 + exLp_b2*filter_x2 - exLp_a1*filter_y1 - exLp_a2*filter_y2;
		//-------------------------------------------------------------------
		 
		return vec4(vout, x1_, x2_, filter_y0); //Look at changing the final returned value. I need to get all values of F[6] returned
	}
	else
		return vec4(xb_, x1dot_, x2dot_, xbdot_);
}