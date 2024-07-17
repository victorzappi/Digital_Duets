//=================================================================================================
//	TWO MASS KEES
//=================================================================================================

vec4 two_mass_excitation_kees(){

	//=================================================================================================
	// constants, some to be replaced with uniforms
	//=================================================================================================
	// everything in International System of Units
	// input
	float ps = 0; 
	float A1_next = 0.0001;
	// previous output
	vec4 prev_out;     //ug/A1, x1, x2 //, A1 at N
	vec4 prev_out_old;  //ug/A1, x1, x2 //, A1 at N-1
	// feedback
	float p1 = 0;// read from outside
	// interpret uniforms
	float gs = dampingMod; 
	float q = pitchFactor; 
	float Ag0 = restArea; 
	float _m1 = masses[0];
	float _m2 = masses[1];
	float _d1 = thicknesses[0];
	float _d2 = thicknesses[1];
	float _k1 = springKs[0];
	float _k2 = springKs[1];
	float _kc = springKs[2];
	float srate = rates[0];
	float srateSquare = rates[1];
	//float rho = 1.14;//1.1760; //1.14;
	//float mu  = 1.86e-5;//1.846e-5; //1.86e-5;
	// const and depending on uniforms
	float Ag01 = Ag0; // see Ishizaka-Flanagan, p 1250.
	float Ag02 = Ag0;
	float lg = 0.014; // see Ishizaka-Flanagan, p 1250.
	const float etak1 = 1000000;
	const float etak2 = 1000000;
	const float etah1 = 5000000;
	const float etah2 = 5000000;
	float m1 = _m1 / q;
	float m2 = _m2 / q;
	float d1 = _d1 / q;
	float d2 = _d2 / q;
	float k1 = _k1 * q;
	float k2 = _k2 * q;
	float h1 = 3*k1;
	float h2 = 3*k2;
	float kc = _kc * q * q;
	float r1open = 2 * 0.2 * sqrt(k1*m1) / (gs*gs);
	float r1closed = 2 * 1.1 *sqrt(k1*m1) / (gs*gs);
	float r2open = 2 * 0.6 * sqrt(k2*m2) / (gs*gs);
	float r2closed = 2 * 1.9 * sqrt(k2*m2) / (gs*gs);

	// working vars
	float ug;
	float ugold;
	float A1;
	float A1old;
	float x1;
	float x2;
	float xold1;
	float xold2;
	float f1;
	float f2;
	float Ag1;
	float Ag2;

	//output vars
	float ug_next;
	float x1_next;
	float x2_next;


	// here we go...

	// read input pressure and first area
	ps = excitationInput * maxPressure;
	A1_next = a1Input[0];

	p1 = getFeedback(quad-numOfStates); // read pressure feedback [mapped to PREVIOUS FDTD STEP!]

	// read previous output
	prev_out = texture(inOutTexture, texture0);
	ug = prev_out.x;
	x1 = prev_out.y;
	x2 = prev_out.z;
	//A1 = prev_out.w;
	A1 = a1Input[1];

	ug = ug*A1;

	prev_out_old = texture(inOutTexture, texture2);
	ugold = prev_out_old.x;
	xold1 = prev_out_old.y;
	xold2 = prev_out_old.z;
	//A1old = prev_out_old.w;
	A1old = a1Input[2];

	ugold = ugold*A1old;

	Ag1 = Ag01 + 2 * lg*x1;
	Ag2 = Ag02 + 2 * lg*x2;

	// Use eq. 18 from Ishizaka-Flanagan
	// computer force on mass 1
	if (x1 > -Ag01 / (2 * lg) && x2 > -Ag02 / (2 * lg)) {
		float Rv1 = 12 * mu*lg*lg*(d1 / (Ag1*Ag1*Ag1));
		float Lg1 = rho*(d1 / Ag1);
		f1 = ps - 1.37*(rho / 2)*(ug / Ag1)*(ug / Ag1) - .5*(Rv1*ug + srate*Lg1*(ug - ugold));
		f1 *= d1*lg;
	}
	else
		f1 = ps * d1*lg;

	// Use eq. 18 from Ishizaka-Flanagan
	// computer force on mass 2
	if (x1 > -Ag01 / (2 * lg)) {
		if (x2 > -Ag02 / (2 * lg)) {
			float Rv1 = 12 * mu*lg*lg*(d1 / (Ag1*Ag1*Ag1));
			float Lg1 = rho*(d1 / Ag1);
			float Rv2 = 12 * mu*lg*lg*(d2 / (Ag2*Ag2*Ag2));
			float Lg2 = rho*(d2 / Ag2);
			f2 = f1 / (d1*lg) - .5*(Rv1 + Rv2)*ug - (Lg1 + Lg2)*srate*(ug - ugold) - .5*rho*ug*ug*(1 / (Ag2*Ag2) - 1 / (Ag1*Ag1));
			f2 *= d2*lg;
		}
		else {
			f2 = ps * d2*lg;
		}
	}
	else
		f2 = 0;

	// advance masses
	// calc. matrix elements
	float h1_ = 0, h2_ = 0, r1_ = 0, r2_ = 0;
	if (x1<-Ag01 / (2 * lg)) {
		h1_ = h1;
		r1_ = r1closed;
	}
	else {
		h1_ = 0;
		r1_ = r1open;
	}
	if (x2<-Ag02 / (2 * lg)) {
		h2_ = h2;
		r2_ = r2closed;
	}
	else {
		h2_ = 0;
		r2_ = r2open;
	}
	float a11 = (k1 + h1_ + kc) / srateSquare + r1_ / srate + m1;
	float a12 = -kc / srateSquare;
	float a21 = a12;
	float a22 = (k2 + h2_ + kc) / srateSquare + r2_ / srate + m2;
	float s1prime = k1*etak1*x1*x1*x1 + h1_*(Ag01 / (2 * lg) +
		etah1*(Ag01 / (2 * lg) + x1)*(Ag01 / (2 * lg) + x1)*(Ag01 / (2 * lg) + x1));
	float s2prime = k2*etak2*x2*x2*x2 + h2_*(Ag02 / (2 * lg) +
		etah2*(Ag02 / (2 * lg) + x2)*(Ag02 / (2 * lg) + x2)*(Ag02 / (2 * lg) + x2));
	float b1 = (2 * m1 + r1_ / srate)*x1 - m1*xold1 - s1prime / srateSquare + f1 / srateSquare;
	float b2 = (2 * m2 + r2_ / srate)*x2 - m2*xold2 - s2prime / srateSquare + f2 / srateSquare;

	float det = a11*a22 - a21*a12;
	if (det == 0) {
		det = 1;
	}
	x1_next = (a22*b1 - a12*b2) / det;
	x2_next = (a11*b2 - a21*b1) / det;


	// advance ug
	Ag1 = Ag01 + 2 * lg*x1_next;
	Ag2 = Ag02 + 2 * lg*x2_next;
	if (Ag1<0 || Ag2<0) {
		ug_next = 0;
	}
	else {
		
		float Rtot = ( rho / 2)*(0.37 / (Ag1*Ag1) + (1 - 2 * (Ag2 /  A1_next)*(1 - Ag2 /  A1_next)) / (Ag2*Ag2))
			*abs(ug)
			+ 12 *  mu* lg* lg*( d1 / (Ag1*Ag1*Ag1) +  d2 / (Ag2*Ag2*Ag2));
		float Ltot =  rho*( d1 / Ag1 +  d2 / Ag2);
		ug_next = (( ps -  p1) / srate + Ltot*ug) / (Rtot / srate + Ltot);

	}

	float vout = ug_next/A1_next; 
	
	//-------------------------------------------------------------------
	// butterworth low pass second order filter
	//-------------------------------------------------------------------
	// current and previous excitation values, inputs of the filter
	float filter_x0 = vout;         // input at N+1
	float filter_x1 = prev_out.r;     // input at N
	float filter_x2 = prev_out_old.r; // input at N-1
	
	// previous filtered outputs
	float filter_y1 = texture(inOutTexture, texture1).a; // output at N  
	float filter_y2 = texture(inOutTexture, texture3).a; // output at N-1

	// current filtered output
	float filter_y0 = exLp_b0*filter_x0 + exLp_b1*filter_x1 + exLp_b2*filter_x2 - exLp_a1*filter_y1 - exLp_a2*filter_y2;
	//-------------------------------------------------------------------

	return vec4(vout, x1_next, x2_next, filter_y0);
}
