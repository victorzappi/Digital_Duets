/*
 * QuadraticMaxima.h
 *
 *  Created on: 2016-06-30
 *      Author: Victor Zappi
 *      Comments: Quadratic class adapted from JASS source code [jass/src/jass/utils/formantsPlotter.java]
 */

#ifndef QUADRATICMAXIMA_H_
#define QUADRATICMAXIMA_H_

class QuadraticMaxima {
public:
	void set(double x0, double x1, double x2, double y0, double y1, double y2);
	bool containsPeak();
	double getUpper3dB();
	double getLower3dB();
	double getPeakX();
	double getPeakY();
	double findX_left(double Y);
	double findX_right(double Y);

private:
	double a;
	double b;
	double c;
	bool hasPeak;
	double peakX;
	double peakY;
};


void QuadraticMaxima::set(double x0, double x1, double x2, double y0, double y1, double y2) {
    //uses lagrangian polynomials to find interpolation formula
    //denominators for each term in the series

    double l_0den = x0*x0 - x0*(x1+x2) + x1*x2;
    double l_1den = x1*x1 - x1*(x0+x2) + x0*x2;
    double l_2den = x2*x2 - x2*(x0+x1) + x0*x1;

    if ( (l_0den != 0) && (l_1den != 0) && (l_2den != 0) ) {

		//a = x^2 term; b = x^1; term c = x^0
		a = y0/l_0den + y1/l_1den + y2/l_2den;
		b = -y0*(x1+x2)/l_0den - y1*(x0+x2)/l_1den - y2*(x0+x1)/l_2den;
		c = y0*x1*x2/l_0den + y1*x0*x2/l_1den + y2*x0*x1/l_2den;

		peakX = -0.5*b/a; //X value of possible maximum
		peakY = a*peakX*peakX + b*peakX + c; //Y value of possible maximum

		//maximum occurs when:
		//-concave down (2a < 0)
		//-middle value larger than y0 and y2
		//-X value is within x0 and x2

		if ( (a<0) && (y1>y0) && (y1>y2) && (peakX>=x0) && (peakX <= x2) )
			hasPeak = true;
		else {
			hasPeak = false;
			peakX = -1;
		}
    }
    else {
		hasPeak = false;
		peakX   = -1;
		peakY   = 0;
    }
}



inline bool QuadraticMaxima::containsPeak() {
    return hasPeak;
}

double QuadraticMaxima::getUpper3dB() {
    return findX_right(peakY - 3);
}

double QuadraticMaxima::getLower3dB() {
    return findX_left(peakY - 3);
}

inline double QuadraticMaxima::getPeakX() {
    return peakX;
}

inline double QuadraticMaxima::getPeakY() {
    return peakY;
}

double QuadraticMaxima::findX_left(double Y) {
    double shiftedC = c - Y;

    if (a!= 0)
    	return (-b + sqrt(b*b-4*a*shiftedC)) / (2*a);
    else
    	return -shiftedC/b;
}

double QuadraticMaxima::findX_right(double Y) {
    double shiftedC = c - Y;
    if (a!=0)
    	return (-b - sqrt(b*b-4*a*shiftedC)) / (2*a);
    else
    	return -shiftedC/b;
}



#endif /* QUADRATICMAXIMA_H_ */
