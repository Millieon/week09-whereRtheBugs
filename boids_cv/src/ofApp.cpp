#include "ofApp.h"
using namespace ofxCv;
using namespace cv;

void ofApp::debugCameraDevices()
{
    // create a collection holding information about all available camera sources
    vector< ofVideoDevice > devices = cameraInput.listDevices();
    
    // iterate through camera source collection and print info for each item
    for(int i = 0; i < devices.size(); i++) {
        cout <<
        "Device ID: " << devices[i].id <<
        " Device Name: " << devices[i].deviceName <<
        " Hardware Name: " << devices[i].hardwareName <<
        " Is Available: " << devices[i].bAvailable <<
        endl;
    }
}

ofApp::~ofApp()
{
    for (int i = 0; i < boids.size(); i++)
    {
        delete boids[i];
    }
}

//--------------------------------------------------------------
void ofApp::setup()
{
//computer vision set up
    contour.setMinAreaRadius(10);
    contour.setMaxAreaRadius(100);
    colorImg.allocate(320,240);
    // initialize image properties
    imgWidth  = 640;
    imgHeight = 480;
    originalInputImg.allocate(imgWidth,imgHeight);
    detectionThreshold = 70;    // very high contrast
    detectedObjectMax  = 1;    // maximum of 10 detected object at a time
    contourMinArea     = 40;    // detect a wide range of different sized objects
    contourMaxArea     = (imgWidth * imgHeight) / 3;
    
    
    hsvImg.allocate(imgWidth, imgHeight);
    hueImg.allocate(imgWidth, imgHeight);
    saturationImg.allocate(imgWidth, imgHeight);
    valueImg.allocate(imgWidth, imgHeight);
    backgroundImg.allocate(imgWidth, imgHeight);
    bckgrndSatDiffImg.allocate(imgWidth, imgHeight);
    
    // initialize camera instance
    debugCameraDevices();   // print information about available camera sources
    cameraInput.setDeviceID(0);     // 0 -> default if at least once camera is available (get device id of other camera sources from running debugCameraDevices() )
    cameraInput.initGrabber(imgWidth, imgHeight); // OF version 0.9.0
    
    // initialize helper values
    labelPosDelta     = 14;
    blobOverlayRadius = 10;
    
    
    
    
    int screenW = ofGetScreenWidth();
    int screenH = ofGetScreenHeight();
    ofSetWindowPosition(screenW/2-300/2, screenH/2-300/2);
    
    //load our typeface
    vagRounded.loadFont("vag.ttf", 16);
    
    bFullscreen    = 0;
    
    //lets set the initial window pos
    //and background color
    //ofSetVerticalSync(true);
    ofSetFrameRate(60);
    ofSetWindowShape(640, 480);
    ofBackground(0,50,50);
    
    // set up the boids
    for (int i = 0; i < 256; i++)
        boids.push_back(new Boid());
    
    framecounter = 0;
    updatecounter = 0;
    
    for (int i = 0; i < boids.size(); i++)
    {
        
        boids[i]->setPosition(objectPos);
    }

    //This sample needs to be a 16-bit .wav file
    mySample.load(ofToDataPath("59210__suonho__abstract_electrofunkbreakbeats_124bpm_mono.wav"));

    int sampleRate = 44100; /* Sampling Rate */
    int bufferSize = 512;

    ofxMaxiSettings::setup(sampleRate, 2, bufferSize);

    // FFT size, window size, hop size
    myFFT.setup(512, 512, 128);

    // of Sound setup
    ofSoundStreamSettings settings;
    settings.setApi(ofSoundDevice::Api::MS_DS);
    auto devices = soundStream.getMatchingDevices("default");

    if (!devices.empty()) {
        settings.setInDevice(devices[0]);
    }

    settings.setInListener(this);
    settings.setOutListener(this);
    settings.sampleRate = sampleRate;
    settings.numOutputChannels = 2;
    settings.numInputChannels = 2;
    settings.bufferSize = bufferSize;
    soundStream.setup(settings);
}


//--------------------------------------------------------------
void ofApp::update(){
    
    // update (read) input from camera feed
    cameraInput.update();
    
    // check if a new frame from the camera source was received
    if (cameraInput.isFrameNew())
    {
        // read (new) pixels from camera input and write them to original input image instance
        //originalInputImg.setFromPixels(cameraInput.getPixels(), imgWidth, imgHeight);    // OF version 0.8.4
        originalCamInputImg.setFromPixels(cameraInput.getPixels());    // OF version 0.9.0
        
        // create HCV color space image based on original (RGB) received camera input image
        hsvImg = originalCamInputImg;
        hsvImg.convertRgbToHsv();
        
        // extract HSV color space channels into separate image instances
        hsvImg.convertToGrayscalePlanarImages(hueImg, saturationImg, valueImg);
        
        // take the absolute value of the difference between the registered background image and the updated saturation color channel image in order to determine the image parts that have changed
        bckgrndSatDiffImg.absDiff(backgroundImg, saturationImg);
        
        // increase the contrast of the image
        bckgrndSatDiffImg.threshold(detectionThreshold);
        
        // apply object detection via OpenCV contour finder class
        contourFinder.findContours(bckgrndSatDiffImg, contourMinArea, contourMaxArea, detectedObjectMax, false);

    }

    //update our window title with the framerate and the position of the window
    //[zach fix] ofSetWindowTitle(ofToString(ofGetFrameRate(), 2)+":fps - pos ("+ofToString((int)windowX)+","+ofToString((int)windowY)+")");
    
    
    if(bFullscreen){
        ofHideCursor();
    }else{
        ofShowCursor();
    }
    
    ofVec3f min(0, 0);
   ofVec3f max(ofGetWidth(), ofGetHeight());
 //   ofVec3f min(objectPos.x-200,objectPos.y-200);
//    ofVec3f max(objectPos.x+200,objectPos.y+200);
    for (int i = 0; i < boids.size(); i++)
    {
        
        boids[i]->update(boids, min, max);
        //boids[i]->setPosition(objectPos);
    }
    updatecounter += 1;
}

//--------------------------------------------------------------
void ofApp::draw()
{
    originalCamInputImg.draw(0 * imgWidth, 0 * imgHeight,imgWidth,imgHeight); // draw row
    framecounter += 1;
    ofSetupScreen();
    
    ofSetHexColor(0xffffffff);  // set color "white" in hexadecimal representation
    
    for (int i = 0; i < contourFinder.nBlobs; i++) {
        // access current blob
        contourFinder.blobs[i].draw(0 * imgWidth, 0 * imgHeight);   // draw current blob in bottom right image grid
        
        // extract RGB color from the center of the current blob based on original input image
        //
        
        // get pixel reference of original input image
        //ofPixels originalInputImagePxls = originalInputImg.getPixelsRef();    // OF version 0.8.4
        ofPixels originalInputImagePxls = originalCamInputImg.getPixels(); // OF version 0.9.0
        
        // get point reference to the center of the current detected blob
        ofPoint blobCenterPnt = contourFinder.blobs[i].centroid;
        
        // get color of pixel in the center of the detected blob
        //ofColor detectedBlobClr = originalInputImagePxls.getColor(blobCenterPnt.x, blobCenterPnt.y);          // OF version 0.9.0
        ofColor detectedBlobClr = originalInputImagePxls.getColor(blobCenterPnt.x / 3, blobCenterPnt.y / 3);    // OF version 0.9.6

        // apply detected color for drawing circle overlay
        ofSetColor(detectedBlobClr);
        ofFill();
        
        // draw circle overlay in bottom left image of the grid (ontop of a copy of the saturation image)
        // OF version 0.8.4
        /*ofCircle(blobCenterPnt.x + 0 * imgWidth,
                   blobCenterPnt.y + 2 * imgHeight,
                   blobOverlayRadius); */
        // OF version 0.9.0
        ofDrawCircle(blobCenterPnt.x + 0 * imgWidth,
                     blobCenterPnt.y + 2 * imgHeight,
                     blobOverlayRadius);
        
        objectPos.x=blobCenterPnt.x;
        objectPos.y=blobCenterPnt.y;


        ofDrawBitmapString("Potential target in vision", objectPos.x, objectPos.y);


        
    }
    

    for (int i = 0; i < boids.size(); i++)
    {
        boids[i]->draw();

        /// Uncomment this to use audio for drawing
        //boids[i]->draw(255, 255, 0, 255, myFFT.magnitudesDB[i]);
    }
    
    //    vagRounded.drawString("frame rate frame " + ofToString(framecounter/ofGetElapsedTimef()) + " update " + ofToString(updatecounter/ofGetElapsedTimef()), 10, 25);
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& output)
{
    std::size_t nChannels = output.getNumChannels();
    for (std::size_t i = 0; i < output.getNumFrames(); i++)
    {
        output[i * nChannels] = mySample.play();
        output[i * nChannels + 1] = output[i * nChannels];

        if (myFFT.process(output[i * nChannels]))
        {
            myFFT.magsToDB();
        }
    }
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& input)
{
    std::size_t nChannels = input.getNumChannels();
    for (std::size_t i = 0; i < input.getNumFrames(); ++i)
    {
        //handle input here
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if(key == 'f'){
        
        bFullscreen = !bFullscreen;
        
        if(!bFullscreen){
            ofSetWindowShape(300,300);
            ofSetFullscreen(false);
            // figure out how to put the window in the center:
            int screenW = ofGetScreenWidth();
            int screenH = ofGetScreenHeight();
            ofSetWindowPosition(screenW/2-300/2, screenH/2-300/2);
        } else if(bFullscreen == 1){
            ofSetFullscreen(true);
        }
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

