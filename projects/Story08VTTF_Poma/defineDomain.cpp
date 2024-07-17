/*
 * defineDomain.cpp
 *
 *  Created on: 2016-06-06
 *      Author: vic
 */

#include "ArtVocSynth.h"
#include "areaFunctions.h"


//----------------------------------------
// main extern var, from main.cpp
//----------------------------------------
extern ArtVocSynth *artVocSynth;
//----------------------------------------

//----------------------------------------
// audio extern var, from main.cpp
//----------------------------------------
extern unsigned int rate;
//----------------------------------------

//----------------------------------------
// domain extern vars, from main.cpp
//----------------------------------------
extern int domainSize[2];
extern float magnifier;
extern int rate_mul;
extern float ds;
extern bool fullscreen;
extern int monitorIndex;
//----------------------------------------

//----------------------------------------
// control extern var, from main.cpp
//----------------------------------------
extern int listenerCoord[2];
extern int feedbCoord[2];
extern int excitationDim[2];
extern int excitationCoord[2];
extern float excitationAngle;
extern excitation_model excitationModel;
extern subGlotEx_type subGlotExType;
extern float excitationVol;
extern float maxPressure;
extern float physicalParams[5];
extern float a1; //TODO this should be calculated and not hardcoded in each function
//----------------------------------------



//----------------------------------------
// area function externs, from areaFunctions.cpp
//----------------------------------------
// story 08 vowels
extern float story08Data_a_spacing;
extern float story08Data_a[44];
extern float story08Data_a_len;

extern float story08Data_i_spacing;
extern float story08Data_i[44];
extern float story08Data_i_len;

extern float story08Data_u_spacing;
extern float story08Data_u[44];
extern float story08Data_u_len;


// story 96 vowels
extern float story96Data_spacing;

extern float story96Data_a[44];
extern float story96Data_a_len;

extern float story96Data_i[42];
extern float story96Data_i_len;

extern float story96Data_u[46];
extern float story96Data_u_len;



// fant vowels
extern float fantData_spacing;

extern float fantData_a[35];
extern float fantData_a_len;

extern float fantData_o[38];
extern float fantData_o_len;


extern float fantData_u[40];
extern float fantData_u_len;


extern float fantData_i_[39];
extern float fantData_i__len;


extern float fantData_i[34];
extern float fantData_i_len;

extern float fantData_e[34];
extern float fantData_e_len;


// domainPixels passed by reference since dynamically allocated in the function
void initDomainPixels(float ***&domainPixels) {
	//---------------------------------------------
	// all is in domain frame of ref, which means no PML nor dead layers included
	//---------------------------------------------

	// domain pixels [aka walls!]
	domainPixels = new float **[domainSize[0]];
	float airVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_air, 0);
	for(int i = 0; i <domainSize[0]; i++) {
		domainPixels[i] = new float *[domainSize[1]];
		for(int j = 0; j <domainSize[1]; j++) {
			domainPixels[i][j]    = new float[4];
			domainPixels[i][j][3] = airVal;
		}
	}
}


// central excitation, no walls
void openSpaceExcitation(float ***&domainPixels) {
	magnifier    = 7;
	fullscreen   = false;
	monitorIndex = 1;

	domainSize[0] = 150;//308;//320//148;//92;
	domainSize[1] = 150;//228;//240//108;//92;

	rate_mul = 1;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+8;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 0;
	feedbCoord[1] = 0;

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = domainSize[0]/2;
	excitationCoord[1] = domainSize[1]/2;

	excitationVol = 1;

	excitationAngle = -1;

	excitationModel = ex_noFeedback;

	subGlotExType = subGlotEx_sin;

	maxPressure = 2000;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	//-----------------------------------------------------
}

void emptyDomain(float ***&domainPixels) {
	excitationDim[0] = 0;
	excitationDim[1] = 0;

	excitationAngle = -1;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	//-----------------------------------------------------
}

void singleWall(float ***&domainPixels) {
	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 0;
	feedbCoord[1] = 0;

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = 0;
	excitationCoord[1] = ( (domainSize[1]/2)-(excitationDim[1]/2));

	excitationAngle = -1;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	float wallVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]+excitationDim[1]+10, 0, domainSize[0]-5, wallVal, domainPixels);
	//-----------------------------------------------------
}

void doubleWall(float ***&domainPixels) {
	rate_mul = 5;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 0;
	feedbCoord[1] = 0;

	excitationDim[0] = 1;
	excitationDim[1] = 11;

	excitationCoord[0] = 1;
	excitationCoord[1] = ( (domainSize[1]/2)-(excitationDim[1]/2));

	excitationAngle = -1;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	float wallVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]-1, 			      0, domainSize[0]-5, wallVal, domainPixels);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]+excitationDim[1], 0, domainSize[0]-5, wallVal, domainPixels);
	ImplicitFDTD::fillTextureColumn( excitationCoord[0]-1, excitationCoord[1]-1, excitationCoord[1]+excitationDim[1], wallVal, domainPixels);
	//-----------------------------------------------------
}

void shrinkingTube(float ***&domainPixels) {
	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 0;
	feedbCoord[1] = 0;

	excitationDim[0] = 1;
	excitationDim[1] = 10;

	excitationCoord[0] = 1;
	excitationCoord[1] = ( (domainSize[1]/2)-(excitationDim[1]/2));

	excitationAngle = -1;

	excitationVol = 0.5;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	float wallVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]-1, 						  0, 10, wallVal, domainPixels);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]+excitationDim[1], 0, 10, wallVal, domainPixels);
	ImplicitFDTD::fillTextureRow( excitationCoord[1], 						  11, 21, wallVal, domainPixels);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]+excitationDim[1]-1,   11, 21, wallVal, domainPixels);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]+1, 						  22, domainSize[0]-5, wallVal, domainPixels);
	ImplicitFDTD::fillTextureRow( excitationCoord[1]+excitationDim[1]-2, 22, domainSize[0]-5, wallVal, domainPixels);
	ImplicitFDTD::fillTextureColumn( excitationCoord[0]-1, excitationCoord[1]-1, excitationCoord[1]+excitationDim[1]-1, wallVal, domainPixels);
	//-----------------------------------------------------

	listenerCoord[0]-=10;
}

void customTube(float ***&domainPixels) {
	//@VIC
/*
	domainSize[0] = 120;
	domainSize[1] = 60;

	rate_mul = 1;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = 0;
	excitationCoord[1] = domainSize[1]/2;

	excitationVol = 0.2;

	excitationAngle = -1;
	excitationModel = ex_clarinetReed;


	subGlotExType = subGlotEx_const;

	feedbCoord[0] = excitationCoord[0]+1;
	feedbCoord[1] = excitationCoord[1];

	a1 = (ds*5) * (ds*5) * M_PI; // to meters

	//-----------------------------------------------------

	initDomainPixels(domainPixels);

	int widthPixels = -1;

*/
	//listenerCoord[0] = createStraightTube(/*ds*70*///0.25, /*0.037338*/0.03, ds, domainSize, domainPixels, widthPixels, true);
	//-----------------------------------------------------


	//@VIC
	rate_mul = 15;
	excitationModel = ex_bodyCover;
	excitationModel = ex_2massKees;

	domainSize[0] = rate_mul+rate_mul*3;
	domainSize[1] = rate_mul+rate_mul;



	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = 0;
	excitationCoord[1] = domainSize[1]/2;

	excitationVol = 1;

	excitationAngle = 0;

	feedbCoord[0] = excitationCoord[0]+1;
	feedbCoord[1] = excitationCoord[1];

	a1 = (ds*rate_mul/2) * (ds*rate_mul/2) * M_PI; // to meters

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	int widthPixels = -1;

	listenerCoord[0] = createStraightTube(rate_mul*3*ds, rate_mul*ds, ds, domainSize, domainPixels, widthPixels, false);

	listenerCoord[0] = listenerCoord[0] - 0.003/ds;
	//-----------------------------------------------------
}

void fantVowel(char vowel, float ***&domainPixels, int asymmetric, bool nasal) {
	domainSize[0] = 80;
	domainSize[1] = 30;

	rate_mul = 4;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 1;
	feedbCoord[1] = listenerCoord[1];

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = feedbCoord[0]-1;
	excitationCoord[1] = feedbCoord[1];

	excitationVol = 0.4;

	excitationAngle = 0;

	excitationModel = ex_bodyCover;//ex_2massKees;

	subGlotExType = subGlotEx_const;

	float *vowelAreaFunc    = NULL;
	float vowelAreaFunc_len = -1;;

	switch(vowel) {
		case 'a':
			vowelAreaFunc     = fantData_a;
			vowelAreaFunc_len = fantData_a_len;
			break;
		case 'e':
			vowelAreaFunc     = fantData_e;
			vowelAreaFunc_len = fantData_e_len;
			break;
		case 'i':
			vowelAreaFunc     = fantData_i;
			vowelAreaFunc_len = fantData_i_len;
			break;
		case 'y': // weird choice, i know...
			vowelAreaFunc     = fantData_i_;
			vowelAreaFunc_len = fantData_i__len;
			break;
		case 'o':
			vowelAreaFunc     = fantData_o;
			vowelAreaFunc_len = fantData_o_len;
			break;
		case 'u':
			vowelAreaFunc     = fantData_u;
			vowelAreaFunc_len = fantData_u_len;
			break;
		default:
			break;

	}

	a1 = vowelAreaFunc[int((vowelAreaFunc_len/0.5))];
	a1 /= 10000; // to meters

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	listenerCoord[0] = createFantVowelTube(vowelAreaFunc, vowelAreaFunc_len, fantData_spacing, ds, domainSize, domainPixels, asymmetric, nasal);
	//-----------------------------------------------------
}

void story96Vowel(char vowel, float ***&domainPixels, int asymmetric, bool nasal) {
	rate_mul = 5;

	float sizes[4][2] = {{80, 30}, {88, 30}, {88, 30}, {100, 36}};

	domainSize[0] = sizes[rate_mul-3][0];
	domainSize[1] = sizes[rate_mul-3][1];

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 1;
	feedbCoord[1] = listenerCoord[1];

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = feedbCoord[0]-1;
	excitationCoord[1] = feedbCoord[1];

	excitationVol = 0.1;

	excitationAngle = 0;

	excitationModel = ex_bodyCover;

	subGlotExType = subGlotEx_const;

	float *vowelAreaFunc    = NULL;
	float vowelAreaFunc_len = -1;;

	switch(vowel) {
		case 'a':
			vowelAreaFunc     = story96Data_a;
			vowelAreaFunc_len = story96Data_a_len;
			break;
		case 'i':
			vowelAreaFunc     = story96Data_i;
			vowelAreaFunc_len = story96Data_i_len;
			break;
		case 'u':
			vowelAreaFunc     = story96Data_u;
			vowelAreaFunc_len = story96Data_u_len;
			break;
		default:
			break;

	}
	a1 = vowelAreaFunc[0];
	a1 /= 10000; // to meters

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	listenerCoord[0] = createStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len, story96Data_spacing, ds, domainSize, domainPixels, asymmetric, nasal);
	//-----------------------------------------------------
}


void story08Vowel(char vowel, float ***&domainPixels, int asymmetric, bool nasal) {
	rate_mul = 5;

	float sizes[10][2] = {{80, 30}, {88, 30}, {88, 30}, {106, 36}, {130, 46}, {130, 46}, {130, 46}, {254, 66}, {254, 66}, {280, 240}};

	domainSize[0] = sizes[rate_mul-3][0];
	domainSize[1] = sizes[rate_mul-3][1];


/*
	rate_mul = 4;
	domainSize[0] = 200;//182;
	domainSize[1] = 60;//60;
*/

/*
	rate_mul = 6;
	domainSize[0] = 200;//182;
	domainSize[1] = 60;//60;
*/


/*
	rate_mul = 12;
	domainSize[0] = 200;//182;
	domainSize[1] = 34;//60;
*/


	// deb's 2D vs 2.5D comparison
/*	rate_mul = 15;
	domainSize[0] = 265;
	domainSize[1] = 40;*/



/*	rate_mul = 20;
	domainSize[0] = 351+50;
	domainSize[1] = 53+20;
	magnifier = 1;*/



/*	rate_mul = 40;
	domainSize[0] = 700;
	domainSize[1] = 107;//+100;
	magnifier = 1;*/
	// tight domain
	//domainSize[0] = 610;
	//domainSize[1] = 107;
	// deb's domain, to run with /u/
	//domainSize[0] = 792;
	//domainSize[1] = 94;


/*	rate_mul = 60;
	domainSize[0] = 1049;
	domainSize[1] = 156+300;
	magnifier = 1;*/


	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 1;
	feedbCoord[1] = listenerCoord[1];

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = feedbCoord[0]-1;
	excitationCoord[1] = feedbCoord[1];

	excitationVol = 0.25;
	//excitationVol = 1; //@VIC

	excitationAngle = 0;

	excitationModel = ex_bodyCover;
	//excitationModel = ex_2massKees; //@VIC
	//excitationModel = ex_2mass; //@VIC

	subGlotExType = subGlotEx_const;

	float *vowelAreaFunc    = NULL;
	float vowelAreaFunc_len = -1;
	float vowelAreaFunc_spacing = -1;

	switch(vowel) {
		case 'a':
			vowelAreaFunc     = story08Data_a;
			vowelAreaFunc_len = story08Data_a_len;
			vowelAreaFunc_spacing = story08Data_a_spacing;
			break;
		case 'i':
			vowelAreaFunc     = story08Data_i;
			vowelAreaFunc_len = story08Data_i_len;
			vowelAreaFunc_spacing = story08Data_i_spacing;
			break;
		case 'u':
			vowelAreaFunc     = story08Data_u;
			vowelAreaFunc_len = story08Data_u_len;
			vowelAreaFunc_spacing = story08Data_u_spacing;
			break;
		default:
			break;

	}

	a1 = vowelAreaFunc[0];
	a1 /= 10000; // to meters

	printf("Vowel: %c - srate_mul: %d\n", vowel, rate_mul);


	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	listenerCoord[0] = createStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len, vowelAreaFunc_spacing, ds, domainSize, domainPixels, asymmetric, nasal);
	//-----------------------------------------------------
	listenerCoord[0] = listenerCoord[0] - 0.003/ds; // 3 mm inside of tube

	printf("Mic placed %d cells after excitation, on the center line of the tube\n\n", listenerCoord[0]);


	// close tube with excitation cells
	float wallVal = ImplicitFDTD::cell_wall;
	int cnt=0;
	bool found = false;
	for(int j=0; j<domainSize[1]; j++) {
		if(!found) {
			if(domainPixels[excitationCoord[0]][j][3] == wallVal)
				found = true;
		}
		else {
			if(domainPixels[excitationCoord[0]][j][3] != wallVal)
				cnt++;
			else
				break;
		}
	}
	excitationDim[1] = cnt; // update excitation dim
	excitationCoord[1] -= cnt/2; // shift excitation to cover the whole glottal constriction

	//a1 = cnt*ds*0.5 * cnt*ds*0.5 * M_PI;
	//a1 *= 0.99;
	//a1 = cnt*ds;
}
void countour(string countour_nameAndPath, float ***&domainPixels) {
	domainSize[0] = 48;//84;//96;
	domainSize[1] = 55;//82;//143;

	//domainSize[0] = 88;//84;//96;
	//domainSize[1] = 20;//82;//143;

	rate_mul = 5;//4;//8;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = 44;
	listenerCoord[1] = 39;

	//listenerCoord[0] = 86;
	//listenerCoord[1] = 10;

	feedbCoord[0] = 14;
	feedbCoord[1] = 1;

	//feedbCoord[0] = 1;
	//feedbCoord[1] = 9;

	// excitation
	excitationDim[0] = 2;
	excitationDim[1] = 1;

	excitationCoord[0] = 14;
	excitationCoord[1] = 0;

	//excitationDim[0] = 1;
	//excitationDim[1] = 2;

	//excitationCoord[0] = 0;
	//excitationCoord[1] = 9;

	excitationVol = 0.2;

	//excitationAngle = 90;

	excitationAngle = 0;

	excitationModel = ex_bodyCover;

	subGlotExType = subGlotEx_const;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	artVocSynth->loadContourFile(domainSize, ds, countour_nameAndPath, domainPixels); //TODO this thing should maybe return a1? but also needs glottal position in contour file
	//-----------------------------------------------------
}
