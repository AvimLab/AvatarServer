/*
*  ofxNCoreVision.cpp
*  NUI Group Community Core Vision
*
*  Created by NUI Group Dev Team A on 2/1/09.
*  Copyright 2009 NUI Group. All rights reserved.
*
*/

#include "ofxNCoreVision.h"
#include "../Controls/gui.h"

/******************************************************************************
* The setup function is run once to perform initializations in the application
*****************************************************************************/
void ofxNCoreVision::_setup(ofEventArgs &e)
{	
	//set the title
	ofSetWindowTitle(" Avatar Server ");
	
	//create filter
	if(filter == NULL)	filter = new ProcessFilters();
	
	//Load Settings from config.xml file
	loadXMLSettings();

	cameraInited = false;

	//Setup Window Properties
	ofSetWindowShape(winWidth,winHeight);
	ofSetVerticalSync(false);	            //Set vertical sync to false for better performance?
	
	//printf("Application Loaded...\n?");

	//load camera/video
	initDevice();
	//set framerate
	ofSetFrameRate(camRate * 1.3);			//This will be based on camera fps in the future

	/*****************************************************************************************************
	* Allocate images (needed for drawing/processing images)
	******************************************************************************************************/
	processedImg.allocate(camWidth, camHeight); //main Image that'll be processed.
	
	processedImg.setUseTexture(false);			//We don't need to draw this so don't create a texture
	sourceImg.allocate(camWidth, camHeight);    //Source Image
	sourceImg.setUseTexture(false);				//We don't need to draw this so don't create a texture
	/******************************************************************************************************/
	//printf("Cameras Loaded...\n");

	//Fonts - Is there a way to dynamically change font size?
	verdana.loadFont("verdana.ttf", 8, true, true);	   //Font used for small images
	bigvideo.loadFont("verdana.ttf", 13, true, true);  //Font used for big images.

	//Static Images
	background.loadImage("images/background.jpg"); //Main (Temp?) Background
	//printf("freedom3?");
	
	//GUI Controls
	controls = ofxGui::Instance(this);
	setupControls();
	
	//printf("Controls Loaded...\n");

	//Setup Calibration
	calib.setup(camWidth, camHeight, &tracker);

	//Allocate Filters
	filter->allocate( camWidth, camHeight );

	/*****************************************************************************************************
	* Startup Modes
	******************************************************************************************************/
	//If Standalone Mode (not an addon)
	if (bStandaloneMode)
	{
		printf("Starting in standalone mode...\n\n");
		showConfiguration = true;
	}
	if (bMiniMode)
	{
		showConfiguration = true;
		bShowInterface = false;
		printf("Starting in Mini Mode...\n\n");
		ofSetWindowShape(190, 200); //minimized size
		filter->bMiniMode = bMiniMode;
	}
	else{
		bShowInterface = true;
		printf("Starting in full mode...\n\n");
	}

	//If Object tracking activated
	if(contourFinder.bTrackObjects)
	{
		templates.loadTemplateXml();
	}
	
	contourFinder.setTemplateUtils(&templates);
	#ifdef TARGET_WIN32
	    //get rid of the console window
        FreeConsole();
	#endif

	printf("Community Core Vision is setup!\n\n");
}

/****************************************************************
*	Load/Save config.xml file Settings
****************************************************************/
void ofxNCoreVision::loadXMLSettings()
{
	// TODO: a seperate XML to map keyboard commands to action
	message = "Loading config.xml...";
	// Can this load via http?
	if ( XML.loadFile("config.xml"))
		message = "Settings Loaded!\n\n";
	else
		message = "No Settings Found...\n\n"; //FAIL
	
	//--------------------------------------------------------------
	//  START BINDING XML TO VARS
	//--------------------------------------------------------------
	winWidth					= XML.getValue("CONFIG:WINDOW:WIDTH", 650);
	winHeight					= XML.getValue("CONFIG:WINDOW:HEIGHT", 600);
	bcamera						= XML.getValue("CONFIG:CAMERA_0:USECAMERA", 1);
	deviceID					= XML.getValue("CONFIG:CAMERA_0:DEVICE", 0);
	camWidth					= XML.getValue("CONFIG:CAMERA_0:WIDTH", 320);
	camHeight					= XML.getValue("CONFIG:CAMERA_0:HEIGHT", 240);
	camRate						= XML.getValue("CONFIG:CAMERA_0:FRAMERATE", 0);
	videoFileName				= XML.getValue("CONFIG:VIDEO:FILENAME", "test_videos/RearDI.m4v");
	maxBlobs					= XML.getValue("CONFIG:BLOBS:MAXNUMBER", 20);
	bShowLabels					= XML.getValue("CONFIG:BOOLEAN:LABELS",0);
	bDrawOutlines				= XML.getValue("CONFIG:BOOLEAN:OUTLINES",0);
	filter->bLearnBakground		= XML.getValue("CONFIG:BOOLEAN:LEARNBG",0);
	filter->bVerticalMirror		= XML.getValue("CONFIG:BOOLEAN:VMIRROR",0);
	filter->bHorizontalMirror	= XML.getValue("CONFIG:BOOLEAN:HMIRROR",0);
	
	//Filters
	filter->bTrackDark			= XML.getValue("CONFIG:BOOLEAN:TRACKDARK", 0);
	filter->bHighpass			= XML.getValue("CONFIG:BOOLEAN:HIGHPASS",1);
	filter->bAmplify			= XML.getValue("CONFIG:BOOLEAN:AMPLIFY", 1);
	filter->bSmooth				= XML.getValue("CONFIG:BOOLEAN:SMOOTH", 1);
	filter->bDynamicBG			= XML.getValue("CONFIG:BOOLEAN:DYNAMICBG", 1);
	//MODES
	bGPUMode					= XML.getValue("CONFIG:BOOLEAN:GPU", 0);
	bMiniMode                   = XML.getValue("CONFIG:BOOLEAN:MINIMODE",0);
	//CONTROLS
	tracker.MOVEMENT_FILTERING	= XML.getValue("CONFIG:INT:MINMOVEMENT",0);
	MIN_BLOB_SIZE				= XML.getValue("CONFIG:INT:MINBLOBSIZE",2);
	MAX_BLOB_SIZE				= XML.getValue("CONFIG:INT:MAXBLOBSIZE",100);
	backgroundLearnRate			= XML.getValue("CONFIG:INT:BGLEARNRATE", 0.01f);
	//Filter Settings
	filter->threshold			= XML.getValue("CONFIG:INT:THRESHOLD",0);
	filter->highpassBlur		= XML.getValue("CONFIG:INT:HIGHPASSBLUR",0);
	filter->highpassNoise		= XML.getValue("CONFIG:INT:HIGHPASSNOISE",0);
	filter->highpassAmp			= XML.getValue("CONFIG:INT:HIGHPASSAMP",0);
	filter->smooth				= XML.getValue("CONFIG:INT:SMOOTH",0);
	minTempArea					= XML.getValue("CONFIG:INT:MINTEMPAREA",0);
	maxTempArea					= XML.getValue("CONFIG:INT:MAXTEMPAREA",0);
	//Tracking Options
	contourFinder.bTrackFingers				= XML.getValue("CONFIG:BOOLEAN:TRACKFINGERS",0);
	contourFinder.bTrackObjects				= XML.getValue("CONFIG:BOOLEAN:TRACKOBJECTS",0);
	contourFinder.bTrackFiducials				= XML.getValue("CONFIG:BOOLEAN:TRACKFIDUCIALS",0);

	//NETWORK SETTINGS
	bTUIOMode					= XML.getValue("CONFIG:BOOLEAN:TUIO",0);
	myTUIO.bOSCMode				= XML.getValue("CONFIG:BOOLEAN:OSCMODE",1);
	myTUIO.bTCPMode				= XML.getValue("CONFIG:BOOLEAN:TCPMODE",1);
	myTUIO.bBinaryMode			= XML.getValue("CONFIG:BOOLEAN:BINMODE",1);
	myTUIO.bHeightWidth			= XML.getValue("CONFIG:BOOLEAN:HEIGHTWIDTH",0);
	tmpLocalHost				= XML.getValue("CONFIG:NETWORK:LOCALHOST", "localhost");
	tmpPort						= XML.getValue("CONFIG:NETWORK:TUIOPORT_OUT", 3333); 
	tmpFlashPort				= XML.getValue("CONFIG:NETWORK:TUIOFLASHPORT_OUT", 3000);
	myTUIO.setup(tmpLocalHost.c_str(), tmpPort, tmpFlashPort); //have to convert tmpLocalHost to a const char*	
	//--------------------------------------------------------------
	//  END XML SETUP
}

void ofxNCoreVision::saveSettings()
{
	XML.setValue("CONFIG:CAMERA_0:USECAMERA", bcamera);
	XML.setValue("CONFIG:CAMERA_0:DEVICE", deviceID);
	XML.setValue("CONFIG:CAMERA_0:WIDTH", camWidth);
	XML.setValue("CONFIG:CAMERA_0:HEIGHT", camHeight);
	XML.setValue("CONFIG:CAMERA_0:FRAMERATE", camRate);
	XML.setValue("CONFIG:BOOLEAN:PRESSURE",bShowPressure);
	XML.setValue("CONFIG:BOOLEAN:LABELS",bShowLabels);
	XML.setValue("CONFIG:BOOLEAN:OUTLINES",bDrawOutlines);
	XML.setValue("CONFIG:BOOLEAN:LEARNBG", filter->bLearnBakground);
	XML.setValue("CONFIG:BOOLEAN:VMIRROR", filter->bVerticalMirror);
	XML.setValue("CONFIG:BOOLEAN:HMIRROR", filter->bHorizontalMirror);
	XML.setValue("CONFIG:BOOLEAN:TRACKDARK", filter->bTrackDark);
	XML.setValue("CONFIG:BOOLEAN:HIGHPASS", filter->bHighpass);
	XML.setValue("CONFIG:BOOLEAN:AMPLIFY", filter->bAmplify);
	XML.setValue("CONFIG:BOOLEAN:SMOOTH", filter->bSmooth);
	XML.setValue("CONFIG:BOOLEAN:DYNAMICBG", filter->bDynamicBG);
	XML.setValue("CONFIG:BOOLEAN:GPU", bGPUMode);
	XML.setValue("CONFIG:INT:MINMOVEMENT", tracker.MOVEMENT_FILTERING);
	XML.setValue("CONFIG:INT:MINBLOBSIZE", MIN_BLOB_SIZE);
	XML.setValue("CONFIG:INT:MAXBLOBSIZE", MAX_BLOB_SIZE);
	XML.setValue("CONFIG:INT:BGLEARNRATE", backgroundLearnRate);
	XML.setValue("CONFIG:INT:THRESHOLD", filter->threshold);
	XML.setValue("CONFIG:INT:HIGHPASSBLUR", filter->highpassBlur);
	XML.setValue("CONFIG:INT:HIGHPASSNOISE", filter->highpassNoise);
	XML.setValue("CONFIG:INT:HIGHPASSAMP", filter->highpassAmp);
	XML.setValue("CONFIG:INT:SMOOTH", filter->smooth);
	XML.setValue("CONFIG:INT:MINTEMPAREA", minTempArea);
	XML.setValue("CONFIG:INT:MAXTEMPAREA", maxTempArea);
	XML.setValue("CONFIG:BOOLEAN:MINIMODE", bMiniMode);
	XML.setValue("CONFIG:BOOLEAN:TUIO",bTUIOMode);
	XML.setValue("CONFIG:BOOLEAN:TRACKFINGERS",contourFinder.bTrackFingers);
	XML.setValue("CONFIG:BOOLEAN:TRACKOBJECTS",contourFinder.bTrackObjects);
	XML.setValue("CONFIG:BOOLEAN:TRACKFIDUCIALS",contourFinder.bTrackFiducials);
	XML.setValue("CONFIG:BOOLEAN:HEIGHTWIDTH", myTUIO.bHeightWidth);
	XML.setValue("CONFIG:BOOLEAN:OSCMODE", myTUIO.bOSCMode);
	XML.setValue("CONFIG:BOOLEAN:TCPMODE", myTUIO.bTCPMode);
	XML.setValue("CONFIG:BOOLEAN:BINMODE", myTUIO.bBinaryMode);
//	XML.setValue("CONFIG:NETWORK:LOCALHOST", myTUIO.localHost);
//	XML.setValue("CONFIG:NETWORK:TUIO_PORT_OUT",myTUIO.TUIOPort);
	XML.saveFile("config.xml");
}

/************************************************
*				Init Device
************************************************/
//Init Device (camera/video)
void ofxNCoreVision::initDevice()
{
	//Pick the Source - camera or video
	if (bcamera)
	{
		cameraInited=true;
		//check if a firefly, ps3 camera, or other is plugged in
		#ifdef TARGET_WIN32
			/****PS3 - PS3 camera only****/
		    if(ofxPS3::getDeviceCount() > 0 && PS3 == NULL)
			{
				PS3 = new ofxPS3();
				PS3->listDevices();
				PS3->initPS3(camWidth, camHeight, camRate);
				camWidth = PS3->getCamWidth();
			    camHeight = PS3->getCamHeight();
				printf("PS3 Camera Mode\nAsked for %i by %i - actual size is %i by %i \n\n", camWidth, camHeight, PS3->getCamWidth(), PS3->getCamHeight());
				return;
			}
			/****ffmv - firefly camera only****/
			else if(ofxffmv::getDeviceCount() > 0 && ffmv == NULL)
			{
			   ffmv = new ofxffmv();
			   ffmv->listDevices();
			   ffmv->initFFMV(camWidth,camHeight);
			   printf("FFMV Camera Mode\nAsked for %i by %i - actual size is %i by %i \n\n", camWidth, camHeight, ffmv->getCamWidth(), ffmv->getCamHeight());
			   camWidth = ffmv->getCamWidth();
			   camHeight = ffmv->getCamHeight();
			   return;
			}
			else if( vidGrabber == NULL ) 
			{
				vidGrabber = new ofVideoGrabber();
				vidGrabber->listDevices();
				vidGrabber->setVerbose(true);
				vidGrabber->initGrabber(camWidth,camHeight);
				printf("Video Grabber Camera Mode\nAsked for %i by %i - actual size is %i by %i \n\n", camWidth, camHeight, vidGrabber->width, vidGrabber->height);
				camWidth = vidGrabber->width;
				camHeight = vidGrabber->height;
				return;
			}
			else if( dsvl == NULL) 
			{
				dsvl = new ofxDSVL();
				dsvl->initDSVL();
				printf("DSVL Camera Mode\nAsked for %i by %i - actual size is %i by %i \n\n", camWidth, camHeight, dsvl->getCamWidth(), dsvl->getCamHeight());
				camWidth = dsvl->getCamWidth();
				camHeight = dsvl->getCamHeight();
				return;
			}
		#else 
			if( vidGrabber == NULL ) 
			{
				vidGrabber = new ofVideoGrabber();
				vidGrabber->listDevices();
				vidGrabber->setVerbose(true);
				vidGrabber->initGrabber(camWidth,camHeight);
				printf("Linux Camera Mode\nAsked for %i by %i - actual size is %i by %i \n\n", camWidth, camHeight, vidGrabber->width, vidGrabber->height);
				camWidth = vidGrabber->width;
				camHeight = vidGrabber->height;
				return;
			}
		#endif
	}
	else
	{
		if( vidPlayer == NULL ) 
		{
            vidPlayer = new ofVideoPlayer();
            vidPlayer->loadMovie( videoFileName );
            vidPlayer->play();
            vidPlayer->setLoopState(OF_LOOP_NORMAL);
			printf("Video Mode\n\n");
			camHeight = vidPlayer->height;
			camWidth = vidPlayer->width;
			return;
        }
	}
}

/******************************************************************************
* The update function runs continuously. Use it to update states and variables
*****************************************************************************/
void ofxNCoreVision::_update(ofEventArgs &e)
{
	bNewFrame = false;

	if(bcamera) //if camera
	{
		#ifdef TARGET_WIN32
			if(PS3!=NULL)//ps3 camera
			{
				bNewFrame = PS3->isFrameNew();
			}
			else if(ffmv!=NULL)
			{
				ffmv->grabFrame();
				bNewFrame = true;
			}
			else if(vidGrabber !=NULL)
			{
				vidGrabber->grabFrame();
				bNewFrame = vidGrabber->isFrameNew();
			}
			else if(dsvl !=NULL)
			{
				bNewFrame = dsvl->isFrameNew();
			}
		#else
			vidGrabber->grabFrame();
			bNewFrame = vidGrabber->isFrameNew();
		#endif
	}
	else //if video
	{
		vidPlayer->idleMovie();
		bNewFrame = vidPlayer->isFrameNew();
	}

	//if no new frame, return
	if(!bNewFrame)
	{
		return;
	}
	else//else process camera frame
	{
		ofBackground(0, 0, 0);

		// Calculate FPS of Camera
		frames++;
		float time = ofGetElapsedTimeMillis();
		if (time > (lastFPSlog + 1000))
		{
			fps = frames;
			frames = 0;
			lastFPSlog = time;
		}//End calculation

		float beforeTime = ofGetElapsedTimeMillis();

		if (bGPUMode)
		{
			grabFrameToGPU(filter->gpuSourceTex);
			filter->applyGPUFilters();
			contourFinder.findContours(filter->gpuReadBackImageGS,  (MIN_BLOB_SIZE * 2) + 1, ((camWidth * camHeight) * .4) * (MAX_BLOB_SIZE * .001), maxBlobs, false);
		}
		else
		{
			grabFrameToCPU();
			filter->applyCPUFilters( processedImg );
			contourFinder.findContours(processedImg,  (MIN_BLOB_SIZE * 2) + 1, ((camWidth * camHeight) * .4) * (MAX_BLOB_SIZE * .001), maxBlobs, false);
		}

		tracker.track(&contourFinder);

		//get DSP time
		differenceTime = ofGetElapsedTimeMillis() - beforeTime;

		//Dynamic Background subtraction LearRate
		if (filter->bDynamicBG)
		{
			filter->fLearnRate = backgroundLearnRate * .0001; //If there are no blobs, add the background faster.
			if (contourFinder.nBlobs > 0) //If there ARE blobs, add the background slower.
			{
				filter->fLearnRate = backgroundLearnRate * .0001;
			}
		}//End Background Learning rate

		if (bTUIOMode)
		{
			//Start sending OSC
			if(contourFinder.bTrackFingers)
			{
				myTUIO.sendTUIO(&getBlobs());
			}

			/*
			if(contourFinder.bTrackObjects)
			{
				myTUIO.sendTUIO(&getObjects());
			}
			*/
		}
	}
}


/************************************************
*				Input Device Stuff
************************************************/
//get pixels from camera
void ofxNCoreVision::getPixels()
{
#ifdef TARGET_WIN32
	if(PS3!=NULL)
	{
		//already grayscale
		processedImg.setFromPixels(PS3->getPixels(), camWidth, camHeight);
	}
	else if(ffmv != NULL)
	{
		processedImg.setFromPixels(ffmv->fcImage[ffmv->getDeviceID()].pData, camWidth, camHeight);
	}
	else if(vidGrabber != NULL ) 
	{
		sourceImg.setFromPixels(vidGrabber->getPixels(), camWidth, camHeight);
		//convert to grayscale
		processedImg = sourceImg;
	}
	else if(dsvl!=NULL)
	{
		if(dsvl->getNumByes() != 1){ //if not grayscale
			sourceImg.setFromPixels(dsvl->getPixels(), camWidth, camHeight);
			//convert to grayscale
			processedImg = sourceImg;
		}
		else
		{	//if grayscale
			processedImg.setFromPixels(dsvl->getPixels(), camWidth, camHeight);
		}
	}
#endif	
}


//Grab frame from CPU
void ofxNCoreVision::grabFrameToCPU()
{
	//Set sourceImg as new camera/video frame
	if (bcamera)
	{
	    #ifdef TARGET_WIN32
			getPixels();
 		#else
            sourceImg.setFromPixels(vidGrabber->getPixels(), camWidth, camHeight);
 			//convert to grayscale
 			processedImg = sourceImg;
 		#endif
	}
	else
	{
		sourceImg.setFromPixels(vidPlayer->getPixels(), camWidth, camHeight);
		//convert to grayscale
		processedImg = sourceImg;
	}
}

//Grab frame from GPU
void ofxNCoreVision::grabFrameToGPU(GLuint target)
{
	//grab the frame to a raw openGL texture
	if (bcamera)
	{
		glEnable(GL_TEXTURE_2D);
		//glPixelStorei(1);
		glBindTexture(GL_TEXTURE_2D, target);

		#ifdef TARGET_WIN32
			if(PS3!=NULL)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, PS3->getPixels());
			}
			else if(vidGrabber!=NULL)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, vidGrabber->getPixels());
			}
			else if(dsvl!=NULL)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, dsvl->getPixels());
			}
		#else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, vidGrabber->getPixels());
		#endif
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D,0);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, target);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, camWidth, camHeight, GL_RGB, GL_UNSIGNED_BYTE, vidPlayer->getPixels());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D,0);
	}
}


/******************************************************************************
* The draw function paints the textures onto the screen. It runs after update.
*****************************************************************************/
void ofxNCoreVision::_draw(ofEventArgs &e)
{
	if (showConfiguration) 
	{
		//if calibration
		if (bCalibration)
		{
			//Don't draw main interface
			calib.passInContourFinder(contourFinder.nBlobs, contourFinder.blobs);
			calib.doCalibration();
		}
		//if mini mode
		else if (bMiniMode)
		{
			drawMiniMode();
		}
		//if full mode
		else if (bShowInterface)
		{
			drawFullMode();
			if(bDrawOutlines || bShowLabels) drawFingerOutlines();

			if(contourFinder.bTrackObjects && isSelecting)
			{	
				ofNoFill();
				ofSetColor(255, 0, 0);
				ofRect(rect.x,rect.y,rect.width,rect.height);
				ofSetColor(0, 255, 0);
				ofRect(minRect.x,minRect.y,minRect.width, minRect.height);
				ofSetColor(0, 0, 255);
				ofRect(maxRect.x, maxRect.y, maxRect.width, maxRect.height);
			}
		}

		//draw gui controls
		if (!bCalibration && !bMiniMode) {controls->draw();}
	}	
}

void ofxNCoreVision::drawFullMode()
{
	ofSetColor(0xFFFFFF);
	//Draw Background Image
	background.draw(0, 0);
	//Draw arrows
	ofSetColor(187, 200, 203);
	ofFill();
	//ofTriangle(680, 420, 680, 460, 700, 440);
	//ofTriangle(70, 420, 70, 460, 50, 440);
	//ofSetColor(255, 255, 0);

	ofSetColor(0xFFFFFF);
	//Draw Image Filters To Screen
	if (bGPUMode) filter->drawGPU();
	else filter->draw();

	ofSetColor(0x969696);

	//bigvideo.drawString("Source", 168, 20);
	bigvideo.drawString("Tracked Blobs", 168, 20);
	//bigvideo.drawString("Tracked", 509, 20);

	//draw link to tbeta website
	//ofSetColor(79, 79, 79);
	ofSetColor(0, 0, 0);
	ofFill();
	ofRect(366, 584, 150, 16);
	ofSetColor(0xFFFFFF);
	ofDrawBitmapString(" AVATAR Server", 366, 596);

	//Display Application information in bottom right
	string str = "Calculation Time: ";
	str+= ofToString(differenceTime, 0)+" ms \n\n";

	if (bcamera)
	{
		string str2 = "Camera Resolution: ";
        str2+= ofToString(camWidth, 0) + " x " + ofToString(camHeight, 0)  + "\n";
		string str4 = "Camera Framerate: ";
		str4+= ofToString(fps, 0)+" FPS \n";
		ofSetColor(0xFFFFFF);
		verdana.drawString(str + str2 + str4, 41, 420);
	}
	else
	{
		string str2 = "Video Resolution: ";
		str2+= ofToString(vidPlayer->width, 0) + " x " + ofToString(vidPlayer->height, 0)  + "\n";
		string str4 = "Video Framerate: ";
		str4+= ofToString(fps, 0)+" FPS \n";
		ofSetColor(0xFFFFFF);
		verdana.drawString(str + str2 + str4, 41, 480);
	}

	if (bTUIOMode)
	{
		//Draw Port and IP to screen
		ofSetColor(0xffffff);
		char buf[256];
		if(myTUIO.bOSCMode)
			sprintf(buf, "Sending TUIO messages to:\nHost: %s\nPort: %i", myTUIO.localHost, myTUIO.TUIOPort);
		else if(myTUIO.bTCPMode)
		{
			if(myTUIO.bIsConnected)
				//sprintf(buf, "Sending XML messages to:\nPort: %i", myTUIO.TUIOFlashPort);
				sprintf(buf, "Sending XML messages to:\nHost: %s\nPort: %i", myTUIO.localHost, myTUIO.TUIOFlashPort);
			else
				sprintf(buf, "Could not bind or send TCP to:\nPort: %i", myTUIO.TUIOFlashPort);
		}
		else if(myTUIO.bBinaryMode)
		{
			if(myTUIO.bIsConnected)
				//sprintf(buf, "Sending XML messages to:\nPort: %i", myTUIO.TUIOFlashPort);
				sprintf(buf, "Sending BINARY messages to:\nHost: %s\nPort: %i", myTUIO.localHost, myTUIO.TUIOFlashPort);
			else
				sprintf(buf, "Could not bind or send TCP to:\nPort: %i", myTUIO.TUIOFlashPort);
		}
		verdana.drawString(buf, 41, 535);
	}
	ofSetColor(0xFF0000);
	verdana.drawString("Press spacebar for mini mode", 356, 572);
}

void ofxNCoreVision::drawMiniMode()
{
	//black background
	ofSetColor(0,0,0);
	ofRect(0,0,ofGetWidth(), ofGetHeight());
	//draw outlines
	if (bDrawOutlines)
	{
		for (int i=0; i<contourFinder.nBlobs; i++)
		{
			contourFinder.blobs[i].drawContours(0,0, camWidth, camHeight+175, ofGetWidth(), ofGetHeight());
		}
		for (int i=0;i<contourFinder.nObjects; i++)
		{
			contourFinder.objects[i].drawContours(0,0, camWidth, camHeight+175, ofGetWidth(), ofGetHeight());
		}
	}

	//draw grey rectagles for text information
	ofSetColor(128,128,128);
	ofFill();
	ofRect(0,ofGetHeight() - 83, ofGetWidth(), 20);
	ofRect(0,ofGetHeight() - 62, ofGetWidth(), 20);
	ofRect(0,ofGetHeight() - 41, ofGetWidth(), 20);
	ofRect(0,ofGetHeight() - 20, ofGetWidth(), 20);

	//draw text
	ofSetColor(250,250,250);
	verdana.drawString("Calc. Time  [ms]:        " + ofToString(differenceTime,0),10, ofGetHeight() - 70 );
	if (bcamera)
	{
		verdana.drawString("Camera [fps]:            " + ofToString(fps,0),10, ofGetHeight() - 50 );
	}
	else 
	{
		verdana.drawString("Video [fps]:              " + ofToString(fps,0),10, ofGetHeight() - 50 );
	}
	verdana.drawString("Blob Count:               " + ofToString(contourFinder.nBlobs,0),10, ofGetHeight() - 29 );
	verdana.drawString("Communication:  " ,10, ofGetHeight() - 9 );

	//draw green tuio circle
	if((myTUIO.bIsConnected || myTUIO.bOSCMode) && bTUIOMode)
		ofSetColor(0x00FF00);	//green = connected
	else
		ofSetColor(0xFF0000);	//red = not connected
	ofFill();
	ofCircle(ofGetWidth() - 17 , ofGetHeight() - 10, 5);
	ofNoFill();
}

void ofxNCoreVision::drawFingerOutlines()
{
	//Find the blobs for drawing
	if(contourFinder.bTrackFingers)
	{
		for (int i=0; i<contourFinder.nBlobs; i++)
		{
			if (bDrawOutlines)
			{
				//Draw contours (outlines) on the source image
				contourFinder.blobs[i].drawContours(40, 30, camWidth, camHeight, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
			}
			if (bShowLabels) //Show ID label;
			{
				float xpos = contourFinder.blobs[i].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
				float ypos = contourFinder.blobs[i].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
	
				ofSetColor(0xCCFFCC);
				char idStr[1024];

				sprintf(idStr, "id: %i", contourFinder.blobs[i].id);

				verdana.drawString(idStr, xpos + 40, ypos + contourFinder.blobs[i].boundingRect.height/2 + 45);
			}
		}
	}
	//Object Drawing
	if(contourFinder.bTrackObjects)
	{
		for (int i=0; i<contourFinder.nObjects; i++)
		{
			if (bDrawOutlines)
			{
				//Draw contours (outlines) on the source image
				contourFinder.objects[i].drawBox(40, 30, camWidth, camHeight, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
			}
			if (bShowLabels) //Show ID label;
			{
				float xpos = contourFinder.objects[i].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
				float ypos = contourFinder.objects[i].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
	
				ofSetColor(0xCCFFCC);
				char idStr[1024];

				sprintf(idStr, "id: %i", contourFinder.objects[i].id);

				verdana.drawString(idStr, xpos + 365, ypos + contourFinder.objects[i].boundingRect.height/2 + 45);
			}
		}
	}


	ofSetColor(0xFFFFFF);
}

/*****************************************************************************
* KEY EVENTS
*****************************************************************************/
void ofxNCoreVision::_keyPressed(ofKeyEventArgs &e)
{
	if (showConfiguration)
	{
		switch (e.key)
		{
		case 'a':
			filter->threshold++;
			controls->update(appPtr->trackedPanel_threshold, kofxGui_Set_Int, &appPtr->filter->threshold, sizeof(int));
			break;
		case 'z':
			filter->threshold--;
			controls->update(appPtr->trackedPanel_threshold, kofxGui_Set_Int, &appPtr->filter->threshold, sizeof(int));
			break;
		case 'b':
			filter->bLearnBakground = true;
			break;
			
		case 'o':
			bDrawOutlines ? bDrawOutlines = false : bDrawOutlines = true;
			controls->update(appPtr->trackedPanel_outlines, kofxGui_Set_Bool, &appPtr->bDrawOutlines, sizeof(bool));
			break;
		case 'h':
			filter->bHorizontalMirror ? filter->bHorizontalMirror = false : filter->bHorizontalMirror = true;
			controls->update(appPtr->propertiesPanel_flipH, kofxGui_Set_Bool, &appPtr->filter->bHorizontalMirror, sizeof(bool));
			break;
		case 'j':
			filter->bVerticalMirror ? filter->bVerticalMirror = false : filter->bVerticalMirror = true;
			controls->update(appPtr->propertiesPanel_flipV, kofxGui_Set_Bool, &appPtr->filter->bVerticalMirror, sizeof(bool));
			break;
		case 't':
			myTUIO.bOSCMode = !myTUIO.bOSCMode;
			myTUIO.bTCPMode = false;
			myTUIO.bBinaryMode = false;
			bTUIOMode = myTUIO.bOSCMode;
			controls->update(appPtr->optionPanel_tuio_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bTCPMode, sizeof(bool));
			controls->update(appPtr->optionPanel_tuio_osc, kofxGui_Set_Bool, &appPtr->myTUIO.bOSCMode, sizeof(bool));
			controls->update(appPtr->optionPanel_bin_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bBinaryMode, sizeof(bool));
			//clear blobs
//			myTUIO.blobs.clear();
			break;
		case 'f':
			myTUIO.bOSCMode = false;
			myTUIO.bTCPMode = !myTUIO.bTCPMode;
			myTUIO.bBinaryMode = false;
			bTUIOMode = myTUIO.bTCPMode;
			controls->update(appPtr->optionPanel_tuio_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bTCPMode, sizeof(bool));
			controls->update(appPtr->optionPanel_tuio_osc, kofxGui_Set_Bool, &appPtr->myTUIO.bOSCMode, sizeof(bool));
			controls->update(appPtr->optionPanel_bin_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bBinaryMode, sizeof(bool));
			//clear blobs
//			myTUIO.blobs.clear();
			break;
		case 'n':
			myTUIO.bOSCMode = false;
			myTUIO.bTCPMode = false;
			myTUIO.bBinaryMode = !myTUIO.bBinaryMode;
			bTUIOMode = myTUIO.bBinaryMode;
			controls->update(appPtr->optionPanel_tuio_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bTCPMode, sizeof(bool));
			controls->update(appPtr->optionPanel_tuio_osc, kofxGui_Set_Bool, &appPtr->myTUIO.bOSCMode, sizeof(bool));
			controls->update(appPtr->optionPanel_bin_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bBinaryMode, sizeof(bool));
			//clear blobs
			//			myTUIO.blobs.clear();
			break;
		case 'g':
			bGPUMode ? bGPUMode = false : bGPUMode = true;
			controls->update(appPtr->gpuPanel_use, kofxGui_Set_Bool, &appPtr->bGPUMode, sizeof(bool));
			filter->bLearnBakground = true;
			break;
		case 'v':
			if (bcamera && vidGrabber != NULL)
				if(PS3)
					PS3->showSettings();
				else if(vidGrabber)
					vidGrabber->videoSettings();
			break;
		case 'l':
			bShowLabels ? bShowLabels = false : bShowLabels = true;
			controls->update(appPtr->trackedPanel_ids, kofxGui_Set_Bool, &appPtr->bShowLabels, sizeof(bool));
			break;
		case 'p':
			bShowPressure ? bShowPressure = false : bShowPressure = true;
			break;
		case ' ':
			if (bMiniMode && !bCalibration) // NEED TO ADD HERE ONLY GO MINI MODE IF NOT CALIBRATING
			{
				bMiniMode = false;
				bShowInterface = true;
				filter->bMiniMode = bMiniMode;
				ofSetWindowShape(800,600); //default size
			}
			else if(!bCalibration)
			{
				bMiniMode = true;
				bShowInterface = false;
				filter->bMiniMode = bMiniMode;
				ofSetWindowShape(190,200); //minimized size
			}
			break;
		case 'x': //Exit Calibrating
			if (bCalibration)
			{	bShowInterface = true;
			bCalibration = false;
			calib.calibrating = false;
			tracker.isCalibrating = false;
			if (bFullscreen == true) ofToggleFullscreen();
			bFullscreen = false;
			}
			break;
		case OF_KEY_RETURN: //Close Template Selection and save it
			if( contourFinder.bTrackObjects && isSelecting )
			{
			isSelecting = false;
			templates.addTemplate(rect,minRect,maxRect,camWidth/320,camHeight/240);
			rect = ofRectangle();
			minRect = rect;
			maxRect = rect;
			minTempArea = 0;
			maxTempArea = 0;
			controls->update(appPtr->TemplatePanel_minArea, kofxGui_Set_Bool, &appPtr->minTempArea, sizeof(float));
			controls->update(appPtr->TemplatePanel_maxArea, kofxGui_Set_Bool, &appPtr->maxTempArea, sizeof(float));
			}
			break;
		default: //Check key character <<<<===== Remove this
			//printf("%c",e.key);
			break;
		}
	}
}

void ofxNCoreVision::_keyReleased(ofKeyEventArgs &e)
{
	if (showConfiguration)
	{
		if ( e.key == 'c' && !bCalibration)
		{
			bShowInterface = false;
			// Enter/Exit Calibration
			bCalibration = true;
			calib.calibrating = true;
			tracker.isCalibrating = true;
			if (bFullscreen == false) ofToggleFullscreen();
			bFullscreen = true;
		}
	}
	if ( e.key == '~' || e.key == '`' && !bMiniMode && !bCalibration) showConfiguration = !showConfiguration;
}

/*****************************************************************************
*	MOUSE EVENTS
*****************************************************************************/
void ofxNCoreVision::_mouseDragged(ofMouseEventArgs &e)
{
	if (showConfiguration)
		controls->mouseDragged(e.x, e.y, e.button); //guilistener
	if(contourFinder.bTrackObjects)
	{
		if( e.x > 385 && e.x < 705 && e.y > 30 && e.y < 270 )
		{
			if( e.x < rect.x || e.y < rect.y )
			{
				rect.width = rect.x - e.x;
				rect.height = rect.y - e.y;
				
				rect.x = e.x;
				rect.y =  e.y;
			}
			else
			{
				rect.width = e.x - rect.x;
				rect.height = e.y - rect.y;
			}
		}
	}
}

void ofxNCoreVision::_mousePressed(ofMouseEventArgs &e)
{
	if (showConfiguration)
	{
		controls->mousePressed( e.x, e.y, e.button ); //guilistener
		if ( contourFinder.bTrackObjects )
		{
			if ( e.x > 385 && e.x < 705 && e.y > 30 && e.y < 270 )
			{
				isSelecting = true;
				rect.x = e.x;
				rect.y = e.y;
				rect.width = 0;
				rect.height = 0;
			}
		}
		//Launch the website in browser
		if ( e.x > 366 && e.x <386 && e.y > 586 )
		ofLaunchBrowser("http://gstanwar.blogspot.in");
	}
}

void ofxNCoreVision::_mouseReleased(ofMouseEventArgs &e)
{
	if (showConfiguration)
		controls->mouseReleased(e.x, e.y, 0); //guilistener
	if( e.x > 385 && e.x < 705 && e.y > 30 && e.y < 270 )
	{
		if	( contourFinder.bTrackObjects && isSelecting )
		{
			minRect = rect;
			maxRect = rect;
		}
	}
}

/*****************************************************************************
* Getters
*****************************************************************************/

std::map<int, Blob> ofxNCoreVision::getBlobs()
{
	return tracker.getTrackedBlobs();
}
/*
std::map<int,Blob> ofxNCoreVision::getObjects()
{
	return tracker.getTrackedObjects();
}
*/
/*****************************************************************************
* ON EXIT
*****************************************************************************/
void ofxNCoreVision::_exit(ofEventArgs &e)
{
	saveSettings();

	//Save templates
	if(contourFinder.bTrackObjects)
		templates.saveTemplateXml();

	// AlexP
	// C++ guarantees that operator delete checks its argument for null-ness
    #ifdef TARGET_WIN32
		delete PS3;		PS3 = NULL;
		delete ffmv; 	ffmv = NULL;
		delete dsvl;	dsvl = NULL;
	#endif
	delete filter;		filter = NULL;
	delete vidGrabber;	vidGrabber = NULL;
	delete vidPlayer;	vidPlayer = NULL;
	// -------------------------------- SAVE STATE ON EXIT
	printf("Vision module has exited!\n");
}

