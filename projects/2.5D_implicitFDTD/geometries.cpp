/*
 * geometries.cpp
 *
 *  Created on: 2016-06-03
 *      Author: Victor Zappi
 */

//#include "ImplicitFDTD.h"
#include "VocalTractFDTD.h"
#include <math.h> // M_PI


extern int excitationCoord[2];
extern int excitationDim[2];
extern float spacing_factor;
extern float radii_factor;
extern float width_factor;


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------
// story 08 vowels, all in cm
//-------------------------------------------------------

// these are freaking segments, not area samples as in Fant!

float story08Data_a_spacing = 0.388*spacing_factor; // in Story 08 each vowel is sampled with different delta, but they're all 44 segments
float story08Data_a[44] = { 0.56, 0.62, 0.66, 0.78, 0.97,
					      1.16, 1.12, 0.82, 0.55, 0.45,
					      0.37, 0.29, 0.21, 0.15, 0.16,
					      0.25, 0.34, 0.43, 0.54, 0.61,
					      0.67, 0.98, 1.76, 2.75, 3.52,
					      4.08, 4.74, 5.61, 6.60, 7.61,
					      8.48, 9.06, 9.29, 9.26, 9.06,
					      8.64, 7.91, 6.98, 6.02, 5.13,
					      4.55, 4.52, 4.71, 4.72 };
float story08Data_a_len = story08Data_a_spacing*44;//0.1709;
float story08Data_a_formants[8] = {696, 1068, 3031, 4064, 5039, 5703, 7065, 7595};

float story08Data_i_spacing = 0.384*spacing_factor;
float story08Data_i[44] = { 0.51, 0.59, 0.62, 0.72, 1.24,
						  2.30, 3.30, 3.59, 3.22, 2.86,
						  3.00, 3.61, 4.39, 4.95, 5.17,
						  5.16, 5.18, 5.26, 5.20, 5.02,
						  4.71, 4.13, 3.43, 2.83, 2.32,
						  1.83, 1.46, 1.23, 1.08, 0.94,
						  0.80, 0.67, 0.55, 0.46, 0.40,
						  0.36, 0.35, 0.35, 0.38, 0.51,
						  0.74, 0.92, 0.96, 0.91 };
float story08Data_i_len = story08Data_i_spacing*44;//0.1690;
float story08Data_i_formants[8] = {263, 2111, 3010, 4138, 5019, 5753, 6575, 7645};

float story08Data_u_spacing = 0.445*spacing_factor;
float story08Data_u[44] = { 0.54, 0.61, 0.66, 0.75, 1.13,
						  1.99, 2.83, 2.90, 2.52, 2.40,
						  2.83, 3.56, 3.99, 3.89, 3.50,
						  3.04, 2.64, 2.44, 2.31, 2.07,
						  1.80, 1.52, 1.14, 0.74, 0.42,
						  0.22, 0.14, 0.20, 0.47, 0.89,
						  1.15, 1.42, 2.17, 3.04, 3.69,
						  4.70, 5.74, 5.41, 3.82, 2.34,
						  1.35, 0.65, 0.29, 0.16 };
float story08Data_u_len = story08Data_u_spacing*44;//0.1959;
float story08Data_u_formants[8] = {259, 757, 2264, 3603, 4173, 5062, 6126, 6594};



//-------------------------------------------------------
// data from "Effects of higher order propagation modes in vocal tract like geometries",
// Journal of the Acoustic Society America 137 (2), February 2015
//-------------------------------------------------------
int duration_experiment = 25; //ms
float c_experiment = 344;
float rho_experiment = 1.19;
float micXpos_experiment[] = {30, 10}; // in mm, first is inside tube, 30 mm from source; second is outside, 10 mm from opening: both on midline [y pos]
float mu_experiment = 0.0025;

float domain_experiment[] = {0.3, 0.3, 0.2}; // m

// two centric and two eccentric tubes
// all in mm
float tubeslen = 85;
float tube0Diameter = 14.5;
float tube1Diameter = 29.5;



//-------------------------------------------------------
// data for symmetry test, partially shared taken from:
// "Effects of higher order propagation modes in vocal tract like geometries",
// Journal of the Acoustic Society America 137 (2), February 2015
// and from:
// "Two-dimensional vocal tracts with three dimensional behavior in the numerical generation of vowels",
// Journal of the Acoustic Society America
//-------------------------------------------------------
int symmTest_duration = 50; // ms
float symmTest_c = 350;
float symmTest_rho = 1.14;
//float symmTest_alpha = 0.005; // alpha for boundaries, better use the global one, for testing
float symmTest_micPos = -0.003; // m, from opening...means inside tube

float symmTest_domain[2][2] = {{0.175, 0.08}, {0.17, 0.15}}; // m
float symmTest_tubeslen = 0.085; // m, each geometry has 2 tubes juxtaposed
double symmTest_a1 = (0.014500/2) * (0.014500/2) * M_PI; // m2, area of first tube's cross section

#define SYMMTEST_SECTION_SAMPLES 321 // computed using octave script crossSection_heightAnalysis.m, with rate_mul = 60 [biggest one we generally run]

// total symmetry
double symmTest_tube0_height_big = 0.029500; // m, as diameter
double symmTest_tube0_widthProfileDs_big = 9.2187e-05; // m, cos all these profiles have been uniformly resampled
double symmTest_tube0_widthProfile_big[SYMMTEST_SECTION_SAMPLES] = {
        0.00000,        0.00329,        0.00465,        0.00569,        0.00656,        0.00732,        0.00800,        0.00863,        0.00921,        0.00975,
        0.01027,        0.01075,        0.01121,        0.01165,        0.01207,        0.01247,        0.01286,        0.01323,        0.01359,        0.01394,
        0.01428,        0.01461,        0.01493,        0.01524,        0.01554,        0.01583,        0.01612,        0.01640,        0.01667,        0.01694,
        0.01720,        0.01745,        0.01770,        0.01794,        0.01818,        0.01841,        0.01864,        0.01887,        0.01909,        0.01930,
        0.01951,        0.01972,        0.01992,        0.02012,        0.02032,        0.02051,        0.02070,        0.02088,        0.02107,        0.02125,
        0.02142,        0.02160,        0.02177,        0.02193,        0.02210,        0.02226,        0.02242,        0.02257,        0.02273,        0.02288,
        0.02303,        0.02317,        0.02332,        0.02346,        0.02360,        0.02374,        0.02387,        0.02400,        0.02414,        0.02426,
        0.02439,        0.02451,        0.02464,        0.02476,        0.02488,        0.02499,        0.02511,        0.02522,        0.02533,        0.02544,
        0.02555,        0.02565,        0.02576,        0.02586,        0.02596,        0.02606,        0.02616,        0.02625,        0.02634,        0.02644,
        0.02653,        0.02662,        0.02670,        0.02679,        0.02687,        0.02696,        0.02704,        0.02712,        0.02720,        0.02727,
        0.02735,        0.02742,        0.02749,        0.02756,        0.02763,        0.02770,        0.02777,        0.02783,        0.02790,        0.02796,
        0.02802,        0.02808,        0.02814,        0.02820,        0.02825,        0.02831,        0.02836,        0.02841,        0.02847,        0.02852,
        0.02856,        0.02861,        0.02866,        0.02870,        0.02874,        0.02879,        0.02883,        0.02887,        0.02890,        0.02894,
        0.02898,        0.02901,        0.02904,        0.02908,        0.02911,        0.02914,        0.02917,        0.02919,        0.02922,        0.02924,
        0.02927,        0.02929,        0.02931,        0.02933,        0.02935,        0.02937,        0.02939,        0.02940,        0.02942,        0.02943,
        0.02944,        0.02945,        0.02946,        0.02947,        0.02948,        0.02949,        0.02949,        0.02949,        0.02950,        0.02950,
        0.02950,        0.02950,        0.02950,        0.02949,        0.02949,        0.02949,        0.02948,        0.02947,        0.02946,        0.02945,
        0.02944,        0.02943,        0.02942,        0.02940,        0.02939,        0.02937,        0.02935,        0.02933,        0.02931,        0.02929,
        0.02927,        0.02924,        0.02922,        0.02919,        0.02917,        0.02914,        0.02911,        0.02908,        0.02904,        0.02901,
        0.02898,        0.02894,        0.02890,        0.02887,        0.02883,        0.02879,        0.02874,        0.02870,        0.02866,        0.02861,
        0.02856,        0.02852,        0.02847,        0.02841,        0.02836,        0.02831,        0.02825,        0.02820,        0.02814,        0.02808,
        0.02802,        0.02796,        0.02790,        0.02783,        0.02777,        0.02770,        0.02763,        0.02756,        0.02749,        0.02742,
        0.02735,        0.02727,        0.02720,        0.02712,        0.02704,        0.02696,        0.02687,        0.02679,        0.02670,        0.02662,
        0.02653,        0.02644,        0.02634,        0.02625,        0.02616,        0.02606,        0.02596,        0.02586,        0.02576,        0.02565,
        0.02555,        0.02544,        0.02533,        0.02522,        0.02511,        0.02499,        0.02488,        0.02476,        0.02464,        0.02451,
        0.02439,        0.02426,        0.02414,        0.02400,        0.02387,        0.02374,        0.02360,        0.02346,        0.02332,        0.02317,
        0.02303,        0.02288,        0.02273,        0.02257,        0.02242,        0.02226,        0.02210,        0.02193,        0.02177,        0.02160,
        0.02142,        0.02125,        0.02107,        0.02088,        0.02070,        0.02051,        0.02032,        0.02012,        0.01992,        0.01972,
        0.01951,        0.01930,        0.01909,        0.01887,        0.01864,        0.01841,        0.01818,        0.01794,        0.01770,        0.01745,
        0.01720,        0.01694,        0.01667,        0.01640,        0.01612,        0.01583,        0.01554,        0.01524,        0.01493,        0.01461,
        0.01428,        0.01394,        0.01359,        0.01323,        0.01286,        0.01247,        0.01207,        0.01165,        0.01121,        0.01075,
        0.01027,        0.00975,        0.00921,        0.00863,        0.00800,        0.00732,        0.00656,        0.00569,        0.00465,        0.00329,
        0.00000
};

double symmTest_tube0_height_small = 0.014500; // m, as diameter
double symmTest_tube0_widthProfileDs_small = 4.5313e-05; // m
double symmTest_tube0_widthProfile_small[SYMMTEST_SECTION_SAMPLES] = {
        0.00000,        0.00233,        0.00329,        0.00402,        0.00463,        0.00517,        0.00566,        0.00610,        0.00651,        0.00689,
        0.00726,        0.00760,        0.00792,        0.00823,        0.00853,        0.00882,        0.00909,        0.00935,        0.00961,        0.00986,
        0.01009,        0.01033,        0.01055,        0.01077,        0.01098,        0.01119,        0.01139,        0.01159,        0.01178,        0.01197,
        0.01216,        0.01234,        0.01251,        0.01268,        0.01285,        0.01302,        0.01318,        0.01334,        0.01349,        0.01364,
        0.01379,        0.01394,        0.01408,        0.01422,        0.01436,        0.01450,        0.01463,        0.01476,        0.01489,        0.01502,
        0.01514,        0.01526,        0.01539,        0.01550,        0.01562,        0.01573,        0.01585,        0.01596,        0.01607,        0.01617,
        0.01628,        0.01638,        0.01648,        0.01658,        0.01668,        0.01678,        0.01687,        0.01697,        0.01706,        0.01715,
        0.01724,        0.01733,        0.01741,        0.01750,        0.01758,        0.01767,        0.01775,        0.01783,        0.01791,        0.01798,
        0.01806,        0.01813,        0.01821,        0.01828,        0.01835,        0.01842,        0.01849,        0.01856,        0.01862,        0.01869,
        0.01875,        0.01881,        0.01888,        0.01894,        0.01900,        0.01905,        0.01911,        0.01917,        0.01922,        0.01928,
        0.01933,        0.01938,        0.01943,        0.01948,        0.01953,        0.01958,        0.01963,        0.01967,        0.01972,        0.01976,
        0.01981,        0.01985,        0.01989,        0.01993,        0.01997,        0.02001,        0.02005,        0.02008,        0.02012,        0.02016,
        0.02019,        0.02022,        0.02026,        0.02029,        0.02032,        0.02035,        0.02038,        0.02040,        0.02043,        0.02046,
        0.02048,        0.02051,        0.02053,        0.02055,        0.02057,        0.02060,        0.02062,        0.02064,        0.02065,        0.02067,
        0.02069,        0.02070,        0.02072,        0.02073,        0.02075,        0.02076,        0.02077,        0.02078,        0.02079,        0.02080,
        0.02081,        0.02082,        0.02083,        0.02083,        0.02084,        0.02084,        0.02085,        0.02085,        0.02085,        0.02085,
        0.02085,        0.02085,        0.02085,        0.02085,        0.02085,        0.02084,        0.02084,        0.02083,        0.02083,        0.02082,
        0.02081,        0.02080,        0.02079,        0.02078,        0.02077,        0.02076,        0.02075,        0.02073,        0.02072,        0.02070,
        0.02069,        0.02067,        0.02065,        0.02064,        0.02062,        0.02060,        0.02057,        0.02055,        0.02053,        0.02051,
        0.02048,        0.02046,        0.02043,        0.02040,        0.02038,        0.02035,        0.02032,        0.02029,        0.02026,        0.02022,
        0.02019,        0.02016,        0.02012,        0.02008,        0.02005,        0.02001,        0.01997,        0.01993,        0.01989,        0.01985,
        0.01981,        0.01976,        0.01972,        0.01967,        0.01963,        0.01958,        0.01953,        0.01948,        0.01943,        0.01938,
        0.01933,        0.01928,        0.01922,        0.01917,        0.01911,        0.01905,        0.01900,        0.01894,        0.01888,        0.01881,
        0.01875,        0.01869,        0.01862,        0.01856,        0.01849,        0.01842,        0.01835,        0.01828,        0.01821,        0.01813,
        0.01806,        0.01798,        0.01791,        0.01783,        0.01775,        0.01767,        0.01758,        0.01750,        0.01741,        0.01733,
        0.01724,        0.01715,        0.01706,        0.01697,        0.01687,        0.01678,        0.01668,        0.01658,        0.01648,        0.01638,
        0.01628,        0.01617,        0.01607,        0.01596,        0.01585,        0.01573,        0.01562,        0.01550,        0.01539,        0.01526,
        0.01514,        0.01502,        0.01489,        0.01476,        0.01463,        0.01450,        0.01436,        0.01422,        0.01408,        0.01394,
        0.01379,        0.01364,        0.01349,        0.01334,        0.01318,        0.01302,        0.01285,        0.01268,        0.01251,        0.01234,
        0.01216,        0.01197,        0.01178,        0.01159,        0.01139,        0.01119,        0.01098,        0.01077,        0.01055,        0.01033,
        0.01009,        0.00986,        0.00961,        0.00935,        0.00909,        0.00882,        0.00853,        0.00823,        0.00792,        0.00760,
        0.00726,        0.00689,        0.00651,        0.00610,        0.00566,        0.00517,        0.00463,        0.00402,        0.00329,        0.00233,
        0.00000
};

// y/z symmetry
double symmTest_tube1_widthProfile_big[SYMMTEST_SECTION_SAMPLES] = {};
double symmTest_tube1_widthProfile_small[SYMMTEST_SECTION_SAMPLES] = {};

// z symmetry
double symmTest_tube2_widthProfile_big[SYMMTEST_SECTION_SAMPLES] = {};
double symmTest_tube2_widthProfile_small[SYMMTEST_SECTION_SAMPLES] = {};

// no symmetry
double symmTest_tube3_widthProfile_big[SYMMTEST_SECTION_SAMPLES] = {};
double symmTest_tube3_widthProfile_small[SYMMTEST_SECTION_SAMPLES] = {};



// story vowels references

// xyz symmetry...total symmetry, circle!
double symmTest_story_xyz_area = 0.000683; // m2
double symmTest_story_xyz_height = 0.029500; // m, as diameter
double symmTest_story_xyz_widthProfileDs = 9.2187e-05; // m, cos all these profiles have been uniformly resampled [ = symmTest_xyz_story_height/SYMMTEST_SECTION_SAMPLES]
double symmTest_story_xyz_widthProfile[SYMMTEST_SECTION_SAMPLES] = {
        0.00000,        0.00329,        0.00465,        0.00569,        0.00656,        0.00732,        0.00800,        0.00863,        0.00921,        0.00975,
        0.01027,        0.01075,        0.01121,        0.01165,        0.01207,        0.01247,        0.01286,        0.01323,        0.01359,        0.01394,
        0.01428,        0.01461,        0.01493,        0.01524,        0.01554,        0.01583,        0.01612,        0.01640,        0.01667,        0.01694,
        0.01720,        0.01745,        0.01770,        0.01794,        0.01818,        0.01841,        0.01864,        0.01887,        0.01909,        0.01930,
        0.01951,        0.01972,        0.01992,        0.02012,        0.02032,        0.02051,        0.02070,        0.02088,        0.02107,        0.02125,
        0.02142,        0.02160,        0.02177,        0.02193,        0.02210,        0.02226,        0.02242,        0.02257,        0.02273,        0.02288,
        0.02303,        0.02317,        0.02332,        0.02346,        0.02360,        0.02374,        0.02387,        0.02400,        0.02414,        0.02426,
        0.02439,        0.02451,        0.02464,        0.02476,        0.02488,        0.02499,        0.02511,        0.02522,        0.02533,        0.02544,
        0.02555,        0.02565,        0.02576,        0.02586,        0.02596,        0.02606,        0.02616,        0.02625,        0.02634,        0.02644,
        0.02653,        0.02662,        0.02670,        0.02679,        0.02687,        0.02696,        0.02704,        0.02712,        0.02720,        0.02727,
        0.02735,        0.02742,        0.02749,        0.02756,        0.02763,        0.02770,        0.02777,        0.02783,        0.02790,        0.02796,
        0.02802,        0.02808,        0.02814,        0.02820,        0.02825,        0.02831,        0.02836,        0.02841,        0.02847,        0.02852,
        0.02856,        0.02861,        0.02866,        0.02870,        0.02874,        0.02879,        0.02883,        0.02887,        0.02890,        0.02894,
        0.02898,        0.02901,        0.02904,        0.02908,        0.02911,        0.02914,        0.02917,        0.02919,        0.02922,        0.02924,
        0.02927,        0.02929,        0.02931,        0.02933,        0.02935,        0.02937,        0.02939,        0.02940,        0.02942,        0.02943,
        0.02944,        0.02945,        0.02946,        0.02947,        0.02948,        0.02949,        0.02949,        0.02949,        0.02950,        0.02950,
        0.02950,        0.02950,        0.02950,        0.02949,        0.02949,        0.02949,        0.02948,        0.02947,        0.02946,        0.02945,
        0.02944,        0.02943,        0.02942,        0.02940,        0.02939,        0.02937,        0.02935,        0.02933,        0.02931,        0.02929,
        0.02927,        0.02924,        0.02922,        0.02919,        0.02917,        0.02914,        0.02911,        0.02908,        0.02904,        0.02901,
        0.02898,        0.02894,        0.02890,        0.02887,        0.02883,        0.02879,        0.02874,        0.02870,        0.02866,        0.02861,
        0.02856,        0.02852,        0.02847,        0.02841,        0.02836,        0.02831,        0.02825,        0.02820,        0.02814,        0.02808,
        0.02802,        0.02796,        0.02790,        0.02783,        0.02777,        0.02770,        0.02763,        0.02756,        0.02749,        0.02742,
        0.02735,        0.02727,        0.02720,        0.02712,        0.02704,        0.02696,        0.02687,        0.02679,        0.02670,        0.02662,
        0.02653,        0.02644,        0.02634,        0.02625,        0.02616,        0.02606,        0.02596,        0.02586,        0.02576,        0.02565,
        0.02555,        0.02544,        0.02533,        0.02522,        0.02511,        0.02499,        0.02488,        0.02476,        0.02464,        0.02451,
        0.02439,        0.02426,        0.02414,        0.02400,        0.02387,        0.02374,        0.02360,        0.02346,        0.02332,        0.02317,
        0.02303,        0.02288,        0.02273,        0.02257,        0.02242,        0.02226,        0.02210,        0.02193,        0.02177,        0.02160,
        0.02142,        0.02125,        0.02107,        0.02088,        0.02070,        0.02051,        0.02032,        0.02012,        0.01992,        0.01972,
        0.01951,        0.01930,        0.01909,        0.01887,        0.01864,        0.01841,        0.01818,        0.01794,        0.01770,        0.01745,
        0.01720,        0.01694,        0.01667,        0.01640,        0.01612,        0.01583,        0.01554,        0.01524,        0.01493,        0.01461,
        0.01428,        0.01394,        0.01359,        0.01323,        0.01286,        0.01247,        0.01207,        0.01165,        0.01121,        0.01075,
        0.01027,        0.00975,        0.00921,        0.00863,        0.00800,        0.00732,        0.00656,        0.00569,        0.00465,        0.00329,
        0.00000
};

// yz symmetry...ellipse!
double symmTest_story_yz_area = 0.000683; // m2
double symmTest_story_yz_height = 0.041704; // m, as major axis
double symmTest_story_yz_widthProfileDs = 1.3033e-04; // m, cos all these profiles have been uniformly resampled [ = symmTest_xyz_story_height/SYMMTEST_SECTION_SAMPLES]
double symmTest_story_yz_widthProfile[SYMMTEST_SECTION_SAMPLES] = {
		 0.00000,        0.00233,        0.00329,        0.00402,        0.00463,        0.00517,        0.00566,        0.00610,        0.00651,        0.00689,
		0.00726,        0.00760,        0.00792,        0.00823,        0.00853,        0.00882,        0.00909,        0.00935,        0.00961,        0.00986,
		0.01009,        0.01033,        0.01055,        0.01077,        0.01098,        0.01119,        0.01139,        0.01159,        0.01178,        0.01197,
		0.01216,        0.01234,        0.01251,        0.01268,        0.01285,        0.01302,        0.01318,        0.01334,        0.01349,        0.01364,
		0.01379,        0.01394,        0.01408,        0.01422,        0.01436,        0.01450,        0.01463,        0.01476,        0.01489,        0.01502,
		0.01514,        0.01526,        0.01539,        0.01550,        0.01562,        0.01573,        0.01585,        0.01596,        0.01607,        0.01617,
		0.01628,        0.01638,        0.01648,        0.01658,        0.01668,        0.01678,        0.01687,        0.01697,        0.01706,        0.01715,
		0.01724,        0.01733,        0.01741,        0.01750,        0.01758,        0.01767,        0.01775,        0.01783,        0.01791,        0.01798,
		0.01806,        0.01813,        0.01821,        0.01828,        0.01835,        0.01842,        0.01849,        0.01856,        0.01862,        0.01869,
		0.01875,        0.01881,        0.01888,        0.01894,        0.01900,        0.01905,        0.01911,        0.01917,        0.01922,        0.01928,
		0.01933,        0.01938,        0.01943,        0.01948,        0.01953,        0.01958,        0.01963,        0.01967,        0.01972,        0.01976,
		0.01981,        0.01985,        0.01989,        0.01993,        0.01997,        0.02001,        0.02005,        0.02008,        0.02012,        0.02016,
		0.02019,        0.02022,        0.02026,        0.02029,        0.02032,        0.02035,        0.02038,        0.02040,        0.02043,        0.02046,
		0.02048,        0.02051,        0.02053,        0.02055,        0.02057,        0.02060,        0.02062,        0.02064,        0.02065,        0.02067,
		0.02069,        0.02070,        0.02072,        0.02073,        0.02075,        0.02076,        0.02077,        0.02078,        0.02079,        0.02080,
		0.02081,        0.02082,        0.02083,        0.02083,        0.02084,        0.02084,        0.02085,        0.02085,        0.02085,        0.02085,
		0.02085,        0.02085,        0.02085,        0.02085,        0.02085,        0.02084,        0.02084,        0.02083,        0.02083,        0.02082,
		0.02081,        0.02080,        0.02079,        0.02078,        0.02077,        0.02076,        0.02075,        0.02073,        0.02072,        0.02070,
		0.02069,        0.02067,        0.02065,        0.02064,        0.02062,        0.02060,        0.02057,        0.02055,        0.02053,        0.02051,
		0.02048,        0.02046,        0.02043,        0.02040,        0.02038,        0.02035,        0.02032,        0.02029,        0.02026,        0.02022,
		0.02019,        0.02016,        0.02012,        0.02008,        0.02005,        0.02001,        0.01997,        0.01993,        0.01989,        0.01985,
		0.01981,        0.01976,        0.01972,        0.01967,        0.01963,        0.01958,        0.01953,        0.01948,        0.01943,        0.01938,
		0.01933,        0.01928,        0.01922,        0.01917,        0.01911,        0.01905,        0.01900,        0.01894,        0.01888,        0.01881,
		0.01875,        0.01869,        0.01862,        0.01856,        0.01849,        0.01842,        0.01835,        0.01828,        0.01821,        0.01813,
		0.01806,        0.01798,        0.01791,        0.01783,        0.01775,        0.01767,        0.01758,        0.01750,        0.01741,        0.01733,
		0.01724,        0.01715,        0.01706,        0.01697,        0.01687,        0.01678,        0.01668,        0.01658,        0.01648,        0.01638,
		0.01628,        0.01617,        0.01607,        0.01596,        0.01585,        0.01573,        0.01562,        0.01550,        0.01539,        0.01526,
		0.01514,        0.01502,        0.01489,        0.01476,        0.01463,        0.01450,        0.01436,        0.01422,        0.01408,        0.01394,
		0.01379,        0.01364,        0.01349,        0.01334,        0.01318,        0.01302,        0.01285,        0.01268,        0.01251,        0.01234,
		0.01216,        0.01197,        0.01178,        0.01159,        0.01139,        0.01119,        0.01098,        0.01077,        0.01055,        0.01033,
		0.01009,        0.00986,        0.00961,        0.00935,        0.00909,        0.00882,        0.00853,        0.00823,        0.00792,        0.00760,
		0.00726,        0.00689,        0.00651,        0.00610,        0.00566,        0.00517,        0.00463,        0.00402,        0.00329,        0.00233,
		0.00000
};

//-------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------



void fromAreasToDiameters(float *areas_in, int nSeg, float *diameters_out, bool flip=true) {
	if(flip) {
		for(int i=0; i<nSeg; i++) {
			diameters_out[nSeg-1-i] = areas_in[i]/M_PI;
			diameters_out[nSeg-1-i] = sqrt(diameters_out[nSeg-1-i])*2*radii_factor;
		}
	}
	else {
		for(int i=0; i<nSeg; i++) {
			diameters_out[i] = areas_in[i]/M_PI;
			diameters_out[i] = sqrt(diameters_out[i])*2*radii_factor;
			//diameters_out[i] = areas_in[i];
			//printf("%d - %f\n", i, diameters_out[i]);
		}
	}
}

void fromAreasToEllipticAxes(float *areas_in, int nSeg, float *axes_out, bool flip=true, bool minorAxis=false) {
	if(flip) {
		for(int i=0; i<nSeg; i++) {
			axes_out[nSeg-1-i] = 2*areas_in[i]/M_PI;
			axes_out[nSeg-1-i] = sqrt(axes_out[nSeg-1-i])*2*radii_factor;
			if(minorAxis)
				axes_out[nSeg-1-i] = axes_out[nSeg-1-i]/2;
		}
	}
	else {
		for(int i=0; i<nSeg; i++) {
			axes_out[i] = 2*areas_in[i]/M_PI;
			axes_out[i] = sqrt(axes_out[i])*2*radii_factor;
			if(minorAxis)
				axes_out[i] = axes_out[i]/2;
			//printf("%d - %f\n", i, axes_out[i] );
		}
	}
}

void fromAreasToHeights(float *areas_in, int nSeg, double refArea, double refHeight, float *heights_out) {
	for(int i=0; i<nSeg; i++) {
		heights_out[i] = sqrt(areas_in[i]/refArea); // scale factor
		heights_out[i] *= refHeight*radii_factor; // reference height * scale factor * shrinking factor...which needs to be further investigated

		//printf("%d - %f\n", i, heights_out[i] );
	}
}

inline float resampleSegments(float *tract, float len, int nSegments, float x_pos, float seg_space) {
	int segment = floor( x_pos / (len/nSegments) );
	segment = (segment>nSegments-1) ? nSegments-1 : segment;
	return tract[segment];
}

// turns an area function into a straight tube
bool createSymmetricTubeDomain(int tube_seg[], int tube_l, int domainSize[], float ***domainPixels, bool openEnd) {
	if(tube_l+1 > domainSize[0]) {
		printf("The specified tube needs at least a domain %d pixels long, while domain is only %d pixels\n", tube_l+1, domainSize[0]);
		return false;
	}

	float wallVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);

	int jumpH = 0;
	int jumpL = 0;
	int shift[tube_l];

	int contourPos[2] = {0, 0};

	for (int i=0; i<tube_l; i++) {
		shift[i] = (tube_seg[i]-1)/2;

		contourPos[0] = excitationCoord[1]+1+shift[i];
		contourPos[1] = excitationCoord[1]-1-shift[i];
		if(contourPos[0] > domainSize[1]) {
			printf("So far the specified tube needs at least a domain %d pixels high, while domain is only %d pixels\n", contourPos[0], domainSize[1]);
			return false;
		}
		else if(contourPos[1] < 0) {
			printf("Please either enlarge the domain or move the excitation up, both of at least %d pixels\n", -contourPos[1]);
			return false;
		}

		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+i+1, excitationCoord[1]+1+shift[i], contourPos[0], wallVal, domainPixels);
		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+i+1, excitationCoord[1]-1-shift[i], contourPos[1], wallVal, domainPixels);

		if (i>0) {
			jumpH = shift[i] - shift[i-1];
			jumpL = shift[i]- shift[i-1];

			// upper contour
			// i am taller than the previous wall cell
			if(jumpH > 1) {
				// put extra wall on top of prev neighbor on top side of tube
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i, excitationCoord[1]+1+shift[i-1], excitationCoord[1]+shift[i-1]+jumpH, wallVal, domainPixels);
			}
			// i am shorter than the previous wall cell
			else if (jumpH < -1) {
				jumpH = -jumpH;
				// put extra wall on top of this pixel on top side of tube
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i+1, excitationCoord[1]+1+shift[i], excitationCoord[1]+shift[i]+jumpH, wallVal, domainPixels);
			}

			// lower contour
			// i am taller than the previous wall cell
			if(jumpL > 1)// put extra wall below prev neighbor on lower side of tube
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i, excitationCoord[1]-shift[i-1]-jumpL, excitationCoord[1]-1-shift[i-1], wallVal, domainPixels);
			// i am shorter than the previous wall cell
			else if(jumpL < -1) { // put extra wall below this pixel on lower side of tube
				jumpL = -jumpL;
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i+1, excitationCoord[1]-shift[i]-jumpL, excitationCoord[1]-1-shift[i], wallVal, domainPixels);
			}
		}
	}

	// close the fucking tube
	// with a wall of excitations...
	float excitationVal = ImplicitFDTD::cell_excitation;
	// on top of current single excitation cell
	ImplicitFDTD::fillTextureColumn( excitationCoord[0], excitationCoord[1]+excitationDim[1],  excitationCoord[1]+excitationDim[1]+shift[0]-1, excitationVal, domainPixels);
	// and below it
	ImplicitFDTD::fillTextureColumn( excitationCoord[0], excitationCoord[1]-shift[0],  excitationCoord[1]-1, excitationVal, domainPixels);
	// ...and 2 wall cells on corners
	// on top
	ImplicitFDTD::fillTexturePixel( excitationCoord[0],  excitationCoord[1]+excitationDim[1]+shift[0], wallVal, domainPixels);
	// below
	ImplicitFDTD::fillTexturePixel( excitationCoord[0],  excitationCoord[1]-shift[0]-1, wallVal, domainPixels);

	// open-end condition
	if(openEnd) {
		// Dirichlet [Peter Gustav Lejeune] uniform boundary condition with p=0
		float noPressVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_nopressure);
		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+tube_l+1, excitationCoord[1]-shift[tube_l-1]-1,
										excitationCoord[1]+shift[tube_l-1]+1, noPressVal, domainPixels);
	}
	return true;
}


void createWidthMapPixels_straightTube(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int yPixels, int xPixels) { // xPixels does not include excitation wall
	// data structure to contain width of a single tube section
	float *columnWidth_c = new float[yPixels];
	float *columnWidth_r = new float[yPixels];
	float *columnWidth_u = new float[yPixels];

	float radius = (yPixels*ds)/2;


	// calculate width of single tube section, cos they're all the same here |:
	for(int j=0; j<yPixels; j++) {
		float segmentLen_c = fabs(radius-(j+0.5)*ds); // center of the pixel
		float segmentLen_u = fabs(radius-(j+1.0)*ds);   // top of the pixel
		columnWidth_c[j] = sqrt(radius*radius - segmentLen_c*segmentLen_c)*2;
		columnWidth_r[j] = columnWidth_c[j]; // same as center
		columnWidth_u[j] = sqrt(radius*radius - segmentLen_u*segmentLen_u)*2;
	}


	// now place width of slices inside the tube and walls above/below

	int start[2] = {excitationCoord[0], excitationCoord[1]+1-(yPixels+1)/2};
	int end[2] = {excitationCoord[0]+xPixels, excitationCoord[1]-1+(yPixels+1)/2};
	if( (yPixels%2)==0 )
		end[1] += 1;
	// leave last column out, for interpolation of inside tube with open space
	for(int i=0; i<xPixels; i++) {
		// walls
		// bottom walls
		widthMapPixels[start[0]+i][start[1]-1][0] = WALL_WIDTH;
		widthMapPixels[start[0]+i][start[1]-1][1] = WALL_WIDTH;
		widthMapPixels[start[0]+i][start[1]-1][2] = WALL_WIDTH;
		// top walls
		widthMapPixels[start[0]+i][end[1]+1][0] = WALL_WIDTH;
		widthMapPixels[start[0]+i][end[1]+1][1] = WALL_WIDTH;
		widthMapPixels[start[0]+i][end[1]+1][2] = MIN_WIDTH;
		for(int j=0; j<yPixels; j++) {
			widthMapPixels[start[0]+i][start[1]+j][0] = columnWidth_c[j];
			widthMapPixels[start[0]+i][start[1]+j][1] = columnWidth_u[j];
			widthMapPixels[start[0]+i][start[1]+j][2] = columnWidth_r[j];
		}
	}

	// last column

	// walls
	// bottom walls
	widthMapPixels[end[0]][start[1]-1][0] = WALL_WIDTH;
	widthMapPixels[end[0]][start[1]-1][1] = ( widthMapPixels[end[0]][start[1]-1][0]+widthMapPixels[end[0]+1][start[1]-1][0] ) / 2; // interpolated with outside
	widthMapPixels[end[0]][start[1]-1][2] = WALL_WIDTH;
	// top walls
	widthMapPixels[end[0]][end[1]+1][0] = WALL_WIDTH;
	widthMapPixels[end[0]][end[1]+1][1] = ( widthMapPixels[end[0]][end[1]+1][0]+widthMapPixels[end[0]+1][end[1]+1][0] ) / 2; // interpolated with outside
	widthMapPixels[end[0]][end[1]+1][2] = MIN_WIDTH;
	// inside tube, same as all the other columns except for right width
	for(int j=0; j<yPixels; j++) {
		widthMapPixels[end[0]][start[1]+j][0] = columnWidth_c[j];
		widthMapPixels[end[0]][start[1]+j][1] = ( widthMapPixels[end[0]][start[1]+j][0]+widthMapPixels[end[0]+1][start[1]+j][0] ) / 2; // interpolated with outside
		widthMapPixels[end[0]][start[1]+j][2] = columnWidth_u[j];
	}

	// last touch, to prevent zeros!
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}



	delete[] columnWidth_c;
	delete[] columnWidth_u;
	delete[] columnWidth_r;
}


void createWidthMapPixels_areaFunction(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int *yPixels, int xPixels, float *diameters) {
	// put basic walls
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(domainPixels[i][j][3] == ImplicitFDTD::cell_wall) {
				widthMapPixels[i][j][0] = WALL_WIDTH;
				widthMapPixels[i][j][1] = WALL_WIDTH;
				widthMapPixels[i][j][2] = WALL_WIDTH;
			}
		}
	}

	// tube sections' widths, excluding excitation layer

	bool useRealRadii = false;
	bool areaMatch = false;

	float radius       = -1;
	float approxRadius = -1;
	float realRadius   = -1;
	float pixelNormPos[2] = {-1, -1};
	float segmentLen[2]   = {-1, -1};

	int segH = -1;
	int startY = -1;
	//int endY = -1;

	// all in meters
	// cycle through segments [from left to right]
	for(int i=0; i<xPixels; i++) {

		segH = yPixels[i]/2; // half height of this segment
		startY = excitationCoord[1]-segH-1; // where segment starts...
		//endY = excitationCoord[1]+segH+1; // where segment ends...
		// ...basically position of enclosing walls

		approxRadius = (yPixels[i]/2.0)*ds;
		realRadius   = diameters[i]/2;


		float realArea = realRadius*realRadius*M_PI;
		float area = 0;

		// cycle through cells in segment [from top to bottom] to calculate width for each pixel cell [from the center of the pixel]
		for(int j=0; j<yPixels[i]; j++) {
			// calculate central and top widths! [top means top border of cell]
			if(!useRealRadii) {
				segmentLen[0] = fabs(approxRadius-j*ds - ds/2); // center
				segmentLen[1] = fabs(approxRadius-j*ds);        // top

				radius = approxRadius;
			}
			else {
				pixelNormPos[0] = fabs(approxRadius-j*ds - ds/2) / approxRadius;
				pixelNormPos[1] = fabs(approxRadius-j*ds) / approxRadius;

				segmentLen[0] = pixelNormPos[0]*realRadius; // center
				segmentLen[1] = pixelNormPos[1]*realRadius; // top

				radius = realRadius;
			}
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] = sqrt(radius*radius - segmentLen[0]*segmentLen[0])*2; // center
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] = sqrt(radius*radius - segmentLen[1]*segmentLen[1])*2; // top

			//printf("%i last seg: %f\n", j, widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2]);

			area += widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0]*ds/2;
			area += widthMapPixels[excitationCoord[2]+1+i][startY+1+j][0]*ds/2;
		}

		float ratio =  realArea/area;

		if(areaMatch) {
			area = 0;
			for(int j=0; j<yPixels[i]; j++) {
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] *= ratio; // center
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] *= ratio;

				area += widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0]*ds/2;
				area += widthMapPixels[excitationCoord[2]+1+i][startY+1+j][0]*ds/2;
			}
			printf("Real area: %f\n", realArea);
			printf("Area: %f\n", area);
			printf("Diff: %f\n", realArea-area);
			printf("Ratio: %f\n", realArea/area);
			printf("--------------\n");
		}

	}

	// first segment [where excitation is] is like wall
	segH = yPixels[0]/2; // half height of this segment
	startY = excitationCoord[1]-segH-1; // where segment starts...
	for(int j=0; j<yPixels[0]; j++) {
		widthMapPixels[excitationCoord[0]][startY+1+j][0] = WALL_WIDTH; // center
		widthMapPixels[excitationCoord[0]][startY+1+j][2] = WALL_WIDTH; // segmentsWidth[1][j][2]; // top, wall
	}


	// if we are in no radiation case [width inited as all NO_RADIATION_WIDTH], we set the width of open space to the width of the central cell of the last segment of the area function
	int seg = xPixels-1;
	int cell = yPixels[seg]/2;
	float open_space_width = widthMapPixels[excitationCoord[0]+1+seg-1][startY+1+cell][0];
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] == NO_RADIATION_WIDTH) {
				widthMapPixels[i][j][0] = open_space_width;
				widthMapPixels[i][j][1] = open_space_width;
				widthMapPixels[i][j][2] = open_space_width;
			}
		}
	}





	// compute all right widths as interpolations, r = (c+c_right)/2
	for(int i = 0; i <domainSize[0]-1; i++) {
		for(int j = 0; j <domainSize[1]-1; j++)
			widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
	}

	//for(int j = 0; j <domainSize[1]-1; j++)


	// this is edges case, i.e., no interpolation except for excitation [and shader...]
/*	for(int i = 0; i <domainSize[0]-1; i++) {
		if(i!=0) {
			for(int j = 0; j <domainSize[1]-1; j++)
				widthMapPixels[i][j][1] = widthMapPixels[i][j][0];
		} else {
			for(int j = 0; j <domainSize[1]-1; j++)
				widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
		}
	}*/

	// last touch, to prevent zeros! [and apply factor]
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			widthMapPixels[i][j][0] *= width_factor;
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			widthMapPixels[i][j][1] *= width_factor;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			widthMapPixels[i][j][2] *= width_factor;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}
}


void createWidthMapPixels_areaFunction_openSpaceWidth(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int *yPixels, int xPixels, float *diameters, float &openSpaceWidth) {

	bool useRealRadii = false;	// both true and false are good!
	bool areaMatch = false;
	bool bottomCellShift = false; // false, otherwise poor results/ instability in Matlab
	bool interpolateVxDepth = true; // true, otherwise very not a good overall formant profile and noise
	bool squeezeOnlyDepthWithinTube = false;

	// put basic walls
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(domainPixels[i][j][3] == ImplicitFDTD::cell_wall) {
				widthMapPixels[i][j][0] = WALL_WIDTH;
				widthMapPixels[i][j][1] = WALL_WIDTH;
				widthMapPixels[i][j][2] = WALL_WIDTH;
			}
		}
	}

	// tube sections' widths, excluding excitation layer

	float radius       = -1;
	float approxRadius = -1;
	float realRadius   = -1;
	float pixelNormPos[2] = {-1, -1};
	float segmentLen[2]   = {-1, -1};

	int segH = -1;
	int startY = -1;
	//int endY = -1;

	// all in meters
	// cycle through segments [from left to right]
	for(int i=0; i<xPixels; i++) {

		segH = yPixels[i]/2; // half height of this segment
		startY = excitationCoord[1]-segH-1; // where segment starts...
		//endY = excitationCoord[1]+segH+1; // where segment ends...
		// ...basically position of enclosing walls

		approxRadius = (yPixels[i]/2.0)*ds;
		realRadius   = diameters[i]/2;


		float realArea = realRadius*realRadius*M_PI;
		float area = 0;

		// cycle through cells in segment [from top to bottom] to calculate width for each pixel cell [from the center of the pixel]
		for(int j=0; j<yPixels[i]; j++) {
			// calculate central and top widths! [top means top border of cell]
			if(!useRealRadii) {
				segmentLen[0] = fabs(approxRadius-j*ds - ds/2  -(ds/2)*bottomCellShift ); // pressure pos
				segmentLen[1] = fabs(approxRadius-j*ds         -(ds/2)*bottomCellShift);  // vy pos
				radius = approxRadius;
			}
			else {
				pixelNormPos[0] = fabs(approxRadius-j*ds - ds/2 -(ds/2)*bottomCellShift) / approxRadius;
				pixelNormPos[1] = fabs(approxRadius-j*ds		-(ds/2)*bottomCellShift) / approxRadius;

				segmentLen[0] = pixelNormPos[0]*realRadius; // center
				segmentLen[1] = pixelNormPos[1]*realRadius; // top

				radius = realRadius;
			}
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] = sqrt(radius*radius - segmentLen[0]*segmentLen[0])*2 *width_factor; // pressure pos
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] = sqrt(radius*radius - segmentLen[1]*segmentLen[1])*2 *width_factor; // vy pos

			//printf("%i last seg: %f\n", j, widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2]);

			area += widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0]*ds/2;
			area += widthMapPixels[excitationCoord[2]+1+i][startY+1+j][0]*ds/2;
		}

		float ratio =  realArea/area;

		if(areaMatch) {
			area = 0;
			for(int j=0; j<yPixels[i]; j++) {
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] *= ratio; // center
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] *= ratio;

				area += widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0]*ds/2;
				area += widthMapPixels[excitationCoord[2]+1+i][startY+1+j][0]*ds/2;
			}
			printf("Real area: %f\n", realArea);
			printf("Area: %f\n", area);
			printf("Diff: %f\n", realArea-area);
			printf("Ratio: %f\n", realArea/area);
			printf("--------------\n");
		}

	}

	// first segment [where excitation is] is like wall
	segH = yPixels[0]/2; // half height of this segment
	startY = excitationCoord[1]-segH-1; // where segment starts...
	for(int j=0; j<yPixels[0]; j++) {
		widthMapPixels[excitationCoord[0]][startY+1+j][0] = WALL_WIDTH; // pressure pos
		widthMapPixels[excitationCoord[0]][startY+1+j][2] = WALL_WIDTH; // segmentsWidth[1][j][2]; // vy pos
	}


	// if we are in no radiation case [width inited as all NO_RADIATION_WIDTH], we set the width of open space to the width of the central cell of the last segment of the area function
	float new_openSpaceWidth;
	if(openSpaceWidth==NO_RADIATION_WIDTH) {
		int seg = xPixels-1;
		int cell = yPixels[seg]/2;
		new_openSpaceWidth = widthMapPixels[excitationCoord[0]+1+seg-1][startY+1+cell][0];

	}
	else
		new_openSpaceWidth = squeezeOnlyDepthWithinTube*openSpaceWidth  +  (1-squeezeOnlyDepthWithinTube)*openSpaceWidth*width_factor;

	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] == openSpaceWidth) {
				widthMapPixels[i][j][0] = new_openSpaceWidth;
				widthMapPixels[i][j][1] = new_openSpaceWidth;
				widthMapPixels[i][j][2] = new_openSpaceWidth;
			}
		}
	}

	openSpaceWidth = new_openSpaceWidth;





	if(interpolateVxDepth) {
		// compute all right widths as interpolations, r = (c+c_right)/2
		for(int i = 0; i <domainSize[0]-1; i++) {
			for(int j = 0; j <domainSize[1]; j++)
				widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
		}
		for(int j = 0; j <domainSize[1]; j++)
			widthMapPixels[domainSize[0]-1][j][1] = ( widthMapPixels[domainSize[0]-1][j][0] + openSpaceWidth ) / 2;
	}
	else {
		for(int i = 0; i <domainSize[0]; i++) {
			for(int j = 0; j <domainSize[1]; j++)
				widthMapPixels[i][j][1] = widthMapPixels[i][j][0]; // copy pressure depth to vx depth
		}
	}

	// last touch, to prevent zeros! [and apply factor]
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			//widthMapPixels[i][j][0] *= width_factor;
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			//widthMapPixels[i][j][1] *= width_factor;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			//widthMapPixels[i][j][2] *= width_factor;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}
}



void createWidthMapPixels_deb(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int *yPixels, int xPixels, float *diameters) {
	// put basic walls
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(domainPixels[i][j][3] == ImplicitFDTD::cell_wall) {
				widthMapPixels[i][j][0] = WALL_WIDTH;
				widthMapPixels[i][j][1] = WALL_WIDTH;
				widthMapPixels[i][j][2] = WALL_WIDTH;
			}
		}
	}

	// tube sections' widths, excluding excitation layer


	float approxRadius = -1;
	float segmentLen[2]   = {-1, -1};

	int segH = -1;
	int startY = -1;
	//int endY = -1;

	// all in meters
	// cycle through segments [from left to right]
	for(int i=0; i<xPixels; i++) {

		segH = yPixels[i]/2; // half height of this segment
		startY = excitationCoord[1]-segH-1; // where segment starts...
		//endY = excitationCoord[1]+segH+1; // where segment ends...
		// ...basically position of enclosing walls

		approxRadius = (yPixels[i]/2.0)*ds;


		// cycle through cells in segment [from top to bottom] to calculate width for each pixel cell [from the bottom of the pixel]
		for(int j=0; j<yPixels[i]; j++) {
			// calculate central and top widths! [top means top border of cell]
			segmentLen[0] = fabs(approxRadius-j*ds - ds); 	// bottom
			segmentLen[1] = fabs(approxRadius-j*ds - ds/2); // center on y

			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] = sqrt(approxRadius*approxRadius - segmentLen[0]*segmentLen[0])*2; // bottom
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] = sqrt(approxRadius*approxRadius - segmentLen[1]*segmentLen[1])*2; // center


			//widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] = approxRadius*2;
			//widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] = approxRadius*2;

			//printf("%i last seg: %f\n", j, widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2]);
		}

	}

	// first segment [where excitation is] is like wall
	segH = yPixels[0]/2; // half height of this segment
	startY = excitationCoord[1]-segH-1; // where segment starts...
	for(int j=0; j<yPixels[0]; j++) {
		widthMapPixels[excitationCoord[0]][startY+1+j][0] = WALL_WIDTH; // center
		widthMapPixels[excitationCoord[0]][startY+1+j][2] = WALL_WIDTH; // segmentsWidth[1][j][2]; // top, wall
	}


	// if we are in no radiation case [width inited as all NO_RADIATION_WIDTH], we set the width of open space to the width of the central cell of the last segment of the area function
	int seg = xPixels-1;
	int cell = yPixels[seg]/2;
	float open_space_width = widthMapPixels[excitationCoord[0]+1+seg-1][startY+1+cell][0];
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] == NO_RADIATION_WIDTH) {
				widthMapPixels[i][j][0] = open_space_width;
				widthMapPixels[i][j][1] = open_space_width;
				widthMapPixels[i][j][2] = open_space_width;
			}
		}
	}





	// compute all right widths as interpolations, r = (c+c_right)/2
	for(int i = 0; i <domainSize[0]-1; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
			//widthMapPixels[i][j][1] = widthMapPixels[i][j][0];
		}
	}


	// last touch, to prevent zeros! [and apply factor]
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			widthMapPixels[i][j][0] *= width_factor;
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			widthMapPixels[i][j][1] *= width_factor;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			widthMapPixels[i][j][2] *= width_factor;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}
}


void fillWidthMap_symmTest_story(float ***widthMapPixels, float ds, int *yPixels, int xPixels, float *heights_interp, double refHeight, double refWidthDs, double *refWidth) {

	// all these working vars are [2], where [0] is center and [1] is top
	double normPixelPos[2] = {0}; // normalized vertical position of current pixel per each segment
	double projectedPixelPos[2] = {0}; // vertical position of each pixel projected on width profile vertical line
	double segmentH = 0; // height of current segment, to normalize pixel pos
	int widthPntIndex[2][2] = {0}; // the index of the width profile points before and after the projectedPixelPos
	double pixelPntDist[2][2] = {0}; // distance between projectedPixelPos and width profile points before and after it, for weighed mean
	double pixelPntWeight[2][2] = {0}; // weight of each distance, for weighed mean
	int startY = -1; // top pixel inside wall boundaries, per each segment

	double scale = -1;

	// to make access clearer
	const int center = 0;
	const int top = 1;

	// cycle through segments [from left to right]
	for(int i=0; i<xPixels; i++) {
		segmentH = yPixels[i]*ds;
		startY = excitationCoord[1] - yPixels[i]/2 - 1; // where segment starts, below wall, cos excitationCoord[1] is vertical center of tube

		scale = heights_interp[i]/refHeight; // we sample the reference width using the normalized pixel position, but then we have to scale it according to height ratio between reference and current segment

		//printf("%d\n", i);

		// cycle through cells in segment [from top to bottom] to calculate width for each pixel cell [from the center of the pixel]
		for(int j=0; j<yPixels[i]; j++) {

			// pixel pos
			// top
			normPixelPos[top] = (ds*j)/segmentH;
			projectedPixelPos[top] = refHeight*normPixelPos[top];
			// center
			normPixelPos[center] = (ds*j + ds/2)/segmentH;
			projectedPixelPos[center] = refHeight*normPixelPos[center];


			// indices of width points that enclose projected pixel
			// top
			widthPntIndex[top][0] = projectedPixelPos[top]/refWidthDs;
			widthPntIndex[top][1] = widthPntIndex[top][0]+1;
			// center
			widthPntIndex[center][0] = projectedPixelPos[center]/refWidthDs;
			widthPntIndex[center][1] = widthPntIndex[center][0]+1;


			// compute width
			// top, always weighed mean
			pixelPntDist[top][0] = projectedPixelPos[top]-refWidthDs*widthPntIndex[top][0];
			pixelPntDist[top][1] = refWidthDs*widthPntIndex[top][1]-projectedPixelPos[top];

			// and relative weights
			pixelPntWeight[top][0] = (refWidthDs - pixelPntDist[top][0]) / refWidthDs;
			pixelPntWeight[top][1] = (refWidthDs - pixelPntDist[top][1]) / refWidthDs;

			// this is scaled!
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][top+1] = scale * ( refWidth[widthPntIndex[top][0]]*pixelPntWeight[top][0] + refWidth[widthPntIndex[top][1]]*pixelPntWeight[top][1] ) /
																	    ( pixelPntWeight[top][0] + pixelPntWeight[top][1] ); // top

			// center
			if(widthPntIndex[center][1]<SYMMTEST_SECTION_SAMPLES-1) { // if not last one, weighed mean
				// distances between enclosing width points and projected pixel
				pixelPntDist[center][0] = projectedPixelPos[center]-refWidthDs*widthPntIndex[center][0];
				pixelPntDist[center][1] = refWidthDs*widthPntIndex[center][1]-projectedPixelPos[center];

				// and relative weights
				pixelPntWeight[center][0] = (refWidthDs - pixelPntDist[center][0]) / refWidthDs;
				pixelPntWeight[center][1] = (refWidthDs - pixelPntDist[center][1]) / refWidthDs;

				// this is scaled!
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][center] = scale * ( refWidth[widthPntIndex[center][0]]*pixelPntWeight[center][0] + refWidth[widthPntIndex[center][1]]*pixelPntWeight[center][1] ) /
																			 ( pixelPntWeight[center][0] + pixelPntWeight[center][1] ); // center

			}
			else // if pixel coincides with last point, we just take width of last point
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][center] = scale * refWidth[widthPntIndex[center][0]]; // center...this is scaled!


			//printf("\t%d\t%f\n", j, widthMapPixels[excitationCoord[0]+1+i][startY+1+j][top]);
		}
	}
}


void createWidthMapPixels_symmTest_story(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int *yPixels, int xPixels, float *heights_interp, double refHeight, double refWidthDs, double *refWidth) {

	// put basic walls
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(domainPixels[i][j][3] == ImplicitFDTD::cell_wall) {
				widthMapPixels[i][j][0] = WALL_WIDTH;
				widthMapPixels[i][j][1] = WALL_WIDTH;
				widthMapPixels[i][j][2] = WALL_WIDTH;
			}
		}
	}

	// tube sections' widths, excluding excitation layer
	fillWidthMap_symmTest_story(widthMapPixels, ds, yPixels, xPixels, heights_interp, refHeight, refWidthDs, refWidth);


	// first segment [where excitation is] is like wall
	int segH = yPixels[0]/2; // half height of this segment
	int startY = excitationCoord[1]-segH-1; // where segment starts...
	for(int j=0; j<yPixels[0]; j++) {
		widthMapPixels[excitationCoord[0]][startY+1+j][0] = WALL_WIDTH; // center
		widthMapPixels[excitationCoord[0]][startY+1+j][2] = WALL_WIDTH; // segmentsWidth[1][j][2]; // top, wall
	}


	// if we are in no radiation case [width inited as all NO_RADIATION_WIDTH], we set the width of open space to the width of the central cell of the last segment of the area function
	int seg = xPixels-1;
	int cell = yPixels[seg]/2;
	float open_space_width = widthMapPixels[excitationCoord[0]+1+seg-1][startY+1+cell][0];
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] == NO_RADIATION_WIDTH) {
				widthMapPixels[i][j][0] = open_space_width;
				widthMapPixels[i][j][1] = open_space_width;
				widthMapPixels[i][j][2] = open_space_width;
			}
		}
	}

	// compute all right widths as interpolations, r = (c+c_right)/2
	for(int i = 0; i <domainSize[0]-1; i++) {
		for(int j = 0; j <domainSize[1]-1; j++)
			widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
	}

	// last touch, to prevent zeros!
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}

}




void createWidthMapPixels_areaFunction_elliptic(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int *yPixels, int xPixels, float *axes, bool rotated=false) {
	// put basic walls
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(domainPixels[i][j][3] == ImplicitFDTD::cell_wall) {
				widthMapPixels[i][j][0] = WALL_WIDTH;
				widthMapPixels[i][j][1] = WALL_WIDTH;
				widthMapPixels[i][j][2] = WALL_WIDTH;
			}
		}
	}

	// tube sections' widths, excluding excitation layer

	// we're gonna intersect the ellipse with a line parallel to x axis and passing through the different vertical pixels of each segment
	// this way we find the width of the pixels

	bool useRealAxes = true;

	double pixelNormPos[2] = {-1, -1};
	double segmentLen[2]   = {-1, -1};


	// semiaxes of each segment, a is along x, b is along y
	// the rotation paramter tells us which the major one and which is the minor one
	// this is done via a scale factor to apply to a, since we always get b [the one on y axis]

	// ellkiptical profile with one axis twice as long as the other one
	double a_scale;
	if(!rotated)
		a_scale = 0.5; // b minor, a major
	else
		a_scale = 2; // b major, a minor

	// x semiaxis
	double a = -1;
	double a_real   = -1;
	double a_approx = -1;

	// y semiaxis
	double b = -1;
	double b_real   = -1;
	double b_approx = -1;

	int segH = -1;
	int startY = -1;

	// all in meters
	// cycle through segments [from left to right]
	for(int i=0; i<xPixels; i++) {

		segH = yPixels[i]/2; // half height of this segment
		startY = excitationCoord[1]-segH-1; // where segment starts...
		// ...basically position of top wall

		b_real = axes[i]/2;
		b_approx = (yPixels[i]/2.0)*ds;

		a_real = b_real*a_scale;
		a_approx = b_approx*a_scale;


		// cycle through cells in segment [from top to bottom] to calculate width for each pixel cell [from the center of the pixel]
		for(int j=0; j<yPixels[i]; j++) {
			// calculate central and top widths! [top means top border of cell]
			if(!useRealAxes) {
				segmentLen[0] = fabs(b_approx-j*ds - ds/2); // center
				segmentLen[1] = fabs(b_approx-j*ds);        // top

				a = a_approx;
				b = b_approx;
			}
			else {
				pixelNormPos[0] = fabs(b_approx-j*ds - ds/2) / b_approx;
				pixelNormPos[1] = fabs(b_approx-j*ds) / b_approx;

				segmentLen[0] = pixelNormPos[0]*b_real; // center
				segmentLen[1] = pixelNormPos[1]*b_real; // top

				a = a_real;
				b = b_real;
			}
			// take only positive solution, which is half of the width...then multiply by 2
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][0] = fabs( sqrt(  (b*b - segmentLen[0]*segmentLen[0]) * (a*a)/(b*b) ) )*2; // center
			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][2] = fabs( sqrt( (b*b - segmentLen[1]*segmentLen[1]) * (a*a)/(b*b) ) )*2; // top
		}
	}

	// first segment [where excitation is] is like wall
	segH = yPixels[0]/2; // half height of this segment
	startY = excitationCoord[1]-segH-1; // where segment starts...
	for(int j=0; j<yPixels[0]; j++) {
		widthMapPixels[excitationCoord[0]][startY+1+j][0] = WALL_WIDTH; // center
		widthMapPixels[excitationCoord[0]][startY+1+j][2] = WALL_WIDTH; // segmentsWidth[1][j][2]; // top, wall
	}


	// if we are in no radiation case [width inited as all NO_RADIATION_WIDTH], we set the width of open space to the width of the central cell of the last segment of the area function
	int seg = xPixels-1;
	int cell = yPixels[seg]/2;
	float open_space_width = widthMapPixels[excitationCoord[0]+1+seg-1][startY+1+cell][0];
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] == NO_RADIATION_WIDTH) {
				widthMapPixels[i][j][0] = open_space_width;
				widthMapPixels[i][j][1] = open_space_width;
				widthMapPixels[i][j][2] = open_space_width;
			}
		}
	}

	// compute all right widths as interpolations, r = (c+c_right)/2
	for(int i = 0; i <domainSize[0]-1; i++) {
		for(int j = 0; j <domainSize[1]-1; j++)
			widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
	}

	// last touch, to prevent zeros!
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}

}


void fillWidthMapTube_symmTest(float ***widthMapPixels, float ds, int *yPixels, int xPixelsExtremes[], double height, double widthDs, double *widthProfile) {

	// all these working vars are [2], where [0] is center and [1] is top
	double normPixelPos[2] = {0}; // normalized vertical position of current pixel per each segment
	double projectedPixelPos[2] = {0}; // vertical position of each pixel projected on width profile vertical line
	double segmentH = 0; // height of current segment, to normalize pixel pos
	int widthPntIndex[2][2] = {0}; // the index of the width profile points before and after the projectedPixelPos
	double pixelPntDist[2][2] = {0}; // distance between projectedPixelPos and width profile points before and after it, for weighed mean
	double pixelPntWeight[2][2] = {0}; // weight of each distance, for weighed mean
	int startY = -1; // top pixel inside wall boundaries, per each segment

	// to make access clearer
	const int center = 0;
	const int top = 1;

	// cycle through segments of first small tube [from left to right]
	for(int i=xPixelsExtremes[0]; i<xPixelsExtremes[1]; i++) {
		segmentH = yPixels[i]*ds;
		startY = excitationCoord[1] - yPixels[i]/2 - 1; // where segment starts, below wall, cos excitationCoord[1] is vertical center of tube

		//printf("%d\n", i);

		// cycle through cells in segment [from top to bottom] to calculate width for each pixel cell [from the center of the pixel]
		for(int j=0; j<yPixels[i]; j++) {

			// pixel pos
			// top
			normPixelPos[top] = (ds*j)/segmentH;
			projectedPixelPos[top] = height*normPixelPos[top];
			// center
			normPixelPos[center] = (ds*j + ds/2)/segmentH;
			projectedPixelPos[center] = height*normPixelPos[center];


			// indices of width points that enclose projected pixel
			// top
			widthPntIndex[top][0] = projectedPixelPos[top]/widthDs;
			widthPntIndex[top][1] = widthPntIndex[top][0]+1;
			// center
			widthPntIndex[center][0] = projectedPixelPos[center]/widthDs;
			widthPntIndex[center][1] = widthPntIndex[center][0]+1;


			// compute width
			// top, always weighed mean
			pixelPntDist[top][0] = projectedPixelPos[top]-widthDs*widthPntIndex[top][0];
			pixelPntDist[top][1] = widthDs*widthPntIndex[top][1]-projectedPixelPos[top];

			// and relative weights
			pixelPntWeight[top][0] = (widthDs - pixelPntDist[top][0]) / widthDs;
			pixelPntWeight[top][1] = (widthDs - pixelPntDist[top][1]) / widthDs;

			widthMapPixels[excitationCoord[0]+1+i][startY+1+j][top+1] = ( widthProfile[widthPntIndex[top][0]]*pixelPntWeight[top][0] + widthProfile[widthPntIndex[top][1]]*pixelPntWeight[top][1] ) /
																	  ( pixelPntWeight[top][0] + pixelPntWeight[top][1] ); // top

			// center
			if(widthPntIndex[center][1]<SYMMTEST_SECTION_SAMPLES-1) { // if not last one, weighed mean
				// distances between enclosing width points and projected pixel
				pixelPntDist[center][0] = projectedPixelPos[center]-widthDs*widthPntIndex[center][0];
				pixelPntDist[center][1] = widthDs*widthPntIndex[center][1]-projectedPixelPos[center];

				// and relative weights
				pixelPntWeight[center][0] = (widthDs - pixelPntDist[center][0]) / widthDs;
				pixelPntWeight[center][1] = (widthDs - pixelPntDist[center][1]) / widthDs;

				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][center] = ( widthProfile[widthPntIndex[center][0]]*pixelPntWeight[center][0] + widthProfile[widthPntIndex[center][1]]*pixelPntWeight[center][1] ) /
																			 ( pixelPntWeight[center][0] + pixelPntWeight[center][1] ); // center

			}
			else // if pixel coincides with last point, we just take width of last point
				widthMapPixels[excitationCoord[0]+1+i][startY+1+j][center] = widthProfile[widthPntIndex[center][0]]; // center


			//printf("\t%d\t%f\n", j, widthMapPixels[excitationCoord[0]+1+i][startY+1+j][top]);

		}
	}
}

void createWidthMapPixels_symmTest(float ***widthMapPixels, float ***domainPixels, int domainSize[], float ds, int *yPixels, int xPixels, double height[], double widthDs[], double *widthProfile[2]) {
	// put basic walls
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(domainPixels[i][j][3] == ImplicitFDTD::cell_wall) {
				widthMapPixels[i][j][0] = WALL_WIDTH;
				widthMapPixels[i][j][1] = WALL_WIDTH;
				widthMapPixels[i][j][2] = WALL_WIDTH;
			}
		}
	}



	// tube sections' widths, excluding excitation layer

	// to make access clearer
	const int small = 0;
	const int big = 1;


	int xPixelsExtremes[2] = {0};

	// x pixels extremes to cover the whole length of the first tube, the small one
	xPixelsExtremes[0] = 0; // start from beginning
	xPixelsExtremes[1] = ceil(xPixels/2); // if odd len, makes first one +1 longer

	// compute top and center widths for small tube
	fillWidthMapTube_symmTest(widthMapPixels, ds, yPixels, xPixelsExtremes, height[small], widthDs[small], widthProfile[small]);


	// x pixels extremes to cover the whole length of the second tube, the bitg one
	xPixelsExtremes[0] = xPixelsExtremes[1]; // start from where we left off
	xPixelsExtremes[1] = xPixels; // go to very end

	// compute top and center widths for small tube
	fillWidthMapTube_symmTest(widthMapPixels, ds, yPixels, xPixelsExtremes, height[big], widthDs[big], widthProfile[big]);





	// first segment [where excitation is] is like wall
	int segH = yPixels[0]/2; // half height of this segment
	int startY = excitationCoord[1]-segH-1; // where segment starts...
	for(int j=0; j<yPixels[0]; j++) {
		widthMapPixels[excitationCoord[0]][startY+1+j][0] = WALL_WIDTH; // center
		widthMapPixels[excitationCoord[0]][startY+1+j][2] = WALL_WIDTH; // segmentsWidth[1][j][2]; // top, wall
	}


	// if we are in no radiation case [width inited as all NO_RADIATION_WIDTH], we set the width of open space to the width of the central cell of the last segment of the area function
	int seg = xPixels-1;
	segH = yPixels[xPixels-1]/2; // half height of this segment
	startY = excitationCoord[1]-segH-1; // where segment starts...
	int cell = yPixels[seg]/2;
	float open_space_width = widthMapPixels[excitationCoord[0]+xPixels][startY+1+cell][0];
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] == NO_RADIATION_WIDTH) {
				widthMapPixels[i][j][0] = open_space_width;
				widthMapPixels[i][j][1] = open_space_width;
				widthMapPixels[i][j][2] = open_space_width;
			}
		}
	}




	// compute all right widths as interpolations, r = (c+c_right)/2
/*	for(int i = 0; i <domainSize[0]-1; i++) {
		for(int j = 0; j <domainSize[1]-1; j++)
			widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
	}
*/

	//VIC it seems that the interpolation in the shader changes the geometry enough to move the formants...
	// and can't be removed, otherwise the simulation explodes
	// better try with a more continuously changing geometry, so that interpolations [all of them] do not affect the acoustics
	// long story short, make elliptic story08 and compare formants with regular one...up to 5k should be same!
	//
	// also
	//
	// in this implementation the computation of the tubes' width is not affected by raddi_factor
	// cos widths are simply resampled from a pre-computed width map...this is wrong and should be fixed, probably in Octave

	// this is edges case, i.e., no interpolation except for excitation [and shader...]
	for(int i = 0; i <domainSize[0]-1; i++) {
		if(i!=0) {
			for(int j = 0; j <domainSize[1]-1; j++)
				widthMapPixels[i][j][1] = widthMapPixels[i][j][0];
		} else {
			for(int j = 0; j <domainSize[1]-1; j++)
				widthMapPixels[i][j][1] = ( widthMapPixels[i][j][0] + widthMapPixels[i+1][j][0] ) / 2;
		}
	}




	// last touch, to prevent zeros!
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			if(widthMapPixels[i][j][0] <MIN_WIDTH)
				widthMapPixels[i][j][0] = MIN_WIDTH;
			if(widthMapPixels[i][j][1] <MIN_WIDTH)
				widthMapPixels[i][j][1] = MIN_WIDTH;
			if(widthMapPixels[i][j][2] <MIN_WIDTH)
				widthMapPixels[i][j][2] = MIN_WIDTH;
		}
	}


}

int createStoryVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds,
						 int domainSize[], float ***domainPixels, float ***widthMapPixels, float &openSpaceWidth, bool openEnd) {
	float ds_cm    = ds*100;
	float x_pos_cm = 0;
	int xPixels    = round((vowelAreaFunc_len)/ds_cm);
	int nSegments  = (vowelAreaFunc_len/vowel_spacing); // recover original number of segments of raw data

	float diameters[nSegments];
	fromAreasToDiameters(vowelAreaFunc, nSegments, diameters, false);

	float *diameters_interp = new float[xPixels];
	int *yPixels = new int[xPixels];
	for(int i=0; i<xPixels; i++) {
		diameters_interp[i] = resampleSegments(diameters, vowelAreaFunc_len, nSegments, x_pos_cm, vowel_spacing); // calculates the area in the requested x position and returns diameter of circle with same area
		diameters_interp[i] /= 100; // back to meters
		x_pos_cm += ds_cm; // next point

		// in meters
		// check which is the closest odd approximation
		yPixels[i] =  floor(diameters_interp[i]/ds); // in meters
		if(yPixels[i]%2 == 0)
			yPixels[i] =  ceil(diameters_interp[i]/ds);
		// juuust in case
		if(yPixels[i]%2 == 0)
			yPixels[i]++;
	}


	if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, openEnd))
		return -1;

	//createWidthMapPixels_areaFunction(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, diameters_interp);
	createWidthMapPixels_areaFunction_openSpaceWidth(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, diameters_interp, openSpaceWidth);
	//createWidthMapPixels_deb(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, diameters_interp);

	delete[] diameters_interp;
	delete[] yPixels;

	return xPixels+1; // +1 is excitation
}

int createTwoTubesCentric(float ds, int domainSize[], float ***domainPixels, float ***widthMapPixels, bool openEnd) {
	// discretized measures

	// both tubes
	float length = tubeslen/1000.0; // to m
	int xPixels = round(length/ds); // same for both tubes
	int totalXpixels = 2*xPixels;

	// first tube
	tube0Diameter*=radii_factor;  // apply global factor
	float diameter0 = tube0Diameter/1000.0; // to m
	int yPixels0 = floor(diameter0/ds);
	if(yPixels0%2 == 0)
		yPixels0 =  ceil(diameter0/ds);
	// juuust in case
	if(yPixels0%2 == 0)
		yPixels0++;

	// second tube
	tube1Diameter*=radii_factor; // apply global factor
	float diameter1 = tube1Diameter/1000.0; // to m
	int yPixels1 = floor(diameter1/ds);
	if(yPixels1%2 == 0)
		yPixels1 =  ceil(diameter1/ds);
	// juuust in case
	if(yPixels1%2 == 0)
		yPixels1++;

	int mxYPixels = yPixels1; // we know second tube has bigger diameter
	if(totalXpixels+1 > domainSize[0]) { // we need an extra pixel at the beginning of the first tube to close it [with excitation]
		printf("The specified tube needs at least a domain %d pixels long, while domain is only %d pixels\n", xPixels, domainSize[0]);
		return -1;
	} else if(mxYPixels > domainSize[1]) {
		printf("The specified tube needs at least a domain %d pixels high, while domain is only %d pixels\n", mxYPixels, domainSize[1]);
		return -1;
	}


	printf("----------------------------------------------------------------------------------\n");
	printf("Geometry  size:\n");
	printf("\n");
	printf("\tlength:\t%f mm (%d pixels => %f mm error)\n", tubeslen*2, totalXpixels, ( totalXpixels*ds-length*2 ) * 1000);
	printf("\n");
	printf("\tSecond tube size:\n");
	printf("\t\tlength: \t%f mm (%d pixels => %f mm error)\n", tubeslen, xPixels, ( xPixels*ds-length ) * 1000);
	printf("\t\tdiamater:\t%f mm (%d pixels => %f mm error)\n", tube0Diameter, yPixels0, ( yPixels0*ds-diameter0 ) * 1000);
	printf("\n");
	printf("\tSecond tube size:\n");
	printf("\t\tlength: \t%f mm (%d pixels => %f mm error)\n", tubeslen, xPixels,( xPixels*ds-length ) * 1000); // same as first tube
	printf("\t\tdiamater:\t%f mm (%d pixels => %f mm error)\n", tube1Diameter, yPixels1, ( yPixels1*ds-diameter1 ) * 1000);
	printf("----------------------------------------------------------------------------------\n");

	// draw geometry

	/*
	// save this for 2TubesEccentric
	float wallVal       = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);
	float excitationVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_excitation);

	// first tube
	int halfWidth = (yPixels0+1)/2;
	int startX    = excitationCoord[0];
	int stopX     = startX+xPixels;
	int top       = excitationCoord[1]+halfWidth;
	int bottom    = excitationCoord[1]-halfWidth;

	// upper boundary
	ImplicitFDTD::fillTextureRow(top,    startX, stopX, wallVal, domainPixels); // here there is also the extra pixel for excitation closure
	// lower boundary
	ImplicitFDTD::fillTextureRow(bottom, startX, stopX, wallVal, domainPixels); // here there is also the extra pixel for excitation closure
	// close excitation side
	ImplicitFDTD::fillTextureColumn(startX, bottom+1, top-1, excitationVal, domainPixels);

	// second tube
	halfWidth = (yPixels1+1)/2;
	startX    = stopX;
	stopX     = startX+xPixels;
	top       = excitationCoord[1]+halfWidth;
	bottom    = excitationCoord[1]-halfWidth;

	// upper boundary
	ImplicitFDTD::fillTextureRow(top,    startX, stopX, wallVal, domainPixels); // here there is also the extra pixel, overlapped with previous tube [which is smaller]
	// lower boundary
	ImplicitFDTD::fillTextureRow(bottom, startX, stopX, wallVal, domainPixels); // here there is also the extra pixel, overlapped with previous tube [which is smaller]

	// fill possible gap between the 2 tubes
	int diff = (yPixels1-yPixels0)/2;
	// otherwise there is no gap
	if(diff > 1) {
		// upper gap
		ImplicitFDTD::fillTextureColumn(startX, top-diff+1, top-1, wallVal, domainPixels);
		// lower gap
		ImplicitFDTD::fillTextureColumn(startX, bottom+1, bottom+diff-1, wallVal, domainPixels);
	}*/


	int *yPixels = new int[totalXpixels];
	float *diameters = new float[totalXpixels];

	for(int i=0; i<xPixels; i++) {
		yPixels[i]  = yPixels0;
		diameters[i] = diameter0;
	}

	for(int i=xPixels; i<2*xPixels; i++) {
		yPixels[i]   = yPixels1;
		diameters[i] = diameter1;
	}


	if(!createSymmetricTubeDomain(yPixels, totalXpixels, domainSize, domainPixels, openEnd))
		return -1;

	createWidthMapPixels_areaFunction(widthMapPixels, domainPixels, domainSize, ds, yPixels, totalXpixels, diameters);

	delete[] yPixels;
	delete[] diameters;

	return totalXpixels+1;
}




int createSymmTestTube(float ds, int domainSize[], double tubeHeight[], double widthDs[], double *widthProfile[2], float ***domainPixels, float ***widthMapPixels, bool openEnd) {
	int xPixels  = round((symmTest_tubeslen*2)/ds);
	int *yPixels = new int[xPixels];

	// length in pixels of first tube
	int xOneTube = ceil(xPixels/2); // if odd len, makes first one +1 longer

	// height in pixels of first tube
	int yTubeSmall = floor(tubeHeight[0]*radii_factor/ds);
	if(yTubeSmall%2 == 0) // must be odd, to be symmetric
		yTubeSmall = ceil(tubeHeight[0]*radii_factor/ds);
	// height in pixels of second tube
	int yTubeBig   = floor(tubeHeight[1]*radii_factor/ds);
	if(yTubeBig%2 == 0) // must be odd, to be symmetric
		yTubeBig = ceil(tubeHeight[1]*radii_factor/ds);

	// assign heights
	for(int i=0; i<xOneTube; i++)
		yPixels[i] = yTubeSmall;
	for(int i=xOneTube; i<xPixels; i++)
		yPixels[i] = yTubeBig;

	if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, openEnd))
		return -1;

	createWidthMapPixels_symmTest(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, tubeHeight, widthDs, widthProfile);



/*
	float *diameters = new float[xPixels];

	for(int i=0; i<ceil(xPixels/2); i++)
		diameters[i] = 0.014500;

	for(int i=ceil(xPixels/2); i<xPixels; i++)
		diameters[i] = 0.029500;

	createWidthMapPixels_areaFunction(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, diameters);

	delete[] diameters;
*/



	delete[] yPixels;

	return xPixels+1; // +1 is excitation
}

int createStoryVowelTube_symmTest(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds, int domainSize[],
								  double refArea, double refHeight, double refWidthDs, double *refWidth,
								  float ***domainPixels, float ***widthMapPixels, bool openEnd) {

	float ds_cm    = ds*100;
	float x_pos_cm = 0;
	int xPixels    = round((vowelAreaFunc_len)/ds_cm);
	int nSegments  = (vowelAreaFunc_len/vowel_spacing); // recover original number of segments of raw data

	float heights[nSegments];
	fromAreasToHeights(vowelAreaFunc, nSegments, refArea, refHeight, heights);

	float *heights_interp = new float[xPixels];
	int *yPixels = new int[xPixels];
	for(int i=0; i<xPixels; i++) {
		heights_interp[i] = resampleSegments(heights, vowelAreaFunc_len, nSegments, x_pos_cm, vowel_spacing); // calculates the area in the requested x position and returns diameter of circle with same area
		heights_interp[i] /= 100; // back to meters
		x_pos_cm += ds_cm; // next point

		// in meters
		// check which is the closest odd approximation
		yPixels[i] =  floor(heights_interp[i]/ds); // in meters
		if(yPixels[i]%2 == 0)
			yPixels[i] =  ceil(heights_interp[i]/ds);
		// juuust in case
		if(yPixels[i]%2 == 0)
			yPixels[i]++;
	}


	if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, openEnd))
		return -1;

	createWidthMapPixels_symmTest_story(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, heights_interp, refHeight, refWidthDs, refWidth);

	delete[] heights_interp;
	delete[] yPixels;

	return xPixels+1; // +1 is excitation
}

// story vowel tube with elliptic cross sections [cross sections symmetric on y and z axes]
int createYzSymmStoryVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds,
						         int domainSize[], float ***domainPixels, float ***widthMapPixels, bool openEnd, bool rotated) {
	float ds_cm    = ds*100;
	float x_pos_cm = 0;
	int xPixels    = round((vowelAreaFunc_len)/ds_cm);
	int nSegments  = (vowelAreaFunc_len/vowel_spacing); // recover original number of segments of raw data

	float axes[nSegments];
	fromAreasToEllipticAxes(vowelAreaFunc, nSegments, axes, false, rotated);
	//fromAreasToDiameters(vowelAreaFunc, nSegments, axes, false);

	float *axes_interp = new float[xPixels];
	int *yPixels = new int[xPixels];
	for(int i=0; i<xPixels; i++) {
		axes_interp[i] = resampleSegments(axes, vowelAreaFunc_len, nSegments, x_pos_cm, vowel_spacing); // calculates the area in the requested x position and returns diameter of circle with same area
		axes_interp[i] /= 100; // back to meters
		x_pos_cm += ds_cm; // next point

		// in meters
		// check which is the closest odd approximation
		yPixels[i] =  floor(axes_interp[i]/ds); // in meters
		if(yPixels[i]%2 == 0)
			yPixels[i] =  ceil(axes_interp[i]/ds);
		// juuust in case
		if(yPixels[i]%2 == 0)
			yPixels[i]++;
	}


	if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, openEnd))
		return -1;

	createWidthMapPixels_areaFunction_elliptic(widthMapPixels, domainPixels, domainSize, ds, yPixels, xPixels, axes_interp);

	delete[] axes_interp;
	delete[] yPixels;

	return xPixels+1; // +1 is excitation
}
