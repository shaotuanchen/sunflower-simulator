/*
	C program to infer the angular rate of the pendulum based on acceleration data.
	
	Authored 2017 by Shao-Tuan Chen.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "logmarkers.h"
#include "l45InSync.h"

/*
 *	Variable declaration:
 *
 *		PI : mathematical constant.
 *
 *  		t : time step, 0.1s since the sampling rate is 10 Hz.
 *
 *		l : length of the pendulum, 0.1 meter.
 *
 *		g : acceleration due to gravity, 9.81 m/s^2.
 *   
 */

#define PI 3.14159265359
#define t 0.1
#define g 9.81
#define theta0 5


/*
 *	Notes:
 *
 *	(0)	"logmarkers.h" is the header file for LOGMARK, a macro to specify 
 * 		performance counting for Sunflower.
 *
 *  	Arrays:  
 *
 *	(1)	long double acceleration[61] : acceleration data acquired with MPU-9250. 
 *
 *	(2)	long double radian[inferLength] : radian as a function of time, based on the length 
 *		of the pendulum "l" and gravity "g".
 *
 *	(3)	long double gcos[inferLength] : cosine component of gravity g. According to 
 *		Equation 4 in the paper, we can obtain theta, the angular displacement
 * 		as a function of time, given the length and the initial angular
 * 		displacement of the pendulum. In the paper, the initial angle is 5
 * 		degrees, and the length is 10 cm. We then calculate "g cos (theta(t))"
 * 		based on "radian" array, and use as the input to Sunflower. The calculation
 * 		can be done offline, since it doesn't require any experimental data,
 * 		and it is a sinusoidal signal with constant amplitude. This is the 
 * 		reason why we've put "LOGMARK" after generating the gcos array. Constant 
 * 		amplitude approximation is the cause as in Figure 3, at the start of
 * 		the pendulum swing there's an overshoot for the inferred angular rate,
 * 		compared to the angular rate from the gyroscope. However, the results 
 *		show the inferred angular rate with high coherence to gyroscope data,  
 *		because the initial angle is small, and constant approximation for gcos 
 * 		can hold. For larger angle swings, we can further consider a second-order 
 *		approximation for gcos, where we treat theta as a damped response to 
 *		improve the accuracy for the inferred angular rate with acceleration data.
 *
 *	(4)	long double sign[inferLength]: sign direction array, based on the period of the 
 *		pendulum. This array creates a square wave, which is applied to give the 
 *		correct direction of the inferred angular rate. The sign array gives a value 
 *		of "+1", when the value of gcos array is positive, and "-1" when the value 
 *		of gcos array is negative.
 *
 *	(5)	long double inferred[inferLength] : output array for inferred angular rate based on 
 *		Equation 10 in the paper, where 0.1 (meter) is the length of the pendulum.
 *		Measurement noise causes erroneous computation of "NaN", in the inferred 
 *		angular rate when |acceleration[i]| < |gcos[i]|. 
 *		To ensure no erroneous value is present, the value of inferred[i] is assigned 
 *		to be assigned[i-1] if |acceleration[i]| < |gcos[i]|.  
 *
 */



/*
 *	Variable declaration:
 *
 *		PI : mathematical constant.
 *
 *  		t : time step, 0.1 second since the sampling rate is 10 Hz.
 *
 *		l : length of the pendulum, 0.1 meter.
 *
 *		g : acceleration due to gravity, 9.81 m/s^2.
 *   
 */

int
startup(int argc, char *argv[]) 
{
			
/*
*	Notes:
*
*	(1)	The acceleration and gcos need to be in sync with a 180 degrees phase angle
*		to calculate the inferred angular rate from acceleration and gcos. 
*
*	(2)	We set the start condition for aligning acceleration with gcos to be when
*		the change of measured acceleration exceeds certain value, here we choose 
*		the threshold to be 0.03 m/s^2.	This is possible since we are measuring the
* 		dynamics of the pendulum from standstill, therefore we can be sure the pendulum
*		has started swinging when the measured acceleration changes significantly.
*
*	(3)	When performing experiment, we always record the angular rate to start from 
*		negative value to positive value. If the sign order is reversed, we just need
*		to change the condition in sign[i] array from ">= PI ? 1 : -1" to ">= PI ? -1 : 1".
*		However, this won't affect the start condition we have set in (2), since either the
*		angular rate changes from positive to negative, or negative to positive, the 
*		measured acceleration in both cases increase in value.
*
*/	
		
		int numberOfSamples = sizeof(acceleration)/sizeof(long double);	
		int startIndex = 0;	
		int i = 0;
				
		long double startValue = acceleration[startIndex];

		for (i = 0; i < numberOfSamples; i++)
		{
				if ((acceleration[i] - acceleration[i+1]) > 0.7) 
				{
					startIndex = i;
					startValue = acceleration[startIndex];
			
					break;
				}
		}
		
		printf ("Start value: %Lf \n", startValue);	
		printf ("Start index: %i, numberOfSamples: %d\n", startIndex, numberOfSamples);
/*
*	Notes:
*
*	(1)	Here we print out the value and index of the measured acceleration which is aligned 
*		with gcos for us to manually check whether the start condition is satisfied.
*
*/	
		
		int 			inferLength = numberOfSamples - startIndex;
		int				noInfer = startIndex;
		
		long double		radian[numberOfSamples];
		long double		gcos[numberOfSamples];
		long double		inferred[numberOfSamples];
		long double		sign[inferLength];
		
		for (i = 0; i < noInfer; i++) 
		{
				gcos[i] = g ;
				inferred[i] = angularRate[i] ; 
		}
		
		for (i = 0; i < inferLength; i++) 
		{
				radian[i] = sqrt(g / l) * t * i;
				gcos[i + startIndex] = g * cos(theta0 * PI / 180 * cos(radian[i]));
				sign[i] = fmod(radian[i], 2 * PI) >= PI ? -1 : 1;
		}
					
		LOGMARK(0);
		
		for (i = 0; i < inferLength; i++) 
		{
				if (fabs(acceleration[i + startIndex]) > fabs(gcos[i + startIndex]))
				{
						inferred[i + startIndex] = sign[i] * sqrt ((-acceleration[i + startIndex] - gcos[i + startIndex]) / l) ; 
				}
				else 
				{
						inferred[i + startIndex] = inferred[i + startIndex - 1] ; 
				}		
		}

		LOGMARK(1);
	
		for ( i = 0; i < numberOfSamples; i++ ) 
		{
				/*printf ("%Lf, %Lf, %Lf, %Lf \n", acceleration[i], angularRate[i], inferred[i], gcos[i]);
				*/
			
			printf("%Lf \n", gcos[i]);
		}
			
	
		LOGMARK(2);
 
		return 0;
	}
