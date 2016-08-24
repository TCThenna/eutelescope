#include "EUTelProcessorTrueHitAnalysis.h"

#include "EUTELESCOPE.h"
#include "EUTelExceptions.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelEventImpl.h"
#include "EUTelGeometryTelescopeGeoDescription.h"
#include "EUTelHistogramManager.h"
#include "EUTelUtility.h"

#include "marlin/Processor.h"
#include "marlin/AIDAProcessor.h"
#include "marlin/Exceptions.h"

#include <AIDA/IHistogramFactory.h>
#include <AIDA/IHistogram1D.h>
#include <AIDA/IHistogram2D.h>
#include <AIDA/ITree.h>

#include <UTIL/CellIDDecoder.h>
#include <IMPL/TrackerHitImpl.h>
#include <IMPL/LCCollectionVec.h>

#include <cmath>
#include <utility>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <map>

using namespace lcio;
using namespace marlin;
using namespace eutelescope;

EUTelProcessorTrueHitAnalysis::EUTelProcessorTrueHitAnalysis() :
	Processor("EUTelProcessorTrueHitAnalysis"),
	_trueHitCollectionName(""),
	_reconstructedHitCollectionName(""),
	_rawDataCollectionName("original_zsdata"),
	_iRun(0),
	_iEvt(0),
	_isFirstEvent(true),
	_histoInfoFileName(""),
	_noOfDetector(0),
	_sensorIDVec(),
	_trueHitCollectionVec(nullptr),
	_reconstructedHitCollectionVec(nullptr),
	_rawDataCollectionVec(nullptr),
	_1DHistos()
	/*_xDiffClusterSize1Histos(),
	_yDiffClusterSize1Histos(),
	_xDiffClusterSize2Histos(),
	_yDiffClusterSize2Histos(),
	_xDiffClusterSize3Histos(),
	_yDiffClusterSize3Histos(),
	_xDiffClusterSize4upHistos(),
	_yDiffClusterSize4upHistos()*/
{

	_description = "EUTelProcessorTrueHitAnalysis compares the true simulated hits with the reconstructed hits.";

	registerInputCollection(LCIO::TRACKERHIT, "TrueHitCollectionName", "Input of True Hit data", _trueHitCollectionName, std::string("true_hits"));

	registerInputCollection(LCIO::TRACKERHIT, "ReconstructedHitCollectionName", "Input of Reconstructed Hit data", _reconstructedHitCollectionName, std::string("hit"));

	registerProcessorParameter("HistoInfoFileName", "This is the name of the histogram information file", _histoInfoFileName, std::string("histoinfo.xml"));
}

void EUTelProcessorTrueHitAnalysis::init() {

	geo::gGeometry().initializeTGeoDescription(EUTELESCOPE::GEOFILENAME, EUTELESCOPE::DUMPGEOROOT);
	_sensorIDVec = geo::gGeometry().sensorIDsVec();

	printParameters();

	_iRun = 0;
	_iEvt = 0;
}

void EUTelProcessorTrueHitAnalysis::processRunHeader(LCRunHeader* rhdr) {

	std::unique_ptr<EUTelRunHeaderImpl> runHeader(new EUTelRunHeaderImpl(rhdr));
	runHeader->addProcessor(type());

	++_iRun;
}

void EUTelProcessorTrueHitAnalysis::readCollections(LCEvent* event) {

	try {

		_trueHitCollectionVec = dynamic_cast<LCCollectionVec*>(event->getCollection(_trueHitCollectionName));
		streamlog_out(DEBUG4) << "_trueHitCollectionVec: " << _trueHitCollectionName.c_str() << " found\n";
	}
	catch (lcio::DataNotAvailableException& e) {

		streamlog_out(DEBUG4) << "_trueHitCollectionVec: " << _trueHitCollectionName.c_str() << " not found in event " << event->getEventNumber() << std::endl;
	}

	try {

		_reconstructedHitCollectionVec = dynamic_cast<LCCollectionVec*>(event->getCollection(_reconstructedHitCollectionName));
		streamlog_out(DEBUG4) << "_reconstructedHitCollectionVec: " << _reconstructedHitCollectionName.c_str() << "found\n";
	}
	catch (lcio::DataNotAvailableException& e) {

		streamlog_out(DEBUG4) << "_reconstructedHitCollectionVec: " << _reconstructedHitCollectionName.c_str() << " not found in event " << event->getEventNumber() << std::endl;
	}
	try {

		_rawDataCollectionVec = dynamic_cast<LCCollectionVec*>(event->getCollection(_rawDataCollectionName));
		streamlog_out(DEBUG4) << "_rawDataCollectionVec: " << _rawDataCollectionName.c_str() << "found\n";
	}
	catch (lcio::DataNotAvailableException& e) {

		streamlog_out(DEBUG4) << "_rawDataCollectionVec: " << _rawDataCollectionName.c_str() << " not found in event " << event->getEventNumber() << std::endl;
	}
}

void EUTelProcessorTrueHitAnalysis::processEvent(LCEvent* event) {

	++_iEvt;

	readCollections(event);

	if (isFirstEvent()) bookHistos();

	EUTelEventImpl* eutelEvent = static_cast<EUTelEventImpl*>(event);
	if (eutelEvent->getEventType() == kEORE) {

		streamlog_out(DEBUG4) << "EORE found: nothing else to do." << std::endl;
		return;
	}
	else if (eutelEvent->getEventType() == kUNKNOWN) {

		streamlog_out(WARNING2) << "Event number " << eutelEvent->getEventNumber() << " is of unknown type. Continue considering it as a normal Data Event." << std::endl;
	}

	fillHistos(event);

	_isFirstEvent = false;
}

void EUTelProcessorTrueHitAnalysis::check (LCEvent*) {
}

void EUTelProcessorTrueHitAnalysis::end() {

	streamlog_out(MESSAGE4) << "Successfully finished\n";
}

/*std::vector<std::pair<double const*, double const*>> EUTelProcessorTrueHitAnalysis::pairHits(std::vector<double const*>  in_a, std::vector<double const*>  in_b) {

	std::vector<std::pair<double const*, double const*>> out;

	for (size_t i = 0; i < in_a.size(); i++) {

		int min_dist_index = 0;
		double min_distance = sqrt(pow(in_a[i][0] - in_b[0][0], 2) + pow(in_a[i][1] - in_b[0][1], 2));

		for (size_t j = 1; j < in_b.size(); j++) {

			if (sqrt(pow(in_a[i][0] - in_b[j][0], 2) + pow(in_a[i][1] - in_b[j][1], 2)) < min_distance) min_dist_index = j;
		}

		out.push_back(std::pair<double const*, double const*>(in_a[i], in_b[min_dist_index]));

		in_b.erase(in_b.begin()+min_dist_index);
	}

	return std::move(out);
}*/

int EUTelProcessorTrueHitAnalysis::findPairIndex(double const* a, std::vector<double const*> vec) {

	if (vec.size() == 1) return 0;
	else {

		int min_dist_index = 0;
		double min_distance = sqrt(pow(a[0] - vec[0][0], 2) + pow(a[1] - vec[0][1], 2));

		for (size_t i = 1; i < vec.size(); i++) {

			double current_distance = sqrt(pow(a[0] - vec[i][0], 2) + pow(a[1] - vec[i][1], 2));

			if (current_distance < min_distance) {

				min_dist_index = i;
				min_distance = current_distance;
			}
		}

		return min_dist_index;
	}
}


void EUTelProcessorTrueHitAnalysis::fillHistos(LCEvent* event) {

	EUTelEventImpl* eutelEvent = static_cast<EUTelEventImpl*>(event);
	EventType type = eutelEvent->getEventType();

	if (type == kEORE) {

		streamlog_out(DEBUG4) << "EORE found: nothing else to do." << std::endl;
		return;
	}
	else if (type == kUNKNOWN) {
		// if it is unknown we had already issued a warning to the user at
		// the beginning of the processEvent. If we get to here, it means
		// that the assumption that the event was a data event was
		// correct, so no harm to continue...
	}

	try {

		LCCollectionVec* _trueHitCollectionVec = dynamic_cast<LCCollectionVec*>(event->getCollection(_trueHitCollectionName));
		CellIDDecoder<TrackerHitImpl> trueHitDecoder(_trueHitCollectionVec);
		LCCollectionVec* _reconstructedHitCollectionVec = dynamic_cast<LCCollectionVec*>(event->getCollection(_reconstructedHitCollectionName));
		CellIDDecoder<TrackerHitImpl> reconstructedHitDecoder(_reconstructedHitCollectionVec);
		LCCollectionVec* _rawDataCollectionVec = dynamic_cast<LCCollectionVec*>(event->getCollection(_rawDataCollectionName));
		CellIDDecoder<TrackerDataImpl> rawDataDecoder(_rawDataCollectionVec);

		std::map<int, std::vector<double const*>> trueHitMap;
		//std::map<int, std::vector<double const*>> reconstructedHitMap;

		for (int i = 0; i < _trueHitCollectionVec->getNumberOfElements(); i++) {

			TrackerHitImpl* trueHit = dynamic_cast<TrackerHitImpl*>(_trueHitCollectionVec->getElementAt(i));
			int detectorID = static_cast<int>(trueHitDecoder(trueHit)["sensorID"]);

			trueHitMap.insert(std::make_pair(detectorID, std::vector<double const*>()));
			trueHitMap.at(detectorID).push_back(trueHit->getPosition());
		}

		for (int i = 0; i < _reconstructedHitCollectionVec->getNumberOfElements(); i++) {

			TrackerHitImpl* reconstructedHit = dynamic_cast<TrackerHitImpl*>(_reconstructedHitCollectionVec->getElementAt(i));
			int detectorID = static_cast<int>(reconstructedHitDecoder(reconstructedHit)["sensorID"]);

			TrackerDataImpl* zsData = dynamic_cast<TrackerDataImpl*>(reconstructedHit->getRawHits().front());

			int pixelType = static_cast<int>(rawDataDecoder(zsData)["sparsePixelType"]);
			int pixelEntries;
			if (pixelType == 1) pixelEntries = 3;//EUTelSimpleSparsePixel
			else if (pixelType == 2) pixelEntries = 4;//EUTelGenericSparsePixel
			else if (pixelType == 3) pixelEntries = 8;//EUTelGeometricPixel
			else if (pixelType == 4) pixelEntries = 7;//EUTelMuPixel
			else {

				streamlog_out(ERROR3) << "Unrecognized pixel type in reconstructed hit raw data";
				return;
			}

			int clusterSize = static_cast<int>(zsData->getChargeValues().size()/pixelEntries);
			if (clusterSize > 4) clusterSize = 4;

			double const* hitPos = reconstructedHit->getPosition();
			int pairIndex = findPairIndex(hitPos, static_cast<std::vector<double const*>>(trueHitMap.at(detectorID)));
			double const* pair = (static_cast<std::vector<double const*>>(trueHitMap.at(detectorID)))[pairIndex];

			double diff_x = hitPos[0] - pair[0];
			double diff_y = hitPos[1] - pair[1];

			(dynamic_cast<AIDA::IHistogram1D*>(_1DHistos[2*(clusterSize-1)].at(detectorID)))->fill(diff_x);
			(dynamic_cast<AIDA::IHistogram1D*>(_1DHistos[2*clusterSize-1].at(detectorID)))->fill(diff_y);

			//delete hitPos;
			//delete [] pair;

			//reconstructedHitMap.at(detectorID).push_back(reconstructedHit->getPosition());
		}

		/*for (auto& detectorID: trueHitMap) {

			for (size_t i = 0; i < (static_cast<std::vector<double const*>>(trueHitMap.at(detectorID.first))).size(); i++) {

				delete [] (static_cast<std::vector<double const*>>(trueHitMap.at(detectorID.first)))[i];
			}
		}*/

		/*for (auto& detectorID: trueHitMap) {

			std::vector<std::pair<double const*, double const*>> hitPairs = pairHits(static_cast<std::vector<double const*>>(trueHitMap.at(detectorID.first)), static_cast<std::vector<double const*>>(reconstructedHitMap.at(detectorID.first)));

			for (size_t i = 0; i < hitPairs.size(); i++) {

				double diff_x = hitPairs[i].first[0] - hitPairs[i].second[0];
				double diff_y = hitPairs[i].first[1] - hitPairs[i].second[1];

				TrackerDataImpl* zsData = dynamic_cast<TrackerDataImpl*>(hitPairs.second
				int clusterSize 
				

				(dynamic_cast<AIDA::IHistogram1D*>(_1DHistos[0].at(detectorID.first)))->fill(diff_x);
				(dynamic_cast<AIDA::IHistogram1D*>(_1DHistos[1].at(detectorID.first)))->fill(diff_y);
			}
		}*/
	}
	catch (lcio::DataNotAvailableException& e) {

		return;
	}

	return;
}

void EUTelProcessorTrueHitAnalysis::bookHistos() {

	streamlog_out(DEBUG5) << "Booking histograms\n";

	std::unique_ptr<EUTelHistogramManager> histoMng = std::make_unique<EUTelHistogramManager>(_histoInfoFileName);
	EUTelHistogramInfo* histoInfo;
	bool isHistoManagerAvailable;

	try {
		isHistoManagerAvailable = histoMng->init();
	}
	catch ( std::ios::failure& e) {
		streamlog_out ( WARNING2 ) << "I/O problem with " << _histoInfoFileName << "\n"
                               << "Continuing without histogram manager"  << std::endl;
		isHistoManagerAvailable = false;
	}
	catch ( ParseException& e ) {
		streamlog_out ( WARNING2 ) << e.what() << "\n"
                               << "Continuing without histogram manager" << std::endl;
		isHistoManagerAvailable = false;
	}

	/*std::string _xDiffClusterSize1HistoName = "xPositionDifferenceClusterSize1";
	std::string _xDiffClusterSize1HistoName = "yPositionDifferenceClusterSize1";
	std::string _xDiffClusterSize2HistoName = "xPositionDifferenceClusterSize2";
	std::string _yDiffClusterSize2HistoName = "yPositionDifferenceClusterSize2";
	std::string _xDiffClusterSize3HistoName = "xPositionDifferenceClusterSize3";
	std::string _yDiffClusterSize3HistoName = "yPositionDifferenceClusterSize3";
	std::string _xDiffClusterSize4upHistoName = "xPositionDifferenceClusterSize4up";
	std::string _yDiffClusterSize4upHistoName = "yPositionDifferenceClusterSize4up";

	std::string tempHistoName;
	std::string basePath;*/

	for (size_t i = 0; i < _sensorIDVec.size(); i++) {

		int sensorID = _sensorIDVec[i];
		std::string basePath = "detector_" + to_string(sensorID);

		for (size_t i = 0; i < _1DHistos.size(); i++) {

			std::string coordinate = (i%2 == 0)?"x":"y";
			std::string clusterSize = (i+3>_1DHistos.size())?to_string(i/2+1)+"+":to_string(i/2+1);

			std::string histoName = coordinate + "PositionDifferenceClusterSize" + clusterSize;

			int histoNBin = 101;
			double histoMin = -0.2;
			double histoMax = 0.2;
			std::string histoTitle = "difference in " + coordinate + " position between true simulated hits and reconstructed hits for cluster size " + clusterSize;
			if (isHistoManagerAvailable) {

				histoInfo = histoMng->getHistogramInfo(histoName);

				if (histoInfo) {

					streamlog_out(DEBUG2) << (* histoInfo) << std::endl;
					histoNBin = histoInfo->_xBin;
					histoMin = histoInfo->_xMin;
					histoMax = histoInfo->_xMax;

					if (histoInfo->_title != "") histoTitle = histoInfo->_title;
				}
			}

			std::string tempHistoName = histoName + "_d" + to_string(sensorID);
                	_1DHistos[i].insert(std::make_pair(sensorID, AIDAProcessor::histogramFactory(this)->createHistogram1D((basePath+tempHistoName).c_str(), histoNBin, histoMin, histoMax)));
                	_1DHistos[i].at(sensorID)->setTitle(histoTitle.c_str());
		}

		/*tempHistoName = _xDiffHistoName + "_d" + to_string(sensorID);
		_xDiffHistos.insert(std::make_pair(sensorID, AIDAProcessor::histogramFactory(this)->createHistogram1D((basePath+tempHistoName).c_str(), xDiffNBin, xDiffMin, xDiffMax)));
		_xDiffHistos[sensorID]->setTitle(xDiffTitle.c_str());

		tempHistoName = _yDiffHistoName + "_d" + to_string(sensorID);
		_yDiffHistos.insert(std::make_pair(sensorID, AIDAProcessor::histogramFactory(this)->createHistogram1D((basePath+tempHistoName).c_str(), yDiffNBin, yDiffMin, yDiffMax)));
		_yDiffHistos[sensorID]->setTitle(yDiffTitle.c_str());*/
	}

	streamlog_out(DEBUG5) << "end of Booking histograms\n";
}
