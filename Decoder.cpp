// Example : decodes a hidden message from a colour picture, using a random algorithm
// usage: Decoder <encoded image> <base image> <decoded image>

// Author : João Faria, joao.faria@fe.up.pt

// Copyright (c) 2011 School of Engineering, Cranfield University
// License : LGPL - http://www.gnu.org/licenses/lgpl.html

#include <cv.h>   	// open cv general include file
#include <highgui.h>	// open cv GUI include file
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

//Set the n-th bit of a sequence to 1
//Based on getBitN function from URL: http://public.cranfield.ac.uk/c5354/teaching/dip/practicals/ip/bit.c
unsigned char setBitN(unsigned char val, int n) {
   
     unsigned char b;
     b = val | (1 << n);
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
	Mat encoded;  // input encoded image object
	Mat base;  // input base image object
	Mat visited;  // input base image object

	// check that command line arguments are provided and images read in OK
	if ((argc == 4) && !(encoded = imread( argv[1], CV_LOAD_IMAGE_COLOR)).empty()
		&& !(base = imread( argv[2], CV_LOAD_IMAGE_COLOR)).empty())
	{
		//error when images are not of the same size
		if(encoded.size!=base.size)
		{
			std::cerr << "Error! Images must have the same size." << std::endl;
			return -1;
		}

		//image is too small
		if(encoded.cols*encoded.rows-32<0)
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
		noiseGenerator(&base,3,randomGenerator);

		int x;
		int y;
		int channel;
		bool keepGoing;

		//matrix which contains the visited pixels/channels
		visited = Mat(encoded.rows,encoded.cols,CV_8UC3);

		//all the pixels/channels are initially not visited
		for(int i=0; i<visited.rows; i++)
			for(int j=0; j<visited.cols; j++)
			{
				visited.at<Vec3b>(i,j)[0]=0;
				visited.at<Vec3b>(i,j)[1]=0;
				visited.at<Vec3b>(i,j)[2]=0;
			}

		//opening input file
		remove(argv[3]);
		std::ofstream file;
		file.open(argv[3], std::ios::out | std::ios::binary | std::ios::app);

		unsigned int fileSize = 0;
		int tmpSize = 0;
		int n = 0;

		//gets file size from first 32 encoded bits
		for(int i=0; i<32; i++)
		{
			keepGoing = true;

			while(keepGoing)
			{
				//Generates random data
				x = randomGenerator(encoded.rows);
				y = randomGenerator(encoded.cols);
				channel = randomGenerator(3);

				//just uses this location if it was not yet used
				if(visited.at<Vec3b>(x,y)[channel]==0)
				{
					//is visited now
					visited.at<Vec3b>(x,y)[channel]=1;

					//presence of a 1
					if(encoded.at<Vec3b>(x,y)[channel]!=base.at<Vec3b>(x,y)[channel])
						tmpSize = setBitN(tmpSize,n);

					n++;

					//if we have a complete byte it is treated
					if((i+1)%8==0)
					{						
						fileSize += ((int)tmpSize)*pow(2.0,(i-7));
						n=0;
						tmpSize=0;
					}

					keepGoing = false;
				}
			}
		}

		char sequence[] = {0};
		n = 0;

		//retrieves file data
		for(int i=0; i<fileSize; i++)
		{
			//for each bit
			for(int j=0; j<8; j++)
			{
				keepGoing = true;

				while(keepGoing)
				{
					//Generates random data
					x = randomGenerator(encoded.rows);
					y = randomGenerator(encoded.cols);
					channel = randomGenerator(3);

					//just uses this location if it was not yet used
					if(visited.at<Vec3b>(x,y)[channel]==0)
					{
						keepGoing=false;
						//is visited now
						visited.at<Vec3b>(x,y)[channel]=1;

						//if pixels are different, we are in presence of a 1
						if(encoded.at<Vec3b>(x,y)[channel]!=base.at<Vec3b>(x,y)[channel])
							sequence[0] = setBitN(sequence[0],n);

						n++;

						//if we have a complete byte, it is treated
						if(n%8==0)
						{
							//writing to the file
							file.write(sequence,1);
							n=0;
							sequence[0]=0;
						}
					}
				}
			}
		}

		//close file
		file.close();

		// all OK : main returns 0
		return 0;
	}

	std::cerr << "Error! Invalid input." << std::endl;

	// not OK : main returns -1
	return -1;
}