#include <windows.h>
#include <iostream>
#include <math.h>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osg/AnimationPath>
#include <osg/Matrix>
#include <osg/Material>
#include <osg/ShapeDrawable>
#include "raaInputController.h"
#include "raaAssetLibrary.h"
#include "raaFacarde.h"
#include "raaSwitchActivator.h"
#include "raaRoadTileFacarde.h"
#include "raaAnimationPointFinder.h"
#include "raaAnimatedFacarde.h"
#include "raaCarFacarde.h"
#include "raaTrafficSystem.h"
#include "TrafficLightControl.h"
#include "TrafficLightFacarde.h"


typedef std::vector<raaAnimationPointFinder>raaAnimationPointFinders;
osg::Group* g_pRoot = 0; // root of the sg
float g_fTileSize = 472.441f; // width/depth of the standard road tiles
std::string g_sDataPath = "../../Data/";

enum raaRoadTileType
{
	Normal,
	LitTJunction,
	LitXJunction,
};

void addRoadTile(std::string sAssetName, std::string sPartName, int xUnit, int yUnit, float fRot, osg::Group* pParent)
{
	raaFacarde* pFacarde = new raaRoadTileFacarde(raaAssetLibrary::getNamedAsset(sAssetName, sPartName), osg::Vec3(g_fTileSize * xUnit, g_fTileSize * yUnit, 0.0f), fRot);
	pParent->addChild(pFacarde->root());
}

osg::Node* buildAnimatedVehicleAsset()
{
	osg::Group* pGroup = new osg::Group();

	osg::Geode* pGB = new osg::Geode();
	osg::ShapeDrawable* pGeomB = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 0.0f), 100.0f, 60.0f, 40.0f));
	osg::Material* pMat = new osg::Material();
	pMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.3f, 0.3f, 0.1f, 1.0f));
	pMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.8f, 0.3f, 1.0f));
	pMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.6f, 1.0f));

	pGroup->addChild(pGB);
	pGB->addDrawable(pGeomB);

	pGB->getOrCreateStateSet()->setAttribute(pMat, osg::StateAttribute::ON || osg::StateAttribute::OVERRIDE);
	pGB->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), osg::StateAttribute::ON || osg::StateAttribute::OVERRIDE);

	return pGroup;
}



osg::AnimationPath* createAnimationPath(raaAnimationPointFinders apfs, osg::Group* pRoadGroup)
{
	float fAnimTime = 0.0f;
	osg::AnimationPath* ap = new osg::AnimationPath();

	for (int i = 0; i < apfs.size(); i++)
	{
		float fDistance = 0.0f;
		osg::Vec3 vs;
		osg::Vec3 ve;

		vs.set(apfs[i].translation().x(), apfs[i].translation().y(), apfs[i].translation().z());

		if (i == apfs.size() - 1)
			ve.set(apfs[0].translation().x(), apfs[0].translation().y(), apfs[0].translation().z());
		else
			ve.set(apfs[i + 1].translation().x(), apfs[i + 1].translation().y(), apfs[i + 1].translation().z());

		float fXSqr = pow((ve.x() - vs.x()), 2);
		float fYSqr = pow((ve.y() - vs.y()), 2);
		float fZSqr = pow((ve.z() - vs.z()), 2);

		fDistance = sqrt(fXSqr + fYSqr);
		ap->insert(fAnimTime, osg::AnimationPath::ControlPoint(apfs[i].translation(), apfs[i].rotation()));
		fAnimTime += (fDistance / 10.0f);
	}

	return ap;
}

void buildRoad(osg::Group* pRoadGroup)
{
	//starting tiles
	addRoadTile("roadStraight",	 "tile0", 0, 0, 0.0f, pRoadGroup);
	addRoadTile("roadTJunction",	"tile1", 1, 0, -90.0f, pRoadGroup);
	addRoadTile("roadStraight",	 "tile2", 1, 1, 90.0f, pRoadGroup);
	addRoadTile("roadXJunction",	 "tile3", 1, 2, 180.0f, pRoadGroup);
	addRoadTile("roadStraight",	 "tile4", 0, 2, 0.0f, pRoadGroup);
	addRoadTile("roadCurve",   	"tile5", -1, 2, -90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile6", -1, 1, -90.0f, pRoadGroup);
	addRoadTile("roadCurve",	"tile7", -1, 0, 0.0f, pRoadGroup);
	
	//my tiles
	addRoadTile("roadStraight", "tile8", 2, 0, 0.0f, pRoadGroup);
	addRoadTile("roadCurve",	 "tile9", 3, 0, -270.0f, pRoadGroup);
	addRoadTile("roadStraight",	 "tile10", 3, 1, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile11", 2, 2, 0.0f, pRoadGroup);
	addRoadTile("roadTJunction", "tile12", 3, 2, 0.0f, pRoadGroup);

	//road leading up
	addRoadTile("roadStraight", "tile13", 1, 3, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile14", 1, 4, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile15", 1, 5, 90.0f, pRoadGroup);
	addRoadTile("roadXJunction", "tile16", 1, 6, 0.0f, pRoadGroup);

	//top half of road
	addRoadTile("roadStraight", "tile17", 0, 6, 0.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile18", -1, 6, 0.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile19", -1, 7, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile20", -1, 8, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile21", -1, 9, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile22", 1, 7, 90.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile23", 1, 8, 270.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile24", -1, 10, 270.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile25", 0, 10, 0.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile26", 1, 10, 0.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile27", 2, 10, 0.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile28", 3, 10, 180.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile29", 0, 10, 0.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile30", 2, 8, 0.0f, pRoadGroup);
	addRoadTile("roadTJunction", "tile32", 3, 8, 0.0f, pRoadGroup); //top right junction
	addRoadTile("roadStraight", "tile33", 3, 9, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile34", 2, 6, 0.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile35", 3, 7, 90.0f, pRoadGroup);
	addRoadTile("roadTJunction", "tile36", 3, 6, 0.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile37", 3, 5, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile38", 3, 4, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile39", 3, 3, 90.0f, pRoadGroup);
	
}

void createCarOne(osg::Group* pRoadGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	//bottom half
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile6", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile5", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile3", 11, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 12, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 4, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile13", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile14", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile15", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile16", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 7, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile17", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile18", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile19", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile21", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile21", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile24", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile24", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile24", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile25", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile25", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile26", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile26", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile27", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile27", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile28", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile28", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile28", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile33", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile33", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile32", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile32", 6, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile32", 7, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile30", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile30", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile23", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile23", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile23", 2, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile22", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile22", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile16", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 14, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 10, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile34", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile34", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile36", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile36", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile36", 4, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile37", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile37", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile38", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile38", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile39", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile39", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile12", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 4, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile9", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));

	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car0"), ap, 90.0);
	g_pRoot->addChild(pCar->root());
}

void createCarTwo(osg::Group* pRoadGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	//bottom half
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile6", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile5", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile3", 11, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 7, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile11", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 0, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile12", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 4, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile9", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 5, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));

	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));

	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car1"), ap, 40.0);
	g_pRoot->addChild(pCar->root());
}

int main(int argc, char** argv)
{
	raaAssetLibrary::start();
	raaTrafficSystem::start();

	osgViewer::Viewer viewer;

	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]) == "-d") g_sDataPath = argv[++i];
	}

	// the root of the scene - use for rendering
	g_pRoot = new osg::Group();
	g_pRoot->ref();

	// build asset library - instances or clones of parts can be created from this
	raaAssetLibrary::loadAsset("roadStraight", g_sDataPath + "roadStraight.osgb");
	raaAssetLibrary::loadAsset("roadCurve", g_sDataPath + "roadCurve.osgb");
	raaAssetLibrary::loadAsset("roadTJunction", g_sDataPath + "roadTJunction.osgb");
	raaAssetLibrary::loadAsset("roadXJunction", g_sDataPath + "roadXJunction.osgb");
	raaAssetLibrary::loadAsset("trafficLight", g_sDataPath + "raaTrafficLight.osgb");
	raaAssetLibrary::insertAsset("vehicle", buildAnimatedVehicleAsset());

	// add a group node to the scene to hold the road sub-tree
	osg::Group* pRoadGroup = new osg::Group();
	g_pRoot->addChild(pRoadGroup);

	// Create road
	buildRoad(pRoadGroup);

	// Add car one
	createCarOne(pRoadGroup);
	//add car two
	createCarTwo(pRoadGroup);

	//Traffic Lights
	osg::Group* trafficLightGroup = new osg::Group();
	g_pRoot->addChild(trafficLightGroup);
	TrafficLightFacarde* tlFacarde1 = new TrafficLightFacarde(raaAssetLibrary::getClonedAsset("trafficLight", "trafficLight0"), osg::Vec3(-175.0f, 1125.0f, 0.0f), -90.0f, 0.08f);
	trafficLightGroup->addChild(tlFacarde1->root());
	TrafficLightFacarde* tlFacarde2 = new TrafficLightFacarde(raaAssetLibrary::getClonedAsset("trafficLight", "trafficLight1"), osg::Vec3(200.0f, 750.0f, 0.0f), 90.0f, 0.08f);
	trafficLightGroup->addChild(tlFacarde2->root());

	//Traffic Light Control
	TrafficLightControl* pJunction = new TrafficLightControl(raaAssetLibrary::getNamedAsset("roadStraight", "tile4"), osg::Vec3(0.0f, 945.0f, 0.0f), 0.0f, 1.0f);
	g_pRoot->addChild(pJunction->root());
	pJunction->addTrafficLight(tlFacarde1);
	pJunction->addTrafficLight(tlFacarde2);

	// osg setup stuff
	osg::GraphicsContext::Traits* pTraits = new osg::GraphicsContext::Traits();
	pTraits->x = 20;
	pTraits->y = 20;
	pTraits->width = 600;
	pTraits->height = 480;
	pTraits->windowDecoration = true;
	pTraits->doubleBuffer = true;
	pTraits->sharedContext = 0;

	osg::GraphicsContext* pGC = osg::GraphicsContext::createGraphicsContext(pTraits);
	osgGA::KeySwitchMatrixManipulator* pKeyswitchManipulator = new osgGA::KeySwitchMatrixManipulator();
	pKeyswitchManipulator->addMatrixManipulator('1', "Trackball", new osgGA::TrackballManipulator());
	pKeyswitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator());
	pKeyswitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator());
	viewer.setCameraManipulator(pKeyswitchManipulator);
	osg::Camera* pCamera = viewer.getCamera();
	pCamera->setGraphicsContext(pGC);
	pCamera->setViewport(new osg::Viewport(0, 0, pTraits->width, pTraits->height));

	// add own event handler - this currently switches on an off the animation points
	viewer.addEventHandler(new raaInputController(g_pRoot));

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the thread model handler
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the record camera path handler
	viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

	// add the LOD Scale handler
	viewer.addEventHandler(new osgViewer::LODScaleHandler);

	// add the screen capture handler
	viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);

	// set the scene to render
	viewer.setSceneData(g_pRoot);

	viewer.realize();

	return viewer.run();
}


