// Example : encodes a message in a colour picture, using a random algorithm
// usage: Encoder <carrier image> <message image> <encoded image>

// Author : João Faria, joao.faria@fe.up.pt

// Copyright (c) 2011 School of Engineering, Cranfield University
// License : LGPL - http://www.gnu.org/licenses/lgpl.html

#include <cv.h>   	// open cv general include file
#include <highgui.h>	// open cv GUI include file
#include <sys/stat.h>
#include <fstream>

using namespace cv; // OpenCV API is in the C++ "cv" namespace


//Hash function from an external source
//Available from the following URL: http://http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

//Get the n-th bit of a sequence
//Original from URL: http://public.cranfield.ac.uk/c5354/teaching/dip/practicals/ip/bit.c
//Slightly changed to fit in data types
unsigned char getBitN(int val, int n) 
{
     unsigned char b;
     b = 1 & (val >> n);
     return b;
 }

unsigned char getBitN2(unsigned char val, int n) 
{
     unsigned char b;
     b = 1 & (val >> n);
     return b;
 }

//generates "random" noise in an image using a gaussian distribution
void noiseGenerator(Mat *image, double sigma, RNG randomGenerator)
{
	double generatedNumber;
	int n;

	//each position has its index value
	for(int i=0; i<image->rows; i++)
		for(int j=0; j<image->cols; j++)
		{
			for(int k=0; k<3; k++)
			{
				//Gaussian distribution.
				//If it was used a linear distribution instead, the image wouldn't
				//keep is caracteristics. This way, the probability of the pixel
				//acquire a value too distant from its original is lower.
				generatedNumber = randomGenerator.gaussian(sigma);
				n = image->at<Vec3b>(i,j)[k]+generatedNumber;

				//if overflow, put inside boundaries
				if(n>255)
					n=255;
				else if(n<0)
					n=0;

				image->at<Vec3b>(i,j)[k] = n;
			}
		}
}

int main( int argc, char** argv )
{
	Mat carrier;  // input carrier image object

	// check that command line arguments are provided and images read in OK
	if ((argc == 4) && !(carrier = imread( argv[1], CV_LOAD_IMAGE_COLOR)).empty())
	{
		//get encoding file size in bytes
		struct stat filestatus;
		stat(argv[2], &filestatus);
		unsigned long fileSize;

		//error when file is too big
		if(filestatus.st_size>=pow(2.0,32))
		{
			std::cerr << "Error! File is too big comparing to image size." << std::endl;
			return -1;
		}
		
		fileSize = filestatus.st_size;

		//error when size doesn't fit
		if(carrier.cols*carrier.rows-32<fileSize)
		{
			std::cerr << "Error! File is too big comparing to image size." << std::endl;
			return -1;
		}
		
		//image is too small
		if(carrier.cols*carrier.rows-32<0)
		{
			std::cerr << "Error! Image is too small." << std::endl;
			return -1;
		}

		string password;

		//asks the user for a password
		std::cout << "Please, introduce a password: ";
		std::cin >> password;

		//password will originate a seed to be used in the generation of "random" numbers
		unsigned long seed = hash((unsigned char*) password.c_str());

		RNG randomGenerator = RNG(seed);
		
		//Generates noise in the carrier image
		noiseGenerator(&carrier,3,randomGenerator);

		int x;
		int y;
		int channel;
		bool keepGoing;

		//matrix which contains the visited pixels/channels
		Mat visited = Mat(carrier.rows,carrier.cols,CV_8UC3);

		//all the pixels/channels are initially not visited
		for(int i=0; i<visited.rows; i++)
			for(int j=0; j<visited.cols; j++)
			{
				visited.at<Vec3b>(i,j)[0]=0;
				visited.at<Vec3b>(i,j)[1]=0;
				visited.at<Vec3b>(i,j)[2]=0;
			}

		//encode size
		for(int i=0; i<32; i++)
		{
			keepGoing = true;

			while(keepGoing)
			{
				//Generates random data
				x = randomGenerator(carrier.rows);
				y = randomGenerator(carrier.cols);
				channel = randomGenerator(3);

				//just uses this location if it was not yet used
				if(visited.at<Vec3b>(x,y)[channel]==0)
				{
					keepGoing=false;
					visited.at<Vec3b>(x,y)[channel]=1;

					//if the bit value is 1, encode it
					if(getBitN(fileSize,i))
					{
						if(carrier.at<Vec3b>(x,y)[channel]<255)
							carrier.at<Vec3b>(x,y)[channel]++;
						//if overflow, decrease value instead of increasing
						else if(carrier.at<Vec3b>(x,y)[channel]==255)
							carrier.at<Vec3b>(x,y)[channel]--;
					}
				}
			}
		}

		//opening input file
		std::ifstream file;
		file.open(argv[2], std::ios::in | std::ios::binary);

		if (!file.is_open())
		{
			std::cerr << "Error! File cannot be opened." << std::endl;
			return -1;
		}

		//to store 8 bits sequences
		char sequence[1];

		unsigned char bit;

		//encode the file itself
		for(int i=0; i<fileSize; i++)
		{
			//reads next byte
			file.read(sequence,1);

			//stores each bit
			for(int j=0; j<8; j++)
			{
				bit = getBitN2(sequence[0],j);
				
				keepGoing = true;

				while(keepGoing)
				{
					//Generates random data
					x = randomGenerator(carrier.rows);
					y = randomGenerator(carrier.cols);
					channel = randomGenerator(3);

					//just uses this location if it was not yet used
					if(visited.at<Vec3b>(x,y)[channel]==0)
					{
						keepGoing=false;
						visited.at<Vec3b>(x,y)[channel]=1;

						//if bit value is one, encode it
						if(bit)
						{
							if(carrier.at<Vec3b>(x,y)[channel]<255)
								carrier.at<Vec3b>(x,y)[channel]++;
							//if overflow, decrease value instead of increasing
							else if(carrier.at<Vec3b>(x,y)[channel]==255)
								carrier.at<Vec3b>(x,y)[channel]--;
						}
					}
				}
			}
		}

		//close file
		file.close();

		std::vector<int> params; //file saving compression parameters
		params.push_back(CV_IMWRITE_PNG_COMPRESSION); //png image
		params.push_back(0); //no compression

		//tries to save the resulting image
		if(!imwrite(argv[3], carrier, params))
		{
			std::cerr << "Error! Unable to save the image." << std::endl;
			return -1;
		}

		// all OK : main returns 0
		return 0;
	}

	std::cerr << "Error! Invalid input." << std::endl;

	// not OK : main returns -1
	return -1;
}