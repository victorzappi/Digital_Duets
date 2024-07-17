/*
 * HyperPixelDrawer.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: vic
 */

#include "HyperPixelDrawer.h"
#include "strnatcmp.h" // for natural sorting


#include <cstring>   // strcmp, memcpy
#include <fstream>   // read/write to file ifstream/ofstream
#include <sstream>   // ostringstream
#include <dirent.h>  // to handle files in dirs
#include <algorithm> // sort


enum operation_ {operation_write, operation_reset};

PixelDrawer_new::PixelDrawer_new(int w, int h, int wrChan, Shader &shader_fbo,
								 int stateNum, int totalQuadNum) : pbo(texture), shaderFbo(shader_fbo) {
	shaders[0] = "";
	shaders[1] = "";

	operation_loc = -1;

	gl_width  = w;
	gl_height = h;

	writeChannels = wrChan;

	// to save a copy of the original image [will come handy for clear()]
	clearFramePixels = new GLfloat[gl_width*gl_height*writeChannels];

	numOfStates     = stateNum;
	numOfResetQuads = totalQuadNum-stateNum;

	// allocate and wipe
	framePixels = new GLfloat[gl_width*gl_height*writeChannels];
	memset(framePixels, 0, sizeof(GLfloat)*gl_width*gl_height*writeChannels);

	drawArraysMainQuads  = new int *[numOfStates];
	for(int i=0; i<numOfStates; i++)
		drawArraysMainQuads[i] = new int[2];
	drawArraysResetQuads = new int *[numOfResetQuads];
	for(int i=0; i<numOfResetQuads; i++)
		drawArraysResetQuads[i] = new int[2];
}

PixelDrawer_new::~PixelDrawer_new() {
	for(int i=0; i<numOfStates; i++)
		delete[] drawArraysMainQuads[i];
	delete[] drawArraysMainQuads;
	for(int i=0; i<numOfResetQuads; i++)
		delete[] drawArraysResetQuads[i];
	delete[] drawArraysResetQuads;

	delete[] framePixels;
	delete[] clearFramePixels;
}


//------------------------------------------------------------------
// protected methods
//------------------------------------------------------------------
bool PixelDrawer_new::initShaders(string shaderLocation) {
	shaders[0] = shaderLocation+"/pixelDrawer_vs.glsl";
	shaders[1] = shaderLocation+"/pixelDrawer_fs.glsl";
	const char* vertex_path_fbo   = {shaders[0].c_str()};
	const char* fragment_path_fbo = {shaders[1].c_str()};

	return shaderDraw.init(vertex_path_fbo, fragment_path_fbo);
}


int PixelDrawer_new::getFrameFileList(string dirname, vector<string> &files, bool ordered) {
	DIR *dir;
	struct dirent *ent;
	int fileCnt = 0;


	// Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
	if ((dir = opendir (dirname.c_str())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			// Ignore dotfiles and . and .. paths
			if(!strncmp(ent->d_name, ".", 1))
				continue;

			//printf("%s\n", ent->d_name);

			// take only .dbx and .txt files
			string name = string(ent->d_name);
			int len		= name.length();

			bool frameFile = false;

			if( (name[len-4]=='.') && (name[len-3]=='f') && (name[len-2]=='r') && (name[len-1]=='m') )
				frameFile = true;

			if(frameFile) {
				fileCnt++;
				printf("%s\n", ent->d_name);
				files.push_back( string( ent->d_name ) );
			}
		}
		closedir (dir);
	} else {
		/* could not open directory */
		printf("Could not open directory %s!\n", dirname.c_str());
		return -1;
	}

	// order by name
	if(ordered)
		sort( files.begin(), files.end(), naturalsort);

	return fileCnt;
}













//----------------------------------------------------------------------------------------------------------------------------------
HyperPixelDrawer::HyperPixelDrawer(int w, int h, int wrChan, Shader &shader_fbo, float ***startFrame,
								   int stateNum, int totalQuadNum, int *cellTypes)
								   : PixelDrawer_new(w, h, wrChan, shader_fbo, stateNum, totalQuadNum) {

	excitationCellType = cellTypes[0];
	airCellType        = cellTypes[1];
	boundaryCellType   = cellTypes[2];

	// save an exact copy of the original image [will come handy for clear()]
	// BUT
	// then we will get rid of everything we don't care about to turn it in just an empty domain...
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels]   = (GLfloat)startFrame[j][i][1]; // area
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels+1] = (GLfloat)startFrame[j][i][2]; // boundary gain [gamma]
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] = (GLfloat)startFrame[j][i][3]; // type
		}
	}
}

HyperPixelDrawer::~HyperPixelDrawer() {
}

int HyperPixelDrawer::init(GLint active_unit, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]) {
	//-----------------------------------------------------------
	// frame
	//-----------------------------------------------------------
	// first put the original domain in our frame data structure
	clear();
	// then we obtain an empty domain [basically we only keep dead cells and PML, cos they are fundamental]
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			// same init as in HyperDrumhead::initFrame()
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels]   = 0; // area = 0
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels+1] = 1; // boundary gain [gamma] = 1 [clamped]
			// but we filter out types we are not interested in
			if(clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] == excitationCellType ||
		       clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] == boundaryCellType)
				clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] = airCellType;
		}
	}


	//-----------------------------------------------------------
	// texture
	//-----------------------------------------------------------
	GLint textureInternalFormat = GL_RGB32F;
	//texture.loadPixels(gl_width, gl_height, GL_R32F, GL_RED, GL_FLOAT, framePixels, active_unit);
	texture.loadPixels(gl_width, gl_height, textureInternalFormat, GL_RGB, GL_FLOAT, framePixels, active_unit);
	// verify type and format
	if(false) {
		printf("\nFormat check for: %d\n", textureInternalFormat);

		GLint format, type;

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);
	}


	//-----------------------------------------------------------
	// shader program
	//-----------------------------------------------------------
	initShaders(shaderLocation);

	shaderDraw.use();

	// these is the drawing textures we will use in the shader to update the main texture [color attachment of FBO]
	glUniform1i(shaderDraw.getUniformLocation("drawingTexture"), active_unit - GL_TEXTURE0);

	operation_loc = shaderDraw.getUniformLocation("operation");
	glUniform1i(operation_loc, operation_write); // most likely case
	if(operation_loc == -1)
		printf("operation uniform can't be found in pixel drawer fragment shader program\n");


	shaderFbo.use();


	//-----------------------------------------------------------
	// pbo
	//-----------------------------------------------------------
	pbo.init(gl_width, gl_height, framePixels, writeChannels);

	// copy vertices indices and strides for the 4 drawing quads and 4 excitation quads and 4 excitation follow up quads and audio quad
	for(int i=0; i<numOfStates; i++) {
		drawArraysMainQuads[i][0] = drawArraysMain[i][0];
		drawArraysMainQuads[i][1] = drawArraysMain[i][1];
	}
	for(int i=0; i<numOfResetQuads; i++) {
		drawArraysResetQuads[i][0] = drawArraysReset[i][0];
		drawArraysResetQuads[i][1] = drawArraysReset[i][1];
	}

	int retval = pbo.trigger(); // test if working

	// apparently this does not happen anymore since we moved from a char PBO to a float PBO...
	if(retval == GL_INVALID_OPERATION) {
		printf("\t***Heads up, we might have stumbled upon an OpenGL bug here!***\n");
		printf("\tThe pbo that is supposed to update the pixels can't be set up.\n");
		printf("\tBut don't panic! Try to slightly increase or decrease domain width [by 1 or 2 pixels].\n");
		printf("\tSorry bout that...it's not totally my fault\n");
	}

	return retval;
}

GLfloat HyperPixelDrawer::getCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	return framePixels[y*writeChannels*gl_width+x*writeChannels + 2];
}

GLfloat HyperPixelDrawer::getCellArea(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	return framePixels[y*writeChannels*gl_width+x*writeChannels];
}

int HyperPixelDrawer::setCellType(int x, int y, GLfloat type) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels + 2;

	// nothing to change
	if(framePixels[index] == type)
		return 1;

	framePixels[index] = type;

	//printf(" = %f\n", type);

	return 0;
}

int HyperPixelDrawer::setCellArea(int x, int y, GLfloat area) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels;

	// nothing to change
	if(framePixels[index] == area)
		return 1;

	framePixels[index] = area;

	//printf(" = %f\n", area);

	return 0;
}

int HyperPixelDrawer::setCell(int x, int y, GLfloat area, GLfloat type, GLfloat bgain) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels;

	framePixels[index]   = area;
	framePixels[index+2] = type;
	if(bgain==-1)
		return 0;
	// in case we also want to change boundary gain...possibly on boundary cells, but whatever
	framePixels[index+1] = bgain;
	return 0;

}

int HyperPixelDrawer::resetCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels + 2;

	// nothing to change
	if(framePixels[index] == clearFramePixels[index])
		return 1;

	framePixels[index] = clearFramePixels[index]; // back to original value

	return 0;
}

void HyperPixelDrawer::update(int state) {
	pbo.trigger();

	shaderDraw.use();

	glUniform1i(operation_loc, operation_write); // drawing

	// don't touch pressure, stored in first channel of each fragment
	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads

	glTextureBarrierNV();

	shaderFbo.use();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // FBO needs full write access
}

void HyperPixelDrawer::reset() {
	shaderDraw.use();

	glUniform1i(operation_loc, operation_write); // back to drawing

	// first put to zero in first channel of fragment [pressure], as hardcoded in the shader
	glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	for(int i=0; i<numOfStates; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[i][0], drawArraysMainQuads[i][1]); // main quads

	glTextureBarrierNV();

	// then put all channels to zero in the audio quad [only other quad in this case]
	glUniform1i(operation_loc, operation_reset);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // need full write access
	for(int i=0; i<numOfResetQuads; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysResetQuads[i][0], drawArraysResetQuads[i][1]); // reset quads

	glTextureBarrierNV();

	shaderFbo.use(); // back to FBO, with full access already (:
}


int HyperPixelDrawer::dumpFrame(hyperDrumheadFrame *frame, string filename) {
	// we combine info passed from outside with the info stored in the pixel drawer
	// we can do it this fast because of friend struct
	frame->width  = gl_width;
	frame->height = gl_height;
	frame->channels = writeChannels;
	frame->pixels = new float[gl_width*gl_height*writeChannels];
	memcpy(frame->pixels, framePixels, gl_width*gl_height*writeChannels*sizeof(float));
	// now the hyper drumhead frame is ready

	return dumper(frame, filename);
}

int HyperPixelDrawer::openFrame(string filename, hyperDrumheadFrame *&frame) {
	// fill it from file
	if(!reader(filename, frame)) // here frame is dynamically allocated
		return 1;
	// use it [take it and use it, like Don Porco]
	if(!loadFrame(frame))
		return 2;
	return 0;
}

bool HyperPixelDrawer::loadFrame(hyperDrumheadFrame *frame) {
	// check if we are opening a frame that has the same size as the current one!
	if( (frame->width != gl_width) || (frame->height != gl_height) ) {
		printf("Heck! Trying to load a frame with size different from current simulation domain!\n");
		return false;
	}

	memcpy(framePixels, frame->pixels, frame->width*frame->height*frame->channels*sizeof(float)); // copy the new frame image

	return true;
}

//------------------------------------------------------------------
// private methods
//------------------------------------------------------------------
int HyperPixelDrawer::dumper(hyperDrumheadFrame *frame, string filename) {
	// if no name is provided, let's count num of frm files in current dir
	if(filename=="") {
		vector<string> files;
		int fileNum = getFrameFileList(".", files);

		if(fileNum<0) {
			printf("Can't open directory \".\"\n");
			return 1;
		}
		ostringstream convert;
		convert << fileNum;
		filename += convert.str() + ".frm";
	}

	ofstream dumpfile;
	dumpfile.open (filename.c_str(), ios::out | ios::binary);

	// dump
	if (dumpfile.is_open()) {
		// second parameter is equivalent to number of char in the memory block [i knooow it's a bit redundant!]
		// had to split into different write commands cos pixels is a dynamically allocated array!

		// start from hyper drumhead specific stuff
		// we can access struct's fields this fast because of friend struct
		dumpfile.write((char *) &frame->numOfAreas, sizeof(int)/sizeof(char));
		for(int i=0; i<frame->numOfAreas; i++) {
			dumpfile.write((char *) &frame->damp[i], sizeof(float)/sizeof(char));
			dumpfile.write((char *) &frame->prop[i], sizeof(float)/sizeof(char));
			dumpfile.write((char *) &frame->exVol[i], sizeof(double)/sizeof(char));
			dumpfile.write((char *) &frame->exFiltFreq[i], sizeof(double)/sizeof(char));
			dumpfile.write((char *) &frame->exFreq[i], sizeof(double)/sizeof(char));
			dumpfile.write((char *) &frame->exType[i], sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frame->listPos[0][i], sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frame->listPos[1][i], sizeof(int)/sizeof(char));

			//printf("listener %d: %d, %d\n", i, frame->listPos[0][i], frame->listPos[1][i]);

			dumpfile.write((char *) &frame->firstFingerMotion[i], sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frame->firstFingerMotionNeg[i], sizeof(bool)/sizeof(char));
			dumpfile.write((char *) &frame->firstFingerPress[i], sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frame->thirdFingerMotion[i], sizeof(int)/sizeof(char));
			dumpfile.write((char *) &frame->thirdFingerMotionNeg[i], sizeof(bool)/sizeof(char));
			dumpfile.write((char *) &frame->thirdFingerPress[i], sizeof(int)/sizeof(char));

			for(int j=0; j<MAX_PRESS_MODES; j++)
				dumpfile.write((char *) &frame->pressRange[i][j], sizeof(float)/sizeof(char));
		}
		for(int j=0; j<MAX_PRESS_MODES; j++)
			dumpfile.write((char *) &frame->pressDelta[j], sizeof(float)/sizeof(char));


		// then generic frame info
		dumpfile.write((char *) &frame->width, sizeof(int)/sizeof(char));
		dumpfile.write((char *) &frame->height, sizeof(int)/sizeof(char));
		dumpfile.write((char *) &frame->channels, sizeof(int)/sizeof(char));
		dumpfile.write((char *) frame->pixels, frame->width*frame->height*frame->channels*sizeof(float)/sizeof(char));

		dumpfile.close();
	}
	else {
		printf("Can't create new file \"%s\"\n", filename.c_str());
		return 2;
	}

	return 0;
}


// frame is dynamically allocated in this function but used somewhere else
bool HyperPixelDrawer::reader(string filename, hyperDrumheadFrame *&frame) {
	ifstream readfile;
	readfile.open (filename.c_str(),  ios::in | ios::binary | ios::ate);


	if(readfile.is_open()) {
		readfile.seekg (0, ios::beg);

		int areas=-1;
		readfile.read((char *) &areas, sizeof(int)/sizeof(char));

		frame = new hyperDrumheadFrame(areas);

		for(int i=0; i<frame->numOfAreas; i++) {
			readfile.read((char *) &frame->damp[i], sizeof(float)/sizeof(char));
			readfile.read((char *) &frame->prop[i], sizeof(float)/sizeof(char));
			readfile.read((char *) &frame->exVol[i], sizeof(double)/sizeof(char));
			readfile.read((char *) &frame->exFiltFreq[i], sizeof(double)/sizeof(char));
			readfile.read((char *) &frame->exFreq[i], sizeof(double)/sizeof(char));
			readfile.read((char *) &frame->exType[i], sizeof(int)/sizeof(char));
			readfile.read((char *) &frame->listPos[0][i], sizeof(int)/sizeof(char));
			readfile.read((char *) &frame->listPos[1][i], sizeof(int)/sizeof(char));

			//printf("listener %d: %d, %d\n", i, frame->listPos[0][i], frame->listPos[1][i]);

			readfile.read((char *) &frame->firstFingerMotion[i], sizeof(int)/sizeof(char));
			readfile.read((char *) &frame->firstFingerMotionNeg[i], sizeof(bool)/sizeof(char));
			readfile.read((char *) &frame->firstFingerPress[i], sizeof(int)/sizeof(char));
			readfile.read((char *) &frame->thirdFingerMotion[i], sizeof(int)/sizeof(char));
			readfile.read((char *) &frame->thirdFingerMotionNeg[i], sizeof(bool)/sizeof(char));
			readfile.read((char *) &frame->thirdFingerPress[i], sizeof(int)/sizeof(char));

			for(int j=0; j<MAX_PRESS_MODES; j++)
				readfile.read((char *) &frame->pressRange[i][j], sizeof(float)/sizeof(char));
		}
		for(int j=0; j<MAX_PRESS_MODES; j++)
			readfile.read((char *) &frame->pressDelta[j], sizeof(float)/sizeof(char));

		readfile.read((char *) &frame->width, sizeof(int)/sizeof(char));
		readfile.read((char *) &frame->height, sizeof(int)/sizeof(char));
		readfile.read((char *) &frame->channels, sizeof(int)/sizeof(char));

		frame->pixels = new float[frame->width*frame->height*frame->channels];
		readfile.read((char *) frame->pixels, frame->width*frame->height*frame->channels*sizeof(float)/sizeof(char));

		readfile.close();
		return true;
	}
	else {
		printf("Can't open file \"%s\"\n", filename.c_str());
		return false;
	}
}


/*
enum operation_ {operation_write, operation_resetPressure, operation_resetRest};

HyperPixelDrawer::HyperPixelDrawer(int w, int h, Shader &shader_fbo, float ***startFrame, int stateNum, int otherQuadNum,
		 	 	 	 	 	 	   int exCType, int airCType, int wallCType) : pbo(texture), shaderFbo(shader_fbo) {
	shaders[0] = "";
	shaders[1] = "";

	operation_loc = -1;

	writeChannels = 1; // we write 3 channels: [area, boudnGain, type]

	gl_width  = w;
	gl_height = h;

	numOfStates     = stateNum;
	numOfResetQuads = otherQuadNum+1; // +1 is audio quad

	drawArraysMainQuads = new int *[numOfStates];
	for(int i=0; i<numOfStates; i++)
		drawArraysMainQuads[i] = new int[2];
	drawArraysResetQuads = new int *[numOfResetQuads];
	for(int i=0; i<numOfResetQuads; i++)
		drawArraysResetQuads[i] = new int[2];

	drawArraysValues = new int *[numOfStates + numOfResetQuads]; // drawing quads [one per each state] + other quads [which include audio quad]
	for(int i=0; i<3*numOfStates + 1; i++)
		drawArraysValues[i] = new int[2];

	// allocate and wipe
	framePixels = new GLfloat[gl_width*gl_height*writeChannels];
	memset(framePixels, 0, sizeof(GLfloat)*gl_width*gl_height*writeChannels);


	excitationCellType = exCType;
	airCellType        = airCType;
	boundaryCellType       = wallCType;

	// create and save a copy of the original image [will come handy for clear()]
	// it still contains possible initial excitation and walls, so that the first clearTexture() call saves these data into the texture
	clearFramePixels = new GLfloat[gl_width*gl_height];
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++)
			clearFramePixels[i*gl_width+j] = (GLfloat)startFrame[j][i][3]; // all air + PML + dead layers
	}
}

HyperPixelDrawer::~HyperPixelDrawer() {
	for(int i=0; i<numOfStates; i++)
		delete[] drawArraysMainQuads[i];
	delete[] drawArraysMainQuads;
	for(int i=0; i<numOfResetQuads; i++)
		delete[] drawArraysResetQuads[i];
	delete[] drawArraysResetQuads;
	for(int i=0; i<3*numOfStates + 1; i++)
		delete[] drawArraysValues[i];
	delete[] drawArraysValues;

	delete[] framePixels;
	delete[] clearFramePixels;
}

int HyperPixelDrawer::init(GLint active_unit, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]) {
	clear();
	GLint textureInternalFormat = GL_R32F;
	//texture.loadPixels(gl_width, gl_height, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, framePixels, active_unit);
	texture.loadPixels(gl_width, gl_height, textureInternalFormat, GL_RED, GL_FLOAT, framePixels, active_unit);

	// verify type and format
	if(false) {
		printf("\nFormat check for: %d\n", textureInternalFormat);

		GLint format, type;

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);
	}

	initShaders(shaderLocation);

	shaderDraw.use();

	// these is the drawing textures we will use in the shader to update the main texture [color attachment of FBO]
	glUniform1i(shaderDraw.getUniformLocation("drawingTexture"), active_unit - GL_TEXTURE0);

	operation_loc = shaderDraw.getUniformLocation("operation");
	glUniform1i(operation_loc, operation_write); // most likely case
	if(operation_loc == -1)
		printf("operation uniform can't be found in pixel drawer fragment shader program\n");

	shaderFbo.use();

	glUniform1i(operation_loc, operation_write); // drawing
	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE); // the 3 channels we write

	pbo.init(gl_width, gl_height, framePixels);

	// copy vertices indices and strides for the 4 drawing quads and 4 excitation quads and 4 excitation follow up quads and audio quad
	for(int i=0; i<numOfStates; i++) {
		drawArraysMainQuads[i][0] = drawArraysMain[i][0];
		drawArraysMainQuads[i][1] = drawArraysMain[i][1];
	}
	for(int i=0; i<numOfResetQuads; i++) {
		drawArraysResetQuads[i][0] = drawArraysReset[i][0];
		drawArraysResetQuads[i][1] = drawArraysReset[i][1];
	}

	int retval = pbo.trigger(); // test if working

	// apparently this does not happen anymore since we moved from a char PBO to a float PBO...
	if(retval == GL_INVALID_OPERATION) {
		printf("\t***Heads up, we might have stumbled upon an OpenGL bug here!***\n");
		printf("\tThe pbo that is supposed to update the pixels can't be set up.\n");
		printf("\tBut don't panic! Try to slightly increase or decrease domain width [by 1 or 2 pixels].\n");
		printf("\tSorry bout that...it's not totally my fault\n");
	}

	// transform the init image into a clear image!
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			// turn into air any excitation and wall cells...cos we do not want them in the original clear frame [they make noise q:]
			if(clearFramePixels[i*gl_width+j] == excitationCellType || clearFramePixels[i*gl_width+j] == boundaryCellType)
				clearFramePixels[i*gl_width+j] = airCellType;
		}
	}
	return retval;
}

void HyperPixelDrawer::update(int state) {
	pbo.trigger();

	shaderDraw.use();

	glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads
	glTextureBarrierNV();

	shaderFbo.use();
}

void HyperPixelDrawer::reset() {
	shaderDraw.use();

	// first put to zero the r channel in the main quads [pressure]
	glUniform1i(operation_loc, operation_resetPressure);
	for(int i=0; i<numOfStates; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[i][0], drawArraysMainQuads[i][1]); // main quads
	glTextureBarrierNV();

	// then put all channels to zero in the other quads [excitation, audio]
	glUniform1i(operation_loc, operation_resetRest); // reset audio quad
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	for(int i=0; i<numOfResetQuads; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysResetQuads[i][0], drawArraysResetQuads[i][1]); // reset quads
	glTextureBarrierNV();

	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE); // back to the original layout
	glUniform1i(operation_loc, operation_write); // back to drawing

	shaderFbo.use();
}

int HyperPixelDrawer::setCell(GLfloat area, GLfloat boundGain, GLfloat type, int x, int y) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	framePixels[y*gl_width*writeChannels+x*writeChannels]   = area;
	framePixels[y*gl_width*writeChannels+x*writeChannels+1] = boundGain;
	framePixels[y*gl_width*writeChannels+x*writeChannels+2] = type;

	//printf(" = %f\n", type);

	return 0;
}

int HyperPixelDrawer::resetCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*gl_width*writeChannels+x*writeChannels+2;

	// nothing to change
	if(framePixels[index] == clearFramePixels[y*gl_width+x])
		return 1;

	framePixels[index] = clearFramePixels[index]; // back to original value

	return 0;
}

//------------------------------------------------------------------
// private methods
//------------------------------------------------------------------
bool HyperPixelDrawer::initShaders(string shaderLocation) {
	shaders[0] = shaderLocation+"/pixelDrawer_vs.glsl";
	shaders[1] = shaderLocation+"/pixelDrawer_fs.glsl";
	const char* vertex_path_fbo      = {shaders[0].c_str()};
	const char* fragment_path_fbo    = {shaders[1].c_str()};

	return shaderDraw.init(vertex_path_fbo, fragment_path_fbo);
}*/
